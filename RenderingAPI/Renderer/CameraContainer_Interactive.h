//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once 

// local includes
#include "CameraContainer_Base.h"
// max sdk
#include <NotificationAPI/InteractiveRenderingAPI_Subscription.h>
#include <NotificationAPI/InteractiveRenderingAPI_Subscription.h>
// std
#include <vector>

class IIRenderMgr;

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::NotificationAPI;

// This is the interactive (active shade) version of ICameraContainer
class CameraContainer_Interactive :
    public CameraContainer_Base,
    private IInteractiveRenderingCallback,
    private INotificationCallback
{
public:

	CameraContainer_Interactive(
        const UnifiedRenderer& renderer, 
        const IRenderSettingsContainer& render_settings_container, 
        IImmediateInteractiveRenderingClient* notification_client,
        IIRenderMgr* pIIRenderMgr,
        const Box2& region,
        Bitmap* const bitmap,
        INode* const camera_node,
        const bool use_camera,
        const ViewExp* view_exp);
    ~CameraContainer_Interactive();

    void set_irender_mgr(IIRenderMgr* pIIRenderMgr);
    void set_region(const Box2& region);
    void set_bitmap(Bitmap* const bitmap);

protected:
    
    // -- inherited from CameraContainer_Base
    virtual void notify_change() override;

    // -- inherited from NotificationAPI::IInteractiveRenderingCallback
    virtual void InteractiveRenderingCallback_NotifyEvent(const IViewEvent& viewEvent, void* userData) override;

    // -- inherited from INotificationCallback
    virtual void NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData) override;

private:

    // Replaces the camera node being monitored with the given one
    void monitor_camera_node(INode* const node);

private:

    // The notification wrapper we're using to internally listen to scene changes
    IImmediateInteractiveRenderingClient* const m_notification_client;

    // Flag that specifies whether the current camera is attached to a viewport (as opposed to a floating active shade window)
    bool m_is_running_in_active_shade_viewport;

    // The camera node currently being monitored; used to determine when a new node must be monitored.
    INode* m_monitored_camera_node;
};

}}	// namespace
