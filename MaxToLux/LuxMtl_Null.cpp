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

#include "main.h"
#include <IMaterialBrowserEntryInfo.h>
#include <IMtlRender_Compatibility.h>
#include <iparamb2.h>
#include <iparamm2.h>

#define LR_NULL_CLASS_ID	Class_ID(0x2d1b1f73, 0x34e27768)

#define NUM_SUBMATERIALS 1 // TODO: number of sub-materials supported by this plug-in
// Reference Indexes
// 
#define PBLOCK_REF NUM_SUBMATERIALS

class LR_Null : public Mtl {
public:
	LR_Null();
	LR_Null(BOOL loading);
	~LR_Null();


	ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp);
	void      Update(TimeValue t, Interval& valid);
	Interval  Validity(TimeValue t);
	void      Reset();

	void NotifyChanged();

	// From MtlBase and Mtl
	virtual void SetAmbient(Color c, TimeValue t);
	virtual void SetDiffuse(Color c, TimeValue t);
	virtual void SetSpecular(Color c, TimeValue t);
	virtual void SetShininess(float v, TimeValue t);
	virtual Color GetAmbient(int mtlNum = 0, BOOL backFace = FALSE);
	virtual Color GetDiffuse(int mtlNum = 0, BOOL backFace = FALSE);
	virtual Color GetSpecular(int mtlNum = 0, BOOL backFace = FALSE);
	virtual float GetXParency(int mtlNum = 0, BOOL backFace = FALSE);
	virtual float GetShininess(int mtlNum = 0, BOOL backFace = FALSE);
	virtual float GetShinStr(int mtlNum = 0, BOOL backFace = FALSE);
	virtual float WireSize(int mtlNum = 0, BOOL backFace = FALSE);


	// Shade and displacement calculation
	virtual void     Shade(ShadeContext& sc);
	virtual float    EvalDisplacement(ShadeContext& sc);
	virtual Interval DisplacementValidity(TimeValue t);

	// SubMaterial access methods
	virtual int		NumSubMtls() { return 0; }
	virtual Mtl*	GetSubMtl(int i);
	virtual void	SetSubMtl(int i, Mtl *m);
#if GET_MAX_RELEASE(VERSION_3DSMAX) < 23900
	virtual TSTR	GetSubMtlSlotName(int i) { return GetSubMtlSlotName(i, false); }
#endif
	virtual TSTR	GetSubMtlSlotName(int i, bool localized);
	virtual TSTR	GetSubMtlTVName(int i);

	// SubTexmap access methods
	virtual int     NumSubTexmaps() { return 0; }
	virtual Texmap* GetSubTexmap(int i);
	virtual void    SetSubTexmap(int i, Texmap *m);
	virtual TSTR    GetSubTexmapSlotName(int i, bool localized);
	virtual TSTR    GetSubTexmapTVName(int i);

	virtual BOOL SetDlgThing(ParamDlg* dlg);

	// Loading/Saving
	virtual IOResult Load(ILoad *iload);
	virtual IOResult Save(ISave *isave);

	// From Animatable
	virtual Class_ID ClassID() { return LR_NULL_CLASS_ID; }
	virtual SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	virtual void GetClassName(TSTR& s, bool localized) { s = GetString(IDS_CLASS_NULL); }

	virtual RefTargetHandle Clone(RemapDir &remap);
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	virtual int NumSubs() { return 1 + NUM_SUBMATERIALS; }
	virtual Animatable* SubAnim(int i);
	virtual TSTR SubAnimName(int i, bool localized);

	// TODO: Maintain the number or references here
	virtual int NumRefs() { return 1 + NUM_SUBMATERIALS; }
	virtual RefTargetHandle GetReference(int i);

	virtual int NumParamBlocks() { return 1; }					  // return number of ParamBlocks in this instance
	virtual IParamBlock2* GetParamBlock(int /*i*/) { return pblock; } // return i'th ParamBlock
	virtual IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	virtual void DeleteThis() { delete this; }

protected:
	virtual void SetReference(int i, RefTargetHandle rtarg);

private:
	Mtl*          submtl[NUM_SUBMATERIALS];  // Fixed size Reference array of sub-materials. (Indexes: 0-(N-1))
	IParamBlock2* pblock;					 // Reference that comes AFTER the sub-materials. (Index: N)

	BOOL          mapOn[NUM_SUBMATERIALS];
	float         spin;
	Interval      ivalid;
};



class LR_NullClassDesc : public ClassDesc2
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() { return TRUE; }
	virtual void* Create(BOOL loading = FALSE) { return new LR_Null(loading); }
	virtual const TCHAR *	ClassName() { return GetString(IDS_CLASS_NULL); }
	virtual const TCHAR*  NonLocalizedClassName()	{ return GetString(IDS_CLASS_NULL); }
	virtual SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	virtual Class_ID ClassID() { return LR_NULL_CLASS_ID; }
	virtual const TCHAR* Category() { return GetString(IDS_SHADERS_CATEGORY); }

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	FPInterface* GetInterface(Interface_ID id) {
		if (IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE == id) {
			return static_cast<IMaterialBrowserEntryInfo*>(this);
		}
		return ClassDesc2::GetInterface(id);
	}

	const MCHAR* GetEntryName() const { return NULL; }
	const MCHAR* GetEntryCategory() const { return _T("Materials\\lux"); }
	Bitmap* GetEntryThumbnail() const { return NULL; }
#endif

	virtual const TCHAR* InternalName() { return _T("LR_Null"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() { return hInstance; }					// returns owning module handle

};


ClassDesc2* GetLR_NullDesc() {
	static LR_NullClassDesc LR_NullDesc;
	return &LR_NullDesc;
}

enum { LR_Null_params };

static ParamBlockDesc2 LR_Null_param_blk(LR_Null_params, _T("params"), 0, GetLR_NullDesc(),
	P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF,
	//rollout
	IDD_NULL_PANEL, IDS_NULL_PARAMS, 0, 0, NULL,
	p_end
);


LR_Null::LR_Null()
	: pblock(nullptr)
{
	for (int i = 0; i < NUM_SUBMATERIALS; i++)
		submtl[i] = nullptr;
	Reset();
}

LR_Null::LR_Null(BOOL loading)
	: pblock(nullptr)
{
	for (int i = 0; i < NUM_SUBMATERIALS; i++)
		submtl[i] = nullptr;

	if (!loading)
		Reset();
}

LR_Null::~LR_Null()
{
	DeleteAllRefs();
}


void LR_Null::Reset()
{
	ivalid.SetEmpty();
	// Always have to iterate backwards when deleting references.
	for (int i = NUM_SUBMATERIALS - 1; i >= 0; i--) {
		if (submtl[i]) {
			DeleteReference(i);
			DbgAssert(submtl[i] == nullptr);
			submtl[i] = nullptr;
		}
		mapOn[i] = FALSE;
	}
	DeleteReference(PBLOCK_REF);

	GetLR_NullDesc()->MakeAutoParamBlocks(this);
}



ParamDlg* LR_Null::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLR_NullDesc()->CreateParamDlgs(hwMtlEdit, imp, this);

	// TODO: Set param block user dialog if necessary
	return masterDlg;
}

BOOL LR_Null::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

Interval LR_Null::Validity(TimeValue t)
{
	Interval valid = FOREVER;

	for (int i = 0; i < NUM_SUBMATERIALS; i++)
	{
		if (submtl[i])
			valid &= submtl[i]->Validity(t);
	}

	float u;
	//pblock->GetValue(pb_spin,t,u,valid);
	return valid;
}

/*===========================================================================*\
 |	Sub-anim & References support
\*===========================================================================*/

RefTargetHandle LR_Null::GetReference(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	else if (i == PBLOCK_REF)
		return pblock;
	else
		return nullptr;
}

void LR_Null::SetReference(int i, RefTargetHandle rtarg)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		submtl[i] = (Mtl *)rtarg;
	else if (i == PBLOCK_REF)
	{
		pblock = (IParamBlock2 *)rtarg;
	}
}

TSTR LR_Null::SubAnimName(int i, bool localized)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return GetSubMtlTVName(i);
	else
		return TSTR(_T(""));
}

Animatable* LR_Null::SubAnim(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	else if (i == PBLOCK_REF)
		return pblock;
	else
		return nullptr;
}

RefResult LR_Null::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget,
	PartID& /*partID*/, RefMessage message, BOOL /*propagate*/)
{
	switch (message) {
	case REFMSG_CHANGE:
	{
		ivalid.SetEmpty();
		if (hTarget == pblock)
		{
			ParamID changing_param = pblock->LastNotifyParamID();
			LR_Null_param_blk.InvalidateUI(changing_param);
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
			for (int i = 0; i < NUM_SUBMATERIALS; i++)
			{
				if (hTarget == submtl[i])
				{
					submtl[i] = nullptr;
					break;
				}
			}
		}
		break;
	}
	}
	return REF_SUCCEED;
}

/*===========================================================================*\
 |	SubMtl get and set
\*===========================================================================*/

Mtl* LR_Null::GetSubMtl(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	return
		nullptr;
}

void LR_Null::SetSubMtl(int i, Mtl* m)
{
	ReplaceReference(i, m);
	// TODO: Set the material and update the UI
}

TSTR LR_Null::GetSubMtlSlotName(int i, bool localized)
{
	// Return i'th sub-material name
	return _T("");
}

TSTR LR_Null::GetSubMtlTVName(int i)
{
	return GetSubMtlSlotName(i, false);
}

/*===========================================================================*\
 |	Texmap get and set
 |  By default, we support none
\*===========================================================================*/

Texmap* LR_Null::GetSubTexmap(int /*i*/)
{
	return nullptr;
}

void LR_Null::SetSubTexmap(int /*i*/, Texmap* /*m*/)
{
}

TSTR LR_Null::GetSubTexmapSlotName(int i, bool localized)
{
	return _T("");
}

TSTR LR_Null::GetSubTexmapTVName(int i)
{
	// Return i'th sub-texture name
	return GetSubTexmapSlotName(i, false);
}



/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000

IOResult LR_Null::Save(ISave* isave)
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res != IO_OK)
		return res;
	isave->EndChunk();

	return IO_OK;
}

IOResult LR_Null::Load(ILoad* iload)
{
	IOResult res;
	while (IO_OK == (res = iload->OpenChunk()))
	{
		int id = iload->CurChunkID();
		switch (id)
		{
		case MTL_HDR_CHUNK:
			res = MtlBase::Load(iload);
			break;
		}

		iload->CloseChunk();
		if (res != IO_OK)
			return res;
	}

	return IO_OK;
}


/*===========================================================================*\
 |	Updating and cloning
\*===========================================================================*/

RefTargetHandle LR_Null::Clone(RemapDir &remap)
{
	LR_Null *mnew = new LR_Null(FALSE);
	*((MtlBase*)mnew) = *((MtlBase*)this);
	// First clone the parameter block
	mnew->ReplaceReference(PBLOCK_REF, remap.CloneRef(pblock));
	// Next clone the sub-materials
	mnew->ivalid.SetEmpty();
	for (int i = 0; i < NUM_SUBMATERIALS; i++) {
		mnew->submtl[i] = nullptr;
		if (submtl[i])
			mnew->ReplaceReference(i, remap.CloneRef(submtl[i]));
		mnew->mapOn[i] = mapOn[i];
	}
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
}

void LR_Null::NotifyChanged()
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void LR_Null::Update(TimeValue t, Interval& valid)
{
	if (!ivalid.InInterval(t)) {

		ivalid.SetInfinite();
		//pblock->GetValue( mtl_mat1_on, t, mapOn[0], ivalid);
		//pblock->GetValue( pb_spin, t, spin, ivalid);

		for (int i = 0; i < NUM_SUBMATERIALS; i++) {
			if (submtl[i])
				submtl[i]->Update(t, ivalid);
		}
	}
	valid &= ivalid;
}

/*===========================================================================*\
 |	Determine the characteristics of the material
\*===========================================================================*/

void LR_Null::SetAmbient(Color /*c*/, TimeValue /*t*/) {}
void LR_Null::SetDiffuse(Color /*c*/, TimeValue /*t*/) {}
void LR_Null::SetSpecular(Color /*c*/, TimeValue /*t*/) {}
void LR_Null::SetShininess(float /*v*/, TimeValue /*t*/) {}

Color LR_Null::GetAmbient(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetAmbient(mtlNum, backFace) : Color(0, 0, 0);
}

Color LR_Null::GetDiffuse(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetDiffuse(mtlNum, backFace) : Color(0, 0, 0);
}

Color LR_Null::GetSpecular(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetSpecular(mtlNum, backFace) : Color(0, 0, 0);
}

float LR_Null::GetXParency(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetXParency(mtlNum, backFace) : 0.0f;
}

float LR_Null::GetShininess(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShininess(mtlNum, backFace) : 0.0f;
}

float LR_Null::GetShinStr(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShinStr(mtlNum, backFace) : 0.0f;
}

float LR_Null::WireSize(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->WireSize(mtlNum, backFace) : 0.0f;
}


/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void LR_Null::Shade(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	if (gbufID)
		sc.SetGBufferID(gbufID);

	if (subMaterial)
		subMaterial->Shade(sc);
	// TODO: compute the color and transparency output returned in sc.out.
}

float LR_Null::EvalDisplacement(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	return (subMaterial) ? subMaterial->EvalDisplacement(sc) : 0.0f;
}

Interval LR_Null::DisplacementValidity(TimeValue t)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;

	Interval iv;
	iv.SetInfinite();
	if (subMaterial)
		iv &= subMaterial->DisplacementValidity(t);

	return iv;
}
