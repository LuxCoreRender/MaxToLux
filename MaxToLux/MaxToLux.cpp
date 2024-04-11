/**************************************************************************
* Copyright (c) 2015-2024 Luxrender.                                      *
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


#define CAMERA_TARGER_CLASSID	Class_ID(4128,0)
#define MAX2016_PHYSICAL_CAMERA Class_ID(1181315608, 686293133)
#define ENV_FOG_CLASS_ID		Class_ID(268435457, 0) 

#include <notify.h>
#include <limits>
#include "main.h"
#include "MaxToLux.h"
#include "maxscript/maxscript.h"

#include "LuxCamera.h"
#include "LuxUtils.h"
#include "LuxMaterials.h"
#include "LuxLights.h"
#include "LuxMesh.h"
#include "Classes.h"

#include <vector>
#include <thread>

#include <render.h>
#include <MeshNormalSpec.h>
#include <Path.h>
#include <bitmap.h>
#include <GraphicsWindow.h>
#include <IColorCorrectionMgr.h>
#include <IGame\IGame.h>
#include <VertexNormal.h>
#include <iostream>
#include <IMaterialBrowserEntryInfo.h>
#include <units.h>
#include <mutex>

#include <interactiverender.h>
#include <mesh.h>

#include <sstream>
#include <iInstanceMgr.h>
#include <unordered_map>

MaxToLuxCamera lxmCamera;
MaxToLuxLights lxmLights;
MaxToLuxMaterials lxmMaterials;
MaxToLuxUtils lxmUtils;
MaxToLuxMesh lxmMesh;

#pragma warning (push)
#pragma warning( disable:4002)
#pragma warning (pop)

namespace luxcore
{
	#include <luxcore.h>
}

using namespace std;
using namespace luxcore;
using namespace luxrays;


extern BOOL FileExists(const TCHAR *filename);
float* pixels;
bool defaultlightset = true;
bool defaultlightchk = true;
bool defaultlightauto = true;
bool deviceArray1 = true, deviceArray2, deviceArray3, deviceArray4, deviceArray5 = false;
int rendertype = 4;
int renderWidth = 0;
int renderHeight = 0;
bool renderingMaterialPreview = false;
int vfbRefreshRateInt = 1;
luxcore::Scene *materialPreviewScene;

int filterIndex;
//bool enableFileSaverOutput;

class MaxToLuxClassDesc :public ClassDesc2 {
public:
	virtual int 			IsPublic() { return 1; }
	virtual void *			Create(BOOL loading) { return new MaxToLux(loading); }
	//virtual void *			Create(BOOL loading) override final;
	virtual const TCHAR *	ClassName() { return GetString(IDS_VRENDTITLE); }
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 23900
	const TCHAR*  NonLocalizedClassName() override { return GetString(IDS_VRENDTITLE); }
#endif
	virtual SClass_ID		SuperClassID() { return RENDERER_CLASS_ID; }
	virtual Class_ID 		ClassID() { return REND_CLASS_ID; }
	virtual const TCHAR* 	Category() { return _T("LuxCoreRenderer"); }
	virtual void			ResetClassParams(BOOL fileReset) { UNREFERENCED_PARAMETER(fileReset); }
};

ClassDesc2* GetRendDesc() {
	static MaxToLuxClassDesc srendCD;
	return &srendCD;
}

MaxToLux::MaxToLux(BOOL loading)
{
	
}

MaxToLux::MaxToLux(IRenderSessionContext& sessionContext, const bool is_interactive_session)
	:m_rendering_logger(sessionContext.GetLogger()),
	m_rendering_process(sessionContext.GetRenderingProcess()),
	m_render_session_context(sessionContext)
	m_is_interactive_renderer(sessionContext.GetNotificationClient() != nullptr)
{
	m_render_session_context.GetRenderSettings().RegisterChangeNotifier(*this);
}

MaxToLux::MaxToLux(IRenderSessionContext& sessionContext, const bool is_interactive_session)
	: m_rendering_process(sessionContext.GetRenderingProcess())
{
	file = NULL;
	sceneNode = NULL;
	viewNode = NULL;
	anyLights = FALSE;
	nlts = nobs = 0;
}

RefResult MaxToLux::NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget, PartID &partID,
	RefMessage message, BOOL propagate)
{
	UNREFERENCED_PARAMETER(propagate);
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(partID);
	UNREFERENCED_PARAMETER(hTarget);
	UNREFERENCED_PARAMETER(changeInt);
	switch (message)
	{
		case REFMSG_CHANGE:
		{
			if (hTarget == pblock)
			{
				ParamID changing_param = pblock->LastNotifyParamID();
				DepthOfFieldblk.InvalidateUI(changing_param);
			}
			break;
		}
	}
	return REF_SUCCEED;
}

::Matrix3 camPos;

int MaxToLux::Open(INode *scene, INode *vnode, ViewParams *viewPar, RendParams &rp, HWND hwnd, DefaultLight* defaultLights, int numDefLights, RendProgressCallback* prog)
{
	UNREFERENCED_PARAMETER(prog);
	UNREFERENCED_PARAMETER(numDefLights);
	UNREFERENCED_PARAMETER(defaultLights);
	UNREFERENCED_PARAMETER(hwnd);
	
	viewNode = vnode;
	camPos = viewPar->affineTM;

	if (rp.inMtlEdit)
	{
		
		renderingMaterialPreview = true;
		Scene* materialPreviewScene = NULL;// *= new Scene();
		Scene *materialPreviewScene = Scene::Create();
		materialPreviewScene = Scene::Create();

		lxmMesh.createMesh(scene, *materialPreviewScene, GetCOREInterface()->GetTime(), renderingMaterialPreview);
		lxmMesh.CreateBox(scene, materialPreviewScene, "dummybox", "dummyboxmesh", "mat_dummy", false, BBox(Point(-.001f, -.001f, .001f), Point(.05f, .05f, 0.07f)));
	}
	else
	{
		renderingMaterialPreview = false;
	}

	return 1;
}