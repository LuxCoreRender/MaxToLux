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

#ifndef MaxToLux__H
#define MaxToLux__H
#include "maxtextfile.h"
#include <iostream>
#include <string>
#include <luxcore.h>
#include <StopWatch.h>
#include <ITabDialog.h>
#include <interactiverender.h>
#include <ISceneEventManager.h>

#define REND_CLASS_ID Class_ID(98,0);

#if MAX_PRODUCT_YEAR_NUMBER >= 2017
#define MAX2017_OVERRIDE override
#else
#define MAX2017_OVERRIDE
#endif

class MaxToLux;

// Render parameters. Add whatever parameters you need here.
// These are typically parameters that needs to be accessed during the
// setup of the renderer and during the rendering.
class LuxRenderParams : public RenderGlobalContext {
public:
	RendType	rendType;				// View, blowup, region etc.
	int			nMinx;
	int			nMiny;
	int			nMaxx;
	int			nMaxy;
	int			nNumDefLights;			// The default lights passed into the renderer
	int			nRegxmin;				// Coords for render blowup etc.
	int			nRegxmax;				// Coords for render blowup etc.
	int			nRegymin;				// Coords for render blowup etc.
	int			nRegymax;				// Coords for render blowup etc.
	//Point2		scrDUV;
	//BitArray	gbufChan;				// The G buffer channels (bitflags)
	//DefaultLight*	pDefaultLights;
	//FrameRendParams*	pFrp;			// Frame specific members
	//RendProgressCallback*	progCallback;	// Render progress callback

	//GBufReader*	gbufReader;
	//GBufWriter*	gbufWriter;

	// Custom options
	// These options are specific to the sample renderer
	int			nMaxDepth;
	int			lenser;
	int			nAntiAliasLevel;
	BOOL		bReflectEnv;

	// Standard options
	// These options are configurable for all plugin renderers
	BOOL		bVideoColorCheck;
	BOOL		bForce2Sided;
	BOOL		bRenderHidden;
	BOOL		bSuperBlack;
	BOOL		bRenderFields;
	BOOL		bNetRender;

	// Render effects
	//Effect*		effect;

	LuxRenderParams();
	void		SetDefaults();
	//void		ComputeViewParams(const ViewParams&vp);
	//Point3		RayDirection(float sx, float sy);

	//int				NumRenderInstances();
	//RenderInstance*	GetRenderInstance(int i);
};
//using namespace MaxSDK::RenderingAPI;


class IMaxToLux : public IInteractiveRender, ReferenceMaker, ActionCallback
{
public:
	void AbortRender() MAX2017_OVERRIDE;

private:
	IParamBlock2* pblock;

public:
	///
	IMaxToLux(MaxToLux* luxRen);
	~IMaxToLux();
	void BeginSession() override;
	void EndSession() override;
	BOOL IsRendering()  override;
#if MAX_PRODUCT_YEAR_NUMBER >= 2015
	BOOL AnyUpdatesPending() override;
#endif

	void OnNotify(NotifyInfo* info);

	static void NotifyCallback(void *param, NotifyInfo *info)
	{
		auto self = static_cast<IMaxToLux*>(param);
		self->OnNotify(info);
	}

public:
	MaxToLux* luxRender = nullptr;

	IIRenderMgr *pIIRenderMgr;

	bool IsRunning() const
	{
		//if (mActiveShader)
			//return mActiveShader->IsRunning();
		return false;
	}
};

class MaxToLux : public Renderer, public ITabDialogObject {
	public:

		CRITICAL_SECTION csect;
	};
	public:
		inline bool bool_cast(int x) { return (x ? true : false); }
		bool InActiveShade()
		{
			return bool_cast(isActiveShade);
		}
#endif