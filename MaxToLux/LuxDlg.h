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

	bool lightsTraceChk, clampingChk;
	int totalPathDepth, diffuseDepth, glossyDepth, specularDepth;
	float lightRays, glossynessThreshold, maxBrightness;

	float luxLinearSenssitivity, luxLinearExposure, luxLinearFstop;
	float reinHardPrescale, reinHardPostscale, reinHardBurn;
	float linearScale;

	bool bloom, vignetting, gaussianFilter, colorAberration, mist, mistBackground, contourLine;
	float bloomSensitivity, bloomExposure;
	float vignettingValue, vignettingTableSize;
	float gaussianScale;
	float colorAberRadius, colorAberWeight;
	float mistColor, mistAmount, mistStartDistance, mistEndDistance;
	float contourRange, contourScale, contourSteps, contourGridSize;

	//float LensRadius;
	TSTR LensRadiusWstr = L"33";
	float LensRadiusFloatTmp = 0.0f;
	int  rendertype, samplerIndex, lightStrategyIndex, filterIndex;
	TSTR rendertypeWstr;
	//TSTR Gpu1;

	float filterXvalue, filterYvalue;
	float filterGuassianAlphavaluem, filterMitchellAvalue, filterGuassianAlphavalue, filterMitchellBvalue;
	float MetropolisLargestEpRatevalue, MetrolpolisImageMutationRatevalue;
	int MetropolisMaxConsecutiveRejectvalue;
	bool defaultlightchk;
	bool defaultlightauto;
	bool enableFileSaverOutput = false;

	short 	type;

	ISpinnerControl *totalPathDepthSpin, *diffuseDepthSpin, *glossyDepthSpin, *specularDepthSpin;
	ISpinnerControl *lightRaysSpin, *glossynessThresholdSpin, *maxBrightnessSpin;

	ISpinnerControl *renderTimeSpin, *renderPassSpin, *renderRefreshSpin;
	ISpinnerControl *gammaValueSpin, *gammaTableSizeSpin;
	ISpinnerControl *luxLinearSenssitivitySpin, *luxLinearExposureSpin, *luxLinearFstopSpin;
	ISpinnerControl *reinHardPrescaleSpin, *reinHardPostscaleSpin, *reinHardBurnSpin;
	ISpinnerControl *linearScaleSpin;

	ISpinnerControl *bloomSensitivitySpin, *bloomExposureSpin;
	ISpinnerControl *vignettingValueSpin, *vignettingTableSizeSpin;
	ISpinnerControl *gaussianScaleSpin;
	ISpinnerControl *colorAberRadiusSpin, *colorAberWeightSpin;
	ISpinnerControl *mistColorSpin, *mistAmountSpin, *mistStartDistanceSpin, *mistEndDistanceSpin;
	ISpinnerControl *contourRangeSpin, *contourScaleSpin, *contourStepsSpin, *contourGridSizeSpin;

	ISpinnerControl *renderNoiseThresholdSpin;
	ISpinnerControl *depthSpinner;
	ISpinnerControl *filterXSpinner;
	ISpinnerControl *filterYSpinner;
	ISpinnerControl *filterGuassianAlphaSpinner;
	ISpinnerControl *filterMitchellASpinner;
	ISpinnerControl *filterMitchellBSpinner;
	ISpinnerControl* metropolisLargestEpRateSpinner;
	ISpinnerControl* metropolisMaxConsecutiveSpin;
	ISpinnerControl* metropolisImageMutationRateSpinner;

	Texmap 			*projMap;   		// a reference
	ICustButton		*envirenmentMapBT;
	Texmap 			*envirenmentMap;   	// a reference

	ICustButton *projMapName;

	MaxToLuxParamDlg(IRendParams *i, BOOL prog, MaxToLux *renderer);
	~MaxToLuxParamDlg();
	void	AcceptParams();
	RefResult NotifyRefChanged(const Interval & changeInt, RefTargetHandle hTarget, PartID & partID, RefMessage message, BOOL propagate);
	void	DeleteThis() { delete this; }
	void	InitParamDialog(HWND hWnd);
	void	InitProgDialog(HWND hWnd);
	void	InitLightTracingDialog(HWND hWnd);
	void	InitEnvirenmentDialog(HWND hWnd);
	void	InitPostProccesDialog(HWND hWnd);
	void	InitPostFilterDialog(HWND hWnd);
	void	InitDepthDialog(HWND hWnd);
	void	InitFilterDialog(HWND hWnd);
	void	InitSamplerDialog(HWND hWnd);
	void	ReleaseControls() {}
	BOOL	FileBrowse();
	void 	BrowseProjectorMap(HWND hWnd);
	void 	AssignProjectorMap(Texmap *m, BOOL newmat, ICustButton *envirenmentMapBT);

	INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};