/**************************************************************************
* Copyright (c) 2015-2022 Luxrender.                                      *
* All rights reserved.                                                    *
*                                                                         *
* DESCRIPTION: Contains the Dll Entry stuff                               *
* AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com                         *
*                                                                         *
*   This file is part of LuxRender.                                       *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
***************************************************************************/

#pragma once

#include <interactiverender.h>
#include <ISceneEventManager.h>
#include <maxapi.h>

// Standard headers.
#include <memory>
#include <vector>

// Forward declarations.
namespace renderer { class Project; }
class InteractiveSession;
class RendererSettings;
class ViewParams;

class MaxToLuxInteractiveRender
	: public IInteractiveRender
{
public:
	MaxToLuxInteractiveRender();
	~MaxToLuxInteractiveRender() override;

	// IInteractiveRender methods.
	void BeginSession() override;
	void EndSession() override;
	void SetOwnerWnd(HWND owner_wnd) override;
	HWND GetOwnerWnd() const override;
	void SetIIRenderMgr(IIRenderMgr* irender_manager) override;
	IIRenderMgr* GetIIRenderMgr(IIRenderMgr* irender_manager) const override;
	void SetBitmap(Bitmap* bitmap) override;
	Bitmap* GetBitmap(Bitmap* bitmap) const override;
	void SetSceneINode(INode* scene_inode) override;
	INode* GetSceneINode() const override;
	void SetUseViewINode(bool use_view_inode) override;
	bool GetUseViewINode() const override;
	void SetViewINode(INode* view_inode) override;
	INode* GetViewINode() const override;
	void SetViewExp(ViewExp* view_exp) override;
	ViewExp* GetViewExp() const override;
	void SetRegion(const Box2& region) override;
	const Box2& GetRegion() const override;
	void SetDefaultLights(DefaultLight* def_lights, int num_def_lights) override;
	const DefaultLight* GetDefaultLights(int& num_def_lights) const override;
	void SetProgressCallback(IRenderProgressCallback* prog_cb) override;
	const IRenderProgressCallback* GetProgressCallback() const override;
	void Render(Bitmap* bitmap) override;
	ULONG GetNodeHandle(int x, int y) override;
	bool GetScreenBBox(Box2& s_bbox, INode* inode) override;
	ActionTableId GetActionTableId() override;
	ActionCallback* GetActionCallback() override;
	BOOL IsRendering() override;

	// IAbortableRenderer methods.
	void AbortRender() override;

	void update_camera_object(INode* camera);
	void add_object_instance(const std::vector<INode*>&);
	void remove_object_instance(const std::vector<INode*>&);
	void update_object_instance(const std::vector<INode*>&);
	void update_material(const std::vector<INode*>& nodes);
	void update_render_view();
	InteractiveSession* get_render_session();

private:
	std::unique_ptr<InteractiveSession>             m_render_session;
	std::unique_ptr<INodeEventCallback>             m_node_callback;
	std::unique_ptr<RedrawViewsCallback>            m_view_callback;
	Bitmap*                                         m_bitmap;
	std::vector<DefaultLight>                       m_default_lights;
	IIRenderMgr*                                    m_irender_manager;
	HWND                                            m_owner_wnd;
	IRenderProgressCallback*                        m_progress_cb;
	TimeValue                                       m_time;
	Box2                                            m_region;
	INode*                                          m_scene_inode;
	ViewExp*                                        m_view_exp;
	INode*                                          m_view_inode;
	bool                                            m_use_view_inode;

	bool prepare_project(
		const RendererSettings&     renderer_settings,
		const ViewParams&           view_params,
		INode*                      camera_node,
		const TimeValue             time);
};