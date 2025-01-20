//**************************************************************************/
// Copyright (c) 2015-2019 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/

#include "main.h"
#include "object.h"
#include "gencam.h"
#include "camera.h"
#include "target.h"
#include "macrorec.h"
#include "decomp.h"
#include "iparamb2.h"
#include "MouseCursors.h"
#include "Max.h"
#include <maxscript/maxscript.h>
#include "bitmap.h"
#include "guplib.h"
#include "gup.h"

#include "ILinkTMCtrl.h"
#include "3dsmaxport.h"
#include <operationdesc.h>

#include <Graphics/Utilities/MeshEdgeRenderItem.h>
#include <Graphics/Utilities/SplineRenderItem.h>
#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/RenderNodeHandle.h>
#include <Graphics/IDisplayManager.h>

#define LUX_CAMERA_CLASS_ID				Class_ID(0x20862d55, 0x43525249)
#define LUX_CAMERA_LOOKAT_CLASS_ID		Class_ID(0x56c93be7, 0x47002ec9)

// Parameter block indices
#define PB_FOV						0
#define PB_TDIST					1
#define PB_HITHER					2
#define PB_YON						3
#define PB_NRANGE					4
#define PB_FRANGE					5
#define PB_MP_EFFECT_ENABLE			6	// mjm - 07.17.00
#define PB_MP_EFF_REND_EFF_PER_PASS	7	// mjm - 07.17.00
#define PB_FOV_TYPE					8	// MC  - 07.17.07

// Depth of Field parameter block indicies
#define PB_DOF_ENABLE	0
#define PB_DOF_FSTOP	1

#define MIN_FSTOP		0.0001f
#define MAX_FSTOP		100.0f

#define WM_SET_TYPE		WM_USER + 0x04002

#define MIN_CLIP	0.000001f
#define MAX_CLIP	1.0e32f

#define NUM_CIRC_PTS	28
#define SEG_INDEX		7

#define RELEASE_SPIN(x)   if (so->x) { ReleaseISpinner(so->x); so->x = NULL;}
#define RELEASE_BUT(x)   if (so->x) { ReleaseICustButton(so->x); so->x = NULL;}

static int waitPostLoad = 0;
static void resetCameraParams();
static HIMAGELIST hCamImages = NULL;

//------------------------------------------------------
class SimpleCamClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new SimpleCamera; }
	const TCHAR *	ClassName() { return GetString(IDS_DB_FREE_CLASS); }
	SClass_ID		SuperClassID() { return CAMERA_CLASS_ID; }
	Class_ID 		ClassID() { return LUX_CAMERA_CLASS_ID; }
	const TCHAR* 	Category() { return _T("MaxToLux");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetCameraParams(); }
	};

static SimpleCamClassDesc simpleCamDesc;

ClassDesc* GetSimpleCamDesc() { return &simpleCamDesc; }


// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

#define DEF_NEAR_CLIP		1.0f
#define DEF_FAR_CLIP		1000.0f

// Class variables of SimpleCamera
Mesh SimpleCamera::mesh;
short SimpleCamera::meshBuilt=0;
float SimpleCamera::dlgFOV = DegToRad(45.0);
int   SimpleCamera::dlgFOVType = FOV_W;
short SimpleCamera::dlgShowCone =0;
short SimpleCamera::dlgShowHorzLine =0;
float SimpleCamera::dlgTDist = FIXED_CONE_DIST;
short SimpleCamera::dlgClip = 0;
float SimpleCamera::dlgNearher = DEF_NEAR_CLIP;
float SimpleCamera::dlgFar = DEF_FAR_CLIP;
float SimpleCamera::dlgNearRange = 0.0f;
float SimpleCamera::dlgFarRange = 1000.0f;
short SimpleCamera::dlgRangeDisplay = 0;
short SimpleCamera::dlgIsOrtho = 0;

short SimpleCamera::dlgDOFEnable = 1;
float SimpleCamera::dlgDOFFStop = 2.0f;
short SimpleCamera::dlgMultiPassEffectEnable = 0;
//short SimpleCamera::dlgMPEffect_REffectPerPass = 0;
Tab<ClassEntry*> SimpleCamera::smCompatibleEffectList;

BOOL SimpleCamera::inCreate=FALSE;

void resetCameraParams() 
{
	SimpleCamera::dlgFOV = DegToRad(45.0);
	SimpleCamera::dlgFOVType = FOV_W;
	SimpleCamera::dlgShowCone =0;
	SimpleCamera::dlgShowHorzLine =0;
	SimpleCamera::dlgTDist = FIXED_CONE_DIST;
	SimpleCamera::dlgClip = 0;
	SimpleCamera::dlgNearher = DEF_NEAR_CLIP;
	SimpleCamera::dlgFar = DEF_FAR_CLIP;
	SimpleCamera::dlgNearRange = 0.0f;
	SimpleCamera::dlgFarRange = 1000.0f;
	SimpleCamera::dlgRangeDisplay = 0;
	SimpleCamera::dlgIsOrtho = 0;

	SimpleCamera::dlgDOFEnable = 1;
	SimpleCamera::dlgDOFFStop = 2.0f;
	SimpleCamera::dlgMultiPassEffectEnable = 0;
	//SimpleCamera::dlgMPEffect_REffectPerPass = 0;
}

SimpleCamera * SimpleCamera::currentEditCam = NULL;
HWND SimpleCamera::hSimpleCamParams = NULL;
HWND SimpleCamera::hDepthOfFieldParams = NULL;
IObjParam *SimpleCamera::iObjParams;
ISpinnerControl *SimpleCamera::fovSpin = NULL;
ISpinnerControl *SimpleCamera::lensSpin = NULL;
ISpinnerControl *SimpleCamera::tdistSpin = NULL;
ISpinnerControl *SimpleCamera::nearSpin = NULL;
ISpinnerControl *SimpleCamera::farSpin = NULL;
ISpinnerControl *SimpleCamera::envNearSpin = NULL;
ISpinnerControl *SimpleCamera::envFarSpin = NULL;
ISpinnerControl *SimpleCamera::fStopSpin = NULL;
ICustButton *SimpleCamera::iFovType = NULL;

static float mmTab[9] = {
	15.0f, 20.0f, 24.0f, 28.0f, 35.0f, 50.0f, 85.0f, 135.0f, 200.0f
	};


static float GetAspect() {
	return GetCOREInterface()->GetRendImageAspect();
	};

static float GetApertureWidth() {
	return GetCOREInterface()->GetRendApertureWidth();
	}


static int GetTargetPoint(TimeValue t, INode *inode, Point3& p) {
	Matrix3 tmat;
	if (inode && inode->GetTargetTM(t,tmat)) {
		p = tmat.GetTrans();
		return 1;
		}
	else 
		return 0;
	}

static int is_between(float a, float b, float c) 
{
	float t;
	if (b>c) { t = b; b = c; c = t;}
	return((a>=b)&&(a<=c));
}

static float interp_vals(float m, float *mtab, float *ntab, int n) 
{
	float frac;
	for (int i=1; i<n; i++) {
		if (is_between(m,mtab[i-1],mtab[i])) {
			frac = (m - mtab[i-1])/(mtab[i]-mtab[i-1]);
			return((1.0f-frac)*ntab[i-1] + frac*ntab[i]);
		}
	}
	return 0.0f;
}

class CameraMeshItem : public MaxSDK::Graphics::Utilities::MeshEdgeRenderItem
{
public:
	CameraMeshItem(Mesh* pMesh)
		: MeshEdgeRenderItem(pMesh, true, false)
	{

	}
	virtual ~CameraMeshItem()
	{
	}
	virtual void Display(MaxSDK::Graphics::DrawContext& drawContext)
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if (vpt->GetViewCamera() == drawContext.GetCurrentNode())
		{
			return;
		}
		__super::Display(drawContext);
	}

	virtual void HitTest(MaxSDK::Graphics::HitTestContext& hittestContext, MaxSDK::Graphics::DrawContext& drawContext)
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if (vpt->GetViewCamera() == drawContext.GetCurrentNode())
		{
			return;
		}
		__super::HitTest(hittestContext, drawContext);
	}
};

class CameraConeItem : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
protected:
	SimpleCamera* mpCamera;
public:
	CameraConeItem(SimpleCamera* cam)
		: mpCamera(cam)
	{

	}
	~CameraConeItem()
	{
		mpCamera = nullptr;
	}

#define CAM_NEAR_CLIP         1	//!< The hither/near distance
#define CAM_FAR_CLIP            2	//!< The yon/far distance.
	
	void BuildCone(TimeValue t, float dist, int colid, BOOL drawSides, BOOL drawDiags, Color color) {
		Point3 posArray[5], tmpArray[2];
		mpCamera->GetConePoints(t, posArray, dist);

		if (colid)
		{
			color = GetUIColor(colid);
		}
		if (drawDiags) {
			tmpArray[0] =  posArray[0];	
			tmpArray[1] =  posArray[2];	
			AddLineStrip(tmpArray, color, 2, false, false);
			tmpArray[0] =  posArray[1];	
			tmpArray[1] =  posArray[3];	
			AddLineStrip(tmpArray, color, 2, false, false);
		}
		AddLineStrip(posArray, color, 4, true, false);
		if (drawSides) {
			color = GetUIColor(COLOR_CAMERA_CONE);
			tmpArray[0] = Point3(0,0,0);
			for (int i=0; i<4; i++) {
				tmpArray[1] = posArray[i];	
				AddLineStrip(tmpArray, color, 2, false, false);
			}
		}
	}
	void BuildConeAndLine(TimeValue t, INode* inode, Color& color)
	{
		if (nullptr == mpCamera)
		{
			DbgAssert(!_T("Invalid camera object!"));
			return;
		}
		Matrix3 tm = inode->GetObjectTM(t);
		if (mpCamera->hasTarget) {
			Point3 pt;
			if (GetTargetPoint(t, inode, pt)){
				float den = Length(tm.GetRow(2));
				float dist = (den!=0)?Length(tm.GetTrans()-pt)/den : 0.0f;
				mpCamera->targDist = dist;
				if (mpCamera->hSimpleCamParams 
					&& (mpCamera->currentEditCam == mpCamera)) 
				{
					mpCamera->tdistSpin->SetValue(mpCamera->GetTDist(t), FALSE);
				}
				if (mpCamera->coneState || (mpCamera->extDispFlags & EXT_DISP_ONLY_SELECTED)) 
				{
					if(mpCamera->manualClip) {
						BuildCone(t, mpCamera->GetClipDist(t, CAM_NEAR_CLIP), COLOR_CAMERA_CLIP, FALSE, TRUE, color);
						BuildCone(t, mpCamera->GetClipDist(t, CAM_FAR_CLIP), COLOR_CAMERA_CLIP, TRUE, TRUE, color);
					}
					else
					{
						BuildCone(t, dist, COLOR_CAMERA_CONE, TRUE, FALSE, color);
					}
				}
			}
		}
		else {
			if (mpCamera->coneState || (mpCamera->extDispFlags & EXT_DISP_ONLY_SELECTED))
				if(mpCamera->manualClip) {
					BuildCone(t, mpCamera->GetClipDist(t, CAM_NEAR_CLIP), COLOR_CAMERA_CLIP, FALSE, TRUE, color);
					BuildCone(t, mpCamera->GetClipDist(t, CAM_FAR_CLIP), COLOR_CAMERA_CLIP, TRUE, TRUE, color);
				}
				else
					BuildCone(t, mpCamera->GetTDist(t), COLOR_CAMERA_CONE, TRUE, FALSE, color);
		}
	}

	void BuildRange(TimeValue t, INode* inode, Color& color)
	{
		if(!mpCamera->rangeDisplay)
			return;
		Matrix3 tm = inode->GetObjectTM(t);
		int cnear = 0;
		int cfar = 0;
		if(!inode->IsFrozen() && !inode->Dependent()) { 
			cnear = COLOR_NEAR_RANGE;
			cfar = COLOR_FAR_RANGE;
		}
		BuildCone(t, mpCamera->GetEnvRange(t, ENV_NEAR_RANGE),cnear, FALSE, FALSE, color);
		BuildCone(t, mpCamera->GetEnvRange(t, ENV_FAR_RANGE), cfar, TRUE, FALSE, color);
	}
	
	virtual void Realize(MaxSDK::Graphics::DrawContext& drawContext) 
	{
		INode* inode = drawContext.GetCurrentNode();
		if (nullptr == inode)
		{
			return;
		}
		inode->SetTargetNodePair(0);
		mpCamera->SetExtendedDisplay(drawContext.GetExtendedDisplayMode());
		ClearLines();
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if ( !vpt 
			|| !vpt->IsAlive() 
			|| !mpCamera->enable)
		{
			return;
		}

		Color color(inode->GetWireColor());
		if (inode->Dependent())
		{
			color = ColorMan()->GetColorAsPoint3(kViewportShowDependencies);
		}
		else if (inode->Selected()) 
		{
			color = GetSelColor();
		}
		else if (inode->IsFrozen())
		{
			color = GetFreezeColor();
		}

		BuildConeAndLine(drawContext.GetTime(), inode, color);
		BuildRange(drawContext.GetTime(), inode, color);
		SplineRenderItem::Realize(drawContext);
	}

	virtual void Display(MaxSDK::Graphics::DrawContext& drawContext)
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if (vpt->GetViewCamera() == drawContext.GetCurrentNode())
		{
			return;
		}
		__super::Display(drawContext);
	}

	virtual void HitTest(MaxSDK::Graphics::HitTestContext& /*hittestContext*/, MaxSDK::Graphics::DrawContext& /*drawContext*/)
	{
		//We don't need cone item been hit
	}
};


class CameraTargetLineItem : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
protected:
	Color mLastColor;
	float mLastDist;
public:
	CameraTargetLineItem()
		: mLastDist(-1.0f)
	{
		mLastColor = Color(0,0,0);
	}
	~CameraTargetLineItem()
	{
	}

	virtual void Realize(MaxSDK::Graphics::DrawContext& drawContext) 
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		INode* inode = drawContext.GetCurrentNode();
		if ( !vpt 
			|| !vpt->IsAlive() )
		{
			return;
		}

		if(nullptr == inode)
		{
			return;
		}

		Color color(inode->GetWireColor());
		if (inode->Dependent())
		{
			color = ColorMan()->GetColorAsPoint3(kViewportShowDependencies);
		}
		else if (inode->Selected()) 
		{
			color = GetSelColor();
		}
		else if (inode->IsFrozen())
		{
			color = GetFreezeColor();
		}
		
		TimeValue t = drawContext.GetTime();
		Matrix3 tm = inode->GetObjectTM(t);
		Point3 pt;
		if (GetTargetPoint(t, inode, pt)){
			float den = Length(tm.GetRow(2));
			float dist = (den!=0)?Length(tm.GetTrans()-pt)/den : 0.0f;
			Color lineColor(inode->GetWireColor());
			if(!inode->IsFrozen() && !inode->Dependent())
			{
				// 6/25/01 2:33pm --MQM--
				// if user has changed the color of the camera,
				// use that color for the target line too
				if ( lineColor == GetUIColor(COLOR_CAMERA_OBJ) )
					lineColor = GetUIColor(COLOR_TARGET_LINE);
			}
			if(mLastColor != lineColor
				|| mLastDist != dist)
			{
				ClearLines();
				Point3 v[2] = {Point3(0,0,0), Point3(0.0f, 0.0f, -dist)};
				AddLineStrip(v, lineColor, 2, false, false);
				v[0].z = -0.02f * dist;
				AddLineStrip(v, lineColor, 2, false, true);
				mLastColor = lineColor;
				mLastDist = dist;
			}
		}

		SplineRenderItem::Realize(drawContext);
	}

	virtual void Display(MaxSDK::Graphics::DrawContext& drawContext)
	{
		return;
	}

    //! [INode.SetTargetNodePair Example]
	virtual void HitTest(MaxSDK::Graphics::HitTestContext& hittestContext, MaxSDK::Graphics::DrawContext& drawContext)
	{
		ViewExp* vpt = const_cast<ViewExp*>(drawContext.GetViewExp());
		if (vpt->GetViewCamera() == drawContext.GetCurrentNode())
		{
			return;
		}
		drawContext.GetCurrentNode()->SetTargetNodePair(0);
		__super::Display(drawContext);
	}
    //! [INode.SetTargetNodePair Example]

	virtual void OnHit(MaxSDK::Graphics::HitTestContext& /*hittestContext*/, MaxSDK::Graphics::DrawContext& drawContext)
	{
		drawContext.GetCurrentNode()->SetTargetNodePair(1);
	}
};

// This changes the relation between FOV and Focal length.
void SimpleCamera::RenderApertureChanged(TimeValue t) {
	UpdateUI(t);
	}

float SimpleCamera::MMtoFOV(float mm) {
	return float(2.0f*atan(0.5f*GetApertureWidth()/mm));
	}

float SimpleCamera::FOVtoMM(float fov)	{
	float w = GetApertureWidth();
	float mm = float((0.5f*w)/tan(fov/2.0f));
	return mm;
	}


float SimpleCamera::CurFOVtoWFOV(float cfov) {
	switch (GetFOVType()) {
		case FOV_H: {
			return float(2.0*atan(GetAspect()*tan(cfov/2.0f)));
			}
		case FOV_D: {
			float w = GetApertureWidth();
			float h = w/GetAspect();
			float d = (float)sqrt(w*w + h*h);	
			return float(2.0*atan((w/d)*tan(cfov/2.0f)));
			}
		default:
			return cfov;
		}
	}	


float SimpleCamera::WFOVtoCurFOV(float fov) {
	switch (GetFOVType()) {
		case FOV_H: {
			return float(2.0*atan(tan(fov/2.0f)/GetAspect()));
			}
		case FOV_D: {
			float w = GetApertureWidth();
			float h = w/GetAspect();
			float d = (float)sqrt(w*w + h*h);	
			return float(2.0*atan((d/w)*tan(fov/2.0f)));
			}
		default:
			return fov;
		}
	}	


static void LoadCamResources() {
	static BOOL loaded=FALSE;
	if (loaded) return;
	HBITMAP hBitmap;
	HBITMAP hMask;
	hCamImages = ImageList_Create(14,14, ILC_COLOR4|ILC_MASK, 3, 0);
	hBitmap = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_FOV));
	hMask   = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_FOVMASK));
	ImageList_Add(hCamImages,hBitmap,hMask);
	DeleteObject(hBitmap);	
	DeleteObject(hMask);	
	}

class DeleteCamResources {
	public:
		~DeleteCamResources() {
			ImageList_Destroy(hCamImages);
			}
	};

static DeleteCamResources theDelete;

static int typeName[NUM_CAM_TYPES] = {
	IDS_DB_FREE_CAM,
	IDS_DB_TARGET_CAM
	};

INT_PTR CALLBACK SimpleCamParamDialogProc( 
	HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
	{
   SimpleCamera *so = DLGetWindowLongPtr<SimpleCamera *>( hDlg);
	if ( !so && message != WM_INITDIALOG ) return FALSE;
	TimeValue t = so->iObjParams->GetTime();

	float tmpSmall, tmpLarge;

	switch ( message )
	{
		case WM_INITDIALOG:
		{
			LoadCamResources();
			so = (SimpleCamera *)lParam;
			DLSetWindowLongPtr( hDlg, so);
			SetDlgFont( hDlg, so->iObjParams->GetAppHFont() );
			
			
			CheckDlgButton( hDlg, IDC_SHOWCAMCONE, so->coneState );
			CheckDlgButton( hDlg, IDC_SHOWHORZLINE, so->horzLineState );
			CheckDlgButton( hDlg, IDC_SHOW_RANGES, so->rangeDisplay );
			CheckDlgButton( hDlg, IDC_IS_ORTHO, so->isOrtho );
			CheckDlgButton( hDlg, IDC_MANUAL_CLIP, so->manualClip );
			EnableWindow( GetDlgItem(hDlg, IDC_HITHER), so->manualClip);
			EnableWindow( GetDlgItem(hDlg, IDC_NEARSPINNER), so->manualClip);
			EnableWindow( GetDlgItem(hDlg, IDC_YON), so->manualClip);
			EnableWindow( GetDlgItem(hDlg, IDC_FARSPINNER), so->manualClip);

			HWND hwndType = GetDlgItem(hDlg, IDC_CAM_TYPE);
			int i;
			for (i=0; i<NUM_CAM_TYPES; i++)
				SendMessage(hwndType, CB_ADDSTRING, 0, (LPARAM)GetString(typeName[i]));
			SendMessage( hwndType, CB_SETCURSEL, i, (LPARAM)0 );
			EnableWindow(hwndType,!so->inCreate);		

			CheckDlgButton( hDlg, IDC_DOF_ENABLE, so->GetDOFEnable(t) );

			CheckDlgButton(hDlg, IDC_ENABLE_MP_EFFECT, so->GetMultiPassEffectEnabled(t) );
			CheckDlgButton(hDlg, IDC_MP_EFFECT_REFFECT_PER_PASS, so->GetMPEffect_REffectPerPass() );

			// build list of multi pass effects
			int selIndex = -1;
			IMultiPassCameraEffect *pIMultiPassCameraEffect = so->GetIMultiPassCameraEffect();
			HWND hEffectList = GetDlgItem(hDlg, IDC_MP_EFFECT);
			SimpleCamera::FindCompatibleMultiPassEffects(so);
			int numClasses = SimpleCamera::smCompatibleEffectList.Count();
			for (int i=0; i<numClasses; i++)
			{
				int index = SendMessage( hEffectList, CB_ADDSTRING, 0, (LPARAM)SimpleCamera::smCompatibleEffectList[i]->CD()->ClassName() );
				if ( pIMultiPassCameraEffect && ( pIMultiPassCameraEffect->ClassID() == SimpleCamera::smCompatibleEffectList[i]->CD()->ClassID() ) )
				{
					selIndex = index;
				}
			}
			SendMessage(hEffectList, CB_SETCURSEL, selIndex, (LPARAM)0);
			EnableWindow( GetDlgItem(hDlg, IDC_PREVIEW_MP_EFFECT), so->GetMultiPassEffectEnabled(t) );
			return FALSE;
		}

		case WM_DESTROY:
			RELEASE_SPIN ( fovSpin );
			RELEASE_SPIN ( lensSpin );
			RELEASE_SPIN ( nearSpin );
			RELEASE_SPIN ( farSpin );
			RELEASE_SPIN ( envNearSpin );
			RELEASE_SPIN ( envFarSpin );
			RELEASE_SPIN ( fStopSpin );
			RELEASE_BUT ( iFovType );
			return FALSE;

		case CC_SPINNER_CHANGE:
			if (!theHold.Holding()) theHold.Begin();
			switch ( LOWORD(wParam) ) {
				case IDC_FOVSPINNER:
					so->SetFOV(t, so->CurFOVtoWFOV(DegToRad(so->fovSpin->GetFVal())));	
					so->lensSpin->SetValue(so->FOVtoMM(so->GetFOV(t)), FALSE);
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
				case IDC_LENSSPINNER:
					so->SetFOV(t, so->MMtoFOV(so->lensSpin->GetFVal()));	
					so->fovSpin->SetValue(RadToDeg(so->WFOVtoCurFOV(so->GetFOV(t))), FALSE);
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
				case IDC_TDISTSPINNER:
					so->SetTDist(t, so->tdistSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#if 0	// this section leaves the spinners unconstrained
				case IDC_NEARSPINNER:
					so->SetClipDist(t, CAM_NEAR_CLIP, so->nearSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
				case IDC_FARSPINNER:
					so->SetClipDist(t, CAM_FAR_CLIP, so->farSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#else	// here, we constrain hither <= yon
				case IDC_NEARSPINNER:
				case IDC_FARSPINNER:
					tmpSmall = so->nearSpin->GetFVal();
					tmpLarge = so->farSpin->GetFVal();

					if(tmpSmall > tmpLarge) {
						if(LOWORD(wParam) == IDC_NEARSPINNER) {
							so->farSpin->SetValue(tmpSmall, FALSE);
							so->SetClipDist(t, CAM_FAR_CLIP, so->farSpin->GetFVal());	
						}
						else	{
							so->nearSpin->SetValue(tmpLarge, FALSE);
							so->SetClipDist(t, CAM_NEAR_CLIP, so->nearSpin->GetFVal());	
						}
					}
					if(LOWORD(wParam) == IDC_NEARSPINNER)
						so->SetClipDist(t, CAM_NEAR_CLIP, so->nearSpin->GetFVal());	
					else
						so->SetClipDist(t, CAM_FAR_CLIP, so->farSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#endif
#if 0	// similar constraint comments apply here
				case IDC_NEAR_RANGE_SPIN:
					so->SetEnvRange(t, ENV_NEAR_RANGE, so->envNearSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
				case IDC_FAR_RANGE_SPIN:
					so->SetEnvRange(t, ENV_FAR_RANGE, so->envFarSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#else
				case IDC_NEAR_RANGE_SPIN:
				case IDC_FAR_RANGE_SPIN:
					tmpSmall = so->envNearSpin->GetFVal();
					tmpLarge = so->envFarSpin->GetFVal();

					if(tmpSmall > tmpLarge) {
						if(LOWORD(wParam) == IDC_NEAR_RANGE_SPIN) {
							so->envFarSpin->SetValue(tmpSmall, FALSE);
							so->SetEnvRange(t, ENV_FAR_RANGE, so->envFarSpin->GetFVal());	
						}
						else	{
							so->envNearSpin->SetValue(tmpLarge, FALSE);
							so->SetEnvRange(t, ENV_NEAR_RANGE, so->envNearSpin->GetFVal());	
						}
					}
					if(LOWORD(wParam) == IDC_NEAR_RANGE_SPIN)
						so->SetEnvRange(t, ENV_NEAR_RANGE, so->envNearSpin->GetFVal());	
					else
						so->SetEnvRange(t, ENV_FAR_RANGE, so->envFarSpin->GetFVal());	
					so->UpdateKeyBrackets(t);
					so->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
					break;
#endif
				case IDC_DOF_FSTOP_SPIN:
					so->SetDOFFStop ( t, so->fStopSpin->GetFVal ());
					so->UpdateKeyBrackets (t);
					break;
				}
			return TRUE;

		case WM_SET_TYPE:
			theHold.Begin();
			so->SetType(wParam);
			theHold.Accept(GetString(IDS_DS_SETCAMTYPE));
			return FALSE;

		case CC_SPINNER_BUTTONDOWN:
			theHold.Begin();
			return TRUE;

		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP:
			if (HIWORD(wParam) || message==WM_CUSTEDIT_ENTER) theHold.Accept(GetString(IDS_DS_PARAMCHG));
			else theHold.Cancel();
			if ((message == CC_SPINNER_BUTTONUP) && (LOWORD(wParam) == IDC_FOVSPINNER))
			{
            ExecuteMAXScriptScript(_T(" InvalidateAllBackgrounds() "), TRUE);
			}
			so->iObjParams->RedrawViews(t,REDRAW_END);
			return TRUE;

		case WM_MOUSEACTIVATE:
			so->iObjParams->RealizeParamPanel();
			return FALSE;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
			so->iObjParams->RollupMouseMessage(hDlg,message,wParam,lParam);
			return FALSE;

		case WM_COMMAND:			
			switch( LOWORD(wParam) )
			{
#if 0	// no longer support creating cams from views
				case IDC_CREATE_VIEW:
				{
					SimpleCamera *camObject;
					TargetObject *targObject;
					INode *camNode, *targNode;
					TSTR targName;
					Matrix3 m, targM;
					vpt = so->iObjParams->GetActiveViewExp();
					if(!vpt->IsPerspView()) {
						MessageBox(hDlg, _T("No way, Jose"), _T("Camera Creation Failure"), MB_OK);
						break;
					}
					camObject = new SimpleCamera(so->hasTarget);
				   	theHold.Begin();	 // begin hold for undo
					// link it up
					so->iObjParams->SelectNode(camNode = so->iObjParams->CreateObjectNode( camObject));
					camNode->InvalidateTM();
					vpt->GetAffineTM(m);
					m = Inverse(m);
					camObject->SetFOV(t, vpt->GetFOV());
					if(!so->hasTarget)
						camObject->SetTDist(t, vpt->GetFocalDist());
					else {
						// Create target object and node
						targObject = new TargetObject;
						assert(targObject);
						targNode = so->iObjParams->CreateObjectNode( targObject);
						targName = camNode->GetName();
						targName += GetString(IDS_DB_DOT_TARGET);
						macroRec->Disable();
						targNode->SetName(targName);
						macroRec->Enable();

						// hook up camera to target using lookat controller.
						so->iObjParams->BindToTarget(camNode,targNode);

						// set the target point based on the lookat direction and dist
						targM = m;
						targM.PreTranslate(Point3(0.0f, 0.0f, -vpt->GetFocalDist()));
						targNode->SetNodeTM(t, targM);
					}
					camNode->SetNodeTM(t, m);
					camObject->Enable(1);
					so->iObjParams->RedrawViews(so->iObjParams->GetTime());  
				    theHold.Accept(GetString(IDS_DS_CREATE));	 

					break;
				}
#endif
				case IDC_MANUAL_CLIP:
					so->SetManualClip( IsDlgButtonChecked( hDlg, IDC_MANUAL_CLIP) );
					EnableWindow( GetDlgItem(hDlg, IDC_HITHER), so->manualClip);
					EnableWindow( GetDlgItem(hDlg, IDC_NEARSPINNER), so->manualClip);
					EnableWindow( GetDlgItem(hDlg, IDC_YON), so->manualClip);
					EnableWindow( GetDlgItem(hDlg, IDC_FARSPINNER), so->manualClip);
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_SHOWCAMCONE:
					so->SetConeState( IsDlgButtonChecked( hDlg, IDC_SHOWCAMCONE ) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_SHOWHORZLINE:
					so->SetHorzLineState( IsDlgButtonChecked( hDlg, IDC_SHOWHORZLINE ) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_SHOW_RANGES:
					so->SetEnvDisplay( IsDlgButtonChecked( hDlg, IDC_SHOW_RANGES ) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_IS_ORTHO:
					so->SetOrtho( IsDlgButtonChecked( hDlg, IDC_IS_ORTHO ) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_15MM:
				case IDC_20MM:
				case IDC_24MM:
				case IDC_28MM:
				case IDC_35MM:
				case IDC_50MM:
				case IDC_85MM:
				case IDC_135MM:
				case IDC_200MM:
					theHold.Begin();
					so->SetFOV(t, so->MMtoFOV(mmTab[LOWORD(wParam) - IDC_15MM]));	
					so->fovSpin->SetValue(RadToDeg(so->WFOVtoCurFOV(so->GetFOV(t))), FALSE);
					so->lensSpin->SetValue(so->FOVtoMM(so->GetFOV(t)), FALSE);
					so->iObjParams->RedrawViews(t,REDRAW_END);
					theHold.Accept(GetString(IDS_DS_CAMPRESET));
					break;		   
				case IDC_FOV_TYPE: 
					so->SetFOVType(so->iFovType->GetCurFlyOff());
					break;
				case IDC_CAM_TYPE:
				{
					int code = HIWORD(wParam);
					if (code==CBN_SELCHANGE) {
						int newType = SendMessage( GetDlgItem(hDlg,IDC_CAM_TYPE), CB_GETCURSEL, 0, 0 );
						PostMessage(hDlg,WM_SET_TYPE,newType,0);
						}
					break;
				}
				case IDC_ENABLE_MP_EFFECT:
					so->SetMultiPassEffectEnabled( t, IsDlgButtonChecked(hDlg, IDC_ENABLE_MP_EFFECT) );
					so->iObjParams->RedrawViews(t);
					break;
				case IDC_MP_EFFECT_REFFECT_PER_PASS:
					so->SetMPEffect_REffectPerPass( IsDlgButtonChecked(hDlg, IDC_MP_EFFECT_REFFECT_PER_PASS) );
					break;
				case IDC_PREVIEW_MP_EFFECT:
					GetCOREInterface()->DisplayActiveCameraViewWithMultiPassEffect();
					break;
				case IDC_MP_EFFECT:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						int index = SendMessage( GetDlgItem(hDlg,IDC_MP_EFFECT), CB_GETCURSEL, 0, 0 );
						Tab<ClassEntry*> &effectList = SimpleCamera::GetCompatibleEffectList();
						if ( (index >= 0) && ( index < effectList.Count() ) )
							{
							IMultiPassCameraEffect *pPrevCameraEffect = so->GetIMultiPassCameraEffect();
							if ( !pPrevCameraEffect || ( pPrevCameraEffect->ClassID() != effectList[index]->CD()->ClassID() ) ) {
								IMultiPassCameraEffect *mpce = reinterpret_cast<IMultiPassCameraEffect *>( effectList[index]->CD()->Create(0) );
								theHold.Begin();
								so->SetIMultiPassCameraEffect(mpce);
								theHold.Accept(GetString(IDS_DS_MULTIPASS));
								}
							}
						else
						{
							DbgAssert(0);
						}
						so->iObjParams->RedrawViews(t);
					}
					break;
				}
			}
			return FALSE;

		default:
			return FALSE;
		}
	}



void SimpleCamera::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
{
	iObjParams = ip;	
	inCreate = (flags & BEGIN_EDIT_CREATE) ? 1 : 0;

	currentEditCam = this;

	if ( !hSimpleCamParams )
	{
		hSimpleCamParams = ip->AddRollupPage(
				hInstance,
				MAKEINTRESOURCE(IDD_FCAMERAPARAM),  // DS 8/15/00 
				//hasTarget ? MAKEINTRESOURCE(IDD_SCAMERAPARAM) : MAKEINTRESOURCE(IDD_FCAMERAPARAM),
				SimpleCamParamDialogProc,
				GetString(IDS_RB_PARAMETERS),
				(LPARAM)this);

		ip->RegisterDlgWnd(hSimpleCamParams);

		{
			iFovType = GetICustButton(GetDlgItem(hSimpleCamParams,IDC_FOV_TYPE));
			//iFovType->SetType(CBT_CHECK);
			//iFovType->SetImage(hCamImages,0,0,0,0,14,14);
			FlyOffData fod[3] = {
				{ 0,0,0,0 },
				{ 1,1,1,1 },
				{ 2,2,2,2 }
				};
			//iFovType->SetFlyOff(3,fod,0/*timeout*/,0/*init val*/,FLY_DOWN);
			//iFovType->SetCurFlyOff(GetFOVType());  // DS 8/8/00
//			ttips[2] = GetResString(IDS_DB_BACKGROUND);
//			iFovType->SetTooltip(1, ttips[2]);
		}

		fovSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_FOVSPINNER));
		fovSpin->SetLimits( MIN_FOV, MAX_FOV, FALSE );
		fovSpin->SetValue(RadToDeg(WFOVtoCurFOV(GetFOV(ip->GetTime()))), FALSE);
		fovSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_FOV), EDITTYPE_FLOAT );
			
		lensSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_LENSSPINNER));
		lensSpin->SetLimits( MIN_LENS, MAX_LENS, FALSE );
		lensSpin->SetValue(FOVtoMM(GetFOV(ip->GetTime())), FALSE);
		lensSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_LENS), EDITTYPE_FLOAT );
			
		nearSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_NEARSPINNER));
		nearSpin->SetLimits( MIN_CLIP, MAX_CLIP, FALSE );
		nearSpin->SetValue(GetClipDist(ip->GetTime(), CAM_NEAR_CLIP), FALSE);
		// xavier robitaille | 03.02.07 | increments proportional to the spinner value
		nearSpin->SetAutoScale();
		nearSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_HITHER), EDITTYPE_UNIVERSE );
			
		farSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_FARSPINNER));
		farSpin->SetLimits( MIN_CLIP, MAX_CLIP, FALSE );
		farSpin->SetValue(GetClipDist(ip->GetTime(), CAM_FAR_CLIP), FALSE);
		// xavier robitaille | 03.02.07 | increments proportional to the spinner value
		farSpin->SetAutoScale();
		farSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_YON), EDITTYPE_UNIVERSE );

		envNearSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_NEAR_RANGE_SPIN));
		envNearSpin->SetLimits( 0.0f, 999999.0f, FALSE );
		envNearSpin->SetValue(GetEnvRange(ip->GetTime(), ENV_NEAR_RANGE), FALSE);
		// alexc | 03.06.09 | increments proportional to the spinner value
        envNearSpin->SetAutoScale();
		envNearSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_NEAR_RANGE), EDITTYPE_UNIVERSE );
			
		envFarSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_FAR_RANGE_SPIN));
		envFarSpin->SetLimits( 0.0f, 999999.0f, FALSE );
		envFarSpin->SetValue(GetEnvRange(ip->GetTime(), ENV_FAR_RANGE), FALSE);
		// alexc | 03.06.09 | increments proportional to the spinner value
        envFarSpin->SetAutoScale();
		envFarSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_FAR_RANGE), EDITTYPE_UNIVERSE );

		tdistSpin = GetISpinner(GetDlgItem(hSimpleCamParams,IDC_TDISTSPINNER));
		tdistSpin->SetLimits( MIN_TDIST, MAX_TDIST, FALSE );
		tdistSpin->SetValue(GetTDist(ip->GetTime()), FALSE);
		tdistSpin->LinkToEdit( GetDlgItem(hSimpleCamParams,IDC_TDIST), EDITTYPE_UNIVERSE );
		// xavier robitaille | 03.02.07 | increments proportional to the spinner value
		tdistSpin->SetAutoScale();
		// display possible multipass camera effect rollup
		IMultiPassCameraEffect *pCurCameraEffect = GetIMultiPassCameraEffect();
		if (pCurCameraEffect)
		{
			pCurCameraEffect->BeginEditParams(ip, flags, prev);
		}

	}
	else
	{
      DLSetWindowLongPtr( hSimpleCamParams, this);
		
		fovSpin->SetValue(RadToDeg(WFOVtoCurFOV(GetFOV(ip->GetTime()))),FALSE);
		nearSpin->SetValue(GetClipDist(ip->GetTime(), CAM_NEAR_CLIP),FALSE);
		farSpin->SetValue(GetClipDist(ip->GetTime(), CAM_FAR_CLIP),FALSE);
		envNearSpin->SetValue(GetEnvRange(ip->GetTime(), ENV_NEAR_RANGE),FALSE);
		envFarSpin->SetValue(GetEnvRange(ip->GetTime(), ENV_FAR_RANGE),FALSE);
		// DS 8/15/00 
//		if(!hasTarget)
			tdistSpin->SetValue(GetTDist(ip->GetTime()),FALSE);

		SetConeState( IsDlgButtonChecked(hSimpleCamParams,IDC_SHOWCAMCONE) );
		SetHorzLineState( IsDlgButtonChecked(hSimpleCamParams,IDC_SHOWHORZLINE) );
		SetEnvDisplay( IsDlgButtonChecked(hSimpleCamParams,IDC_SHOW_RANGES) );
		SetManualClip( IsDlgButtonChecked(hSimpleCamParams,IDC_MANUAL_CLIP) );

		// display possible multipass camera effect rollup
		IMultiPassCameraEffect *pCurCameraEffect = GetIMultiPassCameraEffect();
		if (pCurCameraEffect)
		{
			pCurCameraEffect->BeginEditParams(ip, flags, prev);
		}

	}
	SendMessage( GetDlgItem(hSimpleCamParams, IDC_CAM_TYPE), CB_SETCURSEL, hasTarget, (LPARAM)0 );
}
		
void SimpleCamera::EndEditParams( IObjParam *ip, ULONG flags,Animatable *prev)
{
	dlgFOV = GetFOV(ip->GetTime());
	dlgFOVType = GetFOVType();
	dlgShowCone = IsDlgButtonChecked(hSimpleCamParams, IDC_SHOWCAMCONE );
	dlgRangeDisplay = IsDlgButtonChecked(hSimpleCamParams, IDC_SHOW_RANGES );
	dlgIsOrtho = IsDlgButtonChecked(hSimpleCamParams, IDC_IS_ORTHO);
	dlgShowHorzLine = IsDlgButtonChecked(hSimpleCamParams, IDC_SHOWHORZLINE );
	dlgClip = IsDlgButtonChecked(hSimpleCamParams, IDC_MANUAL_CLIP );
	dlgTDist = GetTDist(ip->GetTime());
	dlgNearher = GetClipDist(ip->GetTime(), CAM_NEAR_CLIP);
	dlgFar = GetClipDist(ip->GetTime(), CAM_FAR_CLIP);
	dlgNearRange = GetEnvRange(ip->GetTime(), ENV_NEAR_RANGE);
	dlgFarRange = GetEnvRange(ip->GetTime(), ENV_FAR_RANGE);

	dlgMultiPassEffectEnable = GetMultiPassEffectEnabled( ip->GetTime() );
	//dlgMPEffect_REffectPerPass = GetMPEffect_REffectPerPass();

	IMultiPassCameraEffect *pCurCameraEffect = GetIMultiPassCameraEffect();
	if (pCurCameraEffect)
	{
		pCurCameraEffect->EndEditParams(ip, flags, prev);
	}

	// Depth of Field
	currentEditCam = NULL;

	if ( flags&END_EDIT_REMOVEUI )
	{
		if ( hDepthOfFieldParams )
		{
			ip->UnRegisterDlgWnd ( hDepthOfFieldParams );
			ip->DeleteRollupPage ( hDepthOfFieldParams );
			hDepthOfFieldParams = NULL;
		}

		ip->UnRegisterDlgWnd(hSimpleCamParams);
		ip->DeleteRollupPage(hSimpleCamParams);
		hSimpleCamParams = NULL;				
	}
	else
	{
      DLSetWindowLongPtr( hSimpleCamParams, 0);
      DLSetWindowLongPtr( hDepthOfFieldParams, 0);
	}
	iObjParams = NULL;
}


static void MakeQuad(Face *f, int a,  int b , int c , int d, int sg, int dv = 0) {
	f[0].setVerts( a+dv, b+dv, c+dv);
	f[0].setSmGroup(sg);
	f[0].setEdgeVisFlags(1,1,0);
	f[1].setVerts( c+dv, d+dv, a+dv);
	f[1].setSmGroup(sg);
	f[1].setEdgeVisFlags(1,1,0);
	}

void SimpleCamera::BuildMesh()	{

	mesh.setNumVerts(179+2);	//pw need to add 2 points to make the bounding box symmetrical
	mesh.setNumFaces(330);

	mesh.setVert(0, Point3(8.87897, -14.2469, -5.28721));
	mesh.setVert(1, Point3(-8.87898, -14.2469, -5.28721));
	mesh.setVert(2, Point3(8.87897, -14.2469, 25.8783));
	mesh.setVert(3, Point3(-8.87898, -14.2469, 25.8783));
	mesh.setVert(4, Point3(8.87897, 9.42955, -5.28721));
	mesh.setVert(5, Point3(-8.87898, 9.42954, -5.28721));
	mesh.setVert(6, Point3(8.87897, 9.42955, 25.8783));
	mesh.setVert(7, Point3(-8.87898, 9.42954, 25.8783));
	mesh.setVert(8, Point3(-6.88202, 24.5003, -18.1687));
	mesh.setVert(9, Point3(-6.88202, 29.7185, -17.2486));
	mesh.setVert(10, Point3(-6.88202, 34.3073, -14.5993));
	mesh.setVert(11, Point3(-6.88203, 37.7132, -10.5403));
	mesh.setVert(12, Point3(-6.88203, 39.5255, -5.56113));
	mesh.setVert(13, Point3(-6.88203, 39.5255, -0.262442));
	mesh.setVert(14, Point3(-6.88203, 37.7133, 4.71669));
	mesh.setVert(15, Point3(-6.88203, 34.3073, 8.77572));
	mesh.setVert(16, Point3(-6.88203, 29.7185, 11.4251));
	mesh.setVert(17, Point3(-6.88203, 24.5004, 12.3452));
	mesh.setVert(18, Point3(-6.88202, 19.2822, 11.4251));
	mesh.setVert(19, Point3(-6.88202, 14.6934, 8.77572));
	mesh.setVert(20, Point3(-6.88202, 11.2874, 4.7167));
	mesh.setVert(21, Point3(-6.88202, 9.47518, -0.262435));
	mesh.setVert(22, Point3(-6.88202, 9.47518, -5.56112));
	mesh.setVert(23, Point3(-6.88202, 11.2874, -10.5403));
	mesh.setVert(24, Point3(-6.88202, 14.6934, -14.5993));
	mesh.setVert(25, Point3(-6.88202, 19.2821, -17.2486));
	mesh.setVert(26, Point3(0.48888, 24.5004, -18.1687));
	mesh.setVert(27, Point3(0.488878, 29.7185, -17.2486));
	mesh.setVert(28, Point3(0.488876, 34.3073, -14.5993));
	mesh.setVert(29, Point3(0.488874, 37.7133, -10.5403));
	mesh.setVert(30, Point3(0.488872, 39.5255, -5.56113));
	mesh.setVert(31, Point3(0.488872, 39.5255, -0.262442));
	mesh.setVert(32, Point3(0.488872, 37.7133, 4.71669));
	mesh.setVert(33, Point3(0.488872, 34.3073, 8.77572));
	mesh.setVert(34, Point3(0.488873, 29.7185, 11.4251));
	mesh.setVert(35, Point3(0.488875, 24.5003, 12.3452));
	mesh.setVert(36, Point3(0.488877, 19.2822, 11.4251));
	mesh.setVert(37, Point3(0.488879, 14.6934, 8.77572));
	mesh.setVert(38, Point3(0.488881, 11.2874, 4.7167));
	mesh.setVert(39, Point3(0.488883, 9.47518, -0.262435));
	mesh.setVert(40, Point3(0.488883, 9.47518, -5.56112));
	mesh.setVert(41, Point3(0.488883, 11.2874, -10.5403));
	mesh.setVert(42, Point3(0.488883, 14.6934, -14.5993));
	mesh.setVert(43, Point3(0.488882, 19.2822, -17.2486));
	mesh.setVert(44, Point3(-6.88202, 24.5004, 12.2439));
	mesh.setVert(45, Point3(-6.88202, 29.7185, 13.164));
	mesh.setVert(46, Point3(-6.88202, 34.3073, 15.8133));
	mesh.setVert(47, Point3(-6.88203, 37.7133, 19.8723));
	mesh.setVert(48, Point3(-6.88203, 39.5255, 24.8515));
	mesh.setVert(49, Point3(-6.88203, 39.5255, 30.1502));
	mesh.setVert(50, Point3(-6.88203, 37.7133, 35.1293));
	mesh.setVert(51, Point3(-6.88203, 34.3073, 39.1883));
	mesh.setVert(52, Point3(-6.88203, 29.7185, 41.8377));
	mesh.setVert(53, Point3(-6.88203, 24.5003, 42.7578));
	mesh.setVert(54, Point3(-6.88202, 19.2822, 41.8377));
	mesh.setVert(55, Point3(-6.88202, 14.6934, 39.1883));
	mesh.setVert(56, Point3(-6.88202, 11.2874, 35.1293));
	mesh.setVert(57, Point3(-6.88202, 9.47518, 30.1502));
	mesh.setVert(58, Point3(-6.88202, 9.47518, 24.8515));
	mesh.setVert(59, Point3(-6.88202, 11.2874, 19.8723));
	mesh.setVert(60, Point3(-6.88202, 14.6934, 15.8133));
	mesh.setVert(61, Point3(-6.88202, 19.2822, 13.164));
	mesh.setVert(62, Point3(0.48888, 24.5003, 12.2439));
	mesh.setVert(63, Point3(0.488878, 29.7185, 13.164));
	mesh.setVert(64, Point3(0.488876, 34.3073, 15.8133));
	mesh.setVert(65, Point3(0.488874, 37.7133, 19.8723));
	mesh.setVert(66, Point3(0.488872, 39.5255, 24.8515));
	mesh.setVert(67, Point3(0.488872, 39.5255, 30.1502));
	mesh.setVert(68, Point3(0.488872, 37.7133, 35.1293));
	mesh.setVert(69, Point3(0.488872, 34.3073, 39.1883));
	mesh.setVert(70, Point3(0.488873, 29.7185, 41.8377));
	mesh.setVert(71, Point3(0.488875, 24.5003, 42.7578));
	mesh.setVert(72, Point3(0.488877, 19.2822, 41.8377));
	mesh.setVert(73, Point3(0.488879, 14.6934, 39.1883));
	mesh.setVert(74, Point3(0.488881, 11.2874, 35.1293));
	mesh.setVert(75, Point3(0.488883, 9.47518, 30.1502));
	mesh.setVert(76, Point3(0.488883, 9.47518, 24.8515));
	mesh.setVert(77, Point3(0.488883, 11.2874, 19.8723));
	mesh.setVert(78, Point3(0.488883, 14.6934, 15.8133));
	mesh.setVert(79, Point3(0.488882, 19.2822, 13.164));
	mesh.setVert(80, Point3(-6.63813, -1.77237, -5.2872));
	mesh.setVert(81, Point3(-5.75096, 2.11456, -5.2872));
	mesh.setVert(82, Point3(-3.26518, 5.23163, -5.2872));
	mesh.setVert(83, Point3(0.326883, 6.96148, -5.2872));
	mesh.setVert(84, Point3(4.31377, 6.96148, -5.2872));
	mesh.setVert(85, Point3(7.90583, 5.23163, -5.2872));
	mesh.setVert(86, Point3(10.3916, 2.11456, -5.2872));
	mesh.setVert(87, Point3(11.2788, -1.77237, -5.2872));
	mesh.setVert(88, Point3(10.3916, -5.6593, -5.2872));
	mesh.setVert(89, Point3(7.90584, -8.77637, -5.2872));
	mesh.setVert(90, Point3(4.31377, -10.5062, -5.2872));
	mesh.setVert(91, Point3(0.326884, -10.5062, -5.2872));
	mesh.setVert(92, Point3(-3.26518, -8.77637, -5.2872));
	mesh.setVert(93, Point3(-5.75096, -5.6593, -5.2872));
	mesh.setVert(94, Point3(-6.63813, -1.77237, -10.2414));
	mesh.setVert(95, Point3(-5.75096, 2.11456, -10.2414));
	mesh.setVert(96, Point3(-3.26518, 5.23163, -10.2414));
	mesh.setVert(97, Point3(0.326883, 6.96148, -10.2414));
	mesh.setVert(98, Point3(4.31377, 6.96148, -10.2414));
	mesh.setVert(99, Point3(7.90583, 5.23163, -10.2414));
	mesh.setVert(100, Point3(10.3916, 2.11456, -10.2414));
	mesh.setVert(101, Point3(11.2788, -1.77237, -10.2414));
	mesh.setVert(102, Point3(10.3916, -5.6593, -10.2414));
	mesh.setVert(103, Point3(7.90584, -8.77637, -10.2414));
	mesh.setVert(104, Point3(4.31377, -10.5062, -10.2414));
	mesh.setVert(105, Point3(0.326884, -10.5062, -10.2414));
	mesh.setVert(106, Point3(-3.26518, -8.77637, -10.2414));
	mesh.setVert(107, Point3(-5.75096, -5.6593, -10.2414));
	mesh.setVert(108, Point3(-6.47523, -1.78092, -10.0877));
	mesh.setVert(109, Point3(-5.45869, 1.01199, -10.0877));
	mesh.setVert(110, Point3(-2.88474, 2.49807, -10.0877));
	mesh.setVert(111, Point3(0.0422633, 1.98196, -10.0877));
	mesh.setVert(112, Point3(1.95273, -0.294841, -10.0877));
	mesh.setVert(113, Point3(1.95273, -3.267, -10.0877));
	mesh.setVert(114, Point3(0.0422639, -5.5438, -10.0877));
	mesh.setVert(115, Point3(-2.88473, -6.05991, -10.0877));
	mesh.setVert(116, Point3(-5.45869, -4.57383, -10.0877));
	mesh.setVert(117, Point3(-6.47523, -1.78092, -27.6608));
	mesh.setVert(118, Point3(-5.45869, 1.01199, -27.6608));
	mesh.setVert(119, Point3(-2.88474, 2.49806, -27.6608));
	mesh.setVert(120, Point3(0.0422633, 1.98196, -27.6608));
	mesh.setVert(121, Point3(1.95273, -0.294847, -27.6608));
	mesh.setVert(122, Point3(1.95273, -3.267, -27.6608));
	mesh.setVert(123, Point3(0.0422639, -5.5438, -27.6608));
	mesh.setVert(124, Point3(-2.88473, -6.05991, -27.6608));
	mesh.setVert(125, Point3(-5.45869, -4.57383, -27.6608));
	mesh.setVert(126, Point3(2.61922, -6.52382, -27.6608));
	mesh.setVert(127, Point3(-7.07981, -6.52383, -27.6608));
	mesh.setVert(128, Point3(2.61922, 3.01396, -27.6608));
	mesh.setVert(129, Point3(-7.07981, 3.01396, -27.6608));
	mesh.setVert(130, Point3(10.3326, -11.7608, -36.1809));
	mesh.setVert(131, Point3(-14.7931, -11.7608, -36.1809));
	mesh.setVert(132, Point3(10.3326, 8.93288, -36.1809));
	mesh.setVert(133, Point3(-14.7932, 8.93288, -36.1809));
	mesh.setVert(134, Point3(8.07739, -2.14196, 25.8783));
	mesh.setVert(135, Point3(7.55307, -0.7014, 25.8783));
	mesh.setVert(136, Point3(6.22545, 0.0651031, 25.8783));
	mesh.setVert(137, Point3(4.71573, -0.201104, 25.8783));
	mesh.setVert(138, Point3(3.73034, -1.37545, 25.8783));
	mesh.setVert(139, Point3(3.73034, -2.90846, 25.8783));
	mesh.setVert(140, Point3(4.71573, -4.08281, 25.8783));
	mesh.setVert(141, Point3(6.22545, -4.34901, 25.8783));
	mesh.setVert(142, Point3(7.55307, -3.58251, 25.8783));
	mesh.setVert(143, Point3(8.07739, -2.14196, 33.3486));
	mesh.setVert(144, Point3(7.55307, -0.701399, 33.3486));
	mesh.setVert(145, Point3(6.22545, 0.0650998, 33.3486));
	mesh.setVert(146, Point3(4.71573, -0.201099, 33.3486));
	mesh.setVert(147, Point3(3.73033, -1.37545, 33.3486));
	mesh.setVert(148, Point3(3.73033, -2.90846, 33.3486));
	mesh.setVert(149, Point3(4.71573, -4.08281, 33.3486));
	mesh.setVert(150, Point3(6.22545, -4.34902, 33.3486));
	mesh.setVert(151, Point3(7.55307, -3.58251, 33.3486));
	mesh.setVert(152, Point3(8.07739, -1.61666, 35.3207));
	mesh.setVert(153, Point3(7.55307, -0.398201, 34.5522));
	mesh.setVert(154, Point3(6.22545, 0.250135, 34.1433));
	mesh.setVert(155, Point3(4.71573, 0.0249721, 34.2854));
	mesh.setVert(156, Point3(3.73033, -0.96833, 34.9118));
	mesh.setVert(157, Point3(3.73033, -2.265, 35.7296));
	mesh.setVert(158, Point3(4.71573, -3.2583, 36.356));
	mesh.setVert(159, Point3(6.22545, -3.48347, 36.4981));
	mesh.setVert(160, Point3(7.55307, -2.83513, 36.0892));
	mesh.setVert(161, Point3(7.86182, 0.695757, 37.1316));
	mesh.setVert(162, Point3(7.38793, 1.5194, 36.1232));
	mesh.setVert(163, Point3(6.18801, 1.95766, 35.5867));
	mesh.setVert(164, Point3(4.82351, 1.80546, 35.773));
	mesh.setVert(165, Point3(3.9329, 1.13401, 36.595));
	mesh.setVert(166, Point3(3.9329, 0.257507, 37.6681));
	mesh.setVert(167, Point3(4.82352, -0.413938, 38.4901));
	mesh.setVert(168, Point3(6.18801, -0.566142, 38.6764));
	mesh.setVert(169, Point3(7.38794, -0.127887, 38.1399));
	mesh.setVert(170, Point3(9.05109, 1.2996, 37.6248));
	mesh.setVert(171, Point3(8.29897, 2.60684, 36.0244));
	mesh.setVert(172, Point3(6.39453, 3.30241, 35.1728));
	mesh.setVert(173, Point3(4.22888, 3.06084, 35.4686));
	mesh.setVert(174, Point3(2.81535, 1.99517, 36.7732));
	mesh.setVert(175, Point3(2.81535, 0.604037, 38.4763));
	mesh.setVert(176, Point3(4.22888, -0.461635, 39.781));
	mesh.setVert(177, Point3(6.39453, -0.7032, 40.0767));
	mesh.setVert(178, Point3(8.29897, -0.00763498, 39.2252));

//pw need to ceate 2 points to make the bounding box symmetrical
	Box3 bbox;
	bbox.Init();
	for (int i =0; i < 170; i++)
		{
		bbox += mesh.getVert(i);
		}
	Point3 minVec = bbox.pmin;
	Point3 maxVec = bbox.pmax;
	float minLen = Length(minVec);
	float maxLen = Length(maxVec);
	if (fabs(minVec.x) > fabs(maxVec.x))
		{
		maxVec.x = -minVec.x;
		}
	else
		{
		minVec.x = -maxVec.x;
		}

	if (fabs(minVec.y) > fabs(maxVec.y))
		{
		maxVec.y = -minVec.y;
		}
	else
		{
		minVec.y = -maxVec.y;
		}

	if (fabs(minVec.z) > fabs(maxVec.z))
		{
		maxVec.z = -minVec.z;
		}
	else
		{
		minVec.z = -maxVec.z;
		}


	mesh.setVert(179, maxVec);
	mesh.setVert(180, minVec);

	mesh.faces[0].setVerts(3, 1, 0);		mesh.faces[0].setEdgeVisFlags(1, 1, 0);		mesh.faces[0].setSmGroup(2);
	mesh.faces[1].setVerts(0, 2, 3);		mesh.faces[1].setEdgeVisFlags(1, 1, 0);		mesh.faces[1].setSmGroup(2);
	mesh.faces[2].setVerts(7, 6, 4);		mesh.faces[2].setEdgeVisFlags(1, 1, 0);		mesh.faces[2].setSmGroup(4);
	mesh.faces[3].setVerts(4, 5, 7);		mesh.faces[3].setEdgeVisFlags(1, 1, 0);		mesh.faces[3].setSmGroup(4);
	mesh.faces[4].setVerts(5, 4, 0);		mesh.faces[4].setEdgeVisFlags(1, 1, 0);		mesh.faces[4].setSmGroup(8);
	mesh.faces[5].setVerts(0, 1, 5);		mesh.faces[5].setEdgeVisFlags(1, 1, 0);		mesh.faces[5].setSmGroup(8);
	mesh.faces[6].setVerts(7, 5, 1);		mesh.faces[6].setEdgeVisFlags(1, 1, 0);		mesh.faces[6].setSmGroup(16);
	mesh.faces[7].setVerts(1, 3, 7);		mesh.faces[7].setEdgeVisFlags(1, 1, 0);		mesh.faces[7].setSmGroup(16);
	mesh.faces[8].setVerts(6, 7, 3);		mesh.faces[8].setEdgeVisFlags(1, 1, 0);		mesh.faces[8].setSmGroup(32);
	mesh.faces[9].setVerts(3, 2, 6);		mesh.faces[9].setEdgeVisFlags(1, 1, 0);		mesh.faces[9].setSmGroup(32);
	mesh.faces[10].setVerts(4, 6, 2);		mesh.faces[10].setEdgeVisFlags(1, 1, 0);	mesh.faces[10].setSmGroup(64);
	mesh.faces[11].setVerts(2, 0, 4);		mesh.faces[11].setEdgeVisFlags(1, 1, 0);	mesh.faces[11].setSmGroup(64);
	mesh.faces[12].setVerts(24, 23, 22);	mesh.faces[12].setEdgeVisFlags(1, 1, 0);	mesh.faces[12].setSmGroup(1);
	mesh.faces[13].setVerts(22, 21, 20);	mesh.faces[13].setEdgeVisFlags(1, 1, 0);	mesh.faces[13].setSmGroup(1);
	mesh.faces[14].setVerts(20, 19, 18);	mesh.faces[14].setEdgeVisFlags(1, 1, 0);	mesh.faces[14].setSmGroup(1);
	mesh.faces[15].setVerts(22, 20, 18);	mesh.faces[15].setEdgeVisFlags(0, 0, 0);	mesh.faces[15].setSmGroup(1);
	mesh.faces[16].setVerts(18, 17, 16);	mesh.faces[16].setEdgeVisFlags(1, 1, 0);	mesh.faces[16].setSmGroup(1);
	mesh.faces[17].setVerts(16, 15, 14);	mesh.faces[17].setEdgeVisFlags(1, 1, 0);	mesh.faces[17].setSmGroup(1);
	mesh.faces[18].setVerts(18, 16, 14);	mesh.faces[18].setEdgeVisFlags(0, 0, 0);	mesh.faces[18].setSmGroup(1);
	mesh.faces[19].setVerts(14, 13, 12);	mesh.faces[19].setEdgeVisFlags(1, 1, 0);	mesh.faces[19].setSmGroup(1);
	mesh.faces[20].setVerts(12, 11, 10);	mesh.faces[20].setEdgeVisFlags(1, 1, 0);	mesh.faces[20].setSmGroup(1);
	mesh.faces[21].setVerts(14, 12, 10);	mesh.faces[21].setEdgeVisFlags(0, 0, 0);	mesh.faces[21].setSmGroup(1);
	mesh.faces[22].setVerts(18, 14, 10);	mesh.faces[22].setEdgeVisFlags(0, 0, 0);	mesh.faces[22].setSmGroup(1);
	mesh.faces[23].setVerts(22, 18, 10);	mesh.faces[23].setEdgeVisFlags(0, 0, 0);	mesh.faces[23].setSmGroup(1);
	mesh.faces[24].setVerts(10, 9, 8);		mesh.faces[24].setEdgeVisFlags(1, 1, 0);	mesh.faces[24].setSmGroup(1);
	mesh.faces[25].setVerts(22, 10, 8);		mesh.faces[25].setEdgeVisFlags(0, 0, 0);	mesh.faces[25].setSmGroup(1);
	mesh.faces[26].setVerts(24, 22, 8);		mesh.faces[26].setEdgeVisFlags(0, 0, 0);	mesh.faces[26].setSmGroup(1);
	mesh.faces[27].setVerts(25, 24, 8);		mesh.faces[27].setEdgeVisFlags(1, 0, 1);	mesh.faces[27].setSmGroup(1);
	mesh.faces[28].setVerts(8, 9, 27);		mesh.faces[28].setEdgeVisFlags(1, 1, 0);	mesh.faces[28].setSmGroup(8);
	mesh.faces[29].setVerts(27, 26, 8);		mesh.faces[29].setEdgeVisFlags(1, 1, 0);	mesh.faces[29].setSmGroup(8);
	mesh.faces[30].setVerts(9, 10, 28);		mesh.faces[30].setEdgeVisFlags(1, 1, 0);	mesh.faces[30].setSmGroup(8);
	mesh.faces[31].setVerts(28, 27, 9);		mesh.faces[31].setEdgeVisFlags(1, 1, 0);	mesh.faces[31].setSmGroup(8);
	mesh.faces[32].setVerts(10, 11, 29);	mesh.faces[32].setEdgeVisFlags(1, 1, 0);	mesh.faces[32].setSmGroup(8);
	mesh.faces[33].setVerts(29, 28, 10);	mesh.faces[33].setEdgeVisFlags(1, 1, 0);	mesh.faces[33].setSmGroup(8);
	mesh.faces[34].setVerts(11, 12, 30);	mesh.faces[34].setEdgeVisFlags(1, 1, 0);	mesh.faces[34].setSmGroup(8);
	mesh.faces[35].setVerts(30, 29, 11);	mesh.faces[35].setEdgeVisFlags(1, 1, 0);	mesh.faces[35].setSmGroup(8);
	mesh.faces[36].setVerts(12, 13, 31);	mesh.faces[36].setEdgeVisFlags(1, 1, 0);	mesh.faces[36].setSmGroup(8);
	mesh.faces[37].setVerts(31, 30, 12);	mesh.faces[37].setEdgeVisFlags(1, 1, 0);	mesh.faces[37].setSmGroup(8);
	mesh.faces[38].setVerts(13, 14, 32);	mesh.faces[38].setEdgeVisFlags(1, 1, 0);	mesh.faces[38].setSmGroup(8);
	mesh.faces[39].setVerts(32, 31, 13);	mesh.faces[39].setEdgeVisFlags(1, 1, 0);	mesh.faces[39].setSmGroup(8);
	mesh.faces[40].setVerts(14, 15, 33);	mesh.faces[40].setEdgeVisFlags(1, 1, 0);	mesh.faces[40].setSmGroup(8);
	mesh.faces[41].setVerts(33, 32, 14);	mesh.faces[41].setEdgeVisFlags(1, 1, 0);	mesh.faces[41].setSmGroup(8);
	mesh.faces[42].setVerts(15, 16, 34);	mesh.faces[42].setEdgeVisFlags(1, 1, 0);	mesh.faces[42].setSmGroup(8);
	mesh.faces[43].setVerts(34, 33, 15);	mesh.faces[43].setEdgeVisFlags(1, 1, 0);	mesh.faces[43].setSmGroup(8);
	mesh.faces[44].setVerts(16, 17, 35);	mesh.faces[44].setEdgeVisFlags(1, 1, 0);	mesh.faces[44].setSmGroup(8);
	mesh.faces[45].setVerts(35, 34, 16);	mesh.faces[45].setEdgeVisFlags(1, 1, 0);	mesh.faces[45].setSmGroup(8);
	mesh.faces[46].setVerts(17, 18, 36);	mesh.faces[46].setEdgeVisFlags(1, 1, 0);	mesh.faces[46].setSmGroup(8);
	mesh.faces[47].setVerts(36, 35, 17);	mesh.faces[47].setEdgeVisFlags(1, 1, 0);	mesh.faces[47].setSmGroup(8);
	mesh.faces[48].setVerts(18, 19, 37);	mesh.faces[48].setEdgeVisFlags(1, 1, 0);	mesh.faces[48].setSmGroup(8);
	mesh.faces[49].setVerts(37, 36, 18);	mesh.faces[49].setEdgeVisFlags(1, 1, 0);	mesh.faces[49].setSmGroup(8);
	mesh.faces[50].setVerts(19, 20, 38);	mesh.faces[50].setEdgeVisFlags(1, 1, 0);	mesh.faces[50].setSmGroup(8);
	mesh.faces[51].setVerts(38, 37, 19);	mesh.faces[51].setEdgeVisFlags(1, 1, 0);	mesh.faces[51].setSmGroup(8);
	mesh.faces[52].setVerts(20, 21, 39);	mesh.faces[52].setEdgeVisFlags(1, 1, 0);	mesh.faces[52].setSmGroup(8);
	mesh.faces[53].setVerts(39, 38, 20);	mesh.faces[53].setEdgeVisFlags(1, 1, 0);	mesh.faces[53].setSmGroup(8);
	mesh.faces[54].setVerts(21, 22, 40);	mesh.faces[54].setEdgeVisFlags(1, 1, 0);	mesh.faces[54].setSmGroup(8);
	mesh.faces[55].setVerts(40, 39, 21);	mesh.faces[55].setEdgeVisFlags(1, 1, 0);	mesh.faces[55].setSmGroup(8);
	mesh.faces[56].setVerts(22, 23, 41);	mesh.faces[56].setEdgeVisFlags(1, 1, 0);	mesh.faces[56].setSmGroup(8);
	mesh.faces[57].setVerts(41, 40, 22);	mesh.faces[57].setEdgeVisFlags(1, 1, 0);	mesh.faces[57].setSmGroup(8);
	mesh.faces[58].setVerts(23, 24, 42);	mesh.faces[58].setEdgeVisFlags(1, 1, 0);	mesh.faces[58].setSmGroup(8);
	mesh.faces[59].setVerts(42, 41, 23);	mesh.faces[59].setEdgeVisFlags(1, 1, 0);	mesh.faces[59].setSmGroup(8);
	mesh.faces[60].setVerts(24, 25, 43);	mesh.faces[60].setEdgeVisFlags(1, 1, 0);	mesh.faces[60].setSmGroup(8);
	mesh.faces[61].setVerts(43, 42, 24);	mesh.faces[61].setEdgeVisFlags(1, 1, 0);	mesh.faces[61].setSmGroup(8);
	mesh.faces[62].setVerts(25, 8, 26);		mesh.faces[62].setEdgeVisFlags(1, 1, 0);	mesh.faces[62].setSmGroup(8);
	mesh.faces[63].setVerts(26, 43, 25);	mesh.faces[63].setEdgeVisFlags(1, 1, 0);	mesh.faces[63].setSmGroup(8);
	mesh.faces[64].setVerts(26, 27, 28);	mesh.faces[64].setEdgeVisFlags(1, 1, 0);	mesh.faces[64].setSmGroup(1);
	mesh.faces[65].setVerts(28, 29, 30);	mesh.faces[65].setEdgeVisFlags(1, 1, 0);	mesh.faces[65].setSmGroup(1);
	mesh.faces[66].setVerts(30, 31, 32);	mesh.faces[66].setEdgeVisFlags(1, 1, 0);	mesh.faces[66].setSmGroup(1);
	mesh.faces[67].setVerts(28, 30, 32);	mesh.faces[67].setEdgeVisFlags(0, 0, 0);	mesh.faces[67].setSmGroup(1);
	mesh.faces[68].setVerts(32, 33, 34);	mesh.faces[68].setEdgeVisFlags(1, 1, 0);	mesh.faces[68].setSmGroup(1);
	mesh.faces[69].setVerts(34, 35, 36);	mesh.faces[69].setEdgeVisFlags(1, 1, 0);	mesh.faces[69].setSmGroup(1);
	mesh.faces[70].setVerts(32, 34, 36);	mesh.faces[70].setEdgeVisFlags(0, 0, 0);	mesh.faces[70].setSmGroup(1);
	mesh.faces[71].setVerts(36, 37, 38);	mesh.faces[71].setEdgeVisFlags(1, 1, 0);	mesh.faces[71].setSmGroup(1);
	mesh.faces[72].setVerts(38, 39, 40);	mesh.faces[72].setEdgeVisFlags(1, 1, 0);	mesh.faces[72].setSmGroup(1);
	mesh.faces[73].setVerts(36, 38, 40);	mesh.faces[73].setEdgeVisFlags(0, 0, 0);	mesh.faces[73].setSmGroup(1);
	mesh.faces[74].setVerts(32, 36, 40);	mesh.faces[74].setEdgeVisFlags(0, 0, 0);	mesh.faces[74].setSmGroup(1);
	mesh.faces[75].setVerts(28, 32, 40);	mesh.faces[75].setEdgeVisFlags(0, 0, 0);	mesh.faces[75].setSmGroup(1);
	mesh.faces[76].setVerts(40, 41, 42);	mesh.faces[76].setEdgeVisFlags(1, 1, 0);	mesh.faces[76].setSmGroup(1);
	mesh.faces[77].setVerts(28, 40, 42);	mesh.faces[77].setEdgeVisFlags(0, 0, 0);	mesh.faces[77].setSmGroup(1);
	mesh.faces[78].setVerts(26, 28, 42);	mesh.faces[78].setEdgeVisFlags(0, 0, 0);	mesh.faces[78].setSmGroup(1);
	mesh.faces[79].setVerts(43, 26, 42);	mesh.faces[79].setEdgeVisFlags(1, 0, 1);	mesh.faces[79].setSmGroup(1);
	mesh.faces[80].setVerts(60, 59, 58);	mesh.faces[80].setEdgeVisFlags(1, 1, 0);	mesh.faces[80].setSmGroup(1);
	mesh.faces[81].setVerts(58, 57, 56);	mesh.faces[81].setEdgeVisFlags(1, 1, 0);	mesh.faces[81].setSmGroup(1);
	mesh.faces[82].setVerts(56, 55, 54);	mesh.faces[82].setEdgeVisFlags(1, 1, 0);	mesh.faces[82].setSmGroup(1);
	mesh.faces[83].setVerts(58, 56, 54);	mesh.faces[83].setEdgeVisFlags(0, 0, 0);	mesh.faces[83].setSmGroup(1);
	mesh.faces[84].setVerts(54, 53, 52);	mesh.faces[84].setEdgeVisFlags(1, 1, 0);	mesh.faces[84].setSmGroup(1);
	mesh.faces[85].setVerts(52, 51, 50);	mesh.faces[85].setEdgeVisFlags(1, 1, 0);	mesh.faces[85].setSmGroup(1);
	mesh.faces[86].setVerts(54, 52, 50);	mesh.faces[86].setEdgeVisFlags(0, 0, 0);	mesh.faces[86].setSmGroup(1);
	mesh.faces[87].setVerts(50, 49, 48);	mesh.faces[87].setEdgeVisFlags(1, 1, 0);	mesh.faces[87].setSmGroup(1);
	mesh.faces[88].setVerts(48, 47, 46);	mesh.faces[88].setEdgeVisFlags(1, 1, 0);	mesh.faces[88].setSmGroup(1);
	mesh.faces[89].setVerts(50, 48, 46);	mesh.faces[89].setEdgeVisFlags(0, 0, 0);	mesh.faces[89].setSmGroup(1);
	mesh.faces[90].setVerts(54, 50, 46);	mesh.faces[90].setEdgeVisFlags(0, 0, 0);	mesh.faces[90].setSmGroup(1);
	mesh.faces[91].setVerts(58, 54, 46);	mesh.faces[91].setEdgeVisFlags(0, 0, 0);	mesh.faces[91].setSmGroup(1);
	mesh.faces[92].setVerts(46, 45, 44);	mesh.faces[92].setEdgeVisFlags(1, 1, 0);	mesh.faces[92].setSmGroup(1);
	mesh.faces[93].setVerts(58, 46, 44);	mesh.faces[93].setEdgeVisFlags(0, 0, 0);	mesh.faces[93].setSmGroup(1);
	mesh.faces[94].setVerts(60, 58, 44);	mesh.faces[94].setEdgeVisFlags(0, 0, 0);	mesh.faces[94].setSmGroup(1);
	mesh.faces[95].setVerts(61, 60, 44);	mesh.faces[95].setEdgeVisFlags(1, 0, 1);	mesh.faces[95].setSmGroup(1);
	mesh.faces[96].setVerts(44, 45, 63);	mesh.faces[96].setEdgeVisFlags(1, 1, 0);	mesh.faces[96].setSmGroup(8);
	mesh.faces[97].setVerts(63, 62, 44);	mesh.faces[97].setEdgeVisFlags(1, 1, 0);	mesh.faces[97].setSmGroup(8);
	mesh.faces[98].setVerts(45, 46, 64);	mesh.faces[98].setEdgeVisFlags(1, 1, 0);	mesh.faces[98].setSmGroup(8);
	mesh.faces[99].setVerts(64, 63, 45);	mesh.faces[99].setEdgeVisFlags(1, 1, 0);	mesh.faces[99].setSmGroup(8);
	mesh.faces[100].setVerts(46, 47, 65);	mesh.faces[100].setEdgeVisFlags(1, 1, 0);	mesh.faces[100].setSmGroup(8);
	mesh.faces[101].setVerts(65, 64, 46);	mesh.faces[101].setEdgeVisFlags(1, 1, 0);	mesh.faces[101].setSmGroup(8);
	mesh.faces[102].setVerts(47, 48, 66);	mesh.faces[102].setEdgeVisFlags(1, 1, 0);	mesh.faces[102].setSmGroup(8);
	mesh.faces[103].setVerts(66, 65, 47);	mesh.faces[103].setEdgeVisFlags(1, 1, 0);	mesh.faces[103].setSmGroup(8);
	mesh.faces[104].setVerts(48, 49, 67);	mesh.faces[104].setEdgeVisFlags(1, 1, 0);	mesh.faces[104].setSmGroup(8);
	mesh.faces[105].setVerts(67, 66, 48);	mesh.faces[105].setEdgeVisFlags(1, 1, 0);	mesh.faces[105].setSmGroup(8);
	mesh.faces[106].setVerts(49, 50, 68);	mesh.faces[106].setEdgeVisFlags(1, 1, 0);	mesh.faces[106].setSmGroup(8);
	mesh.faces[107].setVerts(68, 67, 49);	mesh.faces[107].setEdgeVisFlags(1, 1, 0);	mesh.faces[107].setSmGroup(8);
	mesh.faces[108].setVerts(50, 51, 69);	mesh.faces[108].setEdgeVisFlags(1, 1, 0);	mesh.faces[108].setSmGroup(8);
	mesh.faces[109].setVerts(69, 68, 50);	mesh.faces[109].setEdgeVisFlags(1, 1, 0);	mesh.faces[109].setSmGroup(8);
	mesh.faces[110].setVerts(51, 52, 70);	mesh.faces[110].setEdgeVisFlags(1, 1, 0);	mesh.faces[110].setSmGroup(8);
	mesh.faces[111].setVerts(70, 69, 51);	mesh.faces[111].setEdgeVisFlags(1, 1, 0);	mesh.faces[111].setSmGroup(8);
	mesh.faces[112].setVerts(52, 53, 71);	mesh.faces[112].setEdgeVisFlags(1, 1, 0);	mesh.faces[112].setSmGroup(8);
	mesh.faces[113].setVerts(71, 70, 52);	mesh.faces[113].setEdgeVisFlags(1, 1, 0);	mesh.faces[113].setSmGroup(8);
	mesh.faces[114].setVerts(53, 54, 72);	mesh.faces[114].setEdgeVisFlags(1, 1, 0);	mesh.faces[114].setSmGroup(8);
	mesh.faces[115].setVerts(72, 71, 53);	mesh.faces[115].setEdgeVisFlags(1, 1, 0);	mesh.faces[115].setSmGroup(8);
	mesh.faces[116].setVerts(54, 55, 73);	mesh.faces[116].setEdgeVisFlags(1, 1, 0);	mesh.faces[116].setSmGroup(8);
	mesh.faces[117].setVerts(73, 72, 54);	mesh.faces[117].setEdgeVisFlags(1, 1, 0);	mesh.faces[117].setSmGroup(8);
	mesh.faces[118].setVerts(55, 56, 74);	mesh.faces[118].setEdgeVisFlags(1, 1, 0);	mesh.faces[118].setSmGroup(8);
	mesh.faces[119].setVerts(74, 73, 55);	mesh.faces[119].setEdgeVisFlags(1, 1, 0);	mesh.faces[119].setSmGroup(8);
	mesh.faces[120].setVerts(56, 57, 75);	mesh.faces[120].setEdgeVisFlags(1, 1, 0);	mesh.faces[120].setSmGroup(8);
	mesh.faces[121].setVerts(75, 74, 56);	mesh.faces[121].setEdgeVisFlags(1, 1, 0);	mesh.faces[121].setSmGroup(8);
	mesh.faces[122].setVerts(57, 58, 76);	mesh.faces[122].setEdgeVisFlags(1, 1, 0);	mesh.faces[122].setSmGroup(8);
	mesh.faces[123].setVerts(76, 75, 57);	mesh.faces[123].setEdgeVisFlags(1, 1, 0);	mesh.faces[123].setSmGroup(8);
	mesh.faces[124].setVerts(58, 59, 77);	mesh.faces[124].setEdgeVisFlags(1, 1, 0);	mesh.faces[124].setSmGroup(8);
	mesh.faces[125].setVerts(77, 76, 58);	mesh.faces[125].setEdgeVisFlags(1, 1, 0);	mesh.faces[125].setSmGroup(8);
	mesh.faces[126].setVerts(59, 60, 78);	mesh.faces[126].setEdgeVisFlags(1, 1, 0);	mesh.faces[126].setSmGroup(8);
	mesh.faces[127].setVerts(78, 77, 59);	mesh.faces[127].setEdgeVisFlags(1, 1, 0);	mesh.faces[127].setSmGroup(8);
	mesh.faces[128].setVerts(60, 61, 79);	mesh.faces[128].setEdgeVisFlags(1, 1, 0);	mesh.faces[128].setSmGroup(8);
	mesh.faces[129].setVerts(79, 78, 60);	mesh.faces[129].setEdgeVisFlags(1, 1, 0);	mesh.faces[129].setSmGroup(8);
	mesh.faces[130].setVerts(61, 44, 62);	mesh.faces[130].setEdgeVisFlags(1, 1, 0);	mesh.faces[130].setSmGroup(8);
	mesh.faces[131].setVerts(62, 79, 61);	mesh.faces[131].setEdgeVisFlags(1, 1, 0);	mesh.faces[131].setSmGroup(8);
	mesh.faces[132].setVerts(62, 63, 64);	mesh.faces[132].setEdgeVisFlags(1, 1, 0);	mesh.faces[132].setSmGroup(1);
	mesh.faces[133].setVerts(64, 65, 66);	mesh.faces[133].setEdgeVisFlags(1, 1, 0);	mesh.faces[133].setSmGroup(1);
	mesh.faces[134].setVerts(66, 67, 68);	mesh.faces[134].setEdgeVisFlags(1, 1, 0);	mesh.faces[134].setSmGroup(1);
	mesh.faces[135].setVerts(64, 66, 68);	mesh.faces[135].setEdgeVisFlags(0, 0, 0);	mesh.faces[135].setSmGroup(1);
	mesh.faces[136].setVerts(68, 69, 70);	mesh.faces[136].setEdgeVisFlags(1, 1, 0);	mesh.faces[136].setSmGroup(1);
	mesh.faces[137].setVerts(70, 71, 72);	mesh.faces[137].setEdgeVisFlags(1, 1, 0);	mesh.faces[137].setSmGroup(1);
	mesh.faces[138].setVerts(68, 70, 72);	mesh.faces[138].setEdgeVisFlags(0, 0, 0);	mesh.faces[138].setSmGroup(1);
	mesh.faces[139].setVerts(72, 73, 74);	mesh.faces[139].setEdgeVisFlags(1, 1, 0);	mesh.faces[139].setSmGroup(1);
	mesh.faces[140].setVerts(74, 75, 76);	mesh.faces[140].setEdgeVisFlags(1, 1, 0);	mesh.faces[140].setSmGroup(1);
	mesh.faces[141].setVerts(72, 74, 76);	mesh.faces[141].setEdgeVisFlags(0, 0, 0);	mesh.faces[141].setSmGroup(1);
	mesh.faces[142].setVerts(68, 72, 76);	mesh.faces[142].setEdgeVisFlags(0, 0, 0);	mesh.faces[142].setSmGroup(1);
	mesh.faces[143].setVerts(64, 68, 76);	mesh.faces[143].setEdgeVisFlags(0, 0, 0);	mesh.faces[143].setSmGroup(1);
	mesh.faces[144].setVerts(76, 77, 78);	mesh.faces[144].setEdgeVisFlags(1, 1, 0);	mesh.faces[144].setSmGroup(1);
	mesh.faces[145].setVerts(64, 76, 78);	mesh.faces[145].setEdgeVisFlags(0, 0, 0);	mesh.faces[145].setSmGroup(1);
	mesh.faces[146].setVerts(62, 64, 78);	mesh.faces[146].setEdgeVisFlags(0, 0, 0);	mesh.faces[146].setSmGroup(1);
	mesh.faces[147].setVerts(79, 62, 78);	mesh.faces[147].setEdgeVisFlags(1, 0, 1);	mesh.faces[147].setSmGroup(1);
	mesh.faces[148].setVerts(92, 91, 90);	mesh.faces[148].setEdgeVisFlags(1, 1, 0);	mesh.faces[148].setSmGroup(1);
	mesh.faces[149].setVerts(90, 89, 88);	mesh.faces[149].setEdgeVisFlags(1, 1, 0);	mesh.faces[149].setSmGroup(1);
	mesh.faces[150].setVerts(88, 87, 86);	mesh.faces[150].setEdgeVisFlags(1, 1, 0);	mesh.faces[150].setSmGroup(1);
	mesh.faces[151].setVerts(90, 88, 86);	mesh.faces[151].setEdgeVisFlags(0, 0, 0);	mesh.faces[151].setSmGroup(1);
	mesh.faces[152].setVerts(86, 85, 84);	mesh.faces[152].setEdgeVisFlags(1, 1, 0);	mesh.faces[152].setSmGroup(1);
	mesh.faces[153].setVerts(84, 83, 82);	mesh.faces[153].setEdgeVisFlags(1, 1, 0);	mesh.faces[153].setSmGroup(1);
	mesh.faces[154].setVerts(86, 84, 82);	mesh.faces[154].setEdgeVisFlags(0, 0, 0);	mesh.faces[154].setSmGroup(1);
	mesh.faces[155].setVerts(82, 81, 80);	mesh.faces[155].setEdgeVisFlags(1, 1, 0);	mesh.faces[155].setSmGroup(1);
	mesh.faces[156].setVerts(86, 82, 80);	mesh.faces[156].setEdgeVisFlags(0, 0, 0);	mesh.faces[156].setSmGroup(1);
	mesh.faces[157].setVerts(90, 86, 80);	mesh.faces[157].setEdgeVisFlags(0, 0, 0);	mesh.faces[157].setSmGroup(1);
	mesh.faces[158].setVerts(92, 90, 80);	mesh.faces[158].setEdgeVisFlags(0, 0, 0);	mesh.faces[158].setSmGroup(1);
	mesh.faces[159].setVerts(93, 92, 80);	mesh.faces[159].setEdgeVisFlags(1, 0, 1);	mesh.faces[159].setSmGroup(1);
	mesh.faces[160].setVerts(80, 81, 95);	mesh.faces[160].setEdgeVisFlags(1, 1, 0);	mesh.faces[160].setSmGroup(8);
	mesh.faces[161].setVerts(95, 94, 80);	mesh.faces[161].setEdgeVisFlags(1, 1, 0);	mesh.faces[161].setSmGroup(8);
	mesh.faces[162].setVerts(81, 82, 96);	mesh.faces[162].setEdgeVisFlags(1, 1, 0);	mesh.faces[162].setSmGroup(8);
	mesh.faces[163].setVerts(96, 95, 81);	mesh.faces[163].setEdgeVisFlags(1, 1, 0);	mesh.faces[163].setSmGroup(8);
	mesh.faces[164].setVerts(82, 83, 97);	mesh.faces[164].setEdgeVisFlags(1, 1, 0);	mesh.faces[164].setSmGroup(8);
	mesh.faces[165].setVerts(97, 96, 82);	mesh.faces[165].setEdgeVisFlags(1, 1, 0);	mesh.faces[165].setSmGroup(8);
	mesh.faces[166].setVerts(83, 84, 98);	mesh.faces[166].setEdgeVisFlags(1, 1, 0);	mesh.faces[166].setSmGroup(8);
	mesh.faces[167].setVerts(98, 97, 83);	mesh.faces[167].setEdgeVisFlags(1, 1, 0);	mesh.faces[167].setSmGroup(8);
	mesh.faces[168].setVerts(84, 85, 99);	mesh.faces[168].setEdgeVisFlags(1, 1, 0);	mesh.faces[168].setSmGroup(8);
	mesh.faces[169].setVerts(99, 98, 84);	mesh.faces[169].setEdgeVisFlags(1, 1, 0);	mesh.faces[169].setSmGroup(8);
	mesh.faces[170].setVerts(85, 86, 100);	mesh.faces[170].setEdgeVisFlags(1, 1, 0);	mesh.faces[170].setSmGroup(8);
	mesh.faces[171].setVerts(100, 99, 85);	mesh.faces[171].setEdgeVisFlags(1, 1, 0);	mesh.faces[171].setSmGroup(8);
	mesh.faces[172].setVerts(86, 87, 101);	mesh.faces[172].setEdgeVisFlags(1, 1, 0);	mesh.faces[172].setSmGroup(8);
	mesh.faces[173].setVerts(101, 100, 86);	mesh.faces[173].setEdgeVisFlags(1, 1, 0);	mesh.faces[173].setSmGroup(8);
	mesh.faces[174].setVerts(87, 88, 102);	mesh.faces[174].setEdgeVisFlags(1, 1, 0);	mesh.faces[174].setSmGroup(8);
	mesh.faces[175].setVerts(102, 101, 87);	mesh.faces[175].setEdgeVisFlags(1, 1, 0);	mesh.faces[175].setSmGroup(8);
	mesh.faces[176].setVerts(88, 89, 103);	mesh.faces[176].setEdgeVisFlags(1, 1, 0);	mesh.faces[176].setSmGroup(8);
	mesh.faces[177].setVerts(103, 102, 88);	mesh.faces[177].setEdgeVisFlags(1, 1, 0);	mesh.faces[177].setSmGroup(8);
	mesh.faces[178].setVerts(89, 90, 104);	mesh.faces[178].setEdgeVisFlags(1, 1, 0);	mesh.faces[178].setSmGroup(8);
	mesh.faces[179].setVerts(104, 103, 89);	mesh.faces[179].setEdgeVisFlags(1, 1, 0);	mesh.faces[179].setSmGroup(8);
	mesh.faces[180].setVerts(90, 91, 105);	mesh.faces[180].setEdgeVisFlags(1, 1, 0);	mesh.faces[180].setSmGroup(8);
	mesh.faces[181].setVerts(105, 104, 90);	mesh.faces[181].setEdgeVisFlags(1, 1, 0);	mesh.faces[181].setSmGroup(8);
	mesh.faces[182].setVerts(91, 92, 106);	mesh.faces[182].setEdgeVisFlags(1, 1, 0);	mesh.faces[182].setSmGroup(8);
	mesh.faces[183].setVerts(106, 105, 91);	mesh.faces[183].setEdgeVisFlags(1, 1, 0);	mesh.faces[183].setSmGroup(8);
	mesh.faces[184].setVerts(92, 93, 107);	mesh.faces[184].setEdgeVisFlags(1, 1, 0);	mesh.faces[184].setSmGroup(8);
	mesh.faces[185].setVerts(107, 106, 92);	mesh.faces[185].setEdgeVisFlags(1, 1, 0);	mesh.faces[185].setSmGroup(8);
	mesh.faces[186].setVerts(93, 80, 94);	mesh.faces[186].setEdgeVisFlags(1, 1, 0);	mesh.faces[186].setSmGroup(8);
	mesh.faces[187].setVerts(94, 107, 93);	mesh.faces[187].setEdgeVisFlags(1, 1, 0);	mesh.faces[187].setSmGroup(8);
	mesh.faces[188].setVerts(94, 95, 96);	mesh.faces[188].setEdgeVisFlags(1, 1, 0);	mesh.faces[188].setSmGroup(1);
	mesh.faces[189].setVerts(96, 97, 98);	mesh.faces[189].setEdgeVisFlags(1, 1, 0);	mesh.faces[189].setSmGroup(1);
	mesh.faces[190].setVerts(98, 99, 100);	mesh.faces[190].setEdgeVisFlags(1, 1, 0);	mesh.faces[190].setSmGroup(1);
	mesh.faces[191].setVerts(96, 98, 100);	mesh.faces[191].setEdgeVisFlags(0, 0, 0);	mesh.faces[191].setSmGroup(1);
	mesh.faces[192].setVerts(100, 101, 102);	mesh.faces[192].setEdgeVisFlags(1, 1, 0);	mesh.faces[192].setSmGroup(1);
	mesh.faces[193].setVerts(102, 103, 104);	mesh.faces[193].setEdgeVisFlags(1, 1, 0);	mesh.faces[193].setSmGroup(1);
	mesh.faces[194].setVerts(100, 102, 104);	mesh.faces[194].setEdgeVisFlags(0, 0, 0);	mesh.faces[194].setSmGroup(1);
	mesh.faces[195].setVerts(104, 105, 106);	mesh.faces[195].setEdgeVisFlags(1, 1, 0);	mesh.faces[195].setSmGroup(1);
	mesh.faces[196].setVerts(100, 104, 106);	mesh.faces[196].setEdgeVisFlags(0, 0, 0);	mesh.faces[196].setSmGroup(1);
	mesh.faces[197].setVerts(96, 100, 106);		mesh.faces[197].setEdgeVisFlags(0, 0, 0);	mesh.faces[197].setSmGroup(1);
	mesh.faces[198].setVerts(94, 96, 106);		mesh.faces[198].setEdgeVisFlags(0, 0, 0);	mesh.faces[198].setSmGroup(1);
	mesh.faces[199].setVerts(107, 94, 106);		mesh.faces[199].setEdgeVisFlags(1, 0, 1);	mesh.faces[199].setSmGroup(1);
	mesh.faces[200].setVerts(115, 114, 113);	mesh.faces[200].setEdgeVisFlags(1, 1, 0);	mesh.faces[200].setSmGroup(1);
	mesh.faces[201].setVerts(113, 112, 111);	mesh.faces[201].setEdgeVisFlags(1, 1, 0);	mesh.faces[201].setSmGroup(1);
	mesh.faces[202].setVerts(111, 110, 109);	mesh.faces[202].setEdgeVisFlags(1, 1, 0);	mesh.faces[202].setSmGroup(1);
	mesh.faces[203].setVerts(113, 111, 109);	mesh.faces[203].setEdgeVisFlags(0, 0, 0);	mesh.faces[203].setSmGroup(1);
	mesh.faces[204].setVerts(115, 113, 109);	mesh.faces[204].setEdgeVisFlags(0, 0, 0);	mesh.faces[204].setSmGroup(1);
	mesh.faces[205].setVerts(115, 109, 108);	mesh.faces[205].setEdgeVisFlags(0, 1, 0);	mesh.faces[205].setSmGroup(1);
	mesh.faces[206].setVerts(116, 115, 108);	mesh.faces[206].setEdgeVisFlags(1, 0, 1);	mesh.faces[206].setSmGroup(1);
	mesh.faces[207].setVerts(108, 109, 118);	mesh.faces[207].setEdgeVisFlags(1, 1, 0);	mesh.faces[207].setSmGroup(8);
	mesh.faces[208].setVerts(118, 117, 108);	mesh.faces[208].setEdgeVisFlags(1, 1, 0);	mesh.faces[208].setSmGroup(8);
	mesh.faces[209].setVerts(109, 110, 119);	mesh.faces[209].setEdgeVisFlags(1, 1, 0);	mesh.faces[209].setSmGroup(8);
	mesh.faces[210].setVerts(119, 118, 109);	mesh.faces[210].setEdgeVisFlags(1, 1, 0);	mesh.faces[210].setSmGroup(8);
	mesh.faces[211].setVerts(110, 111, 120);	mesh.faces[211].setEdgeVisFlags(1, 1, 0);	mesh.faces[211].setSmGroup(8);
	mesh.faces[212].setVerts(120, 119, 110);	mesh.faces[212].setEdgeVisFlags(1, 1, 0);	mesh.faces[212].setSmGroup(8);
	mesh.faces[213].setVerts(111, 112, 121);	mesh.faces[213].setEdgeVisFlags(1, 1, 0);	mesh.faces[213].setSmGroup(8);
	mesh.faces[214].setVerts(121, 120, 111);	mesh.faces[214].setEdgeVisFlags(1, 1, 0);	mesh.faces[214].setSmGroup(8);
	mesh.faces[215].setVerts(112, 113, 122);	mesh.faces[215].setEdgeVisFlags(1, 1, 0);	mesh.faces[215].setSmGroup(8);
	mesh.faces[216].setVerts(122, 121, 112);	mesh.faces[216].setEdgeVisFlags(1, 1, 0);	mesh.faces[216].setSmGroup(8);
	mesh.faces[217].setVerts(113, 114, 123);	mesh.faces[217].setEdgeVisFlags(1, 1, 0);	mesh.faces[217].setSmGroup(8);
	mesh.faces[218].setVerts(123, 122, 113);	mesh.faces[218].setEdgeVisFlags(1, 1, 0);	mesh.faces[218].setSmGroup(8);
	mesh.faces[219].setVerts(114, 115, 124);	mesh.faces[219].setEdgeVisFlags(1, 1, 0);	mesh.faces[219].setSmGroup(8);
	mesh.faces[220].setVerts(124, 123, 114);	mesh.faces[220].setEdgeVisFlags(1, 1, 0);	mesh.faces[220].setSmGroup(8);
	mesh.faces[221].setVerts(115, 116, 125);	mesh.faces[221].setEdgeVisFlags(1, 1, 0);	mesh.faces[221].setSmGroup(8);
	mesh.faces[222].setVerts(125, 124, 115);	mesh.faces[222].setEdgeVisFlags(1, 1, 0);	mesh.faces[222].setSmGroup(8);
	mesh.faces[223].setVerts(116, 108, 117);	mesh.faces[223].setEdgeVisFlags(1, 1, 0);	mesh.faces[223].setSmGroup(8);
	mesh.faces[224].setVerts(117, 125, 116);	mesh.faces[224].setEdgeVisFlags(1, 1, 0);	mesh.faces[224].setSmGroup(8);
	mesh.faces[225].setVerts(117, 118, 119);	mesh.faces[225].setEdgeVisFlags(1, 1, 0);	mesh.faces[225].setSmGroup(1);
	mesh.faces[226].setVerts(119, 120, 121);	mesh.faces[226].setEdgeVisFlags(1, 1, 0);	mesh.faces[226].setSmGroup(1);
	mesh.faces[227].setVerts(121, 122, 123);	mesh.faces[227].setEdgeVisFlags(1, 1, 0);	mesh.faces[227].setSmGroup(1);
	mesh.faces[228].setVerts(119, 121, 123);	mesh.faces[228].setEdgeVisFlags(0, 0, 0);	mesh.faces[228].setSmGroup(1);
	mesh.faces[229].setVerts(117, 119, 123);	mesh.faces[229].setEdgeVisFlags(0, 0, 0);	mesh.faces[229].setSmGroup(1);
	mesh.faces[230].setVerts(117, 123, 124);	mesh.faces[230].setEdgeVisFlags(0, 1, 0);	mesh.faces[230].setSmGroup(1);
	mesh.faces[231].setVerts(125, 117, 124);	mesh.faces[231].setEdgeVisFlags(1, 0, 1);	mesh.faces[231].setSmGroup(1);
	mesh.faces[232].setVerts(126, 128, 129);	mesh.faces[232].setEdgeVisFlags(1, 1, 0);	mesh.faces[232].setSmGroup(2);
	mesh.faces[233].setVerts(129, 127, 126);	mesh.faces[233].setEdgeVisFlags(1, 1, 0);	mesh.faces[233].setSmGroup(2);
	mesh.faces[234].setVerts(130, 131, 133);	mesh.faces[234].setEdgeVisFlags(1, 1, 0);	mesh.faces[234].setSmGroup(4);
	mesh.faces[235].setVerts(133, 132, 130);	mesh.faces[235].setEdgeVisFlags(1, 1, 0);	mesh.faces[235].setSmGroup(4);
	mesh.faces[236].setVerts(126, 127, 131);	mesh.faces[236].setEdgeVisFlags(1, 1, 0);	mesh.faces[236].setSmGroup(8);
	mesh.faces[237].setVerts(131, 130, 126);	mesh.faces[237].setEdgeVisFlags(1, 1, 0);	mesh.faces[237].setSmGroup(8);
	mesh.faces[238].setVerts(127, 129, 133);	mesh.faces[238].setEdgeVisFlags(1, 1, 0);	mesh.faces[238].setSmGroup(16);
	mesh.faces[239].setVerts(133, 131, 127);	mesh.faces[239].setEdgeVisFlags(1, 1, 0);	mesh.faces[239].setSmGroup(16);
	mesh.faces[240].setVerts(129, 128, 132);	mesh.faces[240].setEdgeVisFlags(1, 1, 0);	mesh.faces[240].setSmGroup(32);
	mesh.faces[241].setVerts(132, 133, 129);	mesh.faces[241].setEdgeVisFlags(1, 1, 0);	mesh.faces[241].setSmGroup(32);
	mesh.faces[242].setVerts(128, 126, 130);	mesh.faces[242].setEdgeVisFlags(1, 1, 0);	mesh.faces[242].setSmGroup(64);
	mesh.faces[243].setVerts(130, 132, 128);	mesh.faces[243].setEdgeVisFlags(1, 1, 0);	mesh.faces[243].setSmGroup(64);
	mesh.faces[244].setVerts(141, 140, 139);	mesh.faces[244].setEdgeVisFlags(1, 1, 0);	mesh.faces[244].setSmGroup(1);
	mesh.faces[245].setVerts(139, 138, 137);	mesh.faces[245].setEdgeVisFlags(1, 1, 0);	mesh.faces[245].setSmGroup(1);
	mesh.faces[246].setVerts(137, 136, 135);	mesh.faces[246].setEdgeVisFlags(1, 1, 0);	mesh.faces[246].setSmGroup(1);
	mesh.faces[247].setVerts(139, 137, 135);	mesh.faces[247].setEdgeVisFlags(0, 0, 0);	mesh.faces[247].setSmGroup(1);
	mesh.faces[248].setVerts(141, 139, 135);	mesh.faces[248].setEdgeVisFlags(0, 0, 0);	mesh.faces[248].setSmGroup(1);
	mesh.faces[249].setVerts(141, 135, 134);	mesh.faces[249].setEdgeVisFlags(0, 1, 0);	mesh.faces[249].setSmGroup(1);
	mesh.faces[250].setVerts(142, 141, 134);	mesh.faces[250].setEdgeVisFlags(1, 0, 1);	mesh.faces[250].setSmGroup(1);
	mesh.faces[251].setVerts(134, 135, 144);	mesh.faces[251].setEdgeVisFlags(1, 1, 0);	mesh.faces[251].setSmGroup(2);
	mesh.faces[252].setVerts(144, 143, 134);	mesh.faces[252].setEdgeVisFlags(1, 1, 0);	mesh.faces[252].setSmGroup(2);
	mesh.faces[253].setVerts(135, 136, 145);	mesh.faces[253].setEdgeVisFlags(1, 1, 0);	mesh.faces[253].setSmGroup(2);
	mesh.faces[254].setVerts(145, 144, 135);	mesh.faces[254].setEdgeVisFlags(1, 1, 0);	mesh.faces[254].setSmGroup(2);
	mesh.faces[255].setVerts(136, 137, 146);	mesh.faces[255].setEdgeVisFlags(1, 1, 0);	mesh.faces[255].setSmGroup(2);
	mesh.faces[256].setVerts(146, 145, 136);	mesh.faces[256].setEdgeVisFlags(1, 1, 0);	mesh.faces[256].setSmGroup(2);
	mesh.faces[257].setVerts(137, 138, 147);	mesh.faces[257].setEdgeVisFlags(1, 1, 0);	mesh.faces[257].setSmGroup(2);
	mesh.faces[258].setVerts(147, 146, 137);	mesh.faces[258].setEdgeVisFlags(1, 1, 0);	mesh.faces[258].setSmGroup(2);
	mesh.faces[259].setVerts(138, 139, 148);	mesh.faces[259].setEdgeVisFlags(1, 1, 0);	mesh.faces[259].setSmGroup(2);
	mesh.faces[260].setVerts(148, 147, 138);	mesh.faces[260].setEdgeVisFlags(1, 1, 0);	mesh.faces[260].setSmGroup(2);
	mesh.faces[261].setVerts(139, 140, 149);	mesh.faces[261].setEdgeVisFlags(1, 1, 0);	mesh.faces[261].setSmGroup(2);
	mesh.faces[262].setVerts(149, 148, 139);	mesh.faces[262].setEdgeVisFlags(1, 1, 0);	mesh.faces[262].setSmGroup(2);
	mesh.faces[263].setVerts(140, 141, 150);	mesh.faces[263].setEdgeVisFlags(1, 1, 0);	mesh.faces[263].setSmGroup(2);
	mesh.faces[264].setVerts(150, 149, 140);	mesh.faces[264].setEdgeVisFlags(1, 1, 0);	mesh.faces[264].setSmGroup(2);
	mesh.faces[265].setVerts(141, 142, 151);	mesh.faces[265].setEdgeVisFlags(1, 1, 0);	mesh.faces[265].setSmGroup(2);
	mesh.faces[266].setVerts(151, 150, 141);	mesh.faces[266].setEdgeVisFlags(1, 1, 0);	mesh.faces[266].setSmGroup(2);
	mesh.faces[267].setVerts(142, 134, 143);	mesh.faces[267].setEdgeVisFlags(1, 1, 0);	mesh.faces[267].setSmGroup(2);
	mesh.faces[268].setVerts(143, 151, 142);	mesh.faces[268].setEdgeVisFlags(1, 1, 0);	mesh.faces[268].setSmGroup(2);
	mesh.faces[269].setVerts(170, 171, 172);	mesh.faces[269].setEdgeVisFlags(1, 1, 0);	mesh.faces[269].setSmGroup(1);
	mesh.faces[270].setVerts(172, 173, 174);	mesh.faces[270].setEdgeVisFlags(1, 1, 0);	mesh.faces[270].setSmGroup(1);
	mesh.faces[271].setVerts(174, 175, 176);	mesh.faces[271].setEdgeVisFlags(1, 1, 0);	mesh.faces[271].setSmGroup(1);
	mesh.faces[272].setVerts(172, 174, 176);	mesh.faces[272].setEdgeVisFlags(0, 0, 0);	mesh.faces[272].setSmGroup(1);
	mesh.faces[273].setVerts(170, 172, 176);	mesh.faces[273].setEdgeVisFlags(0, 0, 0);	mesh.faces[273].setSmGroup(1);
	mesh.faces[274].setVerts(170, 176, 177);	mesh.faces[274].setEdgeVisFlags(0, 1, 0);	mesh.faces[274].setSmGroup(1);
	mesh.faces[275].setVerts(178, 170, 177);	mesh.faces[275].setEdgeVisFlags(1, 0, 1);	mesh.faces[275].setSmGroup(1);
	mesh.faces[276].setVerts(143, 144, 153);	mesh.faces[276].setEdgeVisFlags(1, 1, 0);	mesh.faces[276].setSmGroup(2);
	mesh.faces[277].setVerts(153, 152, 143);	mesh.faces[277].setEdgeVisFlags(1, 1, 0);	mesh.faces[277].setSmGroup(2);
	mesh.faces[278].setVerts(144, 145, 154);	mesh.faces[278].setEdgeVisFlags(1, 1, 0);	mesh.faces[278].setSmGroup(2);
	mesh.faces[279].setVerts(154, 153, 144);	mesh.faces[279].setEdgeVisFlags(1, 1, 0);	mesh.faces[279].setSmGroup(2);
	mesh.faces[280].setVerts(145, 146, 155);	mesh.faces[280].setEdgeVisFlags(1, 1, 0);	mesh.faces[280].setSmGroup(2);
	mesh.faces[281].setVerts(155, 154, 145);	mesh.faces[281].setEdgeVisFlags(1, 1, 0);	mesh.faces[281].setSmGroup(2);
	mesh.faces[282].setVerts(146, 147, 156);	mesh.faces[282].setEdgeVisFlags(1, 1, 0);	mesh.faces[282].setSmGroup(2);
	mesh.faces[283].setVerts(156, 155, 146);	mesh.faces[283].setEdgeVisFlags(1, 1, 0);	mesh.faces[283].setSmGroup(2);
	mesh.faces[284].setVerts(147, 148, 157);	mesh.faces[284].setEdgeVisFlags(1, 1, 0);	mesh.faces[284].setSmGroup(2);
	mesh.faces[285].setVerts(157, 156, 147);	mesh.faces[285].setEdgeVisFlags(1, 1, 0);	mesh.faces[285].setSmGroup(2);
	mesh.faces[286].setVerts(148, 149, 158);	mesh.faces[286].setEdgeVisFlags(1, 1, 0);	mesh.faces[286].setSmGroup(2);
	mesh.faces[287].setVerts(158, 157, 148);	mesh.faces[287].setEdgeVisFlags(1, 1, 0);	mesh.faces[287].setSmGroup(2);
	mesh.faces[288].setVerts(149, 150, 159);	mesh.faces[288].setEdgeVisFlags(1, 1, 0);	mesh.faces[288].setSmGroup(2);
	mesh.faces[289].setVerts(159, 158, 149);	mesh.faces[289].setEdgeVisFlags(1, 1, 0);	mesh.faces[289].setSmGroup(2);
	mesh.faces[290].setVerts(150, 151, 160);	mesh.faces[290].setEdgeVisFlags(1, 1, 0);	mesh.faces[290].setSmGroup(2);
	mesh.faces[291].setVerts(160, 159, 150);	mesh.faces[291].setEdgeVisFlags(1, 1, 0);	mesh.faces[291].setSmGroup(2);
	mesh.faces[292].setVerts(151, 143, 152);	mesh.faces[292].setEdgeVisFlags(1, 1, 0);	mesh.faces[292].setSmGroup(2);
	mesh.faces[293].setVerts(152, 160, 151);	mesh.faces[293].setEdgeVisFlags(1, 1, 0);	mesh.faces[293].setSmGroup(2);
	mesh.faces[294].setVerts(152, 153, 162);	mesh.faces[294].setEdgeVisFlags(1, 1, 0);	mesh.faces[294].setSmGroup(2);
	mesh.faces[295].setVerts(162, 161, 152);	mesh.faces[295].setEdgeVisFlags(1, 1, 0);	mesh.faces[295].setSmGroup(2);
	mesh.faces[296].setVerts(153, 154, 163);	mesh.faces[296].setEdgeVisFlags(1, 1, 0);	mesh.faces[296].setSmGroup(2);
	mesh.faces[297].setVerts(163, 162, 153);	mesh.faces[297].setEdgeVisFlags(1, 1, 0);	mesh.faces[297].setSmGroup(2);
	mesh.faces[298].setVerts(154, 155, 164);	mesh.faces[298].setEdgeVisFlags(1, 1, 0);	mesh.faces[298].setSmGroup(2);
	mesh.faces[299].setVerts(164, 163, 154);	mesh.faces[299].setEdgeVisFlags(1, 1, 0);	mesh.faces[299].setSmGroup(2);
	mesh.faces[300].setVerts(155, 156, 165);	mesh.faces[300].setEdgeVisFlags(1, 1, 0);	mesh.faces[300].setSmGroup(2);
	mesh.faces[301].setVerts(165, 164, 155);	mesh.faces[301].setEdgeVisFlags(1, 1, 0);	mesh.faces[301].setSmGroup(2);
	mesh.faces[302].setVerts(156, 157, 166);	mesh.faces[302].setEdgeVisFlags(1, 1, 0);	mesh.faces[302].setSmGroup(2);
	mesh.faces[303].setVerts(166, 165, 156);	mesh.faces[303].setEdgeVisFlags(1, 1, 0);	mesh.faces[303].setSmGroup(2);
	mesh.faces[304].setVerts(157, 158, 167);	mesh.faces[304].setEdgeVisFlags(1, 1, 0);	mesh.faces[304].setSmGroup(2);
	mesh.faces[305].setVerts(167, 166, 157);	mesh.faces[305].setEdgeVisFlags(1, 1, 0);	mesh.faces[305].setSmGroup(2);
	mesh.faces[306].setVerts(158, 159, 168);	mesh.faces[306].setEdgeVisFlags(1, 1, 0);	mesh.faces[306].setSmGroup(2);
	mesh.faces[307].setVerts(168, 167, 158);	mesh.faces[307].setEdgeVisFlags(1, 1, 0);	mesh.faces[307].setSmGroup(2);
	mesh.faces[308].setVerts(159, 160, 169);	mesh.faces[308].setEdgeVisFlags(1, 1, 0);	mesh.faces[308].setSmGroup(2);
	mesh.faces[309].setVerts(169, 168, 159);	mesh.faces[309].setEdgeVisFlags(1, 1, 0);	mesh.faces[309].setSmGroup(2);
	mesh.faces[310].setVerts(160, 152, 161);	mesh.faces[310].setEdgeVisFlags(1, 1, 0);	mesh.faces[310].setSmGroup(2);
	mesh.faces[311].setVerts(161, 169, 160);	mesh.faces[311].setEdgeVisFlags(1, 1, 0);	mesh.faces[311].setSmGroup(2);
	mesh.faces[312].setVerts(161, 162, 171);	mesh.faces[312].setEdgeVisFlags(1, 1, 0);	mesh.faces[312].setSmGroup(4);
	mesh.faces[313].setVerts(171, 170, 161);	mesh.faces[313].setEdgeVisFlags(1, 1, 0);	mesh.faces[313].setSmGroup(4);
	mesh.faces[314].setVerts(162, 163, 172);	mesh.faces[314].setEdgeVisFlags(1, 1, 0);	mesh.faces[314].setSmGroup(4);
	mesh.faces[315].setVerts(172, 171, 162);	mesh.faces[315].setEdgeVisFlags(1, 1, 0);	mesh.faces[315].setSmGroup(4);
	mesh.faces[316].setVerts(163, 164, 173);	mesh.faces[316].setEdgeVisFlags(1, 1, 0);	mesh.faces[316].setSmGroup(4);
	mesh.faces[317].setVerts(173, 172, 163);	mesh.faces[317].setEdgeVisFlags(1, 1, 0);	mesh.faces[317].setSmGroup(4);
	mesh.faces[318].setVerts(164, 165, 174);	mesh.faces[318].setEdgeVisFlags(1, 1, 0);	mesh.faces[318].setSmGroup(4);
	mesh.faces[319].setVerts(174, 173, 164);	mesh.faces[319].setEdgeVisFlags(1, 1, 0);	mesh.faces[319].setSmGroup(4);
	mesh.faces[320].setVerts(165, 166, 175);	mesh.faces[320].setEdgeVisFlags(1, 1, 0);	mesh.faces[320].setSmGroup(4);
	mesh.faces[321].setVerts(175, 174, 165);	mesh.faces[321].setEdgeVisFlags(1, 1, 0);	mesh.faces[321].setSmGroup(4);
	mesh.faces[322].setVerts(166, 167, 176);	mesh.faces[322].setEdgeVisFlags(1, 1, 0);	mesh.faces[322].setSmGroup(4);
	mesh.faces[323].setVerts(176, 175, 166);	mesh.faces[323].setEdgeVisFlags(1, 1, 0);	mesh.faces[323].setSmGroup(4);
	mesh.faces[324].setVerts(167, 168, 177);	mesh.faces[324].setEdgeVisFlags(1, 1, 0);	mesh.faces[324].setSmGroup(4);
	mesh.faces[325].setVerts(177, 176, 167);	mesh.faces[325].setEdgeVisFlags(1, 1, 0);	mesh.faces[325].setSmGroup(4);
	mesh.faces[326].setVerts(168, 169, 178);	mesh.faces[326].setEdgeVisFlags(1, 1, 0);	mesh.faces[326].setSmGroup(4);
	mesh.faces[327].setVerts(178, 177, 168);	mesh.faces[327].setEdgeVisFlags(1, 1, 0);	mesh.faces[327].setSmGroup(4);
	mesh.faces[328].setVerts(169, 161, 170);	mesh.faces[328].setEdgeVisFlags(1, 1, 0);	mesh.faces[328].setSmGroup(4);
	mesh.faces[329].setVerts(170, 178, 169);	mesh.faces[329].setEdgeVisFlags(1, 1, 0);	mesh.faces[329].setSmGroup(4);

	mesh.buildNormals();
	mesh.EnableEdgeList(1);
	meshBuilt = 1;
	}

void SimpleCamera::UpdateKeyBrackets(TimeValue t) {
	fovSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_FOV,t));
	lensSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_FOV,t));
	nearSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_HITHER,t));
	farSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_YON,t));
	envNearSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_NRANGE,t));
	envFarSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_FRANGE,t));
	}

void SimpleCamera::UpdateUI(TimeValue t)
{
   if ( hSimpleCamParams && !waitPostLoad && 
        DLGetWindowLongPtr<SimpleCamera*>(hSimpleCamParams)==this && pblock )
	{
		fovSpin->SetValue( RadToDeg(WFOVtoCurFOV(GetFOV(t))), FALSE );
		lensSpin->SetValue(FOVtoMM(GetFOV(t)), FALSE);
		nearSpin->SetValue( GetClipDist(t, CAM_NEAR_CLIP), FALSE );
		farSpin->SetValue( GetClipDist(t, CAM_FAR_CLIP), FALSE );
		envNearSpin->SetValue( GetEnvRange(t, ENV_NEAR_RANGE), FALSE );
		envFarSpin->SetValue( GetEnvRange(t, ENV_FAR_RANGE), FALSE );
		UpdateKeyBrackets(t);

		CheckDlgButton(hSimpleCamParams, IDC_ENABLE_MP_EFFECT, GetMultiPassEffectEnabled(t) );
		//CheckDlgButton(hSimpleCamParams, IDC_MP_EFFECT_REFFECT_PER_PASS, GetMPEffect_REffectPerPass() );

		tdistSpin->SetValue( GetTDist(t), FALSE );

		// DS 8/28/00
		SendMessage( GetDlgItem(hSimpleCamParams, IDC_CAM_TYPE), CB_SETCURSEL, hasTarget, (LPARAM)0 );
	}
}

#define CAMERA_VERSION 5	// current version // mjm - 07.17.00
#define DOF_VERSION 2		// current version // nac - 12.08.00

#define CAMERA_PBLOCK_COUNT	9

static ParamBlockDescID descV0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE, 2 } };	// TDIST

static ParamBlockDescID descV1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE, 2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE, 3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE, 4 } };	// YON

static ParamBlockDescID descV2[] = {
	{ TYPE_FLOAT, NULL, TRUE, 1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE, 2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE, 3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// YON
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// NEAR ENV RANGE
	{ TYPE_FLOAT, NULL, TRUE, 6 } };	// FAR ENV RANGE

static ParamBlockDescID descV3[] = {
	{ TYPE_FLOAT, NULL, TRUE,  1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE,  2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE,  3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE,  4 },		// YON
	{ TYPE_FLOAT, NULL, TRUE,  5 },		// NEAR ENV RANGE
	{ TYPE_FLOAT, NULL, TRUE,  6 },		// FAR ENV RANGE
	{ TYPE_BOOL,  NULL, FALSE, 7 } };	// MULTI PASS EFFECT - RENDER EFFECTS PER PASS

static ParamBlockDescID descV4[] = {
	{ TYPE_FLOAT, NULL, TRUE,  1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE,  2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE,  3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE,  4 },		// YON
	{ TYPE_FLOAT, NULL, TRUE,  5 },		// NEAR ENV RANGE
	{ TYPE_FLOAT, NULL, TRUE,  6 },		// FAR ENV RANGE
	{ TYPE_BOOL,  NULL, FALSE, 7 },		// MULTI PASS EFFECT ENABLE
	{ TYPE_BOOL,  NULL, FALSE, 8 } };	// MULTI PASS EFFECT - RENDER EFFECTS PER PASS
static ParamBlockDescID descV5[] = {
	{ TYPE_FLOAT, NULL, TRUE,  1 },		// FOV
	{ TYPE_FLOAT, NULL, TRUE,  2 },		// TDIST
	{ TYPE_FLOAT, NULL, TRUE,  3 },		// HITHER
	{ TYPE_FLOAT, NULL, TRUE,  4 },		// YON
	{ TYPE_FLOAT, NULL, TRUE,  5 },		// NEAR ENV RANGE
	{ TYPE_FLOAT, NULL, TRUE,  6 },		// FAR ENV RANGE
	{ TYPE_BOOL,  NULL, FALSE, 7 },		// MULTI PASS EFFECT ENABLE
	{ TYPE_BOOL,  NULL, FALSE, 8 }, 	// MULTI PASS EFFECT - RENDER EFFECTS PER PASS
	{ TYPE_INT,   NULL, FALSE, 9 } };	// FOV TYPE

static ParamBlockDescID dofV0[] = {
	{ TYPE_BOOL,  NULL, TRUE,  1 },		// ENABLE
	{ TYPE_FLOAT, NULL, TRUE, 2 } };	// FSTOP

static ParamBlockDescID dofV1[] = {
	{ TYPE_BOOL,  NULL, TRUE,  1 },		// ENABLE
	{ TYPE_FLOAT, NULL, TRUE, 2 } };	// FSTOP

SimpleCamera::SimpleCamera(int look) : targDist(0.0f), mpIMultiPassCameraEffect(NULL), mStereoCameraCallBack(NULL)
{
	// Disable hold, macro and ref msgs.
	SuspendAll	xDisableNotifs(TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
	hasTarget = look;
	depthOfFieldPB = NULL;

	pblock = NULL;
	ReplaceReference( 0, CreateParameterBlock( descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION ) );

	SetFOV( TimeValue(0), dlgFOV );
	SetFOVType(dlgFOVType);
	SetTDist( TimeValue(0), dlgTDist );
	SetClipDist( TimeValue(0), CAM_NEAR_CLIP, dlgNearher );
	SetClipDist( TimeValue(0), CAM_FAR_CLIP, dlgFar );
	SetEnvRange( TimeValue(0), ENV_NEAR_RANGE, dlgNearRange );
	SetEnvRange( TimeValue(0), ENV_FAR_RANGE, dlgFarRange );
	//ReplaceReference( MP_EFFECT_REF, CreateDefaultMultiPassEffect(this) );

	SetMultiPassEffectEnabled(TimeValue(0), dlgMultiPassEffectEnable);
	//SetMPEffect_REffectPerPass(dlgMPEffect_REffectPerPass);

	enable = 0;
	coneState = dlgShowCone;
	rangeDisplay = dlgRangeDisplay;
	horzLineState = dlgShowHorzLine;
	manualClip = dlgClip;
	isOrtho = dlgIsOrtho;

	BuildMesh();
}

BOOL SimpleCamera::IsCompatibleRenderer()
{
	return FALSE;
}

BOOL SimpleCamera::SetFOVControl(Control *c)
	{
	pblock->SetController(PB_FOV,c);
	return TRUE;
	}
Control * SimpleCamera::GetFOVControl() {
	return 	pblock->GetController(PB_FOV);
	}

void SimpleCamera::SetConeState(int s) {
	coneState = s;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

void SimpleCamera::SetHorzLineState(int s) {
	horzLineState = s;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

//--------------------------------------------

static INode* FindNodeRef(ReferenceTarget *rt);

static INode* GetNodeRef(ReferenceMaker *rm) {
	if (rm->SuperClassID()==BASENODE_CLASS_ID) return (INode *)rm;
	else return rm->IsRefTarget()?FindNodeRef((ReferenceTarget *)rm):NULL;
	}

static INode* FindNodeRef(ReferenceTarget *rt) {
	DependentIterator di(rt);
	ReferenceMaker *rm = NULL;
	INode *nd = NULL;
	while ((rm=di.Next()) != NULL) {	
		nd = GetNodeRef(rm);
		if (nd) return nd;
		}
	return NULL;
	}

/*
static INode* FindSelNodeRef(ReferenceTarget *rt) {
	DependentIterator di(rt);
	ReferenceMaker *rm;
	INode *nd = NULL;
	while (rm=di.Next()) {	
		if (rm->SuperClassID()==BASENODE_CLASS_ID) {
			nd = (INode *)rm;
			if(nd->Selected()) return nd;
			}
		else {
			if (rm->IsRefTarget()) {
				nd = FindSelNodeRef((ReferenceTarget *)rm);
				if (nd)
					return nd;
				}
			}
		}
	return NULL;
	}
*/

//----------------------------------------------------------------

class SetCamTypeRest: public RestoreObj {
	public:
		SimpleCamera *theCam;
		int oldHasTarget;
		SetCamTypeRest(SimpleCamera *lt, int newt) {
			theCam = lt;
			oldHasTarget = lt->hasTarget;
			}
		~SetCamTypeRest() { }
		void Restore(int isUndo);
		void Redo();
		int Size() { return 1; }
		virtual TSTR Description() { return _T("Set Camera Type"); }
	};


void SetCamTypeRest::Restore(int isUndo) {
	theCam->hasTarget = oldHasTarget;
	theCam->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	theCam->NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED); // to redraw modifier stack
	}

void SetCamTypeRest::Redo() {
	theCam->hasTarget = !oldHasTarget;
	theCam->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	theCam->NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED); // to redraw modifier stack
	}



class ParamsRest: public RestoreObj {
	public:
		SimpleCamera *theCam;
		BOOL showit;
		ParamsRest(SimpleCamera *c, BOOL show) {
			theCam = c;
			showit = show;
			}
		void ParamsRest::Restore(int isUndo) {
			theCam->NotifyDependents(FOREVER, PART_OBJ, showit?REFMSG_END_MODIFY_PARAMS:REFMSG_BEGIN_MODIFY_PARAMS);
			}
		void ParamsRest::Redo() {
			theCam->NotifyDependents(FOREVER, PART_OBJ, showit?REFMSG_BEGIN_MODIFY_PARAMS:REFMSG_END_MODIFY_PARAMS);
			}
		int Size() { return 1; }
		virtual TSTR Description() { return _T("Camera Params"); }
	};


/*----------------------------------------------------------------*/

static ISubTargetCtrl* findSubCtrl(Control* ctrl, Control*& subCtrl)
{
	ISubTargetCtrl* assign = NULL;
	ISubTargetCtrl* next;
	Control* child;

	subCtrl = NULL;
	for ( next = GetSubTargetInterface(ctrl); next != NULL; next = GetSubTargetInterface(child)) {
		child = next->GetTMController();
		if (child == NULL)
			break;
		if (next->CanAssignTMController()) {
			assign = next;
			subCtrl = child;
		}
	}

	return assign;
}

static bool replaceSubLookatController(Control* old)
{
	Control* child = NULL;
	ISubTargetCtrl* assign = findSubCtrl(old, child);
	if (assign == NULL)
		return false;
	DbgAssert(assign->CanAssignTMController() && child != NULL);

	Control *tmc = NewDefaultMatrix3Controller();
	tmc->Copy(child); // doesn't copy rotation, only scale and position.
	assign->AssignTMController(tmc);

	return true;
}

static void clearTargetController(INode* node)
{
	Control* old = node->GetTMController();

	if (!replaceSubLookatController(old)) {
		Control *tmc = NewDefaultMatrix3Controller();
		tmc->Copy(old); // doesn't copy rotation, only scale and position.
		node->SetTMController(tmc);
	}
}

static bool replaceSubPRSController(Control* old, INode* targNode)
{
	Control* child = NULL;
	ISubTargetCtrl* assign = findSubCtrl(old, child);
	if (assign == NULL)
		return false;
	DbgAssert(assign->CanAssignTMController() && child != NULL);

	Control *laControl = CreateLookatControl();
	laControl->SetTarget(targNode);
	laControl->Copy(child);
	assign->AssignTMController(laControl);

	return true;
}

static void setTargetController(INode* node, INode* targNode)
{
	Control* old = node->GetTMController();
	if (!replaceSubPRSController(old, targNode)) {
		// assign lookat controller
		Control *laControl= CreateLookatControl();
		laControl->SetTarget(targNode);
		laControl->Copy(old);
		node->SetTMController(laControl);
	}
}

void SimpleCamera::SetType(int tp) {     
	if (hasTarget == tp) 
		return;

	Interface *iface = GetCOREInterface();
	TimeValue t = iface->GetTime();
	INode *nd = FindNodeRef(this);
	if (nd==NULL) 
		return;

	BOOL paramsShowing = FALSE;
	if (hSimpleCamParams && (currentEditCam == this)) { // LAM - 8/13/02 - defect 511609
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_END_MODIFY_PARAMS);
		paramsShowing = TRUE;
		if (theHold.Holding()) 
			theHold.Put(new ParamsRest(this,0));
		}

	if (theHold.Holding())
		theHold.Put(new SetCamTypeRest(this,tp));

	int oldtype = hasTarget;
	Interval v;
	float tdist = GetTDist(t,v);
	hasTarget = tp;
	bool bHandled = RaisePreCameraTargetChanged((tp!=0), tdist, t);

	if (bHandled)
	{	// if switch from target to free, update the target distance by using returned value from "RaisePreCameraTargetChanged" call.
		if (oldtype == TARGETED_CAMERA)
		{
			SetTDist(0,tdist);
		}
	}
	else //already be handled, so skip codes of creating/removing target node.
	{	
	if(oldtype==TARGETED_CAMERA) {
		tdist = targDist;
		// get rid of target, assign a PRS controller for all instances
		DependentIterator di(this);
		ReferenceMaker *rm = NULL;
		// iterate through the instances
		while ((rm=di.Next()) != NULL) {
			nd = GetNodeRef(rm);
			if (nd) {
				INode* tn = nd->GetTarget(); 
				Matrix3 tm = nd->GetNodeTM(0);
				if (tn) iface->DeleteNode(tn);  // JBW, make it safe if no target
				// CA - 03/02/05 - 621929: When a LinkTM controller was assigned, the
				// the code replaced the LinkTM controller with a PRS controller.
				// There were two problems with this. The FileLink information in the
				// LinkTM controller was lost, and the sub-controllers weren't
				// properly copied. clearTargetController does this correctly.
				clearTargetController(nd);  // doesn't copy rotation, only scale and position.
				nd->SetNodeTM(0,tm);		// preserve rotation if not animated at least
				SetTDist(0,tdist);	 //?? which one should this be for
				}
			}

		}
	else  {
		DependentIterator di(this);
		ReferenceMaker *rm = NULL;
		// iterate through the instances
		while ((rm=di.Next()) != NULL) {	
			nd = GetNodeRef(rm);
			if (nd) {
				// create a target, assign lookat controller
				Matrix3 tm = nd->GetNodeTM(t);
				Matrix3 targtm = tm;
				targtm.PreTranslate(Point3(0.0f,0.0f,-tdist));
				Object *targObject = new TargetObject;
				INode *targNode = iface->CreateObjectNode( targObject);
				TSTR targName;
				targName = nd->GetName();
				targName += GetString(IDS_DB_DOT_TARGET);
				targNode->SetName(targName);
				targNode->SetNodeTM(0,targtm);
				// CA - 03/02/05 - 621929: When a LinkTM controller was assigned, the
				// the code replaced the LinkTM controller with a Lookat controller.
				// There were two problems with this. The FileLink information in the
				// LinkTM controller was lost, and the sub-controllers weren't
				// properly copied. setTargetController does this correctly.
				setTargetController(nd, targNode);
				targNode->SetIsTarget(1);   
				}
			}
		}
	}

	if (paramsShowing) {
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_BEGIN_MODIFY_PARAMS);
		if (theHold.Holding()) 
			theHold.Put(new ParamsRest(this,1));
		}

	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED); // to redraw modifier stack
	iface->RedrawViews(iface->GetTime());
	}


//---------------------------------------
void SimpleCamera::SetFOV(TimeValue t, float f) {
	pblock->SetValue( PB_FOV, t, f );
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

float SimpleCamera::GetFOV(TimeValue t,Interval& valid) {	
	float f;
	pblock->GetValue( PB_FOV, t, f, valid );
	if ( f < float(0) ) f = float(0);
	return f;
	}

void SimpleCamera::SetFOVType(int ft) 
{
	if(ft == GetFOVType()||ft < 0||ft > 2)
		return;
	pblock->SetValue( PB_FOV_TYPE, 0, ft );
	if (iFovType)
		iFovType->SetCurFlyOff(ft);
	if (fovSpin&&iObjParams)
		fovSpin->SetValue( RadToDeg(WFOVtoCurFOV(GetFOV(iObjParams->GetTime()))), FALSE );

	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

int SimpleCamera::GetFOVType()
{
	return pblock->GetInt( PB_FOV_TYPE );
}
void SimpleCamera::SetTDist(TimeValue t, float f) 
{
	static const float Epsilon = 1e-4F;
	if (abs(GetTDist(t) - f) < Epsilon)
	{
		return; // the same, no update.
	}

	bool bHandled = RaisePreSetCameraTargetDistance(f, t);
	if (bHandled)
		return;

// DS 8/15/00  begin
	if (hasTarget) {
		INode *nd = FindNodeRef(this);
		if (nd==NULL) return;
		INode* tn = nd->GetTarget(); 
		if (tn==NULL) return;
		Point3 ptarg;
		GetTargetPoint(t, nd, ptarg);
		Matrix3 tm = nd->GetObjectTM(t);
		float dtarg = Length(tm.GetTrans()-ptarg)/Length(tm.GetRow(2));

		Point3 delta(0,0,0);
		delta.z = dtarg-f;
		Matrix3 tmAxis = nd->GetNodeTM(t);
		tn->Move(t, tmAxis, delta);
		}
	else 
// DS 8/15/00  end
		{
		pblock->SetValue( PB_TDIST, t, f );
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
		}
	}

float SimpleCamera::GetTDist(TimeValue t,Interval& valid) {	
	float f;
	if (hasTarget) {
		if (mStereoCameraCallBack) // if part of stereo system, we need to ask the system to give back the correct target distance.
		{
			UpdateTargDistance(t, FindNodeRef(this));
		}
		return targDist;
		}
	else {
		pblock->GetValue( PB_TDIST, t, f, valid );
		if ( f < MIN_TDIST ) f = MIN_TDIST;
		return f;
		}
	}

void SimpleCamera::SetManualClip(int onOff)
{
	manualClip = onOff;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

float SimpleCamera::GetClipDist(TimeValue t, int which, Interval &valid)
{
	float f;
	pblock->GetValue( PB_HITHER+which-1, t, f, valid );
	if ( f < MIN_CLIP ) f = MIN_CLIP;
	if ( f > MAX_CLIP ) f = MAX_CLIP;
	return f;
}

void SimpleCamera::SetClipDist(TimeValue t, int which, float f)
{
	pblock->SetValue( PB_HITHER+which-1, t, f );
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}


void SimpleCamera::SetEnvRange(TimeValue t, int which, float f)
{
	pblock->SetValue( PB_NRANGE+which, t, f );
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

float SimpleCamera::GetEnvRange(TimeValue t, int which, Interval &valid)
{
	float f;
	pblock->GetValue( PB_NRANGE+which, t, f, valid );
	return f;
}

void SimpleCamera::SetEnvDisplay(BOOL b, int notify) {
	rangeDisplay = b;
	if(notify)
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

void SimpleCamera::SetDOFEnable (TimeValue t, BOOL onOff) {
	}

BOOL SimpleCamera::GetDOFEnable(TimeValue t,Interval& valid) {	
	return FALSE;
	}

void SimpleCamera::SetDOFFStop (TimeValue t, float f) {
	}

float SimpleCamera::GetDOFFStop(TimeValue t,Interval& valid) {	
	return 2.0f;
	}

void SimpleCamera::SetMultiPassEffectEnabled(TimeValue t, BOOL enabled)
{
	pblock->SetValue(PB_MP_EFFECT_ENABLE, t, enabled);
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	if (hSimpleCamParams)
		EnableWindow( GetDlgItem(hSimpleCamParams, IDC_PREVIEW_MP_EFFECT), enabled );
}

BOOL SimpleCamera::GetMultiPassEffectEnabled(TimeValue t, Interval& valid)
{
	BOOL enabled;
	pblock->GetValue(PB_MP_EFFECT_ENABLE, t, enabled, valid);
	return enabled;
}

void SimpleCamera::SetMPEffect_REffectPerPass(BOOL enabled)
{
	pblock->SetValue(PB_MP_EFF_REND_EFF_PER_PASS, 0, enabled);
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

BOOL SimpleCamera::GetMPEffect_REffectPerPass()
{
	BOOL enabled;
	pblock->GetValue(PB_MP_EFF_REND_EFF_PER_PASS, 0, enabled, FOREVER);
	return enabled;
}

class MultiPassRestore: public RestoreObj {
	public:
		SimpleCamera *theCam;
		SingleRefMaker oldmp;
		SingleRefMaker newmp;
		MultiPassRestore(SimpleCamera *sc, IMultiPassCameraEffect *pOldEffect, IMultiPassCameraEffect *pNewEffect) {
			theCam = sc;
			oldmp.SetRef(pOldEffect);
			newmp.SetRef(pNewEffect);
			};
		void Restore(int isUndo) {
			theCam->SetIMultiPassCameraEffect(reinterpret_cast<IMultiPassCameraEffect *>(oldmp.GetRef()));
			}
		void Redo() {
			theCam->SetIMultiPassCameraEffect(reinterpret_cast<IMultiPassCameraEffect *>(newmp.GetRef()));
			}
		int Size() { return 1; }
		virtual TSTR Description() { return _T("Multi Pass Effect Change"); }
	};



// DS: 11/14/00 Made this undoable to fix Defect 268181.
/*void SimpleCamera::SetIMultiPassCameraEffect(IMultiPassCameraEffect *pIMultiPassCameraEffect)
{

	IMultiPassCameraEffect *pCurCameraEffect = GetIMultiPassCameraEffect();
	if (pCurCameraEffect == pIMultiPassCameraEffect) return; // LAM - 8/12/03
	if (theHold.Holding()) 
		theHold.Put( new MultiPassRestore(this, pCurCameraEffect, pIMultiPassCameraEffect));
	theHold.Suspend();
	if (iObjParams && pCurCameraEffect && (currentEditCam == this)) 
		pCurCameraEffect->EndEditParams(iObjParams, END_EDIT_REMOVEUI);
	ReplaceReference(MP_EFFECT_REF, pIMultiPassCameraEffect);
	if (iObjParams && pIMultiPassCameraEffect && (currentEditCam == this)) {
		pIMultiPassCameraEffect->BeginEditParams(iObjParams, inCreate ? BEGIN_EDIT_CREATE : 0, NULL);

		HWND hEffectList = GetDlgItem(hSimpleCamParams, IDC_MP_EFFECT);
		SendMessage(hEffectList, CB_RESETCONTENT, 0, (LPARAM)0);
		int numClasses = smCompatibleEffectList.Count();
		int selIndex = -1;
		for (int i=0; i<numClasses; i++)
		{
			int index = SendMessage( hEffectList, CB_ADDSTRING, 0, (LPARAM)smCompatibleEffectList[i]->CD()->ClassName() );
			if ( pIMultiPassCameraEffect && ( pIMultiPassCameraEffect->ClassID() == smCompatibleEffectList[i]->CD()->ClassID() ) )
			{
				selIndex = index;
			}
		}
		SendMessage(hEffectList, CB_SETCURSEL, selIndex, (LPARAM)0);
	}
	theHold.Resume();
}*/

/*IMultiPassCameraEffect *SimpleCamera::GetIMultiPassCameraEffect()
{
	return reinterpret_cast<IMultiPassCameraEffect *>( GetReference(MP_EFFECT_REF) );
}*/

// static methods
/*IMultiPassCameraEffect *SimpleCamera::CreateDefaultMultiPassEffect(CameraObject *pCameraObject)
{
	FindCompatibleMultiPassEffects(pCameraObject);

	IMultiPassCameraEffect *pIMultiPassCameraEffect = NULL;
	int numClasses = smCompatibleEffectList.Count();
	for (int i=0; i<numClasses; i++)
	{
		// MultiPassDOF camera effect is the default
		if ( smCompatibleEffectList[i]->CD()->ClassID() == Class_ID(0xd481815, 0x687d799c) )
		{
			pIMultiPassCameraEffect = reinterpret_cast<IMultiPassCameraEffect *>( smCompatibleEffectList[i]->CD()->Create(0) );
		}
	}
	return pIMultiPassCameraEffect;
}*/

void SimpleCamera::FindCompatibleMultiPassEffects(CameraObject *pCameraObject)
{
	smCompatibleEffectList.ZeroCount();
	SubClassList *subList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(MPASS_CAM_EFFECT_CLASS_ID);
	if (subList)
	{
		IMultiPassCameraEffect *pIMultiPassCameraEffect = NULL;
		int i = subList->GetFirst(ACC_PUBLIC);
		theHold.Suspend(); // LAM: added 11/12/00
		while (i >= 0)
		{			
			ClassEntry *c = &(*subList)[i];

			pIMultiPassCameraEffect = reinterpret_cast<IMultiPassCameraEffect *>( c->CD()->Create(0) );
			if ( pIMultiPassCameraEffect != NULL )
			{
				if ( pIMultiPassCameraEffect->IsCompatible(pCameraObject) )
				{
					smCompatibleEffectList.Append(1, &c);
				}
				pIMultiPassCameraEffect->MaybeAutoDelete();
			}

			i = subList->GetNext(ACC_PUBLIC);
		}
		theHold.Resume(); // LAM: added 11/12/00
	}
}

void SimpleCamera::SetOrtho(BOOL b) {
	isOrtho = b;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}


class SCamCreateCallBack: public CreateMouseCallBack {
	public:
	SimpleCamera *ob;
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(SimpleCamera *obj) { ob = obj; }
	};

int SCamCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	if (ob)
		ob->enable = 1;
	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {

		// since we're now allowing the user to set the color of
		// the color wire-frames, we need to set the camera 
		// color to blue instead of the default random object color.
		if ( point == 0 )
		{
			ULONG handle;
			ob->NotifyDependents( FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE );
			INode * node;
			node = GetCOREInterface()->GetINodeByHandle( handle );
			if ( node ) 
			{
				Point3 color = GetUIColor( COLOR_CAMERA_OBJ );
				node->SetWireColor( RGB( color.x*255.0f, color.y*255.0f, color.z*255.0f ) );
			}
		}
		
		//mat[3] = vpt->GetPointOnCP(m);
		mat.SetTrans( vpt->SnapPoint(m,m,NULL,SNAP_IN_3D) );
		if (point==1 && msg==MOUSE_POINT) 
			return 0;
		}
	else
	if (msg == MOUSE_ABORT)
		return CREATE_ABORT;

	return TRUE;
	}

static SCamCreateCallBack sCamCreateCB;

SimpleCamera::~SimpleCamera() {
	sCamCreateCB.ob = NULL;	
	DeleteAllRefsFromMe();
	pblock = NULL;
	}

CreateMouseCallBack* SimpleCamera::GetCreateMouseCallBack() {
	sCamCreateCB.SetObj(this);
	return(&sCamCreateCB);
	}

static void RemoveScaling(Matrix3 &tm) {
	AffineParts ap;
	decomp_affine(tm, &ap);
	tm.IdentityMatrix();
	tm.SetRotate(ap.q);
	tm.SetTrans(ap.t);
	}

void SimpleCamera::GetMat(TimeValue t, INode* inode, ViewExp& vpt, Matrix3& tm) {
	if ( ! vpt.IsAlive() )
	{
		tm.Zero();
		return;
	}
	
	tm = inode->GetObjectTM(t);
	//tm.NoScale();
	RemoveScaling(tm);
	float scaleFactor = vpt.NonScalingObjectSize() * vpt.GetVPWorldWidth(tm.GetTrans()) / 360.0f;
	if (scaleFactor!=(float)1.0)
		tm.Scale(Point3(scaleFactor,scaleFactor,scaleFactor));
	}

void SimpleCamera::GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel )
	{
	box = mesh.getBoundingBox(tm);
	}

void SimpleCamera::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box ){
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}
	
	Matrix3 m = inode->GetObjectTM(t);
	Point3 pt;
	float scaleFactor = vpt->NonScalingObjectSize()*vpt->GetVPWorldWidth(m.GetTrans())/(float)360.0;
	box = mesh.getBoundingBox();
#if 0
	if (!hasTarget) {
		if (coneState) {
			Point3 q[4];
			GetConePoints(t,q,scaleFactor*FIXED_CONE_DIST);
			box.IncludePoints(q,4);
			}
		}
#endif
	box.Scale(scaleFactor);
	Point3 q[4];

	if (GetTargetPoint(t,inode,pt)){
		float d = Length(m.GetTrans()-pt)/Length(inode->GetObjectTM(t).GetRow(2));
		box += Point3(float(0),float(0),-d);
		if (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED)) {
			if(manualClip) {
				GetConePoints(t,q,GetClipDist(t,CAM_FAR_CLIP));
				box.IncludePoints(q,4);
			}
			GetConePoints(t,q,d);
			box.IncludePoints(q,4);
			}
		}
#if 1
	else {
		if (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED)) {
			float d = GetTDist(t);
			box += Point3(float(0),float(0),-d);
			GetConePoints(t,q,d);
			box.IncludePoints(q,4);
			}
		}
#endif
	if( rangeDisplay) {
		Point3 q[4];
		float rad = max(GetEnvRange(t, ENV_NEAR_RANGE), GetEnvRange(t, ENV_FAR_RANGE));
		GetConePoints(t, q, rad);
		box.IncludePoints(q,4);
		}
	}

void SimpleCamera::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
	{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}
	
	int i,nv;
	Matrix3 tm;
	float dtarg = 0.0f;
	Point3 pt;
	GetMat(t,inode,*vpt,tm);
	nv = mesh.getNumVerts();
	box.Init();
	if(!(extDispFlags & EXT_DISP_ZOOM_EXT))
		for (i=0; i<nv; i++) 
			box += tm*mesh.getVert(i);
	else
		box += tm.GetTrans();

	tm = inode->GetObjectTM(t);
	if (hasTarget) {
		if (GetTargetPoint(t,inode,pt)) {
			dtarg = Length(tm.GetTrans()-pt)/Length(tm.GetRow(2));
			box += tm*Point3(float(0),float(0),-dtarg);
			}
		}
#if 0
	else dtarg = FIXED_CONE_DIST;
#else
	else dtarg = GetTDist(t);
#endif
	if (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED)) {
		Point3 q[4];
		if(manualClip) {
			GetConePoints(t,q,GetClipDist(t,CAM_FAR_CLIP));
			box.IncludePoints(q,4,&tm);
			}
		GetConePoints(t,q,dtarg);
		box.IncludePoints(q,4,&tm);
		}
	if( rangeDisplay) {
		Point3 q[4];
		float rad = max(GetEnvRange(t, ENV_NEAR_RANGE), GetEnvRange(t, ENV_FAR_RANGE));
		GetConePoints(t, q,rad);
		box.IncludePoints(q,4,&tm);
		}
	}
 
void SimpleCamera::UpdateTargDistance(TimeValue t, INode* inode) {
	if (hasTarget /*&&hSimpleCamParams*/) {						// 5/29/01 2:31pm --MQM-- move this test case down, so we will still compute the target dist
		                                                        //   even for network rendering (when there is no hSimpleCamParams window
		Point3 pt,v[2];
		float distance;
		bool bHandled = RaisePreCameraTargetDistanceUpdated(distance, t);
		if (bHandled)
		{
			targDist = distance;
			if ( hSimpleCamParams && (currentEditCam == this))		// 5/29/01 2:31pm --MQM--, LAM - 8/13/02 - defect 511609
				tdistSpin->SetValue(distance, FALSE);
			return;
		}
		targDist = GetTargetDistance(t);//default to the current value in the PB2.
		if (GetTargetPoint(t,inode,pt)){
			Matrix3 tm = inode->GetObjectTM(t);
			float den = Length(tm.GetRow(2));
			targDist = (den!=0)?Length(tm.GetTrans()-pt)/den : 0.0f;

		// DS 8/15/00   
//			TCHAR buf[40];
//			_stprintf(buf,_T("%0.3f"),targDist);
//			SetWindowText(GetDlgItem(hSimpleCamParams,IDC_TARG_DISTANCE),buf);

			if ( hSimpleCamParams && (currentEditCam == this))		// 5/29/01 2:31pm --MQM--, LAM - 8/13/02 - defect 511609
				tdistSpin->SetValue(targDist, FALSE);
			}
		}
	}

int SimpleCamera::DrawConeAndLine(TimeValue t, INode* inode, GraphicsWindow *gw, int drawing ) {
	Matrix3 tm = inode->GetObjectTM(t);
	gw->setTransform(tm);
	gw->clearHitCode();
	if (hasTarget) {
		Point3 pt,v[3];
		if (GetTargetPoint(t,inode,pt)){
			float den = Length(tm.GetRow(2));
			float dist = (den!=0)?Length(tm.GetTrans()-pt)/den : 0.0f;
			targDist = dist;
			if (hSimpleCamParams&&(currentEditCam==this)) {
				// DS 8/15/00 
//				TCHAR buf[40];
//				_stprintf(buf,_T("%0.3f"),targDist);
//				SetWindowText(GetDlgItem(hSimpleCamParams,IDC_TARG_DISTANCE),buf);
				tdistSpin->SetValue(GetTDist(t), FALSE);
				}
			if ((drawing != -1) && (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED))) {
				if(manualClip) {
					DrawCone(t, gw, GetClipDist(t, CAM_NEAR_CLIP),COLOR_CAMERA_CLIP,0,1);
					DrawCone(t, gw, GetClipDist(t, CAM_FAR_CLIP),COLOR_CAMERA_CLIP,1,1);
					}
				else
					DrawCone(t,gw,dist,COLOR_CAMERA_CONE,TRUE);
			}
			if(!inode->IsFrozen() && !inode->Dependent())
			{
				// 6/25/01 2:33pm --MQM--
				// if user has changed the color of the camera,
				// use that color for the target line too
				Color color(inode->GetWireColor());
				if ( color != GetUIColor(COLOR_CAMERA_OBJ) )
					gw->setColor( LINE_COLOR, color );
				else
					gw->setColor( LINE_COLOR, GetUIColor(COLOR_TARGET_LINE)); // old method
			}
			v[0] = Point3(0,0,0);
			if (drawing == -1)
				v[1] = Point3(0.0f, 0.0f, -0.9f * dist);
			else
				v[1] = Point3(0.0f, 0.0f, -dist);
			gw->polyline( 2, v, NULL, NULL, FALSE, NULL );	
			}
		}
	else {
		if (coneState || (extDispFlags & EXT_DISP_ONLY_SELECTED))
			if(manualClip) {
				DrawCone(t, gw, GetClipDist(t, CAM_NEAR_CLIP),COLOR_CAMERA_CLIP,0,1);
				DrawCone(t, gw, GetClipDist(t, CAM_FAR_CLIP),COLOR_CAMERA_CLIP,1,1);
				}
			else
				DrawCone(t,gw,GetTDist(t),COLOR_CAMERA_CONE,TRUE);
		}
	return gw->checkHitCode();
	}

BaseInterface*	SimpleCamera::GetInterface(Interface_ID id)
{
	if (IID_STEREO_COMPATIBLE_CAMERA == id) 
		return static_cast<IStereoCompatibleCamera*>(this);
	else 
		return GenCamera::GetInterface(id); 
}

// From IStereoCompatibleCamera
bool SimpleCamera::RaisePreCameraTargetChanged(bool bTarget, float& targetDistance, TimeValue t)
{
	if (mStereoCameraCallBack)
	{
		return mStereoCameraCallBack->PreCameraTargetChanged(t, bTarget, targetDistance);
	}
	return false;
}

bool SimpleCamera::RaisePreSetCameraTargetDistance(float distance, TimeValue t)
{
	if (mStereoCameraCallBack)
	{
		return mStereoCameraCallBack->PreSetCameraTargetDistance(t, distance);
	}
	return false;
}

bool SimpleCamera::RaisePreCameraTargetDistanceUpdated(float& distance, TimeValue t)
{
	if (mStereoCameraCallBack)
	{
		return mStereoCameraCallBack->PreCameraTargetDistanceUpdated(t, distance);
	}
	return false;
}

void SimpleCamera::RegisterIStereoCompatibleCameraCallback(IStereoCameraCallback* callback)
{
	if(NULL != callback)
	{
		DbgAssert(mStereoCameraCallBack == NULL);
		mStereoCameraCallBack = callback;
	}
}
void SimpleCamera::UnRegisterIStereoCompatibleCameraCallback(IStereoCameraCallback* callback)
{
	if(NULL != callback && mStereoCameraCallBack == callback)
	{
		mStereoCameraCallBack = NULL;
	}
}

bool SimpleCamera::HasTargetNode(TimeValue t) const
{
	return (hasTarget != 0);
}

float SimpleCamera::GetTargetDistance(TimeValue t) const
{
	if (!pblock)
	{
		return MIN_TDIST;
	}
	float f;
	Interval valid;
	pblock->GetValue( PB_TDIST, t, f, valid );
	if ( f < MIN_TDIST ) f = MIN_TDIST;
	return f;
}

// From BaseObject
int SimpleCamera::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
		
	HitRegion hitRegion;
	DWORD	savedLimits;
	int res = 0;
	Matrix3 m;
	if (!enable) return  0;
	GraphicsWindow *gw = vpt->getGW();	
	MakeHitRegion(hitRegion,type,crossing,4,p);	
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
	GetMat(t,inode,*vpt,m);
	gw->setTransform(m);
	gw->clearHitCode();
	res = mesh.select( gw, gw->getMaterial(), &hitRegion, flags & HIT_ABORTONHIT ); 
	// if not, check the target line, and set the pair flag if it's hit
	if( !res )	{
		// this special case only works with point selection of targeted lights
		if((type != HITTYPE_POINT) || !inode->GetTarget())
			return 0;
		// don't let line be active if only looking at selected stuff and target isn't selected
		if((flags & HIT_SELONLY) && !inode->GetTarget()->Selected() )
			return 0;
		gw->clearHitCode();
		res = DrawConeAndLine(t, inode, gw, -1);
		if(res != 0)
			inode->SetTargetNodePair(1);
	}
	gw->setRndLimits(savedLimits);
	return res;
}

void SimpleCamera::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}
	
	// Make sure the vertex priority is active and at least as important as the best snap so far
	if(snap->vertPriority > 0 && snap->vertPriority <= snap->priority) {
		Matrix3 tm = inode->GetObjectTM(t);	
		GraphicsWindow *gw = vpt->getGW();	
   	
		gw->setTransform(tm);

		Point2 fp = Point2((float)p->x, (float)p->y);
		IPoint3 screen3;
		Point2 screen2;
		Point3 vert(0.0f,0.0f,0.0f);

		gw->wTransPoint(&vert,&screen3);

		screen2.x = (float)screen3.x;
		screen2.y = (float)screen3.y;

		// Are we within the snap radius?
		int len = (int)Length(screen2 - fp);
		if(len <= snap->strength) {
			// Is this priority better than the best so far?
			if(snap->vertPriority < snap->priority) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = vert * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
				}
			else // Closer than the best of this priority?
			if(len < snap->bestDist) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = vert * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
				}
			}
		}
	}


int SimpleCamera::DrawRange(TimeValue t, INode *inode, GraphicsWindow *gw)
{
	if(!rangeDisplay)
		return 0;
	gw->clearHitCode();
	Matrix3 tm = inode->GetObjectTM(t);
	gw->setTransform(tm);
	int cnear = 0;
	int cfar = 0;
	if(!inode->IsFrozen() && !inode->Dependent()) { 
		cnear = COLOR_NEAR_RANGE;
		cfar = COLOR_FAR_RANGE;
		}
	DrawCone(t, gw, GetEnvRange(t, ENV_NEAR_RANGE),cnear);
	DrawCone(t, gw, GetEnvRange(t, ENV_FAR_RANGE), cfar, TRUE);
	return gw->checkHitCode();
	}

#define MAXVP_DIST 1.0e8f

void SimpleCamera::GetConePoints(TimeValue t, Point3* q, float dist) {
	if (dist>MAXVP_DIST)
		dist = MAXVP_DIST;
	float ta = (float)tan(0.5*(double)GetFOV(t));
	float w = dist * ta;
//	float h = w * (float).75; //  ASPECT ??
	float h = w / GetAspect();
	q[0] = Point3( w, h,-dist);				
	q[1] = Point3(-w, h,-dist);				
	q[2] = Point3(-w,-h,-dist);				
	q[3] = Point3( w,-h,-dist);				
	}

void SimpleCamera::DrawCone(TimeValue t, GraphicsWindow *gw, float dist, int colid, BOOL drawSides, BOOL drawDiags) {
	Point3 q[5], u[3];
	GetConePoints(t,q,dist);
	if (colid)	gw->setColor( LINE_COLOR, GetUIColor(colid));
	if (drawDiags) {
		u[0] =  q[0];	u[1] =  q[2];	
		gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
		u[0] =  q[1];	u[1] =  q[3];	
		gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
		}
	gw->polyline( 4, q, NULL, NULL, TRUE, NULL );	
	if (drawSides) {
		gw->setColor( LINE_COLOR, GetUIColor(COLOR_CAMERA_CONE));
		u[0] = Point3(0,0,0);
		for (int i=0; i<4; i++) {
			u[1] =  q[i];	
			gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
			}
		}
	}

void SimpleCamera::SetExtendedDisplay(int flags)
{
	extDispFlags = flags;
}

static MaxSDK::Graphics::Utilities::MeshEdgeKey CameraMeshKey;
static MaxSDK::Graphics::Utilities::SplineItemKey CameraSplineKey;

unsigned long SimpleCamera::GetObjectDisplayRequirement() const
{
	return 1;
}

bool SimpleCamera::PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& displayContext)
{
	CameraMeshKey.SetFixedSize(true);
	return true;
}

bool SimpleCamera::UpdatePerNodeItems(
	const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
	MaxSDK::Graphics::UpdateNodeContext& nodeContext,
	MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer)
{
	INode* pNode = nodeContext.GetRenderNode().GetMaxNode();
	MaxSDK::Graphics::Utilities::MeshEdgeRenderItem* pMeshItem = new CameraMeshItem(&mesh);
	if (pNode->Dependent())
	{
		Color dependentColor = ColorMan()->GetColorAsPoint3(kViewportShowDependencies);
		pMeshItem->SetColor(dependentColor);
	}
	else if (pNode->Selected()) 
	{
		pMeshItem->SetColor(Color(GetSelColor()));
	}
	else if (pNode->IsFrozen())
	{
		pMeshItem->SetColor(Color(GetFreezeColor()));
	}
	else
	{
		Color color(pNode->GetWireColor());
		pMeshItem->SetColor(color);
	}
	MaxSDK::Graphics::CustomRenderItemHandle tempHandle;
	tempHandle.Initialize();
	tempHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	tempHandle.SetCustomImplementation(pMeshItem);
	ViewExp& vpt = GetCOREInterface()->GetActiveViewExp();
	if (vpt.GetViewCamera() != pNode)
	{
		MaxSDK::Graphics::ConsolidationData data;
		data.Strategy = &MaxSDK::Graphics::Utilities::MeshEdgeConsolidationStrategy::GetInstance();
		data.Key = &CameraMeshKey;
		tempHandle.SetConsolidationData(data);
	}
	targetRenderItemContainer.AddRenderItem(tempHandle);
	/*
	MaxSDK::Graphics::Utilities::SplineRenderItem* pLineItem = new CameraConeItem(this);
	MaxSDK::Graphics::CustomRenderItemHandle coneHandle;
	coneHandle.Initialize();
	coneHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	coneHandle.SetCustomImplementation(pLineItem);
	if (vpt.GetViewCamera() != pNode)
	{
		MaxSDK::Graphics::ConsolidationData data;
		data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
		data.Key = &CameraSplineKey;
		coneHandle.SetConsolidationData(data);
	}
	targetRenderItemContainer.AddRenderItem(coneHandle);
	*/

	if (hasTarget)
	{
		MaxSDK::Graphics::Utilities::SplineRenderItem* pLineItem = new CameraTargetLineItem();
		MaxSDK::Graphics::CustomRenderItemHandle lineHandle;
		lineHandle.Initialize();
		lineHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
		lineHandle.SetCustomImplementation(pLineItem);
		if (vpt.GetViewCamera() != pNode)
		{
			MaxSDK::Graphics::ConsolidationData data;
			data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
			data.Key = &CameraSplineKey;
			lineHandle.SetConsolidationData(data);
		}
		targetRenderItemContainer.AddRenderItem(lineHandle);
	}
	return true;
}

int SimpleCamera::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	if (MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		// 11/15/2010 
		// In Nitrous view port, do not draw camera itself when seeing from the camera
		if (NULL != vpt && vpt->GetViewCamera() == inode)
		{
			return 0;
		}
	}
	Matrix3 m;
	GraphicsWindow *gw = vpt->getGW();
	if (!enable) return  0;

	GetMat(t,inode,*vpt,m);
	gw->setTransform(m);
	DWORD rlim = gw->getRndLimits();
	gw->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY|GW_BACKCULL| (rlim&GW_Z_BUFFER) );
	if (inode->Selected())
		gw->setColor( LINE_COLOR, GetSelColor());
	else if(!inode->IsFrozen() && !inode->Dependent())
	{
		// 6/25/01 2:27pm --MQM--
		// use wire-color to draw the camera
		Color color(inode->GetWireColor());
		gw->setColor( LINE_COLOR, color );	
	}
	if (!MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		mesh.render( gw, gw->getMaterial(), NULL, COMP_ALL);	
	}
	DrawConeAndLine(t,inode,gw,1);
#if 0
	if(horzLineState) {
		Point3 eye, tgt;
		eye = inode->GetObjTMAfterWSM(t).GetTrans();
		if(inode->GetTarget())
			tgt = inode->GetTarget()->GetObjTMAfterWSM(t).GetTrans();
		else {
			m = inode->GetObjTMAfterWSM(t);
			m.PreTranslate(Point3(0.0f, 0.0f, -GetTDist(t)));
			tgt = m.GetTrans();
		}
		tgt[1] = eye[1];
        Point3 pt[10];
        float camDist;
		float ta = (float)tan(0.5*(double)GetFOV(t));

        camDist = (float)sqrt((tgt[0]-eye[0]) * (tgt[0]-eye[0]) + (tgt[2]-eye[2]) * (tgt[2]-eye[2]));
               
        pt[0][0] = -camDist * ta;
        pt[0][1] = 0.0f;
        pt[0][2] = -camDist;
        pt[1][0] = camDist * ta;
        pt[1][1] = 0.0f;
        pt[1][2] = -camDist;
		gw->polyline(2, pt, NULL, NULL, 0, NULL);

	}
#endif
	DrawRange(t, inode, gw);
	gw->setRndLimits(rlim);
	return(0);
	}


RefResult SimpleCamera::EvalCameraState(TimeValue t, Interval& valid, CameraState* cs) {
	cs->isOrtho = IsOrtho();	
	cs->fov = GetFOV(t,valid);
	cs->tdist = GetTDist(t,valid);
	cs->horzLine = horzLineState;
	cs->manualClip = manualClip;
	cs->hither = GetClipDist(t, CAM_NEAR_CLIP, valid);
	cs->yon = GetClipDist(t, CAM_FAR_CLIP, valid);
	cs->nearRange = GetEnvRange(t, ENV_NEAR_RANGE, valid);
	cs->farRange = GetEnvRange(t, ENV_FAR_RANGE, valid);
	return REF_SUCCEED;
	}

//
// Reference Managment:
//

RefResult SimpleCamera::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
	PartID& partID, RefMessage message, BOOL propagate ) 
	{
	switch (message) {		
		case REFMSG_WANT_SHOWPARAMLEVEL: {
			BOOL	*pb = ( BOOL * )partID;
			if ( hTarget == ( RefTargetHandle )depthOfFieldPB )
				*pb = TRUE;
			else
				*pb = FALSE;

			return REF_HALT;
			}

		case REFMSG_GET_PARAM_DIM: {
			GetParamDim *gpd = (GetParamDim*)partID;
			if ( hTarget == ( RefTargetHandle )pblock )
			{
				switch (gpd->index) {
					case PB_FOV:
						gpd->dim = stdAngleDim;
						break;				
					case PB_TDIST:
					case PB_HITHER:
					case PB_YON:
					case PB_NRANGE:
					case PB_FRANGE:
						gpd->dim = stdWorldDim;
						break;	
					case PB_FOV_TYPE:
						gpd->dim = defaultDim;
						break;	
					case PB_MP_EFFECT_ENABLE:
					case PB_MP_EFF_REND_EFF_PER_PASS:
						gpd->dim = defaultDim;
						break;				
					}
				return REF_HALT; 
			}
			return REF_STOP; 
			}

		case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			if ( hTarget == ( RefTargetHandle )pblock )
			{
				switch (gpn->index) {
					case PB_FOV:
						gpn->name = GetString(IDS_RB_FOV);
						break;												
					case PB_TDIST:
						gpn->name = GetString(IDS_DB_TDIST);
						break;												
					case PB_HITHER:
						gpn->name = GetString(IDS_RB_NEARPLANE);
						break;												
					case PB_YON:
						gpn->name = GetString(IDS_RB_FARPLANE);
						break;												
					case PB_NRANGE:
						gpn->name = GetString(IDS_DB_NRANGE);
						break;												
					case PB_FRANGE:
						gpn->name = GetString(IDS_DB_FRANGE);
						break;	
					case PB_FOV_TYPE:
						gpn->name = GetString(IDS_FOV_TYPE);
						break;	
					case PB_MP_EFFECT_ENABLE:
						gpn->name = GetString(IDS_MP_EFFECT_ENABLE);
						break;				
					case PB_MP_EFF_REND_EFF_PER_PASS:
						gpn->name = GetString(IDS_MP_EFF_REND_EFF_PER_PASS);
						break;				
					}
				return REF_HALT; 
			}
			return REF_STOP; 
			}
		}
	return(REF_SUCCEED);
	}


ObjectState SimpleCamera::Eval(TimeValue time){
	// UpdateUI(time);
	return ObjectState(this);
	}

Interval SimpleCamera::ObjectValidity(TimeValue time) {
	Interval ivalid;
	ivalid.SetInfinite();
	if (!waitPostLoad) {
		GetFOV(time,ivalid);
		GetTDist(time,ivalid);
		GetClipDist(time, CAM_NEAR_CLIP, ivalid);
		GetClipDist(time, CAM_FAR_CLIP, ivalid);
		GetEnvRange(time, ENV_NEAR_RANGE, ivalid);
		GetEnvRange(time, ENV_FAR_RANGE, ivalid);
		GetMultiPassEffectEnabled(time, ivalid);
		UpdateUI(time);
		}
	return ivalid;	
	}


//********************************************************
// LOOKAT CAMERA
//********************************************************


//------------------------------------------------------
class LACamClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new SimpleCamera(1); }
	int 			BeginCreate(Interface *i);
	int 			EndCreate(Interface *i);
	const TCHAR *	ClassName() { return GetString(IDS_DB_TARGET_CLASS); }
    SClass_ID		SuperClassID() { return CAMERA_CLASS_ID; }
   	Class_ID		ClassID() { return LUX_CAMERA_LOOKAT_CLASS_ID; }
	const TCHAR* 	Category() { return _T("MaxToLux");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetCameraParams(); }
	};

static LACamClassDesc laCamClassDesc;

extern ClassDesc* GetLookatCamDesc() {return &laCamClassDesc; }

class LACamCreationManager : public MouseCallBack, ReferenceMaker {
	private:
		CreateMouseCallBack *createCB;	
		INode *camNode,*targNode;
		SimpleCamera *camObject;
		TargetObject *targObject;
		int attachedToNode;
		IObjCreate *createInterface;
		ClassDesc *cDesc;
		Matrix3 mat;  // the nodes TM relative to the CP
		IPoint2 pt0;
		int ignoreSelectionChange;
		int lastPutCount;

		void CreateNewObject();	

		virtual void GetClassName(MSTR& s) { s = _M("LACamCreationManager"); }
		int NumRefs() { return 1; }
		RefTargetHandle GetReference(int i) { return (RefTargetHandle)camNode; } 
		void SetReference(int i, RefTargetHandle rtarg) { camNode = (INode *)rtarg; }

		// StdNotifyRefChanged calls this, which can change the partID to new value 
		// If it doesnt depend on the particular message& partID, it should return
		// REF_DONTCARE
	    RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
	    	PartID& partID, RefMessage message, BOOL propagate);

	public:
		void Begin( IObjCreate *ioc, ClassDesc *desc );
		void End();
		
		LACamCreationManager()
			{
			ignoreSelectionChange = FALSE;
			}
		int proc( HWND hwnd, int msg, int point, int flag, IPoint2 m );
	};


#define CID_SIMPLECAMCREATE	CID_USER + 1

class LACamCreateMode : public CommandMode {
		LACamCreationManager proc;
	public:
		void Begin( IObjCreate *ioc, ClassDesc *desc ) { proc.Begin( ioc, desc ); }
		void End() { proc.End(); }

		int Class() { return CREATE_COMMAND; }
		int ID() { return CID_SIMPLECAMCREATE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints = 1000000; return &proc; }
		ChangeForegroundCallback *ChangeFGProc() { return CHANGE_FG_SELECTED; }
		BOOL ChangeFG( CommandMode *oldMode ) { return (oldMode->ChangeFGProc() != CHANGE_FG_SELECTED); }
		void EnterMode() {}
		void ExitMode() {}
		BOOL IsSticky() { return FALSE; }
	};

static LACamCreateMode theLACamCreateMode;

//LACamCreationManager::LACamCreationManager( IObjCreate *ioc, ClassDesc *desc )
void LACamCreationManager::Begin( IObjCreate *ioc, ClassDesc *desc )
	{
	createInterface = ioc;
	cDesc           = desc;
	attachedToNode  = FALSE;
	createCB        = NULL;
	camNode         = NULL;
	targNode        = NULL;
	camObject       = NULL;
	targObject      = NULL;
	CreateNewObject();
	}

//LACamCreationManager::~LACamCreationManager
void LACamCreationManager::End()
	{
	if ( camObject ) {
		camObject->EndEditParams( (IObjParam*)createInterface, 
	                    	          END_EDIT_REMOVEUI, NULL);
		if ( !attachedToNode ) {
			// RB 4-9-96: Normally the hold isn't holding when this 
			// happens, but it can be in certain situations (like a track view paste)
			// Things get confused if it ends up with undo...
			theHold.Suspend(); 
			camObject->DeleteAllRefsFromMe();
			camObject->DeleteAllRefsToMe();
			camObject->DeleteThis();  // JBW 11.1.99, this allows scripted plugin cameras to delete cleanly
			camObject = NULL;
			theHold.Resume();
			// RB 7/28/97: If something has been put on the undo stack since this object was created, we have to flush the undo stack.
			if (theHold.GetGlobalPutCount()!=lastPutCount) {
				GetSystemSetting(SYSSET_CLEAR_UNDO);
				}
			macroRec->Cancel();  // JBW 4/23/99
		} else if ( camNode ) {
			 // Get rid of the reference.
			theHold.Suspend();
			DeleteReference(0);  // sets camNode = NULL
			theHold.Resume();
			}
		}	
	}

RefResult LACamCreationManager::NotifyRefChanged(
	const Interval& changeInt, 
	RefTargetHandle hTarget, 
	PartID& partID,  
	RefMessage message, 
	BOOL propagate) 
	{
	switch (message) {
		case REFMSG_PRENOTIFY_PASTE:
		case REFMSG_TARGET_SELECTIONCHANGE:
		 	if ( ignoreSelectionChange ) {
				break;
				}
		 	if ( camObject && camNode==hTarget ) {
				// this will set camNode== NULL;
				theHold.Suspend();
				DeleteReference(0);
				theHold.Resume();
				goto endEdit;
				}
			// fall through

		case REFMSG_TARGET_DELETED:		
			if ( camObject && camNode==hTarget ) {
				endEdit:
				camObject->EndEditParams( (IObjParam*)createInterface, 0, NULL);
				camObject  = NULL;				
				camNode    = NULL;
				CreateNewObject();	
				attachedToNode = FALSE;
				}
			else if (targNode==hTarget) {
				targNode = NULL;
				targObject = NULL;
				}
			break;		
		}
	return REF_SUCCEED;
	}


void LACamCreationManager::CreateNewObject()
	{
	camObject = (SimpleCamera*)cDesc->Create();
	lastPutCount = theHold.GetGlobalPutCount();

    macroRec->BeginCreate(cDesc);  // JBW 4/23/99
	// Start the edit params process
	if ( camObject ) {
		camObject->BeginEditParams( (IObjParam*)createInterface, BEGIN_EDIT_CREATE, NULL );
		}	
	}

static BOOL needToss;
			
int LACamCreationManager::proc( 
				HWND hwnd,
				int msg,
				int point,
				int flag,
				IPoint2 m )
	{	
	int res = TRUE;	
	TSTR targName;
	ViewExp &vpx = createInterface->GetViewExp(hwnd); 
	if ( ! vpx.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	switch ( msg ) {
		case MOUSE_POINT:
			switch ( point ) {
		case 0: {
					pt0 = m;
					assert( camObject );					
					if ( createInterface->SetActiveViewport(hwnd) ) {
						return FALSE;
						}

					if (createInterface->IsCPEdgeOnInView()) { 
						res = FALSE;
						goto done;
						}

					// if cameras were hidden by category, re-display them
					GetCOREInterface()->SetHideByCategoryFlags(
							GetCOREInterface()->GetHideByCategoryFlags() & ~HIDE_CAMERAS);

					if ( attachedToNode ) {
				   		// send this one on its way
				   		camObject->EndEditParams( (IObjParam*)createInterface, 0, NULL);
						macroRec->EmitScript();  // JBW 4/23/99
						
						// Get rid of the reference.
						if (camNode) {
							theHold.Suspend();
							DeleteReference(0);
							theHold.Resume();
							}

						// new object
						CreateNewObject();   // creates camObject
						}

					needToss = theHold.GetGlobalPutCount()!=lastPutCount;

				   	theHold.Begin();	 // begin hold for undo
					mat.IdentityMatrix();

					// link it up
					INode *l_camNode = createInterface->CreateObjectNode( camObject);
					attachedToNode = TRUE;
					assert( l_camNode );					
					createCB = camObject->GetCreateMouseCallBack();
					createInterface->SelectNode( l_camNode );
					
						// Create target object and node
						targObject = new TargetObject;
						assert(targObject);
						targNode = createInterface->CreateObjectNode( targObject);
						assert(targNode);
						targName = l_camNode->GetName();
						targName += GetString(IDS_DB_DOT_TARGET);
						macroRec->Disable();
						targNode->SetName(targName);
						macroRec->Enable();

						// hook up camera to target using lookat controller.
						createInterface->BindToTarget(l_camNode,targNode);					

					// Reference the new node so we'll get notifications.
					theHold.Suspend();
					ReplaceReference( 0, l_camNode);
					theHold.Resume();

					// Position camera and target at first point then drag.
					mat.IdentityMatrix();
					//mat[3] = vpx.GetPointOnCP(m);
					mat.SetTrans( vpx.SnapPoint(m,m,NULL,SNAP_IN_3D) );
					createInterface->SetNodeTMRelConstPlane(camNode, mat);
						createInterface->SetNodeTMRelConstPlane(targNode, mat);
						camObject->Enable(1);

				   		ignoreSelectionChange = TRUE;
				   		createInterface->SelectNode( targNode,0);
				   		ignoreSelectionChange = FALSE;
						res = TRUE;

					// 6/25/01 2:57pm --MQM-- 
					// set our color to the default camera color
					if ( camNode ) 
					{
						Point3 color = GetUIColor( COLOR_CAMERA_OBJ );
						camNode->SetWireColor( RGB( color.x*255.0f, color.y*255.0f, color.z*255.0f ) );
					}
					}
					break;
					
				case 1:
					if (Length(m-pt0)<2)
						goto abort;
					//mat[3] = vpx.GetPointOnCP(m);
					mat.SetTrans( vpx.SnapPoint(m,m,NULL,SNAP_IN_3D) );
					macroRec->Disable();   // JBW 4/23/99
					createInterface->SetNodeTMRelConstPlane(targNode, mat);
					macroRec->Enable();
				   	ignoreSelectionChange = TRUE;
				   	createInterface->SelectNode( camNode);
				   	ignoreSelectionChange = FALSE;
					
					createInterface->RedrawViews(createInterface->GetTime());  

				    theHold.Accept(OperationDesc(IDS_DS_CREATE, GetString(IDS_DS_CREATE), hInstance, camObject));

					res = FALSE;	// We're done
					break;
				}			
			break;

		case MOUSE_MOVE:
			//mat[3] = vpx.GetPointOnCP(m);
			mat.SetTrans( vpx.SnapPoint(m,m,NULL,SNAP_IN_3D) );
			macroRec->Disable();   // JBW 4/23/99
			createInterface->SetNodeTMRelConstPlane(targNode, mat);
			macroRec->Enable();
			createInterface->RedrawViews(createInterface->GetTime());	   

			macroRec->SetProperty(camObject, _T("target"),   // JBW 4/23/99
				mr_create, Class_ID(TARGET_CLASS_ID, 0), GEOMOBJECT_CLASS_ID, 1, _T("transform"), mr_matrix3, &mat);

			res = TRUE;
			break;

		case MOUSE_FREEMOVE:
			SetCursor(UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Crosshair));
			//Snap Preview
			vpx.SnapPreview(m,m,NULL, SNAP_IN_3D);
			break;

	    case MOUSE_PROPCLICK:
			// right click while between creations
			createInterface->RemoveMode(NULL);
			break;

		case MOUSE_ABORT:
			abort:
			assert( camObject );
			camObject->EndEditParams( (IObjParam*)createInterface,0,NULL);
			macroRec->Cancel();  // JBW 4/23/99
			theHold.Cancel();	 // deletes both the camera and target.
			// Toss the undo stack if param changes have been made
			if (needToss) 
				GetSystemSetting(SYSSET_CLEAR_UNDO);
			camNode = NULL;			
			targNode = NULL;	 	
			createInterface->RedrawViews(createInterface->GetTime()); 
			CreateNewObject();	
			attachedToNode = FALSE;
			res = FALSE;						
		}
	
	done:
 
	return res;
	}

int LACamClassDesc::BeginCreate(Interface *i)
	{
	SuspendSetKeyMode();
	IObjCreate *iob = i->GetIObjCreate();
	
	//iob->SetMouseProc( new LACamCreationManager(iob,this), 1000000 );

	theLACamCreateMode.Begin( iob, this );
	iob->PushCommandMode( &theLACamCreateMode );
	
	return TRUE;
	}

int LACamClassDesc::EndCreate(Interface *i)
	{
	
	ResumeSetKeyMode();
	theLACamCreateMode.End();
	i->RemoveMode( &theLACamCreateMode );
	macroRec->EmitScript();  // JBW 4/23/99

	return TRUE;
	}

RefTargetHandle SimpleCamera::Clone(RemapDir& remap) {
	SimpleCamera* newob = new SimpleCamera();
   newob->ReplaceReference(0,remap.CloneRef(pblock));

	if ( GetReference(MP_EFFECT_REF) )
	{
		newob->ReplaceReference( MP_EFFECT_REF, remap.CloneRef( GetReference(MP_EFFECT_REF) ) );
	}

	newob->enable = enable;
	newob->hasTarget = hasTarget;
	newob->coneState = coneState;
	newob->manualClip = manualClip;
	newob->rangeDisplay = rangeDisplay;
	newob->isOrtho = isOrtho;
	BaseClone(this, newob, remap);
	return(newob);
	}

#define CAMERA_FOV_CHUNK 	0x2680
#define CAMERA_TARGET_CHUNK 0x2682
#define CAMERA_CONE_CHUNK 	0x2684
#define CAMERA_MANUAL_CLIP	0x2686
#define CAMERA_HORIZON		0x2688
#define CAMERA_RANGE_DISP	0x268a
#define CAMERA_IS_ORTHO		0x268c

// IO
IOResult SimpleCamera::Save(ISave *isave) {
		
#if 0
	ULONG nb;
	Interval valid;
	float fov;
	pblock->GetValue( 0, 0, fov, valid );
	
	isave->BeginChunk(CAMERA_FOV_CHUNK);
	isave->Write(&fov, sizeof(FLOAT), &nb);
	isave->EndChunk();
#endif

	if (hasTarget) {
		isave->BeginChunk(CAMERA_TARGET_CHUNK);
		isave->EndChunk();
		}
	if (coneState) {
		isave->BeginChunk(CAMERA_CONE_CHUNK);
		isave->EndChunk();
		}
	if (rangeDisplay) {
		isave->BeginChunk(CAMERA_RANGE_DISP);
		isave->EndChunk();
		}
	if (isOrtho) {
		isave->BeginChunk(CAMERA_IS_ORTHO);
		isave->EndChunk();
		}
	if (manualClip) {
		isave->BeginChunk(CAMERA_MANUAL_CLIP);
		isave->EndChunk();
		}
	if (horzLineState) {
		isave->BeginChunk(CAMERA_HORIZON);
		isave->EndChunk();
		}
	return IO_OK;
	}

class CameraPostLoad : public PostLoadCallback {
public:
	SimpleCamera *sc;
	Interval valid;
	CameraPostLoad(SimpleCamera *cam) { sc = cam;}
	void proc(ILoad *iload) {
		if (sc->pblock->GetVersion() != CAMERA_VERSION) {
			switch (sc->pblock->GetVersion()) {
			case 0:
				sc->ReplaceReference(0,
						UpdateParameterBlock(
							descV0, 2, sc->pblock,
							descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
				break;

			case 1:
				sc->ReplaceReference(0,
						UpdateParameterBlock(
							descV1, 4, sc->pblock,
							descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
				break;

			case 2:
				sc->ReplaceReference(0,
						UpdateParameterBlock(
							descV2, 6, sc->pblock,
							descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
				break;

			case 3:
				sc->ReplaceReference(0,
						UpdateParameterBlock(
							descV3, 7, sc->pblock,
							descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
				break;
// MC 
			case 4:
				sc->ReplaceReference(0,
					UpdateParameterBlock(
					descV4, 8, sc->pblock,
					descV5, CAMERA_PBLOCK_COUNT, CAMERA_VERSION));
				iload->SetObsolete();
			break;

			default:
				assert(0);
				break;
			}
		}

		if (( sc->depthOfFieldPB != NULL ) && ( sc->depthOfFieldPB->GetVersion() != DOF_VERSION )) {
			switch (sc->depthOfFieldPB->GetVersion()) {
			case 1:
			{
				sc->ReplaceReference(DOF_REF,
						UpdateParameterBlock(
							descV0, 2, sc->depthOfFieldPB,
							descV1, 2, DOF_VERSION));
				iload->SetObsolete();

				// Replicate the existing data into an mr dof mp effect as much as possible
				IMultiPassCameraEffect *pIMultiPassCameraEffect = NULL;
				int numClasses = SimpleCamera::smCompatibleEffectList.Count();
				for (int i=0; i<numClasses; i++)
				{
					// mental ray DOF mp effect
					if ( SimpleCamera::smCompatibleEffectList[i]->CD()->ClassID() == Class_ID (0x6cc9546e, 0xb1961b9) ) // from dll\mentalray\translator\src\mrDOF.cpp
					{
						BOOL		dofEnable;
						TimeValue   t = 0;
						Interval	valid;

						sc->depthOfFieldPB->GetValue ( PB_DOF_ENABLE, t, dofEnable, valid );
						if ( dofEnable )
						{
							sc->SetMultiPassEffectEnabled ( t, dofEnable );

							pIMultiPassCameraEffect = reinterpret_cast<IMultiPassCameraEffect *>( SimpleCamera::smCompatibleEffectList[i]->CD()->Create(0) );
							sc->SetIMultiPassCameraEffect(pIMultiPassCameraEffect);
							IParamBlock2	*pb = pIMultiPassCameraEffect->GetParamBlock ( 0 );

							if ( pb )
							{
								float	fstop;
								enum
								{
									prm_fstop,
								};

								sc->depthOfFieldPB->GetValue ( PB_DOF_FSTOP, t, fstop, valid );
								pb->SetValue ( prm_fstop, t, fstop );
							}
						}
					}
				}

				break;
			}
			default:
				assert(0);
				break;
			}
		}

		waitPostLoad--;
		delete this;
	}
};

IOResult  SimpleCamera::Load(ILoad *iload) {
	IOResult res;
	enable = TRUE;
	coneState = 0;
	manualClip = 0;
	horzLineState = 0;
	rangeDisplay = 0;
	isOrtho = 0;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
			case CAMERA_TARGET_CHUNK:
				hasTarget = 1;
				break;
			case CAMERA_CONE_CHUNK:
				coneState = 1;
				break;
// The display environment range property is not loaded and always set to false
			case CAMERA_RANGE_DISP:
				rangeDisplay = 1;
				break;
			case CAMERA_IS_ORTHO:
				isOrtho = 1;
				break;
			case CAMERA_MANUAL_CLIP:
				manualClip = 1;
				break;
			case CAMERA_HORIZON:
				horzLineState = 1;
				break;
			}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}
	waitPostLoad++;
	iload->RegisterPostLoadCallback(new CameraPostLoad(this));
	return IO_OK;
	}

