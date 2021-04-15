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

#define LUX_LIGHT_POINT_CLASS_ID	Class_ID(0x80e11330, 0x6b78e9d4)

#define PBLOCK_REF	0

#define NUM_CIRC_PTS	28

class LuxLightPoint : public LightObject {
	friend class 	GeneralLightCreateCallBack;

	short 	type;
	short 	useLight;					// on/off toggle
	short 	coneDisplay;
	int 	extDispFlags;
	static HWND 	hGeneralLight;
	static IColorSwatch 	*colorSwatch;
	static	LuxLightPoint *currentEditLight;
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
		Class_ID ClassID() {return LUX_LIGHT_POINT_CLASS_ID;}
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

		LuxLightPoint();// {}
		~LuxLightPoint();// {}

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
		int 	Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
		virtual unsigned long GetObjectDisplayRequirement() const;
		void 	GetWorldBoundBox(TimeValue t, INode *mat, ViewExp* vpt, Box3& box);
		void 	GetLocalBoundBox(TimeValue t, INode *mat, ViewExp* vpt, Box3& box);

protected:
	virtual void SetReference(int, RefTargetHandle rtarg);

/*private:
	// Parameter block
	IParamBlock2 *pblock; //ref 0*/
};

void LuxLightPoint::SetReference(int i, RefTargetHandle rtarg)
{
	if (i == PBLOCK_REF)
	{
		pblock = (IParamBlock2*)rtarg;
	}
}

RefTargetHandle LuxLightPoint::GetReference(int i)
{
	if (i == PBLOCK_REF)
	{
		return pblock;
	}
	return nullptr;
}

class LuxLightPointClassDesc : public ClassDesc2
{
public:
	virtual int				IsPublic() 						{ return TRUE; }
	virtual void*			Create(BOOL loading = FALSE) 	{ return new LuxLightPoint(); }
	//int 					BeginCreate(Interface *i);
	//int 					EndCreate(Interface *i);
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	const TCHAR*	NonLocalizedClassName() { return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID		SuperClassID() 			{ return LIGHT_CLASS_ID; }
	virtual Class_ID		ClassID() 				{ return LUX_LIGHT_POINT_CLASS_ID; }
	virtual const TCHAR*	Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR*	InternalName() 			{ return _T("Lux Point"); }		// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE		HInstance() 			{ return hInstance; }			// returns owning module handle
	

};


ClassDesc2* GetLuxLightPointDesc() { 
	static LuxLightPointClassDesc LuxLightPointDesc;
	return &LuxLightPointDesc;
}


enum { LuxLightPoint_params };

enum { point_light_param, common_param };

//TODO: Add enums for various parameters
enum { 
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


static ParamBlockDesc2 LuxLightPoint_param_blk ( LuxLightPoint_params, _T("params"),  0, GetLuxLightPointDesc(),
	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF, 2,
	//rollout
	point_light_param,	IDD_LIGHT_POINT,		IDS_LIGHT_POINT_PARAMS, 0, 0, NULL,
	common_param,		IDD_LIGHT_COMMON_PANEL, IDS_LIGHT_COMMON_PARAMS, 0, 0, NULL,
	// params
	pb_light_color,					_T("color"),		TYPE_RGBA,		P_ANIMATABLE, IDS_LIGHT_COLOR,
		p_default,					Color(1.0f, 1.0f, 1.0f),
		p_ui, point_light_param,	TYPE_COLORSWATCH,					IDC_LIGHT_POINT_COLOR,
	p_end,
	pb_light_power,					_T("power"),		TYPE_FLOAT,		P_ANIMATABLE, IDS_LIGHT_POWER_SPIN,
		p_default,					150.0f,
		p_range,					0.0f, 10000.0f,
		p_ui, point_light_param,	TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_POINT_POWER, IDC_LIGHT_POINT_POWER_SPIN, 1.0f,
	p_end,
	pb_light_efficency,				_T("efficency"),	TYPE_FLOAT,		P_ANIMATABLE, IDS_LIGHT_POWER_SPIN,
		p_default,					1.0f,
		p_range,					0.0f, 10000.0f,
		p_ui, point_light_param,	TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_POINT_EFFICENCY, IDC_LIGHT_POINT_EFFICENCY_SPIN, 0.1f,
	p_end,
	// common
	pb_light_gain,					_T("gain"),			TYPE_RGBA,		P_ANIMATABLE,	IDS_LIGHT_GAIN_SPIN,
		p_default,					Color(1.0f, 1.0f, 1.0f),
		p_ui, common_param,			TYPE_COLORSWATCH,					IDC_LIGHT_GAIN,
		p_end,
	pb_light_multiplier,			_T("multiplier"),	TYPE_FLOAT,		P_ANIMATABLE,	IDS_LIGHT_MULTIPLIER_SPIN,
		p_default, 0.0f,
		p_range, 0.0f, 10000.0f,
		p_ui, common_param,			TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_MULTIPLAIER, IDC_LIGHT_MULTIPLAIER_SPIN, 1.0f,
	p_end,
	pb_light_sample, 				_T("sample"), 		TYPE_INT, 		P_ANIMATABLE, 	IDS_LIGHT_SAMPLE_SPIN,
		p_default, 					-1, 
		p_range, 					-1,1000, 
		p_ui, common_param,			TYPE_SPINNER,		EDITTYPE_INT,	IDC_LIGHT_SAMPLE, IDC_LIGHT_SAMPLE_SPIN, 1,
		p_end,
	pb_light_id,					_T("id"),			TYPE_INT,		P_ANIMATABLE,	IDS_LIGHT_SAMPLE_SPIN,
		p_default,					12345,
		p_range,					1, 99999,
		p_ui, common_param,			TYPE_SPINNER,		EDITTYPE_INT,	IDC_LIGHT_ID,	IDC_LIGHT_ID_SPIN, 1,
		p_end,
	pb_light_importance,			_T("importance"),	TYPE_FLOAT,		P_ANIMATABLE,	IDS_LIGHT_IMPORTANCE_SPIN,
		p_default,					1.0f,
		p_range,					0.0f, 1000.0f,
		p_ui, common_param,			TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_IMPORTANCE, IDC_LIGHT_IMPORTANCE_SPIN, 0.01f,
		p_end,
	p_end
	);


RefResult LuxLightPoint::EvalLightState(TimeValue t, Interval& valid, LightState* ls)
{
	if (useLight)
		ls->color = GetRGBColor(t, valid);
	else
		ls->color = Point3(0, 0, 0);
	return REF_SUCCEED;
}

const TCHAR *LuxLightPoint::GetObjectName(bool localized)
{
	return _T("Lux Point");
}

void LuxLightPoint::InitNodeName(TSTR& s)
{
	s = _T("Lux Point");
	return;
}

void LuxLightPoint::BoxCircle(TimeValue t, float r, float d, Box3& box, int extraPt, Matrix3 *tm) {
	Point3 q[3 * NUM_CIRC_PTS];
	int npts;
	float asp;
	//if (GetSpotShape()) { npts = NUM_CIRC_PTS + extraPt; 	asp = -1.0f; }
	//else { npts = 4 + extraPt;  asp = GetAspect(t); }
	//GetConePoints(t, asp, r, d, q);
	npts = 4 + extraPt;  asp = GetAspect(t);
	box.IncludePoints(q, npts, tm);
}

void LuxLightPoint::BoxDirPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm) {
	int npts;
	Point3 q[3 * NUM_CIRC_PTS];
	//npts = GetSpotShape() ? GetCirXPoints(t, angle, dist, q) : GetRectXPoints(t, angle, dist, q);
	//box.IncludePoints(q, npts, tm);
}


void LuxLightPoint::BoxPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm) {
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

void LuxLightPoint::GetAttenPoints(TimeValue t, float rad, Point3 *q)
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

inline float MaxF(float a, float b) { return a > b ? a : b; }

void LuxLightPoint::BoxLight(TimeValue t, INode *inode, Box3& box, Matrix3 *tm) {
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

void LuxLightPoint::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
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

void LuxLightPoint::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
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

short LuxLightPoint::meshBuilt = 0;
Mesh LuxLightPoint::staticMesh;

void LuxLightPoint::BuildStaticMeshes()
{
	if (!meshBuilt) {
		int nverts = 86;
		int nfaces = 132;

		// Build a leetle octahedron
		staticMesh.setNumVerts(nverts);
		staticMesh.setNumFaces(nfaces);
		float s = 8.0f;
		staticMesh.setVert(0, Point3(0.00806236, 0.000859662, 15.9491));	staticMesh.setVert(1, Point3(6.12905, 0.000857808, 14.254));		staticMesh.setVert(2, Point3(8.97655, 0.000857435, 8.77149));		staticMesh.setVert(3, Point3(7.28636, 0.000858223, 3.33781));
		staticMesh.setVert(4, Point3(3.45486, 0.000859141, -0.449286));		staticMesh.setVert(5, Point3(3.17457, 0.000859236, -4.63154));		staticMesh.setVert(6, Point3(0.00806236, 0.00085922, -6.63699));	staticMesh.setVert(7, Point3(3.06856, 5.30179, 14.254));
		staticMesh.setVert(8, Point3(4.49231, 7.7678, 8.77149));			staticMesh.setVert(9, Point3(3.64721, 6.30405, 3.33781));			staticMesh.setVert(10, Point3(1.73146, 2.98587, -0.449286));		staticMesh.setVert(11, Point3(1.59132, 2.74314, -4.63154));
		staticMesh.setVert(12, Point3(-3.05243, 5.30179, 14.254));			staticMesh.setVert(13, Point3(-4.47618, 7.7678, 8.77149));			staticMesh.setVert(14, Point3(-3.63109, 6.30405, 3.33781));			staticMesh.setVert(15, Point3(-1.71534, 2.98587, -0.449286));
		staticMesh.setVert(16, Point3(-1.57519, 2.74313, -4.63154));		staticMesh.setVert(17, Point3(-6.11293, 0.000858178, 14.254));		staticMesh.setVert(18, Point3(-8.96043, 0.000858302, 8.77149));		staticMesh.setVert(19, Point3(-7.27023, 0.000857661, 3.33781));
		staticMesh.setVert(20, Point3(-3.43873, 0.000857078, -0.449286));	staticMesh.setVert(21, Point3(-3.15845, 0.000857008, -4.63154));	staticMesh.setVert(22, Point3(-3.05243, -5.30007, 14.254));			staticMesh.setVert(23, Point3(-4.47618, -7.76608, 8.77149));
		staticMesh.setVert(24, Point3(-3.63108, -6.30233, 3.33781));		staticMesh.setVert(25, Point3(-1.71533, -2.98415, -0.449286));		staticMesh.setVert(26, Point3(-1.57519, -2.74142, -4.63154));		staticMesh.setVert(27, Point3(3.06856, -5.30007, 14.254));
		staticMesh.setVert(28, Point3(4.49231, -7.76608, 8.77149));			staticMesh.setVert(29, Point3(3.64721, -6.30233, 3.33781));			staticMesh.setVert(30, Point3(1.73146, -2.98415, -0.449286));		staticMesh.setVert(31, Point3(1.59132, -2.74142, -4.63154));
		staticMesh.setVert(32, Point3(13.0697, 0.000858357, 6.68856));		staticMesh.setVert(33, Point3(13.0697, -0.212142, 7.05748));		staticMesh.setVert(34, Point3(13.0697, 0.213859, 7.05748));			staticMesh.setVert(35, Point3(18.9557, 0.00085748, 6.68856));
		staticMesh.setVert(36, Point3(18.9557, -0.212143, 7.05749));		staticMesh.setVert(37, Point3(18.9557, 0.213858, 7.05749));			staticMesh.setVert(38, Point3(-9.94317, 0.000858043, 17.3542));		staticMesh.setVert(39, Point3(-10.204, -0.212143, 17.0934));
		staticMesh.setVert(40, Point3(-10.204, 0.213859, 17.0934));			staticMesh.setVert(41, Point3(-14.1052, 0.000855859, 21.5163));		staticMesh.setVert(42, Point3(-14.3661, -0.212145, 21.2554));		staticMesh.setVert(43, Point3(-14.3661, 0.213856, 21.2554));
		staticMesh.setVert(44, Point3(0.233654, -9.72076, 17.1548));		staticMesh.setVert(45, Point3(-0.135273, -9.87137, 17.0042));		staticMesh.setVert(46, Point3(-0.135273, -9.57014, 17.3054));		staticMesh.setVert(47, Point3(0.233654, -13.8828, 21.3168));
		staticMesh.setVert(48, Point3(-0.135273, -14.0334, 21.1662));		staticMesh.setVert(49, Point3(-0.135273, -13.7322, 21.4674));		staticMesh.setVert(50, Point3(0.233654, -12.6152, 6.93451));		staticMesh.setVert(51, Point3(-0.135273, -12.6152, 6.72151));
		staticMesh.setVert(52, Point3(-0.135273, -12.6152, 7.14751));		staticMesh.setVert(53, Point3(0.233654, -18.5012, 6.9345));			staticMesh.setVert(54, Point3(-0.135273, -18.5012, 6.7215));		staticMesh.setVert(55, Point3(-0.135273, -18.5012, 7.1475));
		staticMesh.setVert(56, Point3(-13.3847, 0.000858086, 7.18046));		staticMesh.setVert(57, Point3(-13.3847, -0.212143, 6.81153));		staticMesh.setVert(58, Point3(-13.3847, 0.213859, 6.81153));		staticMesh.setVert(59, Point3(-19.2707, 0.00085793, 7.18046));
		staticMesh.setVert(60, Point3(-19.2707, -0.212143, 6.81153));		staticMesh.setVert(61, Point3(-19.2707, 0.213858, 6.81153));		staticMesh.setVert(62, Point3(0.233654, 10.054, 17.4559));			staticMesh.setVert(63, Point3(-0.135273, 9.91709, 17.619));
		staticMesh.setVert(64, Point3(-0.135273, 10.1909, 17.2927));		staticMesh.setVert(65, Point3(0.233654, 14.563, 21.2393));			staticMesh.setVert(66, Point3(-0.135273, 14.426, 21.4025));			staticMesh.setVert(67, Point3(-0.135273, 14.6999, 21.0762));
		staticMesh.setVert(68, Point3(0.233654, 13.3943, 6.9345));			staticMesh.setVert(69, Point3(-0.135273, 13.3943, 7.14751));		staticMesh.setVert(70, Point3(-0.135273, 13.3943, 6.7215));			staticMesh.setVert(71, Point3(0.233654, 19.2803, 6.9345));
		staticMesh.setVert(72, Point3(-0.135273, 19.2803, 7.14751));		staticMesh.setVert(73, Point3(-0.135273, 19.2803, 6.7215));			staticMesh.setVert(74, Point3(9.95905, 0.000859429, 17.2913));		staticMesh.setVert(75, Point3(9.69818, -0.212141, 17.5522));
		staticMesh.setVert(76, Point3(9.69818, 0.21386, 17.5522));			staticMesh.setVert(77, Point3(14.1211, 0.000858103, 21.4534));		staticMesh.setVert(78, Point3(13.8602, -0.212142, 21.7143));		staticMesh.setVert(79, Point3(13.8602, 0.213859, 21.7143));
		staticMesh.setVert(80, Point3(0.233654, 0.000858297, 20.543));		staticMesh.setVert(81, Point3(-0.135273, -0.212142, 20.543));		staticMesh.setVert(82, Point3(-0.135273, 0.213859, 20.543));		staticMesh.setVert(83, Point3(0.233654, 0.000857338, 26.429));
		staticMesh.setVert(84, Point3(-0.135273, -0.212143, 26.429));		staticMesh.setVert(85, Point3(-0.135273, 0.213858, 26.429));
		staticMesh.faces[0].setVerts(0, 1, 7);			staticMesh.faces[1].setVerts(1, 2, 8);			staticMesh.faces[2].setVerts(1, 8, 7);			staticMesh.faces[3].setVerts(2, 3, 9);	
		staticMesh.faces[4].setVerts(2, 9, 8);			staticMesh.faces[5].setVerts(3, 4, 10);			staticMesh.faces[6].setVerts(3, 10, 9);			staticMesh.faces[7].setVerts(4, 5, 11);	
		staticMesh.faces[8].setVerts(4, 11, 10);		staticMesh.faces[9].setVerts(5, 6, 11);			staticMesh.faces[10].setVerts(0, 7, 12);		staticMesh.faces[11].setVerts(7, 8, 13);
		staticMesh.faces[12].setVerts(7, 13, 12);		staticMesh.faces[13].setVerts(8, 9, 14);		staticMesh.faces[14].setVerts(8, 14, 13);		staticMesh.faces[15].setVerts(9, 10, 15);
		staticMesh.faces[16].setVerts(9, 15, 14);		staticMesh.faces[17].setVerts(10, 11, 16);		staticMesh.faces[18].setVerts(10, 16, 15);		staticMesh.faces[19].setVerts(11, 6, 16);
		staticMesh.faces[20].setVerts(0, 12, 17);		staticMesh.faces[21].setVerts(12, 13, 18);		staticMesh.faces[22].setVerts(12, 18, 17);		staticMesh.faces[23].setVerts(13, 14, 19);
		staticMesh.faces[24].setVerts(13, 19, 18);		staticMesh.faces[25].setVerts(14, 15, 20);		staticMesh.faces[26].setVerts(14, 20, 19);		staticMesh.faces[27].setVerts(15, 16, 21);
		staticMesh.faces[28].setVerts(15, 21, 20);		staticMesh.faces[29].setVerts(16, 6, 21);		staticMesh.faces[30].setVerts(0, 17, 22);		staticMesh.faces[31].setVerts(17, 18, 23);
		staticMesh.faces[32].setVerts(17, 23, 22);		staticMesh.faces[33].setVerts(18, 19, 24);		staticMesh.faces[34].setVerts(18, 24, 23);		staticMesh.faces[35].setVerts(19, 20, 25);
		staticMesh.faces[36].setVerts(19, 25, 24);		staticMesh.faces[37].setVerts(20, 21, 26);		staticMesh.faces[38].setVerts(20, 26, 25);		staticMesh.faces[39].setVerts(21, 6, 26);
		staticMesh.faces[40].setVerts(0, 22, 27);		staticMesh.faces[41].setVerts(22, 23, 28);		staticMesh.faces[42].setVerts(22, 28, 27);		staticMesh.faces[43].setVerts(23, 24, 29);
		staticMesh.faces[44].setVerts(23, 29, 28);		staticMesh.faces[45].setVerts(24, 25, 30);		staticMesh.faces[46].setVerts(24, 30, 29);		staticMesh.faces[47].setVerts(25, 26, 31);
		staticMesh.faces[48].setVerts(25, 31, 30);		staticMesh.faces[49].setVerts(26, 6, 31);		staticMesh.faces[50].setVerts(0, 27, 1);		staticMesh.faces[51].setVerts(27, 28, 2);
		staticMesh.faces[52].setVerts(27, 2, 1);		staticMesh.faces[53].setVerts(28, 29, 3);		staticMesh.faces[54].setVerts(28, 3, 2);		staticMesh.faces[55].setVerts(29, 30, 4);
		staticMesh.faces[56].setVerts(29, 4, 3);		staticMesh.faces[57].setVerts(30, 31, 5);		staticMesh.faces[58].setVerts(30, 5, 4);		staticMesh.faces[59].setVerts(31, 6, 5);
		staticMesh.faces[60].setVerts(32, 35, 36);		staticMesh.faces[61].setVerts(36, 33, 32);		staticMesh.faces[62].setVerts(33, 36, 37);		staticMesh.faces[63].setVerts(37, 34, 33);
		staticMesh.faces[64].setVerts(34, 37, 35);		staticMesh.faces[65].setVerts(35, 32, 34);		staticMesh.faces[66].setVerts(33, 34, 32);		staticMesh.faces[67].setVerts(37, 36, 35);
		staticMesh.faces[68].setVerts(38, 41, 42);		staticMesh.faces[69].setVerts(42, 39, 38);		staticMesh.faces[70].setVerts(39, 42, 43);		staticMesh.faces[71].setVerts(43, 40, 39);
		staticMesh.faces[72].setVerts(40, 43, 41);		staticMesh.faces[73].setVerts(41, 38, 40);		staticMesh.faces[74].setVerts(39, 40, 38);		staticMesh.faces[75].setVerts(43, 42, 41);
		staticMesh.faces[76].setVerts(44, 47, 48);		staticMesh.faces[77].setVerts(48, 45, 44);		staticMesh.faces[78].setVerts(45, 48, 49);		staticMesh.faces[79].setVerts(49, 46, 45);
		staticMesh.faces[80].setVerts(46, 49, 47);		staticMesh.faces[81].setVerts(47, 44, 46);		staticMesh.faces[82].setVerts(45, 46, 44);		staticMesh.faces[83].setVerts(49, 48, 47);
		staticMesh.faces[84].setVerts(50, 53, 54);		staticMesh.faces[85].setVerts(54, 51, 50);		staticMesh.faces[86].setVerts(51, 54, 55);		staticMesh.faces[87].setVerts(55, 52, 51);
		staticMesh.faces[88].setVerts(52, 55, 53);		staticMesh.faces[89].setVerts(53, 50, 52);		staticMesh.faces[90].setVerts(51, 52, 50);		staticMesh.faces[91].setVerts(55, 54, 53);
		staticMesh.faces[92].setVerts(56, 59, 60);		staticMesh.faces[93].setVerts(60, 57, 56);		staticMesh.faces[94].setVerts(57, 60, 61);		staticMesh.faces[95].setVerts(61, 58, 57);
		staticMesh.faces[96].setVerts(58, 61, 59);		staticMesh.faces[97].setVerts(59, 56, 58);		staticMesh.faces[98].setVerts(57, 58, 56);		staticMesh.faces[99].setVerts(61, 60, 59);
		staticMesh.faces[100].setVerts(62, 65, 66);		staticMesh.faces[101].setVerts(66, 63, 62);		staticMesh.faces[102].setVerts(63, 66, 67);		staticMesh.faces[103].setVerts(67, 64, 63);
		staticMesh.faces[104].setVerts(64, 67, 65);		staticMesh.faces[105].setVerts(65, 62, 64);		staticMesh.faces[106].setVerts(63, 64, 62);		staticMesh.faces[107].setVerts(67, 66, 65);
		staticMesh.faces[108].setVerts(68, 71, 72);		staticMesh.faces[109].setVerts(72, 69, 68);		staticMesh.faces[110].setVerts(69, 72, 73);		staticMesh.faces[111].setVerts(73, 70, 69);
		staticMesh.faces[112].setVerts(70, 73, 71);		staticMesh.faces[113].setVerts(71, 68, 70);		staticMesh.faces[114].setVerts(69, 70, 68);		staticMesh.faces[115].setVerts(73, 72, 71);
		staticMesh.faces[116].setVerts(74, 77, 78);		staticMesh.faces[117].setVerts(78, 75, 74);		staticMesh.faces[118].setVerts(75, 78, 79);		staticMesh.faces[119].setVerts(79, 76, 75);
		staticMesh.faces[120].setVerts(76, 79, 77);		staticMesh.faces[121].setVerts(77, 74, 76);		staticMesh.faces[122].setVerts(75, 76, 74);		staticMesh.faces[123].setVerts(79, 78, 77);
		staticMesh.faces[124].setVerts(80, 83, 84);		staticMesh.faces[125].setVerts(84, 81, 80);		staticMesh.faces[126].setVerts(81, 84, 85);		staticMesh.faces[127].setVerts(85, 82, 81);
		staticMesh.faces[128].setVerts(82, 85, 83);		staticMesh.faces[129].setVerts(83, 80, 82);		staticMesh.faces[130].setVerts(81, 82, 80);		staticMesh.faces[131].setVerts(85, 84, 83);
		for (int i = 0; i < nfaces; i++) {
			staticMesh.faces[i].setSmGroup(i);
			staticMesh.faces[i].setEdgeVisFlags(1, 1, 1);
		}
		staticMesh.buildNormals();
		staticMesh.EnableEdgeList(1);
		staticMesh.InvalidateGeomCache();

		// Build an "arrow"
		meshBuilt = 1;
	}
}

void LuxLightPoint::BuildMeshes(BOOL isnew) {
	BuildStaticMeshes();
	mesh = &staticMesh;
}

void LuxLightPoint::SetExtendedDisplay(int flags)
{
	extDispFlags = flags;
}

MaxSDK::Graphics::Utilities::MeshEdgeKey LightPointMeshKey;
MaxSDK::Graphics::Utilities::SplineItemKey LightPointSplineKey;

unsigned long LuxLightPoint::GetObjectDisplayRequirement() const
{
	return 0;
}

bool LuxLightPoint::PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& displayContext)
{
	LightPointMeshKey.SetFixedSize(true);
	return true;
}

bool LuxLightPoint::UpdatePerNodeItems(
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
	data.Key = &LightPointMeshKey;
	meshHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(meshHandle);

	/*MaxSDK::Graphics::Utilities::SplineRenderItem* pLineItem = new LightConeItem(this);
	MaxSDK::Graphics::CustomRenderItemHandle coneHandle;
	coneHandle.Initialize();
	coneHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	coneHandle.SetCustomImplementation(pLineItem);
	data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
	data.Key = &LightSplineKey;
	coneHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(coneHandle);*/

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

/*ParamDlg* LuxLightPoint::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLuxLightPointDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
}*/

BOOL LuxLightPoint::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

LuxLightPoint * LuxLightPoint::currentEditLight = NULL;

void LuxLightPoint::SetUseLight(int onOff)
{
	//SimpleLightUndo< &GeneralLight::SetUseLight, false >::Hold(this, onOff, useLight);
	useLight = TRUE;//onOff;
	/*if (currentEditLight == this)  // LAM - 8/13/02 - defect 511609
		UpdateUICheckbox(hGeneralLight, IDC_LIGHT_ON, _T("enabled"), onOff); // 5/15/01 11:00am --MQM-- maxscript fix*/
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

void LuxLightPoint::SetHotspot(TimeValue t, float f)
{

	return;
	//pblock->SetValue(PB_HOTSIZE, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightPoint::GetHotspot(TimeValue t, Interval& valid)
{
	return -1.0f;
}

void LuxLightPoint::SetFallsize(TimeValue t, float f)
{
	return;
	//pblock->SetValue(PB_FALLSIZE, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightPoint::GetFallsize(TimeValue t, Interval& valid)
{
	return -1.0f;
}

void LuxLightPoint::SetAtten(TimeValue t, int which, float f)
{
	return;
	//pblock->SetValue((type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1) + which, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightPoint::GetAtten(TimeValue t, int which, Interval& valid)
{
	return -1.0f;
	//float f;
	//pblock->GetValue((type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1) + which, t, f, valid);
	//return f;
}

void LuxLightPoint::SetTDist(TimeValue t, float f)
{
	return;
	//pblock->SetValue(PB_TDIST, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightPoint::GetTDist(TimeValue t, Interval& valid)
{
	return -1.0f;
	//float f;
	//pblock->GetValue(PB_TDIST, t, f, valid);
	//return f;
}

void LuxLightPoint::SetConeDisplay(int s, int notify)
{
	//DualLightUndo< &GeneralLight::SetConeDisplay, TRUE, false >::Hold(this, s, coneDisplay);
	coneDisplay = s;
	if (notify)
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

LuxLightPoint::LuxLightPoint()// : pblock(nullptr)
{
	enable = FALSE;
	useLight = TRUE;
	GetLuxLightPointDesc()->MakeAutoParamBlocks(this);
	BuildMeshes();
	//CheckUIConsistency();

	//RegisterNotification(&NotifyDist, (void *)this, NOTIFY_UNITS_CHANGE);
}

LuxLightPoint::~LuxLightPoint()
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

void LuxLightPoint::GetMat(TimeValue t, INode* inode, ViewExp &vpt, Matrix3& tm)
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

int LuxLightPoint::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
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
		//res = DrawConeAndLine(t, inode, gw, -1);
		if (res != 0)
			inode->SetTargetNodePair(1);
	}
	gw->setRndLimits(savedLimits);
	return res;
}

int LuxLightPoint::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
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
	//DrawConeAndLine(t, inode, gw, 1);
	//DrawAtten(t, inode, gw);
	gw->setRndLimits(rlim);
	return 0;
}

class LightPointCreateCallBack : public CreateMouseCallBack
{
	LuxLightPoint *ob;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(LuxLightPoint *obj) { ob = obj; }
};

int LightPointCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
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

static LightPointCreateCallBack sGeneralLgtCreateCB;

RefResult LuxLightPoint::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
{
	switch (message) {
	case REFMSG_CHANGE:
	{
		//ivalid.SetEmpty();
		//mapValid.SetEmpty();
		if (hTarget == pblock)
		{
			ParamID changing_param = pblock->LastNotifyParamID();
			LuxLightPoint_param_blk.InvalidateUI(changing_param);
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

CreateMouseCallBack* LuxLightPoint::GetCreateMouseCallBack()
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

HWND LuxLightPoint::hGeneralLight = NULL;
IColorSwatch *LuxLightPoint::colorSwatch;

void LuxLightPoint::UpdateUI(TimeValue t)
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

ObjectState LuxLightPoint::Eval(TimeValue time)
{
	UpdateUI(time);
	return ObjectState(this);
}

void LuxLightPoint::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);

	GetLuxLightPointDesc()->BeginEditParams(ip, this, flags, prev);
}

void LuxLightPoint::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	GetLuxLightPointDesc()->EndEditParams(ip, this, flags, next);

	TimeValue t = ip->GetTime();
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
}

RefTargetHandle LuxLightPoint::Clone(RemapDir& remap)
{
	LuxLightPoint* newob = new LuxLightPoint();
	newob->enable = enable;
	newob->useLight = useLight;
	newob->ReplaceReference(PBLOCK_REF, remap.CloneRef(pblock));
	BaseClone(this, newob, remap);
	return(newob);
}

#define ON_OFF_CHUNK 0x2580
#define PARAM2_CHUNK 0x1010

IOResult LuxLightPoint::Save(ISave* isave)
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

IOResult LuxLightPoint::Load(ILoad* iload)
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