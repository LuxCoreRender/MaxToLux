//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <RenderingAPI/Renderer/IEnvironmentContainer.h>

#include <iparamb2.h>

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

class EnvironmentContainer : public IEnvironmentContainer
{
public:

    EnvironmentContainer(const bool loading);
    virtual ~EnvironmentContainer();

    static ClassDesc2& get_class_descriptor();

    // Sets up this environment based on the legacy environment/background texture and color. Will automatically
    // detect whether the given map is an environment or background map.
    void SetLegacyEnvironment(Texmap* const env_tex, const Color background_color);

    void SetBackgroundMode(const BackgroundMode mode);
    void SetBackgroundTexture(Texmap* const texture);
    void SetBackgroundColor(const TimeValue t, const AColor color);
    void SetEnvironmentMode(const EnvironmentMode mode);
    void SetEnvironmentTexture(Texmap* const texture);
    void SetEnvironmentColor(const TimeValue t, const AColor color);

    // -- Inherited from IEnvironmentContainer
    virtual BackgroundMode GetBackgroundMode() const override;
    virtual Texmap* GetBackgroundTexture() const override;
    virtual AColor GetBackgroundColor(const TimeValue t, Interval& validity) const override;
    virtual EnvironmentMode GetEnvironmentMode() const override;
    virtual Texmap* GetEnvironmentTexture() const override;
    virtual AColor GetEnvironmentColor(const TimeValue t, Interval& validity) const override;

    // -- inherited form Texmap
    virtual AColor EvalColor(ShadeContext& sc) override;
    virtual Point3 EvalNormalPerturb(ShadeContext& sc) override;

    // -- inherited from MtlBase
    virtual void Update(TimeValue t, Interval& valid) override;
    virtual void Reset() override;
    virtual Interval Validity(TimeValue t) override;
    virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) override;

    // -- inherited from ReferenceTarget
    virtual RefTargetHandle	Clone( RemapDir	&remap ) override;
    virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate) override;

    // -- inherited from ReferenceMaker
    virtual IOResult Load(ILoad	*iload) override;
    virtual IOResult Save(ISave	*isave) override;
    virtual int NumRefs() override;
    virtual RefTargetHandle	GetReference(int i);
    virtual void SetReference(int	i, RefTargetHandle rtarg);

    // -- inherited from Animatable
    virtual Class_ID ClassID() override;	
    virtual void GetClassName(TSTR&	s) override;
    virtual int NumSubs() override;
    virtual TSTR SubAnimName(int i) override;
    virtual Animatable*	SubAnim(int	i) override;
    virtual int NumParamBlocks() override;
    virtual IParamBlock2* GetParamBlock(int i) override;
    virtual IParamBlock2* GetParamBlockByID(BlockID id) override;
    virtual void DeleteThis() override;

private:

    // Installs the legacy environment map settings onto the param block
    void InstallLegacyEnvironment();

private:

    class ClassDescriptor;
    enum ParameterID;

    // The param block
    static ParamBlockDesc2 m_param_block_desc;
    IParamBlock2* m_param_block;        // reference 0

    // Reference to the legacy environment texture, which we transpose onto this environment object when Update() is called.
    bool m_use_legacy_environment;
    Texmap* m_legacy_environment;
    Color m_legacy_background_color;
};

class EnvironmentContainer::ClassDescriptor : public ClassDesc2
{
public:

    // -- inherited from ClassDesc/ClassDesc2
    virtual const MCHAR* ClassName() override;
    virtual Class_ID ClassID() override;
    virtual	int	IsPublic() override;
    virtual	void* Create(BOOL loading) override;
    virtual	SClass_ID SuperClassID() override;
    virtual	const TCHAR* Category() override;
    virtual	const TCHAR* InternalName() override;
    virtual	HINSTANCE HInstance() override;
};

}}	// namespace

