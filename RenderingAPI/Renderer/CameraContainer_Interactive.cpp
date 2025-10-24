//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "CameraContainer_Interactive.h"

// internal max includes
#include <interactiverender.h>

namespace Max
{;
namespace RenderingAPI
{;

CameraContainer_Interactive::CameraContainer_Interactive(
    const UnifiedRenderer& renderer, 
    const IRenderSettingsContainer& render_settings_container, 
    IImmediateInteractiveRenderingClient* notification_client,
    IIRenderMgr* pIIRenderMgr,
    const Box2& region,
    Bitmap* const bitmap,
    INode* const camera_node,
    const bool use_camera,
    const ViewExp* view_exp)

    : CameraContainer_Base(renderer, render_settings_container),
    m_notification_client(notification_client),
    m_is_running_in_active_shade_viewport(false),
    m_monitored_camera_node(nullptr)
{
    if(m_notification_client != nullptr)
    {
        m_notification_client->MonitorActiveShadeView(*this, nullptr, m_is_running_in_active_shade_viewport);
    }

    set_camera_node(camera_node);
    set_use_camera(use_camera);
    set_bitmap(bitmap);
    set_region(region);
    set_irender_mgr(pIIRenderMgr);

    if(view_exp != nullptr)
    {
        set_view(*view_exp);
    }
}

CameraContainer_Interactive::~CameraContainer_Interactive()
{
    if(m_notification_client != nullptr)
    {
        m_notification_client->StopMonitoringActiveView(*this, nullptr);
    }

    // Stop monitoring the camera node
    monitor_camera_node(nullptr);
}

void CameraContainer_Interactive::set_irender_mgr(IIRenderMgr* pIIRenderMgr)
{
	m_is_running_in_active_shade_viewport = (pIIRenderMgr != nullptr) && (std::wstring(pIIRenderMgr->GetName()) == _M("ReshadeManager")) && (pIIRenderMgr->GetDisplayStyle() == IImageViewer::IV_DOCKED);
}

void CameraContainer_Interactive::notify_change()
{
    // Ensure to monitor the new camera node
    monitor_camera_node(GetCameraNode());

    __super::notify_change();
}

void CameraContainer_Interactive::monitor_camera_node(INode* const node)
{
    if(node != m_monitored_camera_node)
    {
        // Stop monitoring the current node
        if(m_monitored_camera_node != nullptr)
        {
            m_notification_client->StopMonitoringNode(*m_monitored_camera_node, ~size_t(0), *this, nullptr);
        }

        // Monitor the new node
        if(node != nullptr)
        {
            m_notification_client->MonitorNode(*node, NotifierType_Node_Camera, ~size_t(0), *this, nullptr);
        }
        m_monitored_camera_node = node;
    }
}

void CameraContainer_Interactive::InteractiveRenderingCallback_NotifyEvent(const IViewEvent& view_event, void* /*userData*/)
{
    switch(view_event.GetEventType())
    {
    case EventType_View_Properties:
        // We get this notification whenever anything in the view change - not just the transform (e.g. the zoom level). So we need to fetch the entire
        // view parameters again.
        // Fall into...
    case EventType_View_Active:
        {
            // Ignore view changes when active shade is docked to a viewport - we consider the view to be locked in that case.
            if(!m_is_running_in_active_shade_viewport)
            {
                // The active view has changed: fetch the new view
                ViewExp* const view_exp = view_event.GetView();
                if(DbgVerify(view_exp != nullptr))
                {
                    // Set the new viewport parameters
                    set_view(*view_exp);
                }
            }
        }
        break;
    case EventType_View_Deleted:
        {
            // View node was deleted
            INode* const deleted_node = view_event.GetViewCameraOrLightNode();
            if(deleted_node != nullptr)
            {
                camera_node_deleted(*deleted_node);
            }
        }
        break;
    default:
        // Unexpected event type?
        DbgAssert(false);
        break;
    }
}

void CameraContainer_Interactive::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* /*userData*/) 
{
    if(dynamic_cast<const INodeEvent*>(&genericEvent) != nullptr)
    {
        // Notification from camera node
        const INodeEvent& node_event = static_cast<const INodeEvent&>(genericEvent);
        switch(node_event.GetEventType())
        {
        case EventType_Node_Deleted:
            // I don't think we should be receiving this notification; the camera should have been set to null before it gets deleted
            DbgAssert(false);
            break;
        case EventType_Node_ParamBlock:
        case EventType_Node_Transform:
            // Standard camera sends notifications with PART_OBJ, which translate to vertices and faces events
        case EventType_Mesh_Vertices:
        case EventType_Mesh_Faces:
            Invalidate();
            break;
        default:
            // Don't care about other notifications
            break;
        }
    }
    else
    {
        // Unhandled event type?
        DbgAssert(false);
    }
}

void CameraContainer_Interactive::set_region(const Box2& region)
{
    set_region_internal(region);
}

void CameraContainer_Interactive::set_bitmap(Bitmap* const bitmap)
{
    set_bitmap_internal(bitmap);
}

}}	// namespace
