//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma	once

#include <RenderingAPI/Renderer/UnifiedRenderer.h>

// local includes
#include "RenderSessionContext_Offline.h"
#include "RenderingLogger.h"
#include "RenderingProgress.h"
// std
#include <vector>

namespace Max
{
    namespace RenderingAPI
    {
        class InteractiveRenderer;
    }
}

namespace MaxSDK
{;
namespace RenderingAPI
{;

using namespace Max::RenderingAPI;

// Private container for data members of class SimplifiedRenderers. Encapsulates, hides these members from the public API.
// ** Note the class Simplified Renderer implements OFFLINE RENDERING. INTERACTIVE RENDERING is implemented by class InterfactiveRenderer. **
class UnifiedRenderer::Implementation
{
    friend class UnifiedRenderer;
public:
    ~Implementation();

private:
    Implementation();

    // Callback for NOTIFY_PRE_RENDER
    static void pre_render_notification_callback(void *param, NotifyInfo *info);

private:

    // The render session used for offline rendering
    std::shared_ptr<IOfflineRenderSession> m_offline_render_session;

    // The context to be rendered
    std::shared_ptr<RenderSessionContext_Offline> m_render_session_context;

    // Pointer to the interactive renderer, returned by GetInterface()
	std::unique_ptr<InteractiveRenderer> m_interactive_renderer; 

    // Saves the RendParams with which Open() was last called.
    RendParams m_open_rend_params;

    // Saves the time of the last frame rendered. Used for calling RenderEnd()
    TimeValue m_last_render_frame_time;

    // Flags this renderer as currently being in the process of rendering (between calls to Open() and Close())
    bool m_is_rendering;
};

// Private container for data members of class UnifiedRenderer::ClassDescriptor
class UnifiedRenderer::ClassDescriptor::Implementation
{
    friend class UnifiedRenderer::ClassDescriptor;
public:
    ~Implementation();

private:
    Implementation();

private:

};

}}  // namespace