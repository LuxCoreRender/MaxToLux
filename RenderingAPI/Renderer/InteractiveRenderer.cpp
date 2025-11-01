//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "InteractiveRenderer.h"

// local includes
#include "../resource.h"

// Max SDK
#include <Util/DebugHelpers.h>
#include <RenderingAPI/Renderer/UnifiedRenderer.h>
#include <RenderingAPI/Renderer/IInteractiveRenderSession.h>
#include <maxscript/maxscript.h>
#include <notify.h>

namespace Max
{;
namespace RenderingAPI
{;

InteractiveRenderer::InteractiveRenderer(UnifiedRenderer& renderer)
    : m_OwnerWnd(0),
    m_interactiveRenderLoopThread(NULL),
    m_currently_rendering(false),
    m_renderer_plugin(renderer),
    m_last_pre_eval_notification_broadcast_time(0),
    m_bitmap(nullptr),
    m_pIIRenderMgr(nullptr),
    m_pSceneINode(nullptr),
    m_bUseViewINode(false),
    m_pViewINode(nullptr),
    m_pViewExp(nullptr),
    m_pProgCB(nullptr)
{
    
}

InteractiveRenderer::~InteractiveRenderer(void)
{	
    // Make sure the active shade session has stopped
    DbgAssert(m_interactiveRenderLoopThread == nullptr);
    EndSession();
}

void InteractiveRenderer::BeginSession()
{
    // Is the session already running?
    if(m_interactiveRenderLoopThread == nullptr)
    {
        // The session and context should already be null
        DbgAssert((m_render_session_context == nullptr) && (m_interactive_render_session == nullptr));
        m_interactive_render_session.reset();
        m_render_session_context.reset();

        // Create the render session context
        m_render_session_context.reset(
            new RenderSessionContext_Interactive(
                m_renderer_plugin, 
                m_pIIRenderMgr,
                IRenderMessageManager::kSource_ActiveShadeRenderer,
                m_pProgCB,
                m_pIIRenderMgr,
                m_region,
                m_bitmap,
                m_pViewINode,
                m_bUseViewINode,
                m_pViewExp,
                m_pSceneINode,
                m_default_lights.data(), m_default_lights.size()));

        // Do missing maps dialog on beginning of render session
        const TimeValue current_time = GetCOREInterface()->GetTime();
        if(m_render_session_context->do_missing_maps_dialog(current_time, m_OwnerWnd))
        {
            // Initially set m_currently_rendering. This gets rest by the active shade thread whenever rendering is done, or in EndSession() when
            // we terminate the thread.
            m_currently_rendering = true;

            // Create the render session
            {
                IRenderingProcess::NamedTimerGuard(MaxSDK::GetResourceStringAsMSTR(IDS_SESSION_CREATION), m_render_session_context->GetRenderingProcess());
                m_interactive_render_session = m_renderer_plugin.CreateInteractiveSession(*(m_render_session_context));
                m_render_session_context->set_render_session(m_interactive_render_session);
            }
    
            if(DbgVerify(m_interactive_render_session != nullptr))
            {
                {
                    // Pre-eval notification needs to be sent before scene nodes are evaluated, and called again whenever the time changes
                    TimeValue eval_time = current_time;  // To avoid const_cast below, and possibility of notifiee from changing the value
                    //!! TODO Kirin: Maybe stop broadcasting this, it's evil to broadcast notifications like this in active shade - I would need
                    // to replace this with a different mechanism.
                    BroadcastNotification(NOTIFY_RENDER_PREEVAL, &eval_time);
                    m_last_pre_eval_notification_broadcast_time = current_time;
                }

                if(m_interactive_render_session->InitiateInteractiveSession(current_time))
                {
                    // Create the thread for the render session
                    m_interactiveRenderLoopThread = CreateThread(NULL, 0, updateLoopThread, this, 0, nullptr);
                    DbgAssert(m_interactiveRenderLoopThread != nullptr);
                }
            }
            else
            {
                m_render_session_context->GetLogger().LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_CREATION_INTERACTIVE_SESSION));
                m_render_session_context.reset();
            }
        }
    }
}

void InteractiveRenderer::EndSession()
{
    if((m_interactiveRenderLoopThread != nullptr) && (m_render_session_context != nullptr) && (m_interactive_render_session != nullptr))
    {
        // Stop all main thread jobs to avoid deadlocking while we wait for the render thread to finish (this being the main thread, main thread
        // jobs will NOT run while we're waiting)
        m_render_session_context->get_progress().CancelMainThreadJobs();

        // Wait for the thread to finish
        if(m_interactiveRenderLoopThread != nullptr)
        {
            WaitForSingleObject(m_interactiveRenderLoopThread, INFINITE);
            CloseHandle(m_interactiveRenderLoopThread);
            m_interactiveRenderLoopThread = nullptr;
        }

        // Abort the interactive session, now that the update thread has finished.
        if(m_interactive_render_session != nullptr)
        {
            m_interactive_render_session->TerminateInteractiveSession();
        }

        // Destroy the render session
        {
            IRenderingProcess::NamedTimerGuard(MaxSDK::GetResourceStringAsMSTR(IDS_SESSION_DESTRUCTION), m_render_session_context->GetRenderingProcess());
            m_interactive_render_session.reset();
        }
        // Destroy the render session context
        m_render_session_context.reset();

        // Reset m_currently_rendering since we're definitely no longer rendering
        m_currently_rendering = false;

        // Run maxscript garbage collection to get rid of any leftover "leaks" from AMG.
        DbgVerify(ExecuteMAXScriptScript(_T("gc light:true"), true));
    }

    DbgAssert((m_interactiveRenderLoopThread == nullptr) && (m_render_session_context == nullptr) && (m_interactive_render_session == nullptr));
}

void InteractiveRenderer::SetOwnerWnd(HWND	hOwnerWnd)
{
    m_OwnerWnd = hOwnerWnd;
}

DWORD WINAPI InteractiveRenderer::updateLoopThread(LPVOID ptr)
{
    MaxSDK::Util::DebugHelpers::SetThreadName("RenderingAPI_InteractiveRenderer");

    InteractiveRenderer* pRRTInteractive = static_cast<InteractiveRenderer*>(ptr);
    pRRTInteractive->update_loop_thread();
    return 0;
}

void InteractiveRenderer::update_loop_thread()
{
    if(DbgVerify((m_interactive_render_session != nullptr) && (m_render_session_context != nullptr)))
    {
        while(!m_render_session_context->get_progress().HasAbortBeenRequested())
        {
            const TimeValue current_time = GetCOREInterface()->GetTime();

            // Broadcast notifications as necessary
            if(current_time != m_last_pre_eval_notification_broadcast_time)
            {
                TimeValue eval_time = current_time;  // To avoid const_cast below, and possibility of notifiee from changing the value                
                BroadcastNotification(NOTIFY_RENDER_PREEVAL, &eval_time);
                m_last_pre_eval_notification_broadcast_time = current_time;
            }

            // Update the renderer at the new time, process updates, get frame buffer updates
            bool done_rendering = false;

            if(m_interactive_render_session->UpdateInteractiveSession(current_time, done_rendering))
            {
                // Update the state of m_currently_rendering dynamically, such that IsRendering() returns false when rendering is done.
                m_currently_rendering = !done_rendering;
            }
            else
            {
                // Error
                m_render_session_context->GetLogger().LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_UPDATING_SCENE));
                // Abort rendering
                m_render_session_context->get_progress().set_abort();
            }

            // When done rendering, sleep a while to avoid hogging the CPU.
            if(done_rendering && !m_render_session_context->get_progress().HasAbortBeenRequested())
            {
                Sleep(100);     // 100 ms
            }
        }
    }
}	

HWND InteractiveRenderer::GetOwnerWnd() const 
{
    return m_OwnerWnd;
}	

void InteractiveRenderer::SetIIRenderMgr(IIRenderMgr* pIIRenderMgr)
{
    m_pIIRenderMgr = pIIRenderMgr;
    if(m_render_session_context != nullptr)
    {
        m_render_session_context->set_irender_manager(pIIRenderMgr);
    }
}	

IIRenderMgr* InteractiveRenderer::GetIIRenderMgr (IIRenderMgr* /*pIIRenderMgr*/) const 
{
    return m_pIIRenderMgr;
}	

void InteractiveRenderer::SetBitmap(Bitmap* pDestBitmap)
{
    m_bitmap = pDestBitmap;
    if(m_render_session_context != nullptr)
    {
        m_render_session_context->get_camera_container().set_bitmap(pDestBitmap);
    }
}	

Bitmap * InteractiveRenderer::GetBitmap(Bitmap* /*pDestBitmap*/)	const
{
    return m_bitmap;
}	

void InteractiveRenderer::SetSceneINode(INode *pSceneINode)
{
    m_pSceneINode = pSceneINode;
    if(m_render_session_context != nullptr)
    {
        m_render_session_context->get_scene_container().set_scene_root_node(pSceneINode);
    }
}	

INode* InteractiveRenderer::GetSceneINode()	const	
{
    return m_pSceneINode;
}	

void InteractiveRenderer::SetUseViewINode(bool bUseViewINode)
{
    m_bUseViewINode = bUseViewINode;
    if(m_render_session_context != nullptr)
    {
        m_render_session_context->get_camera_container().set_use_camera(bUseViewINode);
    }
}	

bool InteractiveRenderer::GetUseViewINode() const 
{
    return m_bUseViewINode;
}	

void InteractiveRenderer::SetViewINode(INode* pViewINode)
{
    m_pViewINode = pViewINode;
    if(m_render_session_context != nullptr)
    {
        m_render_session_context->get_camera_container().set_camera_node(pViewINode);
    }
}	

INode* InteractiveRenderer::GetViewINode() const 
{
    return m_pViewINode;
}	

void InteractiveRenderer::SetViewExp(ViewExp* pViewExp)
{
    m_pViewExp = pViewExp;
    if((m_render_session_context != nullptr) && (pViewExp != nullptr))
    {
        m_render_session_context->get_camera_container().set_view(*pViewExp);
    }
}	

ViewExp* InteractiveRenderer::GetViewExp() const 
{
    return m_pViewExp;
}	

void InteractiveRenderer::SetRegion(const Box2& region)
{
    m_region = region;
    if(m_render_session_context != nullptr)
    {
        m_render_session_context->get_camera_container().set_region(region);
    }
}	

const Box2& InteractiveRenderer::GetRegion() const 
{
    return m_region;
}	

void InteractiveRenderer::SetDefaultLights(DefaultLight* pDefLights, int numDefLights)
{
    m_default_lights.clear();
    if(pDefLights != nullptr)
    {
        m_default_lights.insert(m_default_lights.begin(), pDefLights, pDefLights + numDefLights);
    }

    if(m_render_session_context != nullptr)
    {
        m_render_session_context->set_default_lights(pDefLights, numDefLights);
    }
}	

const DefaultLight* InteractiveRenderer::GetDefaultLights (int &numDefLights) const	
{
    numDefLights = int(m_default_lights.size());
    return m_default_lights.data();
}	

void InteractiveRenderer::SetProgressCallback(IRenderProgressCallback *pProgCB)
{
    m_pProgCB = pProgCB;
    if(m_render_session_context != nullptr)
    {
        m_render_session_context->get_progress().set_progress_callback(pProgCB);
    }
}	

const IRenderProgressCallback* InteractiveRenderer::GetProgressCallback() const
{
    return m_pProgCB;
}	

void InteractiveRenderer::Render(Bitmap* /*pDestBitmap*/)
{
    
}	

ULONG	InteractiveRenderer::GetNodeHandle(int	/*x*/, int /*y*/)
{
    return 0;
}	

bool InteractiveRenderer::GetScreenBBox(Box2	&/*sBBox*/,	INode* /*pINode*/)
{
    return FALSE;
}	

ActionTableId	InteractiveRenderer::GetActionTableId()
{
    return NULL;
}	

ActionCallback * InteractiveRenderer::GetActionCallback()
{
    return NULL;
}

BOOL InteractiveRenderer::AnyUpdatesPending()
{
    return (m_interactive_render_session != nullptr) && !m_interactive_render_session->IsInteractiveSessionUpToDate(GetCOREInterface()->GetTime());
}

void InteractiveRenderer::AbortRender()
{
    EndSession();
}

BOOL InteractiveRenderer::IsRendering()
{
    return m_currently_rendering;
}

}} // namespace