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

#define LUX_GlOSSY_TRANSLUCENT_CLASS_ID	Class_ID(0x24b19e11, 0x1de467e3)

#define PBLOCK_REF 1
#define NUM_SUBMATERIALS 1 // TODO: number of sub-materials supported by this plug-in
#define NUM_SUBTEXTURES 21
#define NUM_REF NUM_SUBTEXTURES + NUM_SUBMATERIALS + PBLOCK_REF // number of refrences supported by this plug-in

static int seed = rand() % 15400 + 14400;

class Lux_GlossyTranslucent : public Mtl {
public:
	Lux_GlossyTranslucent();
	Lux_GlossyTranslucent(BOOL loading);
	~Lux_GlossyTranslucent();


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
	virtual Class_ID ClassID() {return LUX_GlOSSY_TRANSLUCENT_CLASS_ID;}
	virtual SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	virtual void GetClassName(TSTR& s) {s = GetString(IDS_CLASS_GlOSSY_TRANSLUCENT);}

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
	//Interval	  mapValid;
};



class Lux_GlossyTranslucentClassDesc : public ClassDesc2 
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL loading = FALSE) 		{ return new Lux_GlossyTranslucent(loading); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_GlOSSY_TRANSLUCENT); }
	virtual SClass_ID SuperClassID() 				{ return MATERIAL_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return LUX_GlOSSY_TRANSLUCENT_CLASS_ID; }
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

	virtual const TCHAR* InternalName() 			{ return _T("Lux_GlossyTranslucent"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle

};


ClassDesc2* GetLux_Glossy_TranslucentDesc() { 
	static Lux_GlossyTranslucentClassDesc Lux_GlossyTranslucentDesc;
	return &Lux_GlossyTranslucentDesc; 
}


enum { Lux_GlossyTranslucent_params };

enum { Glass_trans_map, Common_Param, Light_emission };

//TODO: Add enums for various parameters
enum 
{
	diffuse,
	diffuse_map,
	transmit,
	transmit_map,
	specular,
	specular_map,
	specular_bf,
	specular_bf_map,
	uroughness,
	uroughness_map,
	uroughness_bf,
	uroughness_bf_map,
	vroughness,
	vroughness_map,
	vroughness_bf,
	vroughness_bf_map,
	absorption,
	absorption_map,
	absorption_bf,
	absorption_bf_map,
	thikness,
	thikness_map,
	thikness_bf,
	thikness_bf_map,
	index,
	index_map,
	index_bf,
	index_bf_map,
	multibounce,
	multibounce_bf,

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


static ParamBlockDesc2 Lux_GlossyTranslucent_param_blk (
	Lux_GlossyTranslucent_params, _T("glossy trans params"),  0, GetLux_Glossy_TranslucentDesc(),	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
	3,
	//rollout
	Glass_trans_map, IDD_GlOSSY_TRANSLUCENT_PANEL, IDS_PARAMS, 0, 0, NULL,
	Common_Param, IDD_COMMON_PANEL, IDS_COMMON_PARAMS, 0, 0, NULL,
	Light_emission, IDD_LIGHT_PANEL, IDS_LIGHT_PARAMS, 0, 0, NULL,

	// params
	diffuse, _T("Diffuse"), TYPE_RGBA, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_DIFFUSE,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Glass_trans_map, TYPE_COLORSWATCH, IDC_GLOSSY_TRANS_DIFFUSE_COLOR,
		p_end,

	diffuse_map, _T("Diffuse map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_DIFFUSE_MAP,
		p_refno, 2,
		p_subtexno, 0,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_DIFFUSE_MAP,
		p_end,

	transmit, _T("Transmit"), TYPE_RGBA, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_TRANSMIT,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Glass_trans_map, TYPE_COLORSWATCH, IDC_GLOSSY_TRANS_TRANSMIT_COLOR,
		p_end,

	transmit_map, _T("Transmit map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_TRANSMIT_MAP,
		p_refno, 3,
		p_subtexno, 1,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_TRANSMIT_MAP,
		p_end,

	specular, _T("Specular"), TYPE_RGBA, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_SPECULAR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Glass_trans_map, TYPE_COLORSWATCH, IDC_GLOSSY_TRANS_SPECULAR_COLOR,
		p_end,

	specular_map, _T("Specular map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_SPECULAR_MAP,
		p_refno, 4,
		p_subtexno, 2,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_SPECULAR_MAP,
		p_end,

	specular_bf, _T("Specular bf"), TYPE_RGBA, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_SPECULAR_BF,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Glass_trans_map, TYPE_COLORSWATCH, IDC_GLOSSY_TRANS_SPECULAR_BF_COLOR,
		p_end,

	specular_bf_map, _T("Specular bf map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_SPECULAR_BF_MAP,
		p_refno, 5,
		p_subtexno, 3,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_SPECULAR_BF_MAP,
		p_end,

	uroughness, _T("uroughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_UROGHNESS,
		p_default, 0.1f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_UROUGHNESS, IDC_GLOSSY_TRANS_UROUGHNESS_SPIN, 0.0f,
		p_end,

	uroughness_map, _T("uroughness map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_UROGHNESS_MAP,
		p_refno, 6,
		p_subtexno, 4,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_UROUGHNESS_MAP,
		p_end,

	uroughness_bf, _T("uroughness bf"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_UROGHNESS_BF,
		p_default, 0.1f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_UROUGHNESS_BF, IDC_GLOSSY_TRANS_UROUGHNESS_BF_SPIN, 0.0f,
		p_end,

	uroughness_bf_map, _T("uroughness bf map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_UROGHNESS_BF_MAP,
		p_refno, 7,
		p_subtexno, 5,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_UROUGHNESS_BF_MAP,
		p_end,

	vroughness, _T("vuroughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_VROGHNESS,
		p_default, 0.1f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_VROUGHNESS, IDC_GLOSSY_TRANS_VROUGHNESS_SPIN, 0.0f,
		p_end,

	vroughness_map, _T("vroughness map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_VROGHNESS_MAP,
		p_refno, 8,
		p_subtexno, 6,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_VROUGHNESS_MAP,
		p_end,

	vroughness_bf, _T("vroughness bf"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_VROGHNESS_BF,
		p_default, 0.1f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_VROUGHNESS_BF, IDC_GLOSSY_TRANS_VROUGHNESS_BF_SPIN, 0.0f,
		p_end,

	vroughness_bf_map, _T("vroughness bf map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_VROGHNESS_BF_MAP,
		p_refno, 9,
		p_subtexno, 7,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_VROUGHNESS_BF_MAP,
		p_end,
	
	absorption, _T("absorption"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_ABSORPTION,
		p_default, 0.1f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_ABSORPTION, IDC_GLOSSY_TRANS_ABSORPTION_SPIN, 0.0f,
		p_end,

	absorption_map, _T("absorption map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_ABSORPTION_MAP,
		p_refno, 10,
		p_subtexno, 8,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_ABSORPTION_MAP,
		p_end,

	absorption_bf, _T("absorption bf"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_ABSORPTION_BF,
		p_default, 0.1f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_ABSORPTION_BF, IDC_GLOSSY_TRANS_ABSORPTION_BF_SPIN, 0.0f,
		p_end,

	absorption_bf_map, _T("absorption bf map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_ABSORPTION_BF_MAP,
		p_refno, 11,
		p_subtexno, 9,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_ABSORPTION_BF_MAP,
		p_end,

	thikness, _T("thikness"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_THIKNESS,
		p_default, 0.0f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_THIKNESS, IDC_GLOSSY_TRANS_THIKNESS_SPIN, 0.0f,
		p_end,

	thikness_map, _T("thikness map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_THIKNESS_MAP,
		p_refno, 12,
		p_subtexno, 10,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_THIKNESS_MAP,
		p_end,
	
	thikness_bf, _T("thikness bf"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_THIKNESS_BF,
		p_default, 0.0f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_THIKNESS_BF, IDC_GLOSSY_TRANS_THIKNESS_BF_SPIN, 0.0f,
		p_end,

	thikness_bf_map, _T("thikness bf map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_THIKNESS_BF_MAP,
		p_refno, 13,
		p_subtexno, 11,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_THIKNESS_BF_MAP,
		p_end,
	
	index, _T("index"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_INDEX,
		p_default, 0.0f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_INDEX, IDC_GLOSSY_TRANS_INDEX_SPIN, 0.0f,
		p_end,

	index_map, _T("index_map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_INDEX_MAP,
		p_refno, 14,
		p_subtexno, 12,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_INDEX_MAP,
		p_end,

	index_bf, _T("index bf"), TYPE_FLOAT, P_ANIMATABLE, IDS_GlOSSY_TRANSLUCENT_INDEX_BF,
		p_default, 0.0f,
		p_range, 0.0f, 999.0f,
		p_ui, Glass_trans_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GLOSSY_TRANS_INDEX_BF, IDC_GLOSSY_TRANS_INDEX_BF_SPIN, 0.0f,
		p_end,

	index_bf_map, _T("index bf map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_GlOSSY_TRANSLUCENT_INDEX_BF_MAP,
		p_refno, 15,
		p_subtexno, 13,
		p_ui, Glass_trans_map, TYPE_TEXMAPBUTTON, IDC_GLOSSY_TRANS_INDEX_BF_MAP,
		p_end,
	
	multibounce, _T("multibounce"), TYPE_BOOL, 0, IDS_GlOSSY_TRANSLUCENT_MULTIBOUNCE,
		p_default, FALSE,
		p_ui, Glass_trans_map, TYPE_SINGLECHEKBOX, IDC_GLOSSY_TRANS_MULTIBOUNCE,
		p_end,

	multibounce_bf, _T("multibounce bf"), TYPE_BOOL, 0, IDS_GlOSSY_TRANSLUCENT_MULTIBOUNCE_BF,
		p_default, FALSE,
		p_ui, Glass_trans_map, TYPE_SINGLECHEKBOX, IDC_GLOSSY_TRANS_MULTIBOUNCE_BF,
		p_end,

		// Common param
	bump_map, _T("Bump Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_BUMP_MAP,
		p_refno, 16,
		p_subtexno, 14,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_BUMP_MAP,
		p_end,

	normal_map, _T("Normal Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_NORMAL_MAP,
		p_refno, 17,
		p_subtexno, 15,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_NORMAL_MAP,
		p_end,

	interior_map, _T("Interior Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_INTERIOR_MAP,
		p_refno, 18,
		p_subtexno, 16,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_INTERIOR_MAP,
		p_end,

	exterior_map, _T("Exterior Map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EXTERIOR_MAP,
		p_refno, 19,
		p_subtexno, 17,
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
		p_refno, 20,
		p_subtexno, 18,
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
		p_refno, 21,
		p_subtexno, 19,
		p_ui, Light_emission, TYPE_TEXMAPBUTTON, IDC_EMISSION_MAPFILE,
		p_end,

	emission_gamma, _T("emission_gamma"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_GAMMA,
		p_default, 2.2f,
		p_range, 0.0f, 999.0f,
		p_ui, Light_emission, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_GAMMA, IDC_EMISSION_GAMMA_SPIN, 0.1f,
		p_end,

	emission_iesfile, _T("emission_iesfile"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_IESFILE,
		p_refno, 22,
		p_subtexno, 20,
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


Lux_GlossyTranslucent::Lux_GlossyTranslucent()
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

Lux_GlossyTranslucent::Lux_GlossyTranslucent(BOOL loading)
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

Lux_GlossyTranslucent::~Lux_GlossyTranslucent()
{
	DeleteAllRefs();
}


void Lux_GlossyTranslucent::Reset()
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

	GetLux_Glossy_TranslucentDesc()->MakeAutoParamBlocks(this);
}



ParamDlg* Lux_GlossyTranslucent::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLux_Glossy_TranslucentDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
	
}

BOOL Lux_GlossyTranslucent::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

Interval Lux_GlossyTranslucent::Validity(TimeValue t)
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
	pblock->GetValue(/*thikness*/0, t, u, valid);
	return valid;
}

/*===========================================================================*\
 |	Sub-anim & References support
\*===========================================================================*/

RefTargetHandle Lux_GlossyTranslucent::GetReference(int i)
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

void Lux_GlossyTranslucent::SetReference(int i, RefTargetHandle rtarg)
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

TSTR Lux_GlossyTranslucent::SubAnimName(int i)
{
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return GetSubTexmapTVName(i);
	else
		return GetSubTexmapTVName(i-2);
}

Animatable* Lux_GlossyTranslucent::SubAnim(int i)
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

RefResult Lux_GlossyTranslucent::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, 
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
				Lux_GlossyTranslucent_param_blk.InvalidateUI(changing_param);
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

Mtl* Lux_GlossyTranslucent::GetSubMtl(int i)
{
	/*if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];*/
	return 
		nullptr;
}

void Lux_GlossyTranslucent::SetSubMtl(int i, Mtl* m)
{
	//mprintf(_T("\n SetSubMtl Nubmer is : %i \n"), i);
	ReplaceReference(i , m);
	/*if (i == 0)
	{
		Lux_GlossyTranslucent_param_blk.InvalidateUI(base_mat);
		mapValid.SetEmpty();
	}*/
}

TSTR Lux_GlossyTranslucent::GetSubMtlSlotName(int /*i*/)
{
	// Return i'th sub-material name
	//return submtl[i]->GetName();
	return _T("");
}

TSTR Lux_GlossyTranslucent::GetSubMtlTVName(int i)
{
	return GetSubMtlSlotName(i);
}

/*===========================================================================*\
 |	Texmap get and set
\*===========================================================================*/

Texmap* Lux_GlossyTranslucent::GetSubTexmap(int i)
{
	//mprintf(_T("\n GetSubTexmap Nubmer ::::::::::::===>>>  is : Get %i \n"), i);
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return subtexture[i];
	return
		nullptr;
}

void Lux_GlossyTranslucent::SetSubTexmap(int i, Texmap* tx)
{
	//mprintf(_T("\n SetSubTexmap Nubmer ============>>>  is : %i \n"), i);
	ReplaceReference(i + 2, tx);
	/*switch (i)
	{
		case 0:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(ks_map);
			mapValid.SetEmpty();
			break;
		case 1:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(ks_bf_map);
			mapValid.SetEmpty();
			break;
		case 2:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(uroughness_map);
			mapValid.SetEmpty();
			break;
		case 3:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(uroughness_bf_map);
			mapValid.SetEmpty();
			break;
		case 4:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(vroughness_map);
			mapValid.SetEmpty();
			break;
		case 5:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(vroughness_bf_map);
			mapValid.SetEmpty();
			break;
		case 6:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(ka_map);
			mapValid.SetEmpty();
			break;
		case 7:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(ka_bf_map);
			mapValid.SetEmpty();
			break;
		case 8:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(d_map);
			mapValid.SetEmpty(); break;
		case 9:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(d_bf_map);
			mapValid.SetEmpty();
			break;
		case 10:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(index_map);
			mapValid.SetEmpty();
			break;
		case 11:
			Lux_GlossyTranslucent_param_blk.InvalidateUI(index_bf_map);
			mapValid.SetEmpty();
			break;
	}*/
}

TSTR Lux_GlossyTranslucent::GetSubTexmapSlotName(int i)
{
	switch (i)
	{
		case 0:
			return _T("Diffuse map");
		case 1:
			return _T("Transmit map");
		case 2:
			return _T("Specular map");
		case 3:
			return _T("Specular map BF");
		case 4:
			return _T("Uroughness");
		case 5:
			return _T("Uroughness BF");
		case 6:
			return _T("Vroughness");
		case 7:
			return _T("Vroughness BF");
		case 8:
			return _T("Absorption");
		case 9:
			return _T("Absorption BF");
		case 10:
			return _T("Thikness");
		case 11:
			return _T("Thikness BF");
		case 12:
			return _T("index");
		case 13:
			return _T("index BF");
		case 14:
			return _T("Bump map");
		case 15:
			return _T("Normal map");
		case 16:
			return _T("Interior map");
		case 17:
			return _T("Exterior map");
		case 18:
			return _T("Emission color map");
		case 19:
			return _T("emission map");
		case 20:
			return _T("emission ies");
		default: return _T("");
	}
}

TSTR Lux_GlossyTranslucent::GetSubTexmapTVName(int i)
{
	// Return i'th sub-texture name
	return GetSubTexmapSlotName(i);
}


/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define PARAM2_CHUNK 0x1010

IOResult Lux_GlossyTranslucent::Save(ISave* isave)
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK)
		return res;
	isave->EndChunk();

	return IO_OK;
}

IOResult Lux_GlossyTranslucent::Load(ILoad* iload)
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

RefTargetHandle Lux_GlossyTranslucent::Clone(RemapDir &remap)
{
	Lux_GlossyTranslucent *mnew = new Lux_GlossyTranslucent(FALSE);
	*((MtlBase*)mnew) = *((MtlBase*)this);
	// First clone the parameter block
	mnew->ReplaceReference(PBLOCK_REF,remap.CloneRef(pblock));
	// Next clone the sub-materials
	mnew->ivalid.SetEmpty();
	//mnew->mapValid.SetEmpty();
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

void Lux_GlossyTranslucent::NotifyChanged()
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void Lux_GlossyTranslucent::Update(TimeValue t, Interval& valid)
{
	if (!ivalid.InInterval(t))
	{

		ivalid.SetInfinite();
		//pblock->GetValue( mtl_mat1_on, t, mapOn[0], ivalid);
		//pblock->GetValue( pb_spin, t, spin, ivalid);
		//pblock->GetValue(thikness, t, spin, ivalid); /////////////////////////////////////////////////////////

		for (int i=0; i < NUM_SUBMATERIALS; i++)
		{
			if (submtl[i])
				submtl[i]->Update(t,ivalid);
		}
		//mapValid.SetInfinite();
		for (int i = 0; i<NUM_SUBTEXTURES; i++) {
			if (subtexture[i])
				subtexture[i]->Update(t, ivalid);
		}
	}

	//valid &= mapValid;
	valid &= ivalid;
}

/*===========================================================================*\
 |	Determine the characteristics of the material
\*===========================================================================*/

void Lux_GlossyTranslucent::SetAmbient(Color /*c*/, TimeValue /*t*/) {}		
void Lux_GlossyTranslucent::SetDiffuse(Color /*c*/, TimeValue /*t*/) {}		
void Lux_GlossyTranslucent::SetSpecular(Color /*c*/, TimeValue /*t*/) {}
void Lux_GlossyTranslucent::SetShininess(float /*v*/, TimeValue /*t*/) {}

Color Lux_GlossyTranslucent::GetAmbient(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	//pblock->GetValue(kd, GetCOREInterface()->GetTime(), p, ivalid);  /////////////////////////////////////////
	return submtl[0] ? submtl[0]->GetAmbient(mtlNum, backFace) : Color(p.x, p.y, p.z);//Bound(Color(p.x, p.y, p.z));
}

Color Lux_GlossyTranslucent::GetDiffuse(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue( diffuse, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetDiffuse(mtlNum, backFace) : Color(p.x, p.y, p.z);
}

Color Lux_GlossyTranslucent::GetSpecular(int mtlNum, BOOL backFace)
{
	Point3 p;
	pblock->GetValue(diffuse, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetSpecular(mtlNum,backFace): Color(p.x, p.y, p.z);
}

float Lux_GlossyTranslucent::GetXParency(int mtlNum, BOOL backFace)
{
	float t = 0.0f;
	//pblock->GetValue(pb_opacity, 0, t, ivalid);
	return submtl[0] ? submtl[0]->GetXParency(mtlNum,backFace): t;
}

float Lux_GlossyTranslucent::GetShininess(int mtlNum, BOOL backFace)
{
	float sh = 1.0f;
	//pblock->GetValue(pb_shin, 0, sh, ivalid);
	return submtl[0] ? submtl[0]->GetShininess(mtlNum,backFace): sh;
}

float Lux_GlossyTranslucent::GetShinStr(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShinStr(mtlNum,backFace): 0.0f;
}

float Lux_GlossyTranslucent::WireSize(int mtlNum, BOOL backFace)
{
	float wf = 0.0f;
	//pblock->GetValue(pb_wiresize, 0, wf, ivalid);
	return submtl[0] ? submtl[0]->WireSize(mtlNum, backFace) : wf;
}


/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void Lux_GlossyTranslucent::Shade(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	if (gbufID)
		sc.SetGBufferID(gbufID);

	if(subMaterial)
		subMaterial->Shade(sc);
	// TODO: compute the color and transparency output returned in sc.out.
}

float Lux_GlossyTranslucent::EvalDisplacement(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	return (subMaterial) ? subMaterial->EvalDisplacement(sc) : 0.0f;
}

Interval Lux_GlossyTranslucent::DisplacementValidity(TimeValue t)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;

	Interval iv;
	iv.SetInfinite();
	if(subMaterial) 
		iv &= subMaterial->DisplacementValidity(t);

	return iv;
}


