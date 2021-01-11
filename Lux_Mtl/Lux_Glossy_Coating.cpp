/***************************************************************************
 * Copyright 2019-2020 by author Omid Ghotbi "TAO" omidt.gh@gmail.com      *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
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

//#include <string>
#include "Lux_Materal.h"
//#include <maxscript\maxscript.h>

#define LUX_GLOSSY_COATIN_CLASS_ID	Class_ID(0x24a95711, 0x752c3b14)

#define PBLOCK_REF 1
#define NUM_SUBMATERIALS 1
#define NUM_SUBTEXTURES 5
#define NUM_REF NUM_SUBTEXTURES + NUM_SUBMATERIALS + PBLOCK_REF // number of refrences supported by this plug-in

static int seed = rand() % 15400 + 14400;

class Lux_GlossyCoating : public Mtl {
public:
	Lux_GlossyCoating();
	Lux_GlossyCoating(BOOL loading);
	~Lux_GlossyCoating();


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
	virtual Color GetAmbient(int mtlNum=0, BOOL backFace=FALSE);
	virtual Color GetDiffuse(int mtlNum=0, BOOL backFace=FALSE);
	virtual Color GetSpecular(int mtlNum=0, BOOL backFace=FALSE);
	virtual float GetXParency(int mtlNum=0, BOOL backFace=FALSE);
	virtual float GetShininess(int mtlNum=0, BOOL backFace=FALSE);
	virtual float GetShinStr(int mtlNum=0, BOOL backFace=FALSE);
	virtual float WireSize(int mtlNum=0, BOOL backFace=FALSE);


	// Shade and displacement calculation
	virtual void     Shade(ShadeContext& sc);
	virtual float    EvalDisplacement(ShadeContext& sc);
	virtual Interval DisplacementValidity(TimeValue t);

	// SubMaterial access methods
	//virtual int  NumSubMtls() {return NUM_SUBMATERIALS;}
	virtual int  NumSubMtls() { return NUM_SUBMATERIALS; }
	virtual Mtl* GetSubMtl(int i);
	virtual void SetSubMtl(int i, Mtl *m);
	virtual TSTR GetSubMtlSlotName(int i);
	virtual TSTR GetSubMtlTVName(int i);

	// SubTexmap access methods
	virtual int     NumSubTexmaps() { return NUM_SUBTEXTURES; }
	virtual Texmap* GetSubTexmap(int i);
	virtual void    SetSubTexmap(int i, Texmap *tx);
	virtual TSTR    GetSubTexmapSlotName(int i);
	virtual TSTR    GetSubTexmapTVName(int i);

	virtual BOOL SetDlgThing(ParamDlg* dlg);

	// Loading/Saving
	virtual IOResult Load(ILoad *iload);
	virtual IOResult Save(ISave *isave);

	// From Animatable
	virtual Class_ID ClassID() {return LUX_GLOSSY_COATIN_CLASS_ID;}
	virtual SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	virtual void GetClassName(TSTR& s) {s = GetString(IDS_CLASS_GLOSSY_COATING);}

	virtual RefTargetHandle Clone( RemapDir &remap );
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	virtual int NumSubs() { return 1+NUM_SUBMATERIALS; }
	virtual Animatable* SubAnim(int i);
	virtual TSTR SubAnimName(int i);

	// TODO: Maintain the number or references here
	virtual int NumRefs() { return 1 + NUM_REF; }
	virtual RefTargetHandle GetReference(int i);

	virtual int NumParamBlocks() { return 1; }					  // return number of ParamBlocks in this instance
	virtual IParamBlock2* GetParamBlock(int /*i*/) { return pblock; } // return i'th ParamBlock
	virtual IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	virtual void DeleteThis() { delete this; }

protected:
	virtual void SetReference(int i, RefTargetHandle rtarg);

private:
	Mtl*          submtl[NUM_SUBMATERIALS];  // Fixed size Reference array of sub-materials. (Indexes: 0-(N-1))
	Texmap*       subtexture[NUM_SUBTEXTURES];
	IParamBlock2* pblock;					 // Reference that comes AFTER the sub-materials. (Index: N)
	
	BOOL          mapOn[NUM_SUBMATERIALS];
	float         spin;
	Interval      ivalid;
	Interval	  mapValid;
};


class Lux_GlossyCoatingClassDesc : public ClassDesc2 
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL loading = FALSE) 		{ return new Lux_GlossyCoating(loading); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_GLOSSY_COATING); }
	virtual SClass_ID SuperClassID() 				{ return MATERIAL_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return LUX_GLOSSY_COATIN_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

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

	virtual const TCHAR* InternalName() 			{ return _T("Lux_GlossyCoating"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle

};


ClassDesc2* GetLux_Glossy_CoatingDesc() { 
	static Lux_GlossyCoatingClassDesc Lux_GlossyCoatingDesc;
	return &Lux_GlossyCoatingDesc; 
}


enum { Lux_GlossyCoating_params };

enum { Glosyy_coating_map, Common_Param, Light_emission };


//TODO: Add enums for various parameters
enum 
{
	base_Mat,
	specular,
	specular_Map,
	uroughness_Map,
	uroughness,
	vroughness_Map,
	vroughness,
	absorption,
	absorption_Map,
	thikness,
	thikness_Map,
	index,
	multibounce,

	/*Common material parameter begin*/
	bump_map,
	normal_map,
	interior_map,
	exterior_map,
	bump_sample,
	material_id,
	transparency,
	visiable_diff,
	visiable_specular,
	visiable_glossy,
	catch_shadow,
	/*Common material parameter end*/

	/*light emission params begin*/
	emission,
	emission_map,
	emission_power,
	emission_efficency,
	emission_mapfile,
	emission_gamma,
	emission_iesfile,
	emission_flipz,
	emission_samples,
	emission_map_width,
	emission_map_height,
	emission_id,
	enableemission,
	/*light emission params end*/
};


static ParamBlockDesc2 Lux_GlossyCoating_param_blk (
Lux_GlossyCoating_params, _T("params"),  0, GetLux_Glossy_CoatingDesc(),	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
1,
//rollout
Glosyy_coating_map, IDD_GLOSSY_COATING_PANEL, IDS_GLOSSY_COATING_PARAMS, 0, 0, NULL,
// params
base_Mat, _T("Base material"), TYPE_MTL, P_OWNERS_REF, IDS_GLOSSY_COATING_MAT,
	p_refno, 0, 
	p_submtlno, 0,
	p_ui, Glosyy_coating_map, TYPE_MTLBUTTON, IDC_GLOSSY_COATING_BASE_MAT,
	p_end,

specular, _T("Specular"), TYPE_RGBA, P_ANIMATABLE, IDS_GLOSSY_COATING_SPECULAR,
	p_default, Color(0.5f, 0.5f, 0.5f),
	p_ui, Glosyy_coating_map, TYPE_COLORSWATCH, IDC_GLOSSY_COATING_SPECULAR_COLOR,
	p_end,

specular_Map, _T("Specular Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_COATING_SPECULAR_MAP,
	p_refno, 2, 
	p_subtexno, 0,
	p_ui, Glosyy_coating_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_COATING_SPECULAR_MAP,
	p_end,
	
uroughness, _T("uroughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_COATING_UROUGHNESS,
	p_default, 0.1f,
	p_range, 0.0f, 9999.0f,
	p_ui, Glosyy_coating_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_COATING_UROUGHNESS, IDC_GLOSSY_COATING_UROUGHNESS_SPIN, 0.1f,
	p_end,

uroughness_Map, _T("uroughness Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_COATING_UROUGHNESS_MAP,
	p_refno, 3, 
	p_subtexno, 1,
	p_ui, Glosyy_coating_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_COATING_UROUGHNESS_MAP,
	p_end,

vroughness, _T("vroughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_COATING_VROUGHNESS,
	p_default, 0.1f,
	p_range, 0.0f, 9999.0f,
	p_ui, Glosyy_coating_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_COATING_VROUGHNESS, IDC_GLOSSY_COATING_VROUGHNESS_SPIN, 0.1f,
	p_end,

vroughness_Map, _T("vroughness Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_COATING_VROUGHNESS_MAP,
	p_refno, 4, 
	p_subtexno, 2,
	p_ui, Glosyy_coating_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_COATING_VROUGHNESS_MAP,
	p_end,

absorption, _T("Absorption"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_COATING_ABSORPTION,
	p_default, 0.0f,
	p_range, 0.0f, 9999.0f,
	p_ui, Glosyy_coating_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_COATING_ABSORPTION, IDC_GLOSSY_COATING_ABSORPTION_SPIN, 0.1f,
	p_end,

absorption_Map, _T("Absorption Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_COATING_ABSORPTION_MAP,
	p_refno, 5, 
	p_subtexno, 3,
	p_ui, Glosyy_coating_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_COATING_ABSORPTION_MAP,
	p_end,

thikness, _T("thikness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_COATING_THIKNESS,
	p_default, 0.0f,
	p_range, 0.0f, 9999.0f,
	p_ui, Glosyy_coating_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_COATING_THIKNESS, IDC_GLOSSY_COATING_THIKNESS_SPIN, 0.1f,
	p_end,

thikness_Map, _T("Thikness Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_COATING_THIKNESS_MAP,
	p_refno, 6, 
	p_subtexno, 4,
	p_ui, Glosyy_coating_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_COATING_THIKNESS_MAP,
	p_end,

index, _T("index"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_COATING_INDEX,
	p_default, 0.0f,
	p_range, 0.0f, 9999.0f,
	p_ui, Glosyy_coating_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_COATING_INDEX, IDC_GLOSSY_COATING_INDEX_SPIN, 0.1f,
	p_end,

multibounce, _T("Multibounce"), TYPE_BOOL, 0, IDS_GLOSSY_COATING_MULTIBOUNCE,
	p_default, FALSE,
	p_ui, Glosyy_coating_map, TYPE_SINGLECHEKBOX, IDC_GLOSSY_COATING_MULTIBOUNCE_ON,
	p_end,

p_end
);


Lux_GlossyCoating::Lux_GlossyCoating()
	: pblock(nullptr)
{
	for (int i = 0; i < NUM_SUBMATERIALS; i++)
	{
		submtl[i] = nullptr;
	}
	for (int i = 0; i < NUM_SUBTEXTURES; i++)
	{
		subtexture[i] = nullptr;
	}
	Reset();
}

Lux_GlossyCoating::Lux_GlossyCoating(BOOL loading)
	: pblock(nullptr)
{
	for (int i = 0; i < NUM_SUBMATERIALS; i++)
	{
		submtl[i] = nullptr;
	}
	for (int i = 0; i < NUM_SUBTEXTURES; i++)
	{
		subtexture[i] = nullptr;
	}
	
	if (!loading)
		Reset();
}

Lux_GlossyCoating::~Lux_GlossyCoating()
{
	DeleteAllRefs();
}

void Lux_GlossyCoating::Reset()
{
	ivalid.SetEmpty();
	mapValid.SetEmpty();
	// Always have to iterate backwards when deleting references.
	for (int i = NUM_SUBMATERIALS - 1; i >= 0; i--)
	{
		if( submtl[i] )
		{
			DeleteReference(i);
			DbgAssert(submtl[i] == nullptr);
			submtl[i] = nullptr;
		}
	}
	for (int i = NUM_SUBTEXTURES - 1; i >= 0; i--)
	{
		if (subtexture[i])
		{
			DeleteReference(i);
			DbgAssert(subtexture[i] == nullptr);
			subtexture[i] = nullptr;
		}
		//mapOn[i] = FALSE;
	}
	DeleteReference(PBLOCK_REF);

	GetLux_Glossy_CoatingDesc()->MakeAutoParamBlocks(this);
}


ParamDlg* Lux_GlossyCoating::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLux_Glossy_CoatingDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
	
}

BOOL Lux_GlossyCoating::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

Interval Lux_GlossyCoating::Validity(TimeValue t)
{
	Interval valid = FOREVER;

	for (int i = 0; i < NUM_SUBMATERIALS; i++)
	{
		if (submtl[i])
			valid &= submtl[i]->Validity(t);
	}
	for (int i = 0; i < NUM_SUBTEXTURES; i++)
	{
		if (subtexture[i])
			valid &= subtexture[i]->Validity(t);
	}
	float u;
	pblock->GetValue(thikness, t, u, valid);
	return valid;
}

/*===========================================================================*\
 |	Sub-anim & References support
\*===========================================================================*/

RefTargetHandle Lux_GlossyCoating::GetReference(int i)
{
	/*switch (i)
	{
		case 0: return submtl[i]; break;
		case 1: return pblock; break;
		default: return subtexture[i - 2]; break;
	}*/
	if (i == PBLOCK_REF)
		return pblock;
	else if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	else if ((i >= NUM_SUBMATERIALS) && (i < NUM_SUBTEXTURES))
		return subtexture[i - 2];
	else
		return nullptr;

}

void Lux_GlossyCoating::SetReference(int i, RefTargetHandle rtarg)
{
	//mprintf(_T("\n SetReference Nubmer is ------->>>>: %i \n"), i);
	/*switch (i)
	{
		case 0: submtl[i] = (Mtl *)rtarg; break;
		case 1: pblock = (IParamBlock2 *)rtarg; break;
		default: subtexture[i-2] = (Texmap *)rtarg; break;
	}*/
	if (i == PBLOCK_REF)
		pblock = (IParamBlock2 *)rtarg;
	else if ((i >= 0) && (i < NUM_SUBMATERIALS))
		submtl[i] = (Mtl *)rtarg;
	else if ((i >= NUM_SUBMATERIALS) && (i < NUM_SUBTEXTURES))
		subtexture[i - 2] = (Texmap *)rtarg;
}

TSTR Lux_GlossyCoating::SubAnimName(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return GetSubMtlTVName(i);
	else
		return GetSubTexmapTVName(i-2);
}

Animatable* Lux_GlossyCoating::SubAnim(int i)
{
	/*switch (i)
	{
		case 0: return submtl[i];
		case 1: return pblock;
		default: return subtexture[i-2];
	}*/
	if (i == PBLOCK_REF)
		return pblock;
	else if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	else if ((i >= NUM_SUBMATERIALS) && (i < NUM_SUBTEXTURES))
		return subtexture[i - 2];
	else
		return nullptr;
}

RefResult Lux_GlossyCoating::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, 
	PartID& /*partID*/, RefMessage message, BOOL /*propagate*/ ) 
{
	switch (message) {
	case REFMSG_CHANGE:
		{
			ivalid.SetEmpty();
			mapValid.SetEmpty();
			if (hTarget == pblock)
			{
				ParamID changing_param = pblock->LastNotifyParamID();
				Lux_GlossyCoating_param_blk.InvalidateUI(changing_param);
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
				for (int i = 0; i < NUM_SUBTEXTURES; i++)
				{
					if (hTarget == subtexture[i])
					{
						subtexture[i] = nullptr;
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

Mtl* Lux_GlossyCoating::GetSubMtl(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	return 
		nullptr;
}

void Lux_GlossyCoating::SetSubMtl(int i, Mtl* m)
{
	//mprintf(_T("\n SetSubMtl Nubmer is : %i \n"), i);
	ReplaceReference(i , m);
	switch (i)
	{
		case 0:
			Lux_GlossyCoating_param_blk.InvalidateUI(base_Mat);
			mapValid.SetEmpty();
			break;
	}
}

TSTR Lux_GlossyCoating::GetSubMtlSlotName(int i)
{
	switch (i)
	{
		case 0:
			return _T("Base Material");
		default:
			return _T("");
	}
	// Return i'th sub-material name
	//return submtl[i]->GetName();
	//return _T("");
}

TSTR Lux_GlossyCoating::GetSubMtlTVName(int i)
{
	return GetSubMtlSlotName(i);
}

/*===========================================================================*\
 |	Texmap get and set
\*===========================================================================*/

Texmap* Lux_GlossyCoating::GetSubTexmap(int i)
{
	//mprintf(_T("\n GetSubTexmap Nubmer ::::::::::::===>>>  is : Get %i \n"), i);
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return subtexture[i];
	return
		nullptr;
}

void Lux_GlossyCoating::SetSubTexmap(int i, Texmap* tx)
{
	//mprintf(_T("\n SetSubTexmap Nubmer ============>>>  is : %i \n"), i);
	ReplaceReference(i +2, tx);
	switch (i)
	{
		case 0:
			Lux_GlossyCoating_param_blk.InvalidateUI(specular_Map);
			mapValid.SetEmpty();
			break;
		case 1:
			Lux_GlossyCoating_param_blk.InvalidateUI(uroughness_Map);
			mapValid.SetEmpty();
			break;
		case 2:
			Lux_GlossyCoating_param_blk.InvalidateUI(vroughness_Map);
			mapValid.SetEmpty();
			break;
		case 3:
			Lux_GlossyCoating_param_blk.InvalidateUI(absorption_Map);
			mapValid.SetEmpty();
			break;
		case 4:
			Lux_GlossyCoating_param_blk.InvalidateUI(thikness_Map);
			mapValid.SetEmpty();
			break;
	}
}

TSTR Lux_GlossyCoating::GetSubTexmapSlotName(int i)
{
	switch (i)
	{
		case 0:
			return _T("Specular Map");
		case 1:
			return _T("uroughness Map");
		case 2:
			return _T("vroughness Map");
		case 3:
			return _T("Absorption Map");
		case 4:
			return _T("Thikness Map");
		default:
			return _T("");
	}
}

TSTR Lux_GlossyCoating::GetSubTexmapTVName(int i)
{
	// Return i'th sub-texture name
	return GetSubTexmapSlotName(i);
}

/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define PARAM2_CHUNK 0x1010

IOResult Lux_GlossyCoating::Save(ISave* isave)
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK)
		return res;
	isave->EndChunk();

	return IO_OK;
}

IOResult Lux_GlossyCoating::Load(ILoad* iload)
{
	IOResult res;
	while (IO_OK==(res=iload->OpenChunk()))
	{
		int id = iload->CurChunkID();
		switch(id)
		{
		case MTL_HDR_CHUNK:
			res = MtlBase::Load(iload);
			break;
		}

		iload->CloseChunk();
		if (res!=IO_OK)
			return res;
	}

	return IO_OK;
}


/*===========================================================================*\
 |	Updating and cloning
\*===========================================================================*/

RefTargetHandle Lux_GlossyCoating::Clone(RemapDir &remap)
{
	Lux_GlossyCoating *mnew = new Lux_GlossyCoating(FALSE);
	*((MtlBase*)mnew) = *((MtlBase*)this);
	// First clone the parameter block
	mnew->ReplaceReference(PBLOCK_REF,remap.CloneRef(pblock));
	// Next clone the sub-materials
	mnew->ivalid.SetEmpty();
	mnew->mapValid.SetEmpty();
	for (int i = 0; i < NUM_SUBMATERIALS; i++) 
	{
		mnew->submtl[i] = nullptr;
		if (submtl[i])
			mnew->ReplaceReference(i,remap.CloneRef(submtl[i]));
		mnew->mapOn[i] = mapOn[i];
	}
	for (int i = 0; i < NUM_SUBTEXTURES; i++)
	{
		mnew->subtexture[i] = nullptr;
		if (subtexture[i])
			mnew->ReplaceReference(i + 2, remap.CloneRef(subtexture[i]));
		//mnew->mapOn[i] = mapOn[i];
	}
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
	}

void Lux_GlossyCoating::NotifyChanged()
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void Lux_GlossyCoating::Update(TimeValue t, Interval& valid)
{
	if (!ivalid.InInterval(t))
	{

		ivalid.SetInfinite();
		//pblock->GetValue( mtl_mat1_on, t, mapOn[0], ivalid);
		//pblock->GetValue( pb_spin, t, spin, ivalid);
		pblock->GetValue(thikness, t, spin, ivalid);
		
		for (int i=0; i < NUM_SUBMATERIALS; i++)
		{
			if (submtl[i])
				submtl[i]->Update(t,ivalid);
		}
	}

	if (!mapValid.InInterval(t))
	{
		mapValid.SetInfinite();
		for (int i = 0; i<NUM_SUBTEXTURES; i++) {
			if (subtexture[i])
				subtexture[i]->Update(t, mapValid);
		}
	}

	valid &= mapValid;
	valid &= ivalid;
}

/*===========================================================================*\
 |	Determine the characteristics of the material
\*===========================================================================*/

void Lux_GlossyCoating::SetAmbient(Color /*c*/, TimeValue /*t*/) {}		
void Lux_GlossyCoating::SetDiffuse(Color /*c*/, TimeValue /*t*/) {}		
void Lux_GlossyCoating::SetSpecular(Color /*c*/, TimeValue /*t*/) {}
void Lux_GlossyCoating::SetShininess(float /*v*/, TimeValue /*t*/) {}

Color Lux_GlossyCoating::GetAmbient(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	//pblock->GetValue(prm_color, GetCOREInterface()->GetTime(), p, ivalid);
	return submtl[0] ? submtl[0]->GetAmbient(mtlNum, backFace) : Color(p.x, p.y, p.z);//Bound(Color(p.x, p.y, p.z));
}

Color Lux_GlossyCoating::GetDiffuse(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue(specular, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetDiffuse(mtlNum, backFace) : Color(p.x, p.y, p.z);
}

Color Lux_GlossyCoating::GetSpecular(int mtlNum, BOOL backFace)
{
	Point3 p;
	pblock->GetValue(specular, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetSpecular(mtlNum,backFace): Color(p.x, p.y, p.z);
}

float Lux_GlossyCoating::GetXParency(int mtlNum, BOOL backFace)
{
	float t = 0.0f;
	//pblock->GetValue(pb_opacity, 0, t, ivalid);
	return submtl[0] ? submtl[0]->GetXParency(mtlNum,backFace): t;
}

float Lux_GlossyCoating::GetShininess(int mtlNum, BOOL backFace)
{
	float sh = 1.0f;
	//pblock->GetValue(pb_shin, 0, sh, ivalid);
	return submtl[0] ? submtl[0]->GetShininess(mtlNum,backFace): sh;
}

float Lux_GlossyCoating::GetShinStr(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShinStr(mtlNum,backFace): 0.0f;
}

float Lux_GlossyCoating::WireSize(int mtlNum, BOOL backFace)
{
	float wf = 0.0f;
	//pblock->GetValue(pb_wiresize, 0, wf, ivalid);
	return submtl[0] ? submtl[0]->WireSize(mtlNum, backFace) : wf;
}


/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void Lux_GlossyCoating::Shade(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	if (gbufID)
		sc.SetGBufferID(gbufID);

	if(subMaterial)
		subMaterial->Shade(sc);
	// TODO: compute the color and transparency output returned in sc.out.
}

float Lux_GlossyCoating::EvalDisplacement(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	return (subMaterial) ? subMaterial->EvalDisplacement(sc) : 0.0f;
}

Interval Lux_GlossyCoating::DisplacementValidity(TimeValue t)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;

	Interval iv;
	iv.SetInfinite();
	if(subMaterial) 
		iv &= subMaterial->DisplacementValidity(t);

	return iv;
}


