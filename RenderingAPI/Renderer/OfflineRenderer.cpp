//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "OfflineRenderer.h"

// local includes
#include "../resource.h"
#include "InteractiveRenderer.h"
#include "RenderingAPI_RGC.h"
// max sdk
#include <iparamb2.h>
#include <RenderingAPI/Renderer/IOfflineRenderSession.h>
#include <pbbitmap.h>
#include <maxscript/maxscript.h>
#include <notify.h>

namespace MaxSDK
{;
namespace RenderingAPI
{;

namespace
{
    const int k_subclass_chunk_id = 777;

    // Specialization of FrameBufferProcessor for a render element
    class RenderElementFrameBufferProcessor : 
        public FrameBufferProcessor,
        public MaxSDK::Util::Noncopyable
    {
    public:

        RenderElementFrameBufferProcessor(IRenderElement& render_element, IRenderSessionContext& render_session_context)
            : m_render_element(render_element),
            m_render_session_context(render_session_context)
        {
        }

    protected:

        // -- inherited from FrameBufferProcessor
        virtual Bitmap* get_frame_buffer_bitmap() override
        {
            PBBitmap* pb_bitmap = nullptr;
            m_render_element.GetPBBitmap(pb_bitmap);
            return (pb_bitmap != nullptr) ? pb_bitmap->bm : nullptr;
        }
        virtual IRenderSessionContext& get_render_session_context() override
        {
            return m_render_session_context;
        }

    private:

        IRenderElement& m_render_element;
        IRenderSessionContext& m_render_session_context;
    };

    // Guard class which temporarily sets the render progress callback, re-setting it in the destructor
    class RenderProgressCallbackGuard :
        public MaxSDK::Util::Noncopyable
    {
    public:

        RenderProgressCallbackGuard(RenderSessionContext_Offline& context, RendProgressCallback* const prog)
            : m_context(context),
            m_old_prog(context.get_progress().get_progress_callback())
        {
            m_context.get_progress().set_progress_callback(prog);
        }
        ~RenderProgressCallbackGuard()
        {
            m_context.get_progress().set_progress_callback(m_old_prog);
        }

    private:

        RenderSessionContext_Offline& m_context;
        RendProgressCallback* const m_old_prog;
    };  

    // Progress/abort callback interface for use with render effects
    class EffectAbortCallback : public CheckAbortCallback
    {
    public:
        explicit EffectAbortCallback(const IRenderSessionContext& session_context)
            : m_session_context(session_context)
        {
        }

        // -- inherited from CheckAbortCallback
        virtual BOOL Check() override
        {
            return m_session_context.GetRenderingProcess().HasAbortBeenRequested();
        }
        virtual	BOOL Progress(int done, int total) override
        {
            m_session_context.GetRenderingProcess().SetRenderingProgress(done, total, IRenderingProcess::ProgressType::Rendering);
            return Check();
        }
        virtual void SetTitle(const MCHAR *title) override
        {
            m_session_context.GetRenderingProcess().SetRenderingProgressTitle(title);
        }

    private:
        const IRenderSessionContext& m_session_context;
    };
}

//==================================================================================================
// class UnifiedRenderer::Implementation
//==================================================================================================

UnifiedRenderer::Implementation::Implementation()
    : m_last_render_frame_time(0),
    m_is_rendering(false)
{

}

UnifiedRenderer::Implementation::~Implementation()
{

}

void UnifiedRenderer::Implementation::pre_render_notification_callback(void *param, NotifyInfo* /*info*/)
{
    // If NOTIFY_PRE_RENDER is received (from another renderer), then we clear the render session.
    // This is meant to free memory used by the MEdit render session.
    // This only gets triggered when an offline rendering is started - we don't want to clear
    // the MEdit session when starting ActiveShade.
    Implementation* const impl = static_cast<Implementation*>(param);
    // This checks excludes the renderer that emitted the notification, or any renderer that's currently active.
    if(!impl->m_is_rendering)
    {
        impl->m_offline_render_session.reset();
        impl->m_render_session_context.reset();
    }
}

//==================================================================================================
// class UnifiedRenderer
//==================================================================================================

UnifiedRenderer::UnifiedRenderer()	
{
    m_impl = std::unique_ptr<Implementation>(new Implementation());    

    // Listen to NOTIFY_PRE_RENDER from other instances of the renderer
    RegisterNotification(Implementation::pre_render_notification_callback, m_impl.get(), NOTIFY_PRE_RENDER);

}

UnifiedRenderer::~UnifiedRenderer()	
{
    UnRegisterNotification(Implementation::pre_render_notification_callback, m_impl.get(), NOTIFY_PRE_RENDER);

    // The session should already have been deleted, in Close()
    DbgAssert(m_impl->m_offline_render_session == nullptr);
}

int	UnifiedRenderer::Open(INode* const scene, INode* const vnode, ViewParams* const viewPar, RendParams& rp, const HWND /*hwnd*/, DefaultLight* const defaultLights, const int numDefLights, RendProgressCallback* const prog)
{	
    m_impl->m_is_rendering = true;

    //Suspend	undo etc
    SuspendAll uberSuspend(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

    // Initialize the last frame render time
    m_impl->m_last_render_frame_time = rp.firstFrame;

    if(!rp.inMtlEdit)
    {
        // When switching from MEdit to production rendering, we make sure to re-create the render session. This is to
        // to avoid keeping the MEdit render session context around, as it may contain a NotificiationClient which we don't
        // want to have around when rendering production. All this is to improve stability, guarantee that production rendering
        // always goes through the same code path and never re-uses stuff previously translated from MEdit.
        m_impl->m_offline_render_session.reset();
        m_impl->m_render_session_context.reset();
    }

    // Create the render session context if necessary
    if(m_impl->m_render_session_context == nullptr)
    {
        m_impl->m_render_session_context.reset(
            new RenderSessionContext_Offline(
                *this,
                scene,
                defaultLights, numDefLights,
                vnode,
                viewPar,
                rp));
    }
    else
    {
        // Re-initialize the session context
        m_impl->m_render_session_context->reinitialize(
            scene,
            defaultLights, numDefLights,
            vnode,
            viewPar,
            rp);
    }

    // The progress callback must be reset upon returning from this method, as it may no longer be valid (e.g. the tone operator panel's
    // progress callback is destroyed just after this method returns)
    RenderProgressCallbackGuard progress_callback_guard(*(m_impl->m_render_session_context), prog);

    // Set an initial, placeholder progress title
    m_impl->m_render_session_context->get_progress().SetRenderingProgressTitle(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_INITIAL_TITLE));

	//Might	be needed. Enable	again	in Close()
	GetCOREInterface()->DisableSceneRedraw();
	
	if (!rp.inMtlEdit) 
    {
		BroadcastNotification(NOTIFY_PRE_RENDER, (void*)(RendParams*)&rp);
    }

    m_impl->m_open_rend_params = rp;

	return 1;
}

int	UnifiedRenderer::Render(const TimeValue t, Bitmap* const tobm, FrameRendParams &frp, const HWND hwnd, RendProgressCallback* const prog, ViewParams* const viewPar)
{
    //Suspend undo etc
    SuspendAll uberSuspend(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

    // Check that the render session context was correctly created by the call to Open()
    if(DbgVerify(m_impl->m_render_session_context != nullptr))
    {
        // Start with replacing the progress callback with the new one
        // The progress callback must be reset upon returning from this method, as it may no longer be valid (e.g. the tone operator panel's
        // progress callback is destroyed just after this method returns)
        RenderProgressCallbackGuard progress_callback_guard(*(m_impl->m_render_session_context), prog);

        // Pre-eval notification needs to be sent before scene nodes are evaluated, and called again whenever the time changes
        {
            TimeValue eval_time = t;  // To avoid const_cast below, and possibility of notifiee from changing the value                
            BroadcastNotification(NOTIFY_RENDER_PREEVAL, &eval_time);
        }

        // Set an initial, placeholder progress title
        m_impl->m_render_session_context->get_progress().SetRenderingProgressTitle(MaxSDK::GetResourceStringAsMSTR(IDS_PROGRESS_INITIAL_TITLE));
        // Remember the last frame render time
        m_impl->m_last_render_frame_time = t;

        // Update the the session context with any frame-dependent parameters
        m_impl->m_render_session_context->setup_frame_params(tobm, m_impl->m_open_rend_params, frp);
        if(viewPar != nullptr)
        {
            m_impl->m_render_session_context->get_camera_container().set_view(*viewPar);
        }

        // Check for missing maps, but only on first frame  (first frame == render session not created yet)
        if((m_impl->m_offline_render_session != nullptr) || m_impl->m_render_session_context->do_missing_maps_dialog(t, hwnd))
        {
            // Initialize the render session
            if(m_impl->m_offline_render_session == nullptr)
            {
                IRenderingProcess::NamedTimerGuard(MaxSDK::GetResourceStringAsMSTR(IDS_SESSION_CREATION), m_impl->m_render_session_context->GetRenderingProcess());
                m_impl->m_offline_render_session = CreateOfflineSession(*(m_impl->m_render_session_context));
                m_impl->m_render_session_context->set_render_session(m_impl->m_offline_render_session);
            }

            if(m_impl->m_offline_render_session != nullptr)
            {
                if(m_impl->m_offline_render_session->TranslateScene(t))
                {
                    // Report missing UVW channels on rendering first frame
                    if((m_impl->m_open_rend_params.firstFrame == t) && !m_impl->m_open_rend_params.inMtlEdit)
                    {
                        m_impl->m_render_session_context->get_progress().process_reported_missing_uvw_channels(hwnd);
                    }

                    if(!m_impl->m_render_session_context->get_progress().HasAbortBeenRequested())
                    {
                        // Pre-render-frame notification to be sent after scene has been evaluated, but before the render starts - only for offline rendering
                        RenderingAPI_RGC rgc(*this, m_impl->m_open_rend_params, *(m_impl->m_render_session_context), t);
                        BroadcastNotification(NOTIFY_PRE_RENDERFRAME, static_cast<RenderGlobalContext*>(&rgc));

                        const bool render_success = m_impl->m_offline_render_session->RenderOfflineFrame(t);
                        const bool render_aborted = m_impl->m_render_session_context->get_progress().HasAbortBeenRequested();
                        bool render_elements_success = true;

                        // Process the render elements, iff we haven't aborted nor encountered an error
                        if(render_success && !render_aborted)
                        {
                            const std::vector<IRenderElement*> render_elements = m_impl->m_render_session_context->GetRenderElements();
                            if(!render_elements.empty())
                            {
                                m_impl->m_render_session_context->get_progress().SetRenderingProgressTitle(MaxSDK::GetResourceStringAsMSTR(IDS_PROCESSING_RENDER_ELEMENTS));
                                MaxSDK::Util::StopWatch elements_stopwatch;
                                elements_stopwatch.Start();

                                for(IRenderElement* const render_element : render_elements)
                                {
                                    if(render_element != nullptr)
                                    {
                                        RenderElementFrameBufferProcessor element_buffer_processor(*render_element, *(m_impl->m_render_session_context));
                                        if(!m_impl->m_offline_render_session->StoreRenderElementResult(t, *render_element, element_buffer_processor))
                                        {
                                            render_elements_success = false;
                                        }
                                    }
                                }

                                // Report time for processing elements
                                TSTR msg;
                                msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_RENDER_ELEMENTS_PROCESSED), elements_stopwatch.GetElapsedTime() * 1e-3);
                            }
                        }

                        // Reset the progress title string before continuing, to avoid displaying a stale status
                        m_impl->m_render_session_context->get_progress().SetRenderingProgressTitle(_T(""));

                        // Post-render-frame, called after the frame has been rendered, regardless of success - only for offline rendering
                        BroadcastNotification(NOTIFY_POST_RENDERFRAME, static_cast<RenderGlobalContext*>(&rgc));

                        if(render_success && !render_aborted)
                        {
                            // Apply the render effects
                            ApplyRenderEffects(t, tobm, false);

                            // Perform a final update on the frame buffer
                            tobm->RefreshWindow();
                            if (tobm->GetWindow()){
                                UpdateWindow(tobm->GetWindow());
                            }
                        }

                        if(!render_elements_success)
                        {
                            // Output error
                            m_impl->m_render_session_context->get_logger().LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_RENDER_ELEMENTS));
                            return 0;
                        }
                        else if(!render_success || render_aborted)
                        {
                            if(!render_aborted)
                            {
                                // Error rendering
                                m_impl->m_render_session_context->get_logger().LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_RENDERING_FAILED));
                            }
                            return 0;
                        }
                    }

                    // Frame rendered successfully, or process was aborted
                    return 1;
                }
                else
                {
                    // Error translating
                    m_impl->m_render_session_context->get_logger().LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_TRANSLATION_FAILED));
                    return 0;
                }
            }
            else
            {
                // Failed to create render session
                // MAXX-35935: Mute this message in the material editor
                // This is not an ideal fix, but a proper fix would need to happen in
                // max core, beause the concurrent mtledit/activeshade detection logic is broken
                // so for now we just ignore this for material editor, and silently fail.
                if (!m_impl->m_open_rend_params.inMtlEdit)
                    m_impl->m_render_session_context->get_logger().LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_RENDER_SESSION_CREATION));
                return 0;
            }
        }
        else
        {
            // User aborted on missing maps dialog
            return 0;
        }
    }
    else
    {
        // Scene container not initialized: most likely Open() not called, or Open() failed
        return 0;
    }
}

void UnifiedRenderer::Close(HWND /*hwnd*/, RendProgressCallback* prog)
{
    if(DbgVerify(m_impl->m_render_session_context != nullptr))
    {
        // Update the progress callback
        m_impl->m_render_session_context->get_progress().set_progress_callback(prog);

	    //Suspend undo etc
	    SuspendAll uberSuspend(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);
        if(!m_impl->m_render_session_context->GetRenderSettings().GetIsMEditRender())
        {
		    BroadcastNotification(NOTIFY_POST_RENDER);
        }

        // Re-enable scene redraws (disabled in Open())
	    GetCOREInterface()->EnableSceneRedraw();
  
        // Call RenderEnd()
        m_impl->m_render_session_context->CallRenderEnd(m_impl->m_last_render_frame_time);

        // Keep render session around if doing interactive MEdit rendering
        const bool keep_render_session = m_impl->m_render_session_context->GetRenderSettings().GetIsMEditRender() && GetEnableInteractiveMEditSession();
        if(!keep_render_session)
        {
            // Delete the render session
            {
                IRenderingProcess::NamedTimerGuard(MaxSDK::GetResourceStringAsMSTR(IDS_SESSION_DESTRUCTION), m_impl->m_render_session_context->GetRenderingProcess());
                m_impl->m_offline_render_session.reset();
            }

            // Delete the render session context
            m_impl->m_render_session_context.reset();
        }
        else
        {
            // Report timing statistics
            m_impl->m_render_session_context->get_progress().report_timing_statistics();
        }

        // Run maxscript garbage collection to get rid of any leftover "leaks" from AMG.
        DbgVerify(ExecuteMAXScriptScript(_T("gc light:true"), true));
    }

    m_impl->m_is_rendering = false;
}

bool UnifiedRenderer::ApplyRenderEffects(const TimeValue t, Bitmap* const pBitmap, const bool updateDisplay)
{
    if((pBitmap != nullptr)
        && DbgVerify(m_impl->m_render_session_context != nullptr))
    {
        Effect* const effect = m_impl->m_render_session_context->GetEffect();
        if((effect != nullptr) && effect->Active(t))
        {
            RenderingAPI_RGC rgc(*this, m_impl->m_open_rend_params, *(m_impl->m_render_session_context), t);
            EffectAbortCallback abort_callback(*(m_impl->m_render_session_context));
            effect->Apply(t, pBitmap, &rgc, &abort_callback);

            if (updateDisplay)
            {
                pBitmap->RefreshWindow();
                if(pBitmap->GetWindow()) 
                {
                    UpdateWindow(pBitmap->GetWindow());
                }
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

void UnifiedRenderer::ResetParams()
{
    // Reset all param blocks
    const int num_param_blocks = NumParamBlocks();
    for(int i = 0; i < num_param_blocks; ++i)
    {
        IParamBlock2* const param_block = GetParamBlock(i);
        if(param_block != nullptr)
        {
            param_block->ResetAll();
        }
    }
}

RefResult UnifiedRenderer::NotifyRefChanged(const	Interval&	/*changeInt*/, RefTargetHandle /*hTarget*/,	PartID&	/*partID*/,	RefMessage /*message*/,	BOOL /*propagate*/)
{
	return REF_SUCCEED;
}

RefTargetHandle	UnifiedRenderer::Clone(RemapDir	&remap)
{
	UnifiedRenderer* newRend = GetClassDescriptor().CreateRenderer(false);
    if(DbgVerify(newRend != nullptr))
    {
        const int num_refs = NumRefs();
        for(int i = 0; i < num_refs; ++i)
        {
	        newRend->ReplaceReference(i, remap.CloneRef(GetReference(i))); 
        }
    	BaseClone(this, newRend, remap);
    }

	return newRend;
}

BaseInterface* UnifiedRenderer::GetInterface(Interface_ID id)
{
	if(id == TAB_DIALOG_OBJECT_INTERFACE_ID)
    {
		return static_cast<ITabDialogObject*>(this);
    }
	else 
    {
        BaseInterface* subclass_interface = GetInterface_UnifiedRenderer(id);
        if(subclass_interface != nullptr)
        {
            return subclass_interface;
        }
        else
        {
            return Renderer::GetInterface(id);
        }
    }
}

IInteractiveRender* UnifiedRenderer::GetIInteractiveRender() 
{
    // Check if the plugin supports interactive rendering
    if(SupportsInteractiveRendering())
    {
        if(m_impl->m_interactive_renderer == nullptr)
        {
            m_impl->m_interactive_renderer.reset(new InteractiveRenderer(const_cast<UnifiedRenderer&>(*this)));
        }

        return static_cast<IInteractiveRender*>(m_impl->m_interactive_renderer.get());
    }
    else
    {
        return nullptr;
    }
}

void* UnifiedRenderer::GetInterface(ULONG id)
{
    void* subclass_interface = GetInterface_UnifiedRenderer(id);
    if(subclass_interface != nullptr)
    {
        return subclass_interface;
    }
    else
    {
		return Renderer::GetInterface(id);
    }
}

int	UnifiedRenderer::AcceptTab(ITabDialogPluginTab*	tab)
{
    // Here, we assume that all relevant renderers don't support 3ds max's built-in Advanced Lighting and Raytracing settings
    // Derived classes can override and ignore this method if desired.

	 switch	(tab->GetSuperClassID()) {
	 case	RADIOSITY_CLASS_ID:
			return 0;					// Don't show	the	advanced lighting	tab
	 }

	 Class_ID id = tab->GetClassID();
	 if	(id	== RayEngineGlobalData_ClassID)
			return 0;					// Don't show	the	blur raytracer tab

	 //	Accept all other tabs
	 return	TAB_DIALOG_ADD_TAB;
}

IOResult UnifiedRenderer::Load(ILoad* const iload)	
{	
    if(iload != nullptr)
    {
        bool renderer_loaded = false;

        // Open a new chunk
        IOResult chunk_open_result = IO_OK;
        while((chunk_open_result = iload->OpenChunk()) == IO_OK)
        {
            // Read the chunk
            IOResult chunk_read_result = IO_OK;
            switch(iload->CurChunkID())     
            {   
            case k_subclass_chunk_id:
                // Load derived class data
                chunk_read_result = Load_UnifiedRenderer(*iload);
                renderer_loaded = true;
                break;
            }

            // Close the chunk
            if(chunk_read_result == IO_OK)
            {
                chunk_read_result = iload->CloseChunk();
            }

            // Check for error
            if(chunk_read_result != IO_OK)
            {
                return chunk_read_result;
            }
        } 

        if(!renderer_loaded)
        {
            // Hack to enable Load_UnifiedRenderer to be called when loading legacy files, from before we have the k_subclass_chunk_id chunk in place.
            // This is solely to allow RapidRT to install a post-load callback in those cases.
            DbgVerify(Load_UnifiedRenderer(*iload) == IO_OK);
        }

        if(chunk_open_result == IO_END)
        {
            // Finished reading all our chunks
            return IO_OK;
        }
        else
        {
            // error
            return chunk_open_result;
        }
    }
    else
    {
        return IO_OK;
    }

}

IOResult UnifiedRenderer::Save(ISave* const isave)
{
    if(isave != nullptr)
    {
        // Save derived class data
        isave->BeginChunk(k_subclass_chunk_id);
        const IOResult res = Save_UnifiedRenderer(*isave);
        isave->EndChunk();

        return res;
    }
    else
    {
        return IO_OK;
    }
}

Class_ID UnifiedRenderer::ClassID() 
{
    return GetClassDescriptor().ClassID();
}	

SClass_ID UnifiedRenderer::SuperClassID() 
{
    return RENDERER_CLASS_ID; 
}

void UnifiedRenderer::GetClassName(TSTR& s) 
{
    s = GetClassDescriptor().ClassName();
}

int	UnifiedRenderer::NumSubs()
{	
    return NumParamBlocks();
}

TSTR UnifiedRenderer::SubAnimName(int i)	
{
    IParamBlock2* param_block = GetParamBlock(i);
    return (param_block != nullptr) ? param_block->GetLocalName() : _T("");    
}

Animatable*	UnifiedRenderer::SubAnim(int i) 
{ 
    return GetParamBlock(i);
}

void UnifiedRenderer::StopRendering() 
{
    if(DbgVerify(m_impl->m_offline_render_session != nullptr))
    {
        m_impl->m_offline_render_session->StopRendering();
    }
}

void UnifiedRenderer::PauseRendering() 
{
    if(DbgVerify(m_impl->m_offline_render_session != nullptr))
    {
        m_impl->m_offline_render_session->PauseRendering();
    }
}

void UnifiedRenderer::ResumeRendering() 
{
    if(DbgVerify(m_impl->m_offline_render_session != nullptr))
    {
        m_impl->m_offline_render_session->ResumeRendering();
    }
}

void UnifiedRenderer::DeleteThis() 
{
    // Delete the render session and context, in case they weren't deleted in Close() because of interactive MEdit rendering.
    // We do this here, before the destructor is called, to make sure the derived class is still valid.
    m_impl->m_offline_render_session.reset();
    m_impl->m_render_session_context.reset();

    delete this;
}

//==================================================================================================
// class UnifiedRenderer::ClassDescriptor::Implementation
//==================================================================================================

UnifiedRenderer::ClassDescriptor::Implementation::Implementation()
{

}

UnifiedRenderer::ClassDescriptor::Implementation::~Implementation()
{

}

//==================================================================================================
// class UnifiedRenderer::ClassDescriptor
//==================================================================================================

UnifiedRenderer::ClassDescriptor::ClassDescriptor()
    : m_impl(new Implementation()) 
{
    IMtlRender_Compatibility_Renderer::Init(*this);
}

UnifiedRenderer::ClassDescriptor::~ClassDescriptor()
{

}

int	UnifiedRenderer::ClassDescriptor::IsPublic() 
{
    return true;
}

void* UnifiedRenderer::ClassDescriptor::Create(BOOL loading) 
{
    return static_cast<Renderer*>(CreateRenderer(!!loading));
}

SClass_ID UnifiedRenderer::ClassDescriptor::SuperClassID() 
{
    return RENDERER_CLASS_ID;
}

const TCHAR* UnifiedRenderer::ClassDescriptor::Category() 
{
    static const MSTR str = MaxSDK::GetResourceStringAsMSTR(IDS_RENDERER_PLUGIN_CATEGORY);	
    return str;
}

bool UnifiedRenderer::ClassDescriptor::IsCompatibleWrapperMaterial(ClassDesc& mtlBaseClassDesc) const
{
    const Class_ID targetClassID = mtlBaseClassDesc.ClassID();
    if(mtlBaseClassDesc.SuperClassID() == MATERIAL_CLASS_ID)
    {
        return (targetClassID == MULTI_MATERIAL_CLASS_ID)
            || (targetClassID == DXMATERIAL_CLASS_ID)
            || (targetClassID == Class_ID(BAKE_SHELL_CLASS_ID, 0))
            || (targetClassID == XREFMATERIAL_CLASS_ID);
    }
    else
    {
        return false;
    }
}

}}  // namespace