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

#include "Lux_Materal.h"
//#include <maxscript\maxscript.h>

#define Lux_MIX_CLASS_ID		Class_ID(0x26894a44, 0x8ce7173)


#define NUM_SUBMATERIALS 2
#define NUM_SUBTEXTURES 8
#define NUM_REF NUM_SUBMATERIALS + NUM_SUBTEXTURES
// Reference Indexes
// 
#define PBLOCK_REF NUM_REF

static int seed = rand() % 2400 + 1400;

class Lux_Mix : public Mtl {
public:
	Lux_Mix();
	Lux_Mix(BOOL loading);
	~Lux_Mix();


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
	virtual int  NumSubMtls() {return NUM_SUBMATERIALS;}
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
	virtual Class_ID ClassID() {return Lux_MIX_CLASS_ID;}
	virtual SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	virtual void GetClassName(TSTR& s) {s = GetString(IDS_CLASS_MIX);}

	virtual RefTargetHandle Clone( RemapDir &remap );
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	virtual int NumSubs() { return 1 + NUM_REF; }
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
	
	//BOOL          mapOn[NUM_SUBMATERIALS];
	float         spin;
	Interval      ivalid;
	//Interval	  mapValid;
};



class Lux_MixClassDesc : public ClassDesc2 
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL loading = FALSE) 		{ return new Lux_Mix(loading); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_MIX); }
	virtual SClass_ID SuperClassID() 				{ return MATERIAL_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return Lux_MIX_CLASS_ID; }
	virtual const TCHAR* Category()					{ return GetString(IDS_CATEGORY); }

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

	virtual const TCHAR* InternalName() 			{ return _T("Lux_Mix"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetLux_MixDesc() { 
	static Lux_MixClassDesc Lux_MixDesc;
	return &Lux_MixDesc; 
}


enum { Lux_Mix2_params };

enum { Mix_Mtl, Common_Param, Light_emission };


//TODO: Add enums for various parameters
enum {
	/*Main material parameter begin*/
	pb_spin,
	mtl_mat1,
	mtl_mat2,
	prm_color,
	mtl_map,
	prm_ambientcolor,
	pb_opacity,
	prm_spec,
	pb_shin,
	pb_wiresize,
	/*Main material parameter end*/
	
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

	/*Light emission params begin*/
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


static ParamBlockDesc2 Lux_Mix_param_blk (
	Lux_Mix2_params, _T("params"),  0, GetLux_MixDesc(),	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
	3,
	//rollout
	Mix_Mtl,			IDD_MIX_PANEL,			IDS_MAIN_PARAM,		0,	0,	NULL,
	Common_Param,		IDD_COMMON_PANEL,		IDS_COMMON_PARAMS,	0,	0,	NULL,
	Light_emission,		IDD_LIGHT_PANEL,		IDS_LIGHT_PARAMS,	0,	0,	NULL,

	// main params
	mtl_mat1,			_T("mtl_mat1"),			TYPE_MTL,		P_OWNERS_REF,		IDS_MTL1,
		p_refno,		0,
		p_submtlno,		0,		
		p_ui,			Mix_Mtl,				TYPE_MTLBUTTON,			IDC_MIX_MTL1,
		p_end,

	mtl_mat1,			_T("mtl_mat2"),					TYPE_MTL,		P_OWNERS_REF,		IDS_MTL2,
		p_refno,		1,
		p_submtlno,		1,
		p_ui,			Mix_Mtl,				TYPE_MTLBUTTON,					IDC_MIX_MTL2,
		p_end,

	pb_spin,			_T("spin"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPIN,
		p_default,		0.1f,
		p_range,		0.0f, 1000.0f,
		p_ui,			Mix_Mtl,				TYPE_SPINNER,		EDITTYPE_FLOAT, IDC_MIX_AMOUNT_EDIT, IDC_MIX_AMOUNT_SPIN, 0.01f,
		p_end,

	mtl_map,			_T("Amount Map"),			TYPE_TEXMAP,	P_OWNERS_REF,		IDS_AMOUNT_MAP,
		p_refno,		2,
		p_subtexno,		0,
		p_ui,			Mix_Mtl,				TYPE_TEXMAPBUTTON,						IDC_MIX_AMOUNT_MAP,
		p_end,

	// Common param
	bump_map,			_T("Bump Map"),			TYPE_TEXMAP, P_OWNERS_REF, IDS_BUMP_MAP,
		p_refno,		3,
		p_subtexno,		1,
		p_ui,			Common_Param,			TYPE_TEXMAPBUTTON,		IDC_BUMP_MAP,
		p_end,

	normal_map,			_T("Normal Map"),		TYPE_TEXMAP, P_OWNERS_REF, IDS_NORMAL_MAP,
		p_refno,		4,
		p_subtexno,		2,
		p_ui,			Common_Param,			TYPE_TEXMAPBUTTON, IDC_NORMAL_MAP,
		p_end,

	interior_map,		_T("Interior Map"),		TYPE_TEXMAP, P_OWNERS_REF, IDS_INTERIOR_MAP,
		p_refno,		5,
		p_subtexno,		3,
		p_ui,			Common_Param,			TYPE_TEXMAPBUTTON, IDC_INTERIOR_MAP,
		p_end,

	exterior_map,		_T("Exterior Map"),		TYPE_TEXMAP, P_OWNERS_REF, IDS_EXTERIOR_MAP,
		p_refno,		6,
		p_subtexno,		4,
		p_ui,			Common_Param,			TYPE_TEXMAPBUTTON, IDC_EXTERIOR_MAP,
		p_end,

	bump_sample,		_T("Bump Sample"),		TYPE_INT, P_ANIMATABLE, IDS_BUMP_SAMPLE,
		p_default,		-1,
		p_range,		-1, 100,
		p_ui,			Common_Param,			TYPE_SPINNER, EDITTYPE_INT, IDC_SAMPLE_EDIT, IDC_SAMPLE_SPIN, 1,
		p_end,

	material_id,		_T("Material ID"),		TYPE_INT, P_ANIMATABLE, IDS_MATERIAL_ID,
		p_default,		seed,
		p_range,		1, 36000,
		p_ui,			Common_Param,			TYPE_SPINNER, EDITTYPE_INT, IDC_ID_EDIT, IDC_ID_SPIN, 1,
		p_end,

	transparency,		_T("Transparency"),		TYPE_FLOAT, P_ANIMATABLE, IDS_TRANSPARENCY,
		p_default,		1.0f,
		p_range,		0.01f, 1.0f,
		p_ui,			Common_Param,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TRANSPARENCY_EDIT, IDC_TRANSPARENCY_SPIN, 0.01f,
		p_end,

	// Light param
	emission,			_T("emission_color"),	TYPE_RGBA, P_ANIMATABLE, IDS_EMISSION,
		p_default,		Color(0.5f, 0.5f, 0.5f),
		p_ui,			Light_emission,			TYPE_COLORSWATCH, IDC_EMISSION_COLOR,
		p_end,

	emission_map,		_T("emission_map"),		TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_MAP,
		p_refno,		7,
		p_subtexno,		5,
		p_ui,			Light_emission,			TYPE_TEXMAPBUTTON, IDC_EMISSION_MAP,
		p_end,

	emission_power,		_T("emission_power"),	TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_POWER,
		p_default,		0.0f,
		p_range,		0.0f, 999.0f,
		p_ui,			Light_emission,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_POWER, IDC_EMISSION_POWER_SPIN, 0.1f,
		p_end,

	emission_efficency, _T("emission_efficency"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_EFFICENCY,
		p_default,		0.0f,
		p_range,		0.0f, 999.0f,
		p_ui,			Light_emission,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_EFFICENCY, IDC_EMISSION_EFFICENCY_SPIN, 0.1f,
		p_end,

	emission_mapfile,	_T("emission_mapfile"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_MAPFILE,
		p_refno,		8,
		p_subtexno,		6,
		p_ui,			Light_emission,			TYPE_TEXMAPBUTTON, IDC_EMISSION_MAPFILE,
		p_end,

	emission_gamma,		_T("emission_gamma"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_GAMMA,
		p_default,		2.2f,
		p_range,		0.0f, 999.0f,
		p_ui,			Light_emission,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_GAMMA, IDC_EMISSION_GAMMA_SPIN, 0.1f,
		p_end,

	emission_iesfile,	_T("emission_iesfile"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_IESFILE,
		p_refno,		9,
		p_subtexno,		7,
		p_ui,			Light_emission,			TYPE_TEXMAPBUTTON, IDC_EMISSION_IESFILE,
		p_end,

	emission_flipz,		_T("emission_flipz"), TYPE_BOOL, 0, IDS_EMISSION_FLIPZ,
		p_default,		FALSE,
		p_ui,			Light_emission,			TYPE_SINGLECHEKBOX, IDC_EMISSION_FLIPZ,
		p_end,

	emission_samples,	_T("emission_samples"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_SAMPLES,
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

	emission_id,		_T("emission_id"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_ID,
		p_default, 0,
		p_range, 0, 9999,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_ID, IDC_EMISSION_ID_SPIN, 1,
		p_end,

	enableemission,		_T("enableemission"), TYPE_BOOL, 0, IDS_ENABLEEMISSION,
		p_default, FALSE,
		p_ui, Light_emission, TYPE_SINGLECHEKBOX, IDC_ENABLEEMISSION,
		p_end,

	p_end
	);


Lux_Mix::Lux_Mix()
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

Lux_Mix::Lux_Mix(BOOL loading)
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

Lux_Mix::~Lux_Mix()
{
	DeleteAllRefs();
}


void Lux_Mix::Reset()
{
	ivalid.SetEmpty();
	//mapValid.SetEmpty();
	// Always have to iterate backwards when deleting references.
	for (int i = NUM_SUBMATERIALS - 1; i >= 0; i--)
	{
		if( submtl[i] )
		{
			DeleteReference(i);
			DbgAssert(submtl[i] == nullptr);
			submtl[i] = nullptr;
		}
		//mapOn[i] = FALSE;
	}
	for (int i = NUM_SUBTEXTURES - 1; i >= 0; i--) {
		if (subtexture[i])
		{
			DeleteReference(i);
			DbgAssert(subtexture[i] == nullptr);
			subtexture[i] = nullptr;
		}
	}
	DeleteReference(PBLOCK_REF);

	GetLux_MixDesc()->MakeAutoParamBlocks(this);
}



ParamDlg* Lux_Mix::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLux_MixDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
}

BOOL Lux_Mix::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

Interval Lux_Mix::Validity(TimeValue t)
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
	pblock->GetValue(pb_spin,t,u,valid);
	return valid;
}

/*===========================================================================*\
 |	Sub-anim & References support
\*===========================================================================*/

RefTargetHandle Lux_Mix::GetReference(int i)
{
	//mprintf(_T("\n GetReference Nubmer is : %i \n"), i);
	/*switch (i)
	{
		case 0: return submtl[i]; break;
		case 1: return pblock; break;
		default: return subtexture[i-2]; break;
	}*/
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	else if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return subtexture[i - NUM_SUBMATERIALS];
	else
		return nullptr;
}

void Lux_Mix::SetReference(int i, RefTargetHandle rtarg)
{
	//mprintf(_T("\n SetReference Nubmer is : -> %i \n"), i);
	/*switch (i)
	{
		case 0: submtl[i] = (Mtl *)rtarg; break;
		case 1: pblock = (IParamBlock2 *)rtarg; break;
		default: subtexture[i-2] = (Texmap *)rtarg; break;
	}*/
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		submtl[i] = (Mtl *)rtarg;
	else if ((i >= 0) && (i < NUM_SUBTEXTURES))
		subtexture[i - NUM_SUBMATERIALS] = (Texmap *)rtarg;
	else
		pblock = (IParamBlock2 *)rtarg;
}

TSTR Lux_Mix::SubAnimName(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return GetSubMtlTVName(i);
	else
		return GetSubTexmapTVName(i- NUM_SUBMATERIALS);
}

Animatable* Lux_Mix::SubAnim(int i)
{
	/*switch (i)
	{
		case 0: return submtl[i];
		case 1: return pblock;
		default: return subtexture[i - 2];
	}*/
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	else if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return subtexture[i - NUM_SUBMATERIALS];
	else
		return nullptr;
}

RefResult Lux_Mix::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, 
	PartID& /*partID*/, RefMessage message, BOOL /*propagate*/ ) 
{
	switch (message) {
	case REFMSG_CHANGE:
		{
		ivalid.SetEmpty();
		//mapValid.SetEmpty();
			if (hTarget == pblock)
			{
				ParamID changing_param = pblock->LastNotifyParamID();
				Lux_Mix_param_blk.InvalidateUI(changing_param);
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

Mtl* Lux_Mix::GetSubMtl(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	return 
		nullptr;
}

void Lux_Mix::SetSubMtl(int i, Mtl* m)
{
	//mprintf(_T("\n SetSubMtl Nubmer is : %i \n"), i);
	ReplaceReference(i , m);
	/*switch (i)
	{
		case 0:
			Lux_Mix_param_blk.InvalidateUI(mtl_mat1);
			ivalid.SetEmpty();
			mapValid.SetEmpty();
			break;
		case 1:
			Lux_Mix_param_blk.InvalidateUI(mtl_mat2);
			ivalid.SetEmpty();
			mapValid.SetEmpty();
			break;
	}*/
}

TSTR Lux_Mix::GetSubMtlSlotName(int i)
{
	// Return i'th sub-material name
	switch (i)
	{
		case 0:
			return _T("Material 1");
		case 1:
			return _T("Material 2");
		default:
			return _T("");
	}
}

TSTR Lux_Mix::GetSubMtlTVName(int i)
{
	return GetSubMtlSlotName(i);
}

/*===========================================================================*\
 |	Texmap get and set
\*===========================================================================*/

Texmap* Lux_Mix::GetSubTexmap(int i)
{
	//mprintf(_T("\n GetSubTexmap Nubmer ----------->>>  is : Get %i \n"), i);
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return subtexture[i];
	return
		nullptr;
}

void Lux_Mix::SetSubTexmap(int i, Texmap* tx)
{
	//mprintf(_T("\n SetSubTexmap Nubmer ----------->>>  is : %i \n"), i);
	ReplaceReference(i + NUM_SUBMATERIALS, tx);
	/*switch (i)
	{
		case 0:
			Lux_Mix_param_blk.InvalidateUI(mtl_map);
			mapValid.SetEmpty();
			break;
	}*/
}

TSTR Lux_Mix::GetSubTexmapSlotName(int i)
{
	switch (i)
	{
		case 0:
			return _T("Amount map");
		case 1:
			return _T("Bump map");
		case 2:
			return _T("Normal map");
		case 3:
			return _T("Interior map");
		case 4:
			return _T("Exterior map");
		case 5:
			return _T("Emission color map");
		case 6:
			return _T("emission map");
		case 7:
			return _T("emission ies file");
		default:
			return _T("");
	}
}

TSTR Lux_Mix::GetSubTexmapTVName(int i)
{
	// Return i'th sub-texture name
	return GetSubTexmapSlotName(i);
}



/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define PARAM2_CHUNK 0x1010

IOResult Lux_Mix::Save(ISave* isave)
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK)
		return res;
	isave->EndChunk();

	return IO_OK;
}

IOResult Lux_Mix::Load(ILoad* iload)
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

RefTargetHandle Lux_Mix::Clone(RemapDir &remap)
{
	Lux_Mix *mnew = new Lux_Mix(FALSE);
	*((MtlBase*)mnew) = *((MtlBase*)this);
	// First clone the parameter block
	mnew->ReplaceReference(PBLOCK_REF,remap.CloneRef(pblock));
	// Next clone the sub-materials
	mnew->ivalid.SetEmpty();
	//mnew->mapValid.SetEmpty();
	for (int i = 0; i < NUM_SUBMATERIALS; i++) 
	{
		mnew->submtl[i] = nullptr;
		mnew->subtexture[i] = nullptr;
		if (submtl[i])
			mnew->ReplaceReference(i,remap.CloneRef(submtl[i]));
		if (subtexture[i])
			mnew->ReplaceReference(i + NUM_SUBMATERIALS, remap.CloneRef(subtexture[i]));
		//mnew->mapOn[i] = mapOn[i];
	}
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
	}

void Lux_Mix::NotifyChanged()
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void Lux_Mix::Update(TimeValue t, Interval& valid)
{
	if (!ivalid.InInterval(t))
	{

		ivalid.SetInfinite();
		//pblock->GetValue( mtl_mat1_on, t, mapOn[0], ivalid);
		pblock->GetValue( pb_spin, t, spin, ivalid);

		for (int i=0; i < NUM_SUBMATERIALS; i++)
		{
			if (submtl[i])
				submtl[i]->Update(t,ivalid);
		}
		for (int i = 0; i<NUM_SUBTEXTURES; i++) {
			if (subtexture[i])
				subtexture[i]->Update(t, ivalid);
		}
	}

	valid &= ivalid;
}

/*===========================================================================*\
 |	Determine the characteristics of the material
\*===========================================================================*/

void Lux_Mix::SetAmbient(Color /*c*/, TimeValue /*t*/) {}		
void Lux_Mix::SetDiffuse(Color /*c*/, TimeValue /*t*/) {}		
void Lux_Mix::SetSpecular(Color /*c*/, TimeValue /*t*/) {}
void Lux_Mix::SetShininess(float /*v*/, TimeValue /*t*/) {}

Color Lux_Mix::GetAmbient(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue(prm_ambientcolor, GetCOREInterface()->GetTime(), p, ivalid);
	return submtl[0] ? submtl[0]->GetAmbient(mtlNum, backFace) : Color(p.x, p.y, p.z);//Bound(Color(p.x, p.y, p.z));
}

Color Lux_Mix::GetDiffuse(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue(prm_color, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetDiffuse(mtlNum, backFace) : Color(p.x, p.y, p.z);
}

Color Lux_Mix::GetSpecular(int mtlNum, BOOL backFace)
{
	Point3 p;
	pblock->GetValue(prm_spec, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetSpecular(mtlNum,backFace): Color(p.x, p.y, p.z);
}

float Lux_Mix::GetXParency(int mtlNum, BOOL backFace)
{
	float t;
	pblock->GetValue(pb_opacity, 0, t, ivalid);
	return submtl[0] ? submtl[0]->GetXParency(mtlNum,backFace): t;
}

float Lux_Mix::GetShininess(int mtlNum, BOOL backFace)
{
	float sh;
	pblock->GetValue(pb_shin, 0, sh, ivalid);
	return submtl[0] ? submtl[0]->GetShininess(mtlNum,backFace): sh;
}

float Lux_Mix::GetShinStr(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShinStr(mtlNum,backFace): 0.0f;
}

float Lux_Mix::WireSize(int mtlNum, BOOL backFace)
{
	float wf;
	pblock->GetValue(pb_wiresize, 0, wf, ivalid);
	return submtl[0] ? submtl[0]->WireSize(mtlNum, backFace) : wf;
}


/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void Lux_Mix::Shade(ShadeContext& sc)
{
	Mtl* subMaterial = submtl[0]; // mapOn[0] ? submtl[0] : nullptr;
	if (gbufID)
		sc.SetGBufferID(gbufID);

	if(subMaterial)
		subMaterial->Shade(sc);
	// TODO: compute the color and transparency output returned in sc.out.
}

float Lux_Mix::EvalDisplacement(ShadeContext& sc)
{
	Mtl* subMaterial = submtl[0]; // mapOn[0] ? submtl[0] : nullptr;
	return (subMaterial) ? subMaterial->EvalDisplacement(sc) : 0.0f;
}

Interval Lux_Mix::DisplacementValidity(TimeValue t)
{
	Mtl* subMaterial = submtl[0]; // mapOn[0] ? submtl[0] : nullptr;

	Interval iv;
	iv.SetInfinite();
	if(subMaterial) 
		iv &= subMaterial->DisplacementValidity(t);

	return iv;
}


