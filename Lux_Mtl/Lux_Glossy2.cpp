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

#include <string>
#include "Lux_Materal.h"
//#include <maxscript\maxscript.h>

#define LUX_GlOSSY2_CLASS_ID	Class_ID(0x67b86e70, 0x7de456e1)

// Reference Indexes
// 
#define PBLOCK_REF 1
#define NUM_SUBMATERIALS 1 // number of sub-materials supported by this plug-in
#define NUM_SUBTEXTURES 14 // number of sub-textures supported by this plug-in
#define NUM_REF NUM_SUBTEXTURES + NUM_SUBMATERIALS + PBLOCK_REF // number of refrences supported by this plug-in

static int seed = rand() % 4400 + 3400;

class Lux_Glossy2 : public Mtl {
public:
	Lux_Glossy2();
	Lux_Glossy2(BOOL loading);
	~Lux_Glossy2();


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
	virtual TSTR GetSubMtlSlotName(int i, bool localized);
	virtual TSTR GetSubMtlTVName(int i);

	// SubTexmap access methods
	virtual int     NumSubTexmaps() { return NUM_SUBTEXTURES; }
	virtual Texmap* GetSubTexmap(int i);
	virtual void    SetSubTexmap(int i, Texmap *tx);
#if GET_MAX_RELEASE(VERSION_3DSMAX) < 23900
	virtual TSTR	GetSubTexmapSlotName(int i) { return GetSubTexmapSlotName(i, false); }
#endif
	virtual TSTR	GetSubTexmapSlotName(int i, bool localized);
	virtual TSTR    GetSubTexmapTVName(int i);

	virtual BOOL SetDlgThing(ParamDlg* dlg);

	// Loading/Saving
	virtual IOResult Load(ILoad *iload);
	virtual IOResult Save(ISave *isave);

	// From Animatable
	virtual Class_ID ClassID() {return LUX_GlOSSY2_CLASS_ID;}
	virtual SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	virtual void GetClassName(TSTR& s, bool localized) {s = GetString(IDS_CLASS_GLOSSY);}

	virtual RefTargetHandle Clone( RemapDir &remap );
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	virtual int NumSubs() { return 1 + NUM_SUBTEXTURES; }
	virtual Animatable* SubAnim(int i);
	virtual TSTR SubAnimName(int i, bool localized);

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


class Lux_Glossy2ClassDesc : public ClassDesc2 
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL loading = FALSE) 		{ return new Lux_Glossy2(loading); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_GLOSSY); }
	virtual const TCHAR*  NonLocalizedClassName()	{ return GetString(IDS_CLASS_GLOSSY); }
	virtual SClass_ID SuperClassID() 				{ return MATERIAL_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return LUX_GlOSSY2_CLASS_ID; }
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

	virtual const TCHAR* InternalName() 			{ return _T("Lux_Glossy2"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle

};


ClassDesc2* GetLux_Glossy2Desc() { 
	static Lux_Glossy2ClassDesc Lux_Glossy2Desc;
	return &Lux_Glossy2Desc; 
}

enum { Lux_Glossy2_params };

enum { Glossy_map, Common_Param, Light_emission };


//TODO: Add enums for various parameters
enum 
{
	diffuse,
	diffuseMap,
	specular,
	specularMap,
	uroughnessMap,
	uroughness,
	vroughnessMap,
	vroughness,
	absorption,
	absorptionMap,
	thikness,
	thiknessMap,
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

	transparency_map,
};


static ParamBlockDesc2 Lux_Glossy2_param_blk (
	Lux_Glossy2_params, _T("params"),  0, GetLux_Glossy2Desc(),	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
	3,
	//rollout
	Glossy_map,			IDD_GLOSSY2_PANEL,		IDS_MATTE_PARAMS,	0,	0,	NULL,
	Common_Param,		IDD_COMMON_PANEL,		IDS_COMMON_PARAMS,	0,	0,	NULL,
	Light_emission,		IDD_LIGHT_PANEL,		IDS_LIGHT_PARAMS,	0,	0,	NULL,

	// params
	diffuse,			_T("diffuse color"),			TYPE_RGBA,	P_ANIMATABLE,		IDS_GLOSSY_DIFFUSE_COLOR,
		p_default,		Color(0.5f, 0.5f, 0.5f),
		p_ui, Glossy_map, TYPE_COLORSWATCH,		IDC_GLOSSY_DIFFUSE_COLOR,
		p_end,
	
	diffuseMap, _T("diffuse Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_DIFFUSE_MAP,
		p_refno, 2,
		p_subtexno, 0,
		p_ui, Glossy_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_DIFFUSE_MAP,
		p_end,

	specular, _T("specular color"), TYPE_RGBA, P_ANIMATABLE, IDS_GLOSSY_SPECULAR_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Glossy_map, TYPE_COLORSWATCH, IDC_GLOSSY_SPECULAR_COLOR,
		p_end,

	specularMap, _T("specular Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_SPECULAR_MAP,
		p_refno, 3,
		p_subtexno, 1,
		p_ui, Glossy_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_SPECULAR_MAP,
		p_end,

	uroughness, _T("uroughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_UROUGHNESS,
		p_default, 0.1f,
		p_range, 0.0f, 9999.0f,
		p_ui, Glossy_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_UROUGHNESS, IDC_GLOSSY_UROUGHNESS_SPIN, 0.1f,
		p_end,

	uroughnessMap, _T("uroughnessMap"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_UROUGHNESSMAP,
		p_refno, 4, /*Figure out why it crashes if you start on lower number.*/
		p_subtexno, 2,
		p_ui, Glossy_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_UROUGHNESS_MAP,
		p_end,

	vroughness, _T("vroughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_VROUGHNESS,
		p_default, 0.1f,
		p_range, 0.0f, 9999.0f,
		p_ui, Glossy_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_VROUGHNESS, IDC_GLOSSY_VROUGHNESS_SPIN, 0.1f,
		p_end,

	vroughnessMap, _T("vroughnessMap"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_VROUGHNESSMAP,
		p_refno, 5, /*Figure out why it crashes if you start on lower number.*/
		p_subtexno, 3,
		p_ui, Glossy_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_VROUGHNESS_MAP,
		p_end,

	absorption, _T("absorption"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_ABSORPTION_VALUE,
		p_default, 0.0f,
		p_range, 0.0f, 9999.0f,
		p_ui, Glossy_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_ABSORPTION, IDC_GLOSSY_ABSORPTION_SPIN, 0.1f,
		p_end,

	absorptionMap, _T("absorption Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_ABSORPTION_MAP,
		p_refno, 6, /*Figure out why it crashes if you start on lower number.*/
		p_subtexno, 4,
		p_ui, Glossy_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_ABSORPTION_MAP,
		p_end,

	thikness, _T("thikness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_THIKNESS,
		p_default, 0.0f,
		p_range, 0.0f, 9999.0f,
		p_ui, Glossy_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_THIKNESS, IDC_GLOSSY_THIKNESS_SPIN, 0.1f,
		p_end,

	thiknessMap, _T("thikness Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GLOSSY_THIKNESS_MAP,
		p_refno, 7, /*Figure out why it crashes if you start on lower number.*/
		p_subtexno, 5,
		p_ui, Glossy_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_THIKNESS_MAP,
		p_end,

	// IOR overrides color Ks if both are specified
	index, _T("index"), TYPE_FLOAT, P_ANIMATABLE, IDS_GLOSSY_INDEX,
		p_default, 0.0f,
		p_range, 0.0f, 9999.0f,
		p_ui, Glossy_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_INDEX, IDC_GLOSSY_INDEX_SPIN, 0.1f,
		p_end,

	multibounce, _T("Multibounce"), TYPE_BOOL, 0, IDS_GLOSSY_MULTIBOUNCE,
		p_default, FALSE,
		p_ui, Glossy_map, TYPE_SINGLECHEKBOX, IDC_GLOSSY_MULTIBOUNCE_ON,
		p_end,

	// Common param
	bump_map, _T("Bump Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_BUMP_MAP,
		p_refno, 8,
		p_subtexno, 6,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_BUMP_MAP,
		p_end,

	normal_map, _T("Normal Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_NORMAL_MAP,
		p_refno, 9,
		p_subtexno, 7,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_NORMAL_MAP,
		p_end,

	transparency, _T("Transparency"), TYPE_FLOAT, P_ANIMATABLE, IDS_TRANSPARENCY,
		p_default, 1.0f,
		p_range, 0.01f, 1.0f,
		p_ui, Common_Param, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TRANSPARENCY_EDIT, IDC_TRANSPARENCY_SPIN, 0.01f,
		p_end,

	transparency_map, _T("Transparency map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_TRANSPARENCY,
		p_refno, 10,
		p_subtexno, 8,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_TRANSPARENCY_MAP,
		p_end,

	interior_map, _T("Interior Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_INTERIOR_MAP,
		p_refno, 11,
		p_subtexno, 9,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_INTERIOR_MAP,
		p_end,

	exterior_map, _T("Exterior Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EXTERIOR_MAP,
		p_refno, 12,
		p_subtexno, 10,
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

	/*Light emmission begin*/
	emission, _T("emission_color"), TYPE_RGBA, P_ANIMATABLE, IDS_EMISSION,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Light_emission, TYPE_COLORSWATCH, IDC_EMISSION_COLOR,
		p_end,

	emission_map, _T("emission_map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_MAP,
		p_refno, 13,
		p_subtexno, 11,
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
		p_refno, 14,
		p_subtexno, 12,
		p_ui, Light_emission, TYPE_TEXMAPBUTTON, IDC_EMISSION_MAPFILE,
		p_end,

	emission_gamma, _T("emission_gamma"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_GAMMA,
		p_default, 2.2f,
		p_range, 0.0f, 999.0f,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_GAMMA, IDC_EMISSION_GAMMA_SPIN, 0.1f,
		p_end,

	emission_iesfile, _T("emission_iesfile"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_IESFILE,
		p_refno, 15,
		p_subtexno, 13,
		p_ui, Light_emission, TYPE_TEXMAPBUTTON, IDC_EMISSION_IESFILE,
		p_end,

	emission_flipz, _T("emission_flipz"), TYPE_BOOL, 0, IDS_FLIPZ,
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
		/*Light emmission end*/

	p_end
	);


Lux_Glossy2::Lux_Glossy2()
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

Lux_Glossy2::Lux_Glossy2(BOOL loading)
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

Lux_Glossy2::~Lux_Glossy2()
{
	DeleteAllRefs();
}


void Lux_Glossy2::Reset()
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

	GetLux_Glossy2Desc()->MakeAutoParamBlocks(this);
}



ParamDlg* Lux_Glossy2::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLux_Glossy2Desc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
	
}

BOOL Lux_Glossy2::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

Interval Lux_Glossy2::Validity(TimeValue t)
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

RefTargetHandle Lux_Glossy2::GetReference(int i)
{
	/*switch (i)
	{
		//case 0: return subtexture[i]; break;
		case 0: return pblock; break;
		//case 2: return subtexture[i-2]; break;
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

void Lux_Glossy2::SetReference(int i, RefTargetHandle rtarg)
{
	//mprintf(_T("\n SetReference Nubmer is ------->>>>: %i \n"), i);
	/*switch (i)
	{
		//case 0: subtexture[i] = (Texmap *)rtarg; break;
		case 0: pblock = (IParamBlock2 *)rtarg; break;
		//case 2: subtexture[i-2] = (Texmap *)rtarg; break;
		default: subtexture[i-2] = (Texmap *)rtarg; break;
	}*/
	if (i == PBLOCK_REF)
		pblock = (IParamBlock2 *)rtarg;
	else if ((i >= 0) && (i < NUM_SUBMATERIALS))
		submtl[i] = (Mtl *)rtarg;
	else if ((i >= NUM_SUBMATERIALS) && (i < NUM_SUBTEXTURES))
		subtexture[i - 2] = (Texmap *)rtarg;
}

TSTR Lux_Glossy2::SubAnimName(int i, bool localized)
{
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return GetSubTexmapTVName(i);
	else
		return GetSubTexmapTVName(i-2);
}

Animatable* Lux_Glossy2::SubAnim(int i)
{
	/*switch (i)
	{
		//case 0: return subtexture[i];
		case 0: return pblock;
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

RefResult Lux_Glossy2::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, 
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
				Lux_Glossy2_param_blk.InvalidateUI(changing_param);
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

Mtl* Lux_Glossy2::GetSubMtl(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	return 
		nullptr;
}

void Lux_Glossy2::SetSubMtl(int i, Mtl* m)
{
	//mprintf(_T("\n SetSubMtl Nubmer is : %i \n"), i);
	ReplaceReference(i , m);
	switch (i)
	{
	case 0:
		Lux_Glossy2_param_blk.InvalidateUI(diffuseMap);
		mapValid.SetEmpty();
		break;
	case 1:
		Lux_Glossy2_param_blk.InvalidateUI(specularMap);
		mapValid.SetEmpty();
		break;
	case 2:
		Lux_Glossy2_param_blk.InvalidateUI(uroughnessMap);
		mapValid.SetEmpty();
		break;
	case 3:
		Lux_Glossy2_param_blk.InvalidateUI(vroughnessMap);
		mapValid.SetEmpty();
		break;
	case 4:
		Lux_Glossy2_param_blk.InvalidateUI(absorptionMap);
		mapValid.SetEmpty();
		break;
	case 5:
		Lux_Glossy2_param_blk.InvalidateUI(thiknessMap);
		mapValid.SetEmpty();
		break;
	case 6:
		Lux_Glossy2_param_blk.InvalidateUI(bump_map);
		mapValid.SetEmpty();
		break;
	case 7:
		Lux_Glossy2_param_blk.InvalidateUI(normal_map);
		mapValid.SetEmpty();
		break;
	case 8:
		Lux_Glossy2_param_blk.InvalidateUI(interior_map);
		mapValid.SetEmpty();
		break;
	case 9:
		Lux_Glossy2_param_blk.InvalidateUI(exterior_map);
		mapValid.SetEmpty();
		break;
	case 10:
		Lux_Glossy2_param_blk.InvalidateUI(emission_map);
		mapValid.SetEmpty();
		break;
	case 11:
		Lux_Glossy2_param_blk.InvalidateUI(emission_mapfile);
		mapValid.SetEmpty();
		break;
	case 12:
		Lux_Glossy2_param_blk.InvalidateUI(emission_iesfile);
		mapValid.SetEmpty();
		break;
	}
}

TSTR Lux_Glossy2::GetSubMtlSlotName(int i, bool localized)
{
	// Return i'th sub-material name
	return submtl[i]->GetName();
	//return _T("");
}

TSTR Lux_Glossy2::GetSubMtlTVName(int i)
{
	return GetSubMtlSlotName(i, false);
}

/*===========================================================================*\
 |	Texmap get and set
\*===========================================================================*/

Texmap* Lux_Glossy2::GetSubTexmap(int i)
{
	//mprintf(_T("\n GetSubTexmap Nubmer ::::::::::::===>>>  is : Get %i \n"), i);
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return subtexture[i];
	return
		nullptr;
}

void Lux_Glossy2::SetSubTexmap(int i, Texmap* tx)
{
	//mprintf(_T("\n SetSubTexmap Nubmer ============>>>  is : %i \n"), i);
	ReplaceReference(i + 2, tx);
	/*switch (i)
	{
		case 0:
			Lux_Glossy2_param_blk.InvalidateUI(diffuseMap);
			mapValid.SetEmpty();
			break;
		case 1:
			Lux_Glossy2_param_blk.InvalidateUI(specularMap);
			mapValid.SetEmpty();
			break;
		case 2:
			Lux_Glossy2_param_blk.InvalidateUI(uroughnessMap);
			mapValid.SetEmpty();
			break;
		case 3:
			Lux_Glossy2_param_blk.InvalidateUI(vroughnessMap);
			mapValid.SetEmpty();
			break;
		case 4:
			Lux_Glossy2_param_blk.InvalidateUI(absorptionMap);
			mapValid.SetEmpty();
			break;
		case 5:
			Lux_Glossy2_param_blk.InvalidateUI(thiknessMap);
			mapValid.SetEmpty();
			break;
		case 6:
			Lux_Glossy2_param_blk.InvalidateUI(bump_map);
			mapValid.SetEmpty();
			break;
		case 7:
			Lux_Glossy2_param_blk.InvalidateUI(normal_map);
			mapValid.SetEmpty();
			break;
		case 8:
			Lux_Glossy2_param_blk.InvalidateUI(interior_map);
			mapValid.SetEmpty();
			break;
		case 9:
			Lux_Glossy2_param_blk.InvalidateUI(exterior_map);
			mapValid.SetEmpty();
			break;
		case 10:
			Lux_Glossy2_param_blk.InvalidateUI(emission_map);
			mapValid.SetEmpty();
			break;
		case 11:
			Lux_Glossy2_param_blk.InvalidateUI(emission_mapfile);
			mapValid.SetEmpty();
			break;
		case 12:
			Lux_Glossy2_param_blk.InvalidateUI(emission_iesfile);
			mapValid.SetEmpty();
			break;
	}*/
}

TSTR Lux_Glossy2::GetSubTexmapSlotName(int i, bool localized)
{
	switch (i)
	{
		case 0:
			return _T("Diffuse Map");
		case 1:
			return _T("Specular Map");
		case 2:
			return _T("uroughnessMap");
		case 3:
			return _T("vroughnessMap");
		case 4:
			return _T("Absortion Map");
		case 5:
			return _T("Thikness Map");
		case 6:
			return _T("Bump map");
		case 7:
			return _T("Normal map");
		case 8:
			return _T("Transparent map");
		case 9:
			return _T("Interior map");
		case 10:
			return _T("Exterior map");
		case 11:
			return _T("emission_map");
		case 12:
			return _T("emission_mapfile");
		case 13:
			return _T("emission_iesfile");
		default:
			return _T("");
	}
}

TSTR Lux_Glossy2::GetSubTexmapTVName(int i)
{
	// Return i'th sub-texture name
	return GetSubTexmapSlotName(i, false);
}



/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define PARAM2_CHUNK 0x1010

IOResult Lux_Glossy2::Save(ISave* isave)
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK)
		return res;
	isave->EndChunk();

	return IO_OK;
}

IOResult Lux_Glossy2::Load(ILoad* iload)
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

RefTargetHandle Lux_Glossy2::Clone(RemapDir &remap)
{
	Lux_Glossy2 *mnew = new Lux_Glossy2(FALSE);
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

void Lux_Glossy2::NotifyChanged()
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void Lux_Glossy2::Update(TimeValue t, Interval& valid)
{
	if (!ivalid.InInterval(t))
	{

		ivalid.SetInfinite();
		//pblock->GetValue( mtl_mat1_on, t, mapOn[0], ivalid);
		//pblock->GetValue( pb_spin, t, spin, ivalid);
		
		//pblock->GetValue(thikness, t, spin, ivalid);
		
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

void Lux_Glossy2::SetAmbient(Color /*c*/, TimeValue /*t*/) {}		
void Lux_Glossy2::SetDiffuse(Color /*c*/, TimeValue /*t*/) {}		
void Lux_Glossy2::SetSpecular(Color /*c*/, TimeValue /*t*/) {}
void Lux_Glossy2::SetShininess(float /*v*/, TimeValue /*t*/) {}

Color Lux_Glossy2::GetAmbient(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	//pblock->GetValue(prm_color, GetCOREInterface()->GetTime(), p, ivalid);
	return submtl[0] ? submtl[0]->GetAmbient(mtlNum, backFace) : Color(p.x, p.y, p.z);//Bound(Color(p.x, p.y, p.z));
}

Color Lux_Glossy2::GetDiffuse(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue(diffuse, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetDiffuse(mtlNum, backFace) : Color(p.x, p.y, p.z);
}

Color Lux_Glossy2::GetSpecular(int mtlNum, BOOL backFace)
{
	Point3 p;
	pblock->GetValue(diffuse, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetSpecular(mtlNum,backFace): Color(p.x, p.y, p.z);
}

float Lux_Glossy2::GetXParency(int mtlNum, BOOL backFace)
{
	float t = 0.0f;
	//pblock->GetValue(pb_opacity, 0, t, ivalid);
	return submtl[0] ? submtl[0]->GetXParency(mtlNum,backFace): t;
}

float Lux_Glossy2::GetShininess(int mtlNum, BOOL backFace)
{
	float sh = 1.0f;
	//pblock->GetValue(pb_shin, 0, sh, ivalid);
	return submtl[0] ? submtl[0]->GetShininess(mtlNum,backFace): sh;
}

float Lux_Glossy2::GetShinStr(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShinStr(mtlNum,backFace): 0.0f;
}

float Lux_Glossy2::WireSize(int mtlNum, BOOL backFace)
{
	float wf = 0.0f;
	//pblock->GetValue(pb_wiresize, 0, wf, ivalid);
	return submtl[0] ? submtl[0]->WireSize(mtlNum, backFace) : wf;
}


/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void Lux_Glossy2::Shade(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	if (gbufID)
		sc.SetGBufferID(gbufID);

	if(subMaterial)
		subMaterial->Shade(sc);
	// TODO: compute the color and transparency output returned in sc.out.
}

float Lux_Glossy2::EvalDisplacement(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	return (subMaterial) ? subMaterial->EvalDisplacement(sc) : 0.0f;
}

Interval Lux_Glossy2::DisplacementValidity(TimeValue t)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;

	Interval iv;
	iv.SetInfinite();
	if(subMaterial) 
		iv &= subMaterial->DisplacementValidity(t);

	return iv;
}


