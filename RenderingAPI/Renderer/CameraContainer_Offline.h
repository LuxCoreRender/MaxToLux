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

#include "CameraContainer_Base.h"

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

// This is the offline (non active shade) version of ICameraContainer
class CameraContainer_Offline :
    public CameraContainer_Base
{
public:

	CameraContainer_Offline(const UnifiedRenderer& renderer, const IRenderSettingsContainer& render_settings_container);
    ~CameraContainer_Offline();

    // Sets up parameters specific to each frame
    void setup_frame_params(Bitmap* const bitmap, const RendParams& rend_params, const FrameRendParams& frame_rend_params);

protected:
    
private:

};

}}	// namespace
