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

#include <string>
#include "LuxObjects.h"
#include "target.h"
#include "decomp.h"
#include <Graphics/CustomRenderItemHandle.h>

#define LUX_LIGHT_SKY_CLASS_ID	Class_ID(0x52fc5903, 0x1d143c7f)

#define PBLOCK_REF	0

#define NUM_HALF_ARC	5
#define NUM_ARC_PTS	    (2*NUM_HALF_ARC+1)
#define SEG_INDEX		7
#define PB_OMNIDECAY   	8
#define PB_DECAY		11
#define NUM_CIRC_PTS	28

class LuxLightSky : public LightObject {
	friend class 	GeneralLightCreateCallBack;

	short 	shape;
	short 	useLight;					// on/off toggle
	static HWND 	hGeneralLight;
	static IColorSwatch 	*colorSwatch;
	static	LuxLightSky *currentEditLight;
	static short 	meshBuilt;
	static Mesh 	staticMesh;
	static short 	dlgShowCone;
	static short 	dlgShape;
	Mesh 	*mesh;
	Mesh 	skyMesh;

	RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs);
	void 	UpdateUI(TimeValue t);
	//void 	BuildStaticMeshes();
	void	GetMat(TimeValue t, INode* inode, ViewExp &vpt, Matrix3& mat);

	public:
		short 	type = TSPOT_LIGHT;
		short 	enable;
		float 	targDist;
		short 	coneDisplay;
		int 	extDispFlags;
		static HWND 	hSkyLight;

		// Parameter block
		IParamBlock2	*pblock;	//ref 0
		//ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp);
		virtual BOOL SetDlgThing(ParamDlg* dlg);

		// Loading/Saving
		IOResult Load(ILoad *iload);
		IOResult Save(ISave *isave);

		//From Animatable
		Class_ID ClassID() {return LUX_LIGHT_SKY_CLASS_ID;}
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

		LuxLightSky();// {}
		~LuxLightSky();// {}

		int 	HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);

		ObjectState 	Eval(TimeValue time);
		void 			InitNodeName(TSTR& s);
		const TCHAR * GetObjectName(bool localized);
		CreateMouseCallBack * GetCreateMouseCallBack();

		void 	SetUseLight(int onOff);
		BOOL 	GetUseLight(void) { return useLight; }
		void 	SetHotspot(TimeValue time, float f);
		float 	GetHotspot(TimeValue t, Interval& valid = Interval(0, 0));
		int 	GetSkyShape(void) { return shape; }
		void 	SetFallsize(TimeValue time, float f);
		float 	GetFallsize(TimeValue t, Interval& valid = Interval(0, 0));
		void 	SetAtten(TimeValue time, int which, float f);
		float 	GetAtten(TimeValue t, int which, Interval& valid = Interval(0, 0));
		void 	SetTDist(TimeValue time, float f);
		float 	GetTDist(TimeValue t, Interval& valid = Interval(0, 0));
		void 	SetConeDisplay(int s, int notify = TRUE);
		BOOL 	GetConeDisplay(void) { return coneDisplay; }
		void 	BuildMeshes(BOOL isnew = TRUE);
		void 	BuildSkyMesh(float coneSize);
		virtual bool UpdatePerNodeItems(
			const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
			MaxSDK::Graphics::UpdateNodeContext& nodeContext,
			MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer);
		void 	SetExtendedDisplay(int flags);
		//void 	BoxDirPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm);
		void 	BoxCircle(TimeValue t, float r, float d, Box3& box, int extraPt = 0, Matrix3 *tm = NULL);
		void 	BoxLight(TimeValue t, INode *inode, Box3& box, Matrix3 *tm = NULL);
		//void 	BoxPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm);
		void 	GetAttenPoints(TimeValue t, float rad, Point3 *q);

		virtual bool PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext);
		//int 	Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
		virtual unsigned long GetObjectDisplayRequirement() const;
		void 	GetWorldBoundBox(TimeValue t, INode *mat, ViewExp* vpt, Box3& box);
		void 	GetLocalBoundBox(TimeValue t, INode *mat, ViewExp* vpt, Box3& box);
		void 	GetConePoints(TimeValue t, float aspect, float angle, float dist, Point3 *q);
		//int 	GetCirXPoints(TimeValue t, float angle, float dist, Point3 *q);
		void 	DrawCone(TimeValue t, GraphicsWindow *gw, float dist);
		int 	DrawConeAndLine(TimeValue t, INode* inode, GraphicsWindow *gw, int drawing);
		//int 	DrawAtten(TimeValue t, INode *inode, GraphicsWindow *gw);
		void 	DrawAttenCirOrRect(TimeValue t, GraphicsWindow *gw, float dist, BOOL froze, int uicol);
		void 	DrawSkyArcs(TimeValue t, GraphicsWindow *gw, float r, Point3 *q);
		int 	GetRectXPoints(TimeValue t, float angle, float dist, Point3 *q);
		void 	DrawX(TimeValue t, float asp, int npts, float dist, GraphicsWindow *gw, int indx);

protected:
	virtual void SetReference(int, RefTargetHandle rtarg);

/*private:
	// Parameter block
	IParamBlock2 *pblock; //ref 0*/
};

void LuxLightSky::SetReference(int i, RefTargetHandle rtarg)
{
	if (i == PBLOCK_REF)
	{
		pblock = (IParamBlock2*)rtarg;
	}
}

RefTargetHandle LuxLightSky::GetReference(int i)
{
	if (i == PBLOCK_REF)
	{
		return pblock;
	}
	return nullptr;
}

class TSkyLightClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new LuxLightSky(); }
	int 			BeginCreate(Interface *i);
	int 			EndCreate(Interface *i);
	const TCHAR *	ClassName() { return GetString(IDS_TARGET_SKY_CLASS); }
	const TCHAR*	NonLocalizedClassName() { return GetString(IDS_TARGET_SKY_CLASS); }
	SClass_ID		SuperClassID() { return LIGHT_CLASS_ID; }
	Class_ID		ClassID() { return LUX_LIGHT_SKY_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }
	//void			ResetClassParams(BOOL fileReset) { if (fileReset) resetLightParams(); }
	// Class IO

	virtual const TCHAR*	InternalName() { return _T("Lux Sky"); }		// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

	BOOL			NeedsToSave() { return TRUE; }
	IOResult 		Save(ISave *isave);
	IOResult 		Load(ILoad *iload);
	DWORD			InitialRollupPageState() { return 0xfffe; }

};

static TSkyLightClassDesc tspotLightDesc;

ClassDesc2* GetTSkyLightDesc() {
	static TSkyLightClassDesc tspotLightDesc;
	return &tspotLightDesc;
}

enum { LuxLightSky_params };

enum { sphere_light_param, common_param };

//TODO: Add enums for various parameters
enum { 
	pb_light_turbidity,
	pb_light_albedo,
	pb_light_visiable_diffuse,
	pb_light_visiable_glossy,
	pb_light_visiable_specular,
	pb_light_ground_enable,
	pb_light_ground_color,
	// common
	pb_light_gain,
	pb_light_multiplier,
	pb_light_sample,
	pb_light_id,
	pb_light_importance,
};


static ParamBlockDesc2 LuxLightSky_param_blk ( LuxLightSky_params, _T("params"),  0, GetTSkyLightDesc(),
	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF, 2,
	//rollout
	sphere_light_param,	IDD_LIGHT_SKY,			IDS_LIGHT_POINT_PARAMS,		0, 0, NULL,
	common_param,		IDD_LIGHT_COMMON_PANEL, IDS_LIGHT_COMMON_PARAMS,	0, 0, NULL,
	// params
	pb_light_turbidity,				_T("turbidity"),			TYPE_FLOAT,		P_ANIMATABLE, IDS_LIGHT_POWER_SPIN,
		p_default,					2.2f,
		p_range,					0.0f, 30.0f,
		p_ui, sphere_light_param,	TYPE_SPINNER,				EDITTYPE_FLOAT, IDC_LIGHT_SKY_TURBIDITY, IDC_LIGHT_SKY_TURBIDITY_SPIN, 0.1f,
	p_end,
	pb_light_albedo,				_T("groundAlbedo"),			TYPE_RGBA,	P_ANIMATABLE, IDS_LIGHT_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, sphere_light_param,	TYPE_COLORSWATCH,			IDC_LIGHT_SKY_ALBEDO_COLOR,
	p_end,
	pb_light_visiable_diffuse,		_T("visiable diffuse"),		TYPE_BOOL, 0, IDS_LIGHT_COLOR,
		p_default, TRUE,
		p_ui, sphere_light_param,	TYPE_SINGLECHEKBOX,			IDC_LIGHT_SKY_DIFFUSE_CHECK,
	p_end,
	pb_light_visiable_glossy,		_T("visiable glossy"),		TYPE_BOOL, 0, IDS_LIGHT_COLOR,
		p_default, TRUE,
		p_ui, sphere_light_param,	TYPE_SINGLECHEKBOX,			IDC_LIGHT_SKY_GLOSSY_CHECK,
	p_end,
	pb_light_visiable_specular,		_T("visiable specular"),	TYPE_BOOL, 0, IDS_LIGHT_COLOR,
		p_default, TRUE,
		p_ui, sphere_light_param,	TYPE_SINGLECHEKBOX,			IDC_LIGHT_SKY_SPECULAR_CHECK,
	p_end,
	pb_light_ground_enable,			_T("enable ground"),		TYPE_BOOL, 0, IDS_LIGHT_COLOR,
		p_default, TRUE,
		p_ui, sphere_light_param, TYPE_SINGLECHEKBOX,			IDC_LIGHT_SKY_ENABLE_GROUND_CHECK,
	p_end,
	pb_light_ground_color,			_T("ground color"),			TYPE_RGBA, P_ANIMATABLE, IDS_LIGHT_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, sphere_light_param,	TYPE_COLORSWATCH,			IDC_LIGHT_SKY_GROUND_COLOR,
	p_end,
	// common
	pb_light_gain,				_T("gain"),			TYPE_RGBA,		P_ANIMATABLE,	IDS_LIGHT_GAIN_SPIN,
		p_default,				Color(1.0f, 1.0f, 1.0f),
		p_ui, common_param,		TYPE_COLORSWATCH,					IDC_LIGHT_GAIN,
		p_end,
	pb_light_multiplier,		_T("multiplier"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIGHT_MULTIPLIER_SPIN,
		p_default, 0.001f,
		p_range, 0.0f, 10000.0f,
		p_ui, common_param, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_LIGHT_MULTIPLAIER, IDC_LIGHT_MULTIPLAIER_SPIN, 0.001f,
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
	pb_light_importance,		_T("importance"),	TYPE_FLOAT,		P_ANIMATABLE,	IDS_LIGHT_IMPORTANCE_SPIN,
		p_default,				0.1f,
		p_range,				0.0f, 1000.0f,
		p_ui, common_param,		TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_LIGHT_IMPORTANCE, IDC_LIGHT_IMPORTANCE_SPIN, 0.01f,
		p_end,
	p_end
	);


RefResult LuxLightSky::EvalLightState(TimeValue t, Interval& valid, LightState* ls)
{
	if (useLight)
		ls->color = GetRGBColor(t, valid);
	else
		ls->color = Point3(0, 0, 0);
	return REF_SUCCEED;
}

const TCHAR *LuxLightSky::GetObjectName(bool localized)
{
	return _T("Lux Sky");
}

void LuxLightSky::InitNodeName(TSTR& s)
{
	s = _T("Lux Sky");
	return;
}

class TSkyCreationManager : public MouseCallBack, ReferenceMaker
{
private:
	CreateMouseCallBack *createCB;
	INode *lgtNode, *targNode;
	LuxLightSky *lgtObject;
	TargetObject *targObject;
	int attachedToNode;
	IObjCreate *createInterface;
	ClassDesc *cDesc;
	Matrix3 mat;  // the nodes TM relative to the CP
	IPoint2 pt0;
	int ignoreSelectionChange;
	int lastPutCount;

	void CreateNewObject();

	virtual void GetClassName(MSTR& s, bool localized) { s = _M("TSkyCreationManager"); }
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return (RefTargetHandle)lgtNode; }
	void SetReference(int i, RefTargetHandle rtarg) { lgtNode = (INode *)rtarg; }

	// StdNotifyRefChanged calls this, which can change the partID to new value 
	// If it doesnt depend on the particular message& partID, it should return
	// REF_DONTCARE
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);

public:
	void Begin(IObjCreate *ioc, ClassDesc *desc);
	void End();

	TSkyCreationManager() { ignoreSelectionChange = FALSE; }
	int proc(HWND hwnd, int msg, int point, int flag, IPoint2 m);
	BOOL SupportAutoGrid() { return TRUE; }
};

#define CID_TSKYCREATE	CID_USER + 3

class TSkyCreateMode : public CommandMode {
	TSkyCreationManager proc;
public:
	void Begin(IObjCreate *ioc, ClassDesc *desc) { proc.Begin(ioc, desc); }
	void End() { proc.End(); }

	int Class() { return CREATE_COMMAND; }
	int ID() { return CID_TSKYCREATE; }
	MouseCallBack *MouseProc(int *numPoints) { *numPoints = 1000000; return &proc; }
	ChangeForegroundCallback *ChangeFGProc() { return CHANGE_FG_SELECTED; }
	BOOL ChangeFG(CommandMode *oldMode) { return (oldMode->ChangeFGProc() != CHANGE_FG_SELECTED); }
	void EnterMode() {
	}
	void ExitMode() {
	}
	BOOL IsSticky() { return FALSE; }
};

static TSkyCreateMode theTSkyCreateMode;

void TSkyCreationManager::Begin(IObjCreate *ioc, ClassDesc *desc)
{
	createInterface = ioc;
	cDesc = desc;
	attachedToNode = FALSE;
	createCB = NULL;
	lgtNode = NULL;
	targNode = NULL;
	lgtObject = NULL;
	targObject = NULL;
	CreateNewObject();
}

void TSkyCreationManager::End()
{
	if (lgtObject) {
		lgtObject->EndEditParams((IObjParam*)createInterface, END_EDIT_REMOVEUI, NULL);
		if (!attachedToNode) {
			theHold.Suspend();
			//delete lgtObject;
			lgtObject->DeleteAllRefsFromMe();
			lgtObject->DeleteAllRefsToMe();
			lgtObject->DeleteThis();
			lgtObject = NULL;
			theHold.Resume();
			if (theHold.GetGlobalPutCount() != lastPutCount) {
				GetSystemSetting(SYSSET_CLEAR_UNDO);
			}
			macroRec->Cancel();  // JBW 4/23/99
		}
		else if (lgtNode) {
			// Get rid of the reference.
			theHold.Suspend();
			DeleteReference(0);  // sets lgtNode = NULL
			theHold.Resume();
		}
	}
}

RefResult TSkyCreationManager::NotifyRefChanged(
	const Interval& changeInt,
	RefTargetHandle hTarget,
	PartID& partID,
	RefMessage message,
	BOOL propagate)
{
	switch (message) {

	case REFMSG_PRENOTIFY_PASTE:
	case REFMSG_TARGET_SELECTIONCHANGE:
		if (ignoreSelectionChange) {
			break;
		}
		if (lgtObject && lgtNode == hTarget) {
			// this will set camNode== NULL;
			theHold.Suspend();
			DeleteReference(0);
			theHold.Resume();
			goto endEdit;
		}
		// fall through

	case REFMSG_TARGET_DELETED:
		if (lgtObject && lgtNode == hTarget) {
		endEdit:
			lgtObject->EndEditParams((IObjParam*)createInterface, 0, NULL);
			lgtObject = NULL;
			lgtNode = NULL;
			CreateNewObject();
			attachedToNode = FALSE;
		}
		else if (targNode == hTarget) {
			targNode = NULL;
			targObject = NULL;
		}
		break;
	}
	return REF_SUCCEED;
}

void TSkyCreationManager::CreateNewObject()
{
	lgtObject = (LuxLightSky *)cDesc->Create();
	lastPutCount = theHold.GetGlobalPutCount();

	macroRec->BeginCreate(cDesc);  // JBW 4/23/99

	// Start the edit params process
	if (lgtObject) {
		lgtObject->BeginEditParams((IObjParam*)createInterface, BEGIN_EDIT_CREATE, NULL);
	}
}

static BOOL needToss;

int TSkyCreationManager::proc(	HWND hwnd, int msg, int point, int flag, IPoint2 m)
{
	int res = CREATE_CONTINUE;
	TSTR targName;
	ViewExp& vpx = createInterface->GetViewExp(hwnd);
	assert(vpx.IsAlive());

	switch (msg) {
	case MOUSE_POINT:
		switch (point) {
		case 0: {
			pt0 = m;
			assert(lgtObject);
			vpx.CommitImplicitGrid(m, flag); //KENNY MERGE
			if (createInterface->SetActiveViewport(hwnd)) {
				return FALSE;
			}

			if (createInterface->IsCPEdgeOnInView()) {
				res = FALSE;
				goto done;
			}

			// if lights were hidden by category, re-display them
			GetCOREInterface()->SetHideByCategoryFlags(
				GetCOREInterface()->GetHideByCategoryFlags() & ~HIDE_LIGHTS);

			if (attachedToNode) {
				// send this one on its way
				lgtObject->EndEditParams((IObjParam*)createInterface, 0, NULL);
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

			needToss = theHold.GetGlobalPutCount() != lastPutCount;

			theHold.Begin();	 // begin hold for undo
			mat.IdentityMatrix();

			// link it up
			INode *lightNode = createInterface->CreateObjectNode(lgtObject);
			attachedToNode = TRUE;
			assert(lightNode);
			createCB = lgtObject->GetCreateMouseCallBack();
			createInterface->SelectNode(lightNode);

			// Create target object and node
			targObject = new TargetObject;
			assert(targObject);
			targNode = createInterface->CreateObjectNode(targObject);
			assert(targNode);
			targName = lightNode->GetName();
			targName += GetString(IDS_DOT_TARGET);
			macroRec->Disable();
			targNode->SetName(targName);
			macroRec->Enable();

			// hook up camera to target using lookat controller.
			createInterface->BindToTarget(lightNode, targNode);

			// Reference the new node so we'll get notifications.
			theHold.Suspend();
			ReplaceReference(0, lightNode);
			theHold.Resume();

			// Position camera and target at first point then drag.
			mat.IdentityMatrix();
			//mat[3] = vpx.GetPointOnCP(m);
			mat.SetTrans(vpx.SnapPoint(m, m, NULL, SNAP_IN_3D));
			createInterface->SetNodeTMRelConstPlane(lgtNode, mat);
			createInterface->SetNodeTMRelConstPlane(targNode, mat);
			lgtObject->Enable(1);

			ignoreSelectionChange = TRUE;
			createInterface->SelectNode(targNode, 0);
			ignoreSelectionChange = FALSE;
			res = TRUE;

			// 6/19/01 11:37am --MQM-- 
			// set the wire-color of the light to be the default
			// color (yellow)
			if (lgtNode)
			{
				Point3 color = GetUIColor(COLOR_LIGHT_OBJ);
				lgtNode->SetWireColor(RGB(color.x*255.0f, color.y*255.0f, color.z*255.0f));
			}
			break;
		}

		case 1:
			if (Length(m - pt0) < 2)
				goto abort;
			//mat[3] = vpx.GetPointOnCP(m);
			mat.SetTrans(vpx.SnapPoint(m, m, NULL, SNAP_IN_3D));
			macroRec->Disable();   // JBW 4/23/99
			createInterface->SetNodeTMRelConstPlane(targNode, mat);
			macroRec->Enable();

			ignoreSelectionChange = TRUE;
			createInterface->SelectNode(lgtNode);
			ignoreSelectionChange = FALSE;

			theHold.Accept(IDS_LIGHT_CREATE);

			createInterface->AddLightToScene(lgtNode);
			createInterface->RedrawViews(createInterface->GetTime());

			res = FALSE;	// We're done
			break;
		}
		break;

	case MOUSE_MOVE:
		//mat[3] = vpx.GetPointOnCP(m);
		mat.SetTrans(vpx.SnapPoint(m, m, NULL, SNAP_IN_3D));
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
		vpx.SnapPreview(m, m, NULL, SNAP_IN_3D);
		vpx.TrackImplicitGrid(m); //KENNY MERGE
		break;

	case MOUSE_PROPCLICK:
		// right click while between creations
		createInterface->RemoveMode(NULL);
		break;

	case MOUSE_ABORT:
	abort:
		assert(lgtObject);
		lgtObject->EndEditParams((IObjParam*)createInterface, 0, NULL);
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
	if ((res == CREATE_STOP) || (res == CREATE_ABORT))
		vpx.ReleaseImplicitGrid();

	return res;
}

#define I_EXEC_GET_DELEGATING_CLASSDESC	0x11002

int TSkyLightClassDesc::BeginCreate(Interface *i)
{
	SuspendSetKeyMode();
	IObjCreate *iob = i->GetIObjCreate();

	//iob->SetMouseProc( new LACamCreationManager(iob,this), 1000000 );

	ClassDesc* delegatingCD;			// JBW 11.1.99, allows fully functional extending scripted lights
										// by causing the custom command mode to create the delegating class
	i->Execute(I_EXEC_GET_DELEGATING_CLASSDESC, (ULONG_PTR)&delegatingCD);
	theTSkyCreateMode.Begin(iob, (delegatingCD ? delegatingCD : this));

	iob->PushCommandMode(&theTSkyCreateMode);

	return TRUE;
}


int TSkyLightClassDesc::EndCreate(Interface *i)
{
	ResumeSetKeyMode();
	theTSkyCreateMode.End();
	i->RemoveMode(&theTSkyCreateMode);
	macroRec->EmitScript();  // JBW 4/23/99

	return TRUE;
}

inline float MaxF(float a, float b) { return a > b ? a : b; }
inline float MinF(float a, float b) { return a < b ? a : b; }

void LuxLightSky::DrawCone(TimeValue t, GraphicsWindow *gw, float dist)
{
	Point3 q[NUM_CIRC_PTS + 1], u[3];
	int i;
	int dispDecay = 0;
	int dirLight = 0;

	GetConePoints(t, GetSkyShape() ? -1.0f : GetAspect(t), GetHotspot(t), dist, q);

	gw->setColor(LINE_COLOR, GetUIColor(COLOR_HOTSPOT));
	
	if (GetHotspot(t) >= GetFallsize(t)) {
		// draw (far) hotspot circle
		u[0] = q[0];
		u[1] = q[NUM_CIRC_PTS];
		gw->polyline(2, u, NULL, NULL, FALSE, NULL);
	}
	gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
	if (dirLight) {
		// draw 4 axial hotspot lines
		for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
			u[0] = q[i]; 	u[1] = q[i]; u[1].z += dist;
			gw->polyline(2, u, NULL, NULL, FALSE, NULL);
		}
		GetConePoints(t, -1.0f, GetHotspot(t), 0.0f, q);
		// draw (near) hotspot circle
		gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
	}
	else 
	{
		// draw 4 axial lines
		u[0] = Point3(0, 0, 0);
		for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
			u[1] = q[i];
			gw->polyline(2, u, NULL, NULL, FALSE, NULL);
		}
	}

	GetConePoints(t, -1.0f, GetFallsize(t), dist, q);
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_FALLOFF));
	if (GetHotspot(t) < GetFallsize(t)) {
		// draw (far) fallsize circle
		u[0] = q[0];	u[1] = q[NUM_CIRC_PTS];
		gw->polyline(2, u, NULL, NULL, FALSE, NULL);
		u[0] = Point3(0, 0, 0);
	}
	gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
	if (dirLight) 
	{
		float dfar = q[0].z;
		float dnear = 0.0f;
		// draw axial fallsize lines
		for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
			u[0] = q[i];  u[0].z = dfar;	u[1] = q[i]; u[1].z = dnear;
			gw->polyline(2, u, NULL, NULL, FALSE, NULL);
		}
		GetConePoints(t, -1.0f, GetFallsize(t), 0.0f, q);
		// draw (near) fallsize circle
		gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
	}
}

int LuxLightSky::GetRectXPoints(TimeValue t, float angle, float dist, Point3 *q) {
	int i;
	if (dist == 0.0f) dist = .00001f;
	float ang = DegToRad(angle) / 2.0f;
	float da, sn, cs, x, y, z, a;
	float aspect = GetAspect(t);
	float w = dist * (float)tan(ang) * (float)sqrt((double)aspect);
	float h = w / aspect;
	float wang = (float)atan(w / dist);
	float hang = (float)atan(h / dist);
	float aw = float(atan(w / dist)*cos(hang));  // half-angle of top and bottom arcs
	float ah = float(atan(h / dist)*cos(wang));  // half-angle of left and right arcs
	int j = 0;

	// draw horizontal and vertical center lines
	da = wang / float(NUM_HALF_ARC);
	for (i = -NUM_HALF_ARC, a = -wang; i <= NUM_HALF_ARC; i++, a += da)
		q[j++] = Point3(dist*(float)sin(a), 0.0f, -dist * (float)cos(a));
	da = hang / float(NUM_HALF_ARC);
	for (i = -NUM_HALF_ARC, a = -hang; i <= NUM_HALF_ARC; i++, a += da)
		q[j++] = Point3(0.0f, dist*(float)sin(a), -dist * (float)cos(a));


	// draw top and bottom arcs
	da = aw / float(NUM_HALF_ARC);
	sn = (float)sin(hang);
	cs = (float)cos(hang);
	for (i = -NUM_HALF_ARC, a = -aw; i <= NUM_HALF_ARC; i++, a += da) {
		x = dist * (float)sin(a);
		z = -dist * (float)cos(a);
		q[j] = Point3(x, z*sn, z*cs);
		q[j + NUM_ARC_PTS] = Point3(x, -z * sn, z*cs);
		j++;
	}

	j += NUM_ARC_PTS;

	// draw left and right arcs
	da = ah / float(NUM_HALF_ARC);
	sn = (float)sin(wang);
	cs = (float)cos(wang);
	for (i = -NUM_HALF_ARC, a = -ah; i <= NUM_HALF_ARC; i++, a += da) {
		y = dist * (float)sin(a);
		z = -dist * (float)cos(a);
		q[j] = Point3(z*sn, y, z*cs);
		q[j + NUM_ARC_PTS] = Point3(-z * sn, y, z*cs);
		j++;
	}

	return 6 * NUM_ARC_PTS;
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

int LuxLightSky::DrawConeAndLine(TimeValue t, INode* inode, GraphicsWindow *gw, int drawing)
{
	Matrix3 tm = inode->GetObjectTM(t);
	gw->setTransform(tm);
	gw->clearHitCode();
	Point3 pt, v[3];
	if (GetTargetPoint(t, inode, pt)) {
		float den = FLength(tm.GetRow(2));
		float dist = (den != 0) ? FLength(tm.GetTrans() - pt) / den : 0.0f;
		targDist = dist;
		if (hSkyLight && (currentEditLight == this)) { // LAM - 8/13/02 - defect 511609
			const TCHAR *buf = FormatUniverseValue(targDist);
			//SetWindowText(GetDlgItem(hGeneralLight, IDC_TARG_DISTANCE), buf);
		}
		if ((drawing != -1) && (coneDisplay || (extDispFlags & EXT_DISP_ONLY_SELECTED)))
			DrawCone(t, gw, dist);
		if (!inode->IsFrozen() && !inode->Dependent())
		{
			Color color(inode->GetWireColor());
			if (color != GetUIColor(COLOR_LIGHT_OBJ))
				gw->setColor(LINE_COLOR, color);
			else
				gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE)); // old method
		}
		v[0] = Point3(0, 0, 0);
		v[1] = Point3(0.0f, 0.0f, (drawing == -1) ? (-0.9f * dist) : -dist);
		gw->polyline(2, v, NULL, NULL, FALSE, NULL);
	}
	return gw->checkHitCode();
}

void LuxLightSky::GetConePoints(TimeValue t, float aspect, float angle, float dist, Point3 *q)
{
	float ta = (float)tan(0.5*DegToRad(angle));
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

/*int LuxLightSky::GetCirXPoints(TimeValue t, float angle, float dist, Point3 *q) {
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
}*/

HWND LuxLightSky::hSkyLight = NULL;

class LightConeItemSky : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
	LuxLightSky* mpLight;
public:
	LightConeItemSky(LuxLightSky* lt)
		: mpLight(lt)
	{

	}
	~LightConeItemSky()
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
		BuildConeAndLine(drawContext.GetTime(), drawContext.GetCurrentNode());
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

		mpLight->GetConePoints(t, mpLight->GetSkyShape() ? -1.0f : mpLight->GetAspect(t), mpLight->GetHotspot(t), dist, posArray);
		Color color(GetUIColor(COLOR_HOTSPOT));

		if (mpLight->GetSkyShape()) {
			// CIRCULAR
			if (mpLight->GetHotspot(t) >= mpLight->GetFallsize(t)) {
				// draw (far) hotspot circle
				tmpArray[0] = posArray[0];
				tmpArray[1] = posArray[NUM_CIRC_PTS];
				AddLineStrip(tmpArray, color, 2, false, false);
			}
			AddLineStrip(posArray, color, NUM_CIRC_PTS, true, false);
			// draw 4 axial hotspot lines
			tmpArray[0] = Point3(0, 0, 0);
			for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
				tmpArray[1] = posArray[i];
				AddLineStrip(tmpArray, color, 2, false, false);
			}
			mpLight->GetConePoints(t, -1.0f, mpLight->GetHotspot(t), 0.0f, posArray);
			AddLineStrip(posArray, color, NUM_CIRC_PTS, true, false);

			mpLight->GetConePoints(t, -1.0f, mpLight->GetFallsize(t), dist, posArray);
			color = GetUIColor(COLOR_FALLOFF);
			if (mpLight->GetHotspot(t) < mpLight->GetFallsize(t)) {
				// draw (far) fallsize circle
				tmpArray[0] = posArray[0];
				tmpArray[1] = posArray[NUM_CIRC_PTS];
				AddLineStrip(tmpArray, color, 2, false, false);
				tmpArray[0] = Point3(0, 0, 0);
			}
			AddLineStrip(posArray, color, NUM_CIRC_PTS, true, false);

			float cs = (float)cos(DegToRad(mpLight->GetFallsize(t)*0.5f));
			float dfar = posArray[0].z;
			tmpArray[0] = Point3(0, 0, 0);
			for (i = 0; i < NUM_CIRC_PTS; i += SEG_INDEX) {
				//Give a default Point3 instead of a division by zero
				tmpArray[1] = (dist != 0.0f ? -posArray[i] * dfar / dist : Point3(0.0f, 0.0f, 0.0f));
				AddLineStrip(tmpArray, color, 2, false, false);
			}
		}
	}

	void BuildConeAndLine(TimeValue t, INode* inode)
	{
		/*if (nullptr == inode
			|| !mpLight->IsSky())
			return;*/
		Matrix3 tm = inode->GetObjectTM(t);
		Point3 pt;
		if (GetTargetPoint(t, inode, pt)) {
			float den = FLength(tm.GetRow(2));
			float dist = (den != 0) ? FLength(tm.GetTrans() - pt) / den : 0.0f;
			mpLight->targDist = dist;
			if (mpLight->hSkyLight) { // LAM - 8/13/02 - defect 511609
				const TCHAR *buf = FormatUniverseValue(mpLight->targDist);
				//SetWindowText(GetDlgItem(mpLight->hGeneralLight, IDC_TARG_DISTANCE), buf);
			}
			if (mpLight->coneDisplay || (mpLight->extDispFlags & EXT_DISP_ONLY_SELECTED))
				BuildCone(t, dist);
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
		for (int i = 0; i < 6; i++)
		{
			AddLineStrip(&pos[i*NUM_ARC_PTS], color, NUM_ARC_PTS, false, true);
		}
	}

	/*void BuildCircleX(TimeValue t, float angle, float dist, Point3* pos, Color& color) {
		mpLight->GetCirXPoints(t, angle, dist, pos);
		AddLineStrip(pos, color, NUM_CIRC_PTS, true, true); // circle 
		AddLineStrip(&pos[NUM_CIRC_PTS], color, NUM_ARC_PTS, false, true); // vert arc
		AddLineStrip(&pos[NUM_CIRC_PTS + NUM_ARC_PTS], color, NUM_ARC_PTS, false, true);  // horiz arc
	}*/

	void BuildAttenCirOrRect(TimeValue t, float dist, BOOL froze, int uicol) {
		Color color = froze ? GetFreezeColor() : GetUIColor(uicol);
		int npts, indx;
		float asp;
		if (mpLight->GetSkyShape())
		{
			npts = NUM_CIRC_PTS;
			asp = -1.0f;
			indx = SEG_INDEX;
		}
		else
		{
			npts = 4;
			asp = mpLight->GetAspect(t);
			indx = 1;
		}
		BuildX(t, asp, npts, dist, indx, color);
	}
};


class LightTargetLineItem : public MaxSDK::Graphics::Utilities::SplineRenderItem
{
	LuxLightSky* mpLight;
	float mLastDist;
	Color mLastColor;
public:
	LightTargetLineItem(LuxLightSky* lt)
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

		/*if (nullptr == inode
			|| !mpLight->IsSky())
			return;*/

		mpLight->SetExtendedDisplay(drawContext.GetExtendedDisplayMode());
		inode->SetTargetNodePair(0);
		TimeValue t = drawContext.GetTime();
		Matrix3 tm = inode->GetObjectTM(t);
		if (mpLight->type == TSPOT_LIGHT
			|| mpLight->type == TDIR_LIGHT) {
			Point3 pt;
			if (GetTargetPoint(t, inode, pt)) {
				float den = FLength(tm.GetRow(2));
				float dist = (den != 0) ? FLength(tm.GetTrans() - pt) / den : 0.0f;
				mpLight->targDist = dist;
				Color color(inode->GetWireColor());
				if (!inode->IsFrozen() && !inode->Dependent())
				{
					if (color == GetUIColor(COLOR_LIGHT_OBJ))
						color = GetUIColor(COLOR_TARGET_LINE);
				}
				if (mLastColor != color
					|| mLastDist != dist)
				{
					ClearLines();
					Point3 posArray[2] = { Point3(0,0,0), Point3(0.0f, 0.0f, -dist) };
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
		if (nullptr == node)
		{
			node->SetTargetNodePair(1);
		}
	}
};

void LuxLightSky::GetAttenPoints(TimeValue t, float rad, Point3 *q)
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

void LuxLightSky::DrawSkyArcs(TimeValue t, GraphicsWindow *gw, float r, Point3 *q) {
	GetAttenPoints(t, r, q);
	gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
	gw->polyline(NUM_CIRC_PTS, q + NUM_CIRC_PTS, NULL, NULL, TRUE, NULL);
	gw->polyline(NUM_CIRC_PTS, q + 2 * NUM_CIRC_PTS, NULL, NULL, TRUE, NULL);
}

void LuxLightSky::DrawX(TimeValue t, float asp, int npts, float dist, GraphicsWindow *gw, int indx) {
	Point3 q[3 * NUM_CIRC_PTS + 1];
	Point3 u[2];
	GetConePoints(t, asp, GetFallsize(t), dist, q);
	gw->polyline(npts, q, NULL, NULL, TRUE, NULL);
	u[0] = q[0]; u[1] = q[2 * indx];
	gw->polyline(2, u, NULL, NULL, FALSE, NULL);
	u[0] = q[indx]; u[1] = q[3 * indx];
	gw->polyline(2, u, NULL, NULL, FALSE, NULL);
}

void LuxLightSky::DrawAttenCirOrRect(TimeValue t, GraphicsWindow *gw, float dist, BOOL froze, int uicol) {
	if (!froze) gw->setColor(LINE_COLOR, GetUIColor(uicol));
	{
		int npts, indx;
		float asp;
		if (GetSkyShape()) { npts = NUM_CIRC_PTS; 	asp = -1.0f; 	indx = SEG_INDEX; }
		else { npts = 4;  	asp = GetAspect(t); 	indx = 1; }
		DrawX(t, asp, npts, dist, gw, indx);
	}
}

void LuxLightSky::BoxCircle(TimeValue t, float r, float d, Box3& box, int extraPt, Matrix3 *tm) {
	Point3 q[3 * NUM_CIRC_PTS];
	int npts;
	float asp;
	if (GetSkyShape()) { npts = NUM_CIRC_PTS + extraPt; 	asp = -1.0f; }
	else { npts = 4 + extraPt;  asp = GetAspect(t); }
	GetConePoints(t, asp, r, d, q);
	npts = 4 + extraPt;  asp = GetAspect(t);
	box.IncludePoints(q, npts, tm);
}

/*void LuxLightSky::BoxDirPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm) {
	int npts;
	Point3 q[3 * NUM_CIRC_PTS];
	//npts = GetSkyShape() ? GetCirXPoints(t, angle, dist, q) : GetRectXPoints(t, angle, dist, q);
	//box.IncludePoints(q, npts, tm);
}*/


/*void LuxLightSky::BoxPoints(TimeValue t, float angle, float dist, Box3 &box, Matrix3 *tm) {
	BoxCircle(t, angle, dist, box, 0, tm);
}*/

void LuxLightSky::BoxLight(TimeValue t, INode *inode, Box3& box, Matrix3 *tm) {
	coneDisplay = 1;
	Point3 pt;
	float d = GetTDist(t);
	//R6 change 
	/*if (GetTargetPoint(t, inode, pt)) {
		Point3 loc = inode->GetObjectTM(t).GetTrans();
		d = FLength(loc - pt) / FLength(inode->GetObjectTM(t).GetRow(2));
		box += tm ? (*tm)*Point3(0.0f, 0.0f, -d) : Point3(0.0f, 0.0f, -d);
	}
	else {
		if (coneDisplay)
			box += tm ? (*tm)*Point3(0.0f, 0.0f, -d) : Point3(0.0f, 0.0f, -d);
	}*/
	Point3 loc = inode->GetObjectTM(t).GetTrans();
	d = FLength(loc - pt) / FLength(inode->GetObjectTM(t).GetRow(2));
	box += tm ? (*tm)*Point3(0.0f, 0.0f, -d / 4) : Point3(0.0f, 0.0f, -d / 4);
	float rad = 90;//MaxF(GetHotspot(t), GetFallsize(t));
	BoxCircle(t, rad, d, box, 1, tm);

	/*int dispDecay = 0;

	if (dispDecay)
	{
		Point3 q[3 * NUM_CIRC_PTS];
		float rad = 1.5; //0;
		GetAttenPoints(t, rad, q);
		box.IncludePoints(q, 3 * NUM_CIRC_PTS, tm);
	}*/
}

void LuxLightSky::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
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

	box.Translate(Point3(0.0f, 0.0f, -box.pmax.z));

	//this adds the target point, cone, etc.
	BoxLight(t, inode, box, NULL);
}

void LuxLightSky::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
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

short LuxLightSky::meshBuilt = 0;
short LuxLightSky::dlgShowCone = 1;
short LuxLightSky::dlgShape = CIRCLE_LIGHT;

void LuxLightSky::BuildMeshes(BOOL isnew) {
	coneDisplay = 1;//dlgShowCone;
	shape = dlgShape;
	//BuildStaticMeshes();
	BuildSkyMesh(GetHotspot(TimeValue(0)));
	mesh = &skyMesh;
}

#define FZ (0.0f)

void LuxLightSky::BuildSkyMesh(float coneSize)
{
	// build a cone
	if (coneSize < 0.0f)
		return;
	int nverts = 49;
	int nfaces = 94;
	skyMesh.setNumVerts(nverts);
	skyMesh.setNumFaces(nfaces);
	
	skyMesh.setVert(0, Point3(0.0, 0.0, 13.8273));					skyMesh.setVert(1, Point3(-3.02205e-07, 6.91364, 11.9748));	skyMesh.setVert(2, Point3(-3.45682, 5.98739, 11.9748));		skyMesh.setVert(3, Point3(-5.98739, 3.45682, 11.9748));
	skyMesh.setVert(4, Point3(-6.91364, 1.04393e-06, 11.9748));	skyMesh.setVert(5, Point3(-5.98739, -3.45682, 11.9748));		skyMesh.setVert(6, Point3(-3.45682, -5.98739, 11.9748));		skyMesh.setVert(7, Point3(-3.21424e-06, -6.91364, 11.9748));
	skyMesh.setVert(8, Point3(3.45682, -5.98739, 11.9748));		skyMesh.setVert(9, Point3(5.98738, -3.45682, 11.9748));		skyMesh.setVert(10, Point3(6.91364, -5.38454e-06, 11.9748));	skyMesh.setVert(11, Point3(5.98739, 3.45681, 11.9748));
	skyMesh.setVert(12, Point3(3.45683, 5.98738, 11.9748));		skyMesh.setVert(13, Point3(-5.23434e-07, 11.9748, 6.91364));	skyMesh.setVert(14, Point3(-5.98739, 10.3705, 6.91364));		skyMesh.setVert(15, Point3(-10.3705, 5.98739, 6.91364));
	skyMesh.setVert(16, Point3(-11.9748, 1.80814e-06, 6.91364));	skyMesh.setVert(17, Point3(-10.3705, -5.98738, 6.91364));		skyMesh.setVert(18, Point3(-5.98739, -10.3705, 6.91364));		skyMesh.setVert(19, Point3(-5.56722e-06, -11.9748, 6.91364));
	skyMesh.setVert(20, Point3(5.98738, -10.3705, 6.91364));		skyMesh.setVert(21, Point3(10.3705, -5.98739, 6.91364));		skyMesh.setVert(22, Point3(11.9748, -9.3263e-06, 6.91364));	skyMesh.setVert(23, Point3(10.3705, 5.98738, 6.91364));
	skyMesh.setVert(24, Point3(5.9874, 10.3705, 6.91364));			skyMesh.setVert(25, Point3(-6.04409e-07, 13.8273, -6.04409e-07)); skyMesh.setVert(26, Point3(-6.91364, 11.9748, -6.04409e-07));	skyMesh.setVert(27, Point3(-11.9748, 6.91364, -6.04409e-07));
	skyMesh.setVert(28, Point3(-13.8273, 2.08786e-06, -6.04409e-07));	skyMesh.setVert(29, Point3(-11.9748, -6.91364, -6.04409e-07)); skyMesh.setVert(30, Point3(-6.91364, -11.9748, -6.04409e-07));	skyMesh.setVert(31, Point3(-6.42847e-06, -13.8273, -6.04409e-07));
	skyMesh.setVert(32, Point3(6.91363, -11.9748, -6.04409e-07));	skyMesh.setVert(33, Point3(11.9748, -6.91365, -6.04409e-07));	skyMesh.setVert(34, Point3(13.8273, -1.07691e-05, -6.04409e-07));	skyMesh.setVert(35, Point3(11.9748, 6.91363, -6.04409e-07));
	skyMesh.setVert(36, Point3(6.91365, 11.9748, -6.04409e-07));	skyMesh.setVert(37, Point3(-6.04409e-07, 13.8273, -6.04409e-07)); skyMesh.setVert(38, Point3(-6.91364, 11.9748, -6.04409e-07));	skyMesh.setVert(39, Point3(-11.9748, 6.91364, -6.04409e-07));
	skyMesh.setVert(40, Point3(-13.8273, 2.08786e-06, -6.04409e-07));	skyMesh.setVert(41, Point3(-11.9748, -6.91364, -6.04409e-07)); skyMesh.setVert(42, Point3(-6.91364, -11.9748, -6.04409e-07));	skyMesh.setVert(43, Point3(-6.42847e-06, -13.8273, -6.04409e-07));
	skyMesh.setVert(44, Point3(6.91363, -11.9748, -6.04409e-07));	skyMesh.setVert(45, Point3(11.9748, -6.91365, -6.04409e-07));	skyMesh.setVert(46, Point3(13.8273, -1.07691e-05, -6.04409e-07));	skyMesh.setVert(47, Point3(11.9748, 6.91363, -6.04409e-07));
	skyMesh.setVert(48, Point3(6.91365, 11.9748, -6.04409e-07));	skyMesh.faces[0].setVerts(0, 1, 2);
	skyMesh.faces[1].setVerts(0, 2, 3);		skyMesh.faces[2].setVerts(0, 3, 4);		skyMesh.faces[3].setVerts(0, 4, 5);		skyMesh.faces[4].setVerts(0, 5, 6);
	skyMesh.faces[5].setVerts(0, 6, 7);		skyMesh.faces[6].setVerts(0, 7, 8);		skyMesh.faces[7].setVerts(0, 8, 9);		skyMesh.faces[8].setVerts(0, 9, 10);
	skyMesh.faces[9].setVerts(0, 10, 11);	skyMesh.faces[10].setVerts(0, 11, 12);	skyMesh.faces[11].setVerts(0, 12, 1);	skyMesh.faces[12].setVerts(14, 2, 1);
	skyMesh.faces[13].setVerts(1, 13, 14);	skyMesh.faces[14].setVerts(15, 3, 2);	skyMesh.faces[15].setVerts(2, 14, 15);	skyMesh.faces[16].setVerts(16, 4, 3);
	skyMesh.faces[17].setVerts(3, 15, 16);	skyMesh.faces[18].setVerts(17, 5, 4);	skyMesh.faces[19].setVerts(4, 16, 17);	skyMesh.faces[20].setVerts(18, 6, 5);
	skyMesh.faces[21].setVerts(5, 17, 18);	skyMesh.faces[22].setVerts(19, 7, 6);	skyMesh.faces[23].setVerts(6, 18, 19);	skyMesh.faces[24].setVerts(20, 8, 7);
	skyMesh.faces[25].setVerts(7, 19, 20);	skyMesh.faces[26].setVerts(21, 9, 8);	skyMesh.faces[27].setVerts(8, 20, 21);	skyMesh.faces[28].setVerts(22, 10, 9);
	skyMesh.faces[29].setVerts(9, 21, 22);	skyMesh.faces[30].setVerts(23, 11, 10);	skyMesh.faces[31].setVerts(10, 22, 23);	skyMesh.faces[32].setVerts(24, 12, 11);
	skyMesh.faces[33].setVerts(11, 23, 24);	skyMesh.faces[34].setVerts(13, 1, 12);	skyMesh.faces[35].setVerts(12, 24, 13);	skyMesh.faces[36].setVerts(26, 14, 13);
	skyMesh.faces[37].setVerts(13, 25, 26);	skyMesh.faces[38].setVerts(27, 15, 14);	skyMesh.faces[39].setVerts(14, 26, 27);	skyMesh.faces[40].setVerts(28, 16, 15);
	skyMesh.faces[41].setVerts(15, 27, 28);	skyMesh.faces[42].setVerts(29, 17, 16);	skyMesh.faces[43].setVerts(16, 28, 29);	skyMesh.faces[44].setVerts(30, 18, 17);
	skyMesh.faces[45].setVerts(17, 29, 30);	skyMesh.faces[46].setVerts(31, 19, 18);	skyMesh.faces[47].setVerts(18, 30, 31);	skyMesh.faces[48].setVerts(32, 20, 19);
	skyMesh.faces[49].setVerts(19, 31, 32);	skyMesh.faces[50].setVerts(33, 21, 20);	skyMesh.faces[51].setVerts(20, 32, 33);	skyMesh.faces[52].setVerts(34, 22, 21);
	skyMesh.faces[53].setVerts(21, 33, 34);	skyMesh.faces[54].setVerts(35, 23, 22);	skyMesh.faces[55].setVerts(22, 34, 35);	skyMesh.faces[56].setVerts(36, 24, 23);
	skyMesh.faces[57].setVerts(23, 35, 36);	skyMesh.faces[58].setVerts(25, 13, 24);	skyMesh.faces[59].setVerts(24, 36, 25);	skyMesh.faces[60].setVerts(38, 26, 25);
	skyMesh.faces[61].setVerts(25, 37, 38);	skyMesh.faces[62].setVerts(39, 27, 26);	skyMesh.faces[63].setVerts(26, 38, 39);	skyMesh.faces[64].setVerts(40, 28, 27);
	skyMesh.faces[65].setVerts(27, 39, 40);	skyMesh.faces[66].setVerts(41, 29, 28);	skyMesh.faces[67].setVerts(28, 40, 41);	skyMesh.faces[68].setVerts(42, 30, 29);
	skyMesh.faces[69].setVerts(29, 41, 42);	skyMesh.faces[70].setVerts(43, 31, 30);	skyMesh.faces[71].setVerts(30, 42, 43);	skyMesh.faces[72].setVerts(44, 32, 31);
	skyMesh.faces[73].setVerts(31, 43, 44);	skyMesh.faces[74].setVerts(45, 33, 32);	skyMesh.faces[75].setVerts(32, 44, 45);	skyMesh.faces[76].setVerts(46, 34, 33);
	skyMesh.faces[77].setVerts(33, 45, 46);	skyMesh.faces[78].setVerts(47, 35, 34);	skyMesh.faces[79].setVerts(34, 46, 47);	skyMesh.faces[80].setVerts(48, 36, 35);
	skyMesh.faces[81].setVerts(35, 47, 48);	skyMesh.faces[82].setVerts(37, 25, 36);	skyMesh.faces[83].setVerts(36, 48, 37);	skyMesh.faces[84].setVerts(48, 47, 46);
	skyMesh.faces[85].setVerts(46, 45, 44);	skyMesh.faces[86].setVerts(44, 43, 42);	skyMesh.faces[87].setVerts(46, 44, 42);	skyMesh.faces[88].setVerts(42, 41, 40);
	skyMesh.faces[89].setVerts(40, 39, 38);	skyMesh.faces[90].setVerts(42, 40, 38);	skyMesh.faces[91].setVerts(46, 42, 38);	skyMesh.faces[92].setVerts(48, 46, 38);
	skyMesh.faces[93].setVerts(37, 48, 38);

	for (int i = 0; i < nfaces; i++) {
		skyMesh.faces[i].setSmGroup(i);
		skyMesh.faces[i].setEdgeVisFlags(1, 1, 1);
	}
	skyMesh.buildNormals();
	skyMesh.EnableEdgeList(1);
	skyMesh.InvalidateGeomCache();
}

void LuxLightSky::SetExtendedDisplay(int flags)
{
	extDispFlags = flags;
}

MaxSDK::Graphics::Utilities::MeshEdgeKey LightSkyMeshKey;
MaxSDK::Graphics::Utilities::SplineItemKey LightSkySplineKey;

unsigned long LuxLightSky::GetObjectDisplayRequirement() const
{
	return 0;
}

bool LuxLightSky::PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& displayContext)
{
	LightSkyMeshKey.SetFixedSize(true);
	return true;
}

bool LuxLightSky::UpdatePerNodeItems(
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
	data.Key = &LightSkyMeshKey;
	meshHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(meshHandle);

	MaxSDK::Graphics::Utilities::SplineRenderItem* pLineItem = new LightConeItemSky(this);
	//MaxSDK::Graphics::CustomRenderItemHandle coneHandle;
	//coneHandle.Initialize();
	//coneHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	//coneHandle.SetCustomImplementation(pLineItem);
	//data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
	//data.Key = &LightSkySplineKey;
	//coneHandle.SetConsolidationData(data);
	//targetRenderItemContainer.AddRenderItem(coneHandle);

	pLineItem = new LightTargetLineItem(this);
	MaxSDK::Graphics::CustomRenderItemHandle lineHandle;
	lineHandle.Initialize();
	lineHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
	lineHandle.SetCustomImplementation(pLineItem);
	data.Strategy = &MaxSDK::Graphics::Utilities::SplineConsolidationStrategy::GetInstance();
	data.Key = &LightSkySplineKey;
	lineHandle.SetConsolidationData(data);
	targetRenderItemContainer.AddRenderItem(lineHandle);

	return true;
}

/*ParamDlg* LuxLightSky::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLuxLightSkyDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
}*/

BOOL LuxLightSky::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

LuxLightSky * LuxLightSky::currentEditLight = NULL;

void LuxLightSky::SetUseLight(int onOff)
{
	//SimpleLightUndo< &GeneralLight::SetUseLight, false >::Hold(this, onOff, useLight);
	useLight = TRUE;//onOff;
	/*if (currentEditLight == this)  // LAM - 8/13/02 - defect 511609
		UpdateUICheckbox(hGeneralLight, IDC_LIGHT_ON, _T("enabled"), onOff); // 5/15/01 11:00am --MQM-- maxscript fix*/
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

void LuxLightSky::SetHotspot(TimeValue t, float f)
{

	return;
	//pblock->SetValue(PB_HOTSIZE, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSky::GetHotspot(TimeValue t, Interval& valid)
{
	return 30.0f;
}

void LuxLightSky::SetFallsize(TimeValue t, float f)
{
	return;
	//pblock->SetValue(PB_FALLSIZE, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSky::GetFallsize(TimeValue t, Interval& valid)
{
	return 50.0f;
}

void LuxLightSky::SetAtten(TimeValue t, int which, float f)
{
	return;
	//pblock->SetValue((type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1) + which, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSky::GetAtten(TimeValue t, int which, Interval& valid)
{
	return 40.0f;
	//float f;
	//pblock->GetValue((type == OMNI_LIGHT ? PB_OMNIATSTART1 : PB_ATTENSTART1) + which, t, f, valid);
	//return f;
}

void LuxLightSky::SetTDist(TimeValue t, float f)
{
	return;
	//pblock->SetValue(PB_TDIST, t, f);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

float LuxLightSky::GetTDist(TimeValue t, Interval& valid)
{
	return 60.0f;
	//float f;
	//pblock->GetValue(PB_TDIST, t, f, valid);
	//return f;
}

void LuxLightSky::SetConeDisplay(int s, int notify)
{
	//DualLightUndo< &GeneralLight::SetConeDisplay, TRUE, false >::Hold(this, s, coneDisplay);
	coneDisplay = s;
	if (notify)
		NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
}

LuxLightSky::LuxLightSky()// : pblock(nullptr)
{
	enable = TRUE;
	useLight = TRUE;
	coneDisplay = 1;
	GetTSkyLightDesc()->MakeAutoParamBlocks(this);
	BuildMeshes();
	//CheckUIConsistency();

	//RegisterNotification(&NotifyDist, (void *)this, NOTIFY_UNITS_CHANGE);
}

LuxLightSky::~LuxLightSky()
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

void LuxLightSky::GetMat(TimeValue t, INode* inode, ViewExp &vpt, Matrix3& tm)
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

int LuxLightSky::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
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
	std::string tmp = std::to_string(res);
	OutputDebugStringW(L"\n==============================================");
	OutputDebugStringA(tmp.c_str());
	OutputDebugStringW(L"\n");
	//res = 2;
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

class LightSkyCreateCallBack : public CreateMouseCallBack
{
	LuxLightSky *ob;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(LuxLightSky *obj) { ob = obj; }
};

int LightSkyCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
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

static LightSkyCreateCallBack sGeneralLgtCreateCB;

RefResult LuxLightSky::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
{
	switch (message) {
	case REFMSG_CHANGE:
	{
		//ivalid.SetEmpty();
		//mapValid.SetEmpty();
		if (hTarget == pblock)
		{
			ParamID changing_param = pblock->LastNotifyParamID();
			LuxLightSky_param_blk.InvalidateUI(changing_param);
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

CreateMouseCallBack* LuxLightSky::GetCreateMouseCallBack()
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

HWND LuxLightSky::hGeneralLight = NULL;
IColorSwatch *LuxLightSky::colorSwatch;

void LuxLightSky::UpdateUI(TimeValue t)
{
	Point3 color;

	if (hGeneralLight && 
		GetWindowLongPtr(hGeneralLight, GWLP_USERDATA) == (LONG_PTR)this && pblock) {
		color = GetRGBColor(t);
		colorSwatch->SetColor(RGB(FLto255i(color.x), FLto255i(color.y), FLto255i(color.z)));

		//UpdateColBrackets(t);

		float hot = 2.0;
		float fall = 10;

		BuildSkyMesh(max(hot, fall));

	} 

}

ObjectState LuxLightSky::Eval(TimeValue time)
{
	UpdateUI(time);
	return ObjectState(this);
}

void LuxLightSky::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);

	GetTSkyLightDesc()->BeginEditParams(ip, this, flags, prev);
}

void LuxLightSky::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	GetTSkyLightDesc()->EndEditParams(ip, this, flags, next);

	TimeValue t = ip->GetTime();
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
}

RefTargetHandle LuxLightSky::Clone(RemapDir& remap)
{
	LuxLightSky* newob = new LuxLightSky();
	newob->enable = enable;
	newob->useLight = useLight;
	newob->ReplaceReference(PBLOCK_REF, remap.CloneRef(pblock));
	BaseClone(this, newob, remap);
	return(newob);
}

#define ON_OFF_CHUNK 0x2580
#define PARAM2_CHUNK 0x1010

IOResult LuxLightSky::Save(ISave* isave)
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

IOResult LuxLightSky::Load(ILoad* iload)
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

#define PLUG_SHADOW_TYPE_CHUNK 5000

IOResult TSkyLightClassDesc::Save(ISave *isave) {
	isave->BeginChunk(PLUG_SHADOW_TYPE_CHUNK);
	isave->EndChunk();
	return IO_OK;
}

IOResult TSkyLightClassDesc::Load(ILoad *iload) {
	ULONG nb;
	IOResult res;
	int hasPlugShadows = 0;
	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case PLUG_SHADOW_TYPE_CHUNK:
			hasPlugShadows = TRUE;
			break;

		}
		iload->CloseChunk();
		if (res != IO_OK)
			return res;
	}

	return IO_OK;
}