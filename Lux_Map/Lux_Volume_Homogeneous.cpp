/***************************************************************************
 * Copyright 2019-2021 by author Omid Ghotbi "TAO" omidt.gh@gmail.com      *
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

#include "Lux_Map.h"

#include "max.h"
 //#include "dllutilities.h"

#define LUX_VOLUME_HOMO_CLASS_ID	Class_ID(0x199f11f1, 0x5fc03073)

#define NSUBTEX   5 // TODO: number of sub-textures supported by this plugin
// Reference Indexes
#define COORD_REF  0
#define PBLOCK_REF 1
#define TEXTURE1_REF 2
#define TEXTURE2_REF 3
#define TEXTURE3_REF 4
#define TEXTURE4_REF 5
#define TEXTURE5_REF 6

class Lux_Volume_Homo;

namespace
{
	const wchar_t* LuxMixMapFriendlyClassName = L"Lux Volumetric Homogeneous Map";
}

class Lux_Volume_Homo_Sampler : public MapSampler
{
private:
	Lux_Volume_Homo* mTexture;
public:
	Lux_Volume_Homo_Sampler() : mTexture(nullptr) { }
	Lux_Volume_Homo_Sampler(Lux_Volume_Homo *c) { mTexture = c; }
	~Lux_Volume_Homo_Sampler() { }

	void   Set(Lux_Volume_Homo *c) { mTexture = c; }
	AColor Sample(ShadeContext& sc, float u, float v);
	AColor SampleFilter(ShadeContext& sc, float u, float v, float du, float dv);
	float  SampleMono(ShadeContext& sc, float u, float v);
	float  SampleMonoFilter(ShadeContext& sc, float u, float v, float du, float dv);
};


class Lux_Volume_Homo : public Texmap {
public:
	//Constructor/Destructor
	Lux_Volume_Homo();
	virtual ~Lux_Volume_Homo();

	//From MtlBase
	virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp);
	virtual BOOL SetDlgThing(ParamDlg* dlg);
	virtual void Update(TimeValue t, Interval& valid);
	virtual void Reset();
	virtual Interval Validity(TimeValue t);
	virtual ULONG LocalRequirements(int subMtlNum);

	//TODO: Return the number of sub-textures
	virtual int NumSubTexmaps() { return NSUBTEX; }
	//TODO: Return the pointer to the 'i-th' sub-texmap
	virtual Texmap* GetSubTexmap(int i) { return subtex[i]; }
	virtual void SetSubTexmap(int i, Texmap *m);
#if GET_MAX_RELEASE(VERSION_3DSMAX) < 23900
	virtual TSTR GetSubTexmapSlotName(int i) { return GetSubTexmapSlotName(i, false); }
#endif
	virtual TSTR GetSubTexmapSlotName(int i, bool localized);

	//From Texmap
	virtual RGBA   EvalColor(ShadeContext& sc);
	virtual float  EvalMono(ShadeContext& sc);
	virtual Point3 EvalNormalPerturb(ShadeContext& sc);

	//TODO: Returns TRUE if this texture can be used in the interactive renderer
	virtual BOOL SupportTexDisplay() { return FALSE; }
	virtual void ActivateTexDisplay(BOOL onoff);
	virtual DWORD_PTR GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker);
	//TODO: Return UV transformation matrix for use in the viewports
	virtual void GetUVTransform(Matrix3 &uvtrans) { uvGen->GetUVTransform(uvtrans); }
	//TODO: Return the tiling state of the texture for use in the viewports
	virtual int    GetTextureTiling() { return  uvGen->GetTextureTiling(); }
	virtual int    GetUVWSource() { return uvGen->GetUVWSource(); }
	virtual UVGen* GetTheUVGen() { return uvGen; }

	//TODO: Return anim index to reference index
	virtual int SubNumToRefNum(int subNum) { return subNum; }

	// Loading/Saving
	virtual IOResult Load(ILoad *iload);
	virtual IOResult Save(ISave *isave);

	//From Animatable
	virtual Class_ID  ClassID() { return LUX_VOLUME_HOMO_CLASS_ID; }
	virtual SClass_ID SuperClassID() { return TEXMAP_CLASS_ID; }
	virtual void GetClassName(TSTR& s, bool localized) { s = GetString(IDS_CLASS_VOLUME_HOMO); }

	virtual RefTargetHandle Clone(RemapDir &remap);
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);


	virtual int NumSubs() { return 1 + NSUBTEX; }
	virtual Animatable* SubAnim(int i);
	virtual TSTR SubAnimName(int i, bool localized);

	// TODO: Maintain the number or references here
	virtual int NumRefs() { return 2 + NSUBTEX; }
	virtual RefTargetHandle GetReference(int i);


	virtual int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
	virtual IParamBlock2* GetParamBlock(int /*i*/) { return pblock; } // return i'th ParamBlock
	virtual IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	virtual void DeleteThis() { delete this; }

protected:
	virtual void SetReference(int i, RefTargetHandle rtarg);

private:
	UVGen*           uvGen;           // ref 0
	IParamBlock2*    pblock;          // ref 1
	Texmap*          subtex[NSUBTEX]; // Other refs

	static ParamDlg* uvGenDlg;
	Interval         ivalid;
};



class Lux_Volume_HomoClassDesc : public ClassDesc2, public IMtlRender_Compatibility_MtlBase
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() { return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) { return new Lux_Volume_Homo(); }
	virtual const TCHAR *	ClassName() { return GetString(IDS_CLASS_VOLUME_HOMO); }
	const TCHAR*  NonLocalizedClassName() { return GetString(IDS_CLASS_VOLUME_HOMO); }
	virtual SClass_ID SuperClassID() { return TEXMAP_CLASS_ID; }
	virtual Class_ID ClassID() { return LUX_VOLUME_HOMO_CLASS_ID; }
	const TCHAR* Category() { return GetString(IDS_CATEGORY); }

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	FPInterface* GetInterface(Interface_ID id) {
		if (IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE == id) {
			return static_cast<IMaterialBrowserEntryInfo*>(this);
		}
		return ClassDesc2::GetInterface(id);
	}

	const MCHAR* GetEntryName() const { return NULL; }
	const MCHAR* GetEntryCategory() const { return _T("Maps\\lux\\Volume"); }
	Bitmap* GetEntryThumbnail() const { return NULL; }
#endif

	virtual const TCHAR* InternalName() { return _T("Lux_Volume_Homo"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() { return hInstance; }					// returns owning module handle

	bool IsCompatibleWithRenderer(ClassDesc& renderer_class_desc) override;

};

bool Lux_Volume_HomoClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
	// Before 3ds Max 2017, Class_ID::operator==() returned an int.
	return true;
}

ClassDesc2* GetLux_Volume_HomoDesc() {
	static Lux_Volume_HomoClassDesc Lux_Volume_HomoDesc;
	return &Lux_Volume_HomoDesc;
}


enum { Lux_Volume_Homo_params };

enum { Common_Param, Homo_Param };


//TODO: Add enums for various parameters
enum {

	// Common
	IOR,
	IOR_map,
	absorption_color,
	absorption_map,
	absorption_depth,
	emission_color,
	emission_map,
	priority,
	id,

	// Homo
	scattering_color,
	scattering_map,
	scattering_scale,
	asymmetry_color,
	asymmetry_map,
	multiscattering,

};


static ParamBlockDesc2 Lux_Volume_Homo_param_blk(Lux_Volume_Homo_params, _T("params"), 0, GetLux_Volume_HomoDesc(),
	P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
	2,
	//rollout
	Common_Param,	IDD_VOLUME_COMMON_PANEL,		IDS_VOLUME_CLEAR_PARAM,		0,	0,	NULL,
	Homo_Param,		IDD_VOLUME_HOMOGENEOUS_PANEL,	IDS_VOLUME_HOMO_PARAM,		0,	0,	NULL,

	// params
	IOR, _T("volume IOR"), TYPE_FLOAT, P_ANIMATABLE, IDS_VOLUME_IOR,
		p_default, 1.33f,
		p_range, 0.001f, 10.0f,
		p_ui, Common_Param, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_VOLUME_IOR, IDC_VOLUME_IOR_SPIN, 0.01f,
		p_end,

	IOR_map, _T("volume IOR map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_VOLUME_IOR_MAP,
		p_refno, TEXTURE1_REF,
		p_subtexno, 0,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_VOLUME_IOR_MAP,
		p_end,

	absorption_color, _T("volume absorption color"), TYPE_RGBA, P_ANIMATABLE, IDS_VOLUME_ABSORPTION_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Common_Param, TYPE_COLORSWATCH, IDC_VOLUME_ABSORPTION_COLOR,
		p_end,

	absorption_map, _T("volume absorption map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_VOLUME_ABSORPTION_MAP,
		p_refno, TEXTURE2_REF,
		p_subtexno, 1,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_VOLUME_ABSORPTION_MAP,
		p_end,

	absorption_depth, _T("volume absorption depth"), TYPE_FLOAT, P_ANIMATABLE, IDS_VOLUME_ABSORPTION_DEPTH,
		p_default, 0.001f,
		p_range, 0.001f, 1.0f,
		p_ui, Common_Param, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_VOLUME_ABSORPTION_DEPTH, IDC_VOLUME_ABSORPTION_DEPTH_SPIN, 0.001f,
		p_end,

	emission_color, _T("volume emission color"), TYPE_RGBA, P_ANIMATABLE, IDS_VOLUME_EMISSION_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Common_Param, TYPE_COLORSWATCH, IDC_VOLUME_EMISSION_COLOR,
		p_end,

	emission_map, _T("volume emission map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_VOLUME_EMISSION_MAP,
		p_refno, TEXTURE3_REF,
		p_subtexno, 2,
		p_ui, Common_Param, TYPE_TEXMAPBUTTON, IDC_VOLUME_EMISSION_MAP,
		p_end,

	priority, _T("volume priority"), TYPE_INT, P_ANIMATABLE, IDS_VOLUME_PRIORITY,
		p_default, 1,
		p_range, 1, 1000,
		p_ui, Common_Param, TYPE_SPINNER, EDITTYPE_INT, IDC_VOLUME_PRIORITY, IDC_VOLUME_PRIORITY_SPIN, 0.001f,
		p_end,

	id, _T("volume id"), TYPE_INT, P_ANIMATABLE, IDS_VOLUME_ID,
		p_default, 1,
		p_range, 1, 10000,
		p_ui, Common_Param, TYPE_SPINNER, EDITTYPE_INT, IDC_VOLUME_ID, IDC_VOLUME_ID_SPIN, 1,
		p_end,

	// Homo
	scattering_color, _T("volume scattering color"), TYPE_RGBA, P_ANIMATABLE, IDS_VOLUME_HOMO_SCATTERING_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Homo_Param, TYPE_COLORSWATCH, IDC_VOLUME_HOMO_SCATTERING_COLOR,
		p_end,

	scattering_map, _T("volume scattering map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_VOLUME_HOMO_SCATTERING_MAP,
		p_refno, TEXTURE4_REF,
		p_subtexno, 3,
		p_ui, Homo_Param, TYPE_TEXMAPBUTTON, IDC_VOLUME_HOMO_SCATTERING_MAP,
		p_end,

	scattering_scale, _T("volume scattering scale"), TYPE_FLOAT, P_ANIMATABLE, IDS_VOLUME_HOMO_SCATTERING_SCALE,
		p_default, 0.001f,
		p_range, 0.001f, 1.0f,
		p_ui, Homo_Param, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_VOLUME_HOMO_SCATTERING_SCALE, IDC_VOLUME_HOMO_SCATTERING_SCALE_SPIN, 0.001f,
		p_end,

	asymmetry_color, _T("volume asymettery color"), TYPE_RGBA, P_ANIMATABLE, IDS_VOLUME_HOMO_ASYMMETRY_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, Homo_Param, TYPE_COLORSWATCH, IDC_VOLUME_HOMO_ASYMMETRY_COLOR,
		p_end,

	asymmetry_map, _T("volume asymetery map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_VOLUME_HOMO_ASYMMETRY_MAP,
		p_refno, TEXTURE5_REF,
		p_subtexno, 4,
		p_ui, Homo_Param, TYPE_TEXMAPBUTTON, IDC_VOLUME_HOMO_ASYMMETRY_MAP,
		p_end,

	multiscattering, _T("volume multiscattering"), TYPE_BOOL, 0, IDS_VOLUME_HOMO_MULTISCATTERING,
		p_default, FALSE,
		p_ui, Homo_Param, TYPE_SINGLECHEKBOX, IDC_VOLUME_HOMO_MULTISCATTERING,
		p_end,


	/*pb_coords, _T("coords"), TYPE_REFTARG, P_OWNERS_REF, IDS_COORDS,
	p_refno, COORD_REF,
	p_end,*/

	p_end
);


ParamDlg* Lux_Volume_Homo::uvGenDlg;

//--- Lux_Volume_Homo -------------------------------------------------------
Lux_Volume_Homo::Lux_Volume_Homo()
	: pblock(nullptr)
{
	for (int i = 0; i < NSUBTEX; i++)
		subtex[i] = nullptr;
	//TODO: Add all the initializing stuff
	GetLux_Volume_HomoDesc()->MakeAutoParamBlocks(this);
	Reset();
}

Lux_Volume_Homo::~Lux_Volume_Homo()
{

}

//From MtlBase
void Lux_Volume_Homo::Reset()
{
	if (uvGen)
		uvGen->Reset();
	else
		ReplaceReference(0, GetNewDefaultUVGen());
	//TODO: Reset texmap back to its default values
	ivalid.SetEmpty();
}

void Lux_Volume_Homo::Update(TimeValue /*t*/, Interval& /*valid*/)
{
	//TODO: Add code to evaluate anything prior to rendering
}

Interval Lux_Volume_Homo::Validity(TimeValue /*t*/)
{
	//TODO: Update ivalid here
	return ivalid;
}

ParamDlg* Lux_Volume_Homo::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
	IAutoMParamDlg* masterDlg = GetLux_Volume_HomoDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	uvGenDlg = uvGen->CreateParamDlg(hwMtlEdit, imp);
	masterDlg->AddDlg(uvGenDlg);
	//TODO: Set the user dialog proc of the param block, and do other initialization
	return masterDlg;
}

BOOL Lux_Volume_Homo::SetDlgThing(ParamDlg* dlg)
{
	if (dlg == uvGenDlg)
		uvGenDlg->SetThing(uvGen);
	else
		return FALSE;
	return TRUE;
}

void Lux_Volume_Homo::SetSubTexmap(int i, Texmap* m)
{
	ReplaceReference(i + 2, m);
	//TODO Store the 'i-th' sub-texmap managed by the texture
}

TSTR Lux_Volume_Homo::GetSubTexmapSlotName(int i, bool localized)
{
	//TODO: Return the slot name of the 'i-th' sub-texmap
	switch (i)
	{
	case 0:
		return TSTR(_T("IOR"));
	case 1:
		return TSTR(_T("Absorption"));
	case 2:
		return TSTR(_T("Emission"));
	case 3:
		return TSTR(_T("Scattering"));
	case 4:
		return TSTR(_T("Asymmetry"));
	default:
		return TSTR(_T(""));
	}
}


//From ReferenceMaker
RefTargetHandle Lux_Volume_Homo::GetReference(int i)
{
	//TODO: Return the references based on the index
	switch (i)
	{
	case 0: return uvGen;
	case 1: return pblock;
	default: return subtex[i - 2];
		//case 1: return subtex[i];
		//case 2: return subtex[i];
		//default: return nullptr;
	}
}

void Lux_Volume_Homo::SetReference(int i, RefTargetHandle rtarg)
{
	//TODO: Store the reference handle passed into its 'i-th' reference
	switch (i)
	{
	case 0: uvGen = (UVGen *)rtarg; break;
	case 1:	pblock = (IParamBlock2 *)rtarg; break;
	default: subtex[i - 2] = (Texmap *)rtarg; break;
		//case 1: subtex[i-1] = (Texmap *)rtarg; break;
		//case 2:	subtex[i-1] = (Texmap *)rtarg; break;
		//default: nullptr;
	}
}

RefResult Lux_Volume_Homo::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, PartID& /*partID*/, RefMessage message, BOOL /*propagate*/)
{
	switch (message)
	{
	case REFMSG_TARGET_DELETED:
	{
		if (hTarget == uvGen) { uvGen = nullptr; }
		else if (hTarget == pblock) { pblock = nullptr; }
		else
		{
			for (int i = 0; i < NSUBTEX; i++)
			{
				if (subtex[i] == hTarget)
				{
					subtex[i] = nullptr;
					break;
				}
			}
		}
	}
	break;
	}
	return(REF_SUCCEED);
}

//From ReferenceTarget
RefTargetHandle Lux_Volume_Homo::Clone(RemapDir &remap)
{
	Lux_Volume_Homo *mnew = new Lux_Volume_Homo();
	*((MtlBase*)mnew) = *((MtlBase*)this); // copy superclass stuff
	//TODO: Add other cloning stuff
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
}


Animatable* Lux_Volume_Homo::SubAnim(int i)
{
	//TODO: Return 'i-th' sub-anim
	switch (i)
	{
	case 0: return uvGen;
	case 1: return pblock;
	default: return subtex[i - 2];
		//case 1: return subtex[i-2];
		//case 2: return subtex[i-2];
		//default: return nullptr;
	}
}

TSTR Lux_Volume_Homo::SubAnimName(int i, bool localized)
{
	//TODO: Return the sub-anim names
	switch (i)
	{
	case 0: return GetString(IDS_COORDS);
	case 1: return GetString(IDS_CHECKER_PARAMS);
	default: return GetSubTexmapTVName(i - 1);
		//case 1: return GetSubTexmapTVName(i);
		//case 2: return GetSubTexmapTVName(i);
		//default: return nullptr;
	}
}

IOResult Lux_Volume_Homo::Save(ISave* /*isave*/)
{
	//TODO: Add code to allow plug-in to save its data
	return IO_OK;
}

IOResult Lux_Volume_Homo::Load(ILoad* /*iload*/)
{
	//TODO: Add code to allow plug-in to load its data
	return IO_OK;
}

AColor Lux_Volume_Homo::EvalColor(ShadeContext& /*sc*/)
{
	//TODO: Evaluate the color of texture map for the context.
	Point3 p;
	pblock->GetValue(absorption_color, 0, p, ivalid);//ivalid);
	return AColor(p.x, p.y, p.z);
	//return AColor (0.0f,0.0f,0.0f,0.0f);
}

float Lux_Volume_Homo::EvalMono(ShadeContext& sc)
{
	//TODO: Evaluate the map for a "mono" channel
	return Intens(EvalColor(sc));
}

Point3 Lux_Volume_Homo::EvalNormalPerturb(ShadeContext& /*sc*/)
{
	//TODO: Return the perturbation to apply to a normal for bump mapping
	return Point3(0, 0, 0);
}

ULONG Lux_Volume_Homo::LocalRequirements(int subMtlNum)
{
	//TODO: Specify various requirements for the material
	return uvGen->Requirements(subMtlNum);
}

void Lux_Volume_Homo::ActivateTexDisplay(BOOL /*onoff*/)
{
	//TODO: Implement this only if SupportTexDisplay() returns TRUE
}

DWORD_PTR Lux_Volume_Homo::GetActiveTexHandle(TimeValue /*t*/, TexHandleMaker& /*maker*/)
{
	//TODO: Return the texture handle to this texture map
	return 0;
}