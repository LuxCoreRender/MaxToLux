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
//#include "maxscript\maxscript.h"

#define LUX_LIGHT_CLASS_ID	Class_ID(0x5d2f7ac1, 0x7dd93354)

#define PBLOCK_REF 1
#define NUM_SUBMATERIALS 1
#define NUM_SUBTEXTURES 3
#define NUM_REF NUM_SUBTEXTURES + NUM_SUBMATERIALS + PBLOCK_REF // number of refrences supported by this plug-in


class Lux_Light : public Mtl {
public:
	Lux_Light();
	Lux_Light(BOOL loading);
	~Lux_Light();


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


	// Shade calculation
	virtual void     Shade(ShadeContext& sc);

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
	virtual void    SetSubTexmap(int i, Texmap *m);
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
	virtual Class_ID ClassID() {return LUX_LIGHT_CLASS_ID;}
	virtual SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	virtual void GetClassName(TSTR& s, bool localized) {s = GetString(IDS_CLASS_LIGHT);}

	virtual RefTargetHandle Clone( RemapDir &remap );
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	virtual int NumSubs() { return 1+NUM_SUBMATERIALS; }
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
	Mtl*		  submtl[NUM_SUBMATERIALS];  // Fixed size Reference array of sub-materials. (Indexes: 0-(N-1))
	Texmap*       subtexture[NUM_SUBTEXTURES];
	IParamBlock2* pblock;					 // Reference that comes AFTER the sub-materials. (Index: N)

	BOOL          mapOn[NUM_SUBMATERIALS];
	float         spin;
	Interval      ivalid;
	Interval	  mapValid;
};


class Lux_LightClassDesc : public ClassDesc2 
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL loading = FALSE) 		{ return new Lux_Light(loading); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_LIGHT); }
	virtual const TCHAR*  NonLocalizedClassName()	{ return GetString(IDS_CLASS_LIGHT); }
	virtual SClass_ID SuperClassID() 				{ return MATERIAL_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return LUX_LIGHT_CLASS_ID; }
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

	virtual const TCHAR* InternalName() 			{ return _T("Lux_Light"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetLux_LightDesc() { 
	static Lux_LightClassDesc Lux_LightDesc;
	return &Lux_LightDesc; 
}


enum { Lux_Light_params };


//TODO: Add enums for various parameters
enum { 
	emission_color,
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
};


static ParamBlockDesc2 Lux_Light_param_blk ( Lux_Light_params, _T("params"),  0, GetLux_LightDesc(), 
	P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF, 
	//rollout
	IDD_LIGHT_PANEL,		IDS_LIGHT_PARAMS,	0, 0, NULL,

	// params
	emission_color, _T("emission_color"), TYPE_RGBA, P_ANIMATABLE, IDS_EMISSION,
	p_default, Color(1.0f, 1.0f, 1.0f),
	p_ui, TYPE_COLORSWATCH, IDC_EMISSION_COLOR,
	p_end,

	emission_map, _T("emission_map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_MAP,
	p_refno, 2,
	p_subtexno, 0,
	p_ui, TYPE_TEXMAPBUTTON, IDC_EMISSION_MAP,
	p_end,

	emission_power, _T("emission_power"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_POWER,
	p_default, 0.0f,
	p_range, 0.0f, 999.0f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_POWER, IDC_EMISSION_POWER_SPIN, 0.1f,
	p_end,

	emission_efficency, _T("emission_efficency"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_EFFICENCY,
	p_default, 0.0f,
	p_range, 0.0f, 999.0f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_EFFICENCY, IDC_EMISSION_EFFICENCY_SPIN, 0.1f,
	p_end,

	emission_mapfile, _T("emission_mapfile"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_MAPFILE,
	p_refno, 3,
	p_subtexno, 1,
	p_ui, TYPE_TEXMAPBUTTON, IDC_EMISSION_MAPFILE,
	p_end,

	emission_gamma, _T("emission_gamma"), TYPE_FLOAT, P_ANIMATABLE, IDS_EMISSION_GAMMA,
	p_default, 2.2f,
	p_range, 0.0f, 999.0f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_GAMMA, IDC_EMISSION_GAMMA_SPIN, 0.1f,
	p_end,

	emission_iesfile, _T("emission_iesfile"), TYPE_TEXMAP, P_OWNERS_REF, IDS_EMISSION_IESFILE,
	p_refno, 4,
	p_subtexno, 2,
	p_ui, TYPE_TEXMAPBUTTON, IDC_EMISSION_IESFILE,
	p_end,

	emission_flipz, _T("emission_flipz"), TYPE_BOOL, 0, IDS_EMISSION_FLIPZ,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHEKBOX, IDC_EMISSION_FLIPZ,
	p_end,

	emission_samples, _T("emission_samples"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_SAMPLES,
	p_default, -1,
	p_range, -1, 9999,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_SAMPLES, IDC_EMISSION_SAMPLES_SPIN, 1,
	p_end,

	emission_map_width, _T("emission_map_width"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_MAP_WIDTH,
	p_default, 0,
	p_range, 0, 9999,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_MAP_WIDTH, IDC_EMISSION_MAP_WIDTH_SPIN, 1,
	p_end,

	emission_map_height, _T("emission_map_height"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_MAP_HEIGHT,
	p_default, 0,
	p_range, 0, 9999,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_MAP_HEIGHT, IDC_EMISSION_MAP_HEIGHT_SPIN, 1,
	p_end,

	emission_id, _T("emission_id"), TYPE_INT, P_ANIMATABLE, IDS_EMISSION_ID,
	p_default, 0,
	p_range, 0, 9999,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EMISSION_ID, IDC_EMISSION_ID_SPIN, 1,
	p_end,

	enableemission, _T("enableemission"), TYPE_BOOL, 0, IDS_ENABLEEMISSION,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHEKBOX, IDC_ENABLEEMISSION,
	p_end,

	p_end
	);


Lux_Light::Lux_Light()
	: pblock(nullptr)
{
	for (int i=0; i<NUM_SUBMATERIALS; i++) 
		submtl[i] = nullptr;
	for (int i = 0; i < NUM_SUBTEXTURES; i++)
		subtexture[i] = nullptr;

	Reset();
}

Lux_Light::Lux_Light(BOOL loading)
	: pblock(nullptr)
{
	for (int i=0; i<NUM_SUBMATERIALS; i++) 
		submtl[i] = nullptr;
	for (int i = 0; i < NUM_SUBTEXTURES; i++)
		subtexture[i] = nullptr;

	if (!loading)
		Reset();
}

Lux_Light::~Lux_Light()
{
	DeleteAllRefs();
}


void Lux_Light::Reset()
{
	ivalid.SetEmpty();
	mapValid.SetEmpty();
	// Always have to iterate backwards when deleting references.
	for (int i = NUM_SUBMATERIALS - 1; i >= 0; i--) {
		if( submtl[i] ){
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
	}
	DeleteReference(PBLOCK_REF);

	GetLux_LightDesc()->MakeAutoParamBlocks(this);
}



ParamDlg* Lux_Light::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLux_LightDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	// TODO: Set param block user dialog if necessary
	return masterDlg;
}

BOOL Lux_Light::SetDlgThing(ParamDlg* /*dlg*/)
{
	return FALSE;
}

Interval Lux_Light::Validity(TimeValue t)
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
	pblock->GetValue(emission_id, t, u, valid);
	return valid;
}

/*===========================================================================*\
 |	Sub-anim & References support
\*===========================================================================*/

RefTargetHandle Lux_Light::GetReference(int i)
{
	/*if (i > 0)
	{
		switch (i)
		{
			//case 0: return subtexture[i - 2]; break;
			case 1: return pblock; break;
				//case 2: return subtexture[i-2]; break;
			default: return subtexture[i - 2]; break;
		}
	}
	else
		return pblock;*/
	if (i == PBLOCK_REF)
		return pblock;
	else if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	else if ((i >= NUM_SUBMATERIALS) && (i < NUM_SUBTEXTURES))
		return subtexture[i - 2];
	else
		return nullptr;
}

void Lux_Light::SetReference(int i, RefTargetHandle rtarg)
{
	/*switch (i)
	{
		//case 0: subtexture[i] = (Texmap *)rtarg; break;
		case 1: pblock = (IParamBlock2 *)rtarg; break;
		//case 2: subtexture[i-2] = (Texmap *)rtarg; break;
		default: subtexture[i - 2] = (Texmap *)rtarg; break;
	}*/
	if (i == PBLOCK_REF)
		pblock = (IParamBlock2 *)rtarg;
	else if ((i >= 0) && (i < NUM_SUBMATERIALS))
		submtl[i] = (Mtl *)rtarg;
	else if ((i >= NUM_SUBMATERIALS) && (i < NUM_SUBTEXTURES))
		subtexture[i - 2] = (Texmap *)rtarg;
}

TSTR Lux_Light::SubAnimName(int i, bool localized)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return GetSubTexmapTVName(i);
	else 
		return GetSubTexmapTVName(i - 2);
}

Animatable* Lux_Light::SubAnim(int i)
{
	/*switch (i)
	{
		case 0: return subtexture[i];
		case 1: return pblock;
		default: return subtexture[i - 2];
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

RefResult Lux_Light::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget, 
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
				Lux_Light_param_blk.InvalidateUI(changing_param);
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

Mtl* Lux_Light::GetSubMtl(int i)
{
	if ((i >= 0) && (i < NUM_SUBMATERIALS))
		return submtl[i];
	return 
		nullptr;
}

void Lux_Light::SetSubMtl(int i, Mtl* m)
{
	ReplaceReference(i,m);
	switch (i)
	{
		case 0:
			Lux_Light_param_blk.InvalidateUI(emission_map);
			mapValid.SetEmpty();
			break;
		case 1:
			Lux_Light_param_blk.InvalidateUI(emission_mapfile);
			mapValid.SetEmpty();
			break;
		case 2:
			Lux_Light_param_blk.InvalidateUI(emission_iesfile);
			mapValid.SetEmpty();
			break;
	}
}

TSTR Lux_Light::GetSubMtlSlotName(int i, bool localized)
{
	// Return i'th sub-material name
	return submtl[i]->GetName();
	//return _T("");
}

TSTR Lux_Light::GetSubMtlTVName(int i)
{
	return GetSubMtlSlotName(i, false);
}

/*===========================================================================*\
 |	Texmap get and set
 |  By default, we support none
\*===========================================================================*/

Texmap* Lux_Light::GetSubTexmap(int i)
{
	if ((i >= 0) && (i < NUM_SUBTEXTURES))
		return subtexture[i];
	return
		nullptr;
}

void Lux_Light::SetSubTexmap(int i, Texmap* m)
{
	ReplaceReference(i + 2, m);
	switch (i)
	{
		case 0:
			Lux_Light_param_blk.InvalidateUI(emission_map);
			mapValid.SetEmpty();
			break;
		case 1:
			Lux_Light_param_blk.InvalidateUI(emission_mapfile);
			mapValid.SetEmpty();
			break;
		case 2:
			Lux_Light_param_blk.InvalidateUI(emission_iesfile);
			mapValid.SetEmpty();
			break;
	}
}

TSTR Lux_Light::GetSubTexmapSlotName(int i, bool localized)
{
	switch (i)
	{
	case 0:
		return _T("emission_map");
	case 1:
		return _T("emission_mapfile");
	case 2:
		return _T("emission_iesfile");
	default:
		return _T("");
	}
}

TSTR Lux_Light::GetSubTexmapTVName(int i)
{
	// Return i'th sub-texture name
	return GetSubTexmapSlotName(i, false);
}



/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define PARAM2_CHUNK 0x1010

IOResult Lux_Light::Save(ISave* isave)
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK) 
		return res;
	isave->EndChunk();

	return IO_OK;
}

IOResult Lux_Light::Load(ILoad* iload)
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

RefTargetHandle Lux_Light::Clone(RemapDir &remap)
{
	Lux_Light *mnew = new Lux_Light(FALSE);
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

void Lux_Light::NotifyChanged()
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void Lux_Light::Update(TimeValue t, Interval& valid)
{
	if (!ivalid.InInterval(t)) 
	{

		ivalid.SetInfinite();

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

void Lux_Light::SetAmbient(Color /*c*/, TimeValue /*t*/) {}		
void Lux_Light::SetDiffuse(Color /*c*/, TimeValue /*t*/) {}		
void Lux_Light::SetSpecular(Color /*c*/, TimeValue /*t*/) {}
void Lux_Light::SetShininess(float /*v*/, TimeValue /*t*/) {}

Color Lux_Light::GetAmbient(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue(emission_color, GetCOREInterface()->GetTime(), p, ivalid);
	return submtl[0] ? submtl[0]->GetAmbient(mtlNum, backFace) : Color(p.x, p.y, p.z);//Bound(Color(p.x, p.y, p.z));
}

Color Lux_Light::GetDiffuse(int mtlNum, BOOL backFace)
{
	Point3 p;
	//TimeValue t; //Zero for first frame //GetCOREInterface()->GetTime() for every frame
	pblock->GetValue(emission_color, 0, p, ivalid);
	return submtl[0] ? submtl[0]->GetDiffuse(mtlNum, backFace) : Color(p.x, p.y, p.z);
}

Color Lux_Light::GetSpecular(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetSpecular(mtlNum,backFace): Color(1,1,1);
}

float Lux_Light::GetXParency(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetXParency(mtlNum,backFace): 0.0f;
}

float Lux_Light::GetShininess(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShininess(mtlNum,backFace): 1.0f;
}

float Lux_Light::GetShinStr(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->GetShinStr(mtlNum,backFace): 0.0f;
}

float Lux_Light::WireSize(int mtlNum, BOOL backFace)
{
	return submtl[0] ? submtl[0]->WireSize(mtlNum,backFace): 0.0f;
}


/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void Lux_Light::Shade(ShadeContext& sc)
{
	Mtl* subMaterial = mapOn[0] ? submtl[0] : nullptr;
	if (gbufID) 
		sc.SetGBufferID(gbufID);

	if(subMaterial) 
		subMaterial->Shade(sc);
	// TODO: compute the color and transparency output returned in sc.out.
}