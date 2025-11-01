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

// local includes
#include "RenderSessionContext_Interactive.h"
#include "RenderingLogger.h"
#include "RenderingProgress.h"
// max sdk
#include <interactiverender.h>

namespace MaxSDK
{;
namespace RenderingAPI
{
    class UnifiedRenderer;
    class IInteractiveRenderSession;
}
}

namespace Max
{;
namespace RenderingAPI
{;

class InteractiveRenderer : 
    public IInteractiveRender, 
    public MaxSDK::Util::Noncopyable
{
public:

    InteractiveRenderer(UnifiedRenderer& renderer);
	virtual	~InteractiveRenderer();

    // -- from IInteractiveRenderer
    virtual void BeginSession() override;
    virtual void EndSession() override;
    virtual void SetOwnerWnd(HWND	hOwnerWnd) override;
    virtual HWND GetOwnerWnd() const  override;
    virtual void SetIIRenderMgr(IIRenderMgr	*pIIRenderMgr) override;
    virtual IIRenderMgr* GetIIRenderMgr	(IIRenderMgr *pIIRenderMgr)	const	 override;
    virtual void SetBitmap(Bitmap	*pDestBitmap) override;
    virtual Bitmap*	GetBitmap(Bitmap *pDestBitmap) const  override;
    virtual void SetRegion(const Box2	&region) override;
    virtual const	Box2&	GetRegion()	const	 override;
    virtual void SetSceneINode(INode *pSceneINode) override;
    virtual INode* GetSceneINode() const override;
    virtual void SetUseViewINode(bool	bUseViewINode) override;
    virtual bool GetUseViewINode() const override;
    virtual void SetViewINode(INode	*pViewINode) override;
    virtual INode* GetViewINode()	const	override;
    virtual void SetViewExp(ViewExp	*pViewExp) override;
    virtual ViewExp* GetViewExp()	const	override;
    virtual void SetDefaultLights(DefaultLight *pDefLights,	int	numDefLights) override;
    virtual const	DefaultLight*	GetDefaultLights (int	&numDefLights) const override;
    virtual void SetProgressCallback(IRenderProgressCallback *pProgCB) override;
    virtual const	IRenderProgressCallback	*	GetProgressCallback()	const	override;
    virtual void Render(Bitmap *pDestBitmap) override;
    virtual BOOL AnyUpdatesPending() override;
    virtual void AbortRender() override;
    virtual ULONG	GetNodeHandle(int	x, int y) override;
    virtual bool GetScreenBBox(Box2	&sBBox,	INode	*pINode) override;
    virtual ActionTableId	GetActionTableId() override;
    virtual ActionCallback*	GetActionCallback() override;
	
	//From Animatable
	SClass_ID	SuperClassID() { return	RENDERER_CLASS_ID; }

private:

	static DWORD WINAPI	updateLoopThread(LPVOID ptr);
    void update_loop_thread();

    // -- inherited from IInteractiveRender
    virtual BOOL IsRendering();

private:

    HWND							m_OwnerWnd;
    // The render plugin through which render sessions are created
    UnifiedRenderer& m_renderer_plugin;
    // The render session currently in use
    std::shared_ptr<IInteractiveRenderSession> m_interactive_render_session;

    // The handle to the thread from which the active shade session is run
	HANDLE m_interactiveRenderLoopThread;

    // The context to be rendered
    std::shared_ptr<RenderSessionContext_Interactive> m_render_session_context;

    // These are the values which we need to save in order to pass to the constructor of the render session context
    Bitmap* m_bitmap;
    IIRenderMgr* m_pIIRenderMgr;
    INode* m_pSceneINode;
    bool m_bUseViewINode;
    INode* m_pViewINode;
    ViewExp* m_pViewExp;
    Box2 m_region;
    std::vector<DefaultLight> m_default_lights;
    IRenderProgressCallback* m_pProgCB;

    // Specifies whether we're currently rendering
    bool m_currently_rendering;

    // Saves the last time at which NOTIFY_RENDER_PREEVAL was broadcast, in order to know when it needs to be broadcast again.
    TimeValue m_last_pre_eval_notification_broadcast_time;
};

}}  // namespace