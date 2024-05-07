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

#pragma once

#include "parser/Synchronizer.h"
#include "ParamBlock.h"
#include <iparamb2.h>
#include <interactiverender.h>
#include <atomic>


class ActiveShadeRenderCore;
class ActiveShader;
class ActiveShadeBitmapWriter;

class ActiveShadeSynchronizerBridge : public SynchronizerBridge
{
private:
	class ActiveShader *mActiveShader;
	
public:
	
	ActiveShadeSynchronizerBridge(class ActiveShader *as)
		: mActiveShader(as)
	{
	}

	inline const TimeValue t() override
	{
		return GetCOREInterface()->GetTime();
	}

	void LockRenderThread() override;
	void UnlockRenderThread() override;
	void StartToneMapper() override;
	void StopToneMapper() override;
	void SetToneMappingExposure(float exposure) override;
	void ClearFB() override;
	IRenderProgressCallback *GetProgressCB() override;
	IParamBlock2 *GetPBlock() override;
	bool RenderThreadAlive() override;
	const DefaultLight *GetDefaultLights(int &numDefLights) const override;
	void EnableAlphaBuffer();
	void DisableAlphaBuffer();
	void SetTimeLimit(int val);
	void SetPassLimit(int val);
	void SetLimitType(int type);
	void ResetInteractiveTermination();
};

protected:
	ParsedView ParseView();

	void CustomCPUSideSynch() override;
	class ActiveShader *mActiveShader;
};
