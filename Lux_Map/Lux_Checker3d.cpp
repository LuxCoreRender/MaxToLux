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

#define LUX_CHECKER3D_CLASS_ID	Class_ID(0x96107ce, 0x344a3fbb)

#define NSUBTEX		2 // TODO: number of sub-textures supported by this plugin
// Reference Indexes
#define COORD_REF	0
#define PBLOCK_REF	1

class Lux_Checker3d;

class Lux_Checker3d : public Tex3D
{
public:
	//Constructor/Destructor
	Lux_Checker3d();
	virtual ~Lux_Checker3d();

	//From MtlBase
	virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);
	virtual BOOL      SetDlgThing(ParamDlg* dlg);
	virtual void      Update(TimeValue t, Interval& valid);
	virtual void      Reset();
	virtual Interval  Validity(TimeValue t);
	virtual ULONG     LocalRequirements(int subMtlNum);

	//TODO: Return the number of sub-textures
	virtual int NumSubTexmaps() { return NSUBTEX; }
	//TODO: Return the pointer to the 'i-th' sub-texmap
	virtual Texmap* GetSubTexmap(int i) { return subtex[i]; }
	virtual void SetSubTexmap(int i, Texmap *m);
	virtual TSTR GetSubTexmapSlotName(int i);

	//From Texmap
	virtual RGBA EvalColor(ShadeContext& sc);
	virtual float EvalMono(ShadeContext& sc);
	virtual Point3 EvalNormalPerturb(ShadeContext& sc);

	virtual XYZGen *GetTheXYZGen() { return xyzGen; }

	//TODO: Return anim index to reference index
	virtual int SubNumToRefNum(int subNum) { return subNum; }

	//TODO: If your class is derived from Tex3D then you should also
	//implement ReadSXPData for 3D Studio/DOS SXP texture compatibility
	virtual void ReadSXPData(const TCHAR* /*name*/ , void* /*sxpdata*/) { }

	// Loading/Saving
	virtual IOResult Load(ILoad *iload);
	virtual IOResult Save(ISave *isave);

	//From Animatable
	virtual Class_ID ClassID() {return LUX_CHECKER3D_CLASS_ID;}
	virtual SClass_ID SuperClassID() { return TEXMAP_CLASS_ID; }
	virtual void GetClassName(TSTR& s) {s = GetString(IDS_CLASS_CHECKER3D);}

	virtual int NumSubs() { return 1+NSUBTEX; }
	virtual Animatable* SubAnim(int i);
	virtual TSTR SubAnimName(int i);

	// TODO: Maintain the number or references here
	virtual int NumRefs() { return 2+NSUBTEX; }
	virtual RefTargetHandle GetReference(int i);
	virtual RefTargetHandle Clone( RemapDir &remap );
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	virtual int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
	virtual IParamBlock2* GetParamBlock(int /*i*/) { return pblock; } // return i'th ParamBlock
	virtual IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	virtual void DeleteThis() { delete this; }

protected:
	virtual void SetReference(int i, RefTargetHandle rtarg);

private:
	// References
	XYZGen*          xyzGen;          // ref 0
	IParamBlock2*    pblock;          // ref 1
	Texmap*          subtex[NSUBTEX]; // Reference array of sub-materials

	static ParamDlg* xyzGenDlg;
	Interval         ivalid;
};



class Lux_Checker3dClassDesc : public ClassDesc2 
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	, public IMaterialBrowserEntryInfo
#endif
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new Lux_Checker3d(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_CHECKER3D); }
	virtual SClass_ID SuperClassID() 				{ return TEXMAP_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return LUX_CHECKER3D_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

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

	virtual const TCHAR* InternalName() 			{ return _T("Lux_Checker3d"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle

};


ClassDesc2* GetLux_Checker3dDesc() { 
	static Lux_Checker3dClassDesc Lux_Checker3dDesc;
	return &Lux_Checker3dDesc; 
}


enum { Lux_Checker3d_params };


//TODO: Add enums for various parameters
enum {
	texture1,
	texture1_map,
	texture2,
	texture2_map,
	pb_coords,
};


static ParamBlockDesc2 Lux_Checker3d_param_blk ( Lux_Checker3d_params, _T("params"),  0, GetLux_Checker3dDesc(), 
	P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF, 
	//rollout
	IDD_CHECKER3D_PANEL, IDS_CHECKER3D_PARAMS, 0, 0, NULL,
	// params
	texture1, _T("checker texture 1"), TYPE_RGBA, P_ANIMATABLE, IDS_CHECKER_TEXTURE1_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, TYPE_COLORSWATCH, IDC_CHECKER3D_TEXTURE1_COLOR,
		p_end,

	texture1_map, _T("checker texture 1 map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_CHECKER_TEXTURE1_MAP,
		p_refno, 2,
		p_subtexno, 0,
		p_ui, TYPE_TEXMAPBUTTON, IDC_CHECKER3D_TEXTURE1_MAP,
		p_end,

	texture2, _T("checker texture 2"), TYPE_RGBA, P_ANIMATABLE, IDS_CHECKER_TEXTURE2_COLOR,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, TYPE_COLORSWATCH, IDC_CHECKER3D_TEXTURE2_COLOR,
		p_end,

	texture2_map, _T("checker texture 2 map"), TYPE_TEXMAP, P_OWNERS_REF, IDS_CHECKER_TEXTURE2_MAP,
		p_refno, 3,
		p_subtexno, 1,
		p_ui, TYPE_TEXMAPBUTTON, IDC_CHECKER3D_TEXTURE2_MAP,
		p_end,

	pb_coords,			_T("coords"),		TYPE_REFTARG,	P_OWNERS_REF,	IDS_COORDS,
		p_refno,		COORD_REF, 
		p_end,

	p_end
	);


ParamDlg* Lux_Checker3d::xyzGenDlg;

//--- Lux_Checker3d -------------------------------------------------------
Lux_Checker3d::Lux_Checker3d()
	: xyzGen(nullptr)
	, pblock(nullptr)
{
	for (int i = 0; i < NSUBTEX; i++)
		subtex[i] = nullptr;
	//TODO: Add all the initializing stuff
	GetLux_Checker3dDesc()->MakeAutoParamBlocks(this);
	Reset();
}

Lux_Checker3d::~Lux_Checker3d()
{

}

//From MtlBase
void Lux_Checker3d::Reset()
{
	if (xyzGen)
		xyzGen->Reset();
	else
		ReplaceReference( COORD_REF, GetNewDefaultXYZGen());
	//TODO: Reset texmap back to its default values
	ivalid.SetEmpty();
}

void Lux_Checker3d::Update(TimeValue /*t*/, Interval& /*valid*/)
{
	//TODO: Add code to evaluate anything prior to rendering
}

Interval Lux_Checker3d::Validity(TimeValue /*t*/)
{
	//TODO: Update ivalid here
	return ivalid;
}

ParamDlg* Lux_Checker3d::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
	IAutoMParamDlg* masterDlg = GetLux_Checker3dDesc()->CreateParamDlgs(hwMtlEdit, imp, this);
	xyzGenDlg = xyzGen->CreateParamDlg(hwMtlEdit, imp);
	masterDlg->AddDlg(xyzGenDlg);
	//TODO: Set the user dialog proc of the param block, and do other initialization
	return masterDlg;
}

BOOL Lux_Checker3d::SetDlgThing(ParamDlg* dlg)
{
	if (dlg == xyzGenDlg)
		xyzGenDlg->SetThing(xyzGen);
	else
		return FALSE;
	return TRUE;
}

void Lux_Checker3d::SetSubTexmap(int i, Texmap *m)
{
	ReplaceReference(i+2,m);
	//TODO Store the 'i-th' sub-texmap managed by the texture
}

TSTR Lux_Checker3d::GetSubTexmapSlotName(int i)
{
	//TODO: Return the slot name of the 'i-th' sub-texmap
	switch (i)
	{
		case 0:
			return TSTR(_T("Texure 1"));
		case 1:
			return TSTR(_T("Texure 2"));
		default:
			return TSTR(_T(""));
	}
}


//From ReferenceMaker
RefTargetHandle Lux_Checker3d::GetReference(int i)
{
	//TODO: Return the references based on the index
	switch (i) {
		case COORD_REF: return xyzGen;
		case PBLOCK_REF: return pblock;
		default: return subtex[i-2];
		}
}

void Lux_Checker3d::SetReference(int i, RefTargetHandle rtarg)
{
	//TODO: Store the reference handle passed into its 'i-th' reference
	switch(i) {
		case COORD_REF: xyzGen = (XYZGen *)rtarg; break;
		case PBLOCK_REF: pblock = (IParamBlock2 *)rtarg; break;
		default: subtex[i-2] = (Texmap *)rtarg; break;
	}
}

//From ReferenceTarget
RefTargetHandle Lux_Checker3d::Clone(RemapDir &remap)
{
	Lux_Checker3d *mnew = new Lux_Checker3d();
	*((MtlBase*)mnew) = *((MtlBase*)this); // copy superclass stuff
	//TODO: Add other cloning stuff
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
}


Animatable* Lux_Checker3d::SubAnim(int i)
{
	//TODO: Return 'i-th' sub-anim
	switch (i) {
		case 0: return xyzGen;
		case 1: return pblock;
		default: return subtex[i-2];
		}
}

TSTR Lux_Checker3d::SubAnimName(int i)
{
	//TODO: Return the sub-anim names
	switch (i) {
		case 0: return GetString(IDS_COORDS);
		case 1: return GetString(IDS_CHECKER3D_PARAMS);
		default: return GetSubTexmapTVName(i-1);
		}
}

RefResult Lux_Checker3d::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle hTarget,PartID& /*partID*/, RefMessage message, BOOL /*propagate*/ )
{
	switch (message)
	{
	case REFMSG_TARGET_DELETED:
		{
			if      (hTarget == xyzGen) { xyzGen = nullptr; }
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

IOResult Lux_Checker3d::Save(ISave* /*isave*/) 
{
	//TODO: Add code to allow plugin to save its data
	return IO_OK;
}

IOResult Lux_Checker3d::Load(ILoad* /*iload*/) 
{
	//TODO: Add code to allow plugin to load its data
	return IO_OK;
}

AColor Lux_Checker3d::EvalColor(ShadeContext& /*sc*/)
{
	//TODO: Evaluate the color of texture map for the context.
	return AColor (0.0f,0.0f,0.0f,0.0f);
}

float Lux_Checker3d::EvalMono(ShadeContext& sc)
{
	//TODO: Evaluate the map for a "mono" channel
	return Intens(EvalColor(sc));
}

Point3 Lux_Checker3d::EvalNormalPerturb(ShadeContext& /*sc*/)
{
	//TODO: Return the perturbation to apply to a normal for bump mapping
	return Point3(0, 0, 0);
}

ULONG Lux_Checker3d::LocalRequirements(int subMtlNum)
{
	//TODO: Specify various requirements for the material
	return xyzGen->Requirements(subMtlNum);
}

