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

#include "Common.h"
#include "ClassDescs.h"
#include "utils\Thread.h"
#include "utils/Utils.h"
#include "ParamDlg.h"
#include <ITabDialog.h>
#include <Notify.h>
#include <toneop.h>
#if MAX_PRODUCT_YEAR_NUMBER >= 2016
#   include <scene/IPhysicalCamera.h>
#endif

#pragma warning(push, 3)
#pragma warning(disable:4198)
#include <Math/mathutils.h>
#pragma warning(pop)
#include "ParamBlock.h"
#include "imenuman.h"
#include <memory>
#include <shlobj.h>
#include <stack>
#include <WinUser.h>
#include <resource.h>
#include <bitmap.h>
#include <notify.h>
#include <gamma.h> // gamma export for FRS files
#include <IPathConfigMgr.h>  // for IPathConfigMgr

EventSpin ActiveShader::mGlobalLocker;

lass ActiveShadeBitmapWriter;

class ActiveShadeRenderCore : public BaseThread
{
public:
	// termination
	Event terminationReached;
	std::atomic<DWORD> timeLimit = 100;
	std::atomic<unsigned int> passLimit = 1;
	std::atomic<unsigned int> passesDone = 0;
	std::atomic<unsigned int> restartsDone = 0;

	volatile TerminationCriteria termination = Termination_None;
	typedef enum
	{
		Result_OK,
		Result_Aborted,
		Result_Catastrophic
	} TerminationResult;
	Event mRenderThreadExit; // fired when the thread's worker is about to exit, for whichever reason
	Event mShaderCacheReady;

	volatile bool isNormals = false;
	::Bitmap *outputBitmap = 0;
	int renderDevice = 0;
	volatile float exposure = 1.f;
	volatile bool useMaxTonemapper = true;
	ActiveShadeBitmapWriter *writer = 0;

private:
	bool mAlphaEnabled;
	Event frameBufferAlphaEnable;
	Event frameBufferAlphaDisable;
	bool mAdaptiveEnabled;
	CriticalSection bufSec;
	Event eRestart;

	bool DumpFrameBuffer(bool force);

	CriticalSection cameraSec;
	Event cameraChanged;
	ParsedView curView;
	CriticalSection regionSec;
	Event regionChanged;
	int xmin, ymin, xmax, ymax; // used for region rendering, define a rectangle on the framebuffer
	Box2 region;
	bool regionRender = false;
	CriticalSection ctxSec;

	CriticalSection toneMapperSec;
	Event toneMapperChanged; // fired when useMaxToneMapper is being changed

	ActiveShader *activeShader;

private:
	void SetupCamera(const ParsedView& view, const int imageWidth, const int imageHeight, rpr_camera outCamera);

public:

	inline void Lock()
	{
		ctxSec.Lock();
	}

	inline void Unlock()
	{
		ctxSec.Unlock();
	}

	inline ScopeLock GetLock()
	{
		return ScopeLock(ctxSec);
	}

	inline void StartBlit()
	{
		bufSec.Lock();
	}

	inline void EndBlit()
	{
		bufSec.Unlock();
	}

	inline void StartToneMapper()
	{
		toneMapperSec.Lock();
		useMaxTonemapper = false;
		toneMapperSec.Unlock();
		toneMapperChanged.Fire();
	}

	inline void StopToneMapper()
	{
		toneMapperSec.Lock();
		useMaxTonemapper = true;
		toneMapperSec.Unlock();
		toneMapperChanged.Fire();
	}

	inline void SetToneMappingExposure(float val)
	{
		exposure = val;
	}


	inline void SetTimeLimit(int val)
	{
		if (val <= 1)
			val = 1;
		DWORD uval = val;
		uval *= 1000; // seconds to milliseconds
		bool checkReset = (termination == Termination_Time) || (termination == Termination_PassesOrTime);
		bool reset = (checkReset && (uval > timeLimit));
		timeLimit = uval;
		if (reset)
			terminationReached.Reset();
	}

	inline void SetLimitType(int type)
	{
		termination = (TerminationCriteria)type;
	}

	inline void ResetTermination()
	{
		terminationReached.Reset();
	}

	void Worker() override;

	void Restart();

#pragma optimize("", off)
	bool SetView(ParsedView &view)
	{
		bool needResetScene;
		if (!curView.isSame(view, &needResetScene))
		{
			cameraSec.Lock();
			curView = view;
			cameraSec.Unlock();
			cameraChanged.Fire();
			return true;
		}
		return false;
	}

#pragma optimize("", on)
		regionSec.Unlock();
		regionChanged.Fire();
	}
};