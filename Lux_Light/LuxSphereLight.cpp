//**************************************************************************/
// Copyright (c) 2005-2020 LuxCoreRenderer, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: MaxToLux is a part off LuxCoreRenderer
// AUTHOR: Omid GHOTBI (TAO)
//***************************************************************************/

#include "LuxObjects.h"
#include "decomp.h"
#include <Graphics/CustomRenderItemHandle.h>

#define LUX_LIGHT_SPHERE_CLASS_ID	Class_ID(0xc9b7342, 0x7dbc3e6d)

#define PBLOCK_REF	0

#define NUM_HALF_ARC	5
#define NUM_ARC_PTS	    (2*NUM_HALF_ARC+1)
#define SEG_INDEX		7
#define PB_OMNIDECAY   	8
#define PB_DECAY		11
#define NUM_CIRC_PTS	28

class LuxLightSphere : public LightObject {
	friend class 	GeneralLightCreateCallBack;

	short 	type;
	short 	useLight;					// on/off toggle
	short 	coneDisplay;
	int 	extDispFlags;
	static HWND 	hGeneralLight;
	static IColorSwatch 	*colorSwatch;
	static	LuxLightSphere *currentEditLight;
	static short 	meshBuilt;
	static Mesh 	staticMesh;
	Mesh 	*mesh;

	RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs);
	void 	UpdateUI(TimeValue t);
	void 	BuildStaticMeshes();
	void	GetMat(TimeValue t, INode* inode, ViewExp &vpt, Matrix3& mat);

	public:
		short 	enable;

		// Parameter block
		IParamBlock2	*pblock;	//ref 0
		//ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp);
		virtual BOOL SetDlgThing(ParamDlg* dlg);

		// Loading/Saving
		IOResult Load(ILoad *iload);
		IOResult Save(ISave *isave);

		//From Animatable
		Class_ID ClassID() {return LUX_LIGHT_SPHERE_CLASS_ID;}
		SClass_ID SuperClassID() { return LIGHT_CLASS_ID; }
		void GetClassName(TSTR& s, bool localized) {s = GetString(IDS_CLASS_NAME);}

		virtual void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
		virtual void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

		RefTargetHandle Clone( RemapDir &remap );
		RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
			PartID& partID, RefMessage message, BOOL propagate);

		int NumSubs() { return 1; }

		virtual int NumRefs() { return 1; }
		virtual RefTargetHandle GetReference(int i);

		int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
		IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

		void DeleteThis() { delete this; }		
		//Constructor/Destructor

		LuxLightSphere();// {}
		~LuxLightSphere();// {}

		int 	HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);

		ObjectState 	Eval(TimeValue time);
		void 			InitNodeName(TSTR& s);
		const TCHAR * GetObjectName(bool localized);
		CreateMouseCallBack * GetCreateMouseCallBack();

		void 	SetUseLight(int onOff);
		BOOL 	GetUseLight(void) { return useLight; }
		void 	SetHotspot(TimeValue time, float f);
		float 	GetHotspot(TimeValue t, Interval& valid = Interval(0, 0));
		void 	SetFallsize(TimeValue time, float f);
		float 	GetFallsize(TimeValue t, Interval& valid = Interval(0, 0));
		void 	SetAtten(TimeValue time, int which, float f);
		float 	GetAtten(TimeValue t, int which, Interval& valid = Interval(0, 0));
		void 	SetTDist(TimeValue time, float f);
		float 	GetTDist(TimeValue t, Interval& valid = Interval(0, 0));
		void 	SetConeDisplay(int s, int notify = TRUE);
		BOOL 	GetConeDisplay(void) { return coneDisplay; }
		void 	BuildMeshes(BOOL isnew = TRUE);
		virtual bool UpdatePerNodeItems(
			const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
			MaxSDK::Graphics::UpdateNodeContext& nodeContext,
			MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer);
		void 	SetExtendedDisplay(int flags);
		void 	BoxDirPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm);
		void 	BoxCircle(TimeValue t, float r, float d, Box3& box, int extraPt = 0, Matrix3 *tm = NULL);
		void 	BoxLight(TimeValue t, INode *inode, Box3& box, Matrix3 *tm = NULL);
		void 	BoxPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm);
		void 	GetAttenPoints(TimeValue t, float rad, Point3 *q);

		virtual bool PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext);
		//int 	Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
		virtual unsigned long GetObjectDisplayRequirement() const;
		void 	GetWorldBoundBox(TimeValue t, INode *mat, ViewExp* vpt, Box3& box);
		void 	GetLocalBoundBox(TimeValue t, INode *mat, ViewExp* vpt, Box3& box);
		void 	SetDecayRadius(TimeValue time, float f);
		float 	GetDecayRadius(TimeValue t, Interval& valid = Interval(0, 0));
		void 	GetConePoints(TimeValue t, float aspect, float angle, float dist, Point3 *q);
		int 	GetCirXPoints(TimeValue t, float angle, float dist, Point3 *q);
		void 	DrawCone(TimeValue t, GraphicsWindow *gw, float dist);
		int 	DrawConeAndLine(TimeValue t, INode* inode, GraphicsWindow *gw, int drawing);
		int 	DrawAtten(TimeValue t, INode *inode, GraphicsWindow *gw);
		void 	DrawAttenCirOrRect(TimeValue t, GraphicsWindow *gw, float dist, BOOL froze, int uicol);
		void 	DrawSphereArcs(TimeValue t, GraphicsWindow *gw, float r, Point3 *q);

protected:
	virtual void SetReference(int, RefTargetHandle rtarg);

/*private:
	// Parameter block
	IParamBlock2 *pblock; //ref 0*/
};

void LuxLightSphere::SetReference(int i, RefTargetHandle rtarg)
{
	if (i == PBLOCK_REF)
	{
		pblock = (IParamBlock2*)rtarg;
	}
}

RefTargetHandle LuxLightSphere::GetReference(int i)
{
	if (i == PBLOCK_REF)
	{
		return pblock;
	}
	return nullptr;
}

class LuxLightSphereClassDesc : public ClassDesc2
{
public:
	virtual int				IsPublic() 						{ return TRUE; }
	virtual void*			Create(BOOL loading = FALSE) 	{ return new LuxLightSphere(); }
	//int 					BeginCreate(Interface *i);
	//int 					EndCreate(Interface *i);
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_LIGHT_SPHERE_CLASS_NAME); }
	const TCHAR*	NonLocalizedClassName() { return GetString(IDS_LIGHT_SPHERE_CLASS_NAME); }
	virtual SClass_ID		SuperClassID() 			{ return LIGHT_CLASS_ID; }
	virtual Class_ID		ClassID() 				{ return LUX_LIGHT_SPHERE_CLASS_ID; }
	virtual const TCHAR*	Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR*	InternalName() 			{ return _T("Lux Sphere"); }		// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE		HInstance() 			{ return hInstance; }			// returns owning module handle
	

};


ClassDesc2* GetLuxLightSphereDesc() { 
	static LuxLightSphereClassDesc LuxLightSphereDesc;
	return &LuxLightSphereDesc;
}


enum { LuxLightSphere_params };

enum { sphere_light_param, common_param };

//TODO: Add enums for various parameters
enum { 
	pb_light_redius,
	pb_light_color,
	pb_light_power,
	pb_light_efficency,
	// common
	pb_light_gain,
	pb_light_multiplier,
	pb_light_sample,
	pb_light_id,
	pb_light_importance,
};


static ParamBlockDesc2 LuxLightSphere_param_blk ( LuxLightSphere_params, _T("params"),  0, GetLuxLightSphereDesc(),
	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF, 2,
	//rollout
	sphere_light_param,	IDD_LIGHT_SPHERE,		IDS_LIGHT_POINT_PARAMS, 0, 0, NULL,
	common_param,		IDD_LIGHT_COMMON_PANEL, IDS_LIGHT_COMMON_PARAMS, 0, 0, NULL,
	// params
	pb_light_redius,				_T("redius"),	TYPE_FLOAT,			P_ANIMATABLE, IDS_LIGHT_POWER_SPIN,
		p_default, 10.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, sphere_light_param,	TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_POINT_REDIUS, IDC_LIGHT_POINT_REDIUS_SPIN, 1.0f,
	p_end,
	pb_light_color,					_T("color"),		TYPE_RGBA,		P_ANIMATABLE, IDS_LIGHT_COLOR,
		p_default,					Color(1.0f, 1.0f, 1.0f),
		p_ui, sphere_light_param,	TYPE_COLORSWATCH,					IDC_LIGHT_POINT_COLOR,
	p_end,
	pb_light_power,					_T("power"),		TYPE_FLOAT,		P_ANIMATABLE, IDS_LIGHT_POWER_SPIN,
		p_default,					150.0f,
		p_range,					0.0f, 1000.0f,
		p_ui, sphere_light_param,	TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_POINT_POWER, IDC_LIGHT_POINT_POWER_SPIN, 1.0f,
	p_end,
	pb_light_efficency,				_T("efficency"),	TYPE_FLOAT,		P_ANIMATABLE, IDS_LIGHT_POWER_SPIN,
		p_default,					1.0f,
		p_range,					0.0f, 1000.0f,
		p_ui, sphere_light_param,	TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_POINT_EFFICENCY, IDC_LIGHT_POINT_EFFICENCY_SPIN, 0.01f,
	p_end,
	// common
	pb_light_gain,				_T("gain"),			TYPE_RGBA,		P_ANIMATABLE,	IDS_LIGHT_GAIN_SPIN,
		p_default,				Color(1.0f, 1.0f, 1.0f),
		p_ui, common_param,		TYPE_COLORSWATCH,					IDC_LIGHT_GAIN,
		p_end,
	pb_light_multiplier,		_T("multiplier"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIGHT_MULTIPLIER_SPIN,
		p_default, 0.0f,
		p_range, 0.0f, 10000.0f,
		p_ui, common_param, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_LIGHT_MULTIPLAIER, IDC_LIGHT_MULTIPLAIER_SPIN, 1.0f,
	p_end,
	pb_light_sample, 			_T("sample"), 		TYPE_INT, 		P_ANIMATABLE, 	IDS_LIGHT_SAMPLE_SPIN,
		p_default, 				-1, 
		p_range, 				-1,1000, 
		p_ui, common_param,		TYPE_SPINNER,		EDITTYPE_INT,	IDC_LIGHT_SAMPLE, IDC_LIGHT_SAMPLE_SPIN, 1,
		p_end,
	pb_light_id,				_T("id"),			TYPE_INT,		P_ANIMATABLE,	IDS_LIGHT_SAMPLE_SPIN,
		p_default,				12345,
		p_range,				1, 99999,
		p_ui, common_param,		TYPE_SPINNER,		EDITTYPE_INT,	IDC_LIGHT_ID,	IDC_LIGHT_ID_SPIN, 1,
		p_end,
	pb_light_importance,	_T("importance"),		TYPE_FLOAT,		P_ANIMATABLE,	IDS_LIGHT_IMPORTANCE_SPIN,
		p_default,				0.1f,
		p_range,				0.0f, 1000.0f,
		p_ui, common_param,		TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_IMPORTANCE, IDC_LIGHT_IMPORTANCE_SPIN, 0.01f,
		p_end,
	p_end
	);


RefResult LuxLightSphere::EvalLightState(TimeValue t, Interval& valid, LightState* ls)
{
	if (useLight)
		ls->color = GetRGBColor(t, valid);
	else
		ls->color = Point3(0, 0, 0);
	return REF_SUCCEED;
}

const TCHAR *LuxLightSphere::GetObjectName(bool localized)
{
	return _T("Lux Sphere");
}

void LuxLightSphere::InitNodeName(TSTR& s)
{
	s = _T("Lux Sphere");
	return;
}

void LuxLightSphere::DrawCone(TimeValue t, GraphicsWindow *gw, float dist)
{
	Point3 q[NUM_CIRC_PTS + 1], u[3];
	int i;
	BOOL dispDecay = true;

	gw->setColor(LINE_COLOR, GetUIColor(COLOR_HOTSPOT));
	// RECTANGULAR
	gw->polyline(4, q, NULL, NULL, TRUE, NULL);
	u[0] = Point3(0, 0, 0);
	for (i = 0; i < 4; i++) {
		u[1] = q[i];
		gw->polyline(2, u, NULL, NULL, FALSE, NULL);
	}

	GetConePoints(t, GetAspect(t), GetFallsize(t), dist, q);
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_FALLOFF));
	gw->polyline(4, q, NULL, NULL, TRUE, NULL);

	for (i = 0; i < 4; i += 1) {
		u[1] = q[i];
		gw->polyline(2, u, NULL, NULL, FALSE, NULL);
	}
}

int LuxLightSphere::DrawConeAndLine(TimeValue t, INode* inode, GraphicsWindow *gw, int drawing)
{
	Matrix3 tm = inode->GetObjectTM(t);
	gw->setTransform(tm);
	gw->clearHitCode();
	float dist = 20;
	DrawCone(t, gw, dist);
	return gw->checkHitCode();
}

void LuxLightSphere::GetConePoints(TimeValue t, float aspect, float angle, float dist, Point3 *q)
{
	float ta = (float)tan(0.5*DegToRad(angle));
	if (aspect <= 0.0f) {
		// CIRCULAR
		float rad = dist * ta;
		double a;
		int i;
		for (i = 0; i < NUM_CIRC_PTS; i++) {
			a = (double)i * 2.0 * 3.1415926 / (double)NUM_CIRC_PTS;
			q[i] = Point3(rad*(float)sin(a), rad*(float)cos(a), -dist);
		}
		// alexc - june.12.2003 - made Roll Angle Indicator size proportional to radius
		q[i] = q[0];
		q[i].y *= 1.15f; // notch length is 15% of the radius
	}
	else {
		// RECTANGULAR
		float w = dist * ta * (float)sqrt((double)aspect);
		float h = w / aspect;
		q[0] = Point3(w, h, -dist);
		q[1] = Point3(-w, h, -dist);
		q[2] = Point3(-w, -h, -dist);
		q[3] = Point3(w, -h, -dist);
		q[4] = Point3(0.0f, h + 15.0f, -dist);
		q[5] = Point3(0.0f, h, -dist);
	}
}

int LuxLightSphere::GetCirXPoints(TimeValue t, float angle, float dist, Point3 *q) {
	int i;
	float ang = DegToRad(angle) / 2.0f;
	float da = ang / float(NUM_HALF_ARC);
	// first draw circle:
	float d = dist * (float)cos(ang);
	GetConePoints(t, -1.0f, angle, d, q);
	int j = NUM_CIRC_PTS;
	// then draw Arc X
	float a = -ang;
	for (i = -NUM_HALF_ARC; i <= NUM_HALF_ARC; i++, a += da)
		q[j++] = Point3(0.0f, dist*(float)sin(a), -dist * (float)cos(a));
	a = -ang;
	for (i = -NUM_HALF_ARC; i <= NUM_HALF_ARC; i++, a += da)
		q[j++] = Point3(dist*(float)sin(a), 0.0f, -dist * (float)cos(a));
	return NUM_CIRC_PTS + 2 * NUM_ARC_PTS;
}

void LuxLightSphere::SetDecayRadius(TimeValue t, float f)
{
	pblock->SetValue(type == OMNI_LIGHT ? PB_OMNIDECAY : PB_DECAY, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSphere::GetDecayRadius(TimeValue t, Interval& valid)
{
	float f = 10.0f;
	pblock->GetValue(0, t, f, valid);
	return f;
}

inline float MaxF(float a, float b) { return a > b ? a : b; }
inline float MinF(float a, float b) { return a < b ? a : b; }

class LightConeItem : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
	LuxLightSphere* mpLight;
public:
	LightConeItem(LuxLightSphere* lt)
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
		if (nullptr == node)
		{
			return;
		}
		mpLight->SetExtendedDisplay(drawContext.GetExtendedDisplayMode());
		ClearLines();
		//BuildConeAndLine(drawContext.GetTime(), drawContext.GetCurrentNode());
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
		int i;
		BOOL dispDecay = true;

		Color color(GetUIColor(COLOR_HOTSPOT));
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

	void BuildCircleX(TimeValue t, float angle, float dist, Point3* pos, Color& color) {
		mpLight->GetCirXPoints(t, angle, dist, pos);
		AddLineStrip(pos, color, NUM_CIRC_PTS, true, true); // circle 
		AddLineStrip(&pos[NUM_CIRC_PTS], color, NUM_ARC_PTS, false, true); // vert arc
		AddLineStrip(&pos[NUM_CIRC_PTS + NUM_ARC_PTS], color, NUM_ARC_PTS, false, true);  // horiz arc
	}

	void BuildAttenCirOrRect(TimeValue t, float dist, BOOL froze, int uicol) {
		Color color = froze ? GetFreezeColor() : GetUIColor(uicol);
		Point3 posArray[3 * NUM_CIRC_PTS + 2];
		BuildSphereArcs(t, dist, posArray, color);

	}

	int BuildAtten(TimeValue t, INode *inode)
	{
		BOOL dispDecay = true;

		if (dispDecay) {
			BOOL froze = inode->IsFrozen() && !inode->Dependent();
			if (dispDecay) {
				BuildAttenCirOrRect(t, mpLight->GetDecayRadius(t), froze, COLOR_DECAY_RADIUS);
			}
		}
		return 0;
	}
};

void LuxLightSphere::GetAttenPoints(TimeValue t, float rad, Point3 *q)
{
	double a;
	float sn, cs;
	for (int i = 0; i < NUM_CIRC_PTS; i++) {
		a = (double)i * 2.0 * 3.1415926 / (double)NUM_CIRC_PTS;
		sn = rad * (float)sin(a);
		cs = rad * (float)cos(a);
		q[i + 0 * NUM_CIRC_PTS] = Point3(sn, cs, 0.0f);
		q[i + 1 * NUM_CIRC_PTS] = Point3(sn, 0.0f, cs);
		q[i + 2 * NUM_CIRC_PTS] = Point3(0.0f, sn, cs);
	}
}

void LuxLightSphere::DrawSphereArcs(TimeValue t, GraphicsWindow *gw, float r, Point3 *q) {
	GetAttenPoints(t, r, q);
	gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
	gw->polyline(NUM_CIRC_PTS, q + NUM_CIRC_PTS, NULL, NULL, TRUE, NULL);
	gw->polyline(NUM_CIRC_PTS, q + 2 * NUM_CIRC_PTS, NULL, NULL, TRUE, NULL);
}

void LuxLightSphere::DrawAttenCirOrRect(TimeValue t, GraphicsWindow *gw, float dist, BOOL froze, int uicol) {
	if (!froze) gw->setColor(LINE_COLOR, GetUIColor(uicol));
	{
		Point3 q[3 * NUM_CIRC_PTS + 1];
		DrawSphereArcs(t, gw, dist, q);
	}
}

int LuxLightSphere::DrawAtten(TimeValue t, INode *inode, GraphicsWindow *gw)
{
	BOOL dispDecay = true;

	if (dispDecay) {
		Matrix3 tm = inode->GetObjectTM(t);
		gw->setTransform(tm);
		BOOL froze = inode->IsFrozen() && !inode->Dependent();
		if (dispDecay) {
			DrawAttenCirOrRect(t, gw, GetDecayRadius(t), froze, COLOR_DECAY_RADIUS);
		}
	}
	return 0;
}

void LuxLightSphere::BoxCircle(TimeValue t, float r, float d, Box3& box, int extraPt, Matrix3 *tm) {
	Point3 q[3 * NUM_CIRC_PTS];
	int npts;
	float asp;
	//if (GetSpotShape()) { npts = NUM_CIRC_PTS + extraPt; 	asp = -1.0f; }
	//else { npts = 4 + extraPt;  asp = GetAspect(t); }
	//GetConePoints(t, asp, r, d, q);
	npts = 4 + extraPt;  asp = GetAspect(t);
	box.IncludePoints(q, npts, tm);
}

void LuxLightSphere::BoxDirPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm) {
	int npts;
	Point3 q[3 * NUM_CIRC_PTS];
	//npts = GetSpotShape() ? GetCirXPoints(t, angle, dist, q) : GetRectXPoints(t, angle, dist, q);
	//box.IncludePoints(q, npts, tm);
}


void LuxLightSphere::BoxPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm) {
	/*if (IsDir())
		BoxCircle(t, angle, dist, box, 0, tm);
	else
		BoxDirPoints(t, angle, dist, box, tm);*/
}

static int GetTargetPoint(TimeValue t, INode *inode, Point3& p)
{
	Matrix3 tmat;
	if (inode->GetTargetTM(t, tmat)) {
		p = tmat.GetTrans();
		return 1;
	}
	else
		return 0;
}

void LuxLightSphere::BoxLight(TimeValue t, INode *inode, Box3& box, Matrix3 *tm) {
	Point3 pt;
	float d = GetTDist(t);
	//R6 change 
	if (!(extDispFlags & EXT_DISP_GROUP_EXT))
	{
		if (GetTargetPoint(t, inode, pt)) {
			Point3 loc = inode->GetObjectTM(t).GetTrans();
			d = FLength(loc - pt) / FLength(inode->GetObjectTM(t).GetRow(2));
			box += tm ? (*tm)*Point3(0.0f, 0.0f, -d) : Point3(0.0f, 0.0f, -d);
		}
		else {
			if (coneDisplay)
				box += tm ? (*tm)*Point3(0.0f, 0.0f, -d) : Point3(0.0f, 0.0f, -d);
		}
	}
	if (coneDisplay || (extDispFlags & EXT_DISP_ONLY_SELECTED)) {
		float rad = MaxF(GetHotspot(t), GetFallsize(t));
		BoxCircle(t, rad, d, box, 1, tm);
	}

	Point3 q[3 * NUM_CIRC_PTS];
	float rad = 1.5; //0;
	GetAttenPoints(t, rad, q);
	box.IncludePoints(q, 3 * NUM_CIRC_PTS, tm);
}

void LuxLightSphere::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	if (!vpt || !vpt->IsAlive())
	{
		box.Init();
		return;
	}

	Point3 loc = inode->GetObjectTM(t).GetTrans();
	float scaleFactor = vpt->NonScalingObjectSize()*vpt->GetVPWorldWidth(loc) / 360.0f;
	box = mesh->getBoundingBox();//same as GetDeformBBox
	box.Scale(scaleFactor);

	//this adds the target point, cone, etc.
	BoxLight(t, inode, box, NULL);
}

void LuxLightSphere::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	if (!vpt || !vpt->IsAlive())
	{
		box.Init();
		return;
	}

	int nv;
	Matrix3 tm;
	//GetMat(t, inode, *vpt, tm);
	Point3 loc = tm.GetTrans();
	nv = mesh->getNumVerts();
	box.Init();
	if (!(extDispFlags & EXT_DISP_ZOOM_EXT))
		box.IncludePoints(mesh->verts, nv, &tm);
	else
		box += loc;
	tm = inode->GetObjectTM(t);
	BoxLight(t, inode, box, &tm);
}

short LuxLightSphere::meshBuilt = 0;
Mesh LuxLightSphere::staticMesh;

void LuxLightSphere::BuildStaticMeshes()
{
	if (!meshBuilt) {
		int nverts = 90;
		int nfaces = 120;

		// Build a leetle octahedron
		staticMesh.setNumVerts(nverts);
		staticMesh.setNumFaces(nfaces);
		float s = 8.0f;
		staticMesh.setVert(0, Point3(0.0, 0.0, 11.3805));	staticMesh.setVert(1, Point3(-4.97457e-07, 11.3805, -4.97457e-07));	staticMesh.setVert(2, Point3(-11.3805, -9.94915e-07, -4.97457e-07));	staticMesh.setVert(3, Point3(1.35711e-07, -11.3805, -4.97457e-07));
		staticMesh.setVert(4, Point3(11.3805, 1.98983e-06, -4.97457e-07));	staticMesh.setVert(5, Point3(0.0, 0.0, -11.3805));	staticMesh.setVert(6, Point3(0.233655, 17.6349, 0.0));	staticMesh.setVert(7, Point3(-0.135273, 17.6349, 0.213001));
		staticMesh.setVert(8, Point3(-0.135273, 17.6349, -0.213001));	staticMesh.setVert(9, Point3(0.233655, 23.5209, 0.0));	staticMesh.setVert(10, Point3(-0.135273, 23.5209, 0.213001));	staticMesh.setVert(11, Point3(-0.135273, 23.5209, -0.213001));
		staticMesh.setVert(12, Point3(12.4556, 0.000858435, 12.1252));	staticMesh.setVert(13, Point3(12.1947, -0.212142, 12.386));	staticMesh.setVert(14, Point3(12.1947, 0.213859, 12.386));	staticMesh.setVert(15, Point3(16.6177, 0.000857476, 16.2872));
		staticMesh.setVert(16, Point3(16.3568, -0.212143, 16.5481));	staticMesh.setVert(17, Point3(16.3568, 0.213858, 16.5481));	staticMesh.setVert(18, Point3(0.233655, 12.529, 12.529));	staticMesh.setVert(19, Point3(-0.135273, 12.3784, 12.6796));
		staticMesh.setVert(20, Point3(-0.135273, 12.6796, 12.3784));	staticMesh.setVert(21, Point3(0.233655, 16.6911, 16.6911));	staticMesh.setVert(22, Point3(-0.135273, 16.5405, 16.8417));	staticMesh.setVert(23, Point3(-0.135273, 16.8417, 16.5405));
		staticMesh.setVert(24, Point3(0.233655, 0.000858435, 17.3812));	staticMesh.setVert(25, Point3(-0.135273, -0.212142, 17.3812));	staticMesh.setVert(26, Point3(-0.135273, 0.213859, 17.3812));	staticMesh.setVert(27, Point3(0.233655, 0.000857476, 23.2673));
		staticMesh.setVert(28, Point3(-0.135273, -0.212143, 23.2673));	staticMesh.setVert(29, Point3(-0.135273, 0.213858, 23.2673));	staticMesh.setVert(30, Point3(-12.1252, 0.000858435, 12.4556));	staticMesh.setVert(31, Point3(-12.386, -0.212142, 12.1947));
		staticMesh.setVert(32, Point3(-12.386, 0.213859, 12.1947));	staticMesh.setVert(33, Point3(-16.2872, 0.000857476, 16.6177));	staticMesh.setVert(34, Point3(-16.5481, -0.212143, 16.3568));	staticMesh.setVert(35, Point3(-16.5481, 0.213858, 16.3568));
		staticMesh.setVert(36, Point3(0.233655, -12.529, 12.529));	staticMesh.setVert(37, Point3(-0.135273, -12.6796, 12.3784));	staticMesh.setVert(38, Point3(-0.135273, -12.3784, 12.6796));	staticMesh.setVert(39, Point3(0.233655, -16.6911, 16.6911));
		staticMesh.setVert(40, Point3(-0.135273, -16.8417, 16.5405));	staticMesh.setVert(41, Point3(-0.135273, -16.5405, 16.8417));	staticMesh.setVert(42, Point3(-17.535, 0.00085839, 0.245951));	staticMesh.setVert(43, Point3(-17.535, -0.212142, -0.122977));
		staticMesh.setVert(44, Point3(-17.535, 0.213859, -0.122977));	staticMesh.setVert(45, Point3(-23.421, 0.000858234, 0.245953));	staticMesh.setVert(46, Point3(-23.421, -0.212142, -0.122975));	staticMesh.setVert(47, Point3(-23.421, 0.213859, -0.122975));
		staticMesh.setVert(48, Point3(0.233655, -17.7187, 3.76563e-06));	staticMesh.setVert(49, Point3(-0.135273, -17.7187, -0.212997));	staticMesh.setVert(50, Point3(-0.135273, -17.7187, 0.213004));	staticMesh.setVert(51, Point3(0.233655, -23.6047, -3.76563e-06));
		staticMesh.setVert(52, Point3(-0.135273, -23.6047, -0.213004));	staticMesh.setVert(53, Point3(-0.135273, -23.6047, 0.212997));	staticMesh.setVert(54, Point3(-12.4556, 0.000858435, -12.1252));	staticMesh.setVert(55, Point3(-12.1947, -0.212142, -12.386));
		staticMesh.setVert(56, Point3(-12.1947, 0.213859, -12.386));	staticMesh.setVert(57, Point3(-16.6177, 0.000857476, -16.2872));	staticMesh.setVert(58, Point3(-16.3568, -0.212143, -16.5481));	staticMesh.setVert(59, Point3(-16.3568, 0.213858, -16.5481));
		staticMesh.setVert(60, Point3(0.233655, -12.529, -12.529));	staticMesh.setVert(61, Point3(-0.135273, -12.3784, -12.6796));	staticMesh.setVert(62, Point3(-0.135273, -12.6796, -12.3784));	staticMesh.setVert(63, Point3(0.233655, -16.6911, -16.6911));
		staticMesh.setVert(64, Point3(-0.135273, -16.5405, -16.8417));	staticMesh.setVert(65, Point3(-0.135273, -16.8417, -16.5405));	staticMesh.setVert(66, Point3(0.233655, 0.000860222, -23.4969));	staticMesh.setVert(67, Point3(-0.135273, -0.21214, -23.4969));
		staticMesh.setVert(68, Point3(-0.135273, 0.213861, -23.4969));	staticMesh.setVert(69, Point3(0.233655, 0.000859263, -17.6109));	staticMesh.setVert(70, Point3(-0.135273, -0.212141, -17.6109));	staticMesh.setVert(71, Point3(-0.135273, 0.21386, -17.6109));
		staticMesh.setVert(72, Point3(12.1252, 0.000858435, -12.4556));	staticMesh.setVert(73, Point3(12.386, -0.212142, -12.1947));	staticMesh.setVert(74, Point3(12.386, 0.213859, -12.1947));	staticMesh.setVert(75, Point3(16.2872, 0.000857476, -16.6177));
		staticMesh.setVert(76, Point3(16.5481, -0.212143, -16.3568));	staticMesh.setVert(77, Point3(16.5481, 0.213858, -16.3568));	staticMesh.setVert(78, Point3(0.233655, 12.529, -12.529));	staticMesh.setVert(79, Point3(-0.135273, 12.6796, -12.3784));
		staticMesh.setVert(80, Point3(-0.135273, 12.3784, -12.6796));	staticMesh.setVert(81, Point3(0.233655, 16.6911, -16.6911));	staticMesh.setVert(82, Point3(-0.135273, 16.8417, -16.5405));	staticMesh.setVert(83, Point3(-0.135273, 16.5405, -16.8417));
		staticMesh.setVert(84, Point3(17.7902, 0.000858661, -0.245952));	staticMesh.setVert(85, Point3(17.7902, -0.212142, 0.122976));	staticMesh.setVert(86, Point3(17.7902, 0.213859, 0.122975));	staticMesh.setVert(87, Point3(23.6763, 0.000857784, -0.245951));
		staticMesh.setVert(88, Point3(23.6763, -0.212143, 0.122976));	staticMesh.setVert(89, Point3(23.6763, 0.213858, 0.122976));
		staticMesh.faces[0].setVerts(0, 1, 2);	staticMesh.faces[0].setEdgeVisFlags(1, 1, 1);	staticMesh.faces[1].setVerts(0, 2, 3);	staticMesh.faces[1].setEdgeVisFlags(1, 1, 1);
		staticMesh.faces[2].setVerts(0, 3, 4);	staticMesh.faces[2].setEdgeVisFlags(1, 1, 1);	staticMesh.faces[3].setVerts(0, 4, 1);	staticMesh.faces[3].setEdgeVisFlags(1, 1, 1);
		staticMesh.faces[4].setVerts(5, 2, 1);	staticMesh.faces[4].setEdgeVisFlags(1, 1, 1);	staticMesh.faces[5].setVerts(5, 3, 2);	staticMesh.faces[5].setEdgeVisFlags(1, 1, 1);
		staticMesh.faces[6].setVerts(5, 4, 3);	staticMesh.faces[6].setEdgeVisFlags(1, 1, 1);	staticMesh.faces[7].setVerts(5, 1, 4);	staticMesh.faces[7].setEdgeVisFlags(1, 1, 1);
		staticMesh.faces[8].setVerts(6, 9, 10);	staticMesh.faces[8].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[9].setVerts(10, 7, 6);	staticMesh.faces[9].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[10].setVerts(7, 10, 11);	staticMesh.faces[10].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[11].setVerts(11, 8, 7);	staticMesh.faces[11].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[12].setVerts(8, 11, 9);	staticMesh.faces[12].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[13].setVerts(9, 6, 8);	staticMesh.faces[13].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[14].setVerts(7, 8, 6);	staticMesh.faces[14].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[15].setVerts(11, 10, 9);	staticMesh.faces[15].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[16].setVerts(12, 15, 16);	staticMesh.faces[16].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[17].setVerts(16, 13, 12);	staticMesh.faces[17].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[18].setVerts(13, 16, 17);	staticMesh.faces[18].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[19].setVerts(17, 14, 13);	staticMesh.faces[19].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[20].setVerts(14, 17, 15);	staticMesh.faces[20].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[21].setVerts(15, 12, 14);	staticMesh.faces[21].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[22].setVerts(13, 14, 12);	staticMesh.faces[22].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[23].setVerts(17, 16, 15);	staticMesh.faces[23].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[24].setVerts(18, 21, 22);	staticMesh.faces[24].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[25].setVerts(22, 19, 18);	staticMesh.faces[25].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[26].setVerts(19, 22, 23);	staticMesh.faces[26].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[27].setVerts(23, 20, 19);	staticMesh.faces[27].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[28].setVerts(20, 23, 21);	staticMesh.faces[28].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[29].setVerts(21, 18, 20);	staticMesh.faces[29].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[30].setVerts(19, 20, 18);	staticMesh.faces[30].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[31].setVerts(23, 22, 21);	staticMesh.faces[31].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[32].setVerts(24, 27, 28);	staticMesh.faces[32].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[33].setVerts(28, 25, 24);	staticMesh.faces[33].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[34].setVerts(25, 28, 29);	staticMesh.faces[34].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[35].setVerts(29, 26, 25);	staticMesh.faces[35].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[36].setVerts(26, 29, 27);	staticMesh.faces[36].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[37].setVerts(27, 24, 26);	staticMesh.faces[37].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[38].setVerts(25, 26, 24);	staticMesh.faces[38].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[39].setVerts(29, 28, 27);	staticMesh.faces[39].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[40].setVerts(30, 33, 34);	staticMesh.faces[40].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[41].setVerts(34, 31, 30);	staticMesh.faces[41].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[42].setVerts(31, 34, 35);	staticMesh.faces[42].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[43].setVerts(35, 32, 31);	staticMesh.faces[43].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[44].setVerts(32, 35, 33);	staticMesh.faces[44].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[45].setVerts(33, 30, 32);	staticMesh.faces[45].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[46].setVerts(31, 32, 30);	staticMesh.faces[46].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[47].setVerts(35, 34, 33);	staticMesh.faces[47].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[48].setVerts(36, 39, 40);	staticMesh.faces[48].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[49].setVerts(40, 37, 36);	staticMesh.faces[49].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[50].setVerts(37, 40, 41);	staticMesh.faces[50].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[51].setVerts(41, 38, 37);	staticMesh.faces[51].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[52].setVerts(38, 41, 39);	staticMesh.faces[52].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[53].setVerts(39, 36, 38);	staticMesh.faces[53].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[54].setVerts(37, 38, 36);	staticMesh.faces[54].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[55].setVerts(41, 40, 39);	staticMesh.faces[55].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[56].setVerts(42, 45, 46);	staticMesh.faces[56].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[57].setVerts(46, 43, 42);	staticMesh.faces[57].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[58].setVerts(43, 46, 47);	staticMesh.faces[58].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[59].setVerts(47, 44, 43);	staticMesh.faces[59].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[60].setVerts(44, 47, 45);	staticMesh.faces[60].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[61].setVerts(45, 42, 44);	staticMesh.faces[61].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[62].setVerts(43, 44, 42);	staticMesh.faces[62].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[63].setVerts(47, 46, 45);	staticMesh.faces[63].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[64].setVerts(48, 51, 52);	staticMesh.faces[64].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[65].setVerts(52, 49, 48);	staticMesh.faces[65].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[66].setVerts(49, 52, 53);	staticMesh.faces[66].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[67].setVerts(53, 50, 49);	staticMesh.faces[67].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[68].setVerts(50, 53, 51);	staticMesh.faces[68].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[69].setVerts(51, 48, 50);	staticMesh.faces[69].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[70].setVerts(49, 50, 48);	staticMesh.faces[70].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[71].setVerts(53, 52, 51);	staticMesh.faces[71].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[72].setVerts(54, 57, 58);	staticMesh.faces[72].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[73].setVerts(58, 55, 54);	staticMesh.faces[73].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[74].setVerts(55, 58, 59);	staticMesh.faces[74].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[75].setVerts(59, 56, 55);	staticMesh.faces[75].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[76].setVerts(56, 59, 57);	staticMesh.faces[76].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[77].setVerts(57, 54, 56);	staticMesh.faces[77].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[78].setVerts(55, 56, 54);	staticMesh.faces[78].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[79].setVerts(59, 58, 57);	staticMesh.faces[79].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[80].setVerts(60, 63, 64);	staticMesh.faces[80].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[81].setVerts(64, 61, 60);	staticMesh.faces[81].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[82].setVerts(61, 64, 65);	staticMesh.faces[82].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[83].setVerts(65, 62, 61);	staticMesh.faces[83].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[84].setVerts(62, 65, 63);	staticMesh.faces[84].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[85].setVerts(63, 60, 62);	staticMesh.faces[85].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[86].setVerts(61, 62, 60);	staticMesh.faces[86].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[87].setVerts(65, 64, 63);	staticMesh.faces[87].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[88].setVerts(66, 69, 70);	staticMesh.faces[88].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[89].setVerts(70, 67, 66);	staticMesh.faces[89].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[90].setVerts(67, 70, 71);	staticMesh.faces[90].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[91].setVerts(71, 68, 67);	staticMesh.faces[91].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[92].setVerts(68, 71, 69);	staticMesh.faces[92].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[93].setVerts(69, 66, 68);	staticMesh.faces[93].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[94].setVerts(67, 68, 66);	staticMesh.faces[94].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[95].setVerts(71, 70, 69);	staticMesh.faces[95].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[96].setVerts(72, 75, 76);	staticMesh.faces[96].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[97].setVerts(76, 73, 72);	staticMesh.faces[97].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[98].setVerts(73, 76, 77);	staticMesh.faces[98].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[99].setVerts(77, 74, 73);	staticMesh.faces[99].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[100].setVerts(74, 77, 75);	staticMesh.faces[100].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[101].setVerts(75, 72, 74);	staticMesh.faces[101].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[102].setVerts(73, 74, 72);	staticMesh.faces[102].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[103].setVerts(77, 76, 75);	staticMesh.faces[103].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[104].setVerts(78, 81, 82);	staticMesh.faces[104].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[105].setVerts(82, 79, 78);	staticMesh.faces[105].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[106].setVerts(79, 82, 83);	staticMesh.faces[106].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[107].setVerts(83, 80, 79);	staticMesh.faces[107].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[108].setVerts(80, 83, 81);	staticMesh.faces[108].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[109].setVerts(81, 78, 80);	staticMesh.faces[109].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[110].setVerts(79, 80, 78);	staticMesh.faces[110].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[111].setVerts(83, 82, 81);	staticMesh.faces[111].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[112].setVerts(84, 87, 88);	staticMesh.faces[112].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[113].setVerts(88, 85, 84);	staticMesh.faces[113].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[114].setVerts(85, 88, 89);	staticMesh.faces[114].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[115].setVerts(89, 86, 85);	staticMesh.faces[115].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[116].setVerts(86, 89, 87);	staticMesh.faces[116].setEdgeVisFlags(1, 1, 0);	staticMesh.faces[117].setVerts(87, 84, 86);	staticMesh.faces[117].setEdgeVisFlags(1, 1, 0);
		staticMesh.faces[118].setVerts(85, 86, 84);	staticMesh.faces[118].setEdgeVisFlags(1, 0, 1);	staticMesh.faces[119].setVerts(89, 88, 87);	staticMesh.faces[119].setEdgeVisFlags(1, 1, 0);
		staticMesh.buildNormals();
		staticMesh.EnableEdgeList(1);
		staticMesh.InvalidateGeomCache();

		// Build an "arrow"
		meshBuilt = 1;
	}
}

void LuxLightSphere::BuildMeshes(BOOL isnew) {
	BuildStaticMeshes();
	mesh = &staticMesh;
}

void LuxLightSphere::SetExtendedDisplay(int flags)
{
	extDispFlags = flags;
}

MaxSDK::Graphics::Utilities::MeshEdgeKey LightSphereMeshKey;
MaxSDK::Graphics::Utilities::SplineItemKey LightSphereSplineKey;

unsigned long LuxLightSphere::GetObjectDisplayRequirement() const
{
	return 0;
}

bool LuxLightSphere::PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& displayContext)
{
	LightSphereMeshKey.SetFixedSize(true);
	return true;
}

bool LuxLightSphere::UpdatePerNodeItems(
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
		if (useLight)
		{
			Color color = GetIViewportShadingMgr()->GetLightIconColor(*pNode);
			pMeshItem->SetColor(color);
		}
		else
		{
			pMeshItem->SetColor(Color(0, 0, 0));
		}
	}

	MaxSDK::Graphics::CustomRenderItemHandle meshHandle;
	meshHandle.Initialize();
	meshHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	meshHandle.SetCustomImplementation(pMeshItem);
	MaxSDK::Graphics::ConsolidationData data;
	data.Strategy = &MaxSDK::Graphics::Utilities::MeshEdgeConsolidationStrategy::GetInstance();
	data.Key = &LightSphereMeshKey;
	meshHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(meshHandle);

	MaxSDK::Graphics::Utilities::SplineRenderItem* pLineItem = new LightConeItem(this);
	MaxSDK::Graphics::CustomRenderItemHandle coneHandle;
	coneHandle.Initialize();
	coneHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	coneHandle.SetCustomImplementation(pLineItem);
	data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
	data.Key = &LightSphereSplineKey;
	coneHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(coneHandle);

	/*pLineItem = new LightTargetLineItem(this);
	MaxSDK::Graphics::CustomRenderItemHandle lineHandle;
	lineHandle.Initialize();
	lineHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	lineHandle.SetCustomImplementation(pLineItem);
	data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
	data.Key = &LightSplineKey;
	lineHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(lineHandle);*/

	return true;
}

/*ParamDlg* LuxLightSphere::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLuxLightSphereDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
}*/

BOOL LuxLightSphere::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

LuxLightSphere * LuxLightSphere::currentEditLight = NULL;

void LuxLightSphere::SetUseLight(int onOff)
{
	//SimpleLightUndo< &GeneralLight::SetUseLight, false >::Hold(this, onOff, useLight);
	useLight = TRUE;//onOff;
	/*if (currentEditLight == this)  // LAM - 8/13/02 - defect 511609
		UpdateUICheckbox(hGeneralLight, IDC_LIGHT_ON, _T("enabled"), onOff); // 5/15/01 11:00am --MQM-- maxscript fix*/
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

void LuxLightSphere::SetHotspot(TimeValue t, float f)
{

	return;
	//pblock->SetValue(50, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSphere::GetHotspot(TimeValue t, Interval& valid)
{
	return -1.0f;
}

void LuxLightSphere::SetFallsize(TimeValue t, float f)
{
	return;
	//pblock->SetValue(PB_FALLSIZE, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSphere::GetFallsize(TimeValue t, Interval& valid)
{
	return -1.0f;
}

void LuxLightSphere::SetAtten(TimeValue t, int which, float f)
{
	return;
	//pblock->SetValue((type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1) + which, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSphere::GetAtten(TimeValue t, int which, Interval& valid)
{
	return -1.0f;
	//float f;
	//pblock->GetValue((type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1) + which, t, f, valid);
	//return f;
}

void LuxLightSphere::SetTDist(TimeValue t, float f)
{
	return;
	//pblock->SetValue(PB_TDIST, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSphere::GetTDist(TimeValue t, Interval& valid)
{
	return -1.0f;
	//float f;
	//pblock->GetValue(PB_TDIST, t, f, valid);
	//return f;
}

void LuxLightSphere::SetConeDisplay(int s, int notify)
{
	//DualLightUndo< &GeneralLight::SetConeDisplay, TRUE, false >::Hold(this, s, coneDisplay);
	coneDisplay = s;
	if (notify)
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

LuxLightSphere::LuxLightSphere()// : pblock(nullptr)
{
	enable = FALSE;
	useLight = TRUE;
	GetLuxLightSphereDesc()->MakeAutoParamBlocks(this);
	BuildMeshes();
	//CheckUIConsistency();

	//RegisterNotification(&NotifyDist, (void *)this, NOTIFY_UNITS_CHANGE);
}

LuxLightSphere::~LuxLightSphere()
{
	DeleteAllRefsFromMe();
	pblock = NULL;

	/*if (shadType && !shadType->SupportStdMapInterface()) {
		ReplaceReference(AS_SHADTYPE_REF, NULL);
	}

	UnRegisterNotification(&NotifyDist, (void *)this, NOTIFY_UNITS_CHANGE);*/
}

static void RemoveScaling(Matrix3 &tm) {
	AffineParts ap;
	decomp_affine(tm, &ap);
	tm.IdentityMatrix();
	tm.SetRotate(ap.q);
	tm.SetTrans(ap.t);
}

void LuxLightSphere::GetMat(TimeValue t, INode* inode, ViewExp &vpt, Matrix3& tm)
{
	if (!vpt.IsAlive())
	{
		tm.Zero();
		return;
	}

	tm = inode->GetObjectTM(t);
	//	tm.NoScale();
	RemoveScaling(tm);
	float scaleFactor = vpt.NonScalingObjectSize() * vpt.GetVPWorldWidth(tm.GetTrans()) / 360.0f;
	tm.Scale(Point3(scaleFactor, scaleFactor, scaleFactor));
}

int LuxLightSphere::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	if (!vpt || !vpt->IsAlive())
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
	MakeHitRegion(hitRegion, type, crossing, 4, p);
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~(GW_ILLUM | GW_BACKCULL));
	GetMat(t, inode, *vpt, m);
	gw->setTransform(m);
	// if we get a hit on the mesh, we're done
	res = mesh->select(gw, mtl, &hitRegion, flags & HIT_ABORTONHIT);
	// if not, check the target line, and set the pair flag if it's hit
	if (!res) {
		// this special case only works with point selection of targeted lights
		if ((type != HITTYPE_POINT) || !inode->GetTarget())
			return 0;
		// don't let line be active if only looking at selected stuff and target isn't selected
		if ((flags & HIT_SELONLY) && !inode->GetTarget()->Selected())
			return 0;
		gw->clearHitCode();
		res = DrawConeAndLine(t, inode, gw, -1);
		if (res != 0)
			inode->SetTargetNodePair(1);
	}
	gw->setRndLimits(savedLimits);
	return res;
}

/*int LuxLightSphere::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	Matrix3 m;
	if (!enable)
		return 0;
	GraphicsWindow *gw = vpt->getGW();
	GetMat(t, inode, *vpt, m);
	gw->setTransform(m);
	DWORD rlim = gw->getRndLimits();
	gw->setRndLimits(GW_WIREFRAME | GW_EDGES_ONLY | GW_BACKCULL | (gw->getRndMode() & GW_Z_BUFFER));
	if (inode->Selected())
		gw->setColor(LINE_COLOR, GetSelColor());
	else if (!inode->IsFrozen() && !inode->Dependent()) {
		if (useLight)
		{
			// 6/22/01 2:16pm --MQM--
			// use the wire-color to draw the light,
			// instead of COLOR_LIGHT_OBJ
			//! NH - Add code here to change the color of the light depending on the Viewport Shading
			Color color = GetIViewportShadingMgr()->GetLightIconColor(*inode);
			gw->setColor(LINE_COLOR, color);
		}
		// I un-commented this line DS 6/11/99
		else
			gw->setColor(LINE_COLOR, 0.0f, 0.0f, 0.0f);
	}
	mesh->render(gw, gw->getMaterial(),
		(flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, COMP_ALL);
	DrawConeAndLine(t, inode, gw, 1);
	DrawAtten(t, inode, gw);
	gw->setRndLimits(rlim);
	return 0;
}*/

class LightSphereCreateCallBack : public CreateMouseCallBack
{
	LuxLightSphere *ob;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(LuxLightSphere *obj) { ob = obj; }
};

int LightSphereCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	Point3 p0;
	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
	}

	if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
		if (point == 0)
		{
			ULONG handle;
			ob->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
			INode * node;
			node = GetCOREInterface()->GetINodeByHandle(handle);
			if (node)
			{
				Point3 color = GetUIColor(COLOR_LIGHT_OBJ);	// yellow wire color
				node->SetWireColor(RGB(color.x*255.0f, color.y*255.0f, color.z*255.0f));
			}
		}

		//mat.SetTrans(vpt->GetPointOnCP(m));
		mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));

		ob->enable = TRUE;
		if (point == 1) {
			if (msg == MOUSE_POINT)
				return 0;
		}
	}
	else if (msg == MOUSE_ABORT)
		return CREATE_ABORT;

	return TRUE;
}

static LightSphereCreateCallBack sGeneralLgtCreateCB;

RefResult LuxLightSphere::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
{
	switch (message) {
	case REFMSG_CHANGE:
	{
		//ivalid.SetEmpty();
		//mapValid.SetEmpty();
		if (hTarget == pblock)
		{
			ParamID changing_param = pblock->LastNotifyParamID();
			LuxLightSphere_param_blk.InvalidateUI(changing_param);
		}
	}
	break;
	case REFMSG_TARGET_DELETED:
	{
		if (hTarget == pblock)
		{
			pblock = nullptr;
		}
		else
		{
			/*for (int i = 0; i < NUM_SUBTEXTURES; i++)
			{
				if (hTarget == subtexture[i])
				{
					subtexture[i] = nullptr;
					break;
				}
			}*/
		}
		break;
	}
	}
	return REF_SUCCEED;
}

CreateMouseCallBack* LuxLightSphere::GetCreateMouseCallBack()
{
	sGeneralLgtCreateCB.SetObj(this);

	return &sGeneralLgtCreateCB;
}

#if 0
#define FLto255i(x)	((int)((x)*255.0f + 0.5f))
#define FLto255f(x)	((float)((x)*255.0f + 0.5f))
#else
#define FLto255i(x)	((int)((x)*255.0f))
#define FLto255f(x)	((float)((x)*255.0f))
#endif

HWND LuxLightSphere::hGeneralLight = NULL;
IColorSwatch *LuxLightSphere::colorSwatch;

void LuxLightSphere::UpdateUI(TimeValue t)
{
	Point3 color;

	if (hGeneralLight && 
		GetWindowLongPtr(hGeneralLight, GWLP_USERDATA) == (LONG_PTR)this && pblock) {
		color = GetRGBColor(t);
		colorSwatch->SetColor(RGB(FLto255i(color.x), FLto255i(color.y), FLto255i(color.z)));

		//UpdateColBrackets(t);

		// DC - Nov 30, 2001 moved this call into this block so that a NULL pblock is checked
		// bug 313434
		//UpdateControlledUI();
	}  // closes initial update check

}

ObjectState LuxLightSphere::Eval(TimeValue time)
{
	UpdateUI(time);
	return ObjectState(this);
}

void LuxLightSphere::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);

	GetLuxLightSphereDesc()->BeginEditParams(ip, this, flags, prev);
}

void LuxLightSphere::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	GetLuxLightSphereDesc()->EndEditParams(ip, this, flags, next);

	TimeValue t = ip->GetTime();
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
}

RefTargetHandle LuxLightSphere::Clone(RemapDir& remap)
{
	LuxLightSphere* newob = new LuxLightSphere();
	newob->enable = enable;
	newob->useLight = useLight;
	newob->ReplaceReference(PBLOCK_REF, remap.CloneRef(pblock));
	BaseClone(this, newob, remap);
	return(newob);
}

#define ON_OFF_CHUNK 0x2580
#define PARAM2_CHUNK 0x1010

IOResult LuxLightSphere::Save(ISave* isave)
{
	ULONG nb;
	IOResult res;

	isave->BeginChunk(ON_OFF_CHUNK);
	res = isave->Write(&useLight, sizeof(useLight), &nb);
	if (res != IO_OK)
		return res;
	isave->EndChunk();
	return IO_OK;
}

IOResult LuxLightSphere::Load(ILoad* iload)
{
	ULONG nb;
	IOResult res;
	enable = TRUE;
	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID())
		{
		case ON_OFF_CHUNK:
			res = iload->Read(&useLight, sizeof(useLight), &nb);
		}

		iload->CloseChunk();
		if (res != IO_OK)
			return res;
	}
	return IO_OK;
}