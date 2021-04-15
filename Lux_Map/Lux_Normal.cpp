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

#include "Lux_Map.h"
 //#include "dllutilities.h"

#define LUX_NORMAL_CLASS_ID	Class_ID(0x7d03463, 0x5f8965d4)

#define NSUBTEX   1 // TODO: number of sub-textures supported by this plugin
// Reference Indexes
#define COORD_REF  0
#define PBLOCK_REF 1
#define TEXTURE1_REF 2

class Lux_Normal;

class Lux_NormalSampler : public MapSampler
{
private:
	Lux_Normal* mTexture;
public:
	Lux_NormalSampler() : mTexture(nullptr) { }
	Lux_NormalSampler(Lux_Normal *c) { mTexture = c; }
	~Lux_NormalSampler() { }

	void   Set(Lux_Normal *c) { mTexture = c; }
	AColor Sample(ShadeContext& sc, float u, float v);
	AColor SampleFilter(ShadeContext& sc, float u, float v, float du, float dv);
	float  SampleMono(ShadeContext& sc, float u, float v);
	float  SampleMonoFilter(ShadeContext& sc, float u, float v, float du, float dv);
};


class Lux_Normal : public Texmap {
public:
	//Constructor/Destructor
	Lux_Normal();
	virtual ~Lux_Normal();

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
	virtual Class_ID  ClassID() { return LUX_NORMAL_CLASS_ID; }
	virtual SClass_ID SuperClassID() { return TEXMAP_CLASS_ID; }
	virtual void GetClassName(TSTR& s, bool localized) { s = GetString(IDS_CLASS_NORMAL); }

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



class Lux_NormalClassDesc : public ClassDesc2
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() { return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) { return new Lux_Normal(); }
	virtual const TCHAR *	ClassName() { return GetString(IDS_CLASS_NORMAL); }
	const TCHAR*  NonLocalizedClassName() { return GetString(IDS_CLASS_NORMAL); }
	virtual SClass_ID SuperClassID() { return TEXMAP_CLASS_ID; }
	virtual Class_ID ClassID() { return LUX_NORMAL_CLASS_ID; }
	virtual const TCHAR* Category() { return GetString(IDS_CATEGORY); }

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	FPInterface* GetInterface(Interface_ID id) {
		if (IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE == id) {
			return static_cast<IMaterialBrowserEntryInfo*>(this);
		}
		return ClassDesc2::GetInterface(id);
	}

	const MCHAR* GetEntryName() const { return NULL; }
	const MCHAR* GetEntryCategory() const { return _T("Maps\\lux"); }
	Bitmap* GetEntryThumbnail() const { return NULL; }
#endif

	virtual const TCHAR* InternalName() { return _T("Lux_Normal"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() { return hInstance; }					// returns owning module handle

};


ClassDesc2* GetLux_NormalDesc() {
	static Lux_NormalClassDesc Lux_NormalDesc;
	return &Lux_NormalDesc;
}


enum { Lux_Normal_params };


//TODO: Add enums for various parameters
enum {
	value1,
	texture1_map,
	pb_coords,
};


static ParamBlockDesc2 Lux_Normal_param_blk(Lux_Normal_params, _T("params"), 0, GetLux_NormalDesc(),
	P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF,
	//rollout
	IDD_NORMAL_PANEL, IDS_NORMAL_PARAMS, 0, 0, NULL,
	// params
	value1, _T("normal value 1"), TYPE_FLOAT, P_ANIMATABLE, IDS_NORMAL_VALUE,
	p_default, 1.0f,
	p_range, 0.001f, 1.0f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_NORMAL_VALUE, IDC_NORMAL_VALUE_SPIN, 0.001f,
	p_end,

	texture1_map, _T("normal texture 1 map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_NORMAL_TEXTURE_MAP,
	p_refno, TEXTURE1_REF,
	p_subtexno, 0,
	p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_TEXTURE_MAP,
	p_end,

	pb_coords, _T("coords"), TYPE_REFTARG, P_OWNERS_REF, IDS_COORDS,
	p_refno, COORD_REF,
	p_end,

	p_end
);


ParamDlg* Lux_Normal::uvGenDlg;

//--- Lux_Normal -------------------------------------------------------
Lux_Normal::Lux_Normal()
	: pblock(nullptr)
{
	for (int i = 0; i < NSUBTEX; i++)
		subtex[i] = nullptr;
	//TODO: Add all the initializing stuff
	GetLux_NormalDesc()->MakeAutoParamBlocks(this);
	Reset();
}

Lux_Normal::~Lux_Normal()
{

}

//From MtlBase
void Lux_Normal::Reset()
{
	if (uvGen)
		uvGen->Reset();
	else
		ReplaceReference(0, GetNewDefaultUVGen());
	//TODO: Reset texmap back to its default values
	ivalid.SetEmpty();
}

void Lux_Normal::Update(TimeValue /*t*/, Interval& /*valid*/)
{
	//TODO: Add code to evaluate anything prior to rendering
}

Interval Lux_Normal::Validity(TimeValue /*t*/)
{
	//TODO: Update ivalid here
	return ivalid;
}

ParamDlg* Lux_Normal::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
	IAutoMParamDlg* masterDlg = GetLux_NormalDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	uvGenDlg = uvGen->CreateParamDlg(hwMtlEdit, imp);
	masterDlg->AddDlg(uvGenDlg);
	//TODO: Set the user dialog proc of the param block, and do other initialization
	return masterDlg;
}

BOOL Lux_Normal::SetDlgThing(ParamDlg* dlg)
{
	if (dlg == uvGenDlg)
		uvGenDlg->SetThing(uvGen);
	else
		return FALSE;
	return TRUE;
}

void Lux_Normal::SetSubTexmap(int i, Texmap* m)
{
	ReplaceReference(i + 2, m);
	//TODO Store the 'i-th' sub-texmap managed by the texture
}

TSTR Lux_Normal::GetSubTexmapSlotName(int i, bool localized)
{
	//TODO: Return the slot name of the 'i-th' sub-texmap
	switch (i)
	{
	case 0:
		return TSTR(_T("Texure"));
	default:
		return TSTR(_T(""));
	}
}


//From ReferenceMaker
RefTargetHandle Lux_Normal::GetReference(int i)
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

void Lux_Normal::SetReference(int i, RefTargetHandle rtarg)
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

RefResult Lux_Normal::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, PartID& /*partID*/, RefMessage message, BOOL /*propagate*/)
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
RefTargetHandle Lux_Normal::Clone(RemapDir &remap)
{
	Lux_Normal *mnew = new Lux_Normal();
	*((MtlBase*)mnew) = *((MtlBase*)this); // copy superclass stuff
	//TODO: Add other cloning stuff
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
}


Animatable* Lux_Normal::SubAnim(int i)
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

TSTR Lux_Normal::SubAnimName(int i, bool localized)
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

IOResult Lux_Normal::Save(ISave* /*isave*/)
{
	//TODO: Add code to allow plug-in to save its data
	return IO_OK;
}

IOResult Lux_Normal::Load(ILoad* /*iload*/)
{
	//TODO: Add code to allow plug-in to load its data
	return IO_OK;
}

AColor Lux_Normal::EvalColor(ShadeContext& /*sc*/)
{
	//TODO: Evaluate the color of texture map for the context.
	Point3 p;
	pblock->GetValue(value1, 0, p, ivalid);//ivalid);
	return AColor(p.x, p.y, p.z);
	//return AColor (0.0f,0.0f,0.0f,0.0f);
}

float Lux_Normal::EvalMono(ShadeContext& sc)
{
	//TODO: Evaluate the map for a "mono" channel
	return Intens(EvalColor(sc));
}

Point3 Lux_Normal::EvalNormalPerturb(ShadeContext& /*sc*/)
{
	//TODO: Return the perturbation to apply to a normal for bump mapping
	return Point3(0, 0, 0);
}

ULONG Lux_Normal::LocalRequirements(int subMtlNum)
{
	//TODO: Specify various requirements for the material
	return uvGen->Requirements(subMtlNum);
}

void Lux_Normal::ActivateTexDisplay(BOOL /*onoff*/)
{
	//TODO: Implement this only if SupportTexDisplay() returns TRUE
}

DWORD_PTR Lux_Normal::GetActiveTexHandle(TimeValue /*t*/, TexHandleMaker& /*maker*/)
{
	//TODO: Return the texture handle to this texture map
	return 0;
}
