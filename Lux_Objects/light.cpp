//**************************************************************************/
// Copyright (c) 2015-2019 Luxrender.
// All rights reserved.
// 
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com www.3dfine.com
//***************************************************************************/

#include "main.h"
#include "target.h"
#include "hsv.h"
#include "render.h"
#include "shadgen.h"
//#include "genlight.h"
#include "light.h"
#include "notify.h"
#include <bmmlib.h>
#include "macrorec.h"
#include "decomp.h"
#include <marketDefaults.h>
#include <INodeGIProperties.h>
#include <INodeShadingProperties.h>
#include <IViewportShadingMgr.h>
#include <Graphics/Utilities/MeshEdgeRenderItem.h>
#include <Graphics/Utilities/SplineRenderItem.h>
#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/RenderNodeHandle.h>
#include "MouseCursors.h"
#include "3dsmaxport.h"

#define LUX_SPOT_LIGHT_CLASS_ID Class_ID(0x38426803, 0x70f8465c)
#define LUX_DIR_LIGHT_CLASS_ID Class_ID(0x3ec4a74, 0x7fd863ab)
#define LUX_TDIR_LIGHT_CLASS_ID Class_ID(0x302c3bc1, 0x1952205e)
#define LUX_FSPOT_LIGHT_CLASS_ID Class_ID(0x530c2160, 0x59177d0a)

//from 3dswin\src\maxsdk\samples\systems\sunlight\sunclass.h
// The unique 32-bit Class IDs of the slave controller
#define SLAVE_CONTROL_CID1 0x77e3272e
#define SLAVE_CONTROL_CID2 0x13747060

#define DAYLIGHT_SLAVE_CONTROL_CID1 0x35924ed0
#define DAYLIGHT_SLAVE_CONTROL_CID2 0x59cf079c

#define USE_DLG_COLOR

#define WM_SET_TYPE		WM_USER + 0x04002
#define WM_SET_SHADTYPE		WM_USER + 0x04003

#define OMNI_MESH		0
#define DIR_MESH		1

#define NUM_HALF_ARC	5
#define NUM_ARC_PTS	    (2*NUM_HALF_ARC+1)
#define NUM_CIRC_PTS	28
#define SEG_INDEX		7


// Parameter block indices
#define PB_COLOR		0	   
#define PB_INTENSITY	1
#define PB_CONTRAST		2
#define PB_DIFFSOFT		3
#define PB_HOTSIZE		4   
#define PB_FALLSIZE		5  
#define PB_ASPECT		6   
#define PB_ATTENSTART1	7	
#define PB_ATTENEND1 	8	
#define PB_ATTENSTART	9
#define PB_ATTENEND		10
#define PB_DECAY		11
#define PB_SHADCOLOR    12
#define PB_ATMOS_SHAD	 13 // Atmospheric shadows
#define PB_ATMOS_OPACITY 14 // Atmosphere opacity
#define PB_ATMOS_COLAMT	 15 // Atmosphere color influence
#define PB_SHADMULT 	 16
#define PB_SHAD_COLMAP	 17
#define PB_TDIST		 18

// indices for OMNI only
#define PB_OMNIATSTART1      4
#define PB_OMNIATEND1        5
#define PB_OMNIATSTART       6
#define PB_OMNIATEND         7
#define PB_OMNIDECAY   	     8
#define PB_OMNISHADCOLOR     9
#define PB_OMNIATMOS_SHAD	 10 // Atmospheric shadows
#define PB_OMNIATMOS_OPACITY 11 // Atmosphere opacity
#define PB_OMNIATMOS_COLAMT	 12 // Atmosphere color influence
#define PB_OMNISHADMULT 	 13 // Atmosphere color influence
#define PB_OMNISHAD_COLMAP	 14

#define PB_ATM_SHAD(gl)   (gl->type==OMNI_LIGHT? PB_OMNIATMOS_SHAD: PB_ATMOS_SHAD)
#define PB_ATM_OPAC(gl)   (gl->type==OMNI_LIGHT? PB_OMNIATMOS_OPACITY: PB_ATMOS_OPACITY)
#define PB_ATM_COLAMT(gl) (gl->type==OMNI_LIGHT? PB_OMNIATMOS_COLAMT: PB_ATMOS_COLAMT)
#define PB_SHAD_MULT(gl)  (gl->type==OMNI_LIGHT? PB_OMNISHADMULT: PB_SHADMULT)
#define PB_USE_SHAD_COLMAP(gl)   (gl->type==OMNI_LIGHT? PB_OMNISHAD_COLMAP: PB_SHAD_COLMAP)

// Emitter parameter block indices
#define PB_EMITTER_ENABLE		0
#define PB_EMITTER_ENERGY		1
#define PB_EMITTER_DECAY_TYPE	2
#define PB_EMITTER_CA_PHOTONS	3
#define PB_EMITTER_GI_PHOTONS	4

#define DEF_TDIST		240.0f // 160.0f

static int waitPostLoad = 0;

// added a ui consistency check, to check if it's possible to tie the target
// checkbox with the light types in the general params ui dialog
// Properly initialized by CheckUIConsistency in GeneralLight::GeneralLight(int)
static BOOL	uiConsistency = FALSE;
static BOOL simpDialInitialized = FALSE;

#if 0
#define FLto255i(x)	((int)((x)*255.0f + 0.5f))
#define FLto255f(x)	((float)((x)*255.0f + 0.5f))
#else
#define FLto255i(x)	((int)((x)*255.0f))
#define FLto255f(x)	((float)((x)*255.0f))
#endif

inline float MaxF(float a, float b) { return a>b?a:b; }
inline float MinF(float a, float b) { return a<b?a:b; }

inline float MaxAbs(float x, float y, float z) {
	float ax = (float)fabs(x);
	float ay = (float)fabs(y);
	float az = (float)fabs(z);
	return ax>ay? (ax?az:ax):ay>az?ay:az;
	}

#ifdef SIMPLEDIR
static 	BOOL IsSpot(int type)	{ return type == FSPOT_LIGHT || type == TSPOT_LIGHT; }
#else
static 	BOOL IsSpot(int type)	{ return type == FSPOT_LIGHT || type == TSPOT_LIGHT || type == DIR_LIGHT || type==TDIR_LIGHT; }
#endif

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;
void resetLightParams();

//-------------------------------------------------------------------------
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

//static INode* FindSelNodeRef(ReferenceTarget *rt) {
//	DependentIterator di(rt);
//	ReferenceMaker *rm;
//	INode *nd = NULL;
//	while (rm=di.Next()) {	
//		if (rm->SuperClassID()==BASENODE_CLASS_ID) {
//			nd = (INode *)rm;
//			if (nd->Selected()) return nd;
//			}
//		else {
//			if (rm->IsRefTarget()) {
//				nd = FindSelNodeRef((ReferenceTarget *)rm);
//				if (nd)
//					return nd;
//				}
//			}
//		}
//	return NULL;
//	}


static int GetTargetPoint(TimeValue t, INode *inode, Point3& p) 
{
	Matrix3 tmat;
	if (inode->GetTargetTM(t,tmat)) {
		p = tmat.GetTrans();
		return 1;
	}
	else 
		return 0;
}


class LightConeItem : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
	GeneralLight* mpLight;
public:
	LightConeItem(GeneralLight* lt)
		: mpLight(lt)
	{

	}
	~LightConeItem()
	{
		mpLight = nullptr;
	}
	virtual void Realize(MaxSDK::Graphics::DrawContext& drawContext) 
	{
		INode* node = drawContext.GetCurrentNode(); 
		if(nullptr == node)
		{
			return;
		}
		mpLight->SetExtendedDisplay(drawContext.GetExtendedDisplayMode());
		ClearLines();
		BuildConeAndLine(drawContext.GetTime(), drawContext.GetCurrentNode());
		BuildAtten(drawContext.GetTime(), drawContext.GetCurrentNode());
		SplineRenderItem::Realize(drawContext);
	}

	void BuildCone(TimeValue t, float dist)
	{
		if (nullptr == mpLight)
		{
			DbgAssert(true);
			return;
		}
		Point3 posArray[NUM_CIRC_PTS + 1], tmpArray[2];
		int dirLight = mpLight->IsDir();
		int i;
		BOOL dispAtten = (mpLight->useAtten && (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))? TRUE : mpLight->attenDisplay;
		BOOL dispAttenNear = (mpLight->useAttenNear && (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED)) ? TRUE : mpLight->attenNearDisplay;
		BOOL dispDecay = (mpLight->GetDecayType() && (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED)) ? TRUE : mpLight->decayDisplay;

		mpLight->GetConePoints(t, mpLight->GetSpotShape() ? -1.0f : mpLight->GetAspect(t), mpLight->GetHotspot(t), dist, posArray);
		Color color(GetUIColor(COLOR_HOTSPOT));

		if(mpLight->GetSpotShape()) {  
			// CIRCULAR
			if(mpLight->GetHotspot(t) >= mpLight->GetFallsize(t)) {
				// draw (far) hotspot circle
				tmpArray[0] = posArray[0];
				tmpArray[1] = posArray[NUM_CIRC_PTS];
				AddLineStrip(tmpArray, color, 2, false, false);
			}
			AddLineStrip(posArray, color, NUM_CIRC_PTS, true, false);
			if (dirLight) {
				// draw 4 axial hotspot lines
				for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
					tmpArray[0] =  posArray[i]; 
					tmpArray[1] =  posArray[i]; 
					tmpArray[1].z += dist;
					AddLineStrip(tmpArray, color, 2, false, false);
				}
				mpLight->GetConePoints(t, -1.0f, mpLight->GetHotspot(t), 0.0f, posArray);
				AddLineStrip(posArray, color, NUM_CIRC_PTS, true, false);
			}
			else  {
				// draw 4 axial lines
				tmpArray[0] = Point3(0,0,0);
				for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
					tmpArray[1] =  posArray[i];
					AddLineStrip(tmpArray, color, 2, false, false);
				}
			}

			mpLight->GetConePoints(t, -1.0f, mpLight->GetFallsize(t), dist, posArray);
			color = GetUIColor(COLOR_FALLOFF);
			if(mpLight->GetHotspot(t) < mpLight->GetFallsize(t)) {
				// draw (far) fallsize circle
				tmpArray[0] =  posArray[0];
				tmpArray[1] =  posArray[NUM_CIRC_PTS]; 
				AddLineStrip(tmpArray, color, 2, false, false);
				tmpArray[0] = Point3(0,0,0);
			}
			AddLineStrip(posArray, color, NUM_CIRC_PTS, true, false);
			if (dirLight) {
				float dfar = posArray[0].z;
				float dnear = 0.0f;
				if (dispAtten) {
					dfar  = MinF(-mpLight->GetAtten(t,ATTEN_END),dfar);
					dnear = MaxF(-mpLight->GetAtten(t,ATTEN_START),dnear);
				}
				if (dispAttenNear) {
					dfar  = MinF(-mpLight->GetAtten(t,ATTEN1_END),dfar);
					dnear = MaxF(-mpLight->GetAtten(t,ATTEN1_START),dnear);
				}
				if (dispDecay) {
					dfar  = MinF(-mpLight->GetDecayRadius(t),dfar);
				}
				// draw axial fallsize lines
				for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
					tmpArray[0] = posArray[i];	tmpArray[0].z =  dfar;
					tmpArray[1] = posArray[i];	tmpArray[1].z = dnear;
					AddLineStrip(tmpArray, color, 2, false, false);
				}
				mpLight->GetConePoints(t, -1.0f, mpLight->GetFallsize(t), 0.0f, posArray);
				AddLineStrip(posArray, color, NUM_CIRC_PTS, true, false);
			}
			else {
				float cs = (float)cos(DegToRad(mpLight->GetFallsize(t)*0.5f));
				float dfar = posArray[0].z;
				if (dispAtten) 
					dfar  = MinF(-cs * mpLight->GetAtten(t,ATTEN_END),dfar);
				if (dispAttenNear) 
					dfar  = MinF(-cs * mpLight->GetAtten(t,ATTEN1_END),dfar);
				if (dispDecay) 
					dfar  = MinF(-cs * mpLight->GetDecayRadius(t),dfar);
				tmpArray[0] = Point3(0,0,0);
				for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
					//Give a default Point3 instead of a division by zero
					tmpArray[1] = (dist!=0.0f ? -posArray[i]*dfar/dist : Point3(0.0f, 0.0f, 0.0f) );
					AddLineStrip(tmpArray, color,2, false, false);
				}
			}
		}
		else { 
			// RECTANGULAR
			if(mpLight->GetHotspot(t) >= mpLight->GetFallsize(t))
			{
				tmpArray[0] = posArray[4];
				tmpArray[1] = posArray[5];
				AddLineStrip(tmpArray, color, 2, false, false);
			}
			AddLineStrip(posArray, color, 4, true, false);
			if (dirLight) { //DIRLIGHT
				// draw axial hotspot lines
				for (i = 0; i < 4; i += 1) {
					tmpArray[0] =  posArray[i]; 	
					tmpArray[1] =  posArray[i]; 
					tmpArray[1].z += dist;
					AddLineStrip(tmpArray, color, 2, false, false);
				}
				mpLight->GetConePoints(t, mpLight->GetAspect(t), mpLight->GetHotspot(t), 0.0f, posArray);
				// draw (near) hotspot circle
				AddLineStrip(posArray, color, 4, true, false);
			}
			else {
				tmpArray[0] = Point3(0,0,0);
				for (i = 0; i < 4; i++) {
					tmpArray[1] =  posArray[i];
					AddLineStrip(tmpArray, color, 2, false, false);
				}
			}
			mpLight->GetConePoints(t, mpLight->GetAspect(t), mpLight->GetFallsize(t), dist, posArray);
			color = GetUIColor(COLOR_FALLOFF);
			if(mpLight->GetHotspot(t) < mpLight->GetFallsize(t))
			{
				tmpArray[0] = posArray[4];
				tmpArray[1] = posArray[5];
				AddLineStrip(tmpArray, color, 2, false, false);
			}
			AddLineStrip(posArray, color, 4, true, false);
			if (dirLight) {
				// draw axial fallsize lines
				for (i = 0; i < 4; i += 1) {
					tmpArray[0] =  posArray[i]; 	
					tmpArray[1] =  posArray[i]; 
					tmpArray[1].z += dist;
					AddLineStrip(tmpArray, color, 2, false, false);
				}
				mpLight->GetConePoints(t, mpLight->GetAspect(t), mpLight->GetFallsize(t), 0.0f, posArray);
				AddLineStrip(posArray, color, 4, true, false);
			}
			else {
				tmpArray[0] = Point3(0,0,0);
				for (i = 0; i < 4; i += 1) {
					tmpArray[1] = posArray[i];
					AddLineStrip(tmpArray, color, 2, false, false);
				}
			}
		}
	}

	void BuildConeAndLine(TimeValue t, INode* inode)
	{
		if(nullptr == inode
			|| !mpLight->IsSpot())
			return;
		Matrix3 tm = inode->GetObjectTM(t);
		if (mpLight->type == TSPOT_LIGHT) {
			Point3 pt;
			if (GetTargetPoint(t, inode, pt)) {
				float den = FLength(tm.GetRow(2));
				float dist = (den!=0) ? FLength(tm.GetTrans()-pt) / den : 0.0f;
				mpLight->targDist = dist;
				if (mpLight->hSpotLight && (mpLight->currentEditLight == mpLight)) { // LAM - 8/13/02 - defect 511609
					const TCHAR *buf = FormatUniverseValue(mpLight->targDist);
					SetWindowText(GetDlgItem(mpLight->hGeneralLight,IDC_TARG_DISTANCE),buf);
				}
				if (mpLight->coneDisplay || (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))
					BuildCone(t, dist);
			}
		}
		else if (mpLight->type == FSPOT_LIGHT) {
			if (mpLight->coneDisplay || (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))
				BuildCone(t, mpLight->GetTDist(t));
		}
		else if (mpLight->type == DIR_LIGHT) {
			if (mpLight->coneDisplay || (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))
				BuildCone(t, mpLight->GetTDist(t));
		}
		else if (mpLight->type == TDIR_LIGHT) {
			Point3 pt,v[3];
			if (GetTargetPoint(t, inode, pt)) {
				float den = FLength(tm.GetRow(2));
				float dist = (den!=0) ? FLength(tm.GetTrans()-pt) / den : 0.0f;
				mpLight->targDist = dist;
				if (mpLight->hSpotLight && (mpLight->currentEditLight == mpLight)) { // LAM - 8/13/02 - defect 511609
					const TCHAR *buf = FormatUniverseValue(mpLight->targDist);
					SetWindowText(GetDlgItem(mpLight->hGeneralLight,IDC_TARG_DISTANCE),buf);
				}
				if (mpLight->coneDisplay || (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))
					BuildCone(t, dist);
			}
		}
	}

	void BuildX(TimeValue t, float asp, int npts, float dist, int indx, Color& color) {
		Point3 posArray[3 * NUM_CIRC_PTS + 1], tmpArray[2];
		mpLight->GetConePoints(t, asp, mpLight->GetFallsize(t), dist, posArray);
		AddLineStrip(posArray, color, npts, true, true);
		tmpArray[0] = posArray[0]; 
		tmpArray[1] = posArray[2 * indx];
		AddLineStrip(tmpArray, color, 2, false, true);
		tmpArray[0] = posArray[indx]; 
		tmpArray[1] = posArray[3 * indx];
		AddLineStrip(tmpArray, color, 2, false, true);
	}

	void BuildSphereArcs(TimeValue t, float r, Point3* pos, Color& color) {
		mpLight->GetAttenPoints(t, r, pos);
		AddLineStrip(pos, color, NUM_CIRC_PTS, true, true);
		AddLineStrip(&pos[NUM_CIRC_PTS], color, NUM_CIRC_PTS, true, true);
		AddLineStrip(&pos[NUM_CIRC_PTS * 2], color, NUM_CIRC_PTS, true, true);
	}

	// Draw warped rectangle
	void BuildWarpRect(TimeValue t, float angle, float dist, Point3* pos, Color& color) {
		mpLight->GetRectXPoints(t, angle, dist, pos);
		for (int i=0; i<6; i++)
		{
			AddLineStrip(&pos[i*NUM_ARC_PTS], color, NUM_ARC_PTS, false, true);
		}
	}

	void BuildCircleX(TimeValue t, float angle, float dist, Point3* pos, Color& color) {
		mpLight->GetCirXPoints(t, angle,dist, pos);
		AddLineStrip(pos, color, NUM_CIRC_PTS, true, true); // circle 
		AddLineStrip(&pos[NUM_CIRC_PTS], color, NUM_ARC_PTS, false, true); // vert arc
		AddLineStrip(&pos[NUM_CIRC_PTS + NUM_ARC_PTS], color, NUM_ARC_PTS, false, true);  // horiz arc
	}

	void BuildAttenCirOrRect(TimeValue t, float dist, BOOL froze, int uicol) {
		Color color = froze ? GetFreezeColor() : GetUIColor(uicol);
		if (mpLight->IsDir()) {
			int npts,indx;
			float asp;
			if (mpLight->GetSpotShape()) 
			{ 
				npts = NUM_CIRC_PTS; 	
				asp  = -1.0f; 	
				indx = SEG_INDEX; 	
			}
			else 
			{ 	
				npts = 4;  	
				asp  = mpLight->GetAspect(t); 	
				indx = 1; 	
			} 
			BuildX(t, asp, npts, dist, indx, color);
		}
		else {
			Point3 posArray[3 * NUM_CIRC_PTS + 2];
			if (mpLight->type == OMNI_LIGHT
				||(mpLight->IsSpot() && mpLight->overshoot)) 
				BuildSphereArcs(t, dist, posArray, color);
			else {
				if (mpLight->GetSpotShape())  
					BuildCircleX(t, mpLight->GetFallsize(t), dist, posArray, color);
				else 
					BuildWarpRect(t, mpLight->GetFallsize(t), dist, posArray, color);
			}
		}
	}

	int BuildAtten(TimeValue t, INode *inode)
	{
		BOOL dispAtten = (mpLight->useAtten && (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))? TRUE : mpLight->attenDisplay;
		BOOL dispAttenNear = (mpLight->useAttenNear && (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))? TRUE : mpLight->attenNearDisplay;
		BOOL dispDecay = (mpLight->GetDecayType() && (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))? TRUE : mpLight->decayDisplay;

		if (dispAtten||dispAttenNear||dispDecay) {
			BOOL froze = inode->IsFrozen() && !inode->Dependent();
			if (dispAtten) {
				BuildAttenCirOrRect(t, mpLight->GetAtten(t,ATTEN_START), froze, COLOR_START_RANGE);
				BuildAttenCirOrRect(t, mpLight->GetAtten(t,ATTEN_END), froze, COLOR_END_RANGE);
			}
			if (dispAttenNear) {
				BuildAttenCirOrRect(t, mpLight->GetAtten(t,ATTEN1_START), froze, COLOR_START_RANGE1);
				BuildAttenCirOrRect(t, mpLight->GetAtten(t,ATTEN1_END), froze, COLOR_END_RANGE1);
			}
			if (dispDecay) {
				BuildAttenCirOrRect(t, mpLight->GetDecayRadius(t), froze, COLOR_DECAY_RADIUS);
			}
		}
		return 0;
	}
};


class LightTargetLineItem : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
	GeneralLight* mpLight;
	float mLastDist;
	Color mLastColor;
public:
	LightTargetLineItem(GeneralLight* lt)
		: mpLight(lt)
		, mLastDist(-1.0f)
		, mLastColor(0.0f, 0.0f, 0.0f)
	{

	}
	~LightTargetLineItem()
	{
		mpLight = nullptr;
	}
	virtual void Realize(MaxSDK::Graphics::DrawContext& drawContext) 
	{
		INode* inode = drawContext.GetCurrentNode();

		if(nullptr == inode
			|| !mpLight->IsSpot())
			return;

		mpLight->SetExtendedDisplay(drawContext.GetExtendedDisplayMode());
		inode->SetTargetNodePair(0);
		TimeValue t = drawContext.GetTime();
		Matrix3 tm = inode->GetObjectTM(t);
		if (mpLight->type == TSPOT_LIGHT
			|| mpLight->type == TDIR_LIGHT) {
			Point3 pt;
			if (GetTargetPoint(t, inode, pt)) {
				float den = FLength(tm.GetRow(2));
				float dist = (den!=0) ? FLength(tm.GetTrans()-pt) / den : 0.0f;
				mpLight->targDist = dist;
				Color color(inode->GetWireColor());
				if(!inode->IsFrozen() && !inode->Dependent())
				{
					// 6/22/01 2:18pm --MQM-- 
					// if the user has changed the light's wire-frame color,
					// use that color to draw the target line.
					// otherwise, use the standard target-line color.
					if ( color == GetUIColor(COLOR_LIGHT_OBJ) )
						color = GetUIColor(COLOR_TARGET_LINE);
				}
				if (mLastColor != color 
					|| mLastDist != dist)
				{
					ClearLines();
					Point3 posArray[2] = {Point3(0,0,0), Point3(0.0f, 0.0f, -dist)};
					AddLineStrip(posArray, color, 2, false, true);
					mLastColor = color;
					mLastDist = dist;
				}
			}
		}
		SplineRenderItem::Realize(drawContext);
	}
	virtual void OnHit(MaxSDK::Graphics::HitTestContext& /*hittestContext*/, MaxSDK::Graphics::DrawContext& drawContext)
	{
		INode* node = drawContext.GetCurrentNode(); 
		if(nullptr == node)
		{
			node->SetTargetNodePair(1);
		}
	}
};

template <void (GeneralLight::*set)(int), bool notify>
class SimpleLightUndo : public RestoreObj {
public:
	virtual void Restore(int undo);
	virtual void Redo();
	virtual int Size();
	virtual TSTR Description();

	static void Hold(GeneralLight* light, int newVal, int oldVal)
	{
		if (theHold.Holding())
			theHold.Put(new SimpleLightUndo<set, notify>(light, newVal, oldVal));
	}

private:
	SimpleLightUndo(GeneralLight* light, int newVal, int oldVal)
		: _light(light), _redo(newVal), _undo(oldVal) {}

	GeneralLight*		_light;
	int					_redo;
	int					_undo;
};

template <void (GeneralLight::*set)(int), bool notify>
void SimpleLightUndo<set, notify>::Restore(int undo)
{
	(_light->*set)(_undo);
	if (notify)
		_light->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

template <void (GeneralLight::*set)(int), bool notify>
void SimpleLightUndo<set, notify>::Redo()
{
	(_light->*set)(_redo);
	if (notify)
		_light->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

template <void (GeneralLight::*set)(int), bool notify>
int SimpleLightUndo<set, notify>::Size()
{
	return sizeof(*this);
}

template <void (GeneralLight::*set)(int), bool notify>
TSTR SimpleLightUndo<set, notify>::Description()
{
	return _T("SimpleLightUndo");
}

template <void (GeneralLight::*set)(TimeValue, int), bool notify>
class SimpleLightUndoTime : public RestoreObj {
public:
	virtual void Restore(int undo);
	virtual void Redo();
	virtual int Size();
	virtual TSTR Description();

	static void Hold(GeneralLight* light, TimeValue t, int newVal, int oldVal)
	{
		if (theHold.Holding())
			theHold.Put(new SimpleLightUndoTime<set, notify>(light, t, newVal, oldVal));
	}

private:
	SimpleLightUndoTime(GeneralLight* light, TimeValue t, int newVal, int oldVal)
		: _light(light), _t(t), _redo(newVal), _undo(oldVal) {}

	GeneralLight*		_light;
	TimeValue			_t;
	int					_redo;
	int					_undo;
};

template <void (GeneralLight::*set)(TimeValue, int), bool notify>
void SimpleLightUndoTime<set, notify>::Restore(int undo)
{
	(_light->*set)(_t, _undo);
}

template <void (GeneralLight::*set)(TimeValue, int), bool notify>
void SimpleLightUndoTime<set, notify>::Redo()
{
	(_light->*set)(_t, _redo);
}

template <void (GeneralLight::*set)(TimeValue, int), bool notify>
int SimpleLightUndoTime<set, notify>::Size()
{
	return sizeof(*this);
}

template <void (GeneralLight::*set)(TimeValue, int), bool notify>
TSTR SimpleLightUndoTime<set, notify>::Description()
{
	return _T("SimpleLightUndoTime");
}

template <void (GeneralLight::*set)(int, int), int arg, bool notify>
class DualLightUndo : public RestoreObj {
public:
	virtual void Restore(int undo);
	virtual void Redo();
	virtual int Size();
	virtual TSTR Description();

	static void Hold(GeneralLight* light, int newVal, int oldVal)
	{
		if (theHold.Holding())
			theHold.Put(new DualLightUndo<set, arg, notify>(light, newVal, oldVal));
	}

private:
	DualLightUndo(GeneralLight* light, int newVal, int oldVal)
		: _light(light), _redo(newVal), _undo(oldVal) {}

	GeneralLight*		_light;
	int					_redo;
	int					_undo;
};

template <void (GeneralLight::*set)(int, int), int arg, bool notify>
void DualLightUndo<set, arg, notify>::Restore(int undo)
{
	(_light->*set)(_undo, arg);
}

template <void (GeneralLight::*set)(int, int), int arg, bool notify>
void DualLightUndo<set, arg, notify>::Redo()
{
	(_light->*set)(_redo, arg);
}

template <void (GeneralLight::*set)(int, int), int arg, bool notify>
int DualLightUndo<set, arg, notify>::Size()
{
	return sizeof(*this);
}

template <void (GeneralLight::*set)(int, int), int arg, bool notify>
TSTR DualLightUndo<set, arg, notify>::Description()
{
	return _T("DualLightUndo");
}

//-------------------------------------------------------------------------
class OmniLightClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new GeneralLight(OMNI_LIGHT); }
	const TCHAR *	ClassName() { return GetString(IDS_DB_OMNI_CLASS); }
	SClass_ID		SuperClassID() { return LIGHT_CLASS_ID; }
	Class_ID		ClassID() { return LUX_POINT_LIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return _T("MaxToLux");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetLightParams(); }
	DWORD			InitialRollupPageState() { return 0xfffe;}
};

static OmniLightClassDesc omniLightDesc;

ClassDesc *GetOmniLightDesc() { return &omniLightDesc; }

class TSpotLightClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new GeneralLight(TSPOT_LIGHT); }
	int 			BeginCreate(Interface *i);
	int 			EndCreate(Interface *i);
	const TCHAR *	ClassName() { return GetString(IDS_DB_TARGET_SPOT_CLASS); }
	SClass_ID		SuperClassID() { return LIGHT_CLASS_ID; }
	Class_ID		ClassID() { return LUX_SPOT_LIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return _T("MaxToLux");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetLightParams(); }
	// Class IO
	BOOL			NeedsToSave() { return TRUE; }
	IOResult 		Save(ISave *isave);
	IOResult 		Load(ILoad *iload);
	DWORD			InitialRollupPageState() { return 0xfffe;}
	
};

static TSpotLightClassDesc tspotLightDesc;

ClassDesc *GetTSpotLightDesc() { return &tspotLightDesc; }

class DirLightClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new GeneralLight(DIR_LIGHT); }
	const TCHAR *	ClassName() { return GetString(IDS_DB_DIRECTIONAL_CLASS); }
	SClass_ID		SuperClassID() { return LIGHT_CLASS_ID; }
	Class_ID		ClassID() { return LUX_DIR_LIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return _T("MaxToLux");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetLightParams(); }
	DWORD			InitialRollupPageState() { return 0xfffe;}
};

static DirLightClassDesc dirLightDesc;

ClassDesc *GetDirLightDesc() { return &dirLightDesc; }

class TDirLightClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new GeneralLight(TDIR_LIGHT); }
	int 			BeginCreate(Interface *i);
	int 			EndCreate(Interface *i);
	const TCHAR *	ClassName() { return GetString(IDS_DS_TDIRECTIONAL_CLASS); }
	SClass_ID		SuperClassID() { return LIGHT_CLASS_ID; }
	Class_ID		ClassID() { return LUX_TDIR_LIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return _T("MaxToLux");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetLightParams(); }
	DWORD			InitialRollupPageState() { return 0xfffe;}
};

static TDirLightClassDesc tdirLightDesc;

ClassDesc *GetTDirLightDesc() { return &tdirLightDesc; }

class FSpotLightClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new GeneralLight(FSPOT_LIGHT); }
	const TCHAR *	ClassName() { return GetString(IDS_DB_FREE_SPOT_CLASS); }
	SClass_ID		SuperClassID() { return LIGHT_CLASS_ID; }
	Class_ID		ClassID() { return LUX_FSPOT_LIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return _T("MaxToLux");  }
	void			ResetClassParams(BOOL fileReset) { if(fileReset) resetLightParams(); }
	DWORD			InitialRollupPageState() { return 0xfffe;}
};

static FSpotLightClassDesc fspotLightDesc;

ClassDesc *GetFSpotLightDesc() { return &fspotLightDesc; }

#define INITINTENS 255

// Class variables of GeneralLight
BOOL GeneralLight::inCreate=FALSE;
Mesh GeneralLight::staticMesh[2];
short GeneralLight::meshBuilt = 0;
int GeneralLight::dlgRed = INITINTENS;
int GeneralLight::dlgGreen = INITINTENS;
int GeneralLight::dlgBlue = INITINTENS;
int GeneralLight::dlgH = 0;
int GeneralLight::dlgS = 0;
int GeneralLight::dlgV = INITINTENS;
float GeneralLight::dlgIntensity = 1.0f;
float GeneralLight::dlgContrast = 0.0f;
float GeneralLight::dlgDiffsoft = 0.0f;	
float GeneralLight::dlgHotsize = 43.0f;
float GeneralLight::dlgFallsize = 45.0f;
float GeneralLight::dlgTDist = DEF_TDIST;
float GeneralLight::dlgAtten1Start = 0.0f;
float GeneralLight::dlgAtten1End = 40.0f; 
float GeneralLight::dlgAttenStart = 80.0f;
float GeneralLight::dlgAttenEnd = 200.0f; //320.0f;
short GeneralLight::dlgShowCone = 0;
short GeneralLight::dlgUseAtten = 0;
short GeneralLight::dlgShowAtten = 0;
short GeneralLight::dlgShowAttenNear = 0;
short GeneralLight::dlgShowDecay = 0;
short GeneralLight::dlgShape = CIRCLE_LIGHT;
float GeneralLight::dlgAspect = 1.0f;
float GeneralLight::dlgDecayRadius = 40.0f;
short GeneralLight::dlgAtmosShadows = FALSE;
float GeneralLight::dlgAtmosOpacity = 1.0f;
float GeneralLight::dlgAtmosColamt = 1.0f;
short GeneralLight::dlgLightAffectShadColor = 0;
Color24 GeneralLight::dlgShadColor(0,0,0);
float GeneralLight::dlgShadMult = 1.0f;

const Class_ID stdShadMap = Class_ID(STD_SHADOW_MAP_CLASS_ID,0);		// ID for shadow map generator

Class_ID GeneralLight::dlgShadType = GetMarketDefaults()->GetClassID(
	LIGHT_CLASS_ID, Class_ID( OMNI_LIGHT_CLASS_ID, 0),
	_T("ShadowGenerator"), stdShadMap, MarketDefaults::CheckNULL);  
short GeneralLight::dlgUseGlobalShadowParams = GetMarketDefaults()->GetInt(
	LIGHT_CLASS_ID, Class_ID( OMNI_LIGHT_CLASS_ID, 0),
	_T("useGlobalShadowSettings"), 0) != 0;

// Global shadow settings
short GeneralLight::globShadowType = 0;  // 0: ShadowMap   1: RayTrace
short GeneralLight::globAbsMapBias = 0;
float GeneralLight::globMapBias = 1.0f;
float GeneralLight::globMapRange = 4.0f;
int   GeneralLight::globMapSize = 512;
float GeneralLight::globRayBias = .2f;
short GeneralLight::globAtmosShadows = FALSE;
float GeneralLight::globAtmosOpacity = 1.0f;
float GeneralLight::globAtmosColamt = 1.0f;
							 //
GeneralLight * GeneralLight::currentEditLight = NULL;
HWND GeneralLight::hGeneralLight = NULL;
HWND GeneralLight::hICAParam = NULL;
HWND GeneralLight::hAdvEff = NULL;
HWND GeneralLight::hSpotLight = NULL;
// HWND GeneralLight::hOmni = NULL;
HWND GeneralLight::hShadow = NULL;
HWND GeneralLight::hAttenLight = NULL;

void resetLightParams()
{
	GeneralLight::dlgRed = INITINTENS;
	GeneralLight::dlgGreen = INITINTENS;
	GeneralLight::dlgBlue = INITINTENS;
	GeneralLight::dlgH = 0;
	GeneralLight::dlgS = 0;
	GeneralLight::dlgV = INITINTENS;
	GeneralLight::dlgIntensity = 1.0f;
	GeneralLight::dlgContrast = 0.0f;
	GeneralLight::dlgDiffsoft = 0.0f;    // DS 8/11/98
	GeneralLight::dlgHotsize = 43.0f;
	GeneralLight::dlgFallsize = 45.0f;
	GeneralLight::dlgTDist = DEF_TDIST;
	GeneralLight::dlgAtten1Start = 0.0f;
	GeneralLight::dlgAtten1End = 40.0f;
	GeneralLight::dlgAttenStart = 80.0f;
	GeneralLight::dlgAttenEnd = 200.0f;
	GeneralLight::dlgShowCone = 0;
	GeneralLight::dlgUseAtten = 0;
	GeneralLight::dlgShowAtten = 0;
	GeneralLight::dlgShowAttenNear = 0;
	GeneralLight::dlgShowDecay = 0;
	GeneralLight::dlgShape = CIRCLE_LIGHT;
	GeneralLight::dlgAspect = 1.0f;
	GeneralLight::dlgDecayRadius = 40.0f;
	GeneralLight::dlgAtmosShadows = FALSE;
	GeneralLight::dlgAtmosOpacity = 1.0f;
	GeneralLight::dlgAtmosColamt = 1.0f;

	// Global shadow settings
	GeneralLight::globAtmosShadows = FALSE;
	GeneralLight::globAtmosOpacity = 1.0f;
	GeneralLight::globAtmosColamt = 1.0f;

	// Common shadow parameters
	GeneralLight::dlgShadColor.r = 0;
	GeneralLight::dlgShadColor.g = 0;
	GeneralLight::dlgShadColor.b = 0;
	GeneralLight::dlgShadMult = 1.0f;
	GeneralLight::dlgLightAffectShadColor = 0;
	
	GeneralLight::dlgShadType = GetMarketDefaults()->GetClassID(
		LIGHT_CLASS_ID, Class_ID( OMNI_LIGHT_CLASS_ID, 0),
		_T("ShadowGenerator"), stdShadMap, MarketDefaults::CheckNULL);  
	GeneralLight::dlgUseGlobalShadowParams = GetMarketDefaults()->GetInt(
		LIGHT_CLASS_ID, Class_ID( OMNI_LIGHT_CLASS_ID, 0),
		_T("useGlobalShadowSettings"), 0) != 0;

	HWND hl = GetDlgItem(GeneralLight::hGeneralLight, IDC_LIGHT_COLOR);
	if(hl)
		InvalidateRect(hl, NULL, TRUE);
}

IObjParam *GeneralLight::iObjParams;
ISpinnerControl *GeneralLight::redSpin;
ISpinnerControl *GeneralLight::greenSpin;
ISpinnerControl *GeneralLight::blueSpin;
ISpinnerControl *GeneralLight::hSpin;
ISpinnerControl *GeneralLight::sSpin;
ISpinnerControl *GeneralLight::vSpin;
ISpinnerControl *GeneralLight::intensitySpin;
ISpinnerControl *GeneralLight::contrastSpin;
ISpinnerControl *GeneralLight::diffsoftSpin;
ISpinnerControl *GeneralLight::hotsizeSpin;
ISpinnerControl *GeneralLight::fallsizeSpin;
ISpinnerControl *GeneralLight::tDistSpin;
ISpinnerControl *GeneralLight::attenStartSpin;
ISpinnerControl *GeneralLight::attenEndSpin;
ISpinnerControl *GeneralLight::attenStart1Spin;
ISpinnerControl *GeneralLight::attenEnd1Spin;
ISpinnerControl *GeneralLight::aspectSpin;
ISpinnerControl *GeneralLight::decaySpin;
ISpinnerControl *GeneralLight::atmosOpacitySpin;
ISpinnerControl *GeneralLight::atmosColAmtSpin;
ISpinnerControl *GeneralLight::shadMultSpin;
IColorSwatch *GeneralLight::colorSwatch;
IColorSwatch *GeneralLight::shadColorSwatch;
ICustButton *GeneralLight::projMapName;
ICustButton *GeneralLight::shadProjMapName;
ProjMapDADMgr GeneralLight::projDADMgr;
ShadowParamDlg * GeneralLight::shadParamDlg=NULL;


//------------------------------------------------------------------------------
//
// Projection Map Drag and Drop manager;
//
//------------------------------------------------------------------------------
SClass_ID ProjMapDADMgr::GetDragType(HWND hwnd, POINT p) {
	return TEXMAP_CLASS_ID;
	}

BOOL ProjMapDADMgr::OkToDrop(ReferenceTarget *dropThis, HWND hfrom, HWND hto, POINT p, SClass_ID type, BOOL isNew) {
	if (hfrom==hto) return FALSE;
	return (type==TEXMAP_CLASS_ID)?1:0;
	}

ReferenceTarget *ProjMapDADMgr::GetInstance(HWND hwnd, POINT p, SClass_ID type) {
	if  (hwnd==gl->projMapName->GetHwnd())
		return gl->projMap;						
	if  (hwnd==gl->shadProjMapName->GetHwnd())
		return gl->shadProjMap;						
	return NULL;
	}

void ProjMapDADMgr::Drop(ReferenceTarget *dropThis, HWND hwnd, POINT p, SClass_ID type, DADMgr*, BOOL) {
	Texmap *m = (Texmap *)dropThis;
	if  (hwnd==gl->projMapName->GetHwnd())
		gl->AssignProjectorMap(m, FALSE);
	else 
	if  (hwnd==gl->shadProjMapName->GetHwnd())
		gl->AssignShadProjMap(m, FALSE);
	}


// This generic snap routine is used to snap to the 0,0,0 point of the given node.  For lights,
// this works to snap all types.

static void GenericSnap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) 
{
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
			// Closer than the best of this priority?
			else if(len < snap->bestDist) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = vert * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
			}
		}
	}
}

static void InitColorSpinner( HWND hDlg, ISpinnerControl *spin, int editID, float v) 
{
	spin->SetLimits(0.0f, 255.0f, FALSE );
	spin->SetValue(v,FALSE);
	spin->SetScale( 1.0f );
	spin->LinkToEdit( GetDlgItem(hDlg,editID), EDITTYPE_INT );
}

static void InitAngleSpinner( HWND hDlg, ISpinnerControl *spin, int editID, float v)
{
	spin->SetLimits(0.5f, 179.5f, FALSE);
	spin->SetValue(v,FALSE);
	spin->SetScale(.1f);
	spin->LinkToEdit( GetDlgItem(hDlg, editID), EDITTYPE_FLOAT );
}

static void InitRangeSpinner( HWND hDlg, ISpinnerControl *spin, int editID, float v, BOOL allowNegative)
{
//	spin->SetLimits(allowNegative?-999999.0f:0.0f, 999999.0f, FALSE);
	spin->SetLimits(0.0f, 999999.0f, FALSE);
	spin->SetValue(v,FALSE);
	// alexc | 03.06.09 | increments proportional to the spinner value
    spin->SetAutoScale();
	spin->LinkToEdit( GetDlgItem(hDlg, editID), EDITTYPE_UNIVERSE );
}

#define RELEASE_SPIN(x)   if (gl->x) { ReleaseISpinner(gl->x); gl->x = NULL;}
#define RELEASE_COLSW(x)   if (gl->x) { ReleaseIColorSwatch(gl->x); gl->x = NULL;}
#define RELEASE_BUT(x)   if (gl->x) { ReleaseICustButton(gl->x); gl->x = NULL;}


static int AttenFromID(int id) {
	switch (id) {
		case IDC_START_RANGE1_SPIN: return ATTEN1_START;
		case IDC_END_RANGE1_SPIN:   return ATTEN1_END;
		case IDC_START_RANGE_SPIN:  return ATTEN_START;
		case IDC_END_RANGE_SPIN:    return ATTEN_END;
		default: return 0;
		}
	}

ISpinnerControl *GeneralLight::AttenSpinner(int atype) {
	switch (atype) {
		case ATTEN1_START: return attenStart1Spin; break;
		case ATTEN1_END:   return attenEnd1Spin;   break;
		case ATTEN_START:  return attenStartSpin;  break;
		case ATTEN_END:    return attenEndSpin;    break;
		default: return attenStartSpin;
		}
	}

void GeneralLight::ChangeAtten(int atype) {
	float val = AttenSpinner(atype)->GetFVal();
	ISpinnerControl *isp;
	TimeValue t = iObjParams->GetTime();
	SetAtten(t, atype, val);	
	
	for (int i = atype+1; i<4; i++) {
		isp = AttenSpinner(i);
		float f = isp->GetFVal();
		if (val>f) {
			isp->SetValue(val,FALSE);
			SetAtten(t, i, val);	
			}
		}
   for (int i = atype-1; i>=0; i--) {
		isp = AttenSpinner(i);
		float f = isp->GetFVal();
		if (val<f) {
			isp->SetValue(val,FALSE);
			SetAtten(t, i, val);	
			}
		}
	}

void GeneralLight::SetSpinnerBracket(ISpinnerControl *spin, int pbid) {
	spin->SetKeyBrackets(pblock->KeyFrameAtTime(pbid,iObjParams->GetTime()));
	}

static int typeName[NUM_LIGHT_TYPES] = {
	IDS_DB_OMNI,
	IDS_DB_TARGET_SPOT,
	IDS_DB_DIRECTIONAL,
	IDS_DB_FREE_SPOT,
	IDS_DS_TDIRECTIONAL
	};

static int typeFromID[NUM_LIGHT_TYPES] = {
	OMNI_LIGHT,
	TSPOT_LIGHT,
	FSPOT_LIGHT,
	TDIR_LIGHT,
	DIR_LIGHT	
	};

#define NUM_DECAY_TYPES 3

int decayName[NUM_DECAY_TYPES] = { IDS_DECAY_NONE, IDS_DECAY_INVERSE, IDS_DECAY_INVSQ };

static int idFromType(int t) {
	for (int i=0; i<NUM_LIGHT_TYPES; i++)
		if (typeFromID[i]==t) return i;
	return -1;
	}

/**
 * This method returns an index for a simplified light type ui.
 * If light types are added to the light list, or any of the name modified
 * David Cunningham, August 1, 2001
 */

static int idFromTypeSimplified(int t)	{
	switch(t)	{
	case TSPOT_LIGHT:
	case FSPOT_LIGHT:
		return 0;
	case TDIR_LIGHT:
	case DIR_LIGHT:
		return 1;
	case OMNI_LIGHT:
		return 2;
	default:
		return -1;
	}
}

/**
 * Initializes the ui type dialog to the simplified types.
 */

static void InitializeTypeDialogSimplified(HWND	hwndType)	{
	int ret;
	if(!simpDialInitialized)	{
		ret = SendMessage(hwndType, CB_INSERTSTRING , 0, (LPARAM)GetString(IDS_SIMPLIFIED_SPOT));
		ret = SendMessage(hwndType, CB_INSERTSTRING , 1, (LPARAM)GetString(IDS_SIMPLIFIED_DIR));
		ret = SendMessage(hwndType, CB_INSERTSTRING , 2, (LPARAM)GetString(IDS_SIMPLIFIED_OMNI));
		simpDialInitialized = TRUE;
	}
}


static int typeFromIDSimplified(int sel, BOOL targeted)	{
	switch(sel)	{
	case 0:
		return targeted ? TSPOT_LIGHT : FSPOT_LIGHT;
	case 1:
		return targeted ? TDIR_LIGHT : DIR_LIGHT;
	case 2:
		return OMNI_LIGHT;
	default:
		return -1;
	}
}

/**
 * This method is meant to check if the light types are altered (for the UI.)
 * It properly initializes the uiConsistency flag (static variable), which limits
 * the use of idFromTypeSimplified(int t).
 * David Cunningham, August 1, 2001
 */

static void CheckUIConsistency()	{
	// loop checks to see if there is an unknown light type
	for(int i=0;i<NUM_LIGHT_TYPES; i++)	{
		switch(typeFromID[i])	{
		case TSPOT_LIGHT:
		case FSPOT_LIGHT:
		case TDIR_LIGHT:
		case DIR_LIGHT:
		case OMNI_LIGHT:
			break;
		default:
			uiConsistency = FALSE;
			return;
		}
	}
	uiConsistency = TRUE;
}



static ShadowType *GetShadowTypeForIndex(int n) {
	SubClassList *scList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(SHADOW_TYPE_CLASS_ID);
	int count = 0;
	for ( long i = 0; i < scList->Count(ACC_ALL); ++i) {
		if ( (*scList)[ i ].IsPublic() ) {
			ClassDesc* pClassD = (*scList)[ i ].CD();
			if (count==n) {
				HoldSuspend hs; // LAM - 6/12/04 - defect 571821
				return (ShadowType*)( GetCOREInterface()->CreateInstance( pClassD->SuperClassID(), pClassD->ClassID( ) ) );
				}
			count++;
			}
		}
	return NULL;
	}

INT_PTR CALLBACK GeneralLightParamDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{	
	int id;
   GeneralLight *gl = DLGetWindowLongPtr<GeneralLight *>( hDlg);
	if ( !gl && message != WM_INITDIALOG ) 
		return FALSE;
	if (gl->iObjParams==NULL)
		return FALSE;
	TimeValue t = gl->iObjParams->GetTime();
	float tmpHot, tmpFall;
	
	switch ( message ) {
	case WM_INITDIALOG:
		gl = (GeneralLight *)lParam;
	
      DLSetWindowLongPtr( hDlg, gl);
		SetDlgFont( hDlg, gl->iObjParams->GetAppHFont() );
		CheckDlgButton( hDlg, IDC_LIGHT_ON, gl->GetUseLight() );
		CheckDlgButton( hDlg, IDC_GLOBAL_SET, gl->GetUseGlobal() );

		CheckDlgButton( hDlg, IDC_CAST_SHADOWS, gl->GetShadow() );
		CheckDlgButton( hDlg, IDC_OBJECT_SHADOWS, gl->GetShadow() );

		CheckRadioButton( hDlg, IDC_SHADOW_MAPS, IDC_RAY_TRACED_SHADOWS, 
			IDC_SHADOW_MAPS + gl->GetShadowType());
		CheckDlgButton( hDlg, IDC_ABS_MAP_BIAS, gl->GetAbsMapBias() );
		CheckDlgButton( hDlg, IDC_LT_EFFECT_SHADCOL, gl->GetLightAffectsShadow() );
		CheckDlgButton( hDlg, IDC_ATMOS_SHADOWS, gl->GetAtmosShadows(t) );
		CheckDlgButton( hDlg, IDC_USE_SHAD_COLMAP, gl->GetUseShadowColorMap(t) );

		EnableWindow(GetDlgItem(hDlg, IDC_LASPECT), !gl->GetSpotShape());
		EnableWindow(GetDlgItem(hDlg, IDC_LASPECT_SPIN), !gl->GetSpotShape());
		EnableWindow(GetDlgItem(hDlg, IDC_BITMAP_FIT), !gl->GetSpotShape());
		SetDlgItemText(hDlg,IDC_EXCLUDE_DLG, 
			GetString(gl->Include()?IDS_DS_INCLUDE__: IDS_DS_EXCLUDE__));

		CheckDlgButton( hDlg, IDC_SHOW_CONE, gl->GetConeDisplay() );
		CheckRadioButton( hDlg, IDC_RECT_LIGHT, IDC_CIRCLE_LIGHT, IDC_RECT_LIGHT+gl->GetSpotShape());
		CheckDlgButton( hDlg, IDC_OVERSHOOT, gl->GetOvershoot() );
		// Light targeted button
		CheckDlgButton( hDlg, IDC_LIGHT_TARGETED, gl->GetTargeted() );
		// Disable button if the ui is not consistent
		{
		HWND hwndType = GetDlgItem(hDlg, IDC_LIGHT_TYPE);
		BOOL oddControl = gl->IsDaylightOrSunlightSystem();
		if(!uiConsistency)	{
		if (!oddControl) {
			for (int i=0; i<NUM_LIGHT_TYPES; i++)
				SendMessage(hwndType, CB_ADDSTRING, 0, (LPARAM)GetString(typeName[typeFromID[i]]));
			SendMessage( hwndType, CB_SETCURSEL, idFromType(gl->type), (LPARAM)0 );
			}
		}
		else	{	// initialize simplified ui
			if(!gl->inCreate)	{
				InitializeTypeDialogSimplified(hwndType);
				SendMessage(hwndType, CB_SETCURSEL, idFromTypeSimplified(gl->type), (LPARAM)0 );
			}
		}
		EnableWindow(GetDlgItem(hDlg, IDC_LIGHT_TARGETED), !oddControl&&uiConsistency);
		EnableWindow(hwndType,!(gl->inCreate||oddControl));		

		}

//		gl->FixHotFallConstraint();

		if (gl->type==TSPOT_LIGHT || gl->type==TDIR_LIGHT) {
			const TCHAR *buf;
			buf = FormatUniverseValue(gl->targDist);
			SetWindowText(GetDlgItem(hDlg,IDC_TARG_DISTANCE),buf);
			}
		if (gl->hotsizeSpin)
			gl->hotsizeSpin->Enable(!gl->overshoot);

		return FALSE;

	case WM_DESTROY:
		// these spinners no longer exist in the updated UI.
//		RELEASE_SPIN( redSpin);
//		RELEASE_SPIN( greenSpin );
//		RELEASE_SPIN( blueSpin );
//		RELEASE_SPIN( hSpin );
//		RELEASE_SPIN( sSpin );
//		RELEASE_SPIN( vSpin );
		RELEASE_SPIN( intensitySpin );
		RELEASE_SPIN( contrastSpin );
		RELEASE_SPIN( diffsoftSpin );
		RELEASE_COLSW( colorSwatch);
		RELEASE_COLSW( shadColorSwatch);
		RELEASE_SPIN( hotsizeSpin );
		RELEASE_SPIN( fallsizeSpin );
		RELEASE_SPIN( tDistSpin );
		RELEASE_SPIN( aspectSpin );
		RELEASE_SPIN( decaySpin );
		RELEASE_SPIN( attenStartSpin); 
		RELEASE_SPIN( attenEndSpin); 
		RELEASE_SPIN( attenStart1Spin); 
		RELEASE_SPIN( attenEnd1Spin); 
		RELEASE_SPIN( atmosOpacitySpin );
		RELEASE_SPIN( atmosColAmtSpin );
		RELEASE_SPIN( shadMultSpin );
		RELEASE_BUT( projMapName); 
		RELEASE_BUT( shadProjMapName); 
		
		return FALSE;

	case WM_SET_TYPE:
		theHold.Begin();
		gl->SetType(wParam);
		theHold.Accept(GetString(IDS_DS_SETTYPE));
		return FALSE;

	case WM_SET_SHADTYPE:
		theHold.Begin();
		gl->SetShadowGenerator(GetShadowTypeForIndex(wParam));
		theHold.Accept(GetString(IDS_DS_SETSHADTYPE));

		// 5/15/01 11:48am --MQM-- 
		macroRec->SetSelProperty( _T("raytracedShadows"), mr_bool, wParam );
		// 3/29/11 | Zane | viewport now may regard the shadow type 
		gl->iObjParams->RedrawViews(t,REDRAW_INTERACTIVE);
		return FALSE;


	case CC_SPINNER_CHANGE:			
		{
		int id = LOWORD(wParam);
		if (!theHold.Holding()) theHold.Begin();
		BOOL redraw = TRUE;
		switch ( id ) {
		// these spinners no longer exist in the updated UI.
/*		case IDC_LREDSPINNER:
		case IDC_LGREENSPINNER:
		case IDC_LBLUESPINNER:
			gl->SetRGBColor(t, Point3(gl->redSpin->GetFVal()/255.0f, 
					gl->greenSpin->GetFVal()/255.0f, gl->blueSpin->GetFVal()/255.0f ));
			gl->UpdateColBrackets(gl->iObjParams->GetTime());
			break;

		case IDC_LHSPINNER:
		case IDC_LSSPINNER:
		case IDC_LVSPINNER:
			gl->SetUpdateHSVSpin(FALSE);
			gl->SetHSVColor(t, Point3(gl->hSpin->GetFVal()/255.0, 
					gl->sSpin->GetFVal()/255.0, gl->vSpin->GetFVal()/255.0 ));
			gl->UpdateColBrackets(gl->iObjParams->GetTime());
			break;
*/
		case IDC_LMULTSPINNER:
			gl->SetIntensity(t, gl->intensitySpin->GetFVal());
			gl->SetSpinnerBracket(gl->intensitySpin,PB_INTENSITY);
			break;
		case IDC_LCONTRASTSPIN2:
			gl->SetContrast(t, gl->contrastSpin->GetFVal());
			gl->SetSpinnerBracket(gl->contrastSpin,PB_CONTRAST);
			break;
		case IDC_DIFFSOFTSPIN:
			gl->SetDiffuseSoft(t, gl->diffsoftSpin->GetFVal());
			gl->SetSpinnerBracket(gl->diffsoftSpin,PB_DIFFSOFT);
			break;

		case IDC_LHOTSIZESPINNER:
		case IDC_LFALLOFFSPINNER:
			tmpHot = gl->hotsizeSpin->GetFVal();
			tmpFall = gl->fallsizeSpin->GetFVal();

			if((tmpHot > tmpFall - gl->iObjParams->GetLightConeConstraint())) {
				if(LOWORD(wParam) == IDC_LHOTSIZESPINNER)
					tmpFall = tmpHot + gl->iObjParams->GetLightConeConstraint();
				else
					tmpHot = tmpFall - gl->iObjParams->GetLightConeConstraint();
				gl->hotsizeSpin->SetValue(tmpHot, FALSE);
				gl->fallsizeSpin->SetValue(tmpFall, FALSE);
				gl->SetHotspot(t, tmpHot);
				gl->SetFallsize(t, tmpFall);
			}
			else {
				if(LOWORD(wParam) == IDC_LHOTSIZESPINNER)
					gl->SetHotspot(t, tmpHot);
				else
					gl->SetFallsize(t, tmpFall);
			}
			gl->SetSpinnerBracket(gl->hotsizeSpin,PB_HOTSIZE);
			gl->SetSpinnerBracket(gl->fallsizeSpin,PB_FALLSIZE);
			gl->BuildSpotMesh(max(gl->hotsizeSpin->GetFVal(), gl->fallsizeSpin->GetFVal()));
			break;
		case IDC_LTDISTSPINNER:
			gl->SetTDist(t, gl->tDistSpin->GetFVal());
			gl->SetSpinnerBracket(gl->tDistSpin,PB_TDIST);
			break;				   
		case IDC_START_RANGE_SPIN:
			gl->ChangeAtten(ATTEN_START);
			gl->SetSpinnerBracket(gl->attenStartSpin, gl->type==OMNI_LIGHT? PB_OMNIATSTART:PB_ATTENSTART);
			break;
		case IDC_END_RANGE_SPIN:
			gl->ChangeAtten(ATTEN_END);
			gl->SetSpinnerBracket(gl->attenEndSpin, gl->type==OMNI_LIGHT? PB_OMNIATEND:PB_ATTENEND);
			break;
		case IDC_START_RANGE1_SPIN:
			gl->ChangeAtten(ATTEN1_START);
			gl->SetSpinnerBracket(gl->attenStart1Spin, gl->type==OMNI_LIGHT? PB_OMNIATSTART1:PB_ATTENSTART1);
			break;
		case IDC_END_RANGE1_SPIN:
			gl->ChangeAtten(ATTEN1_END);
			gl->SetSpinnerBracket(gl->attenEnd1Spin, gl->type==OMNI_LIGHT? PB_OMNIATEND1:PB_ATTENEND1);
			break;
		case IDC_LASPECT_SPIN:
			gl->SetAspect( t, gl->aspectSpin->GetFVal());
			gl->SetSpinnerBracket(gl->aspectSpin,PB_ASPECT);
			break;
		case IDC_DECAY_SPIN:
			gl->SetDecayRadius( t, gl->decaySpin->GetFVal());
			gl->SetSpinnerBracket(gl->decaySpin, gl->type==OMNI_LIGHT? PB_OMNIDECAY: PB_DECAY);
			break;

		case IDC_ATM_OPACITY_SPIN:
			gl->SetAtmosOpacity( t, gl->atmosOpacitySpin->GetFVal()/100.0f);
			gl->SetSpinnerBracket(gl->atmosOpacitySpin, PB_ATM_OPAC(gl));
			redraw = FALSE;
			break;

		case IDC_ATM_COLAMT_SPIN:
			gl->SetAtmosColAmt( t, gl->atmosColAmtSpin->GetFVal()/100.0f);
			gl->SetSpinnerBracket(gl->atmosColAmtSpin, PB_ATM_COLAMT(gl));
			redraw = FALSE;
			break;
			
		case IDC_SHAD_MULT_SPIN:
			gl->SetShadMult( t, gl->shadMultSpin->GetFVal());
			gl->SetSpinnerBracket(gl->shadMultSpin, PB_SHAD_MULT(gl));
			redraw = FALSE;
			break;
		}				
		if (redraw)
			gl->iObjParams->RedrawViews(gl->iObjParams->GetTime(),REDRAW_INTERACTIVE);
		}
		gl->SetUpdateHSVSpin(TRUE);
		return TRUE;

	case CC_SPINNER_BUTTONDOWN:
		theHold.Begin();
		return TRUE;

	case CC_COLOR_CHANGE:
		id = LOWORD(wParam);
		if(HIWORD(wParam))  {	// button up
			DWORD rgb;
			theHold.Begin();
			switch (id) {
				case IDC_LIGHT_COLOR:
					rgb = gl->colorSwatch->GetColor();
					gl->SetRGBColor(t, Point3(GetRValue(rgb)/255.0f, GetGValue(rgb)/255.0f, GetBValue(rgb)/255.0f));
					break;
				case IDC_SHADOW_COLOR:
					rgb = gl->shadColorSwatch->GetColor();
					gl->SetShadColor(t, Point3(GetRValue(rgb)/255.0f, GetGValue(rgb)/255.0f, GetBValue(rgb)/255.0f));
					break;
				}
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(gl->iObjParams->GetTime(),REDRAW_END);
			gl->UpdateUI(t);
			}
		return TRUE;

	case WM_CUSTEDIT_ENTER:
	case CC_SPINNER_BUTTONUP:
		if (HIWORD(wParam) || message==WM_CUSTEDIT_ENTER) 
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
		else 
			theHold.Cancel();
		gl->iObjParams->RedrawViews(gl->iObjParams->GetTime(),REDRAW_END);
		return TRUE;

	case WM_MOUSEACTIVATE:
		gl->iObjParams->RealizeParamPanel();
		return FALSE;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		gl->iObjParams->RollupMouseMessage(hDlg,message,wParam,lParam);
		return FALSE;

	case WM_COMMAND:
		switch( id = LOWORD(wParam) ) {
		case IDC_LIGHT_ON:
			theHold.Begin();
			gl->SetUseLight( IsDlgButtonChecked( hDlg, IDC_LIGHT_ON ) );
			gl->iObjParams->RedrawViews(t);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;
		case IDC_LIGHT_TARGETED:	{
			int newType = typeFromIDSimplified(SendMessage( GetDlgItem(hDlg,IDC_LIGHT_TYPE), CB_GETCURSEL, 0, 0 ),
														  !gl->GetTargeted());
			PostMessage(hDlg,WM_SET_TYPE,newType,0);
			break;
		}
			
		case IDC_LIGHT_TYPE: {
			int code = HIWORD(wParam);
			if (code==CBN_SELCHANGE) {
				int newType;
				if(!uiConsistency) 
					newType = typeFromID[SendMessage( GetDlgItem(hDlg,IDC_LIGHT_TYPE), CB_GETCURSEL, 0, 0 )];
				
				else	{
					newType = typeFromIDSimplified(SendMessage( GetDlgItem(hDlg,IDC_LIGHT_TYPE), CB_GETCURSEL, 0, 0 ),
									  gl->GetTargeted());
					// disable targeted checkbox if newType is equal to omni
					EnableWindow(GetDlgItem(hDlg, IDC_LIGHT_TARGETED),
						newType != OMNI_LIGHT && !gl->IsDaylightOrSunlightSystem());
				}
				PostMessage(hDlg,WM_SET_TYPE,newType,0);
				}
			break;
			}

		case IDC_SHADOW_TYPE: {
			int code = HIWORD(wParam);
			if (code==CBN_SELCHANGE) {
				int newShadType = SendMessage( GetDlgItem(hDlg,IDC_SHADOW_TYPE), CB_GETCURSEL, 0, 0 );
				PostMessage(hDlg,WM_SET_SHADTYPE,newShadType,0);
				}
			break;
			}
		
		case IDC_SHOW_CONE:
			theHold.Begin();
			gl->SetConeDisplay( IsDlgButtonChecked( hDlg, IDC_SHOW_CONE ) );
			gl->iObjParams->RedrawViews(t);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;

		case IDC_LIGHT_DECAY: {
			int code = HIWORD(wParam);
			if (code==CBN_SELCHANGE) {
				int newType = SendMessage( GetDlgItem(hDlg,IDC_LIGHT_DECAY), CB_GETCURSEL, 0, 0 );
				gl->SetDecayType(newType);
				gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
				gl->iObjParams->RedrawViews(t);
				}
			}
			break;
		case IDC_RECT_LIGHT:
		case IDC_CIRCLE_LIGHT:
			if(wParam > 2048)
				break;		// spurious message sent by dbl-clicking the blue spinner
							// some how ends up here with wParam > 2048 -- so punt it
			theHold.Begin();
			gl->SetSpotShape((id == IDC_RECT_LIGHT)?RECT_LIGHT:CIRCLE_LIGHT);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_SHADOW_MAPS:
		case IDC_RAY_TRACED_SHADOWS:
			CheckRadioButton( hDlg, IDC_SHADOW_MAPS, IDC_RAY_TRACED_SHADOWS, id);
			gl->SetShadowType((id==IDC_SHADOW_MAPS)?0:1);
			break;
		case IDC_GLOBAL_SET:
			theHold.Begin();
			gl->SetUseGlobal( IsDlgButtonChecked( hDlg, id));
			gl->ReloadShadCtrls(hDlg,t);
			if(IsDlgButtonChecked( hDlg, id))	{
				theHold.Begin();
				gl->SetShadowGenerator(GetCOREInterface()->GetGlobalShadowGenerator());
				theHold.Accept(GetString(IDS_DS_SETSHADTYPE));				
			}
			else
			{
				// Also notify dependends when the dialog button is unchecked
				gl->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			}
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;
		case IDC_OBJECT_SHADOWS:
		case IDC_CAST_SHADOWS: {
			theHold.Begin();
			int b = IsDlgButtonChecked(hDlg,id);
			gl->SetShadow(b);
			CheckDlgButton( gl->hGeneralLight, IDC_CAST_SHADOWS, b );
			CheckDlgButton( gl->hShadow, IDC_OBJECT_SHADOWS, b );
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			}
			break;
		case IDC_ABS_MAP_BIAS:
			theHold.Begin();
			gl->SetAbsMapBias( IsDlgButtonChecked( hDlg, id));
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;
		case IDC_ATMOS_SHADOWS:
			theHold.Begin();
			gl->SetAtmosShadows( t, IsDlgButtonChecked( hDlg, id ) );
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;
		case IDC_USE_SHAD_COLMAP:
			theHold.Begin();
			gl->SetUseShadowColorMap( t, IsDlgButtonChecked( hDlg, id ) );
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;
		case IDC_OVERSHOOT:
			theHold.Begin();
			gl->SetOvershoot( IsDlgButtonChecked( hDlg, id));
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_PROJECTOR:
			theHold.Begin();
			gl->SetProjector(IsDlgButtonChecked( hDlg, id));
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;
		case IDC_PROJ_MAPNAME:
			gl->BrowseProjectorMap(hDlg);
//			gl->iObjParams->PutMtlToMtlEditor(gl->projMap);
			break;
		case IDC_SHADPROJ_MAPNAME:
			gl->BrowseShadProjMap(hDlg);
			break;
		case IDC_EXCLUDE_DLG:
			gl->iObjParams->DoExclusionListDialog(&gl->exclList, TRUE /*gl->IsSpot()*/ );
			SetDlgItemText(hDlg,IDC_EXCLUDE_DLG, 
				GetString(gl->Include()?IDS_DS_INCLUDE__: IDS_DS_EXCLUDE__));
			return TRUE;
		case IDC_BITMAP_FIT:
			gl->DoBitmapFit(hDlg,t);
			break;
		case IDC_LT_EFFECT_SHADCOL:
			theHold.Begin();
			gl->SetLightAffectsShadow(IsDlgButtonChecked( hDlg, IDC_LT_EFFECT_SHADCOL ));
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;
		}
		return FALSE;
	default:
		return FALSE;
	}
}

/**
 * This proc handles events sent to the GeneralLight::hICAParam dialog window.
 */

INT_PTR CALLBACK ICAParamDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{	
	int id;
   GeneralLight *gl = DLGetWindowLongPtr<GeneralLight *>( hDlg);
	if ( !gl && message != WM_INITDIALOG ) 
		return FALSE;
	if (gl->iObjParams==NULL)
		return FALSE;
	TimeValue t = gl->iObjParams->GetTime();
//	float tmpHot, tmpFall;
	
	switch ( message ) {
	case WM_INITDIALOG:
		gl = (GeneralLight *)lParam;
	
      DLSetWindowLongPtr( hDlg, gl);
		SetDlgFont( hDlg, gl->iObjParams->GetAppHFont() );
		CheckDlgButton( hDlg, IDC_SHOW_RANGES, gl->GetAttenDisplay() );
		CheckDlgButton( hDlg, IDC_SHOW_RANGES1, gl->GetAttenNearDisplay() );
		CheckDlgButton( hDlg, IDC_USE_ATTEN, gl->GetUseAtten() );
		CheckDlgButton( hDlg, IDC_USE_ATTEN1, gl->GetUseAttenNear() );
		CheckDlgButton( hDlg, IDC_SHOW_DECAY, gl->GetDecayDisplay() );

		{
		HWND hwndType = GetDlgItem(hDlg, IDC_LIGHT_DECAY);
		for (int i=0; i<NUM_DECAY_TYPES; i++)
			SendMessage(hwndType, CB_ADDSTRING, 0, (LPARAM)GetString(decayName[i]));
		SendMessage( hwndType, CB_SETCURSEL, gl->GetDecayType(), (LPARAM)0 );
		}
		return FALSE;

	case WM_DESTROY:
		RELEASE_SPIN( intensitySpin );
		RELEASE_COLSW( colorSwatch);

		RELEASE_SPIN( aspectSpin );

		RELEASE_SPIN( attenStartSpin); 
		RELEASE_SPIN( attenEndSpin); 
		RELEASE_SPIN( attenStart1Spin); 
		RELEASE_SPIN( attenEnd1Spin); 
		
		return FALSE;


	case CC_SPINNER_CHANGE:			
		{
		int id = LOWORD(wParam);
		if (!theHold.Holding()) theHold.Begin();
		BOOL redraw = TRUE;
		switch ( id ) {
		case IDC_LMULTSPINNER:
			gl->SetIntensity(t, gl->intensitySpin->GetFVal());
			gl->SetSpinnerBracket(gl->intensitySpin,PB_INTENSITY);
			break;
		case IDC_START_RANGE_SPIN:
			gl->ChangeAtten(ATTEN_START);
			gl->SetSpinnerBracket(gl->attenStartSpin, gl->type==OMNI_LIGHT? PB_OMNIATSTART:PB_ATTENSTART);
			break;
		case IDC_END_RANGE_SPIN:
			gl->ChangeAtten(ATTEN_END);
			gl->SetSpinnerBracket(gl->attenEndSpin, gl->type==OMNI_LIGHT? PB_OMNIATEND:PB_ATTENEND);
			break;
		case IDC_START_RANGE1_SPIN:
			gl->ChangeAtten(ATTEN1_START);
			gl->SetSpinnerBracket(gl->attenStart1Spin, gl->type==OMNI_LIGHT? PB_OMNIATSTART1:PB_ATTENSTART1);
			break;
		case IDC_END_RANGE1_SPIN:
			gl->ChangeAtten(ATTEN1_END);
			gl->SetSpinnerBracket(gl->attenEnd1Spin, gl->type==OMNI_LIGHT? PB_OMNIATEND1:PB_ATTENEND1);
			break;
		case IDC_LASPECT_SPIN:
			gl->SetAspect( t, gl->aspectSpin->GetFVal());
			gl->SetSpinnerBracket(gl->aspectSpin,PB_ASPECT);
			break;
		case IDC_DECAY_SPIN:
			gl->SetDecayRadius( t, gl->decaySpin->GetFVal());
			gl->SetSpinnerBracket(gl->decaySpin, gl->type==OMNI_LIGHT? PB_OMNIDECAY: PB_DECAY);
			break;
		}				
		if (redraw)
			gl->iObjParams->RedrawViews(gl->iObjParams->GetTime(),REDRAW_INTERACTIVE);
		}
		gl->SetUpdateHSVSpin(TRUE);
		return TRUE;

	case CC_SPINNER_BUTTONDOWN:
		theHold.Begin();
		return TRUE;

	case CC_COLOR_CHANGE:
		id = LOWORD(wParam);
		if(HIWORD(wParam))  {	// button up
			DWORD rgb;
			theHold.Begin();
			switch (id) {
				case IDC_LIGHT_COLOR:
					rgb = gl->colorSwatch->GetColor();
					gl->SetRGBColor(t, Point3(GetRValue(rgb)/255.0f, GetGValue(rgb)/255.0f, GetBValue(rgb)/255.0f));
					break;
				case IDC_SHADOW_COLOR:
					rgb = gl->shadColorSwatch->GetColor();
					gl->SetShadColor(t, Point3(GetRValue(rgb)/255.0f, GetGValue(rgb)/255.0f, GetBValue(rgb)/255.0f));
					break;
				}
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(gl->iObjParams->GetTime(),REDRAW_END);
			gl->UpdateUI(t);
			}
		return TRUE;

	case WM_CUSTEDIT_ENTER:
	case CC_SPINNER_BUTTONUP:
		if (HIWORD(wParam) || message==WM_CUSTEDIT_ENTER) 
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
		else 
			theHold.Cancel();
		gl->iObjParams->RedrawViews(gl->iObjParams->GetTime(),REDRAW_END);
		return TRUE;

	case WM_MOUSEACTIVATE:
		gl->iObjParams->RealizeParamPanel();
		return FALSE;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		gl->iObjParams->RollupMouseMessage(hDlg,message,wParam,lParam);
		return FALSE;

	case WM_COMMAND:
		switch( id = LOWORD(wParam) ) {

//		case IDC_SOFTEN_DIFFUSE:
//			gl->softenDiffuse = IsDlgButtonChecked( hDlg, IDC_SOFTEN_DIFFUSE );
//			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
//			gl->iObjParams->RedrawViews(t);
//			break;
		case IDC_USE_ATTEN1:
			theHold.Begin();
			gl->SetUseAttenNear( IsDlgButtonChecked( hDlg, IDC_USE_ATTEN1 ) );
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_SHOW_RANGES1:
			theHold.Begin();
			gl->SetAttenNearDisplay( IsDlgButtonChecked( hDlg, IDC_SHOW_RANGES1 ) );
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_USE_ATTEN:
			theHold.Begin();
			gl->SetUseAtten( IsDlgButtonChecked( hDlg, IDC_USE_ATTEN ) );
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_SHOW_RANGES:
			theHold.Begin();
			gl->SetAttenDisplay( IsDlgButtonChecked( hDlg, IDC_SHOW_RANGES ) );
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_SHOW_DECAY:
			theHold.Begin();
			gl->SetDecayDisplay( IsDlgButtonChecked( hDlg, IDC_SHOW_DECAY) );
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_LIGHT_DECAY: {
			int code = HIWORD(wParam);
			if (code==CBN_SELCHANGE) {
				int newType = SendMessage( GetDlgItem(hDlg,IDC_LIGHT_DECAY), CB_GETCURSEL, 0, 0 );
				gl->SetDecayType(newType);
				gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
				gl->iObjParams->RedrawViews(t);
				}
			}
			break;
		case IDC_OBJECT_SHADOWS:
		case IDC_CAST_SHADOWS: {
			theHold.Begin();
			int b = IsDlgButtonChecked(hDlg,id);
			gl->SetShadow(b);
			CheckDlgButton( gl->hGeneralLight, IDC_CAST_SHADOWS, b );
			CheckDlgButton( gl->hShadow, IDC_OBJECT_SHADOWS, b );
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			}
			break;
		return FALSE;
		}
	default:
		return FALSE;
	}
}


INT_PTR CALLBACK AdvEffDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
   GeneralLight *gl = DLGetWindowLongPtr<GeneralLight *>( hDlg);
	int id;
	if ( !gl && message != WM_INITDIALOG ) 
		return FALSE;
	if (gl->iObjParams==NULL)
		return FALSE;
	TimeValue t = gl->iObjParams->GetTime();
//	float tmpHot, tmpFall;
	
	switch ( message ) {
	case WM_INITDIALOG:
		gl = (GeneralLight *)lParam;
      DLSetWindowLongPtr( hDlg, gl);
		SetDlgFont( hDlg, gl->iObjParams->GetAppHFont() );
		CheckDlgButton( hDlg, IDC_AFFECT_DIFFUSE, gl->affectDiffuse);
		//CheckDlgButton( hDlg, IDC_SOFTEN_DIFFUSE, gl->softenDiffuse);
		CheckDlgButton( hDlg, IDC_AFFECT_SPECULAR, gl->affectSpecular);
		CheckDlgButton( hDlg, IDC_AMBIENT_ONLY, gl->ambientOnly);
		CheckDlgButton( hDlg, IDC_PROJECTOR, gl->GetProjector() );
		return FALSE;
	case WM_DESTROY:
		
		RELEASE_SPIN( contrastSpin );
		RELEASE_SPIN( diffsoftSpin );

		RELEASE_BUT( projMapName); 
		
		return FALSE;


	case CC_SPINNER_CHANGE:			
		{
		int id = LOWORD(wParam);
		if (!theHold.Holding()) theHold.Begin();
		BOOL redraw = TRUE;
		switch ( id ) {
		case IDC_LCONTRASTSPIN2:
			gl->SetContrast(t, gl->contrastSpin->GetFVal());
			gl->SetSpinnerBracket(gl->contrastSpin,PB_CONTRAST);
			break;
		case IDC_DIFFSOFTSPIN:
			gl->SetDiffuseSoft(t, gl->diffsoftSpin->GetFVal());
			gl->SetSpinnerBracket(gl->diffsoftSpin,PB_DIFFSOFT);
			break;

		case IDC_LASPECT_SPIN:
			gl->SetAspect( t, gl->aspectSpin->GetFVal());
			gl->SetSpinnerBracket(gl->aspectSpin,PB_ASPECT);
			break;
		}				
		if (redraw)
			gl->iObjParams->RedrawViews(gl->iObjParams->GetTime(),REDRAW_INTERACTIVE);
		}
		gl->SetUpdateHSVSpin(TRUE);
		return TRUE;

	case CC_SPINNER_BUTTONDOWN:
		theHold.Begin();
		return TRUE;

	case WM_CUSTEDIT_ENTER:
	case CC_SPINNER_BUTTONUP:
		if (HIWORD(wParam) || message==WM_CUSTEDIT_ENTER) 
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
		else 
			theHold.Cancel();
		gl->iObjParams->RedrawViews(gl->iObjParams->GetTime(),REDRAW_END);
		return TRUE;

	case WM_MOUSEACTIVATE:
		gl->iObjParams->RealizeParamPanel();
		return FALSE;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		gl->iObjParams->RollupMouseMessage(hDlg,message,wParam,lParam);
		return FALSE;

	case WM_COMMAND:
		switch( id = LOWORD(wParam) ) {
//		case IDC_SOFTEN_DIFFUSE:
//			gl->softenDiffuse = IsDlgButtonChecked( hDlg, IDC_SOFTEN_DIFFUSE );
//			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
//			gl->iObjParams->RedrawViews(t);
//			break;
		case IDC_AFFECT_DIFFUSE:
			theHold.Begin();
			gl->SetAffectDiffuse( IsDlgButtonChecked( hDlg, IDC_AFFECT_DIFFUSE ) );
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_AFFECT_SPECULAR:
			theHold.Begin();
			gl->SetAffectSpecular( IsDlgButtonChecked( hDlg, IDC_AFFECT_SPECULAR ) );
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			break;
		case IDC_AMBIENT_ONLY:
			theHold.Begin();
			gl->SetAmbientOnly( IsDlgButtonChecked( hDlg, IDC_AMBIENT_ONLY ) );
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			gl->iObjParams->RedrawViews(t);
			gl->UpdateForAmbOnly();
			break;
		case IDC_PROJECTOR:
			theHold.Begin();
			gl->SetProjector(IsDlgButtonChecked( hDlg, id));
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			break;
		case IDC_PROJ_MAPNAME:
			gl->BrowseProjectorMap(hDlg);
//			gl->iObjParams->PutMtlToMtlEditor(gl->projMap);
			break;
		case IDC_BITMAP_FIT:
			gl->DoBitmapFit(hDlg,t);
			break;
		case IDC_LT_EFFECT_SHADCOL:
			gl->SetLightAffectsShadow(IsDlgButtonChecked( hDlg, IDC_LT_EFFECT_SHADCOL ));
			gl->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
			break;
		return FALSE;
		}
	default:
		return FALSE;
	}
}



void GeneralLight::FixHotFallConstraint() {
	if (iObjParams==NULL)
		return;
	TimeValue t = iObjParams->GetTime();
	if(IsSpot()) {
		if(GetFallsize(t) < GetHotspot(t) + iObjParams->GetLightConeConstraint())
			SetFallsize(t, GetHotspot(t) + iObjParams->GetLightConeConstraint());
		}
	}

void GeneralLight::ReloadShadCtrls(HWND hDlg, TimeValue t) {
//	EnableWindow(GetDlgItem(hDlg, IDC_SHADOW_MAPS), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_RAY_TRACED_SHADOWS), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_MAP_BIAS), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_MAP_BIAS_SPIN), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_MAP_SIZE), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_MAP_SIZE_SPIN), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_MAP_RANGE), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_MAP_RANGE_SPIN), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_RT_BIAS), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_RT_BIAS_SPIN), onOff);
//	EnableWindow(GetDlgItem(hDlg, IDC_ABS_MAP_BIAS), onOff);
	CheckRadioButton( hDlg, IDC_SHADOW_MAPS, IDC_RAY_TRACED_SHADOWS, 
		IDC_SHADOW_MAPS + GetShadowType());
	CheckDlgButton( hDlg, IDC_ABS_MAP_BIAS, GetAbsMapBias() );
//	mapSizeSpin->SetValue( GetMapSize(t),FALSE );
//	mapBiasSpin->SetValue( GetMapBias(t),FALSE );
//	mapRangeSpin->SetValue( GetMapRange(t),FALSE );
//	rayBiasSpin->SetValue( GetRayBias(t),FALSE );
	decaySpin->SetValue( GetDecayRadius(t),FALSE );
	CheckDlgButton( hDlg, IDC_ATMOS_SHADOWS, GetAtmosShadows(t) );
	atmosOpacitySpin->SetValue( GetAtmosOpacity(t)*100.0f,FALSE);
	atmosColAmtSpin->SetValue( GetAtmosColAmt(t)*100.0f,FALSE);
	shadMultSpin->SetValue( GetShadMult(t),FALSE);
	}


void GeneralLight::UpdtShadowTypeSel() {
//	if (hShadow==NULL) return;
//	HWND hwShadType = GetDlgItem(hShadow, IDC_SHADOW_TYPE);
	if (hGeneralLight==NULL || (currentEditLight != this)) return; // LAM - 8/13/02 - defect 511609
	HWND hwShadType = GetDlgItem(hGeneralLight, IDC_SHADOW_TYPE);
	ShadowType *actShad = ActiveShadowType();

	SubClassList *scList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(SHADOW_TYPE_CLASS_ID);
	int cursel = -1;
	for ( long i = 0,j=0; i < scList->Count(ACC_ALL); ++i) {
		if ( (*scList)[ i ].IsPublic() ) {
			ClassDesc* pClassD = (*scList)[ i ].CD();
			if (actShad->ClassID()==pClassD->ClassID())
//			if (shad->ClassID()==pClassD->ClassID())
				cursel = j;
			j++;
			}
		}
	if (cursel>=0)	{
	    SendMessage(hwShadType, CB_SETCURSEL, (WPARAM)cursel, 0L);
	}
}


void GeneralLight::UpdtShadowTypeList(HWND hwndDlg) {
	HWND hwShadType = GetDlgItem(hwndDlg, IDC_SHADOW_TYPE);
	SendMessage(hwShadType, CB_RESETCONTENT, 0L, 0L);
	SubClassList *scList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(SHADOW_TYPE_CLASS_ID);
	int cursel = -1;
	ShadowType *actShad = ActiveShadowType();
//	ShadowType *shad = GetShadowGenerator();
	for ( long i = 0,j=0; i < scList->Count(ACC_ALL); ++i) {
		if ( (*scList)[ i ].IsPublic() ) {
			ClassDesc* pClassD = (*scList)[ i ].CD();
			SendMessage(hwShadType, CB_ADDSTRING, 0L, (LPARAM)(pClassD->ClassName()) );
			if (actShad->ClassID()==pClassD->ClassID())
//			if (shad->ClassID()==pClassD->ClassID())
				cursel = j;
			j++;
			}
		}
	if (cursel>=0)
	    SendMessage(hwShadType, CB_SETCURSEL, (WPARAM)cursel, 0L);
	}


void GeneralLight::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
	{
	iObjParams = ip;
	TimeValue t = ip->GetTime();
	Point3 color;
	inCreate = (flags&BEGIN_EDIT_CREATE)?1:0;

	projDADMgr.Init(this);

	currentEditLight = this;

	if ( !hGeneralLight ) {

		InitGeneralParamDialog(t, ip, flags, prev);
		if(!hICAParam)
			InitICAParamDialog(t, ip, flags, prev);
		if(IsSpot()) 
			InitSpotParamDialog(t, ip, flags, prev);
		if(!hAdvEff)
			InitAdvEffectsDialog(t, ip, flags, prev);

// don't need omni rollup anymore, projector map has been moved to adv
// effects rollout.
// DC - August 1, 2001

		//--OMNI Rollup ---------------------------------------------------
/*
		if (type == OMNI_LIGHT) {
			hOmni = ip->AddRollupPage( 
				hInstance, 
				MAKEINTRESOURCE(IDD_OMNI),
				GeneralLightParamDialogProc, 
				GetString(IDS_DS_PROJ_PARAMS), 
				(LPARAM)this 
				);		
			ip->RegisterDlgWnd(hOmni);				
			}
*/		
		//--Atten Rollup ---------------------------------------------------
/*
		hAttenLight = ip->AddRollupPage( 
				hInstance, 
				MAKEINTRESOURCE(IDD_LIGHT_ATTEN),
				GeneralLightParamDialogProc, 
				GetString(IDS_ATTEN_PARAM), 
				(LPARAM)this 
				);		
		ip->RegisterDlgWnd(hAttenLight);
*/


		//--Shadow Rollup ---------------------------------------------------

		hShadow = ip->AddRollupPage( 
				hInstance, 
				MAKEINTRESOURCE(IDD_LIGHT_SHADOW_RW),
				GeneralLightParamDialogProc, 
				GetString(IDS_DB_SHADOW_PARAMS), 
				(LPARAM)this 
				);		
		ip->RegisterDlgWnd(hShadow);				

		color = GetShadColor(t);
		shadColorSwatch = GetIColorSwatch(GetDlgItem(hShadow, IDC_SHADOW_COLOR), 
				RGB(FLto255i(color.x), FLto255i(color.y), FLto255i(color.z)), 
				GetString(IDS_DS_SHADCOL));
		shadColorSwatch->ForceDitherMode(1);
		//shadColorSwatch->SetModal();

		shadProjMapName = GetICustButton(GetDlgItem(hShadow, IDC_SHADPROJ_MAPNAME));	
		shadProjMapName->SetText(shadProjMap?shadProjMap->GetFullName().data():GetString(IDS_DB_NONE));
		shadProjMapName->SetDADMgr(&projDADMgr);
 
		atmosOpacitySpin = SetupFloatSpinner(hShadow, IDC_ATM_OPACITY_SPIN, IDC_ATM_OPACITY,
			0.0f, 100.0f, GetAtmosOpacity(t)*100.0f,1.0f); 
		atmosColAmtSpin = SetupFloatSpinner(hShadow, IDC_ATM_COLAMT_SPIN, IDC_ATM_COLAMT,
			0.0f, 100.0f, GetAtmosColAmt(t)*100.0f,1.0f); 
		shadMultSpin = SetupFloatSpinner(hShadow, IDC_SHAD_MULT_SPIN, IDC_SHAD_MULT,
			-10000.0f, 10000.0f, GetShadMult(t),0.01f); 

//		UpdtShadowTypeList(hShadow);
	
		shadParamDlg = ActiveShadowType()->CreateShadowParamDlg(ip); 

		if(IsSpot()) {
			if(GetFallsize(t) < GetHotspot(t) + ip->GetLightConeConstraint())
				SetFallsize(t, GetHotspot(t) + ip->GetLightConeConstraint());
			}
		} 
	else {
		// Plug in new values to UI 
      DLSetWindowLongPtr( hGeneralLight, this);
		if(IsSpot())
         DLSetWindowLongPtr( hSpotLight, this);
		// DC 01/08/2001
//		if(type==OMNI_LIGHT)
//       DLSetWindowLongPtr( hOmni, this);
      DLSetWindowLongPtr( hShadow, this);
		if(hICAParam)
         DLSetWindowLongPtr( hICAParam, this);
		if(hAdvEff)
         DLSetWindowLongPtr( hAdvEff, this);
//		if (hAttenLight)
//       DLSetWindowLongPtr( hAttenLight, this);
		
		color = GetRGBColor(t);
//		redSpin->SetValue(FLto255f(color.x), FALSE);
//		greenSpin->SetValue(FLto255f(color.y), FALSE);
//		blueSpin->SetValue(FLto255f(color.z), FALSE);
		color = GetHSVColor(t);
//		hSpin->SetValue(FLto255f(color.x), FALSE);
//		sSpin->SetValue(FLto255f(color.y), FALSE);
//		vSpin->SetValue(FLto255f(color.z), FALSE);
		intensitySpin->SetValue(GetIntensity(t), FALSE);


		attenStartSpin->SetValue(GetAtten(t, ATTEN_START), FALSE);
		attenEndSpin->SetValue(GetAtten(t, ATTEN_END), FALSE);
		SetAttenDisplay( IsDlgButtonChecked(hICAParam, IDC_SHOW_RANGES) );
		SetUseAtten( IsDlgButtonChecked(hICAParam, IDC_USE_ATTEN) );

		attenStart1Spin->SetValue(GetAtten(t, ATTEN1_START), FALSE);
		attenEnd1Spin->SetValue(GetAtten(t, ATTEN1_END), FALSE);
		SetAttenNearDisplay( IsDlgButtonChecked(hICAParam, IDC_SHOW_RANGES1) );
		SetUseAttenNear( IsDlgButtonChecked(hICAParam, IDC_USE_ATTEN1) );
		decaySpin->SetValue( GetDecayRadius(t),FALSE );

		if(IsSpot()) {
			hotsizeSpin->SetValue(GetHotspot(t), FALSE);
			fallsizeSpin->SetValue(GetFallsize(t), FALSE);
			if (type == FSPOT_LIGHT||type==DIR_LIGHT) 
				tDistSpin->SetValue(GetTDist(t), FALSE);
			SetConeDisplay( IsDlgButtonChecked(hSpotLight, IDC_SHOW_CONE) );
			}

		if (projMapName)
			projMapName->SetDADMgr(&projDADMgr);
		if (shadProjMapName)
			shadProjMapName->SetDADMgr(&projDADMgr);

		if (shadParamDlg) {
			shadParamDlg->DeleteThis();
			}
		shadParamDlg = ActiveShadowType()->CreateShadowParamDlg(ip); 
		}	// change made to reflect simpler type selection
	if(!uiConsistency)
	SendMessage( GetDlgItem(hGeneralLight, IDC_LIGHT_TYPE), CB_SETCURSEL, idFromType(type), (LPARAM)0 );
	else
		SendMessage( GetDlgItem(hGeneralLight, IDC_LIGHT_TYPE), CB_SETCURSEL, idFromTypeSimplified(type), (LPARAM)0 );

	UpdateForAmbOnly();

	if (!inCreate)
		ip->AddSFXRollupPage(0);
	}

		
void GeneralLight::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{
	Point3 color;
	TimeValue t = ip->GetTime();
	if (projMapName)
		projMapName->SetDADMgr(NULL);
	if (shadProjMapName)
		shadProjMapName->SetDADMgr(NULL);
	color = GetRGBColor(t);
	dlgRed =   FLto255i(color.x);
	dlgGreen = FLto255i(color.y);
	dlgBlue =  FLto255i(color.z);
	color = GetHSVColor(t);
	dlgH = FLto255i(color.x);
	dlgS = FLto255i(color.y);
	dlgV = FLto255i(color.z);
	dlgIntensity = GetIntensity(t);
	dlgContrast = GetContrast(t);
	dlgDiffsoft = GetDiffuseSoft(t);
	if(IsSpot()) {
		dlgHotsize = GetHotspot(t);
		dlgFallsize = GetFallsize(t);
		dlgTDist = GetTDist(t);
		dlgShowCone = IsDlgButtonChecked(hSpotLight, IDC_SHOW_CONE );
		dlgShape = GetSpotShape();
		dlgAspect = GetAspect(t);
		}

	dlgAtten1Start = GetAtten(t, ATTEN1_START);
	dlgAtten1End = GetAtten(t, ATTEN1_END);
	dlgAttenStart = GetAtten(t, ATTEN_START);
	dlgAttenEnd = GetAtten(t, ATTEN_END);
	dlgShowAtten = IsDlgButtonChecked(hICAParam, IDC_SHOW_RANGES );
	dlgShowAttenNear = IsDlgButtonChecked(hICAParam, IDC_SHOW_RANGES1 );
	dlgShowDecay = IsDlgButtonChecked(hICAParam, IDC_SHOW_DECAY );
	dlgDecayRadius = GetDecayRadius(t);
	dlgAtmosOpacity = GetAtmosOpacity(t);
	dlgAtmosColamt = GetAtmosColAmt(t);
	dlgAtmosShadows = GetAtmosShadows(t);
	Point3 col = GetShadColor(t);
	dlgShadColor.r  = int(col.x*255.0f);
	dlgShadColor.g  = int(col.y*255.0f);
	dlgShadColor.b  = int(col.z*255.0f);
	dlgLightAffectShadColor = GetLightAffectsShadow();
	dlgShadMult = GetShadMult(t);
	dlgShadType = shadType->ClassID();
	dlgUseGlobalShadowParams = useGlobalShadowParams;

	currentEditLight = NULL;

	if ( flags&END_EDIT_REMOVEUI ) {
		HWND hwgl = hGeneralLight;
		hGeneralLight = NULL; // This keeps UpdateUI from jumping in
		simpDialInitialized = FALSE;

		if ( IsSpot()&&hSpotLight ) {
			ip->UnRegisterDlgWnd(hSpotLight);		
			ip->DeleteRollupPage(hSpotLight);
			hSpotLight = NULL;
			}

		if(hAdvEff)	{
			ip->UnRegisterDlgWnd(hAdvEff);
			ip->DeleteRollupPage(hAdvEff);
			hAdvEff = NULL;
			}

		if(hICAParam)	{
			ip->UnRegisterDlgWnd(hICAParam);
			ip->DeleteRollupPage(hICAParam);
			hICAParam = NULL;
		}
		// DC 01/08/2001
//		if ((type==OMNI_LIGHT)&&hOmni) {
//			ip->UnRegisterDlgWnd(hOmni);		
//			ip->DeleteRollupPage(hOmni);
//			hOmni = NULL;
//		}
//		ip->UnRegisterDlgWnd(hAttenLight);		
//		ip->DeleteRollupPage(hAttenLight);

		ip->UnRegisterDlgWnd(hShadow);		
		ip->DeleteRollupPage(hShadow);
		hShadow = NULL;

		ip->UnRegisterDlgWnd(hwgl);		
		ip->DeleteRollupPage(hwgl);

		if (shadParamDlg)
			shadParamDlg->DeleteThis(); 

		ip->DeleteSFXRollupPage();
		} 
	else {
      DLSetWindowLongPtr( hGeneralLight, 0);
		if( IsSpot() ) {
         DLSetWindowLongPtr( hSpotLight, 0);
			}
		// DC 01/08/2001
//		if (type==OMNI_LIGHT) {
//       DLSetWindowLongPtr( hOmni, 0);
//			}		
      DLSetWindowLongPtr( hICAParam, 0);
      DLSetWindowLongPtr( hShadow, 0);
		}
	iObjParams = NULL;
	}


void GeneralLight::EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags) {

	if ((flags&FILE_ENUM_CHECK_AWORK1)&&TestAFlag(A_WORK1)) return; // LAM - 4/21/03

	if ((flags&FILE_ENUM_INACTIVE)||projector)
		if (projMap) projMap->EnumAuxFiles(nameEnum,flags);

	// LAM - 4/21/03 - handle remainder of references 
	for (int i=0; i<NumRefs(); i++) {
		if (i != PROJMAP_REF) {
			ReferenceMaker *srm = GetReference(i);
			if (srm) srm->EnumAuxFiles(nameEnum,flags);
		}
	}
	
	// LAM - 4/21/03 - and pick up custom attributes. Sets A_WORK1 flag
	Animatable::EnumAuxFiles( nameEnum, flags ); 
} 

TSTR GeneralLight::SvGetRelTip(IGraphObjectManager *gom, IGraphNode *gNodeTarger, int id, IGraphNode *gNodeMaker)
{
	TSTR strBuf = SvGetName(gom, gNodeMaker, false) + _T(" -> ") + gNodeTarger->GetAnim()->SvGetName(gom, gNodeTarger, false) + _T(" ");
	strBuf += Include() ? GetString(IDS_LIGHT_INCLUDE_SV) : GetString(IDS_LIGHT_EXCLUDE_SV);
	return strBuf;
}

bool GeneralLight::SvCanDetachRel(IGraphObjectManager *gom, IGraphNode *gNodeTarget, int id, IGraphNode *gNodeMaker)
{
	return true;
}

bool GeneralLight::SvDetachRel(IGraphObjectManager *gom, IGraphNode *gNodeTarget, int id, IGraphNode *gNodeMaker)
{
	if( gNodeTarget->GetAnim() ) {
		ExclList *exList = GetExclList();
		exList->RemoveNode( (INode*)gNodeTarget->GetAnim() );
		return true;
	}

	return false;
}

SvGraphNodeReference GeneralLight::SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags)
{
	SvGraphNodeReference nodeRef = Object::SvTraverseAnimGraph( gom, owner, id, flags );

	if( nodeRef.stat == SVT_PROCEED ) {
		ExclList *exList = GetExclList();
		for( int i=0; i<exList->Count(); i++ )
			gom->AddRelationship( nodeRef.gNode, (Animatable*)((*exList)[i]), i, RELTYPE_LIGHT );
	}

	return nodeRef;
}

bool GeneralLight::SvHandleRelDoubleClick(IGraphObjectManager *gom, IGraphNode *gNodeTarget, int id, IGraphNode *gNodeMaker)
{
	GetCOREInterface()->DoExclusionListDialog(&exclList, TRUE);
	return true;
}

void GeneralLight::DoBitmapFit(HWND hwnd, TimeValue t) {
	if(!(IsSpot()&&shape==RECT_LIGHT)) return;
	BitmapInfo bi;
	TheManager->SelectFileInputEx(&bi, hwnd, GetString(IDS_DB_SELECT_TO_FIT));
	if (bi.Name()[0]) {
		TheManager->GetImageInfo(&bi, bi.Name());
       	int w = bi.Width();
        int h = bi.Height();
		if (h) {
			SetAspect(t,float(w)/float(h));
			iObjParams->RedrawViews(t,REDRAW_END);
			}
		}
	}

void GeneralLight::AssignProjectorMap(Texmap *m, BOOL newmat) {	
	assert(!m || IsTex(m));
	ReplaceReference(PROJMAP_REF,m);
	if (projMap) {
		projMapName->SetText(projMap->GetFullName().data());
		if (newmat) 
			projMap->RecursInitSlotType(type==OMNI_LIGHT?MAPSLOT_ENVIRON:MAPSLOT_TEXTURE);
		} 
	else {
		projMapName->SetText(GetString(IDS_DB_NONE));
		}		
	if (projMap) {
		projector = TRUE;
		// DC 01/08/2001
		CheckDlgButton(hAdvEff,IDC_PROJECTOR, projector);
		// CheckDlgButton(type==OMNI_LIGHT?hOmni:hSpotLight,IDC_PROJECTOR, projector);
		}
	NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
	}


void GeneralLight::AssignShadProjMap(Texmap *m, BOOL newmat) {	
	assert(!m || IsTex(m));
	ReplaceReference(SHADPROJMAP_REF,m);
	if (shadProjMap) {
		shadProjMapName->SetText(shadProjMap->GetFullName().data());
		if (newmat) 
			shadProjMap->RecursInitSlotType(type==OMNI_LIGHT?MAPSLOT_ENVIRON:MAPSLOT_TEXTURE);
		} 
	else {
		shadProjMapName->SetText(GetString(IDS_DB_NONE));
		}		
	if (shadProjMap) {
		SetUseShadowColorMap( 0, TRUE);
		CheckDlgButton(hShadow, IDC_USE_SHAD_COLMAP, TRUE);
		}
	NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
	}


void GeneralLight::BrowseProjectorMap(HWND hWnd) {	
	BOOL newMat, cancel;
	Texmap *m = (Texmap *)iObjParams->DoMaterialBrowseDlg(
		hWnd,BROWSE_MAPSONLY|BROWSE_INCNONE,newMat,cancel);
	if (!cancel)
		AssignProjectorMap(m,newMat);
	}

void GeneralLight::BrowseShadProjMap(HWND hWnd) {	
	BOOL newMat, cancel;
	Texmap *m = (Texmap *)iObjParams->DoMaterialBrowseDlg(
		hWnd,BROWSE_MAPSONLY|BROWSE_INCNONE,newMat,cancel);
	if (!cancel) {
		AssignShadProjMap(m,newMat);
		//SetUseShadowColorMap(TimeValue(0),1);
		}
	}

void GeneralLight::SetProjMap(Texmap* pmap)
	{
	ReplaceReference(PROJMAP_REF,pmap);
	if (projMap) projector = TRUE;	
	NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
	}

void GeneralLight::SetShadowProjMap(Texmap* pmap)
	{
	ReplaceReference(SHADPROJMAP_REF,pmap);
	NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
	}

void GeneralLight::UpdateTargDistance(TimeValue t, INode* inode) {
	if (type == TSPOT_LIGHT&&hSpotLight) {
		Point3 pt,v[3];
		if (GetTargetPoint(t, inode, pt)) {
			Matrix3 tm = inode->GetObjectTM(t);
			float den = FLength(tm.GetRow(2));
			float dist = (den!=0) ? FLength(tm.GetTrans()-pt) / den : 0.0f;
			targDist = dist;
			const TCHAR *buf;
			buf = FormatUniverseValue(targDist);
			SetWindowText(GetDlgItem(hGeneralLight,IDC_TARG_DISTANCE),buf);
			}
		}
	}

const TCHAR *GeneralLight::GetObjectName()
{ 
	switch(type) {
	case OMNI_LIGHT:
		return GetString(IDS_DB_OMNI_LIGHT);
	case DIR_LIGHT:
		return GetString(IDS_DB_DIR_LIGHT);
	case TDIR_LIGHT:
		return GetString(IDS_DS_TDIR_LIGHT);
	case FSPOT_LIGHT:
		return GetString(IDS_DB_FREE_SPOT);
	case TSPOT_LIGHT:
		return GetString(IDS_DB_TARGET_SPOT);
	default:
		return NULL;
	}
}

void GeneralLight::InitNodeName(TSTR& s)
{
	switch(type) {
	case OMNI_LIGHT:
		s = GetString(IDS_DB_OMNI);
		return; 
	case DIR_LIGHT:
		s = GetString(IDS_DB_DIRECT);
		return;
	case TDIR_LIGHT:
		s = GetString(IDS_DS_TDIRECT);
		return;
	case FSPOT_LIGHT:
		s = GetString(IDS_DB_FSPOT); 
		return;
	case TSPOT_LIGHT:
		s = GetString(IDS_DB_SPOT); 
		return;
	default:
		s = GetString(IDS_DB_LIGHT); 
	}
	return;
}

#define FZ (0.0f)

#define SET_QUAD(face, v0, v1, v2, v3) \
	staticMesh[DIR_MESH].faces[face].setVerts(v0, v1, v2); \
	staticMesh[DIR_MESH].faces[face].setEdgeVisFlags(1,1,0); \
	staticMesh[DIR_MESH].faces[face+1].setVerts(v0, v2, v3); \
	staticMesh[DIR_MESH].faces[face+1].setEdgeVisFlags(0,1,1);

void GeneralLight::BuildStaticMeshes()
{
	if(!meshBuilt) {
		int nverts = 312;
		int nfaces = 556;
		
		// Build a leetle octahedron
		staticMesh[OMNI_MESH].setNumVerts(nverts);
		staticMesh[OMNI_MESH].setNumFaces(nfaces);
		float s = 8.0f;
		staticMesh[OMNI_MESH].setVert(0, Point3(-5.66792e-07, 0.103813, 24.7909));
		staticMesh[OMNI_MESH].setVert(1, Point3(-1.59306e-07, -3.6445, 24.0428));
		staticMesh[OMNI_MESH].setVert(2, Point3(-2.89448e-07, -6.6218, 22.3999));
		staticMesh[OMNI_MESH].setVert(3, Point3(-3.94646e-07, -9.02845, 19.3158));
		staticMesh[OMNI_MESH].setVert(4, Point3(-1.87731e-06, -10.403, 14.5684));
		staticMesh[OMNI_MESH].setVert(5, Point3(-4.13718e-07, -9.46476, 9.28082));
		staticMesh[OMNI_MESH].setVert(6, Point3(-5.569e-07, -5.52081, 4.13608));
		staticMesh[OMNI_MESH].setVert(7, Point3(-1.9994e-07, -4.57409, 1.25256));
		staticMesh[OMNI_MESH].setVert(8, Point3(-1.35209e-07, -3.09322, -0.475119));
		staticMesh[OMNI_MESH].setVert(9, Point3(0.0, -3.472, -0.748604));
		staticMesh[OMNI_MESH].setVert(10, Point3(0.0, -3.472, -2.31931));
		staticMesh[OMNI_MESH].setVert(11, Point3(0.0, -3.12293, -3.41008));
		staticMesh[OMNI_MESH].setVert(12, Point3(2.84941e-07, -3.12293, -7.12286));
		staticMesh[OMNI_MESH].setVert(13, Point3(6.5839e-07, -1.33407, -8.99898));
		staticMesh[OMNI_MESH].setVert(14, Point3(1.03331e-06, 0.103839, -8.99898));
		staticMesh[OMNI_MESH].setVert(15, Point3(9.68521e-06, 0.103815, 24.7909));
		staticMesh[OMNI_MESH].setVert(16, Point3(1.43443, -3.35918, 24.0428));
		staticMesh[OMNI_MESH].setVert(17, Point3(2.57379, -6.10985, 22.3999));
		staticMesh[OMNI_MESH].setVert(18, Point3(3.49478, -8.3333, 19.3158));
		staticMesh[OMNI_MESH].setVert(19, Point3(4.0208, -9.60323, 14.5684));
		staticMesh[OMNI_MESH].setVert(20, Point3(3.66174, -8.73639, 9.28082));
		staticMesh[OMNI_MESH].setVert(21, Point3(2.15246, -5.09266, 4.13608));
		staticMesh[OMNI_MESH].setVert(22, Point3(1.79017, -4.218, 1.25256));
		staticMesh[OMNI_MESH].setVert(23, Point3(1.22346, -2.84986, -0.475119));
		staticMesh[OMNI_MESH].setVert(24, Point3(1.36841, -3.1998, -0.748604));
		staticMesh[OMNI_MESH].setVert(25, Point3(1.36841, -3.1998, -2.31931));
		staticMesh[OMNI_MESH].setVert(26, Point3(1.23483, -2.8773, -3.41008));
		staticMesh[OMNI_MESH].setVert(27, Point3(1.23483, -2.8773, -7.12286));
		staticMesh[OMNI_MESH].setVert(28, Point3(0.550266, -1.22462, -8.99898));
		staticMesh[OMNI_MESH].setVert(29, Point3(7.91005e-07, 0.10384, -8.99898));
		staticMesh[OMNI_MESH].setVert(30, Point3(1.89835e-05, 0.103818, 24.7909));
		staticMesh[OMNI_MESH].setVert(31, Point3(2.65048, -2.54664, 24.0428));
		staticMesh[OMNI_MESH].setVert(32, Point3(4.75575, -4.65191, 22.3999));
		staticMesh[OMNI_MESH].setVert(33, Point3(6.45751, -6.35367, 19.3158));
		staticMesh[OMNI_MESH].setVert(34, Point3(7.42947, -7.32563, 14.5684));
		staticMesh[OMNI_MESH].setVert(35, Point3(6.76602, -6.66218, 9.28082));
		staticMesh[OMNI_MESH].setVert(36, Point3(3.97723, -3.87339, 4.13608));
		staticMesh[OMNI_MESH].setVert(37, Point3(3.30779, -3.20396, 1.25256));
		staticMesh[OMNI_MESH].setVert(38, Point3(2.26066, -2.15682, -0.475119));
		staticMesh[OMNI_MESH].setVert(39, Point3(2.5285, -2.42466, -0.748604));
		staticMesh[OMNI_MESH].setVert(40, Point3(2.5285, -2.42466, -2.31931));
		staticMesh[OMNI_MESH].setVert(41, Point3(2.28167, -2.17783, -3.41008));
		staticMesh[OMNI_MESH].setVert(42, Point3(2.28167, -2.17783, -7.12286));
		staticMesh[OMNI_MESH].setVert(43, Point3(1.01676, -0.912917, -8.99898));
		staticMesh[OMNI_MESH].setVert(44, Point3(1.94958e-07, 0.103841, -8.99898));
		staticMesh[OMNI_MESH].setVert(45, Point3(2.54208e-05, 0.103827, 24.7909));
		staticMesh[OMNI_MESH].setVert(46, Point3(3.46302, -1.33059, 24.0428));
		staticMesh[OMNI_MESH].setVert(47, Point3(6.21369, -2.46995, 22.3999));
		staticMesh[OMNI_MESH].setVert(48, Point3(8.43714, -3.39094, 19.3158));
		staticMesh[OMNI_MESH].setVert(49, Point3(9.70707, -3.91696, 14.5684));
		staticMesh[OMNI_MESH].setVert(50, Point3(8.84023, -3.5579, 9.28082));
		staticMesh[OMNI_MESH].setVert(51, Point3(5.1965, -2.04862, 4.13608));
		staticMesh[OMNI_MESH].setVert(52, Point3(4.32184, -1.68633, 1.25256));
		staticMesh[OMNI_MESH].setVert(53, Point3(2.9537, -1.11962, -0.475119));
		staticMesh[OMNI_MESH].setVert(54, Point3(3.30364, -1.26457, -0.748604));
		staticMesh[OMNI_MESH].setVert(55, Point3(3.30364, -1.26457, -2.31931));
		staticMesh[OMNI_MESH].setVert(56, Point3(2.98114, -1.13099, -3.41008));
		staticMesh[OMNI_MESH].setVert(57, Point3(2.98114, -1.13099, -7.12286));
		staticMesh[OMNI_MESH].setVert(58, Point3(1.32846, -0.446424, -8.99898));
		staticMesh[OMNI_MESH].setVert(59, Point3(-5.20297e-07, 0.103842, -8.99898));
		staticMesh[OMNI_MESH].setVert(60, Point3(2.85203e-05, 0.103837, 24.7909));
		staticMesh[OMNI_MESH].setVert(61, Point3(3.74834, 0.103837, 24.0428));
		staticMesh[OMNI_MESH].setVert(62, Point3(6.72565, 0.103837, 22.3999));
		staticMesh[OMNI_MESH].setVert(63, Point3(9.1323, 0.103838, 19.3158));
		staticMesh[OMNI_MESH].setVert(64, Point3(10.5069, 0.103837, 14.5684));
		staticMesh[OMNI_MESH].setVert(65, Point3(9.5686, 0.103839, 9.28082));
		staticMesh[OMNI_MESH].setVert(66, Point3(5.62465, 0.103839, 4.13608));
		staticMesh[OMNI_MESH].setVert(67, Point3(4.67793, 0.10384, 1.25256));
		staticMesh[OMNI_MESH].setVert(68, Point3(3.19706, 0.10384, -0.475119));
		staticMesh[OMNI_MESH].setVert(69, Point3(3.57584, 0.10384, -0.748604));
		staticMesh[OMNI_MESH].setVert(70, Point3(3.57584, 0.103841, -2.31931));
		staticMesh[OMNI_MESH].setVert(71, Point3(3.22676, 0.103841, -3.41008));
		staticMesh[OMNI_MESH].setVert(72, Point3(3.22676, 0.103841, -7.12286));
		staticMesh[OMNI_MESH].setVert(73, Point3(1.43791, 0.103842, -8.99898));
		staticMesh[OMNI_MESH].setVert(74, Point3(-1.47397e-06, 0.103842, -8.99898));
		staticMesh[OMNI_MESH].setVert(75, Point3(2.75666e-05, 0.103847, 24.7909));
		staticMesh[OMNI_MESH].setVert(76, Point3(3.46302, 1.53827, 24.0428));
		staticMesh[OMNI_MESH].setVert(77, Point3(6.21369, 2.67763, 22.3999));
		staticMesh[OMNI_MESH].setVert(78, Point3(8.43714, 3.59862, 19.3158));
		staticMesh[OMNI_MESH].setVert(79, Point3(9.70707, 4.12464, 14.5684));
		staticMesh[OMNI_MESH].setVert(80, Point3(8.84023, 3.76558, 9.28082));
		staticMesh[OMNI_MESH].setVert(81, Point3(5.1965, 2.2563, 4.13608));
		staticMesh[OMNI_MESH].setVert(82, Point3(4.32184, 1.89401, 1.25256));
		staticMesh[OMNI_MESH].setVert(83, Point3(2.9537, 1.3273, -0.475119));
		staticMesh[OMNI_MESH].setVert(84, Point3(3.30364, 1.47225, -0.748604));
		staticMesh[OMNI_MESH].setVert(85, Point3(3.30364, 1.47225, -2.31931));
		staticMesh[OMNI_MESH].setVert(86, Point3(2.98114, 1.33867, -3.41008));
		staticMesh[OMNI_MESH].setVert(87, Point3(2.98114, 1.33867, -7.12286));
		staticMesh[OMNI_MESH].setVert(88, Point3(1.32846, 0.654107, -8.99898));
		staticMesh[OMNI_MESH].setVert(89, Point3(-2.66606e-06, 0.103842, -8.99898));
		staticMesh[OMNI_MESH].setVert(90, Point3(2.32751e-05, 0.103856, 24.7909));
		staticMesh[OMNI_MESH].setVert(91, Point3(2.65048, 2.75431, 24.0428));
		staticMesh[OMNI_MESH].setVert(92, Point3(4.75575, 4.85959, 22.3999));
		staticMesh[OMNI_MESH].setVert(93, Point3(6.45751, 6.56134, 19.3158));
		staticMesh[OMNI_MESH].setVert(94, Point3(7.42947, 7.5333, 14.5684));
		staticMesh[OMNI_MESH].setVert(95, Point3(6.76602, 6.86986, 9.28082));
		staticMesh[OMNI_MESH].setVert(96, Point3(3.97723, 4.08107, 4.13608));
		staticMesh[OMNI_MESH].setVert(97, Point3(3.30779, 3.41163, 1.25256));
		staticMesh[OMNI_MESH].setVert(98, Point3(2.26066, 2.3645, -0.475119));
		staticMesh[OMNI_MESH].setVert(99, Point3(2.5285, 2.63234, -0.748604));
		staticMesh[OMNI_MESH].setVert(100, Point3(2.5285, 2.63234, -2.31931));
		staticMesh[OMNI_MESH].setVert(101, Point3(2.28167, 2.38551, -3.41008));
		staticMesh[OMNI_MESH].setVert(102, Point3(2.28167, 2.38551, -7.12286));
		staticMesh[OMNI_MESH].setVert(103, Point3(1.01675, 1.1206, -8.99898));
		staticMesh[OMNI_MESH].setVert(104, Point3(-3.1429e-06, 0.103841, -8.99898));
		staticMesh[OMNI_MESH].setVert(105, Point3(1.52881e-05, 0.103862, 24.7909));
		staticMesh[OMNI_MESH].setVert(106, Point3(1.43443, 3.56685, 24.0428));
		staticMesh[OMNI_MESH].setVert(107, Point3(2.5738, 6.31752, 22.3999));
		staticMesh[OMNI_MESH].setVert(108, Point3(3.49478, 8.54098, 19.3158));
		staticMesh[OMNI_MESH].setVert(109, Point3(4.0208, 9.81091, 14.5684));
		staticMesh[OMNI_MESH].setVert(110, Point3(3.66174, 8.94407, 9.28082));
		staticMesh[OMNI_MESH].setVert(111, Point3(2.15246, 5.30034, 4.13608));
		staticMesh[OMNI_MESH].setVert(112, Point3(1.79016, 4.42568, 1.25256));
		staticMesh[OMNI_MESH].setVert(113, Point3(1.22346, 3.05754, -0.475119));
		staticMesh[OMNI_MESH].setVert(114, Point3(1.36841, 3.40748, -0.748604));
		staticMesh[OMNI_MESH].setVert(115, Point3(1.36841, 3.40748, -2.31931));
		staticMesh[OMNI_MESH].setVert(116, Point3(1.23483, 3.08498, -3.41008));
		staticMesh[OMNI_MESH].setVert(117, Point3(1.23483, 3.08498, -7.12286));
		staticMesh[OMNI_MESH].setVert(118, Point3(0.550261, 1.4323, -8.99898));
		staticMesh[OMNI_MESH].setVert(119, Point3(-4.09658e-06, 0.10384, -8.99898));
		staticMesh[OMNI_MESH].setVert(120, Point3(5.3971e-06, 0.103866, 24.7909));
		staticMesh[OMNI_MESH].setVert(121, Point3(4.48357e-06, 3.85218, 24.0428));
		staticMesh[OMNI_MESH].setVert(122, Point3(3.96172e-06, 6.82948, 22.3999));
		staticMesh[OMNI_MESH].setVert(123, Point3(3.12123e-06, 9.23613, 19.3158));
		staticMesh[OMNI_MESH].setVert(124, Point3(3.35185e-06, 10.6107, 14.5684));
		staticMesh[OMNI_MESH].setVert(125, Point3(7.0962e-07, 9.67244, 9.28082));
		staticMesh[OMNI_MESH].setVert(126, Point3(0.0, 5.72849, 4.13608));
		staticMesh[OMNI_MESH].setVert(127, Point3(-9.90686e-07, 4.78177, 1.25256));
		staticMesh[OMNI_MESH].setVert(128, Point3(-1.33787e-06, 3.3009, -0.475119));
		staticMesh[OMNI_MESH].setVert(129, Point3(-1.45236e-06, 3.67968, -0.748604));
		staticMesh[OMNI_MESH].setVert(130, Point3(-1.89551e-06, 3.67968, -2.31931));
		staticMesh[OMNI_MESH].setVert(131, Point3(-2.2296e-06, 3.33061, -3.41008));
		staticMesh[OMNI_MESH].setVert(132, Point3(-3.34556e-06, 3.33061, -7.12286));
		staticMesh[OMNI_MESH].setVert(133, Point3(-4.00992e-06, 1.54175, -8.99898));
		staticMesh[OMNI_MESH].setVert(134, Point3(-4.25914e-06, 0.103839, -8.99898));
		staticMesh[OMNI_MESH].setVert(135, Point3(-4.85832e-06, 0.103864, 24.7909));
		staticMesh[OMNI_MESH].setVert(136, Point3(-1.43442, 3.56685, 24.0428));
		staticMesh[OMNI_MESH].setVert(137, Point3(-2.57379, 6.31752, 22.3999));
		staticMesh[OMNI_MESH].setVert(138, Point3(-3.49477, 8.54098, 19.3158));
		staticMesh[OMNI_MESH].setVert(139, Point3(-4.02079, 9.81091, 14.5684));
		staticMesh[OMNI_MESH].setVert(140, Point3(-3.66174, 8.94407, 9.28082));
		staticMesh[OMNI_MESH].setVert(141, Point3(-2.15246, 5.30034, 4.13608));
		staticMesh[OMNI_MESH].setVert(142, Point3(-1.79017, 4.42568, 1.25256));
		staticMesh[OMNI_MESH].setVert(143, Point3(-1.22346, 3.05754, -0.475118));
		staticMesh[OMNI_MESH].setVert(144, Point3(-1.36842, 3.40748, -0.748603));
		staticMesh[OMNI_MESH].setVert(145, Point3(-1.36842, 3.40748, -2.31931));
		staticMesh[OMNI_MESH].setVert(146, Point3(-1.23483, 3.08498, -3.41007));
		staticMesh[OMNI_MESH].setVert(147, Point3(-1.23483, 3.08498, -7.12286));
		staticMesh[OMNI_MESH].setVert(148, Point3(-0.550269, 1.4323, -8.99898));
		staticMesh[OMNI_MESH].setVert(149, Point3(-4.09658e-06, 0.103838, -8.99898));
		staticMesh[OMNI_MESH].setVert(150, Point3(-1.39182e-05, 0.10386, 24.7909));
		staticMesh[OMNI_MESH].setVert(151, Point3(-2.65047, 2.75432, 24.0428));
		staticMesh[OMNI_MESH].setVert(152, Point3(-4.75574, 4.85959, 22.3999));
		staticMesh[OMNI_MESH].setVert(153, Point3(-6.4575, 6.56135, 19.3158));
		staticMesh[OMNI_MESH].setVert(154, Point3(-7.42947, 7.53331, 14.5684));
		staticMesh[OMNI_MESH].setVert(155, Point3(-6.76602, 6.86986, 9.28082));
		staticMesh[OMNI_MESH].setVert(156, Point3(-3.97723, 4.08107, 4.13608));
		staticMesh[OMNI_MESH].setVert(157, Point3(-3.3078, 3.41163, 1.25256));
		staticMesh[OMNI_MESH].setVert(158, Point3(-2.26066, 2.3645, -0.475118));
		staticMesh[OMNI_MESH].setVert(159, Point3(-2.5285, 2.63234, -0.748603));
		staticMesh[OMNI_MESH].setVert(160, Point3(-2.5285, 2.63234, -2.31931));
		staticMesh[OMNI_MESH].setVert(161, Point3(-2.28167, 2.38551, -3.41008));
		staticMesh[OMNI_MESH].setVert(162, Point3(-2.28167, 2.38551, -7.12286));
		staticMesh[OMNI_MESH].setVert(163, Point3(-1.01676, 1.1206, -8.99898));
		staticMesh[OMNI_MESH].setVert(164, Point3(-3.1429e-06, 0.103837, -8.99898));
		staticMesh[OMNI_MESH].setVert(165, Point3(-2.08324e-05, 0.103852, 24.7909));
		staticMesh[OMNI_MESH].setVert(166, Point3(-3.46301, 1.53827, 24.0428));
		staticMesh[OMNI_MESH].setVert(167, Point3(-6.21368, 2.67763, 22.3999));
		staticMesh[OMNI_MESH].setVert(168, Point3(-8.43714, 3.59862, 19.3158));
		staticMesh[OMNI_MESH].setVert(169, Point3(-9.70707, 4.12464, 14.5684));
		staticMesh[OMNI_MESH].setVert(170, Point3(-8.84023, 3.76558, 9.28082));
		staticMesh[OMNI_MESH].setVert(171, Point3(-5.1965, 2.2563, 4.13608));
		staticMesh[OMNI_MESH].setVert(172, Point3(-4.32184, 1.894, 1.25256));
		staticMesh[OMNI_MESH].setVert(173, Point3(-2.9537, 1.3273, -0.475118));
		staticMesh[OMNI_MESH].setVert(174, Point3(-3.30365, 1.47225, -0.748603));
		staticMesh[OMNI_MESH].setVert(175, Point3(-3.30365, 1.47225, -2.31931));
		staticMesh[OMNI_MESH].setVert(176, Point3(-2.98114, 1.33867, -3.41008));
		staticMesh[OMNI_MESH].setVert(177, Point3(-2.98115, 1.33867, -7.12286));
		staticMesh[OMNI_MESH].setVert(178, Point3(-1.32846, 0.654102, -8.99898));
		staticMesh[OMNI_MESH].setVert(179, Point3(-2.66607e-06, 0.103837, -8.99898));
		staticMesh[OMNI_MESH].setVert(180, Point3(-2.39318e-05, 0.103843, 24.7909));
		staticMesh[OMNI_MESH].setVert(181, Point3(-3.74834, 0.103842, 24.0428));
		staticMesh[OMNI_MESH].setVert(182, Point3(-6.72564, 0.103842, 22.3999));
		staticMesh[OMNI_MESH].setVert(183, Point3(-9.13229, 0.103841, 19.3158));
		staticMesh[OMNI_MESH].setVert(184, Point3(-10.5069, 0.103843, 14.5684));
		staticMesh[OMNI_MESH].setVert(185, Point3(-9.5686, 0.10384, 9.28082));
		staticMesh[OMNI_MESH].setVert(186, Point3(-5.62465, 0.10384, 4.13608));
		staticMesh[OMNI_MESH].setVert(187, Point3(-4.67793, 0.103839, 1.25256));
		staticMesh[OMNI_MESH].setVert(188, Point3(-3.19706, 0.103839, -0.475118));
		staticMesh[OMNI_MESH].setVert(189, Point3(-3.57584, 0.103839, -0.748603));
		staticMesh[OMNI_MESH].setVert(190, Point3(-3.57584, 0.103838, -2.31931));
		staticMesh[OMNI_MESH].setVert(191, Point3(-3.22677, 0.103838, -3.41008));
		staticMesh[OMNI_MESH].setVert(192, Point3(-3.22677, 0.103838, -7.12286));
		staticMesh[OMNI_MESH].setVert(193, Point3(-1.43791, 0.103837, -8.99898));
		staticMesh[OMNI_MESH].setVert(194, Point3(-1.47397e-06, 0.103837, -8.99898));
		staticMesh[OMNI_MESH].setVert(195, Point3(-2.27397e-05, 0.103832, 24.7909));
		staticMesh[OMNI_MESH].setVert(196, Point3(-3.46301, -1.33059, 24.0428));
		staticMesh[OMNI_MESH].setVert(197, Point3(-6.21368, -2.46995, 22.3999));
		staticMesh[OMNI_MESH].setVert(198, Point3(-8.43714, -3.39094, 19.3158));
		staticMesh[OMNI_MESH].setVert(199, Point3(-9.70707, -3.91696, 14.5684));
		staticMesh[OMNI_MESH].setVert(200, Point3(-8.84023, -3.5579, 9.28082));
		staticMesh[OMNI_MESH].setVert(201, Point3(-5.1965, -2.04862, 4.13608));
		staticMesh[OMNI_MESH].setVert(202, Point3(-4.32184, -1.68633, 1.25256));
		staticMesh[OMNI_MESH].setVert(203, Point3(-2.9537, -1.11962, -0.475118));
		staticMesh[OMNI_MESH].setVert(204, Point3(-3.30364, -1.26458, -0.748603));
		staticMesh[OMNI_MESH].setVert(205, Point3(-3.30364, -1.26458, -2.31931));
		staticMesh[OMNI_MESH].setVert(206, Point3(-2.98114, -1.13099, -3.41008));
		staticMesh[OMNI_MESH].setVert(207, Point3(-2.98114, -1.13099, -7.12286));
		staticMesh[OMNI_MESH].setVert(208, Point3(-1.32846, -0.446428, -8.99898));
		staticMesh[OMNI_MESH].setVert(209, Point3(-5.20298e-07, 0.103837, -8.99898));
		staticMesh[OMNI_MESH].setVert(210, Point3(-1.82098e-05, 0.103823, 24.7909));
		staticMesh[OMNI_MESH].setVert(211, Point3(-2.65048, -2.54664, 24.0428));
		staticMesh[OMNI_MESH].setVert(212, Point3(-4.75575, -4.65191, 22.3999));
		staticMesh[OMNI_MESH].setVert(213, Point3(-6.4575, -6.35367, 19.3158));
		staticMesh[OMNI_MESH].setVert(214, Point3(-7.42947, -7.32563, 14.5684));
		staticMesh[OMNI_MESH].setVert(215, Point3(-6.76602, -6.66218, 9.28082));
		staticMesh[OMNI_MESH].setVert(216, Point3(-3.97723, -3.87339, 4.13608));
		staticMesh[OMNI_MESH].setVert(217, Point3(-3.30779, -3.20396, 1.25256));
		staticMesh[OMNI_MESH].setVert(218, Point3(-2.26066, -2.15682, -0.475118));
		staticMesh[OMNI_MESH].setVert(219, Point3(-2.5285, -2.42466, -0.748603));
		staticMesh[OMNI_MESH].setVert(220, Point3(-2.5285, -2.42466, -2.31931));
		staticMesh[OMNI_MESH].setVert(221, Point3(-2.28167, -2.17783, -3.41008));
		staticMesh[OMNI_MESH].setVert(222, Point3(-2.28167, -2.17783, -7.12286));
		staticMesh[OMNI_MESH].setVert(223, Point3(-1.01676, -0.91292, -8.99898));
		staticMesh[OMNI_MESH].setVert(224, Point3(1.94958e-07, 0.103837, -8.99898));
		staticMesh[OMNI_MESH].setVert(225, Point3(-1.0342e-05, 0.103817, 24.7909));
		staticMesh[OMNI_MESH].setVert(226, Point3(-1.43443, -3.35917, 24.0428));
		staticMesh[OMNI_MESH].setVert(227, Point3(-2.57379, -6.10984, 22.3999));
		staticMesh[OMNI_MESH].setVert(228, Point3(-3.49478, -8.3333, 19.3158));
		staticMesh[OMNI_MESH].setVert(229, Point3(-4.0208, -9.60323, 14.5684));
		staticMesh[OMNI_MESH].setVert(230, Point3(-3.66174, -8.73639, 9.28082));
		staticMesh[OMNI_MESH].setVert(231, Point3(-2.15246, -5.09266, 4.13608));
		staticMesh[OMNI_MESH].setVert(232, Point3(-1.79017, -4.218, 1.25256));
		staticMesh[OMNI_MESH].setVert(233, Point3(-1.22346, -2.84986, -0.475118));
		staticMesh[OMNI_MESH].setVert(234, Point3(-1.36841, -3.19981, -0.748603));
		staticMesh[OMNI_MESH].setVert(235, Point3(-1.36841, -3.19981, -2.31931));
		staticMesh[OMNI_MESH].setVert(236, Point3(-1.23483, -2.8773, -3.41007));
		staticMesh[OMNI_MESH].setVert(237, Point3(-1.23483, -2.8773, -7.12286));
		staticMesh[OMNI_MESH].setVert(238, Point3(-0.550265, -1.22462, -8.99898));
		staticMesh[OMNI_MESH].setVert(239, Point3(7.91004e-07, 0.103838, -8.99898));
		staticMesh[OMNI_MESH].setVert(240, Point3(-14.8412, -0.846332, 27.8536));
		staticMesh[OMNI_MESH].setVert(241, Point3(-31.5537, -0.846331, 44.5661));
		staticMesh[OMNI_MESH].setVert(242, Point3(-15.3626, -0.846332, 27.3322));
		staticMesh[OMNI_MESH].setVert(243, Point3(-32.0751, -0.846331, 44.0447));
		staticMesh[OMNI_MESH].setVert(244, Point3(-0.749999, -15.447, 27.0037));
		staticMesh[OMNI_MESH].setVert(245, Point3(-0.750007, -32.1595, 43.7162));
		staticMesh[OMNI_MESH].setVert(246, Point3(-0.749999, -14.9256, 27.5251));
		staticMesh[OMNI_MESH].setVert(247, Point3(-0.750007, -31.6381, 44.2376));
		staticMesh[OMNI_MESH].setVert(248, Point3(-20.282, -0.84633, 8.9451));
		staticMesh[OMNI_MESH].setVert(249, Point3(-43.917, -0.846329, 8.9451));
		staticMesh[OMNI_MESH].setVert(250, Point3(-20.282, -0.84633, 8.20774));
		staticMesh[OMNI_MESH].setVert(251, Point3(-43.917, -0.846329, 8.20775));
		staticMesh[OMNI_MESH].setVert(252, Point3(-0.750003, -20.3903, 8.17623));
		staticMesh[OMNI_MESH].setVert(253, Point3(-0.750009, -44.0253, 8.17623));
		staticMesh[OMNI_MESH].setVert(254, Point3(-0.750001, -20.3903, 8.91358));
		staticMesh[OMNI_MESH].setVert(255, Point3(-0.750009, -44.0253, 8.91359));
		staticMesh[OMNI_MESH].setVert(256, Point3(-0.749995, -0.406668, 30.0227));
		staticMesh[OMNI_MESH].setVert(257, Point3(-0.749998, -0.40667, 53.6577));
		staticMesh[OMNI_MESH].setVert(258, Point3(-0.749997, 0.330688, 30.0227));
		staticMesh[OMNI_MESH].setVert(259, Point3(-0.749998, 0.330687, 53.6577));
		staticMesh[OMNI_MESH].setVert(260, Point3(-0.750002, 14.7448, 27.8536));
		staticMesh[OMNI_MESH].setVert(261, Point3(-0.75, 31.4573, 44.5661));
		staticMesh[OMNI_MESH].setVert(262, Point3(-0.750002, 15.2662, 27.3322));
		staticMesh[OMNI_MESH].setVert(263, Point3(-0.75, 31.9787, 44.0447));
		staticMesh[OMNI_MESH].setVert(264, Point3(20.2939, -0.846331, 8.17622));
		staticMesh[OMNI_MESH].setVert(265, Point3(43.9289, -0.846335, 8.17622));
		staticMesh[OMNI_MESH].setVert(266, Point3(20.2939, -0.846331, 8.91358));
		staticMesh[OMNI_MESH].setVert(267, Point3(43.9289, -0.846335, 8.91358));
		staticMesh[OMNI_MESH].setVert(268, Point3(-0.750002, 20.1856, 8.9451));
		staticMesh[OMNI_MESH].setVert(269, Point3(-0.750002, 43.8206, 8.9451));
		staticMesh[OMNI_MESH].setVert(270, Point3(-0.750004, 20.1856, 8.20774));
		staticMesh[OMNI_MESH].setVert(271, Point3(-0.750002, 43.8206, 8.20775));
		staticMesh[OMNI_MESH].setVert(272, Point3(15.3506, -0.84633, 27.0037));
		staticMesh[OMNI_MESH].setVert(273, Point3(32.063, -0.846334, 43.7162));
		staticMesh[OMNI_MESH].setVert(274, Point3(14.8292, -0.84633, 27.5251));
		staticMesh[OMNI_MESH].setVert(275, Point3(31.5417, -0.846334, 44.2376));
		staticMesh[OMNI_MESH].setVert(276, Point3(-14.8412, -0.0963316, 27.8536));
		staticMesh[OMNI_MESH].setVert(277, Point3(-31.5537, -0.0963311, 44.5661));
		staticMesh[OMNI_MESH].setVert(278, Point3(-15.3626, -0.0963316, 27.3322));
		staticMesh[OMNI_MESH].setVert(279, Point3(-32.0751, -0.0963311, 44.0447));
		staticMesh[OMNI_MESH].setVert(280, Point3(6.17634e-07, -15.447, 27.0037));
		staticMesh[OMNI_MESH].setVert(281, Point3(-4.74305e-06, -32.1595, 43.7162));
		staticMesh[OMNI_MESH].setVert(282, Point3(6.79788e-07, -14.9256, 27.5251));
		staticMesh[OMNI_MESH].setVert(283, Point3(-4.6809e-06, -31.6381, 44.2376));
		staticMesh[OMNI_MESH].setVert(284, Point3(-20.282, -0.0963297, 8.9451));
		staticMesh[OMNI_MESH].setVert(285, Point3(-43.917, -0.0963287, 8.9451));
		staticMesh[OMNI_MESH].setVert(286, Point3(-20.282, -0.0963297, 8.20774));
		staticMesh[OMNI_MESH].setVert(287, Point3(-43.917, -0.0963287, 8.20775));
		staticMesh[OMNI_MESH].setVert(288, Point3(-3.17946e-06, -20.3903, 8.17623));
		staticMesh[OMNI_MESH].setVert(289, Point3(-8.93744e-06, -44.0253, 8.17623));
		staticMesh[OMNI_MESH].setVert(290, Point3(-1.30434e-06, -20.3903, 8.91358));
		staticMesh[OMNI_MESH].setVert(291, Point3(-7.06233e-06, -44.0253, 8.91359));
		staticMesh[OMNI_MESH].setVert(292, Point3(4.8434e-06, -0.406669, 30.0227));
		staticMesh[OMNI_MESH].setVert(293, Point3(1.90293e-06, -0.40667, 53.6577));
		staticMesh[OMNI_MESH].setVert(294, Point3(4.96353e-06, 0.330688, 30.0227));
		staticMesh[OMNI_MESH].setVert(295, Point3(2.02306e-06, 0.330687, 53.6577));
		staticMesh[OMNI_MESH].setVert(296, Point3(-2.13004e-06, 14.7448, 27.8536));
		staticMesh[OMNI_MESH].setVert(297, Point3(-1.37759e-07, 31.4573, 44.5661));
		staticMesh[OMNI_MESH].setVert(298, Point3(-2.02231e-06, 15.2662, 27.3322));
		staticMesh[OMNI_MESH].setVert(299, Point3(0.0, 31.9787, 44.0447));
		staticMesh[OMNI_MESH].setVert(300, Point3(20.2939, -0.0963311, 8.17622));
		staticMesh[OMNI_MESH].setVert(301, Point3(43.9289, -0.0963354, 8.17622));
		staticMesh[OMNI_MESH].setVert(302, Point3(20.2939, -0.0963311, 8.91358));
		staticMesh[OMNI_MESH].setVert(303, Point3(43.9289, -0.0963354, 8.91358));
		staticMesh[OMNI_MESH].setVert(304, Point3(-2.32446e-06, 20.1856, 8.9451));
		staticMesh[OMNI_MESH].setVert(305, Point3(-2.28853e-06, 43.8206, 8.9451));
		staticMesh[OMNI_MESH].setVert(306, Point3(-4.19958e-06, 20.1856, 8.20774));
		staticMesh[OMNI_MESH].setVert(307, Point3(-2.2563e-06, 43.8206, 8.20775));
		staticMesh[OMNI_MESH].setVert(308, Point3(15.3506, -0.0963297, 27.0037));
		staticMesh[OMNI_MESH].setVert(309, Point3(32.063, -0.0963335, 43.7162));
		staticMesh[OMNI_MESH].setVert(310, Point3(14.8292, -0.0963297, 27.5251));
		staticMesh[OMNI_MESH].setVert(311, Point3(31.5417, -0.0963335, 44.2376));
		staticMesh[OMNI_MESH].faces[0].setVerts(16, 15, 0);		staticMesh[OMNI_MESH].faces[0].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[0].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[1].setVerts(0, 1, 16);		staticMesh[OMNI_MESH].faces[1].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[1].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[2].setVerts(17, 16, 1);		staticMesh[OMNI_MESH].faces[2].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[2].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[3].setVerts(1, 2, 17);		staticMesh[OMNI_MESH].faces[3].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[3].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[4].setVerts(18, 17, 2);		staticMesh[OMNI_MESH].faces[4].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[4].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[5].setVerts(2, 3, 18);		staticMesh[OMNI_MESH].faces[5].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[5].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[6].setVerts(19, 18, 3);		staticMesh[OMNI_MESH].faces[6].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[6].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[7].setVerts(3, 4, 19);		staticMesh[OMNI_MESH].faces[7].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[7].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[8].setVerts(20, 19, 4);		staticMesh[OMNI_MESH].faces[8].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[8].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[9].setVerts(4, 5, 20);		staticMesh[OMNI_MESH].faces[9].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[9].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[10].setVerts(21, 20, 5);	staticMesh[OMNI_MESH].faces[10].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[10].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[11].setVerts(5, 6, 21);		staticMesh[OMNI_MESH].faces[11].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[11].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[12].setVerts(22, 21, 6);	staticMesh[OMNI_MESH].faces[12].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[12].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[13].setVerts(6, 7, 22);		staticMesh[OMNI_MESH].faces[13].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[13].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[14].setVerts(23, 22, 7);	staticMesh[OMNI_MESH].faces[14].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[14].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[15].setVerts(7, 8, 23);		staticMesh[OMNI_MESH].faces[15].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[15].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[16].setVerts(24, 23, 8);	staticMesh[OMNI_MESH].faces[16].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[16].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[17].setVerts(8, 9, 24);		staticMesh[OMNI_MESH].faces[17].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[17].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[18].setVerts(25, 24, 9);	staticMesh[OMNI_MESH].faces[18].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[18].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[19].setVerts(9, 10, 25);	staticMesh[OMNI_MESH].faces[19].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[19].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[20].setVerts(26, 25, 10);	staticMesh[OMNI_MESH].faces[20].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[20].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[21].setVerts(10, 11, 26);	staticMesh[OMNI_MESH].faces[21].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[21].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[22].setVerts(27, 26, 11);	staticMesh[OMNI_MESH].faces[22].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[22].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[23].setVerts(11, 12, 27);	staticMesh[OMNI_MESH].faces[23].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[23].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[24].setVerts(28, 27, 12);	staticMesh[OMNI_MESH].faces[24].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[24].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[25].setVerts(12, 13, 28);	staticMesh[OMNI_MESH].faces[25].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[25].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[26].setVerts(29, 28, 13);	staticMesh[OMNI_MESH].faces[26].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[26].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[27].setVerts(13, 14, 29);	staticMesh[OMNI_MESH].faces[27].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[27].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[28].setVerts(31, 30, 15);	staticMesh[OMNI_MESH].faces[28].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[28].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[29].setVerts(15, 16, 31);	staticMesh[OMNI_MESH].faces[29].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[29].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[30].setVerts(32, 31, 16);	staticMesh[OMNI_MESH].faces[30].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[30].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[31].setVerts(16, 17, 32);	staticMesh[OMNI_MESH].faces[31].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[31].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[32].setVerts(33, 32, 17);	staticMesh[OMNI_MESH].faces[32].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[32].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[33].setVerts(17, 18, 33);	staticMesh[OMNI_MESH].faces[33].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[33].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[34].setVerts(34, 33, 18);	staticMesh[OMNI_MESH].faces[34].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[34].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[35].setVerts(18, 19, 34);	staticMesh[OMNI_MESH].faces[35].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[35].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[36].setVerts(35, 34, 19);	staticMesh[OMNI_MESH].faces[36].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[36].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[37].setVerts(19, 20, 35);	staticMesh[OMNI_MESH].faces[37].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[37].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[38].setVerts(36, 35, 20);	staticMesh[OMNI_MESH].faces[38].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[38].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[39].setVerts(20, 21, 36);	staticMesh[OMNI_MESH].faces[39].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[39].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[40].setVerts(37, 36, 21);	staticMesh[OMNI_MESH].faces[40].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[40].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[41].setVerts(21, 22, 37);	staticMesh[OMNI_MESH].faces[41].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[41].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[42].setVerts(38, 37, 22);	staticMesh[OMNI_MESH].faces[42].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[42].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[43].setVerts(22, 23, 38);	staticMesh[OMNI_MESH].faces[43].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[43].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[44].setVerts(39, 38, 23);	staticMesh[OMNI_MESH].faces[44].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[44].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[45].setVerts(23, 24, 39);	staticMesh[OMNI_MESH].faces[45].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[45].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[46].setVerts(40, 39, 24);	staticMesh[OMNI_MESH].faces[46].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[46].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[47].setVerts(24, 25, 40);	staticMesh[OMNI_MESH].faces[47].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[47].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[48].setVerts(41, 40, 25);	staticMesh[OMNI_MESH].faces[48].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[48].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[49].setVerts(25, 26, 41);	staticMesh[OMNI_MESH].faces[49].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[49].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[50].setVerts(42, 41, 26);	staticMesh[OMNI_MESH].faces[50].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[50].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[51].setVerts(26, 27, 42);	staticMesh[OMNI_MESH].faces[51].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[51].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[52].setVerts(43, 42, 27);	staticMesh[OMNI_MESH].faces[52].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[52].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[53].setVerts(27, 28, 43);	staticMesh[OMNI_MESH].faces[53].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[53].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[54].setVerts(44, 43, 28);	staticMesh[OMNI_MESH].faces[54].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[54].setSmGroup(9);
		staticMesh[OMNI_MESH].faces[55].setVerts(28, 29, 44);	staticMesh[OMNI_MESH].faces[55].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[55].setSmGroup(9);
		staticMesh[OMNI_MESH].faces[56].setVerts(46, 45, 30);	staticMesh[OMNI_MESH].faces[56].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[56].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[57].setVerts(30, 31, 46);	staticMesh[OMNI_MESH].faces[57].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[57].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[58].setVerts(47, 46, 31);	staticMesh[OMNI_MESH].faces[58].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[58].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[59].setVerts(31, 32, 47);	staticMesh[OMNI_MESH].faces[59].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[59].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[60].setVerts(48, 47, 32);	staticMesh[OMNI_MESH].faces[60].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[60].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[61].setVerts(32, 33, 48);	staticMesh[OMNI_MESH].faces[61].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[61].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[62].setVerts(49, 48, 33);	staticMesh[OMNI_MESH].faces[62].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[62].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[63].setVerts(33, 34, 49);	staticMesh[OMNI_MESH].faces[63].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[63].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[64].setVerts(50, 49, 34);	staticMesh[OMNI_MESH].faces[64].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[64].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[65].setVerts(34, 35, 50);	staticMesh[OMNI_MESH].faces[65].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[65].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[66].setVerts(51, 50, 35);	staticMesh[OMNI_MESH].faces[66].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[66].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[67].setVerts(35, 36, 51);	staticMesh[OMNI_MESH].faces[67].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[67].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[68].setVerts(52, 51, 36);	staticMesh[OMNI_MESH].faces[68].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[68].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[69].setVerts(36, 37, 52);	staticMesh[OMNI_MESH].faces[69].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[69].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[70].setVerts(53, 52, 37);	staticMesh[OMNI_MESH].faces[70].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[70].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[71].setVerts(37, 38, 53);	staticMesh[OMNI_MESH].faces[71].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[71].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[72].setVerts(54, 53, 38);	staticMesh[OMNI_MESH].faces[72].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[72].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[73].setVerts(38, 39, 54);	staticMesh[OMNI_MESH].faces[73].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[73].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[74].setVerts(55, 54, 39);	staticMesh[OMNI_MESH].faces[74].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[74].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[75].setVerts(39, 40, 55);	staticMesh[OMNI_MESH].faces[75].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[75].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[76].setVerts(56, 55, 40);	staticMesh[OMNI_MESH].faces[76].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[76].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[77].setVerts(40, 41, 56);	staticMesh[OMNI_MESH].faces[77].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[77].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[78].setVerts(57, 56, 41);	staticMesh[OMNI_MESH].faces[78].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[78].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[79].setVerts(41, 42, 57);	staticMesh[OMNI_MESH].faces[79].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[79].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[80].setVerts(58, 57, 42);	staticMesh[OMNI_MESH].faces[80].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[80].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[81].setVerts(42, 43, 58);	staticMesh[OMNI_MESH].faces[81].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[81].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[82].setVerts(59, 58, 43);	staticMesh[OMNI_MESH].faces[82].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[82].setSmGroup(12);
		staticMesh[OMNI_MESH].faces[83].setVerts(43, 44, 59);	staticMesh[OMNI_MESH].faces[83].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[83].setSmGroup(12);
		staticMesh[OMNI_MESH].faces[84].setVerts(61, 60, 45);	staticMesh[OMNI_MESH].faces[84].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[84].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[85].setVerts(45, 46, 61);	staticMesh[OMNI_MESH].faces[85].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[85].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[86].setVerts(62, 61, 46);	staticMesh[OMNI_MESH].faces[86].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[86].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[87].setVerts(46, 47, 62);	staticMesh[OMNI_MESH].faces[87].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[87].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[88].setVerts(63, 62, 47);	staticMesh[OMNI_MESH].faces[88].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[88].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[89].setVerts(47, 48, 63);	staticMesh[OMNI_MESH].faces[89].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[89].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[90].setVerts(64, 63, 48);	staticMesh[OMNI_MESH].faces[90].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[90].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[91].setVerts(48, 49, 64);	staticMesh[OMNI_MESH].faces[91].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[91].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[92].setVerts(65, 64, 49);	staticMesh[OMNI_MESH].faces[92].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[92].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[93].setVerts(49, 50, 65);	staticMesh[OMNI_MESH].faces[93].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[93].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[94].setVerts(66, 65, 50);	staticMesh[OMNI_MESH].faces[94].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[94].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[95].setVerts(50, 51, 66);	staticMesh[OMNI_MESH].faces[95].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[95].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[96].setVerts(67, 66, 51);	staticMesh[OMNI_MESH].faces[96].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[96].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[97].setVerts(51, 52, 67);	staticMesh[OMNI_MESH].faces[97].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[97].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[98].setVerts(68, 67, 52);	staticMesh[OMNI_MESH].faces[98].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[98].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[99].setVerts(52, 53, 68);	staticMesh[OMNI_MESH].faces[99].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[99].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[100].setVerts(69, 68, 53);	staticMesh[OMNI_MESH].faces[100].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[100].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[101].setVerts(53, 54, 69);	staticMesh[OMNI_MESH].faces[101].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[101].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[102].setVerts(70, 69, 54);	staticMesh[OMNI_MESH].faces[102].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[102].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[103].setVerts(54, 55, 70);	staticMesh[OMNI_MESH].faces[103].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[103].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[104].setVerts(71, 70, 55);	staticMesh[OMNI_MESH].faces[104].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[104].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[105].setVerts(55, 56, 71);	staticMesh[OMNI_MESH].faces[105].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[105].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[106].setVerts(72, 71, 56);	staticMesh[OMNI_MESH].faces[106].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[106].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[107].setVerts(56, 57, 72);	staticMesh[OMNI_MESH].faces[107].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[107].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[108].setVerts(73, 72, 57);	staticMesh[OMNI_MESH].faces[108].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[108].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[109].setVerts(57, 58, 73);	staticMesh[OMNI_MESH].faces[109].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[109].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[110].setVerts(74, 73, 58);	staticMesh[OMNI_MESH].faces[110].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[110].setSmGroup(8);
		staticMesh[OMNI_MESH].faces[111].setVerts(58, 59, 74);	staticMesh[OMNI_MESH].faces[111].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[111].setSmGroup(8);
		staticMesh[OMNI_MESH].faces[112].setVerts(76, 75, 60);	staticMesh[OMNI_MESH].faces[112].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[112].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[113].setVerts(60, 61, 76);	staticMesh[OMNI_MESH].faces[113].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[113].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[114].setVerts(77, 76, 61);	staticMesh[OMNI_MESH].faces[114].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[114].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[115].setVerts(61, 62, 77);	staticMesh[OMNI_MESH].faces[115].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[115].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[116].setVerts(78, 77, 62);	staticMesh[OMNI_MESH].faces[116].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[116].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[117].setVerts(62, 63, 78);	staticMesh[OMNI_MESH].faces[117].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[117].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[118].setVerts(79, 78, 63);	staticMesh[OMNI_MESH].faces[118].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[118].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[119].setVerts(63, 64, 79);	staticMesh[OMNI_MESH].faces[119].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[119].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[120].setVerts(80, 79, 64);	staticMesh[OMNI_MESH].faces[120].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[120].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[121].setVerts(64, 65, 80);	staticMesh[OMNI_MESH].faces[121].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[121].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[122].setVerts(81, 80, 65);	staticMesh[OMNI_MESH].faces[122].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[122].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[123].setVerts(65, 66, 81);	staticMesh[OMNI_MESH].faces[123].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[123].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[124].setVerts(82, 81, 66);	staticMesh[OMNI_MESH].faces[124].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[124].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[125].setVerts(66, 67, 82);	staticMesh[OMNI_MESH].faces[125].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[125].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[126].setVerts(83, 82, 67);	staticMesh[OMNI_MESH].faces[126].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[126].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[127].setVerts(67, 68, 83);	staticMesh[OMNI_MESH].faces[127].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[127].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[128].setVerts(84, 83, 68);	staticMesh[OMNI_MESH].faces[128].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[128].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[129].setVerts(68, 69, 84);	staticMesh[OMNI_MESH].faces[129].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[129].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[130].setVerts(85, 84, 69);	staticMesh[OMNI_MESH].faces[130].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[130].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[131].setVerts(69, 70, 85);	staticMesh[OMNI_MESH].faces[131].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[131].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[132].setVerts(86, 85, 70);	staticMesh[OMNI_MESH].faces[132].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[132].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[133].setVerts(70, 71, 86);	staticMesh[OMNI_MESH].faces[133].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[133].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[134].setVerts(87, 86, 71);	staticMesh[OMNI_MESH].faces[134].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[134].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[135].setVerts(71, 72, 87);	staticMesh[OMNI_MESH].faces[135].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[135].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[136].setVerts(88, 87, 72);	staticMesh[OMNI_MESH].faces[136].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[136].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[137].setVerts(72, 73, 88);	staticMesh[OMNI_MESH].faces[137].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[137].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[138].setVerts(89, 88, 73);	staticMesh[OMNI_MESH].faces[138].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[138].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[139].setVerts(73, 74, 89);	staticMesh[OMNI_MESH].faces[139].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[139].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[140].setVerts(91, 90, 75);	staticMesh[OMNI_MESH].faces[140].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[140].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[141].setVerts(75, 76, 91);	staticMesh[OMNI_MESH].faces[141].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[141].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[142].setVerts(92, 91, 76);	staticMesh[OMNI_MESH].faces[142].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[142].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[143].setVerts(76, 77, 92);	staticMesh[OMNI_MESH].faces[143].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[143].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[144].setVerts(93, 92, 77);	staticMesh[OMNI_MESH].faces[144].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[144].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[145].setVerts(77, 78, 93);	staticMesh[OMNI_MESH].faces[145].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[145].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[146].setVerts(94, 93, 78);	staticMesh[OMNI_MESH].faces[146].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[146].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[147].setVerts(78, 79, 94);	staticMesh[OMNI_MESH].faces[147].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[147].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[148].setVerts(95, 94, 79);	staticMesh[OMNI_MESH].faces[148].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[148].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[149].setVerts(79, 80, 95);	staticMesh[OMNI_MESH].faces[149].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[149].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[150].setVerts(96, 95, 80);	staticMesh[OMNI_MESH].faces[150].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[150].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[151].setVerts(80, 81, 96);	staticMesh[OMNI_MESH].faces[151].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[151].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[152].setVerts(97, 96, 81);	staticMesh[OMNI_MESH].faces[152].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[152].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[153].setVerts(81, 82, 97);	staticMesh[OMNI_MESH].faces[153].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[153].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[154].setVerts(98, 97, 82);	staticMesh[OMNI_MESH].faces[154].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[154].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[155].setVerts(82, 83, 98);	staticMesh[OMNI_MESH].faces[155].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[155].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[156].setVerts(99, 98, 83);	staticMesh[OMNI_MESH].faces[156].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[156].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[157].setVerts(83, 84, 99);	staticMesh[OMNI_MESH].faces[157].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[157].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[158].setVerts(100, 99, 84);	staticMesh[OMNI_MESH].faces[158].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[158].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[159].setVerts(84, 85, 100);	staticMesh[OMNI_MESH].faces[159].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[159].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[160].setVerts(101, 100, 85);	staticMesh[OMNI_MESH].faces[160].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[160].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[161].setVerts(85, 86, 101);		staticMesh[OMNI_MESH].faces[161].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[161].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[162].setVerts(102, 101, 86);	staticMesh[OMNI_MESH].faces[162].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[162].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[163].setVerts(86, 87, 102);		staticMesh[OMNI_MESH].faces[163].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[163].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[164].setVerts(103, 102, 87);	staticMesh[OMNI_MESH].faces[164].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[164].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[165].setVerts(87, 88, 103);		staticMesh[OMNI_MESH].faces[165].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[165].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[166].setVerts(104, 103, 88);	staticMesh[OMNI_MESH].faces[166].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[166].setSmGroup(9);
		staticMesh[OMNI_MESH].faces[167].setVerts(88, 89, 104);		staticMesh[OMNI_MESH].faces[167].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[167].setSmGroup(9);
		staticMesh[OMNI_MESH].faces[168].setVerts(106, 105, 90);	staticMesh[OMNI_MESH].faces[168].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[168].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[169].setVerts(90, 91, 106);		staticMesh[OMNI_MESH].faces[169].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[169].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[170].setVerts(107, 106, 91);	staticMesh[OMNI_MESH].faces[170].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[170].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[171].setVerts(91, 92, 107);		staticMesh[OMNI_MESH].faces[171].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[171].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[172].setVerts(108, 107, 92);	staticMesh[OMNI_MESH].faces[172].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[172].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[173].setVerts(92, 93, 108);		staticMesh[OMNI_MESH].faces[173].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[173].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[174].setVerts(109, 108, 93);	staticMesh[OMNI_MESH].faces[174].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[174].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[175].setVerts(93, 94, 109);		staticMesh[OMNI_MESH].faces[175].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[175].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[176].setVerts(110, 109, 94);	staticMesh[OMNI_MESH].faces[176].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[176].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[177].setVerts(94, 95, 110);		staticMesh[OMNI_MESH].faces[177].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[177].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[178].setVerts(111, 110, 95);	staticMesh[OMNI_MESH].faces[178].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[178].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[179].setVerts(95, 96, 111);		staticMesh[OMNI_MESH].faces[179].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[179].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[180].setVerts(112, 111, 96);	staticMesh[OMNI_MESH].faces[180].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[180].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[181].setVerts(96, 97, 112);		staticMesh[OMNI_MESH].faces[181].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[181].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[182].setVerts(113, 112, 97);	staticMesh[OMNI_MESH].faces[182].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[182].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[183].setVerts(97, 98, 113);		staticMesh[OMNI_MESH].faces[183].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[183].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[184].setVerts(114, 113, 98);	staticMesh[OMNI_MESH].faces[184].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[184].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[185].setVerts(98, 99, 114);		staticMesh[OMNI_MESH].faces[185].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[185].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[186].setVerts(115, 114, 99);	staticMesh[OMNI_MESH].faces[186].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[186].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[187].setVerts(99, 100, 115);	staticMesh[OMNI_MESH].faces[187].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[187].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[188].setVerts(116, 115, 100);	staticMesh[OMNI_MESH].faces[188].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[188].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[189].setVerts(100, 101, 116);	staticMesh[OMNI_MESH].faces[189].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[189].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[190].setVerts(117, 116, 101);	staticMesh[OMNI_MESH].faces[190].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[190].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[191].setVerts(101, 102, 117);	staticMesh[OMNI_MESH].faces[191].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[191].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[192].setVerts(118, 117, 102);	staticMesh[OMNI_MESH].faces[192].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[192].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[193].setVerts(102, 103, 118);	staticMesh[OMNI_MESH].faces[193].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[193].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[194].setVerts(119, 118, 103);	staticMesh[OMNI_MESH].faces[194].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[194].setSmGroup(12);
		staticMesh[OMNI_MESH].faces[195].setVerts(103, 104, 119);	staticMesh[OMNI_MESH].faces[195].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[195].setSmGroup(12);
		staticMesh[OMNI_MESH].faces[196].setVerts(121, 120, 105);	staticMesh[OMNI_MESH].faces[196].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[196].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[197].setVerts(105, 106, 121);	staticMesh[OMNI_MESH].faces[197].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[197].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[198].setVerts(122, 121, 106);	staticMesh[OMNI_MESH].faces[198].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[198].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[199].setVerts(106, 107, 122);	staticMesh[OMNI_MESH].faces[199].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[199].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[200].setVerts(123, 122, 107);	staticMesh[OMNI_MESH].faces[200].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[200].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[201].setVerts(107, 108, 123);	staticMesh[OMNI_MESH].faces[201].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[201].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[202].setVerts(124, 123, 108);	staticMesh[OMNI_MESH].faces[202].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[202].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[203].setVerts(108, 109, 124);	staticMesh[OMNI_MESH].faces[203].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[203].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[204].setVerts(125, 124, 109);	staticMesh[OMNI_MESH].faces[204].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[204].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[205].setVerts(109, 110, 125);	staticMesh[OMNI_MESH].faces[205].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[205].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[206].setVerts(126, 125, 110);	staticMesh[OMNI_MESH].faces[206].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[206].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[207].setVerts(110, 111, 126);	staticMesh[OMNI_MESH].faces[207].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[207].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[208].setVerts(127, 126, 111);	staticMesh[OMNI_MESH].faces[208].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[208].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[209].setVerts(111, 112, 127);	staticMesh[OMNI_MESH].faces[209].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[209].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[210].setVerts(128, 127, 112);	staticMesh[OMNI_MESH].faces[210].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[210].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[211].setVerts(112, 113, 128);	staticMesh[OMNI_MESH].faces[211].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[211].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[212].setVerts(129, 128, 113);	staticMesh[OMNI_MESH].faces[212].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[212].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[213].setVerts(113, 114, 129);	staticMesh[OMNI_MESH].faces[213].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[213].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[214].setVerts(130, 129, 114);	staticMesh[OMNI_MESH].faces[214].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[214].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[215].setVerts(114, 115, 130);	staticMesh[OMNI_MESH].faces[215].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[215].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[216].setVerts(131, 130, 115);	staticMesh[OMNI_MESH].faces[216].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[216].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[217].setVerts(115, 116, 131);	staticMesh[OMNI_MESH].faces[217].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[217].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[218].setVerts(132, 131, 116);	staticMesh[OMNI_MESH].faces[218].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[218].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[219].setVerts(116, 117, 132);	staticMesh[OMNI_MESH].faces[219].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[219].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[220].setVerts(133, 132, 117);	staticMesh[OMNI_MESH].faces[220].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[220].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[221].setVerts(117, 118, 133);	staticMesh[OMNI_MESH].faces[221].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[221].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[222].setVerts(134, 133, 118);	staticMesh[OMNI_MESH].faces[222].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[222].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[223].setVerts(118, 119, 134);	staticMesh[OMNI_MESH].faces[223].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[223].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[224].setVerts(136, 135, 120);	staticMesh[OMNI_MESH].faces[224].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[224].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[225].setVerts(120, 121, 136);	staticMesh[OMNI_MESH].faces[225].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[225].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[226].setVerts(137, 136, 121);	staticMesh[OMNI_MESH].faces[226].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[226].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[227].setVerts(121, 122, 137);	staticMesh[OMNI_MESH].faces[227].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[227].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[228].setVerts(138, 137, 122);	staticMesh[OMNI_MESH].faces[228].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[228].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[229].setVerts(122, 123, 138);	staticMesh[OMNI_MESH].faces[229].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[229].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[230].setVerts(139, 138, 123);	staticMesh[OMNI_MESH].faces[230].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[230].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[231].setVerts(123, 124, 139);	staticMesh[OMNI_MESH].faces[231].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[231].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[232].setVerts(140, 139, 124);	staticMesh[OMNI_MESH].faces[232].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[232].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[233].setVerts(124, 125, 140);	staticMesh[OMNI_MESH].faces[233].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[233].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[234].setVerts(141, 140, 125);	staticMesh[OMNI_MESH].faces[234].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[234].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[235].setVerts(125, 126, 141);	staticMesh[OMNI_MESH].faces[235].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[235].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[236].setVerts(142, 141, 126);	staticMesh[OMNI_MESH].faces[236].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[236].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[237].setVerts(126, 127, 142);	staticMesh[OMNI_MESH].faces[237].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[237].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[238].setVerts(143, 142, 127);	staticMesh[OMNI_MESH].faces[238].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[238].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[239].setVerts(127, 128, 143);	staticMesh[OMNI_MESH].faces[239].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[239].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[240].setVerts(144, 143, 128);	staticMesh[OMNI_MESH].faces[240].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[240].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[241].setVerts(128, 129, 144);	staticMesh[OMNI_MESH].faces[241].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[241].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[242].setVerts(145, 144, 129);	staticMesh[OMNI_MESH].faces[242].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[242].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[243].setVerts(129, 130, 145);	staticMesh[OMNI_MESH].faces[243].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[243].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[244].setVerts(146, 145, 130);	staticMesh[OMNI_MESH].faces[244].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[244].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[245].setVerts(130, 131, 146);	staticMesh[OMNI_MESH].faces[245].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[245].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[246].setVerts(147, 146, 131);	staticMesh[OMNI_MESH].faces[246].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[246].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[247].setVerts(131, 132, 147);	staticMesh[OMNI_MESH].faces[247].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[247].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[248].setVerts(148, 147, 132);	staticMesh[OMNI_MESH].faces[248].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[248].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[249].setVerts(132, 133, 148);	staticMesh[OMNI_MESH].faces[249].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[249].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[250].setVerts(149, 148, 133);	staticMesh[OMNI_MESH].faces[250].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[250].setSmGroup(9);
		staticMesh[OMNI_MESH].faces[251].setVerts(133, 134, 149);	staticMesh[OMNI_MESH].faces[251].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[251].setSmGroup(9);
		staticMesh[OMNI_MESH].faces[252].setVerts(151, 150, 135);	staticMesh[OMNI_MESH].faces[252].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[252].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[253].setVerts(135, 136, 151);	staticMesh[OMNI_MESH].faces[253].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[253].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[254].setVerts(152, 151, 136);	staticMesh[OMNI_MESH].faces[254].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[254].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[255].setVerts(136, 137, 152);	staticMesh[OMNI_MESH].faces[255].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[255].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[256].setVerts(153, 152, 137);	staticMesh[OMNI_MESH].faces[256].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[256].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[257].setVerts(137, 138, 153);	staticMesh[OMNI_MESH].faces[257].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[257].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[258].setVerts(154, 153, 138);	staticMesh[OMNI_MESH].faces[258].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[258].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[259].setVerts(138, 139, 154);	staticMesh[OMNI_MESH].faces[259].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[259].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[260].setVerts(155, 154, 139);	staticMesh[OMNI_MESH].faces[260].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[260].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[261].setVerts(139, 140, 155);	staticMesh[OMNI_MESH].faces[261].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[261].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[262].setVerts(156, 155, 140);	staticMesh[OMNI_MESH].faces[262].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[262].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[263].setVerts(140, 141, 156);	staticMesh[OMNI_MESH].faces[263].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[263].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[264].setVerts(157, 156, 141);	staticMesh[OMNI_MESH].faces[264].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[264].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[265].setVerts(141, 142, 157);	staticMesh[OMNI_MESH].faces[265].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[265].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[266].setVerts(158, 157, 142);	staticMesh[OMNI_MESH].faces[266].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[266].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[267].setVerts(142, 143, 158);	staticMesh[OMNI_MESH].faces[267].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[267].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[268].setVerts(159, 158, 143);	staticMesh[OMNI_MESH].faces[268].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[268].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[269].setVerts(143, 144, 159);	staticMesh[OMNI_MESH].faces[269].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[269].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[270].setVerts(160, 159, 144);	staticMesh[OMNI_MESH].faces[270].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[270].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[271].setVerts(144, 145, 160);	staticMesh[OMNI_MESH].faces[271].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[271].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[272].setVerts(161, 160, 145);	staticMesh[OMNI_MESH].faces[272].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[272].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[273].setVerts(145, 146, 161);	staticMesh[OMNI_MESH].faces[273].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[273].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[274].setVerts(162, 161, 146);	staticMesh[OMNI_MESH].faces[274].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[274].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[275].setVerts(146, 147, 162);	staticMesh[OMNI_MESH].faces[275].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[275].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[276].setVerts(163, 162, 147);	staticMesh[OMNI_MESH].faces[276].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[276].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[277].setVerts(147, 148, 163);	staticMesh[OMNI_MESH].faces[277].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[277].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[278].setVerts(164, 163, 148);	staticMesh[OMNI_MESH].faces[278].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[278].setSmGroup(12);
		staticMesh[OMNI_MESH].faces[279].setVerts(148, 149, 164);	staticMesh[OMNI_MESH].faces[279].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[279].setSmGroup(12);
		staticMesh[OMNI_MESH].faces[280].setVerts(166, 165, 150);	staticMesh[OMNI_MESH].faces[280].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[280].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[281].setVerts(150, 151, 166);	staticMesh[OMNI_MESH].faces[281].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[281].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[282].setVerts(167, 166, 151);	staticMesh[OMNI_MESH].faces[282].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[282].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[283].setVerts(151, 152, 167);	staticMesh[OMNI_MESH].faces[283].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[283].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[284].setVerts(168, 167, 152);	staticMesh[OMNI_MESH].faces[284].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[284].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[285].setVerts(152, 153, 168);	staticMesh[OMNI_MESH].faces[285].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[285].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[286].setVerts(169, 168, 153);	staticMesh[OMNI_MESH].faces[286].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[286].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[287].setVerts(153, 154, 169);	staticMesh[OMNI_MESH].faces[287].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[287].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[288].setVerts(170, 169, 154);	staticMesh[OMNI_MESH].faces[288].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[288].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[289].setVerts(154, 155, 170);	staticMesh[OMNI_MESH].faces[289].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[289].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[290].setVerts(171, 170, 155);	staticMesh[OMNI_MESH].faces[290].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[290].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[291].setVerts(155, 156, 171);	staticMesh[OMNI_MESH].faces[291].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[291].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[292].setVerts(172, 171, 156);	staticMesh[OMNI_MESH].faces[292].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[292].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[293].setVerts(156, 157, 172);	staticMesh[OMNI_MESH].faces[293].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[293].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[294].setVerts(173, 172, 157);	staticMesh[OMNI_MESH].faces[294].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[294].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[295].setVerts(157, 158, 173);	staticMesh[OMNI_MESH].faces[295].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[295].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[296].setVerts(174, 173, 158);	staticMesh[OMNI_MESH].faces[296].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[296].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[297].setVerts(158, 159, 174);	staticMesh[OMNI_MESH].faces[297].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[297].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[298].setVerts(175, 174, 159);	staticMesh[OMNI_MESH].faces[298].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[298].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[299].setVerts(159, 160, 175);	staticMesh[OMNI_MESH].faces[299].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[299].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[300].setVerts(176, 175, 160);	staticMesh[OMNI_MESH].faces[300].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[300].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[301].setVerts(160, 161, 176);	staticMesh[OMNI_MESH].faces[301].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[301].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[302].setVerts(177, 176, 161);	staticMesh[OMNI_MESH].faces[302].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[302].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[303].setVerts(161, 162, 177);	staticMesh[OMNI_MESH].faces[303].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[303].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[304].setVerts(178, 177, 162);	staticMesh[OMNI_MESH].faces[304].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[304].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[305].setVerts(162, 163, 178);	staticMesh[OMNI_MESH].faces[305].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[305].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[306].setVerts(179, 178, 163);	staticMesh[OMNI_MESH].faces[306].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[306].setSmGroup(8);
		staticMesh[OMNI_MESH].faces[307].setVerts(163, 164, 179);	staticMesh[OMNI_MESH].faces[307].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[307].setSmGroup(8);
		staticMesh[OMNI_MESH].faces[308].setVerts(181, 180, 165);	staticMesh[OMNI_MESH].faces[308].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[308].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[309].setVerts(165, 166, 181);	staticMesh[OMNI_MESH].faces[309].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[309].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[310].setVerts(182, 181, 166);	staticMesh[OMNI_MESH].faces[310].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[310].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[311].setVerts(166, 167, 182);	staticMesh[OMNI_MESH].faces[311].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[311].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[312].setVerts(183, 182, 167);	staticMesh[OMNI_MESH].faces[312].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[312].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[313].setVerts(167, 168, 183);	staticMesh[OMNI_MESH].faces[313].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[313].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[314].setVerts(184, 183, 168);	staticMesh[OMNI_MESH].faces[314].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[314].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[315].setVerts(168, 169, 184);	staticMesh[OMNI_MESH].faces[315].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[315].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[316].setVerts(185, 184, 169);	staticMesh[OMNI_MESH].faces[316].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[316].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[317].setVerts(169, 170, 185);	staticMesh[OMNI_MESH].faces[317].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[317].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[318].setVerts(186, 185, 170);	staticMesh[OMNI_MESH].faces[318].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[318].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[319].setVerts(170, 171, 186);	staticMesh[OMNI_MESH].faces[319].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[319].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[320].setVerts(187, 186, 171);	staticMesh[OMNI_MESH].faces[320].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[320].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[321].setVerts(171, 172, 187);	staticMesh[OMNI_MESH].faces[321].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[321].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[322].setVerts(188, 187, 172);	staticMesh[OMNI_MESH].faces[322].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[322].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[323].setVerts(172, 173, 188);	staticMesh[OMNI_MESH].faces[323].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[323].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[324].setVerts(189, 188, 173);	staticMesh[OMNI_MESH].faces[324].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[324].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[325].setVerts(173, 174, 189);	staticMesh[OMNI_MESH].faces[325].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[325].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[326].setVerts(190, 189, 174);	staticMesh[OMNI_MESH].faces[326].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[326].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[327].setVerts(174, 175, 190);	staticMesh[OMNI_MESH].faces[327].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[327].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[328].setVerts(191, 190, 175);	staticMesh[OMNI_MESH].faces[328].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[328].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[329].setVerts(175, 176, 191);	staticMesh[OMNI_MESH].faces[329].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[329].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[330].setVerts(192, 191, 176);	staticMesh[OMNI_MESH].faces[330].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[330].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[331].setVerts(176, 177, 192);	staticMesh[OMNI_MESH].faces[331].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[331].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[332].setVerts(193, 192, 177);	staticMesh[OMNI_MESH].faces[332].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[332].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[333].setVerts(177, 178, 193);	staticMesh[OMNI_MESH].faces[333].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[333].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[334].setVerts(194, 193, 178);	staticMesh[OMNI_MESH].faces[334].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[334].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[335].setVerts(178, 179, 194);	staticMesh[OMNI_MESH].faces[335].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[335].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[336].setVerts(196, 195, 180);	staticMesh[OMNI_MESH].faces[336].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[336].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[337].setVerts(180, 181, 196);	staticMesh[OMNI_MESH].faces[337].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[337].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[338].setVerts(197, 196, 181);	staticMesh[OMNI_MESH].faces[338].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[338].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[339].setVerts(181, 182, 197);	staticMesh[OMNI_MESH].faces[339].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[339].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[340].setVerts(198, 197, 182);	staticMesh[OMNI_MESH].faces[340].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[340].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[341].setVerts(182, 183, 198);	staticMesh[OMNI_MESH].faces[341].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[341].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[342].setVerts(199, 198, 183);	staticMesh[OMNI_MESH].faces[342].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[342].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[343].setVerts(183, 184, 199);	staticMesh[OMNI_MESH].faces[343].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[343].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[344].setVerts(200, 199, 184);	staticMesh[OMNI_MESH].faces[344].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[344].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[345].setVerts(184, 185, 200);	staticMesh[OMNI_MESH].faces[345].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[345].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[346].setVerts(201, 200, 185);	staticMesh[OMNI_MESH].faces[346].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[346].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[347].setVerts(185, 186, 201);	staticMesh[OMNI_MESH].faces[347].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[347].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[348].setVerts(202, 201, 186);	staticMesh[OMNI_MESH].faces[348].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[348].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[349].setVerts(186, 187, 202);	staticMesh[OMNI_MESH].faces[349].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[349].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[350].setVerts(203, 202, 187);	staticMesh[OMNI_MESH].faces[350].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[350].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[351].setVerts(187, 188, 203);	staticMesh[OMNI_MESH].faces[351].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[351].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[352].setVerts(204, 203, 188);	staticMesh[OMNI_MESH].faces[352].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[352].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[353].setVerts(188, 189, 204);	staticMesh[OMNI_MESH].faces[353].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[353].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[354].setVerts(205, 204, 189);	staticMesh[OMNI_MESH].faces[354].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[354].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[355].setVerts(189, 190, 205);	staticMesh[OMNI_MESH].faces[355].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[355].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[356].setVerts(206, 205, 190);	staticMesh[OMNI_MESH].faces[356].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[356].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[357].setVerts(190, 191, 206);	staticMesh[OMNI_MESH].faces[357].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[357].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[358].setVerts(207, 206, 191);	staticMesh[OMNI_MESH].faces[358].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[358].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[359].setVerts(191, 192, 207);	staticMesh[OMNI_MESH].faces[359].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[359].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[360].setVerts(208, 207, 192);	staticMesh[OMNI_MESH].faces[360].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[360].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[361].setVerts(192, 193, 208);	staticMesh[OMNI_MESH].faces[361].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[361].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[362].setVerts(209, 208, 193);	staticMesh[OMNI_MESH].faces[362].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[362].setSmGroup(9);
		staticMesh[OMNI_MESH].faces[363].setVerts(193, 194, 209);	staticMesh[OMNI_MESH].faces[363].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[363].setSmGroup(9);
		staticMesh[OMNI_MESH].faces[364].setVerts(211, 210, 195);	staticMesh[OMNI_MESH].faces[364].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[364].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[365].setVerts(195, 196, 211);	staticMesh[OMNI_MESH].faces[365].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[365].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[366].setVerts(212, 211, 196);	staticMesh[OMNI_MESH].faces[366].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[366].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[367].setVerts(196, 197, 212);	staticMesh[OMNI_MESH].faces[367].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[367].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[368].setVerts(213, 212, 197);	staticMesh[OMNI_MESH].faces[368].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[368].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[369].setVerts(197, 198, 213);	staticMesh[OMNI_MESH].faces[369].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[369].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[370].setVerts(214, 213, 198);	staticMesh[OMNI_MESH].faces[370].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[370].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[371].setVerts(198, 199, 214);	staticMesh[OMNI_MESH].faces[371].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[371].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[372].setVerts(215, 214, 199);	staticMesh[OMNI_MESH].faces[372].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[372].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[373].setVerts(199, 200, 215);	staticMesh[OMNI_MESH].faces[373].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[373].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[374].setVerts(216, 215, 200);	staticMesh[OMNI_MESH].faces[374].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[374].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[375].setVerts(200, 201, 216);	staticMesh[OMNI_MESH].faces[375].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[375].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[376].setVerts(217, 216, 201);	staticMesh[OMNI_MESH].faces[376].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[376].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[377].setVerts(201, 202, 217);	staticMesh[OMNI_MESH].faces[377].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[377].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[378].setVerts(218, 217, 202);	staticMesh[OMNI_MESH].faces[378].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[378].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[379].setVerts(202, 203, 218);	staticMesh[OMNI_MESH].faces[379].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[379].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[380].setVerts(219, 218, 203);	staticMesh[OMNI_MESH].faces[380].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[380].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[381].setVerts(203, 204, 219);	staticMesh[OMNI_MESH].faces[381].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[381].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[382].setVerts(220, 219, 204);	staticMesh[OMNI_MESH].faces[382].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[382].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[383].setVerts(204, 205, 220);	staticMesh[OMNI_MESH].faces[383].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[383].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[384].setVerts(221, 220, 205);	staticMesh[OMNI_MESH].faces[384].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[384].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[385].setVerts(205, 206, 221);	staticMesh[OMNI_MESH].faces[385].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[385].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[386].setVerts(222, 221, 206);	staticMesh[OMNI_MESH].faces[386].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[386].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[387].setVerts(206, 207, 222);	staticMesh[OMNI_MESH].faces[387].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[387].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[388].setVerts(223, 222, 207);	staticMesh[OMNI_MESH].faces[388].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[388].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[389].setVerts(207, 208, 223);	staticMesh[OMNI_MESH].faces[389].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[389].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[390].setVerts(224, 223, 208);	staticMesh[OMNI_MESH].faces[390].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[390].setSmGroup(12);
		staticMesh[OMNI_MESH].faces[391].setVerts(208, 209, 224);	staticMesh[OMNI_MESH].faces[391].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[391].setSmGroup(12);
		staticMesh[OMNI_MESH].faces[392].setVerts(226, 225, 210);	staticMesh[OMNI_MESH].faces[392].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[392].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[393].setVerts(210, 211, 226);	staticMesh[OMNI_MESH].faces[393].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[393].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[394].setVerts(227, 226, 211);	staticMesh[OMNI_MESH].faces[394].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[394].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[395].setVerts(211, 212, 227);	staticMesh[OMNI_MESH].faces[395].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[395].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[396].setVerts(228, 227, 212);	staticMesh[OMNI_MESH].faces[396].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[396].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[397].setVerts(212, 213, 228);	staticMesh[OMNI_MESH].faces[397].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[397].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[398].setVerts(229, 228, 213);	staticMesh[OMNI_MESH].faces[398].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[398].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[399].setVerts(213, 214, 229);	staticMesh[OMNI_MESH].faces[399].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[399].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[400].setVerts(230, 229, 214);	staticMesh[OMNI_MESH].faces[400].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[400].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[401].setVerts(214, 215, 230);	staticMesh[OMNI_MESH].faces[401].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[401].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[402].setVerts(231, 230, 215);	staticMesh[OMNI_MESH].faces[402].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[402].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[403].setVerts(215, 216, 231);	staticMesh[OMNI_MESH].faces[403].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[403].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[404].setVerts(232, 231, 216);	staticMesh[OMNI_MESH].faces[404].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[404].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[405].setVerts(216, 217, 232);	staticMesh[OMNI_MESH].faces[405].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[405].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[406].setVerts(233, 232, 217);	staticMesh[OMNI_MESH].faces[406].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[406].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[407].setVerts(217, 218, 233);	staticMesh[OMNI_MESH].faces[407].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[407].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[408].setVerts(234, 233, 218);	staticMesh[OMNI_MESH].faces[408].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[408].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[409].setVerts(218, 219, 234);	staticMesh[OMNI_MESH].faces[409].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[409].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[410].setVerts(235, 234, 219);	staticMesh[OMNI_MESH].faces[410].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[410].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[411].setVerts(219, 220, 235);	staticMesh[OMNI_MESH].faces[411].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[411].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[412].setVerts(236, 235, 220);	staticMesh[OMNI_MESH].faces[412].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[412].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[413].setVerts(220, 221, 236);	staticMesh[OMNI_MESH].faces[413].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[413].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[414].setVerts(237, 236, 221);	staticMesh[OMNI_MESH].faces[414].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[414].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[415].setVerts(221, 222, 237);	staticMesh[OMNI_MESH].faces[415].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[415].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[416].setVerts(238, 237, 222);	staticMesh[OMNI_MESH].faces[416].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[416].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[417].setVerts(222, 223, 238);	staticMesh[OMNI_MESH].faces[417].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[417].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[418].setVerts(239, 238, 223);	staticMesh[OMNI_MESH].faces[418].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[418].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[419].setVerts(223, 224, 239);	staticMesh[OMNI_MESH].faces[419].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[419].setSmGroup(5);
		staticMesh[OMNI_MESH].faces[420].setVerts(1, 0, 225);		staticMesh[OMNI_MESH].faces[420].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[420].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[421].setVerts(225, 226, 1);		staticMesh[OMNI_MESH].faces[421].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[421].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[422].setVerts(2, 1, 226);		staticMesh[OMNI_MESH].faces[422].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[422].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[423].setVerts(226, 227, 2);		staticMesh[OMNI_MESH].faces[423].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[423].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[424].setVerts(3, 2, 227);		staticMesh[OMNI_MESH].faces[424].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[424].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[425].setVerts(227, 228, 3);		staticMesh[OMNI_MESH].faces[425].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[425].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[426].setVerts(4, 3, 228);		staticMesh[OMNI_MESH].faces[426].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[426].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[427].setVerts(228, 229, 4);		staticMesh[OMNI_MESH].faces[427].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[427].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[428].setVerts(5, 4, 229);		staticMesh[OMNI_MESH].faces[428].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[428].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[429].setVerts(229, 230, 5);		staticMesh[OMNI_MESH].faces[429].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[429].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[430].setVerts(6, 5, 230);		staticMesh[OMNI_MESH].faces[430].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[430].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[431].setVerts(230, 231, 6);		staticMesh[OMNI_MESH].faces[431].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[431].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[432].setVerts(7, 6, 231);		staticMesh[OMNI_MESH].faces[432].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[432].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[433].setVerts(231, 232, 7);		staticMesh[OMNI_MESH].faces[433].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[433].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[434].setVerts(8, 7, 232);		staticMesh[OMNI_MESH].faces[434].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[434].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[435].setVerts(232, 233, 8);		staticMesh[OMNI_MESH].faces[435].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[435].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[436].setVerts(9, 8, 233);		staticMesh[OMNI_MESH].faces[436].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[436].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[437].setVerts(233, 234, 9);		staticMesh[OMNI_MESH].faces[437].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[437].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[438].setVerts(10, 9, 234);		staticMesh[OMNI_MESH].faces[438].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[438].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[439].setVerts(234, 235, 10);	staticMesh[OMNI_MESH].faces[439].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[439].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[440].setVerts(11, 10, 235);		staticMesh[OMNI_MESH].faces[440].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[440].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[441].setVerts(235, 236, 11);	staticMesh[OMNI_MESH].faces[441].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[441].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[442].setVerts(12, 11, 236);		staticMesh[OMNI_MESH].faces[442].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[442].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[443].setVerts(236, 237, 12);	staticMesh[OMNI_MESH].faces[443].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[443].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[444].setVerts(13, 12, 237);		staticMesh[OMNI_MESH].faces[444].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[444].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[445].setVerts(237, 238, 13);	staticMesh[OMNI_MESH].faces[445].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[445].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[446].setVerts(14, 13, 238);		staticMesh[OMNI_MESH].faces[446].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[446].setSmGroup(24);
		staticMesh[OMNI_MESH].faces[447].setVerts(238, 239, 14);	staticMesh[OMNI_MESH].faces[447].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[447].setSmGroup(24);
		staticMesh[OMNI_MESH].faces[448].setVerts(240, 241, 243);	staticMesh[OMNI_MESH].faces[448].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[448].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[449].setVerts(243, 242, 240);	staticMesh[OMNI_MESH].faces[449].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[449].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[450].setVerts(244, 245, 247);	staticMesh[OMNI_MESH].faces[450].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[450].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[451].setVerts(247, 246, 244);	staticMesh[OMNI_MESH].faces[451].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[451].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[452].setVerts(248, 249, 251);	staticMesh[OMNI_MESH].faces[452].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[452].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[453].setVerts(251, 250, 248);	staticMesh[OMNI_MESH].faces[453].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[453].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[454].setVerts(252, 253, 255);	staticMesh[OMNI_MESH].faces[454].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[454].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[455].setVerts(255, 254, 252);	staticMesh[OMNI_MESH].faces[455].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[455].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[456].setVerts(256, 257, 259);	staticMesh[OMNI_MESH].faces[456].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[456].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[457].setVerts(259, 258, 256);	staticMesh[OMNI_MESH].faces[457].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[457].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[458].setVerts(260, 261, 263);	staticMesh[OMNI_MESH].faces[458].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[458].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[459].setVerts(263, 262, 260);	staticMesh[OMNI_MESH].faces[459].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[459].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[460].setVerts(264, 265, 267);	staticMesh[OMNI_MESH].faces[460].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[460].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[461].setVerts(267, 266, 264);	staticMesh[OMNI_MESH].faces[461].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[461].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[462].setVerts(268, 269, 271);	staticMesh[OMNI_MESH].faces[462].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[462].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[463].setVerts(271, 270, 268);	staticMesh[OMNI_MESH].faces[463].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[463].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[464].setVerts(272, 273, 275);	staticMesh[OMNI_MESH].faces[464].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[464].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[465].setVerts(275, 274, 272);	staticMesh[OMNI_MESH].faces[465].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[465].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[466].setVerts(279, 277, 276);	staticMesh[OMNI_MESH].faces[466].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[466].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[467].setVerts(276, 278, 279);	staticMesh[OMNI_MESH].faces[467].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[467].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[468].setVerts(283, 281, 280);	staticMesh[OMNI_MESH].faces[468].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[468].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[469].setVerts(280, 282, 283);	staticMesh[OMNI_MESH].faces[469].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[469].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[470].setVerts(287, 285, 284);	staticMesh[OMNI_MESH].faces[470].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[470].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[471].setVerts(284, 286, 287);	staticMesh[OMNI_MESH].faces[471].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[471].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[472].setVerts(291, 289, 288);	staticMesh[OMNI_MESH].faces[472].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[472].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[473].setVerts(288, 290, 291);	staticMesh[OMNI_MESH].faces[473].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[473].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[474].setVerts(295, 293, 292);	staticMesh[OMNI_MESH].faces[474].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[474].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[475].setVerts(292, 294, 295);	staticMesh[OMNI_MESH].faces[475].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[475].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[476].setVerts(299, 297, 296);	staticMesh[OMNI_MESH].faces[476].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[476].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[477].setVerts(296, 298, 299);	staticMesh[OMNI_MESH].faces[477].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[477].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[478].setVerts(303, 301, 300);	staticMesh[OMNI_MESH].faces[478].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[478].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[479].setVerts(300, 302, 303);	staticMesh[OMNI_MESH].faces[479].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[479].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[480].setVerts(307, 305, 304);	staticMesh[OMNI_MESH].faces[480].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[480].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[481].setVerts(304, 306, 307);	staticMesh[OMNI_MESH].faces[481].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[481].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[482].setVerts(311, 309, 308);	staticMesh[OMNI_MESH].faces[482].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[482].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[483].setVerts(308, 310, 311);	staticMesh[OMNI_MESH].faces[483].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[483].setSmGroup(1);
		staticMesh[OMNI_MESH].faces[484].setVerts(241, 240, 276);	staticMesh[OMNI_MESH].faces[484].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[484].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[485].setVerts(276, 277, 241);	staticMesh[OMNI_MESH].faces[485].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[485].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[486].setVerts(243, 241, 277);	staticMesh[OMNI_MESH].faces[486].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[486].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[487].setVerts(277, 279, 243);	staticMesh[OMNI_MESH].faces[487].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[487].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[488].setVerts(242, 243, 279);	staticMesh[OMNI_MESH].faces[488].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[488].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[489].setVerts(279, 278, 242);	staticMesh[OMNI_MESH].faces[489].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[489].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[490].setVerts(240, 242, 278);	staticMesh[OMNI_MESH].faces[490].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[490].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[491].setVerts(278, 276, 240);	staticMesh[OMNI_MESH].faces[491].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[491].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[492].setVerts(245, 244, 280);	staticMesh[OMNI_MESH].faces[492].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[492].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[493].setVerts(280, 281, 245);	staticMesh[OMNI_MESH].faces[493].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[493].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[494].setVerts(247, 245, 281);	staticMesh[OMNI_MESH].faces[494].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[494].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[495].setVerts(281, 283, 247);	staticMesh[OMNI_MESH].faces[495].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[495].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[496].setVerts(246, 247, 283);	staticMesh[OMNI_MESH].faces[496].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[496].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[497].setVerts(283, 282, 246);	staticMesh[OMNI_MESH].faces[497].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[497].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[498].setVerts(244, 246, 282);	staticMesh[OMNI_MESH].faces[498].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[498].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[499].setVerts(282, 280, 244);	staticMesh[OMNI_MESH].faces[499].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[499].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[500].setVerts(249, 248, 284);	staticMesh[OMNI_MESH].faces[500].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[500].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[501].setVerts(284, 285, 249);	staticMesh[OMNI_MESH].faces[501].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[501].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[502].setVerts(251, 249, 285);	staticMesh[OMNI_MESH].faces[502].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[502].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[503].setVerts(285, 287, 251);	staticMesh[OMNI_MESH].faces[503].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[503].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[504].setVerts(250, 251, 287);	staticMesh[OMNI_MESH].faces[504].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[504].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[505].setVerts(287, 286, 250);	staticMesh[OMNI_MESH].faces[505].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[505].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[506].setVerts(248, 250, 286);	staticMesh[OMNI_MESH].faces[506].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[506].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[507].setVerts(286, 284, 248);	staticMesh[OMNI_MESH].faces[507].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[507].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[508].setVerts(253, 252, 288);	staticMesh[OMNI_MESH].faces[508].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[508].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[509].setVerts(288, 289, 253);	staticMesh[OMNI_MESH].faces[509].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[509].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[510].setVerts(255, 253, 289);	staticMesh[OMNI_MESH].faces[510].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[510].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[511].setVerts(289, 291, 255);	staticMesh[OMNI_MESH].faces[511].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[511].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[512].setVerts(254, 255, 291);	staticMesh[OMNI_MESH].faces[512].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[512].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[513].setVerts(291, 290, 254);	staticMesh[OMNI_MESH].faces[513].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[513].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[514].setVerts(252, 254, 290);	staticMesh[OMNI_MESH].faces[514].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[514].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[515].setVerts(290, 288, 252);	staticMesh[OMNI_MESH].faces[515].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[515].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[516].setVerts(257, 256, 292);	staticMesh[OMNI_MESH].faces[516].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[516].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[517].setVerts(292, 293, 257);	staticMesh[OMNI_MESH].faces[517].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[517].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[518].setVerts(259, 257, 293);	staticMesh[OMNI_MESH].faces[518].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[518].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[519].setVerts(293, 295, 259);	staticMesh[OMNI_MESH].faces[519].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[519].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[520].setVerts(258, 259, 295);	staticMesh[OMNI_MESH].faces[520].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[520].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[521].setVerts(295, 294, 258);	staticMesh[OMNI_MESH].faces[521].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[521].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[522].setVerts(256, 258, 294);	staticMesh[OMNI_MESH].faces[522].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[522].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[523].setVerts(294, 292, 256);	staticMesh[OMNI_MESH].faces[523].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[523].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[524].setVerts(261, 260, 296);	staticMesh[OMNI_MESH].faces[524].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[524].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[525].setVerts(296, 297, 261);	staticMesh[OMNI_MESH].faces[525].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[525].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[526].setVerts(263, 261, 297);	staticMesh[OMNI_MESH].faces[526].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[526].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[527].setVerts(297, 299, 263);	staticMesh[OMNI_MESH].faces[527].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[527].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[528].setVerts(262, 263, 299);	staticMesh[OMNI_MESH].faces[528].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[528].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[529].setVerts(299, 298, 262);	staticMesh[OMNI_MESH].faces[529].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[529].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[530].setVerts(260, 262, 298);	staticMesh[OMNI_MESH].faces[530].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[530].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[531].setVerts(298, 296, 260);	staticMesh[OMNI_MESH].faces[531].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[531].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[532].setVerts(265, 264, 300);	staticMesh[OMNI_MESH].faces[532].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[532].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[533].setVerts(300, 301, 265);	staticMesh[OMNI_MESH].faces[533].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[533].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[534].setVerts(267, 265, 301);	staticMesh[OMNI_MESH].faces[534].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[534].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[535].setVerts(301, 303, 267);	staticMesh[OMNI_MESH].faces[535].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[535].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[536].setVerts(266, 267, 303);	staticMesh[OMNI_MESH].faces[536].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[536].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[537].setVerts(303, 302, 266);	staticMesh[OMNI_MESH].faces[537].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[537].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[538].setVerts(264, 266, 302);	staticMesh[OMNI_MESH].faces[538].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[538].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[539].setVerts(302, 300, 264);	staticMesh[OMNI_MESH].faces[539].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[539].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[540].setVerts(269, 268, 304);	staticMesh[OMNI_MESH].faces[540].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[540].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[541].setVerts(304, 305, 269);	staticMesh[OMNI_MESH].faces[541].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[541].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[542].setVerts(271, 269, 305);	staticMesh[OMNI_MESH].faces[542].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[542].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[543].setVerts(305, 307, 271);	staticMesh[OMNI_MESH].faces[543].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[543].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[544].setVerts(270, 271, 307);	staticMesh[OMNI_MESH].faces[544].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[544].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[545].setVerts(307, 306, 270);	staticMesh[OMNI_MESH].faces[545].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[545].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[546].setVerts(268, 270, 306);	staticMesh[OMNI_MESH].faces[546].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[546].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[547].setVerts(306, 304, 268);	staticMesh[OMNI_MESH].faces[547].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[547].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[548].setVerts(273, 272, 308);	staticMesh[OMNI_MESH].faces[548].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[548].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[549].setVerts(308, 309, 273);	staticMesh[OMNI_MESH].faces[549].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[549].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[550].setVerts(275, 273, 309);	staticMesh[OMNI_MESH].faces[550].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[550].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[551].setVerts(309, 311, 275);	staticMesh[OMNI_MESH].faces[551].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[551].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[552].setVerts(274, 275, 311);	staticMesh[OMNI_MESH].faces[552].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[552].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[553].setVerts(311, 310, 274);	staticMesh[OMNI_MESH].faces[553].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[553].setSmGroup(2);
		staticMesh[OMNI_MESH].faces[554].setVerts(272, 274, 310);	staticMesh[OMNI_MESH].faces[554].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[554].setSmGroup(4);
		staticMesh[OMNI_MESH].faces[555].setVerts(310, 308, 272);	staticMesh[OMNI_MESH].faces[555].setEdgeVisFlags(1, 1, 0);		staticMesh[OMNI_MESH].faces[555].setSmGroup(4);
		staticMesh[OMNI_MESH].buildNormals();
		staticMesh[OMNI_MESH].EnableEdgeList(1);

		// Build an "arrow"
		nverts = 13;
		nfaces = 16;
		staticMesh[DIR_MESH].setNumVerts(nverts);
		staticMesh[DIR_MESH].setNumFaces(nfaces);
		s = 4.0f;
		float s4 = 16.0f;
		staticMesh[DIR_MESH].setVert( 0, Point3( -s,-s, FZ));
		staticMesh[DIR_MESH].setVert( 1, Point3(  s,-s, FZ));
		staticMesh[DIR_MESH].setVert( 2, Point3(  s, s, FZ));
		staticMesh[DIR_MESH].setVert( 3, Point3( -s, s, FZ));
		staticMesh[DIR_MESH].setVert( 4, Point3( -s,-s, -s4));
		staticMesh[DIR_MESH].setVert( 5, Point3(  s,-s, -s4));
		staticMesh[DIR_MESH].setVert( 6, Point3(  s, s, -s4));
		staticMesh[DIR_MESH].setVert( 7, Point3( -s, s, -s4));
		s *= (float)2.0;
		staticMesh[DIR_MESH].setVert( 8, Point3( -s,-s, -s4));
		staticMesh[DIR_MESH].setVert( 9, Point3(  s,-s, -s4));
		staticMesh[DIR_MESH].setVert(10, Point3(  s, s, -s4));
		staticMesh[DIR_MESH].setVert(11, Point3( -s, s, -s4));
		staticMesh[DIR_MESH].setVert(12, Point3( FZ,FZ, -s4-s));
		SET_QUAD( 0, 1, 0, 4, 5);
		SET_QUAD( 2, 0, 3, 7, 4);
		SET_QUAD( 4, 3, 2, 6, 7);
		SET_QUAD( 6, 2, 1, 5, 6);
		SET_QUAD( 8, 0, 1, 2, 3);
		SET_QUAD(10, 8, 9, 10, 11);
		staticMesh[DIR_MESH].faces[12].setVerts(8,12,9);
		staticMesh[DIR_MESH].faces[12].setEdgeVisFlags(1,1,1);
		staticMesh[DIR_MESH].faces[13].setVerts(9,12,10);
		staticMesh[DIR_MESH].faces[13].setEdgeVisFlags(1,1,1);
		staticMesh[DIR_MESH].faces[14].setVerts(10,12,11);
		staticMesh[DIR_MESH].faces[14].setEdgeVisFlags(1,1,1);
		staticMesh[DIR_MESH].faces[15].setVerts(11,12,8);
		staticMesh[DIR_MESH].faces[15].setEdgeVisFlags(1,1,1);
      for (int i=0; i<nfaces; i++)
			staticMesh[DIR_MESH].faces[i].setSmGroup(i);
		staticMesh[DIR_MESH].buildNormals();
		staticMesh[DIR_MESH].EnableEdgeList(1);

		meshBuilt = 1;
	}
}


void GeneralLight::BuildSpotMesh(float coneSize)
{
	// build a cone
	if(coneSize < 0.0f)
		return;
	int nverts = 9;
	int nfaces = 8;
	spotMesh.setNumVerts(nverts);
	spotMesh.setNumFaces(nfaces);
	double radang = 3.1415926 * coneSize / 360.0;
	float h = 20.0f;					// hypotenuse
	float d = -h * (float)cos(radang);	// dist from origin to cone circle
	float r = h * (float)sin(radang);	// radius of cone circle
	float s = 0.70711f * r;  			// sin(45) * r
	spotMesh.setVert(0, Point3( FZ, FZ, FZ));
	spotMesh.setVert(1, Point3( -r, FZ, d));
	spotMesh.setVert(2, Point3( -s, -s, d));
	spotMesh.setVert(3, Point3( FZ, -r, d));
	spotMesh.setVert(4, Point3(  s, -s, d));
	spotMesh.setVert(5, Point3(  r, FZ, d));
	spotMesh.setVert(6, Point3(  s,  s, d));
	spotMesh.setVert(7, Point3( FZ,  r, d));
	spotMesh.setVert(8, Point3( -s,  s, d));
	spotMesh.faces[0].setVerts(0,1,2);
	spotMesh.faces[1].setVerts(0,2,3);
	spotMesh.faces[2].setVerts(0,3,4);
	spotMesh.faces[3].setVerts(0,4,5);
	spotMesh.faces[4].setVerts(0,5,6);
	spotMesh.faces[5].setVerts(0,6,7);
	spotMesh.faces[6].setVerts(0,7,8);
	spotMesh.faces[7].setVerts(0,8,1);
	for (int i=0; i<nfaces; i++) {
		spotMesh.faces[i].setSmGroup(i);
		spotMesh.faces[i].setEdgeVisFlags(1,1,1);
	}
	spotMesh.buildNormals();
	spotMesh.EnableEdgeList(1);
}

void GeneralLight::UpdateColBrackets(TimeValue t) {
	BOOL isKey = pblock->KeyFrameAtTime(PB_COLOR,t); 
//	redSpin->SetKeyBrackets(isKey);
//	greenSpin->SetKeyBrackets(isKey);
//	blueSpin->SetKeyBrackets(isKey);
	colorSwatch->SetKeyBrackets(isKey);
//	hSpin->SetKeyBrackets(isKey);
//	sSpin->SetKeyBrackets(isKey);
//	vSpin->SetKeyBrackets(isKey);
	isKey = pblock->KeyFrameAtTime(type==OMNI_LIGHT?PB_OMNISHADCOLOR:PB_SHADCOLOR,t); 
	shadColorSwatch->SetKeyBrackets(isKey);
	}

void GeneralLight::UpdateForAmbOnly() {
	BOOL b = !ambientOnly;
	if(hGeneralLight) {
		
		EnableWindow(GetDlgItem(hGeneralLight, IDC_CAST_SHADOWS), b);
		EnableWindow(GetDlgItem(hGeneralLight, IDC_GLOBAL_SET), b);
		}	
	if (contrastSpin)
	{
		if (b)
		{
			Control* c = pblock->GetController(PB_CONTRAST);
			if (c != NULL)
				contrastSpin->Enable(c->IsKeyable());
			else
				contrastSpin->Enable(b);
		}
		else
		{
		contrastSpin->Enable(b);
		}
	}
	if (diffsoftSpin)
	{
		if (b)
		{
			Control* c = pblock->GetController(PB_DIFFSOFT);
			if (c != NULL)
				diffsoftSpin->Enable(c->IsKeyable());
			else
		diffsoftSpin->Enable(b);
		}
		else
		{
			diffsoftSpin->Enable(b);
		}
	}
	if (hShadow) {
		
		EnableWindow(GetDlgItem(hShadow, IDC_SHADOW_MAPS), b);
		EnableWindow(GetDlgItem(hShadow, IDC_RAY_TRACED_SHADOWS), b);
		EnableWindow(GetDlgItem(hShadow, IDC_ABS_MAP_BIAS), b);
		EnableWindow(GetDlgItem(hShadow, IDC_LT_EFFECT_SHADCOL), b);
		EnableWindow(GetDlgItem(hShadow, IDC_OBJECT_SHADOWS), b);
		if (shadColorSwatch) shadColorSwatch->Enable(b);
		if (shadProjMapName) shadProjMapName->Enable(b);
		}
	if(hAdvEff) {
		EnableWindow(GetDlgItem(hAdvEff, IDC_AFFECT_SPECULAR), b);
		EnableWindow(GetDlgItem(hAdvEff, IDC_AFFECT_DIFFUSE), b);
		EnableWindow(GetDlgItem(hAdvEff, IDC_CONTRAST_T), b);
		EnableWindow(GetDlgItem(hAdvEff, IDC_DIFFSOFT_T), b);
	}
	}

void GeneralLight::UpdateUI(TimeValue t)
	{
	Point3 color;

	if ( hGeneralLight && !waitPostLoad &&
		GetWindowLongPtr(hGeneralLight,GWLP_USERDATA)==(LONG_PTR)this && pblock) {
		color = GetRGBColor(t);
//		redSpin->SetValue( FLto255f(color.x), FALSE );
//		greenSpin->SetValue( FLto255f(color.y), FALSE );
//		blueSpin->SetValue( FLto255f(color.z), FALSE );
		colorSwatch->SetColor(RGB(FLto255i(color.x), FLto255i(color.y), FLto255i(color.z)));
		// these controls no longer exist in the new ui - DC
/*
		if(updateHSVSpin) {
			color = GetHSVColor(t);
			hSpin->SetValue( FLto255f(color.x), FALSE );
			sSpin->SetValue( FLto255f(color.y), FALSE );
			vSpin->SetValue( FLto255f(color.z), FALSE );
			}
*/
		UpdateColBrackets(t);

		CheckDlgButton( hGeneralLight, IDC_CAST_SHADOWS, GetShadow() );
		CheckDlgButton( hGeneralLight, IDC_GLOBAL_SET, GetUseGlobal() ); 
//		CheckDlgButton( hShadow, IDC_OBJECT_SHADOWS, GetShadow() );

		intensitySpin->SetValue( GetIntensity(t), FALSE);
		intensitySpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_INTENSITY,t));

		contrastSpin->SetValue( GetContrast(t), FALSE);
		contrastSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_CONTRAST,t));

		diffsoftSpin->SetValue(GetDiffuseSoft(t), FALSE);
		diffsoftSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_DIFFSOFT,t));

		CheckDlgButton( hGeneralLight, IDC_LIGHT_ON, GetUseLight());

		CheckDlgButton( hAdvEff, IDC_AFFECT_DIFFUSE, affectDiffuse);
		CheckDlgButton( hAdvEff, IDC_AFFECT_SPECULAR, affectSpecular);
		CheckDlgButton( hAdvEff, IDC_AMBIENT_ONLY, ambientOnly);

        UpdateForAmbOnly();

		SetDlgItemText(hGeneralLight,IDC_EXCLUDE_DLG, 
			GetString(exclList.TestFlag(NT_INCLUDE)?IDS_DS_INCLUDE__: IDS_DS_EXCLUDE__));

		if(attenStartSpin) {
			attenStartSpin->SetValue( GetAtten(t, ATTEN_START), FALSE);
			attenStartSpin->SetKeyBrackets(pblock->KeyFrameAtTime(type== OMNI_LIGHT? PB_OMNIATSTART:PB_ATTENSTART,t));
			}
		if(attenEndSpin) {
			attenEndSpin->SetValue( GetAtten(t, ATTEN_END), FALSE);
			attenEndSpin->SetKeyBrackets(pblock->KeyFrameAtTime(type== OMNI_LIGHT? PB_OMNIATEND:PB_ATTENEND,t));
			}
		CheckDlgButton( hICAParam, IDC_SHOW_RANGES, GetAttenDisplay() );
		CheckDlgButton( hICAParam, IDC_USE_ATTEN, GetUseAtten() );

		if(attenStart1Spin) {
			attenStart1Spin->SetValue( GetAtten(t, ATTEN1_START), FALSE);
			attenStart1Spin->SetKeyBrackets(pblock->KeyFrameAtTime(type== OMNI_LIGHT? PB_OMNIATSTART1:PB_ATTENSTART1,t));
			}
		if(attenEnd1Spin) {
			attenEnd1Spin->SetValue( GetAtten(t, ATTEN1_END), FALSE);
			attenEnd1Spin->SetKeyBrackets(pblock->KeyFrameAtTime(type== OMNI_LIGHT? PB_OMNIATEND1:PB_ATTENEND1,t));
			}
		if(decaySpin) {
			decaySpin->SetValue( GetDecayRadius(t), FALSE);
			decaySpin->SetKeyBrackets(pblock->KeyFrameAtTime(type==OMNI_LIGHT? PB_OMNIDECAY: PB_DECAY,t));
			}
		CheckDlgButton( hICAParam, IDC_SHOW_RANGES1, GetAttenNearDisplay() );
		CheckDlgButton( hICAParam, IDC_USE_ATTEN1, GetUseAttenNear() );

		if( IsSpot() ) {
			float hot = 0.0f;
			float fall = 0.0f;
			if (hotsizeSpin) {
				hotsizeSpin->SetValue( hot = GetHotspot(t), FALSE );
				hotsizeSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_HOTSIZE,t));
				}

			if (fallsizeSpin) {
				fallsizeSpin->SetValue( fall = GetFallsize(t), FALSE );
				fallsizeSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_FALLSIZE,t));
				}

			if (tDistSpin&&(type == FSPOT_LIGHT||type==DIR_LIGHT)) {
				tDistSpin->SetValue( GetTDist(t), FALSE );
				tDistSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_TDIST,t));
				}

			if (aspectSpin) {
				aspectSpin->SetValue( GetAspect(t), FALSE );
				aspectSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_ASPECT,t));
				}

			CheckDlgButton( hSpotLight, IDC_SHOW_CONE, GetConeDisplay() );

			BuildSpotMesh(max(hot, fall));
			CheckDlgButton( hSpotLight, IDC_OVERSHOOT, GetOvershoot() ); 
			CheckDlgButton( hAdvEff, IDC_PROJECTOR, GetProjector() ); 
			}
	
		CheckDlgButton( hShadow, IDC_LT_EFFECT_SHADCOL, GetLightAffectsShadow() );
		CheckRadioButton( hShadow, IDC_SHADOW_MAPS, IDC_RAY_TRACED_SHADOWS, 
			IDC_SHADOW_MAPS+GetShadowType());
		
		CheckDlgButton( hShadow, IDC_ABS_MAP_BIAS, GetAbsMapBias() ); 
		color = GetShadColor(t);
		shadColorSwatch->SetColor(RGB(FLto255i(color.x), FLto255i(color.y), FLto255i(color.z)));

		CheckDlgButton( hShadow, IDC_USE_SHAD_COLMAP, GetUseShadowColorMap(t) );
		CheckDlgButton( hShadow, IDC_ATMOS_SHADOWS, GetAtmosShadows(t) ); 
		atmosOpacitySpin->SetValue( GetAtmosOpacity(t)*100.0f, FALSE );
		atmosOpacitySpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_ATM_OPAC(this),t));
		atmosColAmtSpin->SetValue( GetAtmosColAmt(t)*100.0f, FALSE );
		atmosColAmtSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_ATM_COLAMT(this),t));
		shadMultSpin->SetValue( GetShadMult(t), FALSE );
		shadMultSpin->SetKeyBrackets(pblock->KeyFrameAtTime(PB_SHAD_MULT(this),t));
		if (projMapName)
			projMapName->SetText(projMap?projMap->GetFullName().data():GetString(IDS_DB_NONE));
		if (shadProjMapName)
			shadProjMapName->SetText(shadProjMap?shadProjMap->GetFullName().data():GetString(IDS_DB_NONE));
		// DS 8/28/00
		// added: DC 08/03/2001
		// reflects simpler selection of type ui
		if(!uiConsistency)
		SendMessage( GetDlgItem(hGeneralLight, IDC_LIGHT_TYPE), CB_SETCURSEL, idFromType(type), (LPARAM)0 );
		else
			SendMessage( GetDlgItem(hGeneralLight, IDC_LIGHT_TYPE), CB_SETCURSEL, idFromTypeSimplified(type), (LPARAM)0 );

		//aszabo|Nov.09.01 - disable param-wired, script or expression controlled params

		// DC - Nov 30, 2001 moved this call into this block so that a NULL pblock is checked
		// bug 313434
		UpdateControlledUI();
		}  // closes initial update check
		
	}


// description for version 0
static ParamBlockDescID descV0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 1 },		// red
	{ TYPE_FLOAT, NULL, TRUE, 2 },		// green
	{ TYPE_FLOAT, NULL, TRUE, 3 },		// blue
	{ TYPE_FLOAT, NULL, TRUE, 7 } };	// hotsize

// description for version 1
static ParamBlockDescID descV1[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 9 },		// tdist
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_FLOAT, NULL, TRUE, 12 } };	// blur

// description for version 2
static ParamBlockDescID descV2[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 9 },		// tdist
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_FLOAT, NULL, TRUE, 12 },  	// blur
	{ TYPE_INT, NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 }};  	// raytrace bias

// description for version 3 -- all but omni
static ParamBlockDescID descV3[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_FLOAT, NULL, TRUE, 12 },  	// blur
	{ TYPE_INT, NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start		<- atten comes last because dir light doesn't have it
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 9 }};		// tdist

// description for version 3 -- only omni
static ParamBlockDescID descV3Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start
	{ TYPE_FLOAT, NULL, TRUE, 6 }};		// atten end

// description for versions 4 -- all but omni

static ParamBlockDescID descV4[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_INT, NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start		<- atten comes last because dir light doesn't have it
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 9 }};		// tdist

// description for version 4 -- only omni
static ParamBlockDescID descV4Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start
	{ TYPE_FLOAT, NULL, TRUE, 6 }};		// atten end

// description for versions 5 - all but omni
static ParamBlockDescID descV5[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_INT,   NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 9 }};		// tdist


// description for version 5 -- only omni
static ParamBlockDescID descV5Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 17 },	 // atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },	 // atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },	 // atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 }};	 // atten end 


// description for versions 6 - all but omni
static ParamBlockDescID descV6[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_INT,   NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 9 }};		// tdist

// description for version 6 -- only omni
static ParamBlockDescID descV6Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },		// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_INT,   NULL, TRUE, 13 }, 	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 }, 	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 }, 	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 }, 	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 }};		// atten end 


// description for versions 7 - all but omni
static ParamBlockDescID descV7[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_INT,   NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 9 }};		// tdist

// description for version 7 -- only omni
static ParamBlockDescID descV7Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },		// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_INT,   NULL, TRUE, 13 }, 	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 }, 	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 }, 	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 }, 	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 }};		// atten end 


// ---------------- Version 8  ( Max 2.0 )

// description for versions 8  - all but omni
static ParamBlockDescID descV8[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_INT,   NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 9 }	   	// TDIST
	};		


//-------- Version 9:  6/1/98----------------

// description for versions 9  - all but omni
static ParamBlockDescID descV9[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_INT,   NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_FLOAT, NULL, TRUE, 9 }	   	// TDIST
	};		

// description for version 9 -- only omni
static ParamBlockDescID descV9Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },		// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_INT,   NULL, TRUE, 13 }, 	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 }, 	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 }, 	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 }, 	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end 
	{ TYPE_RGBA,  NULL, TRUE, 21 }		// shad color
	};


//-------- Version 10:  8/14/98----------------

// description for version 10  - all but omni
static ParamBlockDescID descV10[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_INT,   NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_FLOAT, NULL, TRUE, 9 }	   	// TDIST
	};		

static ParamBlockDescID descV10Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },		// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_INT,   NULL, TRUE, 13 }, 	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 }, 	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 }, 	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 }, 	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end 
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius 
	{ TYPE_RGBA,  NULL, TRUE, 21 }		// shad color
	};


//-------- Version 11:  9/16/98----------------

// description for version 11  - all but omni 
static ParamBlockDescID descV11[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_INT,   NULL, TRUE, 13 },  	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 },  	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 },  	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 },  	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_INT,   NULL, FALSE, 23 },	// atmosphere shadows
	{ TYPE_FLOAT, NULL, TRUE, 24 },		// atmosphere opacity
	{ TYPE_FLOAT, NULL, TRUE, 25 },		// atmosphere color influence
	{ TYPE_FLOAT, NULL, TRUE, 9 }	   	// TDIST
	};		

static ParamBlockDescID descV11Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },		// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_INT,   NULL, TRUE, 13 }, 	// mapSize
	{ TYPE_FLOAT, NULL, TRUE, 14 }, 	// mapBias
	{ TYPE_FLOAT, NULL, TRUE, 15 }, 	// mapRange
	{ TYPE_FLOAT, NULL, TRUE, 16 }, 	// raytrace bias
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end 
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius 
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_INT,   NULL, FALSE, 23 },	// atmosphere shadows
	{ TYPE_FLOAT, NULL, TRUE, 24 },		// atmosphere opacity
	{ TYPE_FLOAT, NULL, TRUE, 25 }		// atmosphere color influence
	};


//-------- Version 12:  11/2/98----------------
// Shadow Generator plugins added.
// description for version 12  - all but omni 
static ParamBlockDescID descV12[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_INT,   NULL, FALSE, 23 },	// atmosphere shadows
	{ TYPE_FLOAT, NULL, TRUE, 24 },		// atmosphere opacity
	{ TYPE_FLOAT, NULL, TRUE, 25 },		// atmosphere color influence
	{ TYPE_FLOAT, NULL, TRUE, 9 }	   	// TDIST
	};		

// description for version 12  - omni's 
static ParamBlockDescID descV12Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },		// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end 
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius 
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_INT,   NULL, FALSE, 23 },	// atmosphere shadows
	{ TYPE_FLOAT, NULL, TRUE, 24 },		// atmosphere opacity
	{ TYPE_FLOAT, NULL, TRUE, 25 }		// atmosphere color influence
	};


//-------- Version 13:  11/9/98----------------
// Added shad mult
// description for version 13  - all but omni 
static ParamBlockDescID descV13[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_INT,   NULL, FALSE, 23 },	// atmosphere shadows
	{ TYPE_FLOAT, NULL, TRUE, 24 },		// atmosphere opacity
	{ TYPE_FLOAT, NULL, TRUE, 25 },		// atmosphere color influence
	{ TYPE_FLOAT, NULL, TRUE, 26 },	   	// shadow multiplier
	{ TYPE_FLOAT, NULL, TRUE, 9 }   	// TDIST
	};		

// description for version 13  - omni's 
static ParamBlockDescID descV13Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },		// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end 
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius 
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_INT,   NULL, FALSE, 23 },	// atmosphere shadows
	{ TYPE_FLOAT, NULL, TRUE, 24 },		// atmosphere opacity
	{ TYPE_FLOAT, NULL, TRUE, 25 },		// atmosphere color influence
	{ TYPE_FLOAT, NULL, TRUE, 26 }	   	// shadow multiplier
	};

//-------- Version 14:  11/20/98----------------
// Added shad mult
// description for version 14  - all but omni 
static ParamBlockDescID descV14[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },	// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 7 },		// hotsize
	{ TYPE_FLOAT, NULL, TRUE, 8 },		// fallsize
	{ TYPE_FLOAT, NULL, TRUE, 11 },		// aspect
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 	
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_INT,   NULL, FALSE, 23 },	// atmosphere shadows
	{ TYPE_FLOAT, NULL, TRUE, 24 },		// atmosphere opacity
	{ TYPE_FLOAT, NULL, TRUE, 25 },		// atmosphere color influence
	{ TYPE_FLOAT, NULL, TRUE, 26 },	   	// shadow multiplier
	{ TYPE_INT,   NULL, FALSE, 27 },	// use shadow colormap
	{ TYPE_FLOAT, NULL, TRUE, 9 }   	// TDIST
	};		

// description for version 14  - omni's 
static ParamBlockDescID descV14Omni[] = {
	{ TYPE_RGBA, NULL, TRUE, 10 },		// color
	{ TYPE_FLOAT, NULL, TRUE, 4 },		// intensity
	{ TYPE_FLOAT, NULL, TRUE, 19},		// contrast
	{ TYPE_FLOAT, NULL, TRUE, 20},		// diffsoft
	{ TYPE_FLOAT, NULL, TRUE, 17 },		// atten start near
	{ TYPE_FLOAT, NULL, TRUE, 18 },		// atten end near
	{ TYPE_FLOAT, NULL, TRUE, 5 },		// atten start 
	{ TYPE_FLOAT, NULL, TRUE, 6 },		// atten end 
	{ TYPE_FLOAT, NULL, TRUE, 22 },		// decay radius 
	{ TYPE_RGBA,  NULL, TRUE, 21 },		// shad color
	{ TYPE_INT,   NULL, FALSE, 23 },	// atmosphere shadows
	{ TYPE_FLOAT, NULL, TRUE, 24 },		// atmosphere opacity
	{ TYPE_FLOAT, NULL, TRUE, 25 },		// atmosphere color influence
	{ TYPE_FLOAT, NULL, TRUE, 26 },	   	// shadow multiplier
	{ TYPE_INT,   NULL, FALSE, 27 }	// use shadow colormap
	};

static ParamBlockDescID emitterV01[] = {
	{ TYPE_BOOL,  NULL, TRUE, PB_EMITTER_ENABLE },		// PB_EMITTER_ENABLE
	{ TYPE_FLOAT, NULL, TRUE, PB_EMITTER_ENERGY },		// PB_EMITTER_ENERGY
	{ TYPE_INT,	  NULL, FALSE, PB_EMITTER_DECAY_TYPE },	// PB_EMITTER_DECAY_TYPE
	{ TYPE_INT,   NULL, TRUE, PB_EMITTER_CA_PHOTONS },	// PB_EMITTER_CA_PHOTONS
	};		
static int numEmitterV01Params = sizeof ( emitterV01 ) / sizeof ( ParamBlockDescID );

// Added global illumination photons
static ParamBlockDescID emitterV02[] = {
	{ TYPE_BOOL,  NULL, TRUE, PB_EMITTER_ENABLE },		// PB_EMITTER_ENABLE
	{ TYPE_FLOAT, NULL, TRUE, PB_EMITTER_ENERGY },		// PB_EMITTER_ENERGY
	{ TYPE_INT,	  NULL, FALSE, PB_EMITTER_DECAY_TYPE },	// PB_EMITTER_DECAY_TYPE
	{ TYPE_INT,   NULL, TRUE, PB_EMITTER_CA_PHOTONS },	// PB_EMITTER_CA_PHOTONS
	{ TYPE_INT,   NULL, TRUE, PB_EMITTER_GI_PHOTONS },	// PB_EMITTER_GI_PHOTONS
	};		
static int numEmitterV02Params = sizeof ( emitterV02 ) / sizeof ( ParamBlockDescID );

#define LIGHT_VERSION 14		// current version
#define EMITTER_VERSION 2		// current version

ParamBlockDescID *descs[LIGHT_VERSION+1] = {
	  descV0, 
	  descV1, 
	  descV2, 
	  descV3, 
	  descV4,  
	  descV5, 
	  descV6,
	  descV7, 
	  descV8, 
	  descV9,
	  descV10,
	  descV11, 
	  descV12, 
	  descV13, 
	  descV14 
	  };	 

ParamBlockDescID *omniDescs[LIGHT_VERSION+1] = {
	  descV0,     //0
	  descV1,     //1
	  descV2,     //2
	  descV3Omni, //3	
	  descV4Omni, //4	
	  descV5Omni, //5	
	  descV6Omni, //6	
	  descV7Omni, //7	
	  descV9Omni, //8	
	  descV9Omni,  //9
	  descV10Omni, //10
	  descV11Omni,  //11
	  descV12Omni,  //12
	  descV13Omni,  //13
	  descV14Omni  //14
	  };	
	
static ParamVersionDesc emitterVersions[] = {
	ParamVersionDesc ( emitterV01, numEmitterV01Params, 1 )
};

static int numEmitterVersions = sizeof ( emitterVersions ) / sizeof ( ParamVersionDesc );

static ParamVersionDesc curEmitterVersion ( emitterV02, numEmitterV02Params, EMITTER_VERSION );

// number of parameters for different light types (omni, target spot, free directional, free spot, target directional )
static int pbDim1[5] = { 4,  9,  2,  9,  4};
static int pbDim2[5] = { 4, 13,  2, 13, 13};
static int pbDim3[5] = { 4, 12, 10, 13, 12};
static int pbDim4[5] = { 4, 11,  9, 12, 11};
static int pbDim5[5] = { 6, 13, 14, 14, 13};  
static int pbDim6[5] = { 10, 13, 14, 14, 13};  
static int pbDim7[5] = { 11, 14, 15, 15, 14};  
static int pbDim8[5] = { 12, 15, 16, 16, 15};  
static int pbDim9[5] = { 13, 16, 17, 17, 16};  
static int pbDim10[5] = { 14, 17, 18, 18, 17};  
static int pbDim11[5] = { 17, 20, 21, 21, 20};  
static int pbDim12[5] = { 13, 16, 17, 17, 16};  
static int pbDim13[5] = { 14, 17, 18, 18, 17};  
static int pbDim14[5] = { 15, 18, 19, 19, 18};  

static int *pbdims[LIGHT_VERSION+1] = {
	pbDim1,	pbDim1,	pbDim2, pbDim3,	pbDim4,
	pbDim5,	pbDim6,	pbDim7,	pbDim8,	pbDim9, 
	pbDim10, pbDim11, pbDim12, pbDim13, pbDim14
	};

static int GetDim(int vers, int type) {
	return pbdims[vers][type];
	}

static  ParamBlockDescID* GetDesc(int vers, int type) {
	return type == OMNI_LIGHT ? omniDescs[vers] : descs[vers];
	}

//====================================================================



void GeneralLight::BuildMeshes(BOOL isnew) {
	if( IsSpot() ) {
		if (isnew) {
			SetHotspot( TimeValue(0), dlgHotsize);
			SetFallsize( TimeValue(0), dlgFallsize);
			if(dlgTDist < 1.0f)
				SetTDist( TimeValue(0), DEF_TDIST);
			else
				SetTDist( TimeValue(0), dlgTDist);
			SetAspect( TimeValue(0), dlgAspect);
			coneDisplay = dlgShowCone;
			shape = dlgShape;
			}
		BuildSpotMesh(GetHotspot(TimeValue(0)));
		if(type == DIR_LIGHT||type == TDIR_LIGHT) {
			BuildStaticMeshes();
			mesh = &staticMesh[DIR_MESH];
			}
		else
			mesh = &spotMesh;
		}
	else {
		BuildStaticMeshes();
		mesh = &staticMesh[type == OMNI_LIGHT ? OMNI_MESH : DIR_MESH];
		}
	}

//static void NotifyPreSaveOld(void *param, NotifyInfo *info) {
//	((GeneralLight *)param)->PreSaveOld();
//	}

//static void NotifyPostSaveOld(void *param, NotifyInfo *info) {
//	((GeneralLight *)param)->PostSaveOld();
//	}

void GeneralLight::PreSaveOld() { 
//	if (GetSavingVersion()==2000) {
//		int oldver = 8;
//		temppb = UpdateParameterBlock(GetDesc(LIGHT_VERSION,type),GetDim(LIGHT_VERSION,type),
//			pblock,	GetDesc(oldver,type),GetDim(oldver,type), oldver);
//		}
	}

void GeneralLight::PostSaveOld() { 
//	if (temppb) {
//		temppb->DeleteThis();
//		temppb = NULL;
//		}
	}

struct NChange {
	TCHAR *oldname;
	TCHAR *newname;
	}; 

//static void NameChangeNotify(void *param, NotifyInfo *info) {
//	GeneralLight *lt = (GeneralLight *)param;
//	NChange *nc = (NChange *)info->callParam;
//	NameTab* nt = lt->GetExclList();
//	int i = nt->FindName(nc->oldname);
//	if (i>=0)  
//		nt->SetName(i,nc->newname);
//	}


static void NotifyDist(void *ptr, NotifyInfo *info) {
	GeneralLight *gl = (GeneralLight *)ptr;
	switch(info->intcode) {
		case NOTIFY_UNITS_CHANGE:
			const TCHAR *buf;
			buf = FormatUniverseValue(gl->targDist);
			SetWindowText(GetDlgItem(gl->hGeneralLight,IDC_TARG_DISTANCE),buf);
			break;
		}
	}



GeneralLight::GeneralLight(int tp)
{
	// Disable hold, macros and ref msgs.
	SuspendAll	xDisable(TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE);
	type = tp;
	temppb = pblock = emitter = NULL;
	projMap = shadProjMap = NULL;
	shadType = ASshadType = NULL;
	ReplaceReference(0, CreateParameterBlock( GetDesc(LIGHT_VERSION,type), GetDim(LIGHT_VERSION,type), LIGHT_VERSION ) );	
	ReplaceReference(EMITTER_REF, CreateParameterBlock( emitterV02, sizeof ( emitterV02 ) / sizeof ( ParamBlockDescID ), EMITTER_VERSION ));	

#ifdef USE_DLG_COLOR
	SetRGBColor( TimeValue(0), Point3(dlgRed/255.0f, dlgGreen/255.0f, dlgBlue/255.0f));
#else
	SetRGBColor( TimeValue(0), Point3(float(INITINTENS)/255.0f, float(INITINTENS)/255.0f, float(INITINTENS)/255.0f));
#endif
	SetShadColor( TimeValue(0), Point3(0,0,0));
	SetIntensity( TimeValue(0), dlgIntensity);	
	SetContrast( TimeValue(0), dlgContrast);
	SetDiffuseSoft( TimeValue(0), dlgDiffsoft);
	coneDisplay = 0;
	shape = CIRCLE_LIGHT;
	useLight = TRUE;
	attenDisplay = 0;
	decayDisplay = 0;
	useAtten = 0;
	attenNearDisplay = 0;
	useAttenNear = 0;
	enable=FALSE;
	shadow = GetMarketDefaults()->GetInt(
		LIGHT_CLASS_ID, Class_ID( OMNI_LIGHT_CLASS_ID, 0),
		_T("CastShadows"), 0) != 0;
	shadowType = 0;

	useGlobalShadowParams = FALSE;

	absMapBias = FALSE;	 // DS 7/27/98
	overshoot = FALSE;
	projector= FALSE;
	projMap = NULL;
    shadProjMap = NULL;

	ASshadType = shadType = NULL;

	void* s = CreateInstance(SHADOW_TYPE_CLASS_ID, dlgShadType);
	if (s == NULL)
		s = CreateInstance(SHADOW_TYPE_CLASS_ID, stdShadMap);
	ReplaceReference( SHADTYPE_REF, (ShadowType *)s);

	updateHSVSpin = TRUE;
	softenDiffuse = FALSE;
	affectDiffuse = TRUE;
	affectSpecular = TRUE;
	ambientOnly = FALSE;
	decayType = DECAY_NONE;
	ltAffectsShadow = FALSE; 
	targDist = 0.0f;

	SetAtten( TimeValue(0), ATTEN1_START, dlgAtten1Start);
	SetAtten( TimeValue(0), ATTEN1_END, dlgAtten1End);
	SetAtten( TimeValue(0), ATTEN_START, dlgAttenStart);
	SetAtten( TimeValue(0), ATTEN_END, dlgAttenEnd);
	attenDisplay = dlgShowAtten;
	attenNearDisplay = dlgShowAttenNear;
	decayDisplay = dlgShowDecay;
	useAtten = dlgUseAtten;

	SetDecayRadius( TimeValue(0), dlgDecayRadius);
	SetAtmosOpacity( TimeValue(0), dlgAtmosOpacity);
	SetAtmosColAmt( TimeValue(0), dlgAtmosColamt);
	SetAtmosShadows( TimeValue(0), dlgAtmosShadows);
	SetShadColor(TimeValue(0), Point3(float(dlgShadColor.r)/255.0f, float(dlgShadColor.g)/255.0f, float(dlgShadColor.b)/255.0f));
	SetShadMult( TimeValue(0), dlgShadMult);
	SetLightAffectsShadow(dlgLightAffectShadColor);
	SetUseGlobal(dlgUseGlobalShadowParams);

	BuildMeshes();	
	// this call determines whether the ui is consistent with what is expected
	// mainly this just checks if all the expected light types exist and are in the 
	// right position
	//CheckUIConsistency();

	RegisterNotification(&NotifyDist, (void *)this, NOTIFY_UNITS_CHANGE );

//	RegisterNotification(NotifyPreSaveOld, (void *)this, NOTIFY_FILE_PRE_SAVE_OLD);
//	RegisterNotification(NotifyPostSaveOld, (void *)this, NOTIFY_FILE_POST_SAVE_OLD);
//	RegisterNotification(NameChangeNotify, (void *)this, NOTIFY_NODE_RENAMED);
	}

//----------------------------------------------------------------
static BOOL HasTarg(int tp) {
	switch (tp) {
		case OMNI_LIGHT: return FALSE;
		case TSPOT_LIGHT: return TRUE;
		case DIR_LIGHT: return FALSE;
		case FSPOT_LIGHT: return FALSE;
		case TDIR_LIGHT: return TRUE;
		}
	return FALSE;
	}


//----------------------------------------------------------------

class SetTypeRest: public RestoreObj {
	public:
		GeneralLight *theLight;
		int oldtype,newtype;
		SingleRefMaker oldpb;
		SetTypeRest(GeneralLight *lt, int oldt, int newt, IParamBlock *opb) {
			theLight = lt;
			oldtype = oldt;
			newtype = newt;
			oldpb.SetRef(opb); // hang onto old param block.
			}
		~SetTypeRest() { }
		void SwapPBlocks();
		void SetTypeRest::Restore(int isUndo) {
			theLight->type = oldtype;
			SwapPBlocks();
			}
		void SetTypeRest::Redo() {
			theLight->type = newtype;
			SwapPBlocks();
			}
		int Size() { return 1; }
		virtual TSTR Description() { return _T("Set Light Type"); }
	};

void SetTypeRest::SwapPBlocks() {
	if (oldpb.GetRef()) {
		IParamBlock *pbsave = theLight->pblock;
		pbsave->SetAFlag(A_LOCK_TARGET);
		theLight->ReplaceReference( 0, oldpb.GetRef());	
		pbsave->ClearAFlag(A_LOCK_TARGET);
		oldpb.SetRef(pbsave);		
		}
	theLight->BuildMeshes(0);
	theLight->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	theLight->NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED); // to redraw modifier stack
	}



class ParamsRest: public RestoreObj {
	public:
		GeneralLight *theLight;
		BOOL showit;
		ParamsRest(GeneralLight *lt, BOOL show) {
			theLight = lt;
			showit = show;
			}
		void ParamsRest::Restore(int isUndo) {
			theLight->NotifyDependents(FOREVER, PART_OBJ, showit?REFMSG_END_MODIFY_PARAMS:REFMSG_BEGIN_MODIFY_PARAMS);
			}
		void ParamsRest::Redo() {
			theLight->NotifyDependents(FOREVER, PART_OBJ, showit?REFMSG_BEGIN_MODIFY_PARAMS:REFMSG_END_MODIFY_PARAMS);
			}
		int Size() { return 1; }
		virtual TSTR Description() { return _T("Light Params"); }
	};


/*----------------------------------------------------------------*/

BOOL GeneralLight::IsDaylightOrSunlightSystem() {
	if (!HasTarg(type)) {
		DependentIterator di(this);
		ReferenceMaker *rm = NULL;
		// iterate through the instances
		while ((rm=di.Next()) != NULL) {	
			INode *nd = GetNodeRef(rm);
			if (nd) {
				if (nd->GetTMController()->ClassID()==Class_ID(DAYLIGHT_SLAVE_CONTROL_CID1,DAYLIGHT_SLAVE_CONTROL_CID2) // Daylight system
					|| nd->GetTMController()->ClassID()==Class_ID(SLAVE_CONTROL_CID1,SLAVE_CONTROL_CID2)) //Sunlight system
				return TRUE;
				}
			}
		}
	return FALSE;
	}

void GeneralLight::SetType(int tp) {     
	if (type == tp) 
		return;

	BOOL oldHasTarg = HasTarg(type);
	BOOL newHasTarg = HasTarg(tp);

	// This is here to notify the viewports that the light type is changing.
	// It has to come before the change so that the viewport parameters don't
	// get screwed up.		// DB 4/29/99
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_OBREF_CHANGE);


	Interface *iface = GetCOREInterface();
	TimeValue t = iface->GetTime();
	INode *nd = FindNodeRef(this);
	if (nd==NULL) 
		return;

	BOOL paramsShowing = FALSE;
	if (hGeneralLight && (currentEditLight == this)) { // LAM - 8/13/02 - defect 511609
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_END_MODIFY_PARAMS);
		paramsShowing = TRUE;
		if (theHold.Holding()) 
			theHold.Put(new ParamsRest(this,0));
		}

	// bug fix
	// when changing the type of a light, this variable needs to be reinitialized or else
	// the shadow generator selector will be locked into its default (Shadow Map)
//	ASshadType = NULL;
// >>>>>the above was commented out when i went to make ASShadType a reference
// not sure whether i shd add to new version of the commented line, which is
// ReplaceReference( AS_SHADTYPE_REF, NULL );

	int oldtype = type;
	Interval v;
	float tdist = GetTDist(t,v);
	if (oldtype==OMNI_LIGHT) 
		tdist = 100.0f;
	else if(HasTarg(oldtype))
		tdist = targDist;
	type = tp;
	int *pbd = pbdims[LIGHT_VERSION];
	IParamBlock *pbnew = NULL;
	if (pbd[oldtype]!=pbd[type]) {
		pbnew = UpdateParameterBlock(
			GetDesc(LIGHT_VERSION,oldtype), GetDim(LIGHT_VERSION, oldtype), pblock,
			GetDesc(LIGHT_VERSION,type), GetDim(LIGHT_VERSION,type), LIGHT_VERSION);
		//ReplaceReference( 0, pbnew);	
		}

	if (theHold.Holding()) {
		theHold.Put(new SetTypeRest(this, oldtype,tp, pbnew?pblock:NULL));
		if (pbnew) {
			theHold.Suspend();
			ReplaceReference( 0, pbnew);	
			theHold.Resume();
			}
		}
	else if (pbnew) {
			ReplaceReference( 0, pbnew);	
		}


	HWND hwgl = hGeneralLight;
	hGeneralLight = NULL; // This keeps UpdateUI from jumping in
	
	BuildMeshes(oldtype==OMNI_LIGHT?1:0);

	hGeneralLight = hwgl;

	if (oldHasTarg && !newHasTarg) {
		// get rid of target, assign a PRS controller for all instances
		DependentIterator di(this);
		ReferenceMaker *rm = NULL;
		// iterate through the instances
		while ((rm=di.Next()) != NULL) {
			nd = GetNodeRef(rm);
			if (nd) {
				INode* tn = nd->GetTarget(); 
				Matrix3 tm = nd->GetNodeTM(0);
				if (tn) 
					iface->DeleteNode(tn);
				Control *tmc = NewDefaultMatrix3Controller();
				tmc->Copy(nd->GetTMController()); // didn't work!!?
				nd->SetTMController(tmc);
				nd->SetNodeTM(0,tm);
				SetTDist(t,tdist);	 //?? which one should this be for
				}
			}

		
		}
	if (newHasTarg && !oldHasTarg) {
		DependentIterator di(this);
		ReferenceMaker *rm = NULL;
		// iterate through the instances
		while ((rm=di.Next()) != NULL) {	
			nd = GetNodeRef(rm);
			if (nd) {

				// > 9/9/02 - 2:17pm --MQM-- bug #435799
				// we were losing the light wire color when deleting/restoring the target.
				// (basically, the target had priority on color, so it forced the light
				// wire color to be set to the new target color).
				Color lightWireColor(nd->GetWireColor());

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
				Control *laControl= CreateLookatControl();
				targNode->SetNodeTM(0,targtm);
				laControl->SetTarget(targNode);
				laControl->Copy(nd->GetTMController());
				nd->SetTMController(laControl);
				targNode->SetIsTarget(1);   

				// > 9/9/02 - 2:19pm --MQM-- 
				// force color of the new target...
				targNode->SetWireColor( lightWireColor.toRGB() );
				}
			}
		}

	if (IsSpot()&&!IsDir()) {
		// make sure hotspot and fallof are properly constrained.
		SuspendAnimate();
		SetHotspot(t, GetHotspot(t,FOREVER));
		SetFallsize(t, GetFallsize(t,FOREVER));
		ResumeAnimate();
		}

	if (paramsShowing) {
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_BEGIN_MODIFY_PARAMS);
		if (theHold.Holding()) 
			theHold.Put(new ParamsRest(this,1));
		}


	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED); // to redraw modifier stack
	iface->RedrawViews(t);
	}

//----------------------------------------------------------------

class ShadTypeRest: public RestoreObj, public ReferenceMaker {
	public:
		GeneralLight *theLight;
		ShadowType *oldtype,*newtype;
		ShadTypeRest(GeneralLight *lt, ShadowType *s) {
			oldtype = newtype = NULL;
			theLight = lt;
			theHold.Suspend();
			ReplaceReference(0,lt->GetShadowGenerator());
			ReplaceReference(1,s);
			theHold.Resume();
			}
		~ShadTypeRest() {
			DeleteAllRefsFromMe();
			}
		void Restore(int isUndo);
		void Redo();
		int Size() { return 1; }
		TSTR Description() { return _T("Set Shadow Type"); }

		virtual void GetClassName(MSTR& s) { s = _M("ShadTypeRest"); }

		// ReferenceMaker 
		RefResult NotifyRefChanged( const Interval& changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message, BOOL propagate ) { 
			if (message==REFMSG_TARGET_DELETED) {
				if (hTarget==oldtype) 
				    oldtype = NULL;
				if (hTarget==newtype) 
				    newtype = NULL;
				}
		   	return REF_SUCCEED; 
		   	}
		int NumRefs() { return 2; }
		RefTargetHandle GetReference(int i) { return i==0?oldtype:newtype; }
private:
		virtual void SetReference(int i, RefTargetHandle rtarg) { 
			if (i==0) oldtype = (ShadowType *)rtarg; else newtype = (ShadowType *)rtarg;
			}
public:
		BOOL CanTransferReference(int i) { return FALSE;}
	};


void ShadTypeRest::Restore(int isUndo) {
	theHold.Suspend();
	theLight->SetShadowGenerator(oldtype);
	theHold.Resume();
	}

void ShadTypeRest::Redo() {
	theHold.Suspend();
	theLight->SetShadowGenerator(newtype);
	theHold.Resume();
	}

static int ShadTypeFromGen(ShadowType *s) {	
	if (s) {
		if (s->ClassID()==Class_ID(STD_SHADOW_MAP_CLASS_ID,0))
			return 0;
		if (s->ClassID()==Class_ID(STD_RAYTRACE_SHADOW_CLASS_ID,0))
			return 1;
		else
			return 0xFFFF;
		}
	return -1;
	}

int GeneralLight::GetShadowType() { 
	return useGlobalShadowParams? ShadTypeFromGen(GetCOREInterface()->GetGlobalShadowGenerator()):
		 ShadTypeFromGen(shadType); 
	}

void GeneralLight::SetShadowType(int a) { 
	ShadowType *s = a ?NewDefaultRayShadowType() : NewDefaultShadowMapType();
	if (useGlobalShadowParams) {
		GetCOREInterface()->SetGlobalShadowGenerator(s); 	
		globShadowType = a; 
		}
	else {
		SetShadowGenerator(s); 	
		shadowType = a;
		}

	// 5/15/01 11:48am --MQM-- 
	macroRec->SetSelProperty( _T("raytracedShadows"), mr_bool, a );
	macroRec->Disable();
	CheckRadioButton( hGeneralLight, IDC_SHADOW_MAPS, IDC_RAY_TRACED_SHADOWS, IDC_SHADOW_MAPS + GetShadowType() );
	macroRec->Enable();
	}

/*----------------------------------------------------------------*/
void GeneralLight::SetShadowGenerator(ShadowType *s) {
	if (s==NULL) 
		return; 
	Interface *ip = GetCOREInterface();
	if (theHold.Holding())
		theHold.Put(new ShadTypeRest(this,s));
	theHold.Suspend();

	if (hGeneralLight && (currentEditLight == this)) { // LAM - 8/13/02 - defect 511609
		if (!inCreate)
			ip->DeleteSFXRollupPage();
		shadParamDlg->DeleteThis(); 
		}
	if (useGlobalShadowParams) 
		ip->SetGlobalShadowGenerator(s);
	else 
		ReplaceReference(SHADTYPE_REF,s);
	if (hGeneralLight && (currentEditLight == this)) { // LAM - 8/13/02 - defect 511609
		
		shadParamDlg = ActiveShadowType()->CreateShadowParamDlg(ip);  
		if (!inCreate)
			ip->AddSFXRollupPage();
		UpdtShadowTypeSel();
		}
	shadowType = ShadTypeFromGen(shadType);
	NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);	
	theHold.Resume();
	}

void GeneralLight::SetUseGlobal(int a) { 
	if (useGlobalShadowParams==a) return;
	SimpleLightUndo< &GeneralLight::SetUseGlobal, false >::Hold(this, a, useGlobalShadowParams);
	if (currentEditLight==this)  // LAM - 8/13/02 - defect 511609
		UpdateUICheckbox( hGeneralLight, IDC_GLOBAL_SET, _T("useGlobalShadowSettings"), a ); // 5/15/01 11:35am --MQM--  
	Interface *ip = GetCOREInterface();
	if (hGeneralLight && (currentEditLight==this)) {
		if (!inCreate)
			ip->DeleteSFXRollupPage();
		shadParamDlg->DeleteThis(); 
		}
	useGlobalShadowParams =a; 
	if (hGeneralLight && (currentEditLight==this)) {
		shadParamDlg = ActiveShadowType()->CreateShadowParamDlg(ip);  
		if (!inCreate)
			ip->AddSFXRollupPage();
		UpdtShadowTypeList(hGeneralLight);
		}
	}

ShadowType * GeneralLight::GetShadowGenerator()
{
	return ActiveShadowType();
}

ShadowType * GeneralLight::ActiveShadowType() { 

   if(useGlobalShadowParams) {
		Interface *ip = GetCOREInterface();
		return ip->GetGlobalShadowGenerator();		
	} else 
		return shadType; 
	}


BOOL GeneralLight::SetHotSpotControl(Control *c)
	{
	pblock->SetController(PB_HOTSIZE,c);
	return TRUE;
	}

BOOL GeneralLight::SetFalloffControl(Control *c)
	{
	pblock->SetController(PB_FALLSIZE,c);
	return TRUE;
	}

BOOL GeneralLight::SetColorControl(Control *c)
	{
	pblock->SetController(PB_COLOR,c);
	return TRUE;
	}


Control* GeneralLight::GetHotSpotControl(){
	return	pblock->GetController(PB_HOTSIZE);
	}

Control* GeneralLight::GetFalloffControl(){
	return	pblock->GetController(PB_FALLSIZE);
	}

Control* GeneralLight::GetColorControl() {
	return	pblock->GetController(PB_COLOR);
	}

void GeneralLight::SetIntensity(TimeValue t, float f)
{
	pblock->SetValue( PB_INTENSITY, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float GeneralLight::GetIntensity(TimeValue t, Interval& valid)
{
	float f;
	pblock->GetValue( PB_INTENSITY, t, f, valid );
	return f;
}



void GeneralLight::SetContrast(TimeValue t, float f)
{
	pblock->SetValue( PB_CONTRAST, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float GeneralLight::GetContrast(TimeValue t, Interval& valid)
{
	float f;
	pblock->GetValue( PB_CONTRAST, t, f, valid );
	return f;
}

// these are 0...100 UI level coordinates, converted to 0..1 diffSoft by update
void GeneralLight::SetDiffuseSoft(TimeValue t, float f)
{
	pblock->SetValue( PB_DIFFSOFT, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float GeneralLight::GetDiffuseSoft(TimeValue t, Interval& valid)
{
	float f;
	pblock->GetValue( PB_DIFFSOFT, t, f, valid );
	return f;
}


void GeneralLight::SetHotspot(TimeValue t, float f)
{
	if(!IsSpot())
		return;
	if(f < 0.5f)
		f = 0.5f;
	if(!IsDir() && (f > 179.5f))
		f = 179.5f;
	pblock->SetValue( PB_HOTSIZE, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float GeneralLight::GetHotspot(TimeValue t, Interval& valid)
{
	Interval iv;

	if(!IsSpot())
		return -1.0f;
	float f;
	pblock->GetValue( PB_HOTSIZE, t, f, valid );
	if(GetFallsize(t, iv) < f )
		return GetFallsize(t, iv);
	return f;
}

void GeneralLight::SetFallsize(TimeValue t, float f)
{
	if(!IsSpot())
		return;
	if(f < 0.5f)
		f = 0.5f;
	if(!IsDir() && (f > 179.5f))
		f = 179.5f;
	pblock->SetValue( PB_FALLSIZE, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float GeneralLight::GetFallsize(TimeValue t, Interval& valid)
{
	if(!IsSpot())
		return -1.0f;
	float f;
	pblock->GetValue( PB_FALLSIZE, t, f, valid );
	return f;
}

void GeneralLight::SetAtten(TimeValue t, int which, float f)
{
//	if(type == DIR_LIGHT)
//		return;
	pblock->SetValue( (type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1) + which, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float GeneralLight::GetAtten(TimeValue t, int which, Interval& valid)
{
//	if(type == DIR_LIGHT)
//		return -1.0f;
	float f;
	pblock->GetValue( (type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1) + which, t, f, valid );
	return f;
}

void GeneralLight::SetTDist(TimeValue t, float f)
{
	if(type != FSPOT_LIGHT&&type != DIR_LIGHT)
		return;
	pblock->SetValue( PB_TDIST, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float GeneralLight::GetTDist(TimeValue t, Interval& valid)
{
	if(!IsSpot())
		return -1.0f;
	if( type == TDIR_LIGHT || type == TSPOT_LIGHT)
		return 0.0f;
	float f;
	pblock->GetValue( PB_TDIST, t, f, valid );
	return f;
}

void GeneralLight::SetDecayRadius(TimeValue t, float f)
	{
	pblock->SetValue( type==OMNI_LIGHT? PB_OMNIDECAY: PB_DECAY, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

float GeneralLight::GetDecayRadius(TimeValue t, Interval& valid)
	{
	float f;
	pblock->GetValue( type==OMNI_LIGHT? PB_OMNIDECAY: PB_DECAY, t, f, valid );
	return f;
	}


void GeneralLight::SetRGBColor(TimeValue t, Point3 &rgb) 
{
	pblock->SetValue( PB_COLOR, t, rgb );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

Point3 GeneralLight::GetRGBColor(TimeValue t, Interval& valid) 
{	
	Point3 rgb;
	pblock->GetValue( PB_COLOR, t, rgb, valid );
	return rgb;
}

void GeneralLight::SetShadColor(TimeValue t, Point3 &rgb) 
	{
	pblock->SetValue( type == OMNI_LIGHT ? PB_OMNISHADCOLOR : PB_SHADCOLOR, t, rgb );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

Point3 GeneralLight::GetShadColor(TimeValue t, Interval& valid) 
	{	
	Point3 rgb;
	pblock->GetValue( type == OMNI_LIGHT ?PB_OMNISHADCOLOR : PB_SHADCOLOR, t, rgb, valid );
	return rgb;
	}

void GeneralLight::SetShadMult(TimeValue t, float m) {
	pblock->SetValue( PB_SHAD_MULT(this), t, m );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}
	
float GeneralLight::GetShadMult(TimeValue t, Interval &valid) {
	float f;
	pblock->GetValue( PB_SHAD_MULT(this), t, f, valid );
	return f;
	}

void GeneralLight::SetHSVColor(TimeValue t, Point3& hsv) 
{
	DWORD rgb = HSVtoRGB ((int)(hsv[0]*255.0f), (int)(hsv[1]*255.0f), (int)(hsv[2]*255.0f));
	Point3 rgbf;
	rgbf[0] = GetRValue(rgb) / 255.0f;
	rgbf[1] = GetGValue(rgb) / 255.0f;
	rgbf[2] = GetBValue(rgb) / 255.0f;
	SetRGBColor(t, rgbf);
}

Point3 GeneralLight::GetHSVColor(TimeValue t, Interval& valid) 
{	
	int h, s, v;
	Point3 rgbf = GetRGBColor(t, valid);
	DWORD rgb = RGB((int)(rgbf[0]*255.0f), (int)(rgbf[1]*255.0f), (int)(rgbf[2]*255.0f));
	RGBtoHSV (rgb, &h, &s, &v);
	return Point3(h/255.0f, s/255.0f, v/255.0f);
}

void GeneralLight::SetUseLight(int onOff)
{
	SimpleLightUndo< &GeneralLight::SetUseLight, false >::Hold(this, onOff, useLight);
	useLight = onOff;
	if (currentEditLight==this)  // LAM - 8/13/02 - defect 511609
		UpdateUICheckbox( hGeneralLight, IDC_LIGHT_ON, _T("enabled"), onOff ); // 5/15/01 11:00am --MQM-- maxscript fix
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}


void GeneralLight::SetConeDisplay(int s, int notify) 
{
	DualLightUndo< &GeneralLight::SetConeDisplay, TRUE, false >::Hold(this, s, coneDisplay);
	coneDisplay = s;
	if(notify && IsSpot())
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

void GeneralLight::SetOvershoot(int a)
{
	SimpleLightUndo< &GeneralLight::SetOvershoot, false >::Hold(this, a, overshoot);
	overshoot = a;
	if(IsSpot()) {
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
		if (currentEditLight==this) {
			if (hotsizeSpin)
				hotsizeSpin->Enable(!overshoot);
		}
	}
}

void GeneralLight::SetSpotShape(int s)
{
	SimpleLightUndo< &GeneralLight::SetSpotShape, false >::Hold(this, s, shape);
	shape = s;
	if(IsSpot()) {
		if (currentEditLight==this) {
			CheckRadioButton(hSpotLight, IDC_RECT_LIGHT, IDC_CIRCLE_LIGHT,
				shape == RECT_LIGHT ? IDC_RECT_LIGHT : IDC_CIRCLE_LIGHT );
			EnableWindow(GetDlgItem(hSpotLight, IDC_LASPECT), !shape);
			EnableWindow(GetDlgItem(hSpotLight, IDC_LASPECT_SPIN), !shape);
			EnableWindow(GetDlgItem(hSpotLight, IDC_BITMAP_FIT), !shape);
			UpdateUI(GetCOREInterface()->GetTime());
		}
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}
}

void GeneralLight::SetAspect(TimeValue t, float f)
{
	if(!IsSpot())
		return;
	pblock->SetValue( PB_ASPECT, t, f );
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float GeneralLight::GetAspect(TimeValue t, Interval& valid)
{
	if(!IsSpot())
		return -1.0f;
	float f;
	pblock->GetValue( PB_ASPECT, t, f, valid );
	return f;
}

void GeneralLight::SetMapBias(TimeValue t, float f) {
	ActiveShadowType()->SetMapBias(t,f);
	}

float GeneralLight::GetMapBias(TimeValue t, Interval& valid) {
	return ActiveShadowType()->GetMapBias(t,valid);
	}

void GeneralLight::SetMapRange(TimeValue t, float f) {
	ActiveShadowType()->SetMapRange(t,f);
	}

float GeneralLight::GetMapRange(TimeValue t, Interval& valid){
	return ActiveShadowType()->GetMapRange(t,valid);
	}

void GeneralLight::SetMapSize(TimeValue t, int f) {
	ActiveShadowType()->SetMapSize(t,f);
	}

int GeneralLight::GetMapSize(TimeValue t, Interval& v){
	return ActiveShadowType()->GetMapSize(t,v);
	}

void GeneralLight::SetRayBias(TimeValue t, float f) {
	ActiveShadowType()->SetRayBias(t,f);
	}

float GeneralLight::GetRayBias(TimeValue t, Interval& v){
	return ActiveShadowType()->GetRayBias(t,v);
	}

void GeneralLight::SetAtmosShadows(TimeValue t, int onOff)
	{
	SimpleLightUndoTime< &GeneralLight::SetAtmosShadows, false >::Hold(this, t, onOff, atmosShadows);
	atmosShadows = onOff;
	if (currentEditLight==this)  // LAM - 8/13/02 - defect 511609
		UpdateUICheckbox( hGeneralLight, IDC_ATMOS_SHADOWS, _T("atmosShadows"), onOff ); 
	macroRec->Disable();										// 5/22/01 10:47am --MQM-- UpdateUICheckbox is taking care of the macro recorder output
	pblock->SetValue( PB_ATM_SHAD(this), t, onOff );
	macroRec->Enable();
	}

int GeneralLight::GetAtmosShadows(TimeValue t)
	{
	int onOff;
	Interval valid = Interval(0,0);
	pblock->GetValue( PB_ATM_SHAD(this), t, onOff, valid );
	return onOff;
	}

void GeneralLight::SetUseShadowColorMap(TimeValue t, int onOff) {
	pblock->SetValue( PB_USE_SHAD_COLMAP(this), t, onOff );
	}

int GeneralLight::GetUseShadowColorMap(TimeValue t) {
	int onOff;
	Interval valid = Interval(0,0);
	pblock->GetValue( PB_USE_SHAD_COLMAP(this), t, onOff, valid );
	return onOff;
	}

void GeneralLight::SetAtmosOpacity(TimeValue t, float f)
	{
	atmosOpacity = f;
//	if (useGlobalShadowParams)  globAtmosOpacity = f;
//	else 
		pblock->SetValue( PB_ATM_OPAC(this), t, f );
	}

float GeneralLight::GetAtmosOpacity(TimeValue t, Interval& valid)
{
	float f;
//	if (useGlobalShadowParams)  return globAtmosOpacity;
	pblock->GetValue( PB_ATM_OPAC(this), t, f, valid );
	return f;
}

void GeneralLight::SetAtmosColAmt(TimeValue t, float f)
{
	atmosColAmt = f;
//	if (useGlobalShadowParams)  globAtmosColamt = f;
//	else 
		pblock->SetValue( PB_ATM_COLAMT(this), t, f );
}

float GeneralLight::GetAtmosColAmt(TimeValue t, Interval& valid)
{
	float f;
//	if (useGlobalShadowParams)  return globAtmosColamt;
	pblock->GetValue( PB_ATM_COLAMT(this), t, f, valid );
	return f;
}
 
// LAM - 8/13/02 - defect 511609 - moved following Sets in from header due to currentEditLight access
void GeneralLight::SetLightAffectsShadow( BOOL b ) 
{
	SimpleLightUndo< &GeneralLight::SetLightAffectsShadow, true >::Hold(this, b, ltAffectsShadow);
	ltAffectsShadow = b;
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_LT_EFFECT_SHADCOL, _T("lightAffectsShadow"), b ); 
}

void GeneralLight::SetShadow( int a ) 				
{
	SimpleLightUndo< &GeneralLight::SetShadow, true >::Hold(this, a, shadow);
	shadow = a;
	if (currentEditLight==this) {
		UpdateUICheckbox( hGeneralLight, IDC_CAST_SHADOWS, _T("castShadows"), a ); 
		UpdateUICheckbox( hShadow, IDC_OBJECT_SHADOWS, NULL, a ); 
	}
} 

void GeneralLight::SetAbsMapBias( int a ) 			
{
	SimpleLightUndo< &GeneralLight::SetAbsMapBias, true >::Hold(this, a, useGlobalShadowParams ? globAbsMapBias : absMapBias);
	if ( useGlobalShadowParams )  
		globAbsMapBias = a;   
	else 
		absMapBias = a;     
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_ABS_MAP_BIAS, _T("absoluteMapBias"), a ); 
}

void GeneralLight::SetProjector( int a )			
{
	SimpleLightUndo< &GeneralLight::SetProjector, true >::Hold(this, a, projector);
	projector = a;
	if (currentEditLight==this) 
		UpdateUICheckbox( hAdvEff, IDC_PROJECTOR, _T("projector"), a ); 
}

void GeneralLight::SetAffectDiffuse( BOOL onOff ) 	
{
	SimpleLightUndo< &GeneralLight::SetAffectDiffuse, true >::Hold(this, onOff, affectDiffuse);
	affectDiffuse = onOff;
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_AFFECT_DIFFUSE, _T("affectDiffuse"), onOff ); 
} // 5/14/01 5:47pm --MQM-- maxscript fixes (update GUI on change + output maxscript)

void GeneralLight::SetAffectSpecular( BOOL onOff ) 
{
	SimpleLightUndo< &GeneralLight::SetAffectSpecular, true >::Hold(this, onOff, affectSpecular);
	affectSpecular = onOff;
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_AFFECT_SPECULAR, _T("affectSpecular"), onOff ); 
}

void GeneralLight::SetAmbientOnly( BOOL onOff ) 	
{
	SimpleLightUndo< &GeneralLight::SetAmbientOnly, true >::Hold(this, onOff, ambientOnly);
	ambientOnly = onOff;
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_AMBIENT_ONLY, _T("ambientOnly"), onOff ); 
}


// aspect <= 0   ==>  circular cone	(NUM_CIRC_PTS returned)
// aspect  > 0	 ==>  rectangular cone (4+2 points returned)
void GeneralLight::GetConePoints(TimeValue t, float aspect, float angle, float dist, Point3 *q) 
	{
	float ta = (float)tan(0.5*DegToRad(angle));   
	if(aspect <= 0.0f) { 
		// CIRCULAR
		float rad = dist * ta;
		double a;
		if(IsDir())
			rad = angle;
      int i;
      for(i = 0; i < NUM_CIRC_PTS; i++) {
			a = (double)i * 2.0 * 3.1415926 / (double)NUM_CIRC_PTS;
			q[i] = Point3(rad*(float)sin(a), rad*(float)cos(a), -dist);
			}
        // alexc - june.12.2003 - made Roll Angle Indicator size proportional to radius
		q[i] = q[0];
        q[i].y *= 1.15f; // notch length is 15% of the radius
		}
	else {		 
		// RECTANGULAR
		float w = IsDir()? angle : dist * ta * (float)sqrt((double)aspect);
		float h = w / aspect;
		q[0] = Point3( w, h,-dist);				
		q[1] = Point3(-w, h,-dist);				
		q[2] = Point3(-w,-h,-dist);				
		q[3] = Point3( w,-h,-dist);
		q[4] = Point3( 0.0f, h+15.0f, -dist);
		q[5] = Point3( 0.0f, h, -dist);
		}
	}

#define HOTCONE		0
#define FALLCONE	1

void GeneralLight::DrawCone(TimeValue t, GraphicsWindow *gw, float dist) 
	{
	Point3 q[NUM_CIRC_PTS+1], u[3];
	int dirLight = IsDir();
	int i;
	BOOL dispAtten = (useAtten && (extDispFlags & EXT_DISP_ONLY_SELECTED))?TRUE:attenDisplay;
	BOOL dispAttenNear = (useAttenNear && (extDispFlags & EXT_DISP_ONLY_SELECTED))?TRUE:attenNearDisplay;
	BOOL dispDecay = (GetDecayType()&&(extDispFlags & EXT_DISP_ONLY_SELECTED))?TRUE:decayDisplay;

	GetConePoints(t, GetSpotShape() ? -1.0f : GetAspect(t), GetHotspot(t), dist, q);
	gw->setColor( LINE_COLOR, GetUIColor(COLOR_HOTSPOT));
	if(GetSpotShape()) {  
		// CIRCULAR
		if(GetHotspot(t) >= GetFallsize(t)) {
			// draw (far) hotspot circle
			u[0] = q[0];
			u[1] = q[NUM_CIRC_PTS];
			gw->polyline( 2, u, NULL, NULL, FALSE, NULL);
			}
		gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
		if (dirLight) {
			// draw 4 axial hotspot lines
			for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
				u[0] =  q[i]; 	u[1] =  q[i]; u[1].z += dist;
				gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
				}
			GetConePoints(t, -1.0f, GetHotspot(t), 0.0f, q);
			// draw (near) hotspot circle
			gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
			}
		else  {
			// draw 4 axial lines
			u[0] = Point3(0,0,0);
			for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
				u[1] =  q[i];
				gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
				}
			}
	
		GetConePoints(t, -1.0f, GetFallsize(t), dist, q);
		gw->setColor( LINE_COLOR, GetUIColor(COLOR_FALLOFF));
		if(GetHotspot(t) < GetFallsize(t)) {
			// draw (far) fallsize circle
			u[0] = q[0];	u[1] = q[NUM_CIRC_PTS];
			gw->polyline( 2, u, NULL, NULL, FALSE, NULL);
			u[0] = Point3(0,0,0);
			}
		gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
		if (dirLight) {
			float dfar = q[0].z;
			float dnear = 0.0f;
			if (dispAtten) {
				dfar  = MinF(-GetAtten(t,ATTEN_END),dfar);
				dnear = MaxF(-GetAtten(t,ATTEN_START),dnear);
				}
			if (dispAttenNear) {
				dfar  = MinF(-GetAtten(t,ATTEN1_END),dfar);
				dnear = MaxF(-GetAtten(t,ATTEN1_START),dnear);
				}
			if (dispDecay) {
				dfar  = MinF(-GetDecayRadius(t),dfar);
				}
			// draw axial fallsize lines
			for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
				u[0] =  q[i];  u[0].z = dfar;	u[1] =  q[i]; u[1].z = dnear;
				gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
				}
			GetConePoints(t, -1.0f, GetFallsize(t), 0.0f, q);
			// draw (near) fallsize circle
			gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
			}
		else {
			float cs = (float)cos(DegToRad(GetFallsize(t)*0.5f));
			float dfar = q[0].z;
			if (dispAtten) 
				dfar  = MinF(-cs*GetAtten(t,ATTEN_END),dfar);
			if (dispAttenNear) 
				dfar  = MinF(-cs*GetAtten(t,ATTEN1_END),dfar);
			if (dispDecay) 
				dfar  = MinF(-cs*GetDecayRadius(t),dfar);
			for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
            //Give a default Point3 instead of a division by zero
            u[1] = (dist!=0.0f ? -q[i]*dfar/dist : Point3(0.0f, 0.0f, 0.0f) );
				gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
				}
			}
		}
	else { 
		// RECTANGULAR
		if(GetHotspot(t) >= GetFallsize(t))
			gw->polyline( 2, q+4, NULL, NULL, FALSE, NULL);
		gw->polyline(4, q, NULL, NULL, TRUE, NULL);
		if (dirLight) { //DIRLIGHT
			// draw axial hotspot lines
			for (i = 0; i < 4; i += 1) {
				u[0] =  q[i]; 	u[1] =  q[i]; u[1].z += dist;
				gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
				}
			GetConePoints(t, GetAspect(t), GetHotspot(t), 0.0f, q);
			// draw (near) hotspot circle
			gw->polyline(4, q, NULL, NULL, TRUE, NULL);
			}
		else {
			u[0] = Point3(0,0,0);
			for (i = 0; i < 4; i++) {
				u[1] =  q[i];
				gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
				}
			}
		GetConePoints(t, GetAspect(t), GetFallsize(t), dist, q);
		gw->setColor( LINE_COLOR, GetUIColor(COLOR_FALLOFF));
		if(GetHotspot(t) < GetFallsize(t))
			gw->polyline( 2, q+4, NULL, NULL, FALSE, NULL);
		gw->polyline(4, q, NULL, NULL, TRUE, NULL);
		if (dirLight) {
			// draw axial fallsize lines
			for (i = 0; i < 4; i += 1) {
				u[0] =  q[i]; 	u[1] =  q[i]; u[1].z += dist;
				gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
				}
			GetConePoints(t, GetAspect(t), GetFallsize(t), 0.0f, q);
			// draw (near) fallsize circle
			gw->polyline(4, q, NULL, NULL, TRUE, NULL);
			}
		else {
			for (i = 0; i < 4; i += 1) {
				u[1] =  q[i];
				gw->polyline( 2, u, NULL, NULL, FALSE, NULL );	
				}
			}
		}
	}

int GeneralLight::DrawConeAndLine(TimeValue t, INode* inode, GraphicsWindow *gw, int drawing ) 
	{
	if(!IsSpot())
		return 0;
	Matrix3 tm = inode->GetObjectTM(t);
	gw->setTransform(tm);
	gw->clearHitCode();
	if (type == TSPOT_LIGHT) {
		Point3 pt,v[3];
		if (GetTargetPoint(t, inode, pt)) {
			float den = FLength(tm.GetRow(2));
			float dist = (den!=0) ? FLength(tm.GetTrans()-pt) / den : 0.0f;
			targDist = dist;
			if (hSpotLight && (currentEditLight==this)) { // LAM - 8/13/02 - defect 511609
					const TCHAR *buf = FormatUniverseValue(targDist);
					SetWindowText(GetDlgItem(hGeneralLight,IDC_TARG_DISTANCE),buf);
				}
			if ((drawing != -1) && (coneDisplay || (extDispFlags & EXT_DISP_ONLY_SELECTED))) 
				DrawCone(t, gw, dist);
			if(!inode->IsFrozen() && !inode->Dependent())
			{
				// 6/22/01 2:18pm --MQM-- 
				// if the user has changed the light's wire-frame color,
				// use that color to draw the target line.
				// otherwise, use the standard target-line color.
				Color color(inode->GetWireColor());
				if ( color != GetUIColor(COLOR_LIGHT_OBJ) )
					gw->setColor( LINE_COLOR, color );
				else
					gw->setColor( LINE_COLOR, GetUIColor(COLOR_TARGET_LINE)); // old method
			}
			v[0] = Point3(0,0,0);
			v[1] = Point3(0.0f, 0.0f, (drawing == -1)? (-0.9f * dist): -dist);
			gw->polyline( 2, v, NULL, NULL, FALSE, NULL );	
			}
		}
	else if (type == FSPOT_LIGHT) {
		if ((drawing != -1) && (coneDisplay || (extDispFlags & EXT_DISP_ONLY_SELECTED)))
			DrawCone(t, gw, GetTDist(t));
		}
	else if (type == DIR_LIGHT) {
		if ((drawing != -1) && (coneDisplay || (extDispFlags & EXT_DISP_ONLY_SELECTED)))
			DrawCone(t, gw, GetTDist(t));
		}
	else if (type == TDIR_LIGHT) {
		Point3 pt,v[3];
		if (GetTargetPoint(t, inode, pt)) {
			float den = FLength(tm.GetRow(2));
			float dist = (den!=0) ? FLength(tm.GetTrans()-pt) / den : 0.0f;
			targDist = dist;
			if (hSpotLight && (currentEditLight==this)) { // LAM - 8/13/02 - defect 511609
				const TCHAR *buf = FormatUniverseValue(targDist);
				SetWindowText(GetDlgItem(hGeneralLight,IDC_TARG_DISTANCE),buf);
				}
			if ((drawing != -1) && (coneDisplay || (extDispFlags & EXT_DISP_ONLY_SELECTED)))
				DrawCone(t, gw, dist);

			// > 6/12/02 - 8:3am --MQM-- 
			// target directs should also draw the target line in the regular cyan color.
			Color color(inode->GetWireColor());
			if ( color != GetUIColor(COLOR_LIGHT_OBJ) )
				gw->setColor( LINE_COLOR, color );
			else
				gw->setColor( LINE_COLOR, GetUIColor(COLOR_TARGET_LINE) );

			v[0] = Point3(0,0,0);
			v[1] = Point3(0.0f, 0.0f, (drawing == -1)? (-0.9f * dist): -dist);
			gw->polyline( 2, v, NULL, NULL, FALSE, NULL );	
			}
		}
	return gw->checkHitCode();
	}

 // LAM - 8/13/02 - defect 511609 - added (currentEditLight==this) test to following sets
void GeneralLight::SetUseAtten(int s) 
{
	SimpleLightUndo< &GeneralLight::SetUseAtten, false >::Hold(this, s, useAtten);
	useAtten = s;
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_USE_ATTEN, _T("useFarAtten"), s );	// 5/15/01 11:25am --MQM-- 
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

void GeneralLight::SetUseAttenNear(int s) 
	{
	SimpleLightUndo< &GeneralLight::SetUseAttenNear, false >::Hold(this, s, useAttenNear);
	useAttenNear = s;
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_USE_ATTEN1, _T("useNearAtten"), s );	// 5/15/01 11:25am --MQM-- 
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

void GeneralLight::SetAttenDisplay(int s) 
	{
	SimpleLightUndo< &GeneralLight::SetAttenDisplay, false >::Hold(this, s, attenDisplay);
	attenDisplay = s;
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_SHOW_RANGES, _T("showFarAtten"), s );	// 5/15/01 11:25am --MQM-- 
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

void GeneralLight::SetAttenNearDisplay(int s) 
	{
	SimpleLightUndo< &GeneralLight::SetAttenNearDisplay, false >::Hold(this, s, attenNearDisplay);
	attenNearDisplay = s;
	if (currentEditLight==this) 
		UpdateUICheckbox( hGeneralLight, IDC_SHOW_RANGES1, _T("showNearAtten"), s );	// 5/15/01 11:25am --MQM-- 
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

void GeneralLight::SetDecayDisplay(int s) 
	{
	SimpleLightUndo< &GeneralLight::SetDecayDisplay, false >::Hold(this, s, decayDisplay);
	decayDisplay = s;
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	}

void GeneralLight::GetAttenPoints(TimeValue t, float rad, Point3 *q)
	{
	double a;
	float sn, cs;
	for(int i = 0; i < NUM_CIRC_PTS; i++) {
		a = (double)i * 2.0 * 3.1415926 / (double)NUM_CIRC_PTS;
		sn = rad * (float)sin(a);
		cs = rad * (float)cos(a);
		q[i+0*NUM_CIRC_PTS] = Point3(sn, cs, 0.0f);
		q[i+1*NUM_CIRC_PTS] = Point3(sn, 0.0f, cs);
		q[i+2*NUM_CIRC_PTS] = Point3(0.0f, sn, cs);
		}
	}

void GeneralLight::DrawX(TimeValue t, float asp, int npts, float dist, GraphicsWindow *gw, int indx) {
	Point3 q[3*NUM_CIRC_PTS+1];
	Point3 u[2];
	GetConePoints(t, asp, GetFallsize(t), dist, q);
	gw->polyline(npts, q,NULL, NULL, TRUE, NULL);
	u[0] = q[0]; u[1] = q[2*indx];
	gw->polyline(2, u,NULL, NULL, FALSE, NULL);
	u[0] = q[indx]; u[1] = q[3*indx];
	gw->polyline(2, u,NULL, NULL, FALSE, NULL);
	}

int GeneralLight::GetCirXPoints(TimeValue t, float angle, float dist, Point3 *q) {
	int i;
	float ang = DegToRad(angle)/2.0f;
	float da = ang/float(NUM_HALF_ARC);
	// first draw circle:
	float d = dist*(float)cos(ang);
	GetConePoints(t, -1.0f, angle, d, q);
	int j=NUM_CIRC_PTS;
	// then draw Arc X
	float a = -ang;
	for(i = -NUM_HALF_ARC; i<= NUM_HALF_ARC; i++, a+=da) 
		q[j++] = Point3(0.0f, dist*(float)sin(a), -dist*(float)cos(a));
	a = -ang;	
	for(i = -NUM_HALF_ARC; i<= NUM_HALF_ARC; i++, a+=da) 
		q[j++] = Point3(dist*(float)sin(a), 0.0f, -dist*(float)cos(a));
	return NUM_CIRC_PTS + 2*NUM_ARC_PTS;
	}

void GeneralLight::DrawSphereArcs(TimeValue t, GraphicsWindow *gw, float r, Point3 *q) {
	GetAttenPoints(t, r, q);
	gw->polyline(NUM_CIRC_PTS, q,				NULL, NULL, TRUE, NULL);
	gw->polyline(NUM_CIRC_PTS, q+NUM_CIRC_PTS,	NULL, NULL, TRUE, NULL);
	gw->polyline(NUM_CIRC_PTS, q+2*NUM_CIRC_PTS,NULL, NULL, TRUE, NULL);
	}

// Draw the the arcs cut out of the sphere by the viewing cone.
int GeneralLight::GetRectXPoints(TimeValue t, float angle, float dist, Point3 *q) {
	int i;
	if(dist==0.0f) dist = .00001f;
	float ang = DegToRad(angle)/2.0f;
	float da,sn,cs,x,y,z,a;
	float aspect = GetAspect(t);
	float w = dist * (float)tan(ang) * (float)sqrt((double)aspect);
	float h = w/aspect;
	float wang = (float)atan(w/dist);
	float hang = (float)atan(h/dist);
	float aw = float(atan(w/dist)*cos(hang));  // half-angle of top and bottom arcs
	float ah = float(atan(h/dist)*cos(wang));  // half-angle of left and right arcs
	int j = 0;

	// draw horizontal and vertical center lines
	da = wang/float(NUM_HALF_ARC);
	for(i = -NUM_HALF_ARC, a = -wang; i<= NUM_HALF_ARC; i++, a+=da) 
		q[j++] = Point3(dist*(float)sin(a), 0.0f, -dist*(float)cos(a));
	da = hang/float(NUM_HALF_ARC);
	for(i = -NUM_HALF_ARC, a = -hang; i<= NUM_HALF_ARC; i++, a+=da) 
		q[j++] = Point3(0.0f, dist*(float)sin(a), -dist*(float)cos(a));


	// draw top and bottom arcs
	da = aw/float(NUM_HALF_ARC);
	sn = (float)sin(hang);
	cs = (float)cos(hang);
	for (i = -NUM_HALF_ARC, a = -aw; i<= NUM_HALF_ARC; i++, a+=da) {
		x =  dist*(float)sin(a); 
		z = -dist*(float)cos(a);
		q[j]             = Point3(x, z*sn, z*cs);  				
		q[j+NUM_ARC_PTS] = Point3(x,-z*sn, z*cs);  				
		j++;
		}
	
	j+= NUM_ARC_PTS;

	// draw left and right arcs
	da = ah/float(NUM_HALF_ARC);
	sn = (float)sin(wang);
	cs = (float)cos(wang);
	for (i = -NUM_HALF_ARC, a = -ah; i<= NUM_HALF_ARC; i++, a+=da) {
		y =  dist*(float)sin(a); 
		z = -dist*(float)cos(a);
		q[j]             = Point3( z*sn, y, z*cs);  				
		q[j+NUM_ARC_PTS] = Point3(-z*sn, y, z*cs);  				
		j++;
		}

	return 6*NUM_ARC_PTS;
	}

// Draw warped rectangle
void GeneralLight::DrawWarpRect(TimeValue t, GraphicsWindow *gw, float angle, float dist, Point3 *q) {
 	GetRectXPoints(t, angle,dist,q);
	for (int i=0; i<6; i++)
		gw->polyline(NUM_ARC_PTS, q+i*NUM_ARC_PTS,NULL, NULL, FALSE, NULL);  
	}

void GeneralLight::DrawCircleX(TimeValue t, GraphicsWindow *gw, float angle, float dist, Point3 *q) {
 	GetCirXPoints(t, angle,dist,q);
	gw->polyline(NUM_CIRC_PTS, q,NULL, NULL, TRUE, NULL);  // circle 
	gw->polyline(NUM_ARC_PTS, q+NUM_CIRC_PTS,NULL, NULL, FALSE, NULL); // vert arc
	gw->polyline(NUM_ARC_PTS, q+NUM_CIRC_PTS+NUM_ARC_PTS,NULL, NULL, FALSE, NULL);  // horiz arc
	}

void GeneralLight::DrawAttenCirOrRect(TimeValue t, GraphicsWindow *gw, float dist, BOOL froze, int uicol) {
	if (!froze) gw->setColor( LINE_COLOR, GetUIColor(uicol));
	if (IsDir()) {
		int npts,indx;
		float asp;
		if (GetSpotShape()) { npts = NUM_CIRC_PTS; 	asp  = -1.0f; 	indx = SEG_INDEX; 	}
		else { 	npts = 4;  	asp  = GetAspect(t); 	indx = 1; 	} 
		DrawX(t, asp, npts, dist, gw, indx);
		}
	else {
		Point3 q[3*NUM_CIRC_PTS+1];
		if (type==OMNI_LIGHT||(IsSpot()&&overshoot)) 
			DrawSphereArcs(t, gw, dist, q);
		else {
			if (GetSpotShape())  
				DrawCircleX(t, gw, GetFallsize(t),dist,q);
			else 
				DrawWarpRect(t, gw, GetFallsize(t),dist,q);
			}
		}
	}


int GeneralLight::DrawAtten(TimeValue t, INode *inode, GraphicsWindow *gw)
	{
	BOOL dispAtten = (useAtten && (extDispFlags & EXT_DISP_ONLY_SELECTED))?TRUE:attenDisplay;
	BOOL dispAttenNear = (useAttenNear && (extDispFlags & EXT_DISP_ONLY_SELECTED))?TRUE:attenNearDisplay;
	BOOL dispDecay = (GetDecayType()&&(extDispFlags & EXT_DISP_ONLY_SELECTED))?TRUE:decayDisplay;

	if (dispAtten||dispAttenNear||dispDecay) {
		Matrix3 tm = inode->GetObjectTM(t);
		gw->setTransform(tm);
		BOOL froze = inode->IsFrozen() && !inode->Dependent();
	 	if (dispAtten) {
	 		DrawAttenCirOrRect(t, gw, GetAtten(t,ATTEN_START), froze, COLOR_START_RANGE);
	 		DrawAttenCirOrRect(t, gw, GetAtten(t,ATTEN_END), froze, COLOR_END_RANGE);
	 		}
	 	if (dispAttenNear) {
	 		DrawAttenCirOrRect(t, gw, GetAtten(t,ATTEN1_START), froze, COLOR_START_RANGE1);
	 		DrawAttenCirOrRect(t, gw, GetAtten(t,ATTEN1_END), froze, COLOR_END_RANGE1);
	 		}
	 	if (dispDecay) {
	 		DrawAttenCirOrRect(t, gw, GetDecayRadius(t), froze, COLOR_DECAY_RADIUS);
	 		}
		}
	return 0;
	}

GeneralLight::~GeneralLight() 
{
	DeleteAllRefsFromMe();
	pblock = NULL;

	if( shadType && !shadType->SupportStdMapInterface() ){
		ReplaceReference( AS_SHADTYPE_REF, NULL );
//		delete ASshadType;
//		ASshadType = NULL;
	}

	UnRegisterNotification(&NotifyDist, (void *)this, NOTIFY_UNITS_CHANGE );
//	UnRegisterNotification(NotifyPreSaveOld, (void *)this, NOTIFY_FILE_PRE_SAVE_OLD);
//	UnRegisterNotification(NotifyPostSaveOld, (void *)this, NOTIFY_FILE_POST_SAVE_OLD);
//	UnRegisterNotification(NameChangeNotify, (void *)this, NOTIFY_NODE_RENAMED);
}


// 5/15/01 9:51am --MQM-- 
// fixes for macrorecorder
void GeneralLight::UpdateUICheckbox( HWND hwnd, int dlgItem, TCHAR *name, int val )
{
	if ( hwnd )
	{
		// spit out macro recorder stuff
		if (name && !theHold.RestoreOrRedoing())
			macroRec->SetSelProperty( name, mr_bool, val);

		// update the checkbox
		macroRec->Disable();
		CheckDlgButton( hwnd, dlgItem, val );
		macroRec->Enable();
	}
}


class GeneralLightCreateCallBack: public CreateMouseCallBack 
{
	GeneralLight *ob;
public:
	int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(GeneralLight *obj) { ob = obj; }
};

int GeneralLightCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat) 
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	Point3 p0;
	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}

	if (msg==MOUSE_POINT || msg==MOUSE_MOVE) {		

		// 6/19/01 11:37am --MQM-- wire-color changes
		// since we're now allowing the user to set the color of
		// the light wire-frames, we need to set the initial light 
		// color to yellow, instead of the default random object color.
		if ( point == 0 )
		{
			ULONG handle;
			ob->NotifyDependents( FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE );
			INode * node;
			node = GetCOREInterface()->GetINodeByHandle( handle );
			if ( node ) 
			{
				Point3 color = GetUIColor( COLOR_LIGHT_OBJ );	// yellow wire color
				node->SetWireColor( RGB( color.x*255.0f, color.y*255.0f, color.z*255.0f ) );
			}
		}

		//mat.SetTrans(vpt->GetPointOnCP(m));
			mat.SetTrans( vpt->SnapPoint(m,m,NULL,SNAP_IN_3D) );

		ob->enable = TRUE;
		if (point==1) {
			if (msg==MOUSE_POINT) 
					return 0;
		}
	}
	else if (msg == MOUSE_ABORT)
		return CREATE_ABORT;

	return TRUE;
}

static GeneralLightCreateCallBack sGeneralLgtCreateCB;

CreateMouseCallBack* GeneralLight::GetCreateMouseCallBack() 
{
	sGeneralLgtCreateCB.SetObj(this);

	return &sGeneralLgtCreateCB;
}

static void RemoveScaling(Matrix3 &tm) {
	AffineParts ap;
	decomp_affine(tm, &ap);
	tm.IdentityMatrix();
	tm.SetRotate(ap.q);
	tm.SetTrans(ap.t);
	}

void GeneralLight::GetMat(TimeValue t, INode* inode, ViewExp &vpt, Matrix3& tm) 
{
	if ( ! vpt.IsAlive() )
	{
		tm.Zero();
		return;
	}
	
	tm = inode->GetObjectTM(t);
//	tm.NoScale();
	RemoveScaling(tm);
	float scaleFactor = vpt.NonScalingObjectSize() * vpt.GetVPWorldWidth(tm.GetTrans()) / 360.0f;
	tm.Scale(Point3(scaleFactor,scaleFactor,scaleFactor));
}

void GeneralLight::GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel )
{
	box = mesh->getBoundingBox(tm);
}


void GeneralLight::BoxCircle(TimeValue t, float r, float d, Box3& box, int extraPt, Matrix3 *tm) {
	Point3 q[3*NUM_CIRC_PTS];
	int npts;
	float asp;
	if ( GetSpotShape()) { 	npts =  NUM_CIRC_PTS+extraPt; 	asp = -1.0f; }
	else { npts = 4+extraPt;  asp = GetAspect(t); } 
 	GetConePoints(t, asp , r, d, q);
 	box.IncludePoints(q,npts,tm);
	}

void GeneralLight::BoxDirPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm) {
	int npts;
	Point3 q[3*NUM_CIRC_PTS];
	npts = GetSpotShape()? GetCirXPoints(t,angle,dist,q): GetRectXPoints(t,angle,dist,q);
	box.IncludePoints(q,npts,tm);
	}


void GeneralLight::BoxPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm) {
	if (IsDir())
		BoxCircle(t, angle, dist, box, 0,tm);
	else 
		BoxDirPoints(t, angle, dist, box, tm);
	}

void GeneralLight::BoxLight(TimeValue t, INode *inode, Box3& box, Matrix3 *tm) {
	Point3 pt;
	float d = GetTDist(t);
	//R6 change 
	if(!(extDispFlags & EXT_DISP_GROUP_EXT)) 
	{
	if (GetTargetPoint(t, inode, pt)) {
		Point3 loc = inode->GetObjectTM(t).GetTrans();
		d = FLength(loc - pt) / FLength(inode->GetObjectTM(t).GetRow(2));
		box += tm? (*tm)*Point3(0.0f, 0.0f, -d): Point3(0.0f, 0.0f, -d);
		}
	else {
		if (coneDisplay) 
			box += tm? (*tm)*Point3(0.0f, 0.0f, -d): Point3(0.0f, 0.0f, -d);
		}
	}
	if( coneDisplay || (extDispFlags & EXT_DISP_ONLY_SELECTED)) {
		float rad = MaxF(GetHotspot(t), GetFallsize(t));
		if (IsDir()) 
			BoxCircle(t,rad,0.0f,box,1,tm);
		BoxCircle(t,rad,d,box,1,tm);
		}
	BOOL dispAtten, dispAttenNear, dispDecay;
	if (useAtten && (extDispFlags & EXT_DISP_ONLY_SELECTED))  
		dispAtten = dispAttenNear = TRUE;
	else {
		dispAtten = attenDisplay;
		dispAttenNear = attenNearDisplay;
		}
	dispDecay = (GetDecayType()&&(extDispFlags & EXT_DISP_ONLY_SELECTED))?TRUE:decayDisplay;
	if( dispAtten||dispAttenNear||dispDecay) {
		if (type==OMNI_LIGHT||(IsSpot()&&overshoot)) { 
			Point3 q[3*NUM_CIRC_PTS];
			float rad = 0;
			if (dispAtten) rad = MaxF(GetAtten(t, ATTEN_START), GetAtten(t, ATTEN_END));
			if (dispAttenNear) { 
				rad = MaxF(GetAtten(t, ATTEN1_START), rad);
				rad = MaxF(GetAtten(t, ATTEN1_END), rad);
				}
			if (dispDecay) rad = MaxF(rad,GetDecayRadius(t));
			GetAttenPoints(t, rad, q);
			box.IncludePoints(q,3*NUM_CIRC_PTS,tm);
			}
		else  {
			if (dispAtten) {
				BoxPoints(t, GetFallsize(t), GetAtten(t,ATTEN_END), box, tm);
				BoxPoints(t, GetFallsize(t), GetAtten(t,ATTEN_START), box, tm);
				}
			if (dispAttenNear) {
				BoxPoints(t, GetFallsize(t), GetAtten(t,ATTEN1_END), box, tm);
				BoxPoints(t, GetFallsize(t), GetAtten(t,ATTEN1_START), box, tm);
				}
			if (dispDecay) {
				BoxPoints(t, GetFallsize(t), GetDecayRadius(t), box, tm);
				}
			}
		}
	}

void GeneralLight::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}
	
	Point3 loc = inode->GetObjectTM(t).GetTrans();
	float scaleFactor = vpt->NonScalingObjectSize()*vpt->GetVPWorldWidth(loc) / 360.0f;
	box = mesh->getBoundingBox();//same as GetDeformBBox
	box.Scale(scaleFactor);
	//JH 6/13/03 careful, the mesh extents in z are (0, ~-20)
	//thus the scale alone is wrong
	//move the top of the box back to the object space origin
	//#1055329,Boundbox of Omni_light error.
	if (type!=OMNI_LIGHT)
	{
		box.Translate(Point3(0.0f,0.0f,-box.pmax.z));
	}
	
	//this adds the target point, cone, etc.
	BoxLight(t, inode, box, NULL);
	}

void GeneralLight::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
	{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}
	
	int nv;
	Matrix3 tm;
	GetMat(t, inode,*vpt,tm);
	Point3 loc = tm.GetTrans();
	nv = mesh->getNumVerts();
	box.Init();
	if(!(extDispFlags & EXT_DISP_ZOOM_EXT)) 
		box.IncludePoints(mesh->verts,nv,&tm);
	else
		box += loc;
	tm = inode->GetObjectTM(t);
	BoxLight(t, inode, box, &tm);
	}


// From BaseObject
int GeneralLight::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) 
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	HitRegion hitRegion;
	DWORD savedLimits;
	int res = 0;
	Matrix3 m;
	if (!enable) 
		return 0;
	GraphicsWindow *gw = vpt->getGW();	
	Material *mtl = gw->getMaterial();
	MakeHitRegion(hitRegion,type,crossing,4,p);	
	gw->setRndLimits( ((savedLimits = gw->getRndLimits()) | GW_PICK) & ~(GW_ILLUM|GW_BACKCULL));
	GetMat(t,inode,*vpt,m);
	gw->setTransform(m);
	// if we get a hit on the mesh, we're done
	res = mesh->select( gw, mtl, &hitRegion, flags & HIT_ABORTONHIT);
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

void GeneralLight::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) 
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}
	
	GenericSnap(t,inode,snap,p,vpt);
}

void GeneralLight::SetExtendedDisplay(int flags)
{
	extDispFlags = flags;
}

MaxSDK::Graphics::Utilities::MeshEdgeKey LightMeshKey;
MaxSDK::Graphics::Utilities::SplineItemKey LightSplineKey;

unsigned long GeneralLight::GetObjectDisplayRequirement() const
{
	return 0;
}

bool GeneralLight::PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& displayContext)
{
	LightMeshKey.SetFixedSize(true);
	return true;
}

bool GeneralLight::UpdatePerNodeItems(
	const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
	MaxSDK::Graphics::UpdateNodeContext& nodeContext,
	MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer)
{
	INode* pNode = nodeContext.GetRenderNode().GetMaxNode();
	MaxSDK::Graphics::Utilities::MeshEdgeRenderItem* pMeshItem = new MaxSDK::Graphics::Utilities::MeshEdgeRenderItem(mesh, true, false);
	if (pNode->Dependent())
	{
		pMeshItem->SetColor(Color(ColorMan()->GetColor(kViewportShowDependencies)));
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
		if(useLight)
		{
			Color color = GetIViewportShadingMgr()->GetLightIconColor(*pNode);
			pMeshItem->SetColor(color);
		}
		else
		{
			pMeshItem->SetColor(Color(0,0,0));
		}
	}

	MaxSDK::Graphics::CustomRenderItemHandle meshHandle;
	meshHandle.Initialize();
	meshHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	meshHandle.SetCustomImplementation(pMeshItem);
	MaxSDK::Graphics::ConsolidationData data;
	data.Strategy = &MaxSDK::Graphics::Utilities::MeshEdgeConsolidationStrategy::GetInstance();
	data.Key = &LightMeshKey;
	meshHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(meshHandle);

	MaxSDK::Graphics::Utilities::SplineRenderItem* pLineItem = new LightConeItem(this);
	MaxSDK::Graphics::CustomRenderItemHandle coneHandle;
	coneHandle.Initialize();
	coneHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	coneHandle.SetCustomImplementation(pLineItem);
	data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
	data.Key = &LightSplineKey;
	coneHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(coneHandle);

	pLineItem = new LightTargetLineItem(this);
	MaxSDK::Graphics::CustomRenderItemHandle lineHandle;
	lineHandle.Initialize();
	lineHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	lineHandle.SetCustomImplementation(pLineItem);
	data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
	data.Key = &LightSplineKey;
	lineHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(lineHandle);

	return true;
}

int GeneralLight::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) 
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	Matrix3 m;
	if (!enable) 
		return 0;
	GraphicsWindow *gw = vpt->getGW();
	GetMat(t,inode,*vpt,m);
	gw->setTransform(m);
	DWORD rlim = gw->getRndLimits();
	gw->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY|GW_BACKCULL|(gw->getRndMode() & GW_Z_BUFFER));
	if (inode->Selected())
		gw->setColor( LINE_COLOR, GetSelColor());
	else if(!inode->IsFrozen() && !inode->Dependent())	{
		if(useLight)
		{
			// 6/22/01 2:16pm --MQM--
			// use the wire-color to draw the light,
			// instead of COLOR_LIGHT_OBJ
			//! NH - Add code here to change the color of the light depending on the Viewport Shading
			Color color = GetIViewportShadingMgr()->GetLightIconColor(*inode);
			gw->setColor( LINE_COLOR, color );
		}
		// I un-commented this line DS 6/11/99
		else
			gw->setColor( LINE_COLOR, 0.0f, 0.0f, 0.0f);
	}
	mesh->render( gw, gw->getMaterial(),
		(flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, COMP_ALL);	
	DrawConeAndLine(t, inode, gw, 1);
	DrawAtten(t, inode, gw);
	gw->setRndLimits(rlim);
	return 0 ;
}


RefResult GeneralLight::EvalLightState(TimeValue t, Interval& valid, LightState* ls) 
{
	if(useLight)
		ls->color = GetRGBColor(t,valid);
	else
		ls->color = Point3(0,0,0);
	ls->on = useLight;
	ls->intens = GetIntensity(t, valid);
	ls->hotsize = GetHotspot(t, valid);
	ls->fallsize = GetFallsize(t, valid);
#if 0
	if(ls->useAtten = GetUseAtten()) {
		ls->attenStart = GetAtten(t, ATTEN_START, valid);
		ls->attenEnd = GetAtten(t, ATTEN_END, valid);
	}
	else
		ls->attenStart = ls->attenEnd = 0.0f;
#endif
	
	ls->useNearAtten = GetUseAttenNear();
	ls->nearAttenStart = GetAtten(t, ATTEN1_START, valid);
	ls->nearAttenEnd = GetAtten(t, ATTEN1_END, valid);

	ls->useAtten = GetUseAtten();
	ls->attenStart = GetAtten(t, ATTEN_START, valid);
	ls->attenEnd = GetAtten(t, ATTEN_END, valid);
	if (ls->extra == 0x0100) {
		// Special code for the interactive viewport in order to handle
		// DecayType.  If extra is set to 0x0100, we assume that we are called
		// from interactive viewport code.
		//
		// Set bits on the extra field to pass on the type of attenuation in
		// use.
		//
		// Note that attenStart contains the DecayRadius if useAtten is not
		// DECAY_NONE, i.e. either DECAY_INV or DECAY_INVSQ.  The GFX code will
		// use this to handle these two types of attenuation.
		//
		// Please note that attenEnd always contains the attenEnd value of the Far
		// Attenuation.
		//
		int decay = GetDecayType();
		if (decay > 0) {
			ls->attenStart = GetDecayRadius(t, valid);
		}
		switch (decay) {
			case DECAY_NONE:
			default:
				ls->extra |= GW_ATTEN_NONE;
				break;
			case DECAY_INV:
				ls->extra |= GW_ATTEN_LINEAR;
				break;
			case DECAY_INVSQ:
				ls->extra |= GW_ATTEN_QUAD;
				break;
		}
		if (ls->useAtten) {
			ls->extra |= GW_ATTEN_END;
		}

	}
	ls->shape = GetSpotShape();
	ls->aspect = GetAspect(t, valid);
	ls->overshoot = GetOvershoot();
	ls->shadow = GetShadow();
	ls->ambientOnly = ambientOnly;
	ls->affectDiffuse = affectDiffuse;
	ls->affectSpecular = affectSpecular;

	if(type == OMNI_LIGHT)
		ls->type = OMNI_LGT;
	else if(type == DIR_LIGHT||type==TDIR_LIGHT)
		ls->type = DIRECT_LGT;
	else
		ls->type = SPOT_LGT;

	return REF_SUCCEED;
}

//
// Reference Managment:
//
					
RefResult GeneralLight::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate ) 
{
	switch (message) {
	case REFMSG_NODE_NAMECHANGE:
	case REFMSG_CHANGE:
		if(iObjParams)
			UpdateUI(iObjParams->GetTime());
		break;

	case REFMSG_NODE_HANDLE_CHANGED:
		{
		IMergeManager* imm = (IMergeManager*)partID;
		if (imm) {
			exclList.OnMerge(imm);
			}
		return REF_STOP;
		}
	case REFMSG_GET_PARAM_DIM: {
		GetParamDim *gpd = (GetParamDim*)partID;
		if ( hTarget == ( RefTargetHandle )pblock )
		{
			if (type == OMNI_LIGHT) {
				switch (gpd->index) {
					case PB_OMNISHADCOLOR:
					case PB_COLOR:
						gpd->dim = stdColor255Dim;
						break;
					case PB_INTENSITY:
					case PB_CONTRAST:
					case PB_DIFFSOFT:
	//				case PB_OMNIMAP_SIZE:
	//				case PB_OMNIMAP_RANGE:
						gpd->dim = defaultDim;
						break;					
					case PB_OMNIATSTART1:
					case PB_OMNIATEND1:
					case PB_OMNIATSTART:
					case PB_OMNIATEND:
					case PB_OMNIDECAY:
						gpd->dim = stdWorldDim;
						break;					
					case PB_OMNIATMOS_COLAMT:
					case PB_OMNIATMOS_OPACITY:
						gpd->dim = stdPercentDim;
						break;
					}
				}
			else {
				switch (gpd->index) {
					case PB_SHADCOLOR:
					case PB_COLOR:
						gpd->dim = stdColor255Dim;
						break;
					case PB_INTENSITY:
					case PB_CONTRAST:
					case PB_DIFFSOFT:
					case PB_ASPECT:
						gpd->dim = defaultDim;
						break;					
					case PB_HOTSIZE:
					case PB_FALLSIZE:
						gpd->dim = IsDir()?stdWorldDim:defaultDim;
						break;					
					case PB_TDIST:
					case PB_ATTENSTART1:
					case PB_ATTENEND1:
					case PB_ATTENSTART:
					case PB_ATTENEND:
					case PB_DECAY:
						gpd->dim = stdWorldDim;
						break;					
					case PB_ATMOS_COLAMT:
					case PB_ATMOS_OPACITY:
						gpd->dim = stdPercentDim;
						break;
					}
				}
			return REF_HALT; 
		}
		return REF_STOP;
	}

	case REFMSG_GET_PARAM_NAME: {
		GetParamName *gpn = (GetParamName*)partID;
		if ( hTarget == ( RefTargetHandle )pblock )
		{
			if (type == OMNI_LIGHT) {
				switch (gpn->index) {
					case PB_COLOR: gpn->name = GetString(IDS_DB_COLOR); break;
					case PB_INTENSITY:  gpn->name = GetString(IDS_DB_MULTIPLIER); break;
					case PB_CONTRAST:  gpn->name = GetString(IDS_DS_CONTRAST); break;
					case PB_DIFFSOFT:  gpn->name = GetString(IDS_DS_DIFFSOFT); break;
					case PB_OMNIATSTART1: gpn->name = GetString(IDS_DS_ATTENSTARTNEAR); break;
					case PB_OMNIATEND1: gpn->name = GetString(IDS_DS_ATTENENDNEAR); break;
					case PB_OMNIATSTART: gpn->name = GetString(IDS_DB_ATTENSTART); break;
					case PB_OMNIATEND: gpn->name = GetString(IDS_DB_ATTENEND); break;
					case PB_OMNIDECAY: gpn->name = GetString(IDS_DECAY_RADIUS); break;
					case PB_OMNISHADCOLOR: gpn->name = GetString(IDS_DS_SHADCOL); break;
					case PB_OMNIATMOS_OPACITY: gpn->name = GetString(IDS_ATMOS_OPACITY); break;
					case PB_OMNIATMOS_COLAMT: gpn->name = GetString(IDS_ATMOS_COLAMT); break;
					case PB_OMNISHADMULT: gpn->name = GetString(IDS_SHAD_DENSITY); break;  
					}
				}
			else {			
				switch (gpn->index) {
					case PB_COLOR: gpn->name =  GetString(IDS_DB_COLOR); break;
					case PB_INTENSITY: gpn->name = GetString(IDS_DB_MULTIPLIER); break;
					case PB_CONTRAST:  gpn->name = GetString(IDS_DS_CONTRAST); break;
					case PB_DIFFSOFT:  gpn->name = GetString(IDS_DS_DIFFSOFT); break;
					case PB_HOTSIZE:  gpn->name = GetString(IDS_DB_HOTSIZE);	break;
					case PB_FALLSIZE: gpn->name = GetString(IDS_DB_FALLSIZE);	break;
					case PB_ASPECT:	gpn->name = GetString(IDS_DB_ASPECT_RATIO);	break;
					case PB_ATTENSTART1: gpn->name = GetString(IDS_DS_ATTENSTARTNEAR); break;
					case PB_ATTENEND1:	gpn->name = GetString(IDS_DS_ATTENENDNEAR);	break;
					case PB_ATTENSTART:	gpn->name = GetString(IDS_DB_ATTENSTART);	break;
					case PB_ATTENEND: gpn->name = GetString(IDS_DB_ATTENEND);	break;
					case PB_DECAY: gpn->name = GetString(IDS_DECAY_RADIUS); break;
					case PB_TDIST:	gpn->name = GetString(IDS_DB_TDIST);		break;
					case PB_SHADCOLOR: gpn->name = GetString(IDS_DS_SHADCOL); break;
					case PB_ATMOS_OPACITY: gpn->name = GetString(IDS_ATMOS_OPACITY); break;
					case PB_ATMOS_COLAMT: gpn->name = GetString(IDS_ATMOS_COLAMT); break;
					case PB_SHADMULT: gpn->name = GetString(IDS_SHAD_DENSITY); break;  
					}
				}
		}
		return REF_HALT; 
		}
		return REF_STOP;
	}
	return(REF_SUCCEED);
}

ObjectState GeneralLight::Eval(TimeValue time)
{
	UpdateUI(time);
	return ObjectState(this);
}

Interval GeneralLight::ObjectValidity(TimeValue time) 
	{
	Interval valid;
	valid.SetInfinite();
	if (!waitPostLoad) {
		GetRGBColor(time, valid);
		GetIntensity(time, valid);
		GetContrast(time, valid);
		GetDiffuseSoft(time, valid);
		if(IsSpot()) {
			GetHotspot(time, valid);
			GetFallsize(time, valid);
			GetTDist(time, valid);
			GetAspect(time, valid);
			}
		GetAtten(time, ATTEN_START, valid);
		GetAtten(time, ATTEN_END, valid);
		GetAtten(time, ATTEN1_START, valid);
		GetAtten(time, ATTEN1_END, valid);
		GetDecayRadius(time, valid);
		GetMapSize(time,valid);
		GetMapRange(time,valid);
		GetRayBias(time,valid);
		GetAtmosOpacity(time,valid);
		GetAtmosColAmt(time,valid);
		GetShadColor(time,valid);
		GetShadMult(time,valid);
		UpdateUI(time);
		}
	return valid;
	}

RefTargetHandle GeneralLight::Clone(RemapDir& remap) 
	{
	GeneralLight* newob = new GeneralLight(type);
	newob->enable = enable;
	newob->coneDisplay = coneDisplay;
	newob->useLight = useLight;
	newob->attenDisplay = attenDisplay;
	newob->useAtten = useAtten;
	newob->useAttenNear = useAttenNear;
	newob->attenNearDisplay = attenNearDisplay;
	newob->decayDisplay = decayDisplay;
	newob->shape = shape;
	newob->shadow = shadow;
	newob->shadowType = shadowType;
	newob->overshoot = overshoot;
	newob->projector = projector;
	newob->absMapBias = absMapBias;
	newob->exclList = exclList;
	newob->softenDiffuse = softenDiffuse;
	newob->affectDiffuse = affectDiffuse;
	newob->affectSpecular = affectSpecular;
	newob->ambientOnly = ambientOnly;
	newob->decayType = decayType;
	newob->atmosShadows = atmosShadows;
	newob->atmosOpacity = atmosOpacity;
	newob->atmosColAmt = atmosColAmt;
   newob->ReplaceReference(PBLOCK_REF,remap.CloneRef(pblock));
   if (projMap)     newob->ReplaceReference(PROJMAP_REF,remap.CloneRef(projMap));
   if (shadProjMap) newob->ReplaceReference(SHADPROJMAP_REF,remap.CloneRef(shadProjMap));
   if (shadType)    newob->ReplaceReference(SHADTYPE_REF,remap.CloneRef(shadType));
   if (ASshadType)  newob->ReplaceReference(AS_SHADTYPE_REF,remap.CloneRef(ASshadType));
   if (emitter)     newob->ReplaceReference(EMITTER_REF ,remap.CloneRef(emitter));
	BaseClone(this, newob, remap);
	return(newob);
	}


#define COLOR_CHUNK 0x2550		// obsolete
#define HOTSIZE_CHUNK 0x2552	// obsolete
#define CONE_DISPLAY_CHUNK	0x2560
#define RANGE_DISPLAY_CHUNK	0x2561
#define USE_ATTEN_CHUNK 0x2562
#define CROSSOVER_CHUNK 0x2563
#define SHADOW_CHUNK 0x2570
#define SHADOW_TYPE_CHUNK 0x2571
#define GLOBALSHADOW_CHUNK 0x2572
#define SPOTSHAPE_CHUNK 0x2573
#define ABS_MAP_BIAS_CHUNK 0x2574
#define OVERSHOOT_CHUNK 0x2575
#define OLD_EXCL_LIST_CHUNK 0x2576
#define PROJECTOR_CHUNK 0x2577
#define MAP_RANGE_CHUNK 0x2578
#define MAP_BIAS_CHUNK 0x2579
#define MAP_SIZE_CHUNK 0x257A
#define RAY_BIAS_CHUNK 0x257B
#define ON_OFF_CHUNK 0x2580
#define NEAR_RANGE_DISPLAY_CHUNK 0x2590
#define NEAR_USE_ATTEN_CHUNK     0x25A0
#define DONT_AFFECT_DIFF_CHUNK     0x25B0
#define DONT_AFFECT_SPEC_CHUNK     0x25B1
#define SOFTEN_DIFF_CHUNK     0x25C0
#define DECAY_TYPE_CHUNK     0x25D0
#define LT_EFFECT_SHAD_CHUNK 0x25E0
#define AMBIENT_ONLY_CHUNK 0x2600
#define DECAY_DISPLAY_CHUNK 0x2700
#define EXCL_LIST_CHUNK 0x2800

// IO
IOResult GeneralLight::Save(ISave *isave) 
{
	ULONG nb;
	
	isave->BeginChunk(ON_OFF_CHUNK);
	isave->Write(&useLight, sizeof(useLight), &nb);
	isave->EndChunk();
	isave->BeginChunk(CONE_DISPLAY_CHUNK);
	isave->Write(&coneDisplay, sizeof(coneDisplay), &nb);
	isave->EndChunk();
	isave->BeginChunk(RANGE_DISPLAY_CHUNK);
	isave->Write(&attenDisplay, sizeof(attenDisplay), &nb);
	isave->EndChunk();
	isave->BeginChunk(USE_ATTEN_CHUNK);
	isave->Write(&useAtten, sizeof(useAtten), &nb);
	isave->EndChunk();
	isave->BeginChunk(NEAR_RANGE_DISPLAY_CHUNK);
	isave->Write(&attenNearDisplay, sizeof(attenNearDisplay), &nb);
	isave->EndChunk();
	isave->BeginChunk(NEAR_USE_ATTEN_CHUNK);
	isave->Write(&useAttenNear, sizeof(useAttenNear), &nb);
	isave->EndChunk();
	isave->BeginChunk(DECAY_DISPLAY_CHUNK);
	isave->Write(&decayDisplay, sizeof(decayDisplay), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHADOW_CHUNK);
	isave->Write(&shadow, sizeof(shadow), &nb);
	isave->EndChunk();
	isave->BeginChunk(SHADOW_TYPE_CHUNK);
	isave->Write(&shadowType, sizeof(shadowType), &nb);
	isave->EndChunk();
	isave->BeginChunk(GLOBALSHADOW_CHUNK);
	isave->Write(&useGlobalShadowParams, sizeof(useGlobalShadowParams), &nb);
	isave->EndChunk();

	if (IsSpot()) {
		isave->BeginChunk(SPOTSHAPE_CHUNK);
		isave->Write(&shape, sizeof(shape), &nb);
		isave->EndChunk();
		if (overshoot) {
			isave->BeginChunk(OVERSHOOT_CHUNK);
			isave->EndChunk();
			}
		}

	if (projector) {
		isave->BeginChunk(PROJECTOR_CHUNK);
		isave->EndChunk();
		}
	if (!affectDiffuse) {
		isave->BeginChunk(DONT_AFFECT_DIFF_CHUNK);
		isave->EndChunk();
		}
	if (!affectSpecular) {
		isave->BeginChunk(DONT_AFFECT_SPEC_CHUNK);
		isave->EndChunk();
		}
	if (softenDiffuse) {
		isave->BeginChunk(SOFTEN_DIFF_CHUNK);
		isave->EndChunk();
		}
	if (decayType) {
		isave->BeginChunk(DECAY_TYPE_CHUNK);
		isave->Write(&decayType, sizeof(decayType), &nb);
		isave->EndChunk();
		}

	if (ltAffectsShadow) {
		isave->BeginChunk(LT_EFFECT_SHAD_CHUNK);
		isave->EndChunk();
		}

	if (ambientOnly) {
		isave->BeginChunk(AMBIENT_ONLY_CHUNK);
		isave->EndChunk();
		}
	//if (exclList.Count()>0) {
	isave->BeginChunk(EXCL_LIST_CHUNK);
	exclList.Save(isave);
	isave->EndChunk();
	//	}


	return IO_OK;
	}

void GeneralLight::FixOldVersions(ILoad *iload) {
	Point3 rgb;
	if (pblock->GetVersion() != LIGHT_VERSION) {
		int oldvers = pblock->GetVersion();
		if (oldvers>LIGHT_VERSION) 
			return;
		iload->SetObsolete();
		if (oldvers<12) {
			// loading a pre-pluggable shadow generators file
			// Split the parameters in two; the light part and the 
			// shadow generator part
			IParamBlock* oldPB = pblock;
			IParamBlock* newParamBlk = UpdateParameterBlock(GetDesc(oldvers,type), GetDim(oldvers,type), oldPB,
		                  GetDesc(LIGHT_VERSION,type), GetDim(LIGHT_VERSION,type), LIGHT_VERSION);
			if (shadowType) {
				ReplaceReference(SHADTYPE_REF,NewDefaultRayShadowType());
				}
			shadType->ConvertParamBlk( GetDesc(oldvers,type), GetDim(oldvers,type), oldPB);
			ReplaceReference( PBLOCK_REF, newParamBlk );	
			if (shadowType) {
				Renderer *rend = GetCOREInterface()->GetCurrentRenderer();
				if (rend->ClassID()==Class_ID(SREND_CLASS_ID,0)) {
					IScanRenderer *sr = (IScanRenderer*)rend;
					shadType->SetMaxDepth(TimeValue(0), sr->GetMaxRayDepth());
					}
				}
			shadType->SetAbsMapBias(TimeValue(0),absMapBias);
			if (oldvers<12) {
				SetShadMult( TimeValue(0), dlgShadMult );
				if (oldvers<11) {
					SetAtmosOpacity( TimeValue(0), dlgAtmosOpacity);
					SetAtmosColAmt( TimeValue(0), dlgAtmosColamt);
					SetAtmosShadows( TimeValue(0), dlgAtmosShadows);
					if (oldvers<10) {
						Control* c = pblock->GetController(type==OMNI_LIGHT?PB_OMNIATEND1:PB_ATTENEND1);
						if (c) {
							c = (Control *)CloneRefHierarchy(c);
							pblock->SetController(type==OMNI_LIGHT?PB_OMNIDECAY:PB_DECAY,c);
					   		}
						else 
							SetDecayRadius(0,GetAtten(0,ATTEN1_END));
						if (oldvers<8) {
							if (softenDiffuse) SetDiffuseSoft(0,1.0f);
							}
						}
					}
				}
			}

		else 
			ReplaceReference(PBLOCK_REF,
				UpdateParameterBlock(
					GetDesc(oldvers,type), GetDim(oldvers,type), pblock,
					GetDesc(LIGHT_VERSION,type), GetDim(LIGHT_VERSION,type), LIGHT_VERSION));
		}
	BuildSpotMesh(max(GetHotspot(0), GetFallsize(0)));
	}

class LightPostLoad : public PostLoadCallback {
public:
	GeneralLight *gl;
	Interval valid;
	LightPostLoad(GeneralLight *l) { gl = l;}
	void proc(ILoad *iload) {
		gl->FixOldVersions(iload);
		waitPostLoad--;
		delete this;
		}
	};

IOResult  GeneralLight::Load(ILoad *iload) 
{
	ULONG nb;
	IOResult res;
	enable = TRUE;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
		case ON_OFF_CHUNK:
			res = iload->Read(&useLight,sizeof(useLight), &nb);
			break;
		case CONE_DISPLAY_CHUNK:
			res = iload->Read(&coneDisplay,sizeof(coneDisplay), &nb);
			break;
		case RANGE_DISPLAY_CHUNK:
			res = iload->Read(&attenDisplay,sizeof(attenDisplay), &nb);
			break;
		case DECAY_DISPLAY_CHUNK:
			res = iload->Read(&decayDisplay,sizeof(decayDisplay), &nb);
			break;
		case NEAR_USE_ATTEN_CHUNK:
			res = iload->Read(&useAttenNear,sizeof(useAttenNear), &nb);
			break;
		case NEAR_RANGE_DISPLAY_CHUNK:
			res = iload->Read(&attenNearDisplay,sizeof(attenNearDisplay), &nb);
			break;
		case USE_ATTEN_CHUNK:
			res = iload->Read(&useAtten,sizeof(useAtten), &nb);
			break;
		case SHADOW_CHUNK:
			res = iload->Read(&shadow,sizeof(shadow), &nb);
			break;
		case SHADOW_TYPE_CHUNK:// for reading old files
			res = iload->Read(&shadowType,sizeof(shadowType), &nb);
			break;
		case GLOBALSHADOW_CHUNK:
			res = iload->Read(&useGlobalShadowParams,sizeof(useGlobalShadowParams), &nb);
			break;
		case SPOTSHAPE_CHUNK:
			res = iload->Read(&shape,sizeof(shape), &nb);
			break;
		case ABS_MAP_BIAS_CHUNK: // for reading old files
			res = iload->Read(&absMapBias,sizeof(absMapBias), &nb);
			break;
		case OVERSHOOT_CHUNK:
			overshoot = TRUE;
			break;
		case PROJECTOR_CHUNK:
			projector = TRUE;
			break;
		case DONT_AFFECT_DIFF_CHUNK:
			affectDiffuse = FALSE;
			break;
		case DONT_AFFECT_SPEC_CHUNK:
			affectSpecular = FALSE;
			break;
		case SOFTEN_DIFF_CHUNK:
			softenDiffuse = TRUE;
			break;
		case OLD_EXCL_LIST_CHUNK:  
		case EXCL_LIST_CHUNK: 
			res = exclList.Load(iload);
			break;
		case DECAY_TYPE_CHUNK:
			res = iload->Read(&decayType,sizeof(decayType), &nb);
			break;
		case LT_EFFECT_SHAD_CHUNK:
			ltAffectsShadow = TRUE;
			break;
		case AMBIENT_ONLY_CHUNK:
			ambientOnly = TRUE;
			break;
		}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
	}
	waitPostLoad++;
	iload->RegisterPostLoadCallback(new LightPostLoad(this));
//	waitPostLoad++;
	iload->RegisterPostLoadCallback(new ParamBlockPLCB( emitterVersions, numEmitterVersions, &curEmitterVersion, this, EMITTER_REF ));
	return IO_OK;
}


// target spot light creation stuff...

class TSpotCreationManager : public MouseCallBack, ReferenceMaker 
{
private:
	CreateMouseCallBack *createCB;	
	INode *lgtNode,*targNode;
	GeneralLight *lgtObject;
	TargetObject *targObject;
	int attachedToNode;
	IObjCreate *createInterface;
	ClassDesc *cDesc;
	Matrix3 mat;  // the nodes TM relative to the CP
	IPoint2 pt0;
	int ignoreSelectionChange;
	int lastPutCount;

	void CreateNewObject();	

	virtual void GetClassName(MSTR& s) { s = _M("TSpotCreationManager"); }
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return (RefTargetHandle)lgtNode; } 
	void SetReference(int i, RefTargetHandle rtarg) { lgtNode = (INode *)rtarg; }

	// StdNotifyRefChanged calls this, which can change the partID to new value 
	// If it doesnt depend on the particular message& partID, it should return
	// REF_DONTCARE
    RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
	    	PartID& partID, RefMessage message, BOOL propagate);

public:
	void Begin( IObjCreate *ioc, ClassDesc *desc );
	void End();
		
	TSpotCreationManager()	{ ignoreSelectionChange = FALSE; }
	int proc( HWND hwnd, int msg, int point, int flag, IPoint2 m );
	BOOL SupportAutoGrid(){return TRUE;}
};


#define CID_TSPOTCREATE	CID_USER + 3

class TSpotCreateMode : public CommandMode {
	TSpotCreationManager proc;
public:
	void Begin( IObjCreate *ioc, ClassDesc *desc ) { proc.Begin( ioc, desc ); }
	void End() { proc.End(); }

	int Class() { return CREATE_COMMAND; }
	int ID() { return CID_TSPOTCREATE; }
	MouseCallBack *MouseProc(int *numPoints) { *numPoints = 1000000; return &proc; }
	ChangeForegroundCallback *ChangeFGProc() { return CHANGE_FG_SELECTED; }
	BOOL ChangeFG( CommandMode *oldMode ) { return (oldMode->ChangeFGProc() != CHANGE_FG_SELECTED); }
	void EnterMode() {
		}
	void ExitMode() {
		}
	BOOL IsSticky() { return FALSE; }
};

static TSpotCreateMode theTSpotCreateMode;

void TSpotCreationManager::Begin( IObjCreate *ioc, ClassDesc *desc )
{
	createInterface = ioc;
	cDesc           = desc;
	attachedToNode  = FALSE;
	createCB        = NULL;
	lgtNode         = NULL;
	targNode        = NULL;
	lgtObject       = NULL;
	targObject      = NULL;
	CreateNewObject();
}

void TSpotCreationManager::End()
{
	if ( lgtObject ) {
		lgtObject->EndEditParams( (IObjParam*)createInterface, END_EDIT_REMOVEUI, NULL);
		if ( !attachedToNode ) {
			// RB 4-9-96: Normally the hold isn't holding when this 
			// happens, but it can be in certain situations (like a track view paste)
			// Things get confused if it ends up with undo...
			theHold.Suspend(); 
			//delete lgtObject;
			lgtObject->DeleteAllRefsFromMe();
			lgtObject->DeleteAllRefsToMe();
			lgtObject->DeleteThis();  // JBW 11.1.99, this allows scripted plugin lights to delete cleanly
			lgtObject = NULL;
			theHold.Resume();
			// RB 7/28/97: If something has been put on the undo stack since this object was created, we have to flush the undo stack.
			if (theHold.GetGlobalPutCount()!=lastPutCount) {
				GetSystemSetting(SYSSET_CLEAR_UNDO);
				}
			macroRec->Cancel();  // JBW 4/23/99
		} 
		else if ( lgtNode ) {
			 // Get rid of the reference.
			theHold.Suspend();
			DeleteReference(0);  // sets lgtNode = NULL
			theHold.Resume();
		}
	}	
}

RefResult TSpotCreationManager::NotifyRefChanged(
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
	 	if (lgtObject && lgtNode==hTarget) {
			// this will set camNode== NULL;
			theHold.Suspend();
			DeleteReference(0);
			theHold.Resume();
			goto endEdit;
		}
		// fall through

	case REFMSG_TARGET_DELETED:		
		if ( lgtObject && lgtNode==hTarget ) {
			endEdit:
			lgtObject->EndEditParams( (IObjParam*)createInterface, 0, NULL);
			lgtObject  = NULL;				
			lgtNode    = NULL;
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


void TSpotCreationManager::CreateNewObject()
{
	lgtObject = (GeneralLight *)cDesc->Create();
	lastPutCount = theHold.GetGlobalPutCount();

    macroRec->BeginCreate(cDesc);  // JBW 4/23/99
	
	// Start the edit params process
	if ( lgtObject ) {
		lgtObject->BeginEditParams( (IObjParam*)createInterface, BEGIN_EDIT_CREATE, NULL );
	}	
}

static void whoa(){};

static BOOL needToss;
			
int TSpotCreationManager::proc( 
				HWND hwnd,
				int msg,
				int point,
				int flag,
				IPoint2 m )
{	
   int res = CREATE_CONTINUE;
	TSTR targName;	
	ViewExp& vpx = createInterface->GetViewExp(hwnd); 
	assert( vpx.IsAlive() );

	switch ( msg ) {
	case MOUSE_POINT:
		switch ( point ) {
	case 0: {
			pt0 = m;
			assert( lgtObject );					
			vpx.CommitImplicitGrid(m, flag); //KENNY MERGE
			if ( createInterface->SetActiveViewport(hwnd) ) {
				return FALSE;
			}

			if (createInterface->IsCPEdgeOnInView()) { 
				res = FALSE;
				goto done;
			}

			// if lights were hidden by category, re-display them
			GetCOREInterface()->SetHideByCategoryFlags(
					GetCOREInterface()->GetHideByCategoryFlags() & ~HIDE_LIGHTS);

			if ( attachedToNode ) {
		   		// send this one on its way
		   		lgtObject->EndEditParams( (IObjParam*)createInterface, 0, NULL);
				macroRec->EmitScript();  // JBW 4/23/99
					
				// Get rid of the reference.
				if (lgtNode) {
					theHold.Suspend();
					DeleteReference(0);
					theHold.Resume();
				}

				// new object
				CreateNewObject();   // creates lgtObject
			}

			needToss = theHold.GetGlobalPutCount()!=lastPutCount;

		   	theHold.Begin();	 // begin hold for undo
			mat.IdentityMatrix();

			// link it up
			INode *lightNode  = createInterface->CreateObjectNode( lgtObject);
			attachedToNode = TRUE;
			assert( lightNode );					
			createCB = lgtObject->GetCreateMouseCallBack();					
			createInterface->SelectNode( lightNode );
					
			// Create target object and node
			targObject = new TargetObject;
			assert(targObject);
			targNode = createInterface->CreateObjectNode( targObject);
			assert(targNode);
			targName = lightNode->GetName();
			targName += GetString(IDS_DB_DOT_TARGET);
			macroRec->Disable();
			targNode->SetName(targName);
			macroRec->Enable();
				
			// hook up camera to target using lookat controller.
			createInterface->BindToTarget(lightNode,targNode);

			// Reference the new node so we'll get notifications.
			theHold.Suspend();
			ReplaceReference( 0, lightNode);
			theHold.Resume();
			
			// Position camera and target at first point then drag.
			mat.IdentityMatrix();
			//mat[3] = vpx.GetPointOnCP(m);
				mat.SetTrans( vpx.SnapPoint(m,m,NULL,SNAP_IN_3D) );
			createInterface->SetNodeTMRelConstPlane(lgtNode, mat);
			createInterface->SetNodeTMRelConstPlane(targNode, mat);
			lgtObject->Enable(1);

		   	ignoreSelectionChange = TRUE;
		   	createInterface->SelectNode( targNode,0);
		   	ignoreSelectionChange = FALSE;
			res = TRUE;

			// 6/19/01 11:37am --MQM-- 
			// set the wire-color of the light to be the default
			// color (yellow)
			if ( lgtNode ) 
			{
				Point3 color = GetUIColor( COLOR_LIGHT_OBJ );
				lgtNode->SetWireColor( RGB( color.x*255.0f, color.y*255.0f, color.z*255.0f ) );
			}
			break;
			}
					
		case 1:
			if (Length(m-pt0)<2)
				goto abort;
			//mat[3] = vpx.GetPointOnCP(m);
				mat.SetTrans( vpx.SnapPoint(m,m,NULL,SNAP_IN_3D) );
			macroRec->Disable();   // JBW 4/23/99
			createInterface->SetNodeTMRelConstPlane(targNode, mat);
			macroRec->Enable();

			ignoreSelectionChange = TRUE;
		   	createInterface->SelectNode( lgtNode);
		   	ignoreSelectionChange = FALSE;
					
		    theHold.Accept(IDS_DS_CREATE);	 

			createInterface->AddLightToScene(lgtNode); 
			createInterface->RedrawViews(createInterface->GetTime());  

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

		macroRec->SetProperty(lgtObject, _T("target"),   // JBW 4/23/99
			mr_create, Class_ID(TARGET_CLASS_ID, 0), GEOMOBJECT_CLASS_ID, 1, _T("transform"), mr_matrix3, &mat);

		res = TRUE;
		break;

	case MOUSE_FREEMOVE:
		SetCursor(UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::Crosshair));
			//Snap Preview
				vpx.SnapPreview(m,m,NULL, SNAP_IN_3D);
		vpx.TrackImplicitGrid(m); //KENNY MERGE
		break;

    case MOUSE_PROPCLICK:
		// right click while between creations
		createInterface->RemoveMode(NULL);
		break;
		
	case MOUSE_ABORT:
		abort:
		assert( lgtObject );
		lgtObject->EndEditParams( (IObjParam*)createInterface,0,NULL);
		// Toss the undo stack if param changes have been made
		macroRec->Cancel();  // JBW 4/23/99
		theHold.Cancel();	 // deletes both the camera and target.
		if (needToss) 
			GetSystemSetting(SYSSET_CLEAR_UNDO);
		lgtNode = NULL;			
		targNode = NULL;	 	
		createInterface->RedrawViews(createInterface->GetTime()); 
		CreateNewObject();	
		attachedToNode = FALSE;
		res = FALSE;						
	}
	
done:
	if ((res == CREATE_STOP)||(res==CREATE_ABORT))
		vpx.ReleaseImplicitGrid();
 
	return res;	
}

//----------------------------------------------------------------------

//#include "..\..\include\imaxscrpt.h"  //gdf 11,09.1999 see below.
//gdf 11.09.1999 SDK sample code imaxscrpt.h isn't part of the maxsdk (move to maxapi.h for R4)
#define I_EXEC_GET_DELEGATING_CLASSDESC	0x11002

int TSpotLightClassDesc::BeginCreate(Interface *i)
{
	SuspendSetKeyMode();
	IObjCreate *iob = i->GetIObjCreate();
	
	//iob->SetMouseProc( new LACamCreationManager(iob,this), 1000000 );

	ClassDesc* delegatingCD;			// JBW 11.1.99, allows fully functional extending scripted lights
										// by causing the custom command mode to create the delegating class
	i->Execute(I_EXEC_GET_DELEGATING_CLASSDESC, (ULONG_PTR)&delegatingCD);
	theTSpotCreateMode.Begin( iob, (delegatingCD ? delegatingCD : this) );

	iob->PushCommandMode( &theTSpotCreateMode );
	
	return TRUE;
}


int TSpotLightClassDesc::EndCreate(Interface *i)
{
	ResumeSetKeyMode();
	theTSpotCreateMode.End();
	i->RemoveMode( &theTSpotCreateMode );
	macroRec->EmitScript();  // JBW 4/23/99

	return TRUE;
}

//----------------------------------------------------------------------

int TDirLightClassDesc::BeginCreate(Interface *i)
	{
	SuspendSetKeyMode();
	IObjCreate *iob = i->GetIObjCreate();
	
	//iob->SetMouseProc( new LACamCreationManager(iob,this), 1000000 );

	ClassDesc* delegatingCD;			// JBW 11.1.99, allows fully functional extending scripted lights
										// by causing the custom command mode to create the delegating class
	i->Execute(I_EXEC_GET_DELEGATING_CLASSDESC, (ULONG_PTR)&delegatingCD);
	theTSpotCreateMode.Begin( iob, (delegatingCD ? delegatingCD : this) );
	iob->PushCommandMode( &theTSpotCreateMode );
	
	return TRUE;
	}

int TDirLightClassDesc::EndCreate(Interface *i)
	{
	ResumeSetKeyMode();
	theTSpotCreateMode.End();
	i->RemoveMode( &theTSpotCreateMode );

	return TRUE;
	}

//----------------------------------------------------------------------
//  Light descriptors for rendering
//----------------------------------------------------------------------

#define COS_45 0.7071067f
#define COS_45_2X 1.4142136f

static float stepFactor[] = {50.0f,80.0f,140.0f};
#define MAXSTEPS 1000

class AttenRanges {
	public:
		float aStart, aEnd;	// Attenuation start and end and hot spot scaling for volume shading		
		float aNearStart, aNearEnd;	// Near Attenuation start and end and hot spot scaling for volume shading		
		float decayRadius;
	};

class BaseObjLight : public ObjLightDesc {
	public:		
		ExclList exclList;
		Color intensCol;   	// intens*color	
		// RB: put these into a different structure and pass them to
		// AttenIllum to avoid threading problems.
		//float aStart, aEnd;	// Attenuation start and end and hot spot scaling for volume shading		
		//float aNearStart, aNearEnd;	// Near Attenuation start and end and hot spot scaling for volume shading		
		Color shadColor;
		float shadMult;
		float contrast,kA,kB,diffSoft;
		int decayType;
		Texmap *shadProjMap;
		BOOL ltAfctShad;
		BOOL diffSoften;
		BOOL dontNeedShadBuf;
		int atmosShadows;
		float atmosOpacity;
		float atmosColAmt;
		BOOL doingShadColor;
		BOOL ambOnly;
		BOOL isDefaultShadowMap;
		float decayRadius;
		GeneralLight* gl;
		BOOL forceShadowBuffer;

		BaseObjLight(INode *n, BOOL forceShadowBuf);
		void DeleteThis() {delete this;}
		ExclList* GetExclList() { return &exclList; }  
		int Update(TimeValue t, const RendContext& rc, RenderGlobalContext * rgc, BOOL shadows, BOOL shadowGeomChanged);
		void UpdateGlobalLightLevel(Color globLightLevel) {
			intensCol = ls.intens*ls.color*globLightLevel;
			}
		virtual void ApplyAtmosShadows(ShadeContext &sc, Point3 lpos, Color  &color, float &shadAtten); 
		inline void ApplyShadowColor(Color &color, const float &shadAtten, const Color &scol);
		inline void ApplyShadowColor(IlluminateComponents& illumComp, const float &shadAtten);
		float ApplyDistanceAtten( float dist );
		void ApplySpecularDiffuseAtten( Color& lightClr, IlluminateComponents& illumComp );

		void TraverseVolume(ShadeContext& sc,const Ray &ray,int samples,float tStop,float attenStart,float attenEnd,DWORD flags,LightRayTraversal *proc);
		virtual BOOL IntersectRay(Ray &ray, float &t0, float &t1, AttenRanges &ranges) {return FALSE;}
		virtual int  IntersectRayMultiple(Ray &ray, float t0, float t1, float *t);
		virtual Color AttenuateIllum(ShadeContext& sc,Point3 p,Color &colStep,Point3 &dp,int filt, float ldp, float &distAtten, AttenRanges &ranges) {return Color(0,0,0);}		
		virtual BOOL UseAtten()=0;
		virtual BOOL IsFacingLight(Point3 &dir) {return FALSE;}
		virtual int LightType()=0;

		ShadowType * ActiveShadowType();

		inline float ContrastFunc(float nl) {
			if (diffSoft!=0.0f) {
				float p = nl*nl*(2.0f-nl);	// based on Hermite interpolant 
				nl = diffSoft*p + (1.0f-diffSoft)*nl;
				}
			return (contrast==0.0f)? nl: 
				nl/(kA*nl+kB);  //  the "Bias" function described in Graphics Gems IV, pp. 401ff
			}
		inline float Decay(float att,float dist, float r0) {
			float s;
			if (decayType==DECAY_NONE || dist==0.0f) return att;
			if ((s=r0/dist)>=1.0f) return att;
			return  (decayType==DECAY_INVSQ)?  s*s*att: s*att;
			}
	};



BaseObjLight::BaseObjLight(INode *n, BOOL forceShadowBuf) : ObjLightDesc(n) {
	ObjectState os = n->EvalWorldState(TimeValue(0));
	assert(os.obj->SuperClassID()==LIGHT_CLASS_ID);
	gl = (os.obj->GetInterface(I_MAXSCRIPTPLUGIN) != NULL) ? (GeneralLight*)os.obj->GetReference(0) : (GeneralLight*)os.obj;  // JBW 4/7/99
	exclList = *(gl->GetExclList());
	forceShadowBuffer = forceShadowBuf;
	isDefaultShadowMap = ActiveShadowType()->SupportStdMapInterface();
	}	

static Color blackCol(0,0,0);
 
int BaseObjLight::Update(TimeValue t, const RendContext& rc, RenderGlobalContext * rgc, BOOL shadows, BOOL shadowGeomChanged) {
	ObjLightDesc::Update(t,rc,rgc,shadows,shadowGeomChanged);
	intensCol = ls.intens*ls.color*rc.GlobalLightLevel();
	ObjectState os = inode->EvalWorldState(t);
    GeneralLight* lob = (GeneralLight *)os.obj;		
	contrast = lob->GetContrast(t);
	diffSoft = lob->GetDiffuseSoft(t)/100.0f;
	decayType = lob->GetDecayType();
	//shadColor= lob->GetShadColor(t)*lob->GetShadMult(t);
	shadColor= lob->GetShadColor(t);
	shadMult = lob->GetShadMult(t);
	ambOnly = lob->GetAmbientOnly();
	decayRadius = lob->GetDecayRadius(t);
	doingShadColor = (shadColor == Color(0,0,0))?0:1;
	shadProjMap = lob->GetUseShadowColorMap(t)?lob->GetShadowProjMap():NULL;
	if( shadProjMap ){
		Interval v;
		shadProjMap->Update(t, v );
	}
	ltAfctShad = lob->GetLightAffectsShadow();
	ambOnly = lob->GetAmbientOnly();
	float a = contrast/204.0f + 0.5f; // so "a" varies from .5 to .99
	kA = (2.0f-1.0f/a);
	kB = 1.0f-kA;
	diffSoften = lob->GetSoftenDiffuse();
	//dontNeedShadBuf = (intensCol==Color(0,0,0))?1:0;   		

	// if light is black and don't have a shadowProjMap or a non-black shadow color,
	// then don't need a shadow buffer
	dontNeedShadBuf =  ((intensCol==blackCol)&&(!(shadProjMap||(!(shadColor==blackCol)))))?1:0;
			 	

	atmosShadows = /*lob->GetShadow()&&*/shadows&&lob->GetAtmosShadows(t);
	atmosOpacity = lob->GetAtmosOpacity(t);
	atmosColAmt = lob->GetAtmosColAmt(t);

	return 1;
	}

void BaseObjLight::ApplyAtmosShadows(ShadeContext &sc, Point3 lpos, Color  &color, float &shadAtten) 
{ 
	if (sc.globContext && sc.globContext->atmos && sc.shadow) {
		Color col, trans;
		trans = Color(1.f, 1.f, 1.f);
		col = Color(1.f,1.f,1.f);
        SCMode oldmode = sc.mode;
		sc.mode = SCMODE_SHADOW;
		sc.SetAtmosSkipLight(this);
		sc.globContext->atmos->Shade(sc, lightPos, sc.P(), col, trans);
		sc.SetAtmosSkipLight(NULL);
		sc.mode = oldmode;
		trans = 1.0f - (atmosOpacity * (1.0f - trans));
		col = 1.0f - (atmosColAmt * (1.0f - col));
		col *= trans;
		// CA - 7/28/03 - We multiply color by col and then divide
		// by the inverse of the intensity, because ApplyShadowColor
		// is going to multiply the color by the attenuation. This
		// works, unless col is (0,0,0), which causes atmospheric shadows
		// to mess. I changed this to check the intensity for 0,
		// and skip the multiply of color by col for this case.
		float a = Intens(col);
		if (a!=0) {
			color *= col;
			color *= 1.0f/a;   // DS 10/12/00: this is because ApplyShadowColor puts the attenuation back in.
		}
		shadAtten *= a;
	}
}

void BaseObjLight::ApplyShadowColor(Color &color, const float &shadAtten, const Color &scol) 
{
	float k =  (1.0f-shadAtten)*shadMult;
	if (k>=0.0f) {
		if (k<=1.0f) 
			// 0 < k < 1 
//			first eq: color - k * color + k * sCol * color => (1.0f-k)*color + k*sCol*color
			color = ltAfctShad? color*(1.0f-k+k*scol):	(1.0f-k)*color + k*scol;
		else  	
			// k>1			
			color = ltAfctShad? color*(k*scol): k*scol;
	} else {	// k<0
		color = ltAfctShad? color*(1.0f+k*scol): color + k*scol;
	}
}

// apply the shadow color to the light components
// NB, use only illumComp.lightDiffuseColor, finalColor & finalColorNS for output
// ...scrambled by diff/spec scaling after this call
void BaseObjLight::ApplyShadowColor(IlluminateComponents& illumComp, const float &shadAtten) 
{
	float k =  (1.0f-shadAtten)*shadMult;
	if (k>=0.0f) {
		if (k<=1.0f){ 
			// 0 < k < 1 
			illumComp.lightDiffuseColor = (1.0f-k) * illumComp.filteredColor;
			illumComp.shadowColor *= k;
		} else { 	
			// k>1			
			illumComp.lightDiffuseColor.Black();
			illumComp.shadowColor *= k;
		}
	} else {	// k<0
		illumComp.lightDiffuseColor = illumComp.filteredColor;
		illumComp.shadowColor *= k;	//NB...shadowClr is negative
	}
	// if the light affects the shadow, multiply it's color into the shadow color
	if( ltAfctShad )
			illumComp.shadowColor *= illumComp.filteredColor;

	// final color is the sum of the two
	illumComp.finalColor = illumComp.lightDiffuseColor + illumComp.shadowColor;
	illumComp.finalColorNS = illumComp.filteredColor;
}

float BaseObjLight::ApplyDistanceAtten( float dist )
{
	float atten = 1.0f;
	if(ls.useNearAtten) {
	   	if(dist<ls.nearAttenStart)		// Beyond range 
	   		return 0;
		if(dist<ls.nearAttenEnd)	{
			// in attenuated range
			float u = (dist - ls.nearAttenStart)/(ls.nearAttenEnd-ls.nearAttenStart);
			atten = u*u*(3-2*u);  /* smooth cubic curve */
			}
		}
	if (ls.useAtten) {
	   	if(dist>ls.attenEnd)		/* Beyond range */
	   		return 0;
		if(dist>ls.attenStart)	{
			/* Outside full-intensity range */
			float u = (ls.attenEnd-dist)/(ls.attenEnd-ls.attenStart);
			atten *= u*u*(3-2*u);  /* smooth cubic curve */
			}
		}

//	atten = Decay(atten,dist,ls.nearAttenEnd); // old way, changed by dan
	atten = Decay(atten,dist, decayRadius);
	return atten;
}

void BaseObjLight::ApplySpecularDiffuseAtten( Color& lightClr, IlluminateComponents& illumComp )
{
	Color black(0,0,0);

	illumComp.lightAmbientColor = ( ambOnly )? lightClr : black;
	illumComp.ambientAtten = ( ambOnly )? 1.0f : 0.0f;
	illumComp.lightDiffuseColor = ( !ambOnly && gl->affectDiffuse )? lightClr : black;
	illumComp.diffuseAtten = ( !ambOnly && gl->affectDiffuse  )? 1.0f : 0.0f;
	illumComp.lightSpecularColor = ( !ambOnly && gl->affectSpecular )? lightClr : black;
	illumComp.specularAtten = ( !ambOnly && gl->affectSpecular )? 1.0f : 0.0f;
}


int BaseObjLight::IntersectRayMultiple(Ray &ray, float t0, float t1, float *t) { 
	t[0] = t0;
	t[1] = t1;
	return 2;
	}

void BaseObjLight::TraverseVolume(
		ShadeContext& sc,
		const Ray &ray,int samples,float tStop,
		float attenStart, float attenEnd,
		DWORD flags,
		LightRayTraversal *proc)
	{
	Point3 dp, p;
	float ot, t, dt, ldp, t0, t1, dtLight, distAtten;
	Color illum, colStep;	
	Ray lr;
	int filt = 0;
	if (sc.GetAtmosSkipLight()==this)
		return;
	AttenRanges ranges;

#if 1	
	// > 4/18/02 - 1:23am --MQM-- 
	// if in a Light Tracer regathering ray, 
	// test whether or not we should skip this light
	if ( SHADECONTEXT_IS_REGATHERING( sc ) )
	{
		INodeGIProperties *props = static_cast<INodeGIProperties *>( inode->GetInterface( NODEGIPROPERTIES_INTERFACE ) ); // this is the inode of the light itself
		if ( props->GIGetIsExcluded() )
			return;
	}
#endif

	if (flags&TRAVERSE_LOWFILTSHADOWS) filt=1;
	if (flags&TRAVERSE_HIFILTSHADOWS) filt=2;
	if (flags&TRAVERSE_USESAMPLESIZE) filt=3;

	lr.p   = worldToLight * ray.p;
	lr.dir = VectorTransform(worldToLight,ray.dir);
	
	if (LightType()==DIR_LIGHT&&ls.useNearAtten&&ls.useAtten) {
		float mid = (ls.nearAttenEnd+ls.attenStart)/2.0f;
		ranges.aStart    = (ls.attenStart-mid)*attenStart + mid;
		ranges.aEnd      = (ls.attenEnd-mid)*attenEnd + mid;
		ranges.aNearStart= (ls.nearAttenStart-mid)*attenStart + mid;
		ranges.aNearEnd  = (ls.nearAttenEnd-mid)*attenEnd + mid;

		// DS 8/14/98  added decay radius, have to scale it like nearAttenEnd because that
		// is what we formely used for the decay radius.
		ranges.decayRadius = (decayRadius-mid)*attenEnd + mid;
		}
	else {
		ranges.aStart    = ls.attenStart * attenStart;
		ranges.aEnd      = ls.attenEnd * attenEnd;
		ranges.aNearStart= ls.nearAttenStart * attenStart;
		ranges.aNearEnd  = ls.nearAttenEnd * attenEnd;
		ranges.decayRadius  = decayRadius * attenEnd;
		}
		
	// See where we hit the light volume
	if (!IntersectRay(lr,t0,t1,ranges)) {
		// We missed.
		proc->Step(0.0f,tStop,Color(0,0,0),1.0f);
		return;
		}

	if (t1<0.0f || t0>tStop) {
		// We missed.
		proc->Step(0.0f,tStop,Color(0,0,0),1.0f);
		return;
		}
	if (t0<0.0f) t0 = 0.0f;
	if (t1>tStop) t1 = tStop;
	if (LightType()!=OMNI_LIGHT) {
		if (UseAtten() && (t1-t0) > (2.0f*ranges.aEnd)) {
			// We can only do this if we're not looking into a spot light.
			if (!IsFacingLight(lr.dir)) {
				t1 = 2.0f*ranges.aEnd + t0;
				}
			}
		}

	// The first (large) step
	if (t0!=0.0f) {
		if (!proc->Step(0.0f,t0,Color(0,0,0),1.0f)) {
			return;
			}
		}
	float tm[10];
	int n;	
	n = IntersectRayMultiple(lr,t0,t1,tm);

	dtLight = (t1-t0)/float(samples);
	
	for (int i=0; i<n-1; i++) {
		// Set everything up
		//dtLight = LightStepSize(sc,lr,t0,t1,minStep,filt);
		dt      = dtLight;
		if (dt<0.0001f) dt = 0.0001f;
		ot      = tm[i];
		t       = tm[i] + dt;	
		dp      = lr.dir * dt;
		ldp     = FLength(dp);
		colStep = intensCol * ldp;
		p       = lr.p + lr.dir*ot;

		// Traverse the ray inside of the light volume
		//for ( ; t<t1; t+=dt,p+=dp) {
		for (int sp=0; sp<samples && t<=tm[i+1]; sp++,t+=dt,p+=dp) {
			distAtten = 1.0f;
			illum = AttenuateIllum(sc,p,colStep,dp,filt,ldp,distAtten,ranges);

			// Call the callback.
			if (!proc->Step(ot,t,illum,distAtten)) {
				return;
				}
			ot = t;
			}

		// Do the remainder
		p      -= dp;
		ot      = t-dt;
		dt      = tm[i+1]-ot;
		dp      = lr.dir * dt;
		p      += dp;

		ldp     = FLength(dp);
		colStep = intensCol * ldp;

		// Attenuate the light at this point
		distAtten = 1.0f;
		illum = AttenuateIllum(sc,p,colStep, dp,filt,ldp,distAtten,ranges);

		// Call the callback.
		if (!proc->Step(ot,tm[i+1],illum,distAtten)) {
			return;
			}
		}

	// The last (large) step
	if (t1<tStop) {
		proc->Step(t1,tStop,Color(0,0,0),1.0f);
		}	
	}

ShadowType* BaseObjLight::ActiveShadowType()
{
	if( forceShadowBuffer ){
		if( gl->ASshadType == NULL ){
			if( gl->shadType->SupportStdMapInterface() )
				gl->ReplaceReference(AS_SHADTYPE_REF,gl->shadType);
			else 
				gl->ReplaceReference(AS_SHADTYPE_REF, NewDefaultShadowMapType() );
		}
		return gl->ASshadType; 

	}
	return gl->ActiveShadowType();
}


//--- Omni Light ------------------------------------------------

class OmniLight : public BaseObjLight, IIlluminationComponents {		
	ShadowGenerator *shadGen[6];
	Matrix3 tmCamToLight[6]; 
	BOOL shadow, doShadows, shadowRay, projector;
	int shadsize;
	BOOL needMultiple;
	BOOL genCanDoOmni;
	float zfac, xscale, yscale, fov, sz2,size,sizeClip,sampSize,sampSize2;
	public:		
		OmniLight(INode *inode, BOOL forceShadowBuf );
		~OmniLight();
		void FreeShadGenBuffers();
		void FreeShadGens();
		int Update(TimeValue t, const RendContext& rc, RenderGlobalContext *rgc, BOOL shadows, BOOL shadowGeomChanged);
		int UpdateViewDepParams(const Matrix3& worldToCam);
		AColor SampleProjMap(ShadeContext &sc,  Point3 p, Point3 dp, Texmap *map);
		BOOL Illuminate(ShadeContext &sc, Point3& normal, Color& color, Point3 &dir, float& dot_nl, float &diffuseCoef);
		BOOL Illuminate(ShadeContext &sc, Point3& normal, IlluminateComponents& illumComp );
		BOOL IntersectRay(Ray &ray,float &t0,float &t1, AttenRanges &ranges);
		BOOL NeedMultiple() { return needMultiple; }
		int  IntersectRayMultiple(Ray &ray, float t0, float t1, float *t);
		Color AttenuateIllum(ShadeContext& sc,Point3 p,Color &colStep,Point3 &dp,int filt, float ldp, float &distAtten, AttenRanges &ranges);		
		BOOL UseAtten() {return TRUE;}
		int LightType() { return OMNI_LIGHT; }
		BaseInterface *GetInterface(Interface_ID id) { 
			if(id == IID_IIlluminationComponents) 
				return (IIlluminationComponents*)this;
			else 
				return BaseObjLight::GetInterface(id);
		}
	};

OmniLight::OmniLight(INode *inode, BOOL forceShadowBuf ) : BaseObjLight(inode, forceShadowBuf){
	shadow = GetMarketDefaults()->GetInt(
		LIGHT_CLASS_ID, Class_ID( OMNI_LIGHT_CLASS_ID, 0),
		_T("CastShadows"), 0) != 0;
	projector = doShadows =  shadowRay =  FALSE; 
	Texmap* projMap = gl->projMap;
	needMultiple = FALSE;
	genCanDoOmni = ActiveShadowType()->CanDoOmni();
	for (int i=0; i<6; i++) 
		shadGen[i] = NULL;
	if(gl->GetShadow()) {
		if (genCanDoOmni)
			shadGen[0] = ActiveShadowType()->CreateShadowGenerator(gl, this, SHAD_OMNI); 
		else {
			for (int i=0; i<6; i++) {
				shadGen[i] = ActiveShadowType()->CreateShadowGenerator(gl, this, 0); 
				}
			}
		}
	}

OmniLight::~OmniLight() {
	FreeShadGens();
	}

void OmniLight::FreeShadGens() {
	for (int i=0; i<6; i++) {
		if (shadGen[i]) {
			shadGen[i]->DeleteThis();
			shadGen[i] = NULL;
			}
		}
	}

void OmniLight::FreeShadGenBuffers() {
	for (int i=0; i<6; i++) {
		if (shadGen[i]) {
			shadGen[i]->FreeBuffer();
			}
		}
	}


int OmniLight::UpdateViewDepParams(const Matrix3& worldToCam) {
	BaseObjLight::UpdateViewDepParams(worldToCam);
	for (int i=0; i<6; i++) 
		if (shadGen[i])
			shadGen[i]->UpdateViewDepParams(worldToCam);
	return 1;
	}

static Point3 MapToDir(Point3 p, int k) {
	switch(k) {
		case 0: return Point3(  p.z, p.y, -p.x); // +X
		case 1: return Point3( -p.z, p.y,  p.x); // -X
		case 2: return Point3(  p.x, p.z, -p.y); // +Y 
		case 3:	return Point3(  p.x,-p.z,  p.y); // -Y
		case 4: return Point3( -p.x, p.y, -p.z); // +Z
		case 5: return p;                        // -Z
		}
 	return p;
	}

static void GetMatrixForDir(Matrix3 &origm, Matrix3 &tm, int k ) {
	tm = origm;
	switch(k) {
		case 0:	tm.PreRotateY(-HALFPI); break;	// Map 0: +X axis	
		case 1: tm.PreRotateY( HALFPI); break;	// Map 1: -X axis	
		case 2:	tm.PreRotateX( HALFPI); break;	// Map 2: +Y axis	
		case 3:	tm.PreRotateX(-HALFPI); break; 	// Map 3: -Y axis	
		case 4:	tm.PreRotateY(   PI  );	break; 	// Map 4: +Z axis	
		case 5: 						break; 	// Map 5: -Z axis	
		}
	}

static int WhichDir(Point3 &p) {
	int j = MaxComponent(p);  // the component with the maximum abs value
	return  (p[j]<0.0f) ? 2*j+1 : 2*j;
	}

int OmniLight::Update(TimeValue t, const RendContext & rc,
		RenderGlobalContext *rgc, BOOL shadows, BOOL shadowGeomChanged)
{
	BaseObjLight::Update(t,rc,rgc,shadows,shadowGeomChanged);	

	ObjectState os = inode->EvalWorldState(t);
	LightObject* lob = (LightObject *)os.obj;		
	assert(os.obj->SuperClassID()==LIGHT_CLASS_ID);
	GeneralLight* gl = (lob->GetInterface(I_MAXSCRIPTPLUGIN) != NULL) ? (GeneralLight*)lob->GetReference(0) : (GeneralLight*)lob;  // JBW 4/7/99

	shadsize = ActiveShadowType()->MapSize(t);
	if (shadsize<=0) shadsize = 10;
	size = (float)shadsize;
	sizeClip = size-.0001f;
	sz2 = .5f*(float)shadsize;
	fov = HALFPI; // 90 degree fov
	zfac = -float(0.5*(double)shadsize /tan(fov*0.5));
	xscale = zfac;								
	yscale = -zfac;
	shadow = gl->GetShadow()&shadows;	
	if (ambOnly) shadow = FALSE;
	shadowRay = gl->GetShadowType();
	sampSize = gl->GetMapRange(t);
	sampSize2 = sampSize/2.0f;

	int res=1;

//	ULONG flags = gl->GetAbsMapBias()?SHAD_BIAS_ABSOLUTE:0;
	projector =  gl->GetProjector();
	if (projector){
		Texmap* projMap = gl->GetProjMap();
		if( projMap ) projMap->Update(t,FOREVER);
	}
	if (shadow)
	{	
		if (dontNeedShadBuf)
		{
			FreeShadGenBuffers();
		}
		else
		{
			float clipDist = ls.useAtten? ls.attenEnd : DONT_CLIP;

			if (shadowGeomChanged)
			{
				//NEWSHAD
				if (shadGen[0] && genCanDoOmni) // mjm - 06.08.00 - added check for non-NULL shadGen
				{
					if (!shadGen[0]->Update(t,rc,rgc,lightToWorld,1.0f,fov,clipDist))
						res = FALSE;
				}
				else
				{
					for (int i=0; i<6; i++)
					{
						if (shadGen[i]) // mjm - 06.08.00 - added check for non-NULL shadGen
						{
							Matrix3 tm;
							GetMatrixForDir(lightToWorld,tm,i);
							res = shadGen[i]->Update(t,rc,rgc,tm,1.0f,fov,clipDist);
						}
					}
				}
			}
		}
	}
	shadow = shadow && shadGen[0];	
	if (ambOnly)
		shadow = FALSE;  // Shouldn't this come before create shadow gens?
	if (dontNeedShadBuf)
		shadow = FALSE;
	needMultiple = shadow || (gl->projMap&&projector);
	return res;
}

//--- Omni Light Shade Context, for projector lights------------------------
class SCOmni: public ShadeContext {
	public:
		ShadeContext *origsc;
		TimeValue curtime;
		Point3 ltPos; // position of point in light space
		Point3 view;  // unit vector from light to point, in light space
		Point2 uv,duv;
		Point3 dp; 
		IPoint2 scrpos;
		float curve;
		int projType;

		BOOL 	  InMtlEditor() { return origsc->InMtlEditor(); }
		LightDesc* Light(int n) { return NULL; }
		TimeValue CurTime() { return curtime; }
		int NodeID() { return -1; }
		int FaceNumber() { return 0; }
		int ProjType() { return projType; }
		Point3 Normal() { return Point3(0,0,0); }
		Point3 GNormal() { return Point3(0,0,0); }
		Point3 ReflectVector(){ return Point3(0,0,0); }
		Point3 RefractVector(float ior){ return Point3(0,0,0); }
		Point3 CamPos() { return Point3(0,0,0); }
		Point3 V() { return view; }
		void SetView(Point3 p) { view = p; }
		Point3 P() { return ltPos; }	
		Point3 DP() { return dp; }
		Point3 PObj() { return ltPos; }
		Point3 DPObj(){ return Point3(0,0,0); } 
		Box3 ObjectBox() { return Box3(Point3(-1,-1,-1),Point3(1,1,1));}   	  	
		Point3 PObjRelBox() { return view; }
		Point3 DPObjRelBox() { return Point3(0,0,0); }
		void ScreenUV(Point2& UV, Point2 &Duv) { UV = uv; Duv = duv; }
		IPoint2 ScreenCoord() { return scrpos;} 
		Point3 UVW(int chan) { return Point3(uv.x, uv.y, 0.0f); }
		Point3 DUVW(int chan) { return Point3(duv.x, duv.y, 0.0f);  }
		void DPdUVW(Point3 dP[3],int chan) {}  // dont need bump vectors
		void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG=TRUE) {}   // returns Background color, bg transparency
		
		float Curve() { return curve; }

		// Transform to and from internal space
		Point3 PointTo(const Point3& p, RefFrame ito) { return p; } 
		Point3 PointFrom(const Point3& p, RefFrame ifrom) { return p; } 
		Point3 VectorTo(const Point3& p, RefFrame ito) { return p; } 
		Point3 VectorFrom(const Point3& p, RefFrame ifrom) { return p; } 
		SCOmni(){ doMaps = TRUE; 	curve = 0.0f;  dp = Point3(0.0f,0.0f,0.0f);		}
	};


AColor OmniLight::SampleProjMap(ShadeContext &sc,  Point3 p, Point3 dp, Texmap *map) {
	int j = WhichDir(p);  
	Point3 plt = MapToDir(p,j);	 // map plt into the buffer's space 
	float x = sz2 + xscale*plt.x/plt.z;
	float y = sz2 + yscale*plt.y/plt.z;

	SCOmni scomni;
	float ifs = 1.0f/(float)shadsize;
	scomni.projType = 0; // perspective
	scomni.curtime = sc.CurTime();
	scomni.curve = (float)fabs(1.0f/xscale); 
	scomni.ltPos = plt;
	scomni.view = FNormalize(p);
	scomni.uv.x = x*ifs;			
	scomni.uv.y = 1.0f-y*ifs;			
	scomni.scrpos.x = 0;
	scomni.scrpos.y = 0;
	scomni.filterMaps = sc.filterMaps;
	scomni.globContext = sc.globContext;
	scomni.mtlNum  = sc.mtlNum;
	scomni.origsc = &sc;


	float d;
	d = MaxAbs(dp.x,dp.y,dp.z);

	scomni.duv.x = scomni.duv.y = float(fabs(xscale*d*ifs/plt.z));
	d = 0.5*scomni.duv.x;

	if (scomni.uv.x<d) scomni.duv.x = scomni.uv.x*2.0f;
	else if (1.0f-scomni.uv.x<d) scomni.duv.x = (1.0f-scomni.uv.x)*2.0f;

	if (scomni.uv.y<d) scomni.duv.y = scomni.uv.y*2.0f;
	else if (1.0f-scomni.uv.y<d) scomni.duv.y = (1.0f-scomni.uv.y)*2.0f;

	// antialiasing for 3D textures: 
	scomni.dp = Point3(d,d,d);

	return map->EvalColor(scomni);
	}

static inline float Square(float v) { return v*v; }

//----------------------------------------
// Adjacent buffers:
//------------------------0	1 2	3 4	5 ----
static int  ADJPOSX[6] = {4,5,0,0,1,0};
static int  ADJNEGX[6] = {5,4,1,1,0,1};
static int  ADJPOSY[6] = {3,3,5,4,3,3};
static int  ADJNEGY[6] = {2,2,4,5,2,2};
//-----------------------------------------


//#define METHOD1

BOOL OmniLight::Illuminate(ShadeContext &sc, Point3& normal, Color& color, Point3 &dir, float& dot_nl, float &diffuseCoef) 
{
	if( ls.intens == 0.0f )
		return FALSE;

	float dist;
	Point3 p = sc.P();
	Point3 L = lightPos-p;  // vector from light to p in camera space
	Point3 plt;
	// quick test for light behind surface

	if (ambOnly) {
		if ((dist = FLength(L))==0.0f) return 0;
		L /= dist;
		dir = L;
		plt = camToLight * p; 
		dist = FLength(plt);
		diffuseCoef = dot_nl = 1.0f;
		}

	else {
		if (DotProd(normal,L)<=0.0f)	
			return 0;

		if ((dist = FLength(L))==0.0f) return 0;
		L /= dist;
		dir = L;
		plt = camToLight * p; 
		dist = FLength(plt);
		if (uniformScale)  {
			dot_nl = DotProd(normal,L);	
			}
		else {
			Point3	N = FNormalize(VectorTransform(camToLight,normal));
			dot_nl = -DotProd(N,plt/dist);
			if (dot_nl<=0.0f) return 0;
			}
		diffuseCoef = ContrastFunc(dot_nl);
		}

	float atten=1.0f;
	float shadAtten=1.0f;

	if(ls.useNearAtten) {
	   	if(dist<ls.nearAttenStart)		// Beyond range 
	   		return 0;
		if(dist<ls.nearAttenEnd)	{
			// in attenuated range
			float u = (dist - ls.nearAttenStart)/(ls.nearAttenEnd-ls.nearAttenStart);
			atten = u*u*(3-2*u);  // smooth cubic curve 
			}
		}

	if(ls.useAtten) {
	   	if(dist>ls.attenEnd)		// Beyond range 
	   		return 0;
		if(dist>ls.attenStart)	{
			// in attenuation range
			float u = (ls.attenEnd - dist)/(ls.attenEnd-ls.attenStart);
			atten *= u*u*(3-2*u);  // smooth cubic curve 
			}
		}

//	atten = Decay(atten,dist,ls.nearAttenEnd);
	atten = Decay(atten,dist,decayRadius);

	color = intensCol;
	float x,y;

	if (sc.shadow&&shadow) { 
		// determine which of 6 buffers contains the point.
		int j = WhichDir(plt);  
		if (!isDefaultShadowMap) {
			if (genCanDoOmni) 
				shadAtten = shadGen[0]->Sample(sc,normal,color);
			else 
				shadAtten = shadGen[j]->Sample(sc,normal,color);
			if (gl->projMap&&projector) {
//				Point3 q = MapToDir(plt,j);	 // map plt into the buffer's space 
//				x = sz2 + xscale*q.x/q.z;
//				y = sz2 + yscale*q.y/q.z;
				color *= SampleProjMap(sc,plt,sc.DP(), gl->projMap);
				}
			}
		else 	{
			Point3 q = MapToDir(plt,j);	 // map plt into the buffer's space 
			x = sz2 + xscale*q.x/q.z;
			y = sz2 + yscale*q.y/q.z;
			Point3 n = VectorTransform(camToLight,normal);
			Point3 n1 = MapToDir(n,j);
			float k0 = 1.0f/(zfac*DotProd(n1,q));
			float k = q.z*q.z*k0;
			float a = shadGen[j]->Sample(sc, x, y, q.z, -n1.x*k, n1.y*k);

			// Check for sample rectangles that overlap into the next buffer:
			if (x-sampSize2<0.0f) {
				int j2 = ADJNEGX[j];
				Point3 q2 = MapToDir(plt,j2);	
				float x2 = sz2 + xscale*q2.x/q2.z;
				float y2 = sz2 + yscale*q2.y/q2.z;
				Point3 n2 = MapToDir(n,j2);
				k = q2.z*q2.z*k0;
				float a2 = shadGen[j2]->Sample(sc, x2, y2, q2.z, -n2.x*k, n2.y*k);
				if (a2>=0)
					a = (a*(x+sampSize2) + a2*(sampSize2-x))/sampSize;
				}
			else if (x+sampSize2>size) {
				int j2 = ADJPOSX[j];
				Point3 q2 = MapToDir(plt,j2);
				float x2 = sz2 + xscale*q2.x/q2.z;
				float y2 = sz2 + yscale*q2.y/q2.z;
				Point3 n2 = MapToDir(n,j2);
				k = q2.z*q2.z*k0;
				float a2 = shadGen[j2]->Sample(sc, x2, y2, q2.z, -n2.x*k, n2.y*k);
				if (a2>=0)
					a = (a*(size-x+sampSize2) + a2*(x+sampSize2-size))/sampSize;
				}
			if (y-sampSize2<0.0f) {
				int j2 = ADJNEGY[j];
				Point3 q2 = MapToDir(plt,j2);
				float x2 = sz2 + xscale*q2.x/q2.z;
				float y2 = sz2 + yscale*q2.y/q2.z;
				Point3 n2 = MapToDir(n,j2);
				k = q2.z*q2.z*k0;
				float a2 = shadGen[j2]->Sample(sc, x2, y2, q2.z, -n2.x*k, n2.y*k);
				if (a2>=0)
					a = (a*(y+sampSize2) + a2*(sampSize2-y))/sampSize;
				}
			else if (y+sampSize2>size) {
				int j2 = ADJPOSY[j];
				Point3 q2 = MapToDir(plt,j2);
				float x2 = sz2 + xscale*q2.x/q2.z;
				float y2 = sz2 + yscale*q2.y/q2.z;
				Point3 n2 = MapToDir(n,j2);
				k = q2.z*q2.z*k0;
				float a2 = shadGen[j2]->Sample(sc, x2, y2, q2.z, -n2.x*k, n2.y*k);
				if (a2>=0)
					a = (a*(size-y+sampSize2) + a2*(y+sampSize2-size))/sampSize;
				}
			shadAtten = a;
			//atten *= a;  // DS 6-5-98  keep shadow atten separate for doing shadow color

			if (gl->projMap&&projector) 
				color *= SampleProjMap(sc,plt,sc.DP(),gl->projMap);
			}
		}
	else {
		// No shadows:
		if (gl->projMap&&projector) {
//			int j = WhichDir(plt);  
//			Point3 q = MapToDir(plt,j);	 // map plt into the buffer's space 
//			x = sz2 + xscale*q.x/q.z;
//			y = sz2 + yscale*q.y/q.z;
			color *= SampleProjMap(sc,plt,sc.DP(),gl->projMap);
			}
		}
	// ATMOSPHERIC SHADOWS
	if (gl->GetShadow() && atmosShadows)
		ApplyAtmosShadows(sc,lightPos,color, shadAtten);
	if (shadAtten!=1.0f) {
		Color scol = shadColor;
		if (shadProjMap) 
			scol = SampleProjMap(sc,plt,sc.DP(),shadProjMap);
		ApplyShadowColor(color, shadAtten,scol);
		}
	if (atten!=1.0f)
		color *= atten;
	return 1;
	}

// component output lighting
BOOL OmniLight::Illuminate(ShadeContext &sc, Point3& N, IlluminateComponents& illumComp )
{
	if( ls.intens == 0.0f )
		return FALSE;

	float dist;
	Point3 p = sc.P();
	illumComp.L = lightPos-p;  // vector from light to p in camera space
	Point3 plt;
	// quick test for light behind surface

	if (ambOnly) {
		// ambient only light...affects only mtl ambient channel
		if ((dist = FLength(illumComp.L))==0.0f) return 0;
		illumComp.L /= dist;
		plt = camToLight * p; 
		dist = FLength(plt);
		illumComp.geometricAtten = illumComp.NL = 1.0f;

	} else {
		// normal diffuse, specular light
		if ( DotProd(N, illumComp.L) <= 0.0f )	//quick out w/ unnormalized L
			return 0;
		
		// normalize L
		if ((dist = FLength(illumComp.L))==0.0f) return 0;
		illumComp.L /= dist;	

		plt = camToLight * p; // compute point in light space
		dist = FLength(plt);  // set up for distance atten	

		if ( uniformScale) {
			illumComp.NL = DotProd(N, illumComp.L );
		} else {
			// fix NL for non-uniform scale using light space points/vectors
			Point3	N = FNormalize(VectorTransform(camToLight, N));
			illumComp.NL = -DotProd(N, plt/dist);
			if (illumComp.NL <=0.0f ) 
				return 0;
		}
		illumComp.geometricAtten = ContrastFunc(illumComp.NL);

	}// end, not ambient only

	// do the distance attenuation
	illumComp.distanceAtten = ApplyDistanceAtten( dist );

	Color color = intensCol;
	// apply projector map
	if (gl->projMap&&projector) 
		color *= SampleProjMap(sc,plt,sc.DP(), gl->projMap);

	illumComp.rawColor = color;

	// now do shadow attenuation
	float shadAtten=1.0f;
	float x,y;
	if (sc.shadow && shadow) { 
		// determine which of 6 buffers contains the point.
		int j = WhichDir(plt);  

		if (!isDefaultShadowMap) {
			if (genCanDoOmni) 
				shadAtten = shadGen[0]->Sample(sc, N, color);
			else 
				shadAtten = shadGen[j]->Sample(sc, N, color);
	
		} else {
			// Default shadow map handling
			illumComp.filteredColor = color; // never any filtering for shadow maps
			Point3 q = MapToDir(plt,j);	 // map plt into the buffer's space 
			x = sz2 + xscale*q.x/q.z;
			y = sz2 + yscale*q.y/q.z;
			Point3 n = VectorTransform(camToLight,N);
			Point3 n1 = MapToDir(n,j);
			float k0 = 1.0f/(zfac*DotProd(n1,q));
			float k = q.z*q.z*k0;
			float a = shadGen[j]->Sample(sc, x, y, q.z, -n1.x*k, n1.y*k);

			// Check for sample rectangles that overlap into the next buffer:
			if (x-sampSize2<0.0f) {
				int j2 = ADJNEGX[j];
				Point3 q2 = MapToDir(plt,j2);	
				float x2 = sz2 + xscale*q2.x/q2.z;
				float y2 = sz2 + yscale*q2.y/q2.z;
				Point3 n2 = MapToDir(n,j2);
				k = q2.z*q2.z*k0;
				float a2 = shadGen[j2]->Sample(sc, x2, y2, q2.z, -n2.x*k, n2.y*k);
				if (a2>=0)
					a = (a*(x+sampSize2) + a2*(sampSize2-x))/sampSize;

			} else if (x+sampSize2>size) {
				int j2 = ADJPOSX[j];
				Point3 q2 = MapToDir(plt,j2);
				float x2 = sz2 + xscale*q2.x/q2.z;
				float y2 = sz2 + yscale*q2.y/q2.z;
				Point3 n2 = MapToDir(n,j2);
				k = q2.z*q2.z*k0;
				float a2 = shadGen[j2]->Sample(sc, x2, y2, q2.z, -n2.x*k, n2.y*k);
				if (a2>=0)
					a = (a*(size-x+sampSize2) + a2*(x+sampSize2-size))/sampSize;
			}

			if (y-sampSize2<0.0f) {
				int j2 = ADJNEGY[j];
				Point3 q2 = MapToDir(plt,j2);
				float x2 = sz2 + xscale*q2.x/q2.z;
				float y2 = sz2 + yscale*q2.y/q2.z;
				Point3 n2 = MapToDir(n,j2);
				k = q2.z*q2.z*k0;
				float a2 = shadGen[j2]->Sample(sc, x2, y2, q2.z, -n2.x*k, n2.y*k);
				if (a2>=0)
					a = (a*(y+sampSize2) + a2*(sampSize2-y))/sampSize;

			} else if (y+sampSize2>size) {
				int j2 = ADJPOSY[j];
				Point3 q2 = MapToDir(plt,j2);
				float x2 = sz2 + xscale*q2.x/q2.z;
				float y2 = sz2 + yscale*q2.y/q2.z;
				Point3 n2 = MapToDir(n,j2);
				k = q2.z*q2.z*k0;
				float a2 = shadGen[j2]->Sample(sc, x2, y2, q2.z, -n2.x*k, n2.y*k);
				if (a2>=0)
					a = (a*(size-y+sampSize2) + a2*(y+sampSize2-size))/sampSize;
			}
			shadAtten = a;

		}// end, else default shadow map
	} // end, shadows on

	// Atmosphere Shadows
	if (gl->GetShadow() && atmosShadows)
		ApplyAtmosShadows(sc, lightPos, color, shadAtten);

	// RT shadows & atmosphere's may affect color
	// never any filtering for shadow maps, raw == filtered color
	illumComp.filteredColor = color; 

	if( shadAtten != 1.0f) {
		illumComp.shadowColor = shadColor;
		illumComp.shadowAtten = shadAtten;
		if (shadProjMap) 
			illumComp.shadowColor = SampleProjMap(sc, plt, sc.DP(), shadProjMap);
		ApplyShadowColor( illumComp, shadAtten);
	
	} else {
		illumComp.shadowColor.Black();
		illumComp.shadowAtten = 1.0f;
		illumComp.lightDiffuseColor = illumComp.finalColor = illumComp.finalColorNS = illumComp.rawColor;
	}

	if (illumComp.distanceAtten != 1.0f){
		illumComp.finalColor *= illumComp.distanceAtten;
		illumComp.finalColorNS *= illumComp.distanceAtten;
		illumComp.lightDiffuseColor *= illumComp.distanceAtten;
		illumComp.shadowColor *= illumComp.distanceAtten;
	}

	ApplySpecularDiffuseAtten( illumComp.lightDiffuseColor, illumComp );
	illumComp.shapeAtten = 1.0f; // none for omni's
	
	return 1;
}



static int __cdecl cmp( const void *e1, const void *e2 ) {
	float &f1 = *((float *)e1);
	float &f2 = *((float *)e2);
	return f1<f2?-1:1;
	}

#define OkIntersect(tt,axis)  if (tt>=t0 && tt<=t1) { p = ray.p + tt*ray.dir;	if ( MaxComponent(p)!=axis) s[n++]=tt; }


int OmniLight::IntersectRayMultiple(Ray &ray, float t0, float t1, float *tm) {
	if (!needMultiple) { 
		tm[0] = t0;
		tm[1] = t1;
		return 2;
		}

	// intersect ray with the 6 sub-sectors;
	float t;
	float s[6];
	Point3 p;
	int n = 0;

	//---------------------------------------------------------
	//  plane:  n.P=0     ray:  P = p+t*dir
	//   =>  t = -n.p/n.dir
	//---------------------------------------------------------
	//  compute intersections with the 6 bounding planes.	
	t = -(ray.p.x+ray.p.y)/(ray.dir.x+ray.dir.y);  // x+y=0
	OkIntersect(t,2);
	t = -(ray.p.x-ray.p.y)/(ray.dir.x-ray.dir.y);  // x-y=0
	OkIntersect(t,2);
	t = -(ray.p.y+ray.p.z)/(ray.dir.y+ray.dir.z);  // y+z=0
	OkIntersect(t,0);
	t = -(ray.p.y-ray.p.z)/(ray.dir.y-ray.dir.z);  // y-z=0
	OkIntersect(t,0);
	t = -(ray.p.z+ray.p.x)/(ray.dir.z+ray.dir.x);  // z+x=0
	OkIntersect(t,1);
	t = -(ray.p.z-ray.p.x)/(ray.dir.z-ray.dir.x);  // z-x=0
	OkIntersect(t,1);

	tm[0] = t0;
	if (n==0) { tm[1] = t1;	return 2;	}
	if (n==1) { tm[1] = s[0];	tm[2] = t1;	return 3;	}
	if (n==2) {
		tm[3] = t1;
		if (s[0]>s[1]) { tm[1] = s[1];	tm[2] = s[0];	}
		else {	tm[1] = s[0];	tm[2] = s[1];	}
		return 4;
		}

#define ORDER3(n1,n2,n3) { tm[1] = s[n1];  tm[2] = s[n2];  tm[3] = s[n3]; }

	if (n==3) {
		tm[4] = t1;
		if (s[0]<s[1]) { 
			if (s[1]<s[2]) ORDER3(0,1,2)
			else {
				if (s[0]<s[2]) ORDER3(0,2,1)
				else ORDER3(2,0,1)
				}
			}
		else {	// s[1]<s[0]
			if (s[0]<s[2]) ORDER3(1,0,2)
			else {
				if (s[2]<s[1]) ORDER3(2,1,0)
				else ORDER3(1,2,0)
				}
			}
		return 5;
		}
	// I don't think this case can ever happen, but here's code for it.
	qsort(s,n,sizeof(float),cmp);
	for (int i=0; i<n; i++)
		tm[i+1] = s[i];
	tm[n+1] = t1;
	return n+2;
	}


BOOL OmniLight::IntersectRay(Ray &ray,float &t0,float &t1, AttenRanges &ranges)
	{
	float r = ranges.aEnd;
	float a, b, c, ac4, b2;
	float root;	

	a = DotProd(ray.dir,ray.dir);
	b = DotProd(ray.dir,ray.p) * 2.0f;
	c = DotProd(ray.p,ray.p) - r*r;
	
	ac4 = 4.0f * a * c;
	b2 = b*b;

	if (ac4 > b2) return FALSE;
	
	root = float(sqrt(b2-ac4));
	t0 = (-b + root) / (2.0f * a);
	t1 = (-b - root) / (2.0f * a);
	if (t0 > t1) {float temp=t0;t0=t1;t1=temp;}

	return TRUE;
	}

#define SIZECLIP(x) if (x>=sizeClip) x = sizeClip;	else if (x<0.0f) x = 0.0f;

Color OmniLight::AttenuateIllum(
		ShadeContext& sc,
		Point3 p,Color &colStep,Point3 &dp,int filt, float ldp, float &distAtten, AttenRanges &ranges)
	{
	// If we're not using attenuation, this is going to be a pretty boring volume!
	float dist = FLength(p);				 
	float atten = 1.0f;

//  DS 9/6/00: removed this test to fix #229053. I don't see why this test was put here.
//	if (!isDefaultShadowMap) 
//   		return Color(0.0f,0.0f,0.0f);

	distAtten = 1.0f;

	// Handle light distance attenuation 
	if(ls.useNearAtten) {
	   	if(dist<ranges.aNearStart) { // beyond range
			distAtten = 0.0f;
	   		return Color(0.0f,0.0f,0.0f);
			}
		if(dist<ranges.aNearEnd)	{
			// in attenuated range
			float u = (dist - ranges.aNearStart)/(ranges.aNearEnd-ranges.aNearStart);
			atten = distAtten = u*u*(3-2*u);  // smooth cubic curve 
			}
		}
	if (ls.useAtten) {		
	   	if(dist>ranges.aEnd)	{	// Beyond range 
			distAtten = 0.0f;
	   		return Color(0.0f,0.0f,0.0f);
			} 
		else 
		if(dist>ranges.aStart) {
			// in attenuated range
			float u = (ranges.aEnd-dist)/(ranges.aEnd-ranges.aStart);	
			atten = distAtten *= u*u*(3-2*u);
			}
		}	

//	atten = Decay(atten,dist,ranges.aNearEnd);
	atten = Decay(atten, dist, ranges.decayRadius);

//	if (sc.shadow && shadow && shadBuf[0]) { 
	if (sc.shadow && shadow && isDefaultShadowMap) { 
		// determine which of 6 buffers contains the point.
		Point3 pm = p+0.5f*dp;
		int j = WhichDir(pm);  // use midpoint to make sure we get correct buffer
		Point3 q = MapToDir(p,j);	 // map p into the buffer's space 
		float x,y;
		x = sz2 + xscale*q.x/q.z;
		y = sz2 + yscale*q.y/q.z;
		SIZECLIP(x);
		SIZECLIP(y);

		if (filt) {
			if (filt==1) { // Medium
				atten *= shadGen[j]->FiltSample(int(x), int(y), q.z, filt);
				}
			else 
			if (filt==2) {	// High
				Point3 q2 = MapToDir(p+dp,j);
				if (q2.z<0.0f) {
					float x2 = sz2 + xscale*q2.x/q2.z;
					float y2 = sz2 + yscale*q2.y/q2.z;
					SIZECLIP(x2);
					SIZECLIP(y2);
					float a = shadGen[j]->LineSample(int(x), int(y), q.z, int(x2), int(y2), q2.z);
					atten *= a;
					}
				}
			else
			if (filt==3) { // Use sample range
				atten *= shadGen[j]->Sample(sc,x,y,q.z,0.0f,0.0f);
				}
			if (atten==0.0f) return Color(0.0f,0.0f,0.0f);
			} 
		else {	//Low	
			if (!shadGen[j]->QuickSample(int(x), int(y), q.z)) {
				return Color(0.0f,0.0f,0.0f);
				}
			}		
		}
	if (gl->projMap&&projector) {
//		int j = WhichDir(p);  
//		Point3 q = MapToDir(p,j);	 // map plt into the buffer's space 
//		float x = sz2 + xscale*q.x/q.z;
//		float y = sz2 + yscale*q.y/q.z;
		Point3 pcam = lightToCam * p; 
		float d = sc.ProjType()? sc.RayDiam(): sc.RayConeAngle()*fabs(pcam.z);
		AColor pc = SampleProjMap(sc, p, Point3(d,d,d), gl->projMap);
		return atten*colStep*Color(pc.r,pc.g,pc.b);
		}
	
	return atten*colStep;
	}

//--- Spot Light Shade Context, for projector lights------------------------

class SCLight: public ShadeContext {
	public:
		ShadeContext *origsc;
		TimeValue curtime;
		Point3 ltPos; // position of point in light space
		Point3 view;  // unit vector from light to point, in light space
		Point3 dp; 
		Point2 uv,duv;
		IPoint2 scrpos;
		float curve;
		int projType;

		BOOL 	  InMtlEditor() { return origsc->InMtlEditor(); }
		LightDesc* Light(int n) { return NULL; }
		TimeValue CurTime() { return curtime; }
		int NodeID() { return -1; }
		int FaceNumber() { return 0; }
		int ProjType() { return projType; }
		Point3 Normal() { return Point3(0,0,0); }
		Point3 GNormal() { return Point3(0,0,0); }
		Point3 ReflectVector(){ return Point3(0,0,0); }
		Point3 RefractVector(float ior){ return Point3(0,0,0); }
		Point3 CamPos() { return Point3(0,0,0); }
		Point3 V() { return view; }
		void SetView(Point3 v) { view = v; }
		Point3 P() { return ltPos; }	
		Point3 DP() { return dp; }
		Point3 PObj() { return ltPos; }
		Point3 DPObj(){ return Point3(0,0,0); } 
		Box3 ObjectBox() { return Box3(Point3(-1,-1,-1),Point3(1,1,1));}   	  	
		Point3 PObjRelBox() { return view; }
		Point3 DPObjRelBox() { return Point3(0,0,0); }
		void ScreenUV(Point2& UV, Point2 &Duv) { UV = uv; Duv = duv; }
		IPoint2 ScreenCoord() { return scrpos;} 
		Point3 UVW(int chan) { return Point3(uv.x, uv.y, 0.0f); }
		Point3 DUVW(int chan) { return Point3(duv.x, duv.y, 0.0f);  }
		void DPdUVW(Point3 dP[3], int chan) {}  // dont need bump vectors
		void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG=TRUE) {}   // returns Background color, bg transparency
		
		float Curve() { return curve; }

		// Transform to and from internal space
		Point3 PointTo(const Point3& p, RefFrame ito) { return p; } 
		Point3 PointFrom(const Point3& p, RefFrame ifrom) { return p; } 
		Point3 VectorTo(const Point3& p, RefFrame ito) { return p; } 
		Point3 VectorFrom(const Point3& p, RefFrame ifrom) { return p; } 
		SCLight(){ doMaps = TRUE; 	curve = 0.0f;	dp = Point3(0.0f,0.0f,0.0f); }
	};

#define Dx (ray.dir.x)
#define Dy (ray.dir.y)
#define Dz (ray.dir.z)
#define Px (ray.p.x)
#define Py (ray.p.y)
#define Pz (ray.p.z)

//--- Directional Light ------------------------------------------------

class DirLight : public BaseObjLight, IIlluminationComponents {
	Point3 lightDir;  // light direction in render space	
	ShadowGenerator *shadGen;
	BOOL rect;
	BOOL overshoot,projector;
	BOOL shadow, doShadows, shadowRay;
	int shadsize;
	float hotsz, fallsz, fallsq;	
	float xscale, yscale, sz2, curve;
	float out_range,in_range, range_span;
	float hotpct,ihotpct;
	float aspect;
	float sw2, sh2;
//	Texmap* projMap;
	public:
		DirLight(INode *inode, BOOL forceShadowBuf );
		~DirLight() {
			FreeShadGens();
			}
		int Update(TimeValue t, const RendContext& rc, RenderGlobalContext *rgc, BOOL shadows, BOOL shadowGeomChanged);
		int UpdateViewDepParams(const Matrix3& worldToCam);
		void FreeShadGens() {	if (shadGen) { shadGen->DeleteThis();	shadGen = NULL;   }		}
		void FreeShadGenBuffers() {	if (shadGen)  shadGen->FreeBuffer();  		}
		BOOL Illuminate(ShadeContext &sc, Point3& normal, Color& color, Point3 &dir, float& dot_nl, float &diffuseCoef);
		BOOL Illuminate(ShadeContext &sc, Point3& N, IlluminateComponents& illumComp );
		virtual void ApplyAtmosShadows(ShadeContext &sc, Point3 lpos, Color  &color, float &shadAtten); 
		Color AttenuateIllum( ShadeContext& sc,	Point3 p,Color &colStep,Point3 &dp,int filt, float ldp, float &distAtten, AttenRanges &ranges);		
		BOOL IntersectRay(Ray &ray,float &t0,float &t1, AttenRanges &ranges);
		float SampleMaps(ShadeContext& sc, Point3 pin, Point3 norm, Point3 plt, Color& lcol, float &shadCol, IlluminateComponents* pIllumComp);
		AColor SampleProjMap(ShadeContext &sc, Point3 plt, Point3 dp, float x, float y, Texmap *map);
		float RectAtten(float px, float py);
		float CircAtten(float px, float py);
		BOOL UseAtten() {return FALSE;}
		int LightType() { return DIR_LIGHT; }
		BaseInterface *GetInterface(Interface_ID id) { 
			if(id == IID_IIlluminationComponents) 
				return (IIlluminationComponents*)this;
			else 
				return BaseObjLight::GetInterface(id);
		}
	};

DirLight::DirLight(INode *inode, BOOL forceShadowBuf ) : BaseObjLight(inode, forceShadowBuf) {
	shadow = GetMarketDefaults()->GetInt(
		LIGHT_CLASS_ID, Class_ID( OMNI_LIGHT_CLASS_ID, 0),
		_T("CastShadows"), 0) != 0;
	rect = overshoot = shadowRay =  FALSE; 
//	projMap = NULL;
	shadGen = NULL;
	if(gl->GetShadow()) 
		shadGen = ActiveShadowType()->CreateShadowGenerator(gl, this, SHAD_PARALLEL); 
	}

int DirLight::Update(TimeValue t, const RendContext &rc, 
		RenderGlobalContext *rgc, BOOL shadows, BOOL shadowGeomChanged)
{
	int res = 1;
	BaseObjLight::Update(t,rc,rgc,shadows,shadowGeomChanged);
	hotsz = ls.hotsize;
	fallsz = ls.fallsize;
	fallsq = fallsz*fallsz;
	hotpct = ls.hotsize/ls.fallsize;
	ihotpct = 1.0f - hotpct;

	ObjectState os = inode->EvalWorldState(t);
	LightObject* lob = (LightObject *)os.obj;		
	assert(os.obj->SuperClassID()==LIGHT_CLASS_ID);
	GeneralLight* gl = (lob->GetInterface(I_MAXSCRIPTPLUGIN) != NULL) ? (GeneralLight*)lob->GetReference(0) : (GeneralLight*)lob;  // JBW 4/7/99

	//bank = ls.bank;
	rect = gl->GetSpotShape()==RECT_LIGHT?1:0;
	overshoot = gl->GetOvershoot();
	shadow = gl->GetShadow()&&shadows;	
	if (ambOnly) shadow = FALSE;
	shadowRay = gl->GetShadowType();
	projector =  gl->GetProjector();

	aspect = rect?gl->GetAspect(t):1.0f;
//	shadsize = gl->GetMapSize(t);
	shadsize = ActiveShadowType()->MapSize(t);
	if (shadsize<=0) shadsize = 10;
	sz2 = .5f*(float)shadsize;
	xscale =  sz2/fallsz;
	curve =(float)fabs(1.0f/xscale); 
	yscale = -xscale*aspect;
	sw2  = fallsz;
	sh2  = sw2/aspect;

	if (projector){
//		projMap = gl->GetProjMap();
		if( gl->projMap ) gl->projMap->Update(t,FOREVER);
	}
	if (shadow)
	{
		if (dontNeedShadBuf)
		{
			FreeShadGenBuffers();
		}
		else
		{
			if (shadGen && shadowGeomChanged) // mjm - 06.08.00 - added check for non-NULL shadGen
			{
				float clipDist = ls.useAtten? ls.attenEnd : DONT_CLIP;
				//NEWSHAD
				res = shadGen->Update(t,rc,rgc,lightToWorld, aspect,fallsz,clipDist);
			}
		}
	}
	if (dontNeedShadBuf) shadow = FALSE;
//	doShadows = shadow && (shadBuf || rayBuf);	
	doShadows = shadow && shadGen;	
	return res;
};

int DirLight::UpdateViewDepParams(const Matrix3& worldToCam) {
	BaseObjLight::UpdateViewDepParams(worldToCam);
	// The light by convention shines down the negative Z axis, so
	// the lightDir is the positive Z axis
	lightDir = FNormalize(lightToCam.GetRow(2));

	// NEWSHAD
	if (shadGen) 
		shadGen->UpdateViewDepParams(worldToCam);
	return 1;
	}

#define DIR(i) ray.dir[i]
#define POS(i)   ray.p[i]
#define BIGFLOAT 1.0e15f
 
BOOL IntersectBox(Ray &ray,float &t0,float &t1, Box3& b) {
	float q0 = -BIGFLOAT;
	float q1 =  BIGFLOAT;
	for (int i=0; i<3; i++) {
		if (DIR(i)==0.0f) {	// parallel to this pair of planes
			if ( POS(i)<b.pmin[i] || POS(i)>b.pmax[i] ) return FALSE;
			}
		else {
			float r0 = (b.pmin[i]-POS(i))/DIR(i);
			float r1 = (b.pmax[i]-POS(i))/DIR(i);
			if (r0 > r1) {float temp=r0; r0=r1; r1=temp;}
			if (r0 > q0) q0 = r0;					
			if (r1 < q1) q1 = r1;					
			if (q0>q1) return FALSE;   // missed box
			if (q1<0.0f) return FALSE; // Box is behind ray origin.
			}
		}
	t0 = q0;
	t1 = q1;
	return TRUE;
	}

BOOL IntersectCyl(Ray &ray,float &t0,float &t1, float Rsq) {
	float A,B,C;
	A = (Dx*Dx + Dy*Dy);
	B = 2.0f*(Dx*Px + Dy*Py);
	C = Px*Px + Py*Py - Rsq;
	if (fabs(A)<float(1.0e-30))
		return FALSE;
	float d = B*B-4.0f*A*C;
	if (d<0.0f) 
		return FALSE;
	float s = (float)sqrt(d);
	t0 = (-B + s)/(2.0f*A);
	t1 = (-B - s)/(2.0f*A);
	if (t0 > t1) {float temp=t0; t0=t1; t1=temp;}

	int behind = 0;
	if (Pz + t0*Dz > 0.0f) behind = 1;
	if (Pz + t1*Dz > 0.0f) behind +=2;
	switch (behind) { 
		case 0: break;
		case 1: t0 = -Pz/Dz; 	break;  // intersect with plane z==0
		case 2: t1 = -Pz/Dz; 	break;	// intersect with plane z==0
		case 3: return FALSE;
		}
	return TRUE;
	}

BOOL DirLight::IntersectRay(Ray &ray,float &t0,float &t1, AttenRanges &ranges) {
//	if (overshoot) return FALSE;
	if (rect) {
		Box3 b;
		float sx = fallsz;
		float sy = fallsz/aspect;
		b.pmin = Point3( -sx, -sy, -BIGFLOAT);
		if (ls.useAtten)
			b.pmin.z = -ranges.aEnd;
		b.pmax = Point3(  sx,  sy, 0.0f);
		return  IntersectBox(ray,t0,t1,b);
		}
	else 
		return IntersectCyl(ray,t0,t1,fallsq);
	}

float DirLight::RectAtten(float px, float py) {
	float u = 0.0f;
	float ux = (float)fabs(px)/sw2;
	float uy = (float)fabs(py)/sh2;
	if (ux>1.0f||uy>1.0f) 
		return 0.0f;
	int inflag = 0;
	if(ux>hotpct) { inflag  = 1; ux = (ux-hotpct)/ihotpct; }
	if(uy>hotpct) {	inflag |= 2; uy = (uy-hotpct)/ihotpct; }
	switch(inflag) {
		case 1:	u = 1.0f-ux;			break;
		case 2:	u = 1.0f-uy;			break;
		case 3:	u = (1.0f-ux)*(1.0f-uy); break;
		case 0: return 1.0f;
		}
	return  u*u*(3.0f-2.0f*u);  
	}


float DirLight::CircAtten(float px, float py) {
	float dsq = px*px + py*py;
	if (dsq > fallsq) 
		return 0.0f;
	float d = (float) sqrt(dsq);
	if ( d > hotsz ) {
		float u = (fallsz - d)/(fallsz-hotsz);
		return u*u*(3.0f-2.0f*u); 
		}
	else return 1.0f;
	}

Color DirLight::AttenuateIllum(
		ShadeContext& sc,
		Point3 p,Color &colStep,Point3 &dp,int filt, float ldp, float &distAtten, AttenRanges &ranges)
	{		
	float atten = 1.0f;

	/* Handle light distance attenuation */
	distAtten = 1.0f;
	float dist = -p.z;

	if(ls.useNearAtten) {
	   	if(dist<ranges.aNearStart) { // beyond range
			distAtten = 0.0f;
	   		return Color(0.0f,0.0f,0.0f);
			}
		if(dist<ranges.aNearEnd)	{
			// in attenuated range
			float u = (dist - ranges.aNearStart)/(ranges.aNearEnd-ranges.aNearStart);
			atten = distAtten = u*u*(3-2*u);  // smooth cubic curve 
			}
		}
	if (ls.useAtten) {
	   	if(dist>ranges.aEnd) {		// Beyond range 
			distAtten = 0.0f;
	   		return Color(0.0f,0.0f,0.0f);
			}
		else if(dist>ranges.aStart){
			// in attenuated range
			float u = (ranges.aEnd-dist)/(ranges.aEnd-ranges.aStart);
			atten = distAtten *= u*u*(3-2*u);  /* smooth cubic curve */
			}
		}

//	atten = Decay(atten,dist,ranges.aNearEnd);
	atten = Decay(atten,dist, ranges.decayRadius);

	if (overshoot) {
		if (!(doShadows|| (gl->projMap&&projector) )) 
			return colStep;
		}
	else {
		if (p.z > 0.0f) 
			return Color(0,0,0); // behind light
		atten *= rect ?  RectAtten(p.x,p.y) : CircAtten(p.x,p.y);
		}

	float x,y;	

	x = sz2 + xscale*p.x;
	y = sz2 + yscale*p.y;				
	
	if (sc.shadow && shadow && shadGen) {
		if (filt==1) {						
			atten *= shadGen->FiltSample(int(x), int(y), p.z, filt);
			if (atten==0.0f) return Color(0.0f,0.0f,0.0f);
		} else if (filt==2) {
			Point3 p2 = p+dp;			
			int x2 = int(sz2 + xscale*p2.x);
			int y2 = int(sz2 + yscale*p2.y);
			atten *= shadGen->LineSample(
				int(x), int(y), p.z, x2, y2, p2.z);
							
			if (atten==0.0f) return Color(0.0f,0.0f,0.0f);
		} else if (filt==3) {
			// Use sample range
			atten *= shadGen->Sample(sc,x,y,p.z,0.0f,0.0f);
		} else {		
			if (!shadGen->QuickSample(int(x), int(y), p.z))
				return Color(0,0,0);
			}
		}

	if (gl->projMap&&projector) {
		Point3 pcam = lightToCam * p; 
		float d = sc.ProjType()? sc.RayDiam(): sc.RayConeAngle()*fabs(pcam.z);
		AColor pc = SampleProjMap(sc, p, Point3(d,d,d), x,y,gl->projMap);
		return atten*colStep*Color(pc.r,pc.g,pc.b);
		}
	
	return colStep * atten;
	}

static Color whitecol(1.0f,1.0f,1.0f);

BOOL DirLight::Illuminate(ShadeContext &sc, Point3& normal, Color& color, Point3 &dir, float& dot_nl, float &diffuseCoef) {
	float dot, atten;

	if( ls.intens == 0.0f )
		return FALSE;

	if (ambOnly) {
		dot = 1.0f;
		}
	else {
		dot = DotProd(normal,lightDir);
		if (dot<0.0f) 
			return 0;
		}

	color = intensCol;
	// find distance from the light axis:
	Point3 p = sc.P();
	Point3 q = camToLight*p; 

	atten = 1.0f;
	float dist = -q.z;

	// Handle light distance attenuation 
	if(ls.useNearAtten) {
	   	if(dist<ls.nearAttenStart)		// Beyond range 
	   		return 0;
		if(dist<ls.nearAttenEnd)	{
			// in attenuated range
			float u = (dist - ls.nearAttenStart)/(ls.nearAttenEnd-ls.nearAttenStart);
			atten = u*u*(3-2*u);  /* smooth cubic curve */
			}
		}
	if (ls.useAtten) {
	   	if(dist>ls.attenEnd)		/* Beyond range */
	   		return 0;
		if(dist>ls.attenStart)	{
			/* Outside full-intensity range */
			float u = (ls.attenEnd-dist)/(ls.attenEnd-ls.attenStart);
			atten *= u*u*(3-2*u);  /* smooth cubic curve */
			}
		}

//	atten = Decay(atten,dist,ls.nearAttenEnd);
	atten = Decay(atten,dist, decayRadius);

	if (overshoot) {
		if (!(doShadows||(gl->projMap&&projector))) {
			diffuseCoef = ContrastFunc(dot_nl = dot);
			dir = lightDir;
			if (atten!=1.0f) 
				color *= atten;
			return 1;
			}
		}
	else {
		if (q.z>=0.0f) return 0;
		atten*= rect? RectAtten(q.x, q.y) : CircAtten(q.x,q.y);
		if (atten==0.0f) return 0;
		}

	IPoint2 sp = sc.ScreenCoord();

	float shadAtten=1.0f;
	if (doShadows||(gl->projMap&&projector)) {
		atten *= SampleMaps(sc, p, normal,q, color,shadAtten,NULL);
	}
	if (atten==0.0f) return 0;


	// ATMOSPHERIC SHADOWS
	if (gl->GetShadow() && atmosShadows)
		ApplyAtmosShadows(sc,lightPos,color, shadAtten);

	dir = lightDir;
	diffuseCoef = ContrastFunc(dot_nl = dot);
	if (shadAtten!=1.0f) {
		Color scol = shadColor;
		if (shadProjMap) {
			float x =  xscale*q.x + sz2;
			float y =  yscale*q.y + sz2;
			scol = SampleProjMap(sc,p,sc.DP(),x,y,shadProjMap);
			}
		ApplyShadowColor(color, shadAtten,scol);
		}
	if (atten!=1.0f) 
		color *= atten;
	return 1;
	}

//Component wise Illuminate, if returns false components will not be set
BOOL DirLight::Illuminate(ShadeContext &sc, Point3& N, IlluminateComponents& illumComp )
{
	float dot, atten;

	if( ls.intens == 0.0f )
		return FALSE;

	if (ambOnly) {
		dot = 1.0f;
	} else {
		dot = DotProd( N,lightDir); // both are normalized
		if (dot < 0.0f) 
			return 0;
	}

	illumComp.L = lightDir;
	illumComp.NL = dot;
	illumComp.geometricAtten = ContrastFunc( dot );
	Color color = intensCol;

	// find distance from the light axis:
	Point3 p = sc.P();
	Point3 q = camToLight*p; 

	// Handle light distance attenuation 
	float dist = -q.z;
	atten = ApplyDistanceAtten( dist );
	illumComp.distanceAtten = atten;

	if (overshoot) {
		if ( ! (doShadows||(gl->projMap&&projector)) ){
			// no shadows or projector maps
			illumComp.shapeAtten = 1.0f;
			illumComp.shadowColor.Black();
			illumComp.shadowAtten = 1.0f;

			illumComp.finalColor = illumComp.finalColorNS 
				= illumComp.lightDiffuseColor = color * atten;
			ApplySpecularDiffuseAtten( illumComp.lightDiffuseColor, illumComp );
			return 1;
		}
	
	} else {
		// no overshoot, do shape attenuation
		if (q.z >= 0.0f) return 0;
		illumComp.shapeAtten = rect? RectAtten(q.x, q.y) : CircAtten(q.x,q.y);
		atten *= illumComp.shapeAtten;
		if (atten==0.0f) return 0;
	}

//	IPoint2 sp = sc.ScreenCoord();

	// do the shadow/projector map,
	float shadAtten=1.0f;
	if (doShadows|| (gl->projMap&&projector)) {
		atten *= SampleMaps(sc, p, N, q, color, shadAtten, &illumComp);
	}
	if (atten==0.0f) return 0;


	// compute Atmosphere Shadows
	if (gl->GetShadow() && atmosShadows)
		ApplyAtmosShadows(sc, lightPos, color, shadAtten);

	illumComp.filteredColor = illumComp.rawColor =color;

	// apply the shadows
	if (shadAtten!=1.0f) {
		illumComp.shadowColor = shadColor;
		illumComp.shadowAtten = shadAtten;
		if (shadProjMap) {
			float x =  xscale*q.x + sz2;
			float y =  yscale*q.y + sz2;
			illumComp.shadowColor = SampleProjMap(sc,p,sc.DP(),x,y,shadProjMap);
		}
		ApplyShadowColor(illumComp, shadAtten);

	} else {
		illumComp.shadowColor.Black();
		illumComp.shadowAtten = 1.0f;
		illumComp.lightDiffuseColor = illumComp.finalColor = illumComp.finalColorNS = illumComp.rawColor;
	}

	// apply composite attenuation = distanceAtten * shapeAtten
	if (atten!=1.0f) {
		illumComp.finalColor *= atten;
		illumComp.finalColorNS *= atten;
		illumComp.lightDiffuseColor *= atten;
		illumComp.shadowColor *= atten;
	}
	ApplySpecularDiffuseAtten( illumComp.lightDiffuseColor, illumComp );
	return 1;
}


void DirLight::ApplyAtmosShadows(ShadeContext &sc, Point3 lpos, Color  &color, float &shadAtten) 
{ 
	if (sc.globContext && sc.globContext->atmos && sc.shadow) {
		Color col, trans;
		trans = Color(1.f, 1.f, 1.f);
		col = Color(1.f,1.f,1.f);
        SCMode oldmode = sc.mode;
		sc.mode = SCMODE_SHADOW;
		sc.SetAtmosSkipLight(this);
		// For directional lights this is a little bit different. We need to
		// calculate the point on the plane of the light (perpendicular to
		// the light direction), and compute the shadow from there.
		// To calculate the point on the plane, pos, we use these two equations:
		//    DotProd(lightPos, lightDir) = DotProd(pos, lightDir); p is on the plane
		//    pos = sc.P() + t * lightDir; pos is on the ray through sc.P()
		//    Solving we get t = DotProd(lightPos - sc.P(), lightDir);
		Point3 p = sc.P();
		sc.globContext->atmos->Shade(sc, p + lightDir * DotProd(lightPos - p, lightDir),
			p, col, trans);
		sc.SetAtmosSkipLight(NULL);
		sc.mode = oldmode;
		trans = 1.0f - (atmosOpacity * (1.0f - trans));
		col = 1.0f - (atmosColAmt * (1.0f - col));
		col *= trans;
		// CA - 7/28/03 - We multiply color by col and then divide
		// by the inverse of the intensity, because ApplyShadowColor
		// is going to multiply the color by the attenuation. This
		// works, unless col is (0,0,0), which causes atmospheric shadows
		// to mess. I changed this to check the intensity for 0,
		// and skip the multiply of color by col for this case.
		float a = Intens(col);
		if (a!=0) {
			color *= col;
			color *= 1.0f/a;   // DS 10/12/00: this is because ApplyShadowColor puts the attenuation back in.
		}
		shadAtten *= a;
	}
}

AColor DirLight::SampleProjMap(ShadeContext &sc,  Point3 plt, Point3 dp, float x, float y, Texmap* map) {
	SCLight sclight;
	float ifs = 1.0f/(float)shadsize;
	sclight.origsc = &sc;
	sclight.projType = 1; // parallel
	sclight.curtime = sc.CurTime();;
	sclight.curve = curve; 
	sclight.ltPos = plt;
	sclight.view = FNormalize(Point3(plt.x, plt.y, 0.0f));
	sclight.uv.x = x*ifs;			
	sclight.uv.y = 1.0f-y*ifs;			
	sclight.uv.x = x*ifs;			
	sclight.uv.y = 1.0f-y*ifs;			
	sclight.scrpos.x = (int)(x+0.5);
	sclight.scrpos.y = (int)(y+0.5);
	sclight.filterMaps = sc.filterMaps;
	sclight.mtlNum  = sc.mtlNum;
	sclight.globContext = sc.globContext;

	// antialiasing for 2D textures:
	// how big is the pixel in terms of 3D space:
	float d;
	d = MaxAbs(dp.x,dp.y,dp.z);

	float duv = fabs(d * ifs * xscale);
	if (duv>0.4f) duv = 0.4f;
	sclight.duv.x = sclight.duv.y = duv;

	// clip 
	float hd = duv*0.5f;
	if (sclight.uv.x + hd >0.999999) sclight.uv.x = .9999999-hd;
	if (sclight.uv.x - hd <0.0f) sclight.uv.x = hd;
	if (sclight.uv.y + hd >0.999999) sclight.uv.y = .9999999-hd;
	if (sclight.uv.y - hd <0.0f) sclight.uv.y = hd;

	// antialiasing for 3D textures: 
	sclight.dp = Point3(d,d,d);

	// antialiasing forg for 3D textures: TBD

	return map->EvalColor(sclight);
	}

float DirLight::SampleMaps(ShadeContext& sc, Point3 pin, Point3 norm, Point3 plt, Color& lcol, float &shadAtten, IlluminateComponents* pIllumComp)
{
	float mx,my,x,y,atten;
	Point3 n;

	atten = 1.0f;
	mx =  xscale*plt.x;
	my =  yscale*plt.y;

	// If it falls outside the rectangle, bail out 
	if ((float)fabs(mx)>sz2 || (float)fabs(my)>sz2)	{
		if(pIllumComp) {
			pIllumComp->shapeAtten = overshoot ? 1.0f:0.0f;
			pIllumComp->filteredColor = pIllumComp->rawColor = lcol;
		}
		if (overshoot) return(1.0f);	// No attenuation 
		else return(0.0f);
	}

	x = sz2 + mx;
	y = sz2 + my;

	if (doShadows&&sc.shadow) {
		if (isDefaultShadowMap) {
			Point3 n = VectorTransform(camToLight,norm);
			if (n.z!=0.0f) {
				shadAtten = shadGen->Sample(sc, x, y, plt.z, -n.x/(n.z*xscale), -n.y/(n.z*yscale));
				if (shadAtten==0.0f&&!doingShadColor) return atten;
			}
		} else {
			shadAtten = shadGen->Sample(sc,norm,lcol);
			if (shadAtten==0.0f&&!doingShadColor) return atten;
		}
	}
	if(pIllumComp) 
		pIllumComp->filteredColor = lcol;
	
	if (gl->projMap&&projector) {
		//Color c = SampleProjMap(sc,plt,sc.DP(),x,y, gl->projMap);
		/*lcol *= c;
		if(pIllumComp){
			pIllumComp->rawColor *= c;
			pIllumComp->filteredColor *= c;
		}*/
	}
	return(atten);
}

//--- Spot Light -------------------------------------------------------
class SpotLight: public BaseObjLight, IIlluminationComponents  {	
	friend class SCLight;
	Point3 lightDir;  // light direction in render space
	ShadowGenerator *shadGen;
	BOOL rect;
	BOOL overshoot,projector, shadowRay;
	BOOL shadow;
	int shadsize;
	float hot_cos, fall_cos, fall_tan, fall_sin;
	float hotpct, ihotpct;	
	float zfac, xscale,yscale, fov, sz2, curve;
	float out_range,in_range, range_span;
	Point2 rectv0, rectv1;
//	Texmap* projMap;
	public:
		SpotLight(INode *inode, BOOL forceShadowBuf );
		~SpotLight() {	FreeShadGens();		}
		void FreeShadGens() {	if (shadGen) { shadGen->DeleteThis();	shadGen = NULL;   }		}
		void FreeShadGenBuffers() {		if (shadGen)  shadGen->FreeBuffer(); 	}
		int Update(TimeValue t, const RendContext& rc, RenderGlobalContext *rgc, BOOL shadows, BOOL shadowGeomChanged);
		int UpdateViewDepParams(const Matrix3& worldToCam);
		BOOL Illuminate(ShadeContext &sc, Point3& normal, Color& color, Point3 &dir, float& dot_nl, float &diffuseCoef);
		BOOL Illuminate(ShadeContext &sc, Point3& N, IlluminateComponents& illumComp );
		float SampleMaps(ShadeContext& sc, Point3 pin, Point3 norm, Point3 plt, Color& lcol, float &shadAtten, IlluminateComponents* pIllumComp=NULL);
		BOOL IntersectCone(Ray &ray,float tanAng,float &t0,float &t1);
		BOOL IntersectRectCone(Ray &ray,float &t0,float &t1);
		BOOL IntersectRay(Ray &ray,float &t0,float &t1, AttenRanges &ranges);
		Color AttenuateIllum(ShadeContext& sc, Point3 p, Color &colStep,Point3 &dp,int filt, float ldp, float &distAtten, AttenRanges &ranges);		
		AColor SampleProjMap(ShadeContext& sc, Point3 plt, Point3 dp, float x, float y, Texmap *map);
		BOOL UseAtten() {return ls.useAtten;}
		BOOL IsFacingLight(Point3 &dir);
		int LightType() { return FSPOT_LIGHT; }
		BaseInterface *GetInterface(Interface_ID id) { 
			if(id == IID_IIlluminationComponents) 
				return (IIlluminationComponents*)this;
			else 
				return BaseObjLight::GetInterface(id);
		}
	};

SpotLight::SpotLight(INode *inode, BOOL forceShadowBuf ):BaseObjLight(inode, forceShadowBuf) {
	shadow = GetMarketDefaults()->GetInt(
		LIGHT_CLASS_ID, Class_ID( OMNI_LIGHT_CLASS_ID, 0),
		_T("CastShadows"), 0) != 0;
	rect = overshoot =  shadowRay = FALSE; 
	shadGen = NULL;
	if(gl->GetShadow()) 
		shadGen = ActiveShadowType()->CreateShadowGenerator(gl, this, 0); 
//	projMap = NULL;
	}


BOOL SpotLight::IntersectCone(
		Ray &ray,float tanAng,float &t0,float &t1)
	{
#define FLOAT double
	FLOAT A, Dx2, Dy2, Dz2, den, k, root, z0, z1;

	Dx2 = Dx*Dx;
	Dy2 = Dy*Dy;
	Dz2 = Dz*Dz;
	A   = tanAng*tanAng;	

	den = 2.0f*(Dx2+Dy2-A*Dz2);
	if (den == 0.0f ) return FALSE;

	k = -2.0f*Dx*Px - 2.0f*Dy*Py + 2.0f*A*Dz*Pz;
	root = k*k - 2.0f*den*(Px*Px+Py*Py-A*Pz*Pz);
	if (root<0.0f) return FALSE;
	root = (float)sqrt(root);

	t0 = (float)((k-root)/den);
	t1 = (float)((k+root)/den);
	if (t0 > t1) {float temp=t0;t0=t1;t1=temp;}

	// t0 and t1 may be the iintersection with the reflected cone in the
	// positive Z direction.
	z0 = ray.p.z + ray.dir.z*t0;
	z1 = ray.p.z + ray.dir.z*t1;
	BOOL out0 = z0 > 0.0f ? TRUE : FALSE;
	BOOL out1 = z1 > 0.0f ? TRUE : FALSE;
	if (out0 && out1) {
		return FALSE;	
		}

	if (out0) {			  
		// Must be in the reflected cone, shooting out and hitting the real cone.
		
		// Do an extra check to make sure we really are inside the
		// cone and it's not just round-off.
		if (t0>0) {
			FLOAT l = (Px*Px + Py*Py)/A;
			if (Pz*Pz < l) return FALSE;
			}

		t0 = t1;
		t1 = float(1.0E30);
		}

	if (out1) {
		// Must be inside the cone, shooting out and hitting the reflected cone.
		t1 = t0;
		t0 = 0.0f;
		}
		
	return TRUE;
	}


static BOOL IntersectConeSide(
		Ray &ray,int X, int Y,
		float cs, float sn, float tn,		
		float &t)
	{
	Point2 p(ray.p[X],ray.p.z), v(cs,sn), c(ray.dir[X],ray.dir.z);
	float cv = -c.DotProd(v);
	if (cv!=0.0f) {
		float pv = p.DotProd(v);
		t = pv/cv;
		if (t>0.0f) {
			Point3 pt = ray.p + ray.dir*t;
			if (pt.z<0.0f) {
				if ((float)fabs(pt[Y]/pt.z) <= tn) {
					return TRUE;
					}
				}
			}
		}
	return FALSE;
	}

BOOL SpotLight::IntersectRectCone(
		Ray &ray,float &t0,float &t1)
	{	
	float sn0 = rectv0.y;
	float sn1 = rectv1.y;
	float cs0 = rectv0.x;
	float cs1 = rectv1.x;
	float tn0 = (float)fabs(sn0/cs0);
	float tn1 = (float)fabs(sn1/cs1);
	int hit = 0;
	float t;
	BOOL in = FALSE;
	
	if (IntersectConeSide(ray, 0, 1, cs0, sn0, tn1, t)) {		
		t0 = t1 = t;
		hit++;
		}

	if (IntersectConeSide(ray, 1, 0, cs1, sn1, tn0, t)) {
		if (hit) {
			if (t<t0) t0 = t;
			if (t>t1) t1 = t;
		} else {
			t0 = t1 = t;
			}
		hit++;
		}

	if (IntersectConeSide(ray, 0, 1, -cs0, sn0, tn1, t)) {
		if (hit) {
			if (t<t0) t0 = t;
			if (t>t1) t1 = t;
		} else {
			t0 = t1 = t;
			}
		hit++;
		}

	if (IntersectConeSide(ray, 1, 0, cs1, -sn1, tn0, t)) {
		if (hit) {
			if (t<t0) t0 = t;
			if (t>t1) t1 = t;
		} else {
			t0 = t1 = t;
			}
		hit++;
		}
	
	float xz = float(fabs(ray.p.x)/fabs(ray.p.z));
	float yz = float(fabs(ray.p.y)/fabs(ray.p.z));
	if (ray.p.z < 0.0f && xz<tn0 && yz<tn1) {
		in = TRUE;
		}

	if (hit == 0) {
		if (in) {
			t0 = 0.0f;
			t1 = float(1.0E30);
			return TRUE;
		} else {
			return FALSE;
			}
		}

	if (hit == 1) {
		if (in) t0 = 0.0f;			
		else t1 = float(1.0E30);			
		}
	
	return TRUE;
	}

#undef Dx
#undef Dy
#undef Dz
#undef Px
#undef Py
#undef Pz

BOOL SpotLight::IntersectRay(Ray &ray,float &t0,float &t1, AttenRanges &ranges)
	{
	BOOL res = rect ? IntersectRectCone(ray,t0,t1): IntersectCone(ray,fall_tan,t0,t1);	
	if (ls.useAtten&&(ray.dir.z!=0.0f)) {

		// intersect ray with plane at z = -ranges.aFarEnd;
		float zf = -ranges.aEnd;
		float t = (zf-ray.p.z)/ray.dir.z;		
		if (ray.p.z<zf) { 
			// p beyond far end plane
			if (ray.dir.z>0) {
				// ray pointing toward light
				if (t>t0)
					t0 = t;
				}
			}
		if (ray.p.z>zf) {
			// p on same side of far end plane as light
			if (ray.dir.z<0) {
				// ray pointing away from light, may hit far end plane
				if (t<t1)
					t1 = t;
				}
			}	
		}
	return res;
	}

BOOL SpotLight::IsFacingLight(Point3 &dir)
	{
	return dir.z>0.0f;
	}

Color SpotLight::AttenuateIllum(
		ShadeContext& sc,
		Point3 p,Color &colStep,Point3 &dp,int filt, float ldp, float &distAtten, AttenRanges &ranges)
	{		
	float mx,my,x,y;	
	float atten = 1.0f, cosang;
	float lv = FLength(p);
	if (lv==0.0f) 
		return colStep;	

	// Handle light distance attenuation 
	distAtten = 1.0f;
	if(ls.useNearAtten) {
	   	if(lv<ranges.aNearStart) { // beyond range
			distAtten = 0.0f;
	   		return Color(0.0f,0.0f,0.0f);
			}
		if(lv<ranges.aNearEnd)	{
			// in attenuated range
			float u = (lv - ranges.aNearStart)/(ranges.aNearEnd-ranges.aNearStart);
			atten = distAtten = u*u*(3-2*u);  // smooth cubic curve 
			}
		}
	if(ls.useAtten) {		
		if(lv>ranges.aEnd) {		// Beyond range
			distAtten = 0.0f;
			return Color(0,0,0);
			}
		if(lv>ranges.aStart)	{
			// in attenuated range
			float u = (ranges.aEnd-lv)/(ranges.aEnd-ranges.aStart);
			atten = distAtten *= u*u*(3-2*u); /* smooth cubic curve */			  
			}
		}

//	atten = Decay(atten, lv, ranges.aNearEnd);
	atten = Decay(atten, lv, ranges.decayRadius);
	
	cosang = (float)fabs(p.z/lv);
	if ((!rect)&&(!overshoot) && (cosang<hot_cos) && (hot_cos!=fall_cos)) {
		if (cosang<fall_cos) {
			// RB: point is outside of falloff cone
			return Color(0.0f,0.0f,0.0f);
			}
		float u = (cosang-fall_cos)/(hot_cos-fall_cos);
		atten *= u*u*(3.0f-2.0f*u);  /* smooth cubic curve */
		}
	
	// ---DS -- 4/8/96 added ||projMap

	if ((p.z<0.0f && (sc.shadow && shadow)) || rect || (gl->projMap&&projector)) { 
		mx =  xscale*p.x/p.z;
		my =  yscale*p.y/p.z;
		x = sz2 + mx;
		y = sz2 + my;				
				
		if (sc.shadow && shadow) {
			if (filt) {
				if (filt==1) {
					atten *= shadGen->FiltSample(int(x), int(y), p.z, filt);
					} 
				else if (filt==2) {
					Point3 p2 = p+dp;
					if (p2.z<0.0f) {
						int x2 = int(sz2 + xscale*p2.x/p2.z);
						int y2 = int(sz2 + yscale*p2.y/p2.z);
						atten *= shadGen->LineSample(
							int(x), int(y), p.z, x2, y2,p2.z);
						}
					}
				else if (filt==3) {
					// Use sample range
					atten *= shadGen->Sample(sc,x,y,p.z,0.0f,0.0f);
					}
				if (atten==0.0f) return Color(0.0f,0.0f,0.0f);
			} else {		
				if (!shadGen->QuickSample(int(x), int(y), p.z)) {
					return Color(0.0f,0.0f,0.0f);
					}
				}
			}		
		if (gl->projMap&&projector) {
			Point3 pcam = lightToCam * p; 
			float d = sc.ProjType()? sc.RayDiam(): sc.RayConeAngle()*fabs(pcam.z);
			AColor pc = SampleProjMap(sc,p,Point3(d,d,d),x,y, gl->projMap);
			return atten*colStep*Color(pc.r,pc.g,pc.b);
			}


		if (rect && !(overshoot)) {
			float u = 0.0f;
			float ux = (float)fabs(mx)/sz2;
			float uy = (float)fabs(my)/sz2;
			if (ux>1.0f || uy>1.0f) {
				atten = 0.0f;
				goto skipAtten;
				}
			int inflag = 0;
			if(ux>hotpct) {
				inflag = 1;	
				ux=(ux-hotpct)/ihotpct;		
				}
			if(uy>hotpct) {	
				inflag |= 2;
				uy=(uy-hotpct)/ihotpct;
				}
			switch(inflag) {
				case 1:	u = 1.0f-ux;			break;
				case 2:	u = 1.0f-uy;			break;
				case 3:	u = (1.0f-ux)*(1.0f-uy); break;
				case 0:	goto skipAtten;
				}
			atten *= u*u*(3.0f-2.0f*u);  /* smooth cubic curve */
			}
		
		}
	

skipAtten:
	return colStep * atten;
	}

#define MAX(a,b) (((a)>(b))?(a):(b))

int SpotLight::Update(TimeValue t, const RendContext &rc,
		RenderGlobalContext *rgc, BOOL shadows, BOOL shadowGeomChanged)
{
	int res = 1;
	BaseObjLight::Update(t,rc,rgc,shadows, shadowGeomChanged);

	float hs = DegToRad(ls.hotsize);
	float fs = DegToRad(ls.fallsize);
	fall_tan = (float)tan(fs/2.0f);
	hot_cos = (float)cos(hs/2.0f);
	fall_cos =(float)cos(fs/2.0f);
	fall_sin = (float)sin(fs/2.0f);
	hotpct = ls.hotsize/ls.fallsize;
	ihotpct = 1.0f - hotpct;		

	ObjectState os = inode->EvalWorldState(t);
	LightObject* lob = (LightObject *)os.obj;		
	assert(os.obj->SuperClassID()==LIGHT_CLASS_ID);
	GeneralLight* gl = (lob->GetInterface(I_MAXSCRIPTPLUGIN) != NULL) ? (GeneralLight*)lob->GetReference(0) : (GeneralLight*)lob;  // JBW 4/7/99

	//bank = ls.bank;
	rect = gl->GetSpotShape()==RECT_LIGHT?1:0;
	overshoot = gl->GetOvershoot();
	shadow = gl->GetShadow()&shadows;	
	if (ambOnly) shadow = FALSE;
	shadowRay = gl->GetShadowType();
	projector =  gl->GetProjector();
	fov = MAX(fs,hs);

	float aspect = rect?gl->GetAspect(t):1.0f;
//	shadsize = gl->GetMapSize(t);
	shadsize = ActiveShadowType()->MapSize(t);
	if (shadsize<=0) shadsize = 10;
	sz2 = .5f*(float)shadsize;
	 
	fov = 2.0f* (float)atan(tan(fov*0.5f)*sqrt(aspect));
	zfac = -sz2 /(float)tan(0.5*(double)fov);
	xscale = zfac;								
	yscale = -zfac*aspect;
	curve =(float)fabs(1.0f/xscale); 

	//rectv0.y = float(tan(fs*0.5) * sqrt(aspect));
	//rectv1.y = rectv0.y/aspect;
	rectv0.y = fall_sin * (float)sqrt(aspect);
	rectv1.y = fall_sin / (float)sqrt(aspect);

	rectv0.x = rectv1.x = fall_cos;
	rectv0 = Normalize(rectv0);
	rectv1 = Normalize(rectv1);

//	ULONG flags = gl->GetAbsMapBias()?SHAD_BIAS_ABSOLUTE:0;
	Interval v;
	if (projector){
//		projMap = gl->GetProjMap();
		if( gl->projMap ) gl->projMap->Update(t,v);
	} // else projMap = NULL;

	if (shadow)
	{	
		if (dontNeedShadBuf)
		{
			FreeShadGenBuffers();
		}
		else 
		{
			float clipDist = ls.useAtten? ls.attenEnd : DONT_CLIP;
			if (shadGen && shadowGeomChanged) // mjm - 06.08.00 - added check for non-NULL shadGen
				res = shadGen->Update(t,rc,rgc,lightToWorld,aspect,fov,clipDist);
		}
	}
	shadow = shadow&&shadGen;	
	if (dontNeedShadBuf) shadow = FALSE;
	return res;
}

int  SpotLight::UpdateViewDepParams(const Matrix3& worldToCam) {
	BaseObjLight::UpdateViewDepParams(worldToCam);
	// get light direction in cam space
	lightDir = -FNormalize(lightToCam.GetRow(2));
	if (shadGen)
		shadGen->UpdateViewDepParams(worldToCam);
	return 1;
	}

static Color gray(.5f,.5f,.5f);

// Componentwise Illumination function
BOOL SpotLight::Illuminate(ShadeContext &sc, Point3& N, IlluminateComponents& illumComp )
{
	if( ls.intens == 0.0f )
		return FALSE;

	/* SPOTlight : compute light value */
	float atten,angle,conelimit,dist;
	Point3 p = sc.P();
	Point3 L = lightPos-p;  // vector from light to p in camera space
	Point3 plt,LL;

	if (!ambOnly) {
		// quick test for light behind surface
		if (DotProd(N,L)<=0.0f)	
			return 0;
	}
	// prepare point & dist in light space
	plt = camToLight * p;
	dist = FLength(plt);
	if (dist==0.0f) return 0;
	LL = -plt/dist;  // normalized lightspace L

	// get the distance atten
	atten = illumComp.distanceAtten = ApplyDistanceAtten( dist );

	conelimit = rect ? (fall_cos/COS_45_2X) : fall_cos;
	Color color = intensCol;


	// > 11/11/02 - 6:19pm --MQM-- do same thing as omni for "rawColor"
	Color c2 = intensCol;
	if ( gl->projMap && projector )
	{
		float x =  xscale * plt.x / plt.z + sz2;
		float y =  yscale * plt.y / plt.z + sz2;
		c2 *= SampleProjMap( sc, plt, sc.DP(), x, y, gl->projMap );
	}
	illumComp.rawColor = c2;


	L = FNormalize(L);

	if (ambOnly) {
		illumComp.NL = illumComp.geometricAtten = 1.0f;
	} else {
		if (uniformScale) {
			illumComp.NL = DotProd(N, L);	
		} else {
			Point3	Nl = FNormalize( VectorTransform( camToLight, N) );
			// Is light is in front of surface? 
			if ((illumComp.NL= DotProd(Nl, LL) )<=0.0f) 
				return 0;
		}
		illumComp.geometricAtten = ContrastFunc(illumComp.NL);
	}

	float shadAtten = 1.0f;
	illumComp.shapeAtten = 1.0f;

	// Is point in light cone? 
	if ((angle = LL.z) < conelimit) {
		if (!overshoot)	
			return 0; 
		goto overshoot_bypass;
	}

	// is there a shadow or projector map?
	if (shadow||rect||(gl->projMap&&projector)) {
		atten *= SampleMaps(sc, p, N, plt, color, shadAtten, &illumComp );
	}

	// handle the cone falloff
	if((!rect)&&(!overshoot) && (angle < hot_cos)) {
		float u = (angle-fall_cos)/(hot_cos-fall_cos);
		illumComp.shapeAtten = u*u*(3.0f-2.0f*u);  // smooth cubic curve 
		atten *= illumComp.shapeAtten;
	}

overshoot_bypass:
	if (atten==0.0f) 
		return 0;

	illumComp.L = L;  // direction in camera space

	// ATMOSPHERIC SHADOWS
	if (gl->GetShadow() && atmosShadows)
		ApplyAtmosShadows(sc, lightPos, color, shadAtten);

	// > 11/11/02 - 6:06pm --MQM-- take this out...mimic omni (look about 30 lines above too)
	illumComp.filteredColor = color;
//	illumComp.rawColor = illumComp.filteredColor = color; // never any filtering for shadow maps

	if( shadAtten != 1.0f) {
		illumComp.shadowColor = shadColor;
		illumComp.shadowAtten = shadAtten;
		if (shadProjMap){ 
			float x =  xscale * plt.x / plt.z + sz2;
			float y =  yscale * plt.y / plt.z + sz2;
			illumComp.shadowColor = SampleProjMap(sc, plt, sc.DP(), x, y, shadProjMap);
		}
		ApplyShadowColor( illumComp, shadAtten);
	
	} else {
		illumComp.shadowColor.Black();
		illumComp.shadowAtten = 1.0f;
		illumComp.lightDiffuseColor = illumComp.finalColor = illumComp.finalColorNS = color;
	}

	if (atten != 1.0f){
		illumComp.finalColor *= atten;
		illumComp.finalColorNS *= atten;
		illumComp.lightDiffuseColor *= atten;
		illumComp.shadowColor *= atten;
	}

	ApplySpecularDiffuseAtten( illumComp.lightDiffuseColor, illumComp );
	return 1;
}

BOOL SpotLight::Illuminate(ShadeContext &sc, Point3& normal, Color& color, 
		Point3 &dir, float& dot_nl, float &diffuseCoef)
{

	if( ls.intens == 0.0f )
		return FALSE;

	/* SPOTlight : compute light value */
	float atten,angle,conelimit,dist;
	Point3 p = sc.P();
	Point3 L = lightPos-p;  // vector from light to p in camera space
	Point3 plt,LL;

	if (!ambOnly) {
		// quick test for light behind surface
		if (DotProd(normal,L)<=0.0f)	
			return 0;
		}
	plt = camToLight * p;
	dist = FLength(plt);
	if (dist==0.0f) return 0;
	LL = -plt/dist;  
	atten = 1.0f;

	// Handle light distance attenuation 
	if(ls.useNearAtten) {
	   	if(dist<ls.nearAttenStart)		// Beyond range 
	   		return 0;
		if(dist<ls.nearAttenEnd)	{
			// in attenuated range
			float u = (dist - ls.nearAttenStart)/(ls.nearAttenEnd-ls.nearAttenStart);
			atten = u*u*(3-2*u);  /* smooth cubic curve */
			}
		}

	if(ls.useAtten) {
	   	if(dist>ls.attenEnd)		// Beyond range 
	   		return 0;
		if(dist>ls.attenStart)	{
			/* Outside full-intensity range */
			float u = (ls.attenEnd-dist)/(ls.attenEnd-ls.attenStart);
			atten *= u*u*(3-2*u);  // smooth cubic curve 
			}
		}

//	atten = Decay(atten,dist,ls.nearAttenEnd);
	atten = Decay(atten, dist, decayRadius);

	conelimit = rect ? (fall_cos/COS_45_2X) : fall_cos;
	color = intensCol;

	L = FNormalize(L);

	if (ambOnly) {
		dot_nl = diffuseCoef = 1.0f;
		}
	else {
		if (uniformScale) {
			dot_nl = DotProd(normal,L);	
			}
		else {
			Point3	N = FNormalize(VectorTransform(camToLight,normal));
			// Is light is in front of surface? 
			if ((dot_nl= DotProd(N,LL))<=0.0f) 
				return 0;
			}
		diffuseCoef = ContrastFunc(dot_nl);
		}
	float shadAtten = 1.0f;

	// Is point in light cone? 
	if ((angle = LL.z) < conelimit) {
		if (!overshoot)	
			return 0; 
		goto overshoot_bypass;
		}

	if (shadow||rect||(gl->projMap&&projector)) {
		atten *= SampleMaps(sc,p,normal,plt,color,shadAtten);
		}
	if((!rect)&&(!overshoot) && (angle < hot_cos)) {
		float u = (angle-fall_cos)/(hot_cos-fall_cos);
		atten *= u*u*(3.0f-2.0f*u);  // smooth cubic curve 
		}								
	overshoot_bypass:
	if (atten==0.0f) 
		return 0;
	dir = L;  // direction in camera space

	// ATMOSPHERIC SHADOWS
	if (gl->GetShadow() && atmosShadows)
		ApplyAtmosShadows(sc,lightPos,color, shadAtten);

	if (shadAtten!=1.0f) {
		Color scol = shadColor;
		if (shadProjMap) {
			float x =  xscale*plt.x/plt.z + sz2;
			float y =  yscale*plt.y/plt.z + sz2;
			scol = SampleProjMap(sc,plt,sc.DP(),x,y,shadProjMap);
			}
		ApplyShadowColor(color, shadAtten,scol);
		}
	if (atten!=1.0f) color *= atten;
	return 1;
}


AColor SpotLight::SampleProjMap(ShadeContext &sc,  Point3 plt, Point3 dp, 
		float x, float y, Texmap *map) {
	SCLight scspot;
	float ifs = 1.0f/(float)shadsize;
	scspot.origsc = &sc;
	scspot.projType = 0; // perspective
	scspot.curtime = sc.CurTime();
	scspot.curve =(float)fabs(1.0f/xscale); 
	scspot.ltPos = plt;
	scspot.view = FNormalize(plt);
	scspot.uv.x = x*ifs;			
	scspot.uv.y = 1.0f-y*ifs;			

	scspot.scrpos.x = (int)(x+0.5);
	scspot.scrpos.y = (int)(y+0.5);
	scspot.filterMaps = sc.filterMaps;
	scspot.mtlNum  = sc.mtlNum;

	// antialiasing for 2D textures:
	float d = MaxAbs(dp.x,dp.y,dp.z);

	float duv = 0.0f;
	if (plt.z!=0.0f)
		duv = fabs(xscale*d*ifs/plt.z);
	if (duv>.9f) duv = .9f;
	scspot.duv.x = scspot.duv.y = duv;

	// clip 
	float hd = duv*0.5f;
	if (scspot.uv.x + hd >0.999999) scspot.uv.x = .9999999-hd;
	if (scspot.uv.x - hd <0.0f) scspot.uv.x = hd;
	if (scspot.uv.y + hd >0.999999) scspot.uv.y = .9999999-hd;
	if (scspot.uv.y - hd <0.0f) scspot.uv.y = hd;

	// antialiasing for 3D textures: 
	scspot.dp = Point3(d,d,d);
	
	return map->EvalColor(scspot);
	}

float SpotLight::SampleMaps(ShadeContext& sc, Point3 pin, 
		Point3 norm, Point3 plt, Color& lcol, float &shadAtten, IlluminateComponents* pIllumComp) {
	float mx,my,x,y,atten;
	Point3 pout,n;
	Color col;
	atten = 1.0f;
	if (plt.z<0.0f) {
		mx =  xscale*plt.x/plt.z;
		my =  yscale*plt.y/plt.z;
		x = sz2 + mx;
		y = sz2 + my;

		/* If it falls outside the rectangle, bail out */
		if ((float)fabs(mx)>sz2 || (float)fabs(my)>sz2)	{
			if(pIllumComp){
				pIllumComp->rawColor = pIllumComp->filteredColor = lcol;
				pIllumComp->shapeAtten = (overshoot)? 1.0f : 0.0f;
			}
			if(overshoot) 
				return(1.0f);	/* No attenuation */
			else 
				return(0.0f);
		}

		if(gl->projMap&&projector) 
			lcol *= SampleProjMap(sc,plt,sc.DP(),x,y,gl->projMap);

		if(pIllumComp) pIllumComp->rawColor = lcol;

		if (sc.shadow&&shadow) {
			if (isDefaultShadowMap) {
				Point3 n = VectorTransform(camToLight,norm);
				float k = plt.z*plt.z/(zfac*DotProd(n,plt));
				shadAtten = shadGen->Sample(sc, x,y, plt.z, -n.x*k, n.y*k);
				if (shadAtten==0.0f) return atten;
			} else {
				shadAtten = shadGen->Sample(sc,norm,lcol);
				if (shadAtten==0.0f) return atten;
			}
		}
		if(pIllumComp) pIllumComp->filteredColor = lcol;
		
		// Calculate rectangular dropoff if outside the hotspot area 
		if (rect && !(overshoot)) {
			float u = 0.0f;
			float ux = (float)fabs(mx)/sz2;
			float uy = (float)fabs(my)/sz2;
			int inflag = 0;
			if(ux>hotpct) {
				inflag = 1;	
				ux=(ux-hotpct)/ihotpct;		
				}
			if(uy>hotpct) {	
				inflag |= 2;
				uy=(uy-hotpct)/ihotpct;
				}
			switch(inflag) {
				case 1:	u = 1.0f-ux;			break;
				case 2:	u = 1.0f-uy;			break;
				case 3:	u = (1.0f-ux)*(1.0f-uy); break;
				case 0:	goto noAtten;
				}
				float a = u*u*(3.0f-2.0f*u);  /* smooth cubic curve */
				if(pIllumComp) pIllumComp->shapeAtten = a;
				atten *= a;  /* smooth cubic curve */
			}
		}
noAtten:		  
	return(atten);
	}


ObjLightDesc *GeneralLight::CreateLightDesc(INode *inode, BOOL forceShadowBuf )
	{
	switch (type) {
		case OMNI_LIGHT: 	return new OmniLight(inode, forceShadowBuf);
		case DIR_LIGHT:
		case TDIR_LIGHT:   	return new DirLight(inode, forceShadowBuf);
		case FSPOT_LIGHT:	
		case TSPOT_LIGHT:	return new SpotLight(inode, forceShadowBuf);
		default:			return NULL;
		}
	}

#define PLUG_SHADOW_TYPE_CHUNK 5000

//---------------------------------------------------------------------------
// Class IO: The spotlight does the saving and loading of the shadow parameters
// for all the light classes.
IOResult TSpotLightClassDesc::Save(ISave *isave){
//	ULONG nb;
	isave->BeginChunk(PLUG_SHADOW_TYPE_CHUNK);
	isave->EndChunk();
//	isave->BeginChunk(SHADOW_TYPE_CHUNK);
//	isave->Write(&GeneralLight::globShadowType, sizeof(GeneralLight::globShadowType), &nb);
//	isave->EndChunk();
//	isave->BeginChunk(ABS_MAP_BIAS_CHUNK);
//	isave->Write(&GeneralLight::globAbsMapBias, sizeof(GeneralLight::globAbsMapBias), &nb);
//	isave->EndChunk();
//	isave->BeginChunk(MAP_BIAS_CHUNK);
//	isave->Write(&GeneralLight::globMapBias, sizeof(GeneralLight::globMapBias), &nb);
//	isave->EndChunk();
//	isave->BeginChunk(MAP_RANGE_CHUNK);
//	isave->Write(&GeneralLight::globMapRange, sizeof(GeneralLight::globMapRange), &nb);
//	isave->EndChunk();
//	isave->BeginChunk(MAP_SIZE_CHUNK);
//	isave->Write(&GeneralLight::globMapSize, sizeof(GeneralLight::globMapSize), &nb);
//	isave->EndChunk();
//	isave->BeginChunk(RAY_BIAS_CHUNK);
//	isave->Write(&GeneralLight::globRayBias, sizeof(GeneralLight::globRayBias), &nb);
//	isave->EndChunk();
	return IO_OK;
	}

IOResult TSpotLightClassDesc::Load(ILoad *iload){
	ULONG nb;
	IOResult res;
	int hasPlugShadows = 0;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
			case PLUG_SHADOW_TYPE_CHUNK:
				hasPlugShadows = TRUE;
				break;
			case SHADOW_TYPE_CHUNK:
				res = iload->Read(&GeneralLight::globShadowType, sizeof(GeneralLight::globShadowType), &nb);
				break;
			case ABS_MAP_BIAS_CHUNK:
				res = iload->Read(&GeneralLight::globAbsMapBias, sizeof(GeneralLight::globAbsMapBias), &nb);
				break;
			case MAP_BIAS_CHUNK:
				res = iload->Read(&GeneralLight::globMapBias, sizeof(GeneralLight::globMapBias), &nb);
				break;
			case MAP_RANGE_CHUNK:
				res = iload->Read(&GeneralLight::globMapRange, sizeof(GeneralLight::globMapRange), &nb);
				break;
			case MAP_SIZE_CHUNK:
				res = iload->Read(&GeneralLight::globMapSize, sizeof(GeneralLight::globMapSize), &nb);
				break;
			case RAY_BIAS_CHUNK:
				res = iload->Read(&GeneralLight::globRayBias, sizeof(GeneralLight::globRayBias), &nb);
				break;

			}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}

	if (!hasPlugShadows) {
		// Convert global parameters to the new style
		ShadowType *st = (GeneralLight::globShadowType)?
			NewDefaultRayShadowType():NewDefaultShadowMapType();
		st->SetMapRange(TimeValue(0),GeneralLight::globMapRange);
		st->SetMapSize(TimeValue(0),GeneralLight::globMapSize);
		st->SetMapBias(TimeValue(0),GeneralLight::globMapBias);
		st->SetAbsMapBias(TimeValue(0),GeneralLight::globAbsMapBias);
		st->SetRayBias(TimeValue(0),GeneralLight::globRayBias);
		GetCOREInterface()->SetGlobalShadowGenerator(st);
		}
	return IO_OK;
	}

/**
 *  Initializes the controls in the General Parameters dialog rollout.
 */

void GeneralLight::InitGeneralParamDialog( TimeValue t, IObjParam *ip, ULONG flags,Animatable *prev )
{

	hGeneralLight = ip->AddRollupPage( 
				hInstance, 
				/*type == DIR_LIGHT ? MAKEINTRESOURCE(IDD_DIRLIGHTPARAM) :*/ 
				// note:  there are two different dialogs depending on whether the user
				//			 is in create mode or not
				MAKEINTRESOURCE(inCreate ? IDD_LIGHT_PARAM2_CREATE : IDD_LIGHT_PARAM2_MOD ),
				GeneralLightParamDialogProc, 
				GetString(IDS_DB_GENERAL_PARAMS), 
				(LPARAM)this);		
	ip->RegisterDlgWnd(hGeneralLight);

	

	if (type == FSPOT_LIGHT||type==DIR_LIGHT) {
			tDistSpin = GetISpinner( GetDlgItem(hGeneralLight, IDC_LTDISTSPINNER) );
			tDistSpin->SetLimits(1.0f, 10000.0f, FALSE);
			tDistSpin->SetValue(GetTDist(t),FALSE);
			tDistSpin->SetScale(1.0f);
			tDistSpin->LinkToEdit( GetDlgItem(hGeneralLight, IDC_LTDIST), EDITTYPE_UNIVERSE );
	}
	ShowWindow(GetDlgItem(hGeneralLight, IDC_TARG_DISTANCE), type == OMNI_LIGHT ? SW_HIDE : SW_SHOW);
	EnableWindow(GetDlgItem(hGeneralLight, IDC_LIGHT_TARGETED),
		type != OMNI_LIGHT && !IsDaylightOrSunlightSystem());
	ShowWindow(GetDlgItem(hGeneralLight, IDC_LTDIST), type == FSPOT_LIGHT||type==DIR_LIGHT ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hGeneralLight, IDC_LTDISTSPINNER), type == FSPOT_LIGHT||type==DIR_LIGHT ? SW_SHOW : SW_HIDE);
	
	
	UpdtShadowTypeList(hGeneralLight);

	// these controls do not exist anymore in the new UI - DC

//		redSpin   =	GetISpinner( GetDlgItem(hGeneralLight, IDC_LREDSPINNER) );
//		greenSpin = GetISpinner( GetDlgItem(hGeneralLight, IDC_LGREENSPINNER) );
//		blueSpin  = GetISpinner( GetDlgItem(hGeneralLight, IDC_LBLUESPINNER) );
//		hSpin 	  = GetISpinner( GetDlgItem(hGeneralLight, IDC_LHSPINNER) );
//		sSpin 	  = GetISpinner( GetDlgItem(hGeneralLight, IDC_LSSPINNER) );
//		vSpin 	  = GetISpinner( GetDlgItem(hGeneralLight, IDC_LVSPINNER) );
//		color = GetRGBColor(t);
//		InitColorSpinner(hGeneralLight, redSpin, IDC_LRED, FLto255f(color.x));
//		InitColorSpinner(hGeneralLight, greenSpin, IDC_LGREEN, FLto255f(color.y));
//		InitColorSpinner(hGeneralLight, blueSpin, IDC_LBLUE, FLto255f(color.z));

		//colorSwatch->SetModal();
//		color = GetHSVColor(t);
//		InitColorSpinner(hGeneralLight, hSpin, IDC_LH, FLto255f(color.x));
//		InitColorSpinner(hGeneralLight, sSpin, IDC_LS, FLto255f(color.y));
//		InitColorSpinner(hGeneralLight, vSpin, IDC_LV, FLto255f(color.z));


}

/**
 *  Initializes the controls in the Intensity/Colour/Attenuation Parameters dialog rollout.
 */


void GeneralLight::InitICAParamDialog( TimeValue t,  IObjParam *ip, ULONG flags,Animatable *prev )
{
	hICAParam = ip->AddRollupPage(
				hInstance,
				MAKEINTRESOURCE(IDD_ICA_PARAM),
				ICAParamDialogProc,
				GetString(IDS_DB_ICA_PARAMS),
				(LPARAM)this);
	ip->RegisterDlgWnd(hICAParam);
	Point3 color;
	color = GetRGBColor(t);
	colorSwatch = GetIColorSwatch(GetDlgItem(hICAParam, IDC_LIGHT_COLOR), 
			RGB(FLto255i(color.x), FLto255i(color.y), FLto255i(color.z)), 
			GetString(IDS_DS_LIGHTCOL));
	colorSwatch->ForceDitherMode(1);

	attenStartSpin = GetISpinner( GetDlgItem(hICAParam,IDC_START_RANGE_SPIN) );
	attenEndSpin = GetISpinner( GetDlgItem(hICAParam,IDC_END_RANGE_SPIN) );
	InitRangeSpinner(hICAParam, attenStartSpin, IDC_START_RANGE, GetAtten(t, ATTEN_START), IsDir());
	InitRangeSpinner(hICAParam, attenEndSpin, IDC_END_RANGE, GetAtten(t, ATTEN_END), IsDir());
	int temp = IsDlgButtonChecked(hICAParam, IDC_SHOW_RANGES);
	if (GetAttenDisplay() != temp)
		SetAttenDisplay( temp );
	temp = IsDlgButtonChecked(hICAParam, IDC_USE_ATTEN);
	if (GetUseAtten() != temp)
		SetUseAtten( temp );

	attenStart1Spin = GetISpinner( GetDlgItem(hICAParam,IDC_START_RANGE1_SPIN) );
	attenEnd1Spin = GetISpinner( GetDlgItem(hICAParam,IDC_END_RANGE1_SPIN) );
	InitRangeSpinner(hICAParam, attenStart1Spin, IDC_START_RANGE1, GetAtten(t, ATTEN1_START),IsDir());
	InitRangeSpinner(hICAParam, attenEnd1Spin, IDC_END_RANGE1, GetAtten(t, ATTEN1_END),IsDir());
	temp = IsDlgButtonChecked(hICAParam, IDC_SHOW_RANGES1);
	if (GetAttenNearDisplay() != temp)
		SetAttenNearDisplay( temp );
	temp = IsDlgButtonChecked(hICAParam, IDC_USE_ATTEN1);
	if (GetUseAttenNear() != temp)
		SetUseAttenNear( temp );

	decaySpin = GetISpinner( GetDlgItem(hICAParam,IDC_DECAY_SPIN) );
	InitRangeSpinner( hICAParam, decaySpin, IDC_DECAY_EDIT, GetDecayRadius(t), IsDir());

	intensitySpin = GetISpinner( GetDlgItem(hICAParam, IDC_LMULTSPINNER) );
	intensitySpin->SetLimits(-(float)1.0e30, (float)1.0e30, FALSE);
	intensitySpin->SetValue(GetIntensity(t),FALSE);
	intensitySpin->SetScale(0.01f);
	intensitySpin->SetAutoScale(TRUE);
	intensitySpin->LinkToEdit( GetDlgItem(hICAParam, IDC_LMULT), EDITTYPE_FLOAT );

}

/**
 *  Initializes the controls in the Advanced Effects Parameters dialog rollout.
 */


void GeneralLight::InitAdvEffectsDialog(TimeValue t, IObjParam *ip, ULONG flags,Animatable *prev )
{
	hAdvEff = ip->AddRollupPage(
				hInstance,
				MAKEINTRESOURCE(IDD_ADV_EFFECTS),
				AdvEffDialogProc,
				GetString(IDS_ADV_EFFECTS),
				(LPARAM)this);

	// CHANGE HWND BELOW!!!
	contrastSpin = SetupFloatSpinner(hAdvEff, IDC_LCONTRASTSPIN2, IDC_LCONTRAST2,0.0f,100.0f,
		0.0f,1.0f);
	diffsoftSpin = SetupFloatSpinner(hAdvEff, IDC_DIFFSOFTSPIN, IDC_DIFFSOFT,0.0f,100.0f,
		0.0f,1.0f);

	contrastSpin->SetValue(GetContrast(t), FALSE);
	diffsoftSpin->SetValue(GetDiffuseSoft(t), FALSE);

	projMapName = GetICustButton(GetDlgItem(hAdvEff, IDC_PROJ_MAPNAME));	
	projMapName->SetText(projMap?projMap->GetFullName().data():GetString(IDS_DB_NONE));
	projMapName->SetDADMgr(&projDADMgr);


}

void GeneralLight::InitSpotParamDialog(TimeValue t,  IObjParam *ip, ULONG flags,Animatable *prev )	{
	hSpotLight = ip->AddRollupPage( 
					hInstance, 
					MAKEINTRESOURCE(IDD_SPOTLIGHT2),
					GeneralLightParamDialogProc, 
					IsDir() ? GetString(IDS_DB_DIR_PARAMS) : GetString(IDS_DB_SPOT_PARAMS), 
					(LPARAM)this
					);		
	ip->RegisterDlgWnd(hSpotLight);				

	aspectSpin = GetISpinner( GetDlgItem(hSpotLight, IDC_LASPECT_SPIN) );
	aspectSpin->SetLimits(0.001f, 100.0f, FALSE);
	aspectSpin->SetValue(GetAspect(t),FALSE);
	aspectSpin->SetScale(0.01f);
	aspectSpin->LinkToEdit( GetDlgItem(hSpotLight, IDC_LASPECT), EDITTYPE_FLOAT );

	
	int temp = IsDlgButtonChecked(hSpotLight, IDC_SHOW_CONE);
	if (GetConeDisplay() != temp)
		SetConeDisplay( temp );

	hotsizeSpin = GetISpinner( GetDlgItem(hSpotLight,IDC_LHOTSIZESPINNER) );
	fallsizeSpin = GetISpinner( GetDlgItem(hSpotLight,IDC_LFALLOFFSPINNER) );

	if(IsDir()) {
		hotsizeSpin->SetLimits(.001f, (float)1.0e30, FALSE);
		hotsizeSpin->SetValue(GetHotspot(t),FALSE);
		hotsizeSpin->SetScale(1.0f);
		hotsizeSpin->LinkToEdit( GetDlgItem(hSpotLight, IDC_LHOTSIZE), EDITTYPE_UNIVERSE );

		fallsizeSpin->SetLimits(.001f, (float)1.0e30, FALSE);
		fallsizeSpin->SetValue(GetFallsize(t),FALSE);
		fallsizeSpin->SetScale(1.0f);
		fallsizeSpin->LinkToEdit( GetDlgItem(hSpotLight, IDC_LFALLOFF), EDITTYPE_UNIVERSE );
	}
	else {
		InitAngleSpinner(hSpotLight, hotsizeSpin, IDC_LHOTSIZE, GetHotspot(t) );
		InitAngleSpinner(hSpotLight, fallsizeSpin, IDC_LFALLOFF, GetFallsize(t) );
	}
}			

void GeneralLight::UpdateControlledUI()
{
	//aszabo|Nov.09.01 - mimic the functionality in the parammaps
	//This will disable controls that are driven by scripted, wire, expression controllers

	// should exit from function if pblock does not exist.
	// leaving the DbgAssert as a warning
	// bug 313434
   DbgAssert(pblock != NULL);
	if(pblock == NULL)	
		return;
	Control* c;
	
	// Multiplier
	if(intensitySpin != NULL)
	{
		c = pblock->GetController(PB_INTENSITY);
		if (c != NULL)
			intensitySpin->Enable(c->IsKeyable());
		else
			intensitySpin->Enable(TRUE);
	}
	if(colorSwatch != NULL)
	{
		c = pblock->GetController(PB_COLOR);
		if (c != NULL)
			colorSwatch->Enable(c->IsKeyable());
	}
	
	// Decay
	if(decaySpin != NULL)
	{
		c = pblock->GetController(type==OMNI_LIGHT? PB_OMNIDECAY: PB_DECAY);
		if (c != NULL)
			decaySpin->Enable(c->IsKeyable());
	}
	
	// Attenuation
	if(attenStartSpin != NULL)
	{
		c = pblock->GetController((type == OMNI_LIGHT ? PB_OMNIATSTART : PB_ATTENSTART));
		if (c != NULL)
			attenStartSpin->Enable(c->IsKeyable());
	}
	if(attenEndSpin != NULL)
	{
		c = pblock->GetController((type == OMNI_LIGHT ? PB_OMNIATEND : PB_ATTENEND));
		if (c != NULL)
			attenEndSpin->Enable(c->IsKeyable());
	}
	if(attenStart1Spin != NULL)
	{
		c = pblock->GetController((type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1));
		if (c != NULL)
			attenStart1Spin->Enable(c->IsKeyable());
	}
	if(attenEnd1Spin != NULL)
	{
		c = pblock->GetController((type == OMNI_LIGHT ? PB_OMNIATEND1 : PB_ATTENEND1));
		if (c != NULL)
			attenEnd1Spin->Enable(c->IsKeyable());
	}
	
	// Light Cone params
	if(hotsizeSpin != NULL)
	{
		c = pblock->GetController(PB_HOTSIZE);
		if (c != NULL)
			hotsizeSpin->Enable(c->IsKeyable());
	}
	if(fallsizeSpin != NULL)
	{
		c = pblock->GetController(PB_FALLSIZE);
		if (c != NULL)
			fallsizeSpin->Enable(c->IsKeyable());
	}
	if (ambientOnly == 0)
	{
		if(contrastSpin != NULL)
		{
			c = pblock->GetController(PB_CONTRAST);
			if (c != NULL)
				contrastSpin->Enable(c->IsKeyable());
		}
		if(diffsoftSpin != NULL)
		{
			c = pblock->GetController(PB_DIFFSOFT);
			if (c != NULL)
				diffsoftSpin->Enable(c->IsKeyable());
		}
	}
	if (GetSpotShape() == RECT_LIGHT)
	{
		if(aspectSpin != NULL)
		{
			c = pblock->GetController(PB_ASPECT);
			if (c != NULL)
				aspectSpin->Enable(c->IsKeyable());
		}
	}	

	//Target Distance
	if (type == FSPOT_LIGHT || type == DIR_LIGHT) 
	{
		if(tDistSpin != NULL)
		{
			c = pblock->GetController(PB_TDIST);
			if (c != NULL)
				tDistSpin->Enable(c->IsKeyable());
		}
	}
	// Shadow
	if(shadColorSwatch != NULL)
	{
		c = pblock->GetController(type == OMNI_LIGHT ?PB_OMNISHADCOLOR : PB_SHADCOLOR);
		if (c != NULL)
			shadColorSwatch->Enable(c->IsKeyable());
	}
	if(shadMultSpin != NULL)
	{
		c = pblock->GetController(PB_SHAD_MULT(this));
		if (c != NULL)
			shadMultSpin->Enable(c->IsKeyable());
	}
	// Atmosphere
	if(atmosOpacitySpin != NULL)
	{
		c = pblock->GetController(PB_ATM_OPAC(this));
		if (c != NULL)
			atmosOpacitySpin->Enable(c->IsKeyable());
	}
	if(atmosColAmtSpin != NULL)
	{
		c = pblock->GetController(PB_ATM_COLAMT(this));
		if (c != NULL)
			atmosColAmtSpin->Enable(c->IsKeyable());
	}

}
