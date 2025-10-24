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

#include "CameraContainer_Offline.h"

#include <bitmap.h>

// FUBAR
#undef max

namespace Max
{;
namespace RenderingAPI
{;

CameraContainer_Offline::CameraContainer_Offline(
    const UnifiedRenderer& renderer,
    const IRenderSettingsContainer& render_settings_container)

    : CameraContainer_Base(renderer, render_settings_container)
{

}

CameraContainer_Offline::~CameraContainer_Offline()
{

}

void CameraContainer_Offline::setup_frame_params(Bitmap* const bitmap, const RendParams& rend_params, const FrameRendParams& frame_rend_params)
{
    // Setup the bitmap
    set_bitmap_internal(bitmap);

    // Determine region and blow parameters
    const IPoint2 resolution = (bitmap != nullptr) ? IPoint2(bitmap->Width(), bitmap->Height()) : IPoint2(1, 1);
    Box2 new_region = Box2(IPoint2(0, 0), IPoint2(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())); 
    Point2 new_blowup_offset = Point2(0.0f, 0.0f);
    float new_blowup_zoom = 1.0f;

    switch(rend_params.rendType)
    {
    default:
        DbgAssert(false);
        // fall into...
    case RENDTYPE_CROP_SEL:
    case RENDTYPE_NORMAL:
    case RENDTYPE_SELECT:
        // No region or blowup
        break;
    case RENDTYPE_REGION:
    case RENDTYPE_REGION_SEL:
        // Bottom and right values are decremented by 1, since Max specifies the region in an exclusive manner (i.e. last pixel row/column excluded).
        new_region = Box2(IPoint2(frame_rend_params.regxmin, frame_rend_params.regymin), IPoint2(frame_rend_params.regxmax - 1, frame_rend_params.regymax - 1));
        break;
    case RENDTYPE_REGIONCROP:
        {
            // A crop render is a region render when the output bitmap is cropped to cover only the region.
            // We treat it as a blowup render, as that's exactly what it is - only with a reduced resolution.
            const Point2 region_center(
                frame_rend_params.regxmin + ((frame_rend_params.regxmax - frame_rend_params.regxmin) * 0.5f),
                frame_rend_params.regymin + ((frame_rend_params.regymax - frame_rend_params.regymin) * 0.5f));
            const Point2 fullsize_center(
                rend_params.width * 0.5f,
                rend_params.height * 0.5f);
            // The RendParams store the original resolution (whereas the actual resolution of the Bitmap is that of the cropped image)
            new_blowup_offset = region_center - fullsize_center;
            // Zoom enough to preserve the original image dimension. We assume that X and Y are zoomed identically.
            new_blowup_zoom = static_cast<float>(rend_params.width) / resolution.x;
        }
        break;
    case RENDTYPE_BLOWUP:
        new_blowup_offset = Point2(
            (frame_rend_params.blowupCenter.x - (rend_params.width * 0.5f)) * frame_rend_params.blowupFactor.x,
            (frame_rend_params.blowupCenter.y - (rend_params.height * 0.5f)) * frame_rend_params.blowupFactor.y);
        new_blowup_zoom = (frame_rend_params.blowupFactor.x + frame_rend_params.blowupFactor.y) * 0.5f;     // Unify both X and Y factors to force uniform blowup
        break;
    }

    set_region_internal(new_region);
    set_blowup_internal(new_blowup_offset, new_blowup_zoom);
}

}}	// namespace
