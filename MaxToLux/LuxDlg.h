//**************************************************************************/
// Copyright (c) 2015-2024 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/

#pragma once

#include "main.h"
#include "MaxToLux.h"
#include <maxscript\maxscript.h>
#include "3dsmaxport.h"
#include <string>
#include "Classes.h""

#undef min
#undef max

class MaxToLuxParamDlg : public RendParamDlg, public ReferenceMaker {
public:
	MaxToLux *rend;
	IRendParams *ir;
	HWND hPanel;
	HWND MaxToLuxParamDlg::hAdvEff = NULL;
	//HWND hDlg;
	BOOL prog;
	HFONT hFont;

	static IObjParam *iObjParams;

	std::vector<int> renderSelectdDevice;
	bool deviceArray1 = true, deviceArray2, deviceArray3, deviceArray4, deviceArray5 = false;

	TSTR workFileName;
	bool gammaEnable, autoLinear, luxLinear, reinHard, linear, denoiser, denoiserAllSteps;
	int haltTime, refereshTime, haltSpp;
	float haltThreshold, gammaValue;
	int gammaTableSize;

    //float LensRadius;
	TSTR LensRadiusWstr = L"33";
	float LensRadiusFloatTmp = 0.0f;
	int  rendertype, samplerIndex, lightStrategyIndex, filterIndex;
	TSTR rendertypeWstr;
	//TSTR Gpu1;

};