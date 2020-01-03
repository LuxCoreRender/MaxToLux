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

#include "Lux_Mtl.h"
//#include <maxscript\maxscript.h>

#define Lux_Mirror_CLASS_ID	Class_ID(0xcca41e8, 0x381d207f)


#define NUM_SUBMATERIALS 1 //was 16 TODO: number of sub-materials supported by this plug-in
#define NUM_SUBTEXTURES 8
#define Num_REF 10
// Reference Indexes
// 
#define PBLOCK_REF 1

static int seed = rand() % 5400 + 6400;

class Lux_Mirror : public Mtl {
public:
	Lux_Mirror();
	Lux_Mirror(BOOL loading);
	~Lux_Mirror();


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
	virtual int  NumSubMtls() { return 0; }
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
	virtual Class_ID ClassID() {return Lux_Mirror_CLASS_ID;}
	virtual SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	virtual void GetClassName(TSTR& s) {s = GetString(IDS_CLASS_MIRROR);}

	virtual RefTargetHandle Clone( RemapDir &remap );
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	virtual int NumSubs() { return 1+NUM_SUBMATERIALS; }
	virtual Animatable* SubAnim(int i);
	virtual TSTR SubAnimName(int i);

	// TODO: Maintain the number or references here
	virtual int NumRefs() { return 1 + Num_REF; }
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



class Lux_MirrorClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL loading = FALSE) 		{ return new Lux_Mirror(loading); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_MIRROR); }
	virtual SClass_ID SuperClassID() 				{ return MATERIAL_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return Lux_Mirror_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("Lux_Mirror"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetLux_MirrorDesc() { 
	static Lux_MirrorClassDesc Lux_MirrorDesc;
	return &Lux_MirrorDesc;
}


enum { Lux_Mirror_params };

enum { Mirror_map, Common_Param, Light_emission };

//TODO: Add enums for various parameters
enum 
{
	reflection_color,
	mtl_reflection_map,

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


static ParamBlockDesc2 Lux_Mirror_param_blk (
	Lux_Mirror_params, _T("params"),  0, GetLux_MirrorDesc(),	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
	3,
	//rollout
	Mirror_map,				IDD_MIRROR_PANEL,		IDS_MIRROR_PARAMS,		0,	0,	NULL,
	Common_Param,			IDD_COMMON_PANEL,		IDS_COMMON_PARAMS,		0,	0,	NULL,
	Light_emission,			IDD_LIGHT_PANEL,		IDS_LIGHT_PARAMS,		0,	0,	NULL,
	// params
	reflection_color,			_T("mtl_mirror_reflection_color"),			TYPE_RGBA,	P_ANIMATABLE,		IDS_COLOR,
		p_default,		Color(0.5f, 0.5f, 0.5f),
		p_ui,			Mirror_map,				TYPE_COLORSWATCH,				IDC_MIRROR_REFLECTION_COLOR,
		p_end,
	mtl_reflection_map,	_T("mtl_mirror_reflection_map"),	TYPE_TEXMAP,	P_OWNERS_REF,	IDS_DIFFUSE_MAP,
		p_refno,		2,
		p_subtexno,		0,
		p_ui, Mirror_map,						TYPE_TEXMAPBUTTON,				IDC_MIRROR_REFLECTION_MAP,
		p_end,

	// Common param
	bump_map, _T("Bump Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_BUMP_MAP,
		p_refno, 3,
		p_subtexno, 1,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_BUMP_MAP,
		p_end,

	normal_map, _T("Normal Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_NORMAL_MAP,
		p_refno, 4,
		p_subtexno, 2,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_NORMAL_MAP,
		p_end,

	interior_map, _T("Interior Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_INTERIOR_MAP,
		p_refno, 5,
		p_subtexno, 3,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_INTERIOR_MAP,
		p_end,

	exterior_map, _T("Exterior Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EXTERIOR_MAP,
		p_refno, 6,
		p_subtexno, 4,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_EXTERIOR_MAP,
		p_end,

	bump_sample, _T("Bump Sample"), TYPE_INT, P_ANIMATABLE, IDS_BUMP_SAMPLE,
		p_default, -1,
		p_range, -1, 100,
		p_ui, Common_Param, TYPE_SPINNER, EDITTYPE_INT, IDC_SAMPLE_EDIT, IDC_SAMPLE_SPIN, 1,
		p_end,

	material_id, _T("Material ID"), TYPE_INT, P_ANIMATABLE, IDS_MATERIAL_ID,
		p_default, seed,
		p_range, 1, 36000,
		p_ui, Common_Param, TYPE_SPINNER, EDITTYPE_INT, IDC_ID_EDIT, IDC_ID_SPIN, 1,
		p_end,

	transparency, _T("Transparency"), TYPE_FLOAT, P_ANIMATABLE, IDS_TRANSPARENCY,
		p_default, 1.0f,
		p_range, 0.01f, 1.0f,
		p_ui, Common_Param, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TRANSPARENCY_EDIT, IDC_TRANSPARENCY_SPIN, 0.01f,
		p_end,

	// Light
	emission, _T("emission_color"), TYPE_RGBA, P_ANIMATABLE, IDS_EMISSION,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Light_emission, TYPE_COLORSWATCH, IDC_EMISSION_COLOR,
		p_end,

	emission_map, _T("emission_map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_MAP,
		p_refno, 7,
		p_subtexno, 5,
		p_ui, Light_emission, TYPE_TEXMAPBUTTON, IDC_EMISSION_MAP,
		p_end,

	emission_power, _T("emission_power"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_POWER,
		p_default, 0.0f,
		p_range, 0.0f, 999.0f,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_POWER, IDC_EMISSION_POWER_SPIN, 0.1f,
		p_end,

	emission_efficency, _T("emission_efficency"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_EFFICENCY,
		p_default, 0.0f,
		p_range, 0.0f, 999.0f,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_EFFICENCY, IDC_EMISSION_EFFICENCY_SPIN, 0.1f,
		p_end,

	emission_mapfile, _T("emission_mapfile"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_MAPFILE,
		p_refno, 8,
		p_subtexno, 6,
		p_ui, Light_emission, TYPE_TEXMAPBUTTON, IDC_EMISSION_MAPFILE,
		p_end,

	emission_gamma, _T("emission_gamma"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_GAMMA,
		p_default, 2.2f,
		p_range, 0.0f, 999.0f,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_GAMMA, IDC_EMISSION_GAMMA_SPIN, 0.1f,
		p_end,

	emission_iesfile, _T("emission_iesfile"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_IESFILE,
		p_refno, 9,
		p_subtexno, 7,
		p_ui, Light_emission, TYPE_TEXMAPBUTTON, IDC_EMISSION_IESFILE,
		p_end,

	emission_flipz, _T("emission_flipz"), TYPE_BOOL, 0, IDS_EMISSION_FLIPZ,
		p_default, FALSE,
		p_ui, Light_emission, TYPE_SINGLECHEKBOX, IDC_EMISSION_FLIPZ,
		p_end,

	emission_samples, _T("emission_samples"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_SAMPLES,
		p_default, -1,
		p_range, -1, 9999,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_SAMPLES, IDC_EMISSION_SAMPLES_SPIN, 1,
		p_end,

	emission_map_width, _T("emission_map_width"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_MAP_WIDTH,
		p_default, 0,
		p_range, 0, 9999,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_MAP_WIDTH, IDC_EMISSION_MAP_WIDTH_SPIN, 1,
		p_end,

	emission_map_height, _T("emission_map_height"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_MAP_HEIGHT,
		p_default, 0,
		p_range, 0, 9999,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_MAP_HEIGHT, IDC_EMISSION_MAP_HEIGHT_SPIN, 1,
		p_end,

	emission_id, _T("emission_id"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_ID,
		p_default, 0,
		p_range, 0, 9999,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_ID, IDC_EMISSION_ID_SPIN, 1,
		p_end,

	enableemission, _T("enableemission"), TYPE_BOOL, 0, IDS_ENABLEEMISSION,
		p_default, FALSE,
		p_ui, Light_emission, TYPE_SINGLECHEKBOX, IDC_ENABLEEMISSION,
		p_end,

	p_end
	);


Lux_Mirror::Lux_Mirror()
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

Lux_Mirror::Lux_Mirror(BOOL loading)
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

Lux_Mirror::~Lux_Mirror()
{
	DeleteAllRefs();
}


void Lux_Mirror::Reset()
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
		mapOn[i] = FALSE;
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

	GetLux_MirrorDesc()->MakeAutoParamBlocks(this);
}



ParamDlg* Lux_Mirror::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLux_MirrorDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
}

BOOL Lux_Mirror::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

Interval Lux_Mirror::Validity(TimeValue t)
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
	//float u;
	//pblock->GetValue(pb_spin,t,u,valid);
	return valid;
}

/*===========================================================================*\
 |	Sub-anim & References support
\*===========================================================================*/

RefTargetHandle Lux_Mirror::GetReference(int i)
{
	switch (i)
	{
		//case 0: return subtexture[i]; break;
		case 0: return pblock; break;
		//case 2: return subtexture[i-2]; break;
		default: return subtexture[i - 2]; break;
	}

}

void Lux_Mirror::SetReference(int i, RefTargetHandle rtarg)
{
	//mprintf(_T("\n SetReference Nubmer is ------->>>>: %i \n"), i);
	switch (i)
	{
		//case 0: subtexture[i] = (Texmap *)rtarg; break;
		case 0: pblock = (IParamBlock2 *)rtarg; break;
		//case 2: subtexture[i-2] = (Texmap *)rtarg; break;
		default: subtexture[i-2] = (Texmap *)rtarg; break;
	}
}

TSTR Lux_Mirror::SubAnimName(int i)
{
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return GetSubTexmapTVName(i);
	else
		return GetSubTexmapTVName(i-2);
}

Animatable* Lux_Mirror::SubAnim(int i)
{
	switch (i)
	{
		//case 0: return subtexture[i];
		case 0: return pblock;
		default: return subtexture[i-2];
	}
}

RefResult Lux_Mirror::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, 
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
				Lux_Mirror_param_blk.InvalidateUI(changing_param);
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

Mtl* Lux_Mirror::GetSubMtl(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	return 
		nullptr;
}

void Lux_Mirror::SetSubMtl(int i, Mtl* m)
{
	//mprintf(_T("\n SetSubMtl Nubmer is : %i \n"), i);
	ReplaceReference(i , m);
	if (i == 0)
	{
		//Lux_Mirror_param_blk.InvalidateUI(mtl_mat1);
		//mapValid.SetEmpty();
	}
}

TSTR Lux_Mirror::GetSubMtlSlotName(int i)
{
	// Return i'th sub-material name
	return submtl[i]->GetName();
	//return _T("");
}

TSTR Lux_Mirror::GetSubMtlTVName(int i)
{
	return GetSubMtlSlotName(i);
}

/*===========================================================================*\
 |	Texmap get and set
\*===========================================================================*/

Texmap* Lux_Mirror::GetSubTexmap(int i)
{
	//mprintf(_T("\n GetSubTexmap Nubmer ::::::::::::===>>>  is : Get %i \n"), i);
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return subtexture[i];
	return
		nullptr;
}

void Lux_Mirror::SetSubTexmap(int i, Texmap* tx)
{
	//mprintf(_T("\n SetSubTexmap Nubmer ============>>>  is : %i \n"), i);
	ReplaceReference(i +2, tx);
	switch (i)
	{
		case 0:
			Lux_Mirror_param_blk.InvalidateUI(mtl_reflection_map);
			mapValid.SetEmpty();
			break;
		case 1:
			Lux_Mirror_param_blk.InvalidateUI(bump_map);
			mapValid.SetEmpty();
			break;
		case 2:
			Lux_Mirror_param_blk.InvalidateUI(normal_map);
			mapValid.SetEmpty();
			break;
		case 3:
			Lux_Mirror_param_blk.InvalidateUI(interior_map);
			mapValid.SetEmpty();
			break;
		case 4:
			Lux_Mirror_param_blk.InvalidateUI(exterior_map);
			mapValid.SetEmpty();
			break;
		case 5:
			Lux_Mirror_param_blk.InvalidateUI(emission_map);
			mapValid.SetEmpty();
			break;
		case 6:
			Lux_Mirror_param_blk.InvalidateUI(emission_mapfile);
			mapValid.SetEmpty();
			break;
		case 7:
			Lux_Mirror_param_blk.InvalidateUI(emission_iesfile);
			mapValid.SetEmpty();
			break;
	}
}

TSTR Lux_Mirror::GetSubTexmapSlotName(int i)
{
	switch (i)
	{
		case 0:
			return _T("Diffuse Map");
		case 1:
			return _T("Bump map");
		case 2:
			return _T("Normal map");
		case 3:
			return _T("Interior map");
		case 4:
			return _T("Exterior map");
		case 5:
			return _T("emission_map");
		case 6:
			return _T("emission_mapfile");
		case 7:
			return _T("emission_iesfile");
		default: return _T("");
	}
}

TSTR Lux_Mirror::GetSubTexmapTVName(int i)
{
	// Return i'th sub-texture name
	return GetSubTexmapSlotName(i);
}



/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define PARAM2_CHUNK 0x1010

IOResult Lux_Mirror::Save(ISave* isave)
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK)
		return res;
	isave->EndChunk();

	return IO_OK;
}

IOResult Lux_Mirror::Load(ILoad* iload)
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

RefTargetHandle Lux_Mirror::Clone(RemapDir &remap)
{
	Lux_Mirror *mnew = new Lux_Mirror(FALSE);
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

void Lux_Mirror::NotifyChanged()
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void Lux_Mirror::Update(TimeValue t, Interval& valid)
{
	if (!ivalid.InInterval(t))
	{

		ivalid.SetInfinite();
		//pblock->GetValue( mtl_mat1_on, t, mapOn[0], ivalid);
		//pblock->GetValue( pb_spin, t, spin, ivalid);

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

void Lux_Mirror::SetAmbient(Color /*c*/, TimeValue /*t*/) {}		
void Lux_Mirror::SetDiffuse(Color /*c*/, TimeValue /*t*/) {}		
void Lux_Mirror::SetSpecular(Color /*c*/, TimeValue /*t*/) {}
void Lux_Mirror::SetShininess(float /*v*/, TimeValue /*t*/) {}

Color Lux_Mirror::GetAmbient(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue(reflection_color, GetCOREInterface()->GetTime(), p, ivalid);
	return submtl[0] ? submtl[0]->GetAmbient(mtlNum, backFace) : Color(p.x, p.y, p.z);//Bound(Color(p.x, p.y, p.z));
}

Color Lux_Mirror::GetDiffuse(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue(reflection_color, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetDiffuse(mtlNum, backFace) : Color(p.x, p.y, p.z);
}

Color Lux_Mirror::GetSpecular(int mtlNum, BOOL backFace)
{
	Point3 p;
	pblock->GetValue(reflection_color, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetSpecular(mtlNum,backFace): Color(p.x, p.y, p.z);
}

float Lux_Mirror::GetXParency(int mtlNum, BOOL backFace)
{
	float t = 0.0f;
	//pblock->GetValue(pb_opacity, 0, t, ivalid);
	return submtl[0] ? submtl[0]->GetXParency(mtlNum,backFace): t;
}

float Lux_Mirror::GetShininess(int mtlNum, BOOL backFace)
{
	float sh = 1.0f;
	//pblock->GetValue(pb_shin, 0, sh, ivalid);
	return submtl[0] ? submtl[0]->GetShininess(mtlNum,backFace): sh;
}

float Lux_Mirror::GetShinStr(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShinStr(mtlNum,backFace): 0.0f;
}

float Lux_Mirror::WireSize(int mtlNum, BOOL backFace)
{
	float wf = 0.0f;
	//pblock->GetValue(pb_wiresize, 0, wf, ivalid);
	return submtl[0] ? submtl[0]->WireSize(mtlNum, backFace) : wf;
}


/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void Lux_Mirror::Shade(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	if (gbufID)
		sc.SetGBufferID(gbufID);

	if(subMaterial)
		subMaterial->Shade(sc);
	// TODO: compute the color and transparency output returned in sc.out.
}

float Lux_Mirror::EvalDisplacement(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	return (subMaterial) ? subMaterial->EvalDisplacement(sc) : 0.0f;
}

Interval Lux_Mirror::DisplacementValidity(TimeValue t)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;

	Interval iv;
	iv.SetInfinite();
	if(subMaterial) 
		iv &= subMaterial->DisplacementValidity(t);

	return iv;
}


