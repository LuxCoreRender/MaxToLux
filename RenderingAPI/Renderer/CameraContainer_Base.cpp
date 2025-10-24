//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "CameraContainer_Base.h"

// local
#include "Utils.h"

// max sdk
#include <bitmap.h>
#include <units.h>
#include <maxapi.h>
#include <genlight.h>
#include <trig.h>
#include <Scene/IPhysicalCamera.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>

// std includes
#include <limits>
#include <algorithm>

#undef min
#undef max

using namespace MaxSDK;

namespace
{
    bool operator!=(const ViewParams& a, const ViewParams& b)
    {
        return (memcmp(&a, &b, sizeof(a)) != 0);
    }

    inline ViewExp13* GetViewExp13( ViewExp* viewExp ) 
    {
        return (viewExp==NULL?  NULL : reinterpret_cast<ViewExp13*>(viewExp->Execute(ViewExp::kEXECUTE_GET_VIEWEXP_13)));
    }

    template<typename T>
    T minmax(const T& value, const T& min, const T& max)
    {
        DbgAssert(min <= max);
        return std::max(std::min(value, max), min);
    }

    inline void RemoveScaling(Matrix3 &m) 
    {
        for(int i = 0; i < 3; i++) 
            m.SetRow(i, Normalize (m.GetRow(i)));
    }

    inline float zoom_factor_to_aperture_width(const float zoom_factor)
    {
        return zoom_factor * 400.0f;        // 400.0 is used throughout max
    }

    inline float aperture_width_to_zoom_factor(const float aperture_width)
    {
        return aperture_width / 400.0f;     // 400.0 is used throughout max
    }
}

namespace Max
{;
namespace RenderingAPI
{;

CameraContainer_Base::CameraContainer_Base(
    const UnifiedRenderer& renderer,
    const IRenderSettingsContainer& render_settings_container)
    
    : m_renderer(renderer),
    m_render_settings_container(render_settings_container),
    m_explicit_camera_node(nullptr),
    m_view_exp_camera_node(nullptr),
    m_use_explicit_camera(true),
    m_bitmap_resolution(1, 1),
    m_pixel_aspect_ratio(1.0f),
    m_bitmap(nullptr),
    m_region(IPoint2(0, 0), IPoint2(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())),   // infinite region
    m_blowup_image_plane_offset_in_pixels(0.0f, 0.0f),      // no offset
    m_blowup_zoom(1.0f)     // no blowup
{

}

CameraContainer_Base::~CameraContainer_Base()
{

}

void CameraContainer_Base::set_camera_node(INode* const camera_node)
{
    if(camera_node != m_explicit_camera_node)
    {
        m_explicit_camera_node = camera_node;
        Invalidate();
    }
}

void CameraContainer_Base::set_use_camera(const bool use_camera)
{
    if(m_use_explicit_camera != use_camera)
    {
        m_use_explicit_camera = use_camera;
        Invalidate();
    }
}

bool CameraContainer_Base::get_use_camera() const
{
    return m_use_explicit_camera;
}

void CameraContainer_Base::set_view(const ViewParams& view_params)
{
    if(view_params != m_view_params)
    {
        m_view_params = view_params;
        m_view_exp_camera_node = nullptr;   // ViewParams does not specify a camera node
        Invalidate();
    }
}

void CameraContainer_Base::set_view(const ViewExp& view_exp)
{
    // Convert ViewExp to ViewParams
    ViewExp13* vpt = GetViewExp13(const_cast<ViewExp*>(&view_exp));
    DbgAssert(vpt != nullptr);
    if(vpt != nullptr)
    {
        ViewParams new_view_params;
        vpt->GetAffineTM(new_view_params.affineTM);
        new_view_params.projType = vpt->IsPerspView() ? PROJ_PERSPECTIVE : PROJ_PARALLEL;
        new_view_params.zoom = vpt->GetZoom();
        new_view_params.fov = vpt->GetFOV();
        new_view_params.distance = vpt->GetFocalDist();
        new_view_params.hither = 0.0f;
        new_view_params.yon = std::numeric_limits<float>::max();
        new_view_params.nearRange = 0.0f;
        new_view_params.farRange = std::numeric_limits<float>::max();

        INode* const new_view_exp_camera_node = vpt->GetViewCamera();

        if((new_view_params != m_view_params) || (m_view_exp_camera_node != new_view_exp_camera_node))
        {
            m_view_params = new_view_params;
            m_view_exp_camera_node = new_view_exp_camera_node;
            Invalidate();
        }
    }
}

INode* CameraContainer_Base::GetCameraNode() const
{
    return m_use_explicit_camera ? m_explicit_camera_node : m_view_exp_camera_node;
}

IPhysicalCamera* CameraContainer_Base::GetPhysicalCamera(const TimeValue t) const 
{
    INode* camera_node = GetCameraNode();
    if(camera_node != nullptr)
    {
        const ObjectState& object_state = camera_node->EvalWorldState(t);
        IPhysicalCamera* const physical_camera = dynamic_cast<IPhysicalCamera*>(object_state.obj);
        return physical_camera;
    }
    else
    {
        return nullptr;
    }
}

MotionTransforms CameraContainer_Base::EvaluateCameraTransform(const TimeValue t, Interval& validity, const MotionBlurSettings* camera_motion_blur_settings) const
{
    // Re-evaluate transform, using provided motion blur settings, if necessary
    if(camera_motion_blur_settings != nullptr)
    {
        INode* const camera_node = GetCameraNode();
        if(camera_node != nullptr)
        {
            // Ensure to correctly evaluate the transform of the physical camera
            IPhysicalCamera::RenderTransformEvaluationGuard physical_camera_transform_guard(camera_node, t);
            const MotionTransforms transforms = Utils::EvaluateMotionTransformsForNode(*camera_node, t, validity, *camera_motion_blur_settings);
            return transforms;
        }
    }

    // Fallback to what was already evaluated
    update_view_params_cache(t, validity);
    return m_view_params_cache.transform;
}

CameraContainer_Base::ProjectionType CameraContainer_Base::GetProjectionType(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.projection_type;
}

float CameraContainer_Base::GetPerspectiveFOVRadians(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.perspective_fov_radians;
}

float CameraContainer_Base::GetOrthographicApertureWidth(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.orthographic_aperture_width;
}

float CameraContainer_Base::GetOrthographicApertureHeight(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);

    const IPoint2 resolution = GetResolution();
    const float aspect_ratio = GetPixelAspectRatio() * resolution.x / resolution.y;
    return m_view_params_cache.orthographic_aperture_width / aspect_ratio;
}

bool CameraContainer_Base::GetDOFEnabled(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.dof_enabled;
}

float CameraContainer_Base::GetLensFocalLength(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.lens_focal_length;
}

float CameraContainer_Base::GetFocusPlaneDistance(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.focus_distance;
}

float CameraContainer_Base::GetLensApertureRadius(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.aperture_radius;
}

bool CameraContainer_Base::GetClipEnabled(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.clip_enabled;
}

float CameraContainer_Base::GetClipNear(const TimeValue t, Interval& validity) const 
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.clip_near;
}

float CameraContainer_Base::GetClipFar(const TimeValue t, Interval& validity) const 
{
    update_view_params_cache(t, validity);
    return m_view_params_cache.clip_far;
}

void CameraContainer_Base::set_bitmap_internal(Bitmap* const bitmap)
{
    const IPoint2 new_resolution = (bitmap != nullptr) ? IPoint2(bitmap->Width(), bitmap->Height()) : IPoint2(1, 1);
    const float new_aspect = (bitmap != nullptr) ? bitmap->Aspect() : 1.0f;
    if((bitmap != m_bitmap) || (new_resolution != m_bitmap_resolution) || (new_aspect != m_pixel_aspect_ratio))
    {
        m_bitmap = bitmap;
        m_bitmap_resolution = new_resolution;
        m_pixel_aspect_ratio = new_aspect;
        Invalidate();
    }
}

void CameraContainer_Base::set_down_res_factor(const unsigned int factor)
{
    if(factor != m_down_res_factor)
    {
        DbgAssert(factor >= 1);
        m_down_res_factor = factor;
        Invalidate();
    }
}

unsigned int CameraContainer_Base::get_effective_down_res_factor() const
{
    const IPoint2 resolution = get_resolution_ignoring_down_res_factor();
    // Make sure the factor isn't smaller than the render resolution, as that would result in a zero resolution.
    const unsigned int clamped_factor = std::min<unsigned int>(std::min<unsigned int>(m_down_res_factor, resolution.x), resolution.y);
    return clamped_factor;
}

IPoint2 CameraContainer_Base::get_resolution_ignoring_down_res_factor() const
{
    return m_bitmap_resolution;
}

Box2 CameraContainer_Base::get_region_ignoring_down_res_factor() const
{
    const IPoint2 resolution = get_resolution_ignoring_down_res_factor();

    if((m_region.left > m_region.right) || (m_region.top > m_region.bottom))
    {
        // Empty region: treat this as having region turned off (that's how active shade works, anyway)
        return Box2(IPoint2(0, 0), IPoint2(resolution.x - 1, resolution.y - 1));
    }
    else
    {
        // Clip to resolution
        const int left = minmax<int>(m_region.left, 0, resolution.x - 1);
        const int top = minmax<int>(m_region.top, 0, resolution.y - 1);
        const int right = minmax<int>(m_region.right, left, resolution.x - 1);
        const int bottom = minmax<int>(m_region.bottom, top, resolution.y - 1);

        return Box2(IPoint2(left, top), IPoint2(right, bottom));
    }
}

IPoint2 CameraContainer_Base::GetResolution() const
{
    // Apply the down-resolution factor
    const IPoint2 resolution = m_bitmap_resolution / std::max(get_effective_down_res_factor(), 1u);
    return resolution;
}

float CameraContainer_Base::GetPixelAspectRatio() const
{
    return m_pixel_aspect_ratio;
}

Bitmap* CameraContainer_Base::GetBitmap() const
{
    return m_bitmap;
}

void CameraContainer_Base::set_region_internal(const Box2& region)
{
    if(!(m_region == region))
    {
        m_region = region;
        Invalidate();
    }
}

void CameraContainer_Base::set_blowup_internal(const Point2& offset, const float zoom)
{
    if((offset != m_blowup_image_plane_offset_in_pixels) || (zoom != m_blowup_zoom))
    {
        m_blowup_image_plane_offset_in_pixels = offset;
        m_blowup_zoom = zoom;
        Invalidate();
    }
}

Box2 CameraContainer_Base::GetRegion() const
{
    Box2 region = get_region_ignoring_down_res_factor();
    const unsigned int down_res_factor = get_effective_down_res_factor();
    
    // Apply down-res factor
    region.left /= std::max(down_res_factor, 1u);
    region.right /= std::max(down_res_factor, 1u);
    region.top /= std::max(down_res_factor, 1u);
    region.bottom /= std::max(down_res_factor, 1u);

    DbgAssert((region.right <= GetResolution().x) && (region.bottom <= GetResolution().y));

    return region;
}

Point2 CameraContainer_Base::GetImagePlaneOffset(const TimeValue t, Interval& validity) const
{
    update_view_params_cache(t, validity);
    // m_blowup_image_plane_offset_in_pixels doesn't take the down-res factor into account, but m_view_params_cache does
    return (m_blowup_image_plane_offset_in_pixels / std::max(get_effective_down_res_factor(), 1u)) + m_view_params_cache.film_plane_offset;
}

void CameraContainer_Base::Invalidate()
{
    // Invalidate the cache
    m_view_params_cache.validity.SetEmpty();
    notify_change();
}

void CameraContainer_Base::notify_change()
{
    // Notify the clients of the change
    for(auto notifier : m_change_notifiers)
    {
        notifier->NotifyCameraChanged();
    }
}

void CameraContainer_Base::update_view_params_cache(const TimeValue t, Interval& validity) const
{
    if(!m_view_params_cache.validity.InInterval(t))
    {
        // Reset cached values to default, to account for any values that may not get set because they're not available on the camera being used.
        m_view_params_cache = ViewParamsCache();
        m_view_params_cache.validity.SetInfinite(); // start with an infinite validity, to be narrowed as needed

        INode* const camera_node = GetCameraNode();
        if(camera_node != nullptr)
        {
            // Ensure to correctly evaluate the transform of the physical camera
            IPhysicalCamera::RenderTransformEvaluationGuard physical_camera_transform_guard(camera_node, t);

            // Fetch the transform matrix
            // GetObjectTM overwrites the validity, so we need this little trick
            m_view_params_cache.transform_tminus2 = camera_node->GetObjTMAfterWSM(t - 2, nullptr);      // don't care about validity
            const MotionBlurSettings camera_node_motion_blur_settings = Utils::ApplyMotionBlurSettingsFromNode(*camera_node, t, m_view_params_cache.validity, GetGlobalMotionBlurSettings(t, validity), m_renderer);
            m_view_params_cache.transform = Utils::EvaluateMotionTransformsForNode(*camera_node, t, m_view_params_cache.validity, camera_node_motion_blur_settings);

            // Evaluate the object - a camera or light object
            const ObjectState& object_state = camera_node->EvalWorldState(t);
            DbgAssert(object_state.obj != nullptr);
            if(object_state.obj != nullptr)
            {
                DbgAssert((object_state.obj->SuperClassID() == CAMERA_CLASS_ID) || (object_state.obj->SuperClassID() == LIGHT_CLASS_ID));
                if(object_state.obj->SuperClassID() == CAMERA_CLASS_ID)
                {
                    CameraObject* const camera_object = static_cast<CameraObject*>(object_state.obj);
                    const float fov_radians = camera_object->GetFOV(t, m_view_params_cache.validity);
                    const float tdist = camera_object->GetTDist(t, m_view_params_cache.validity);

                    if(camera_object->IsOrtho())
                    {
                        m_view_params_cache.projection_type = ProjectionType::Orthographic;
                        m_view_params_cache.orthographic_aperture_width = 2.0f * tdist * tanf(fov_radians * 0.5f);
                    }
                    else
                    {
                        m_view_params_cache.projection_type = ProjectionType::Perspective;
                        m_view_params_cache.perspective_fov_radians = fov_radians;
                        m_view_params_cache.focus_distance = tdist;
                    }

                    // Clipping ranges
                    {
                        m_view_params_cache.clip_enabled = !!camera_object->GetManualClip();
                        m_view_params_cache.clip_near = m_view_params_cache.clip_enabled ? camera_object->GetClipDist(t, CAM_HITHER_CLIP, m_view_params_cache.validity) : 0.0f;
                        m_view_params_cache.clip_far = m_view_params_cache.clip_enabled ? camera_object->GetClipDist(t, CAM_YON_CLIP, m_view_params_cache.validity) : std::numeric_limits<float>::max();
                        m_view_params_cache.env_near = camera_object->GetEnvRange(t, ENV_NEAR_RANGE, m_view_params_cache.validity);
                        m_view_params_cache.env_far = camera_object->GetEnvRange(t, ENV_FAR_RANGE, m_view_params_cache.validity);
                    }

                    // Evaluate physical camera parameters
                    IPhysicalCamera* const physical_camera = dynamic_cast<IPhysicalCamera*>(object_state.obj);
                    if(physical_camera != nullptr)
                    {
                        m_view_params_cache.film_width = physical_camera->GetFilmWidth(t, m_view_params_cache.validity);
                        m_view_params_cache.lens_focal_length = physical_camera->GetEffectiveLensFocalLength(t, m_view_params_cache.validity);
                        m_view_params_cache.dof_enabled = physical_camera->GetDOFEnabled(t, m_view_params_cache.validity);
                        m_view_params_cache.aperture_radius = physical_camera->GetLensApertureRadius(t, m_view_params_cache.validity, true);
                        m_view_params_cache.film_plane_offset = physical_camera->GetFilmPlaneOffsetInPixels(GetResolution(), m_pixel_aspect_ratio, t, m_view_params_cache.validity);
                        m_view_params_cache.film_plane_offset.x = -m_view_params_cache.film_plane_offset.x; // invert the whole offset, and then invert y -> invert only x
                        m_view_params_cache.focus_distance = physical_camera->GetFocusDistance(t, m_view_params_cache.validity);
                    }
                }
                else if(object_state.obj->SuperClassID() == LIGHT_CLASS_ID)
                {
                    LightObject* const light_object = static_cast<LightObject*>(object_state.obj);

                    LightState light_state;
                    light_object->EvalLightState(t, m_view_params_cache.validity, &light_state);

                    switch(light_state.type) {
                    case SPOT_LGT:			
                        {
                            const float aspect = (light_state.shape == CIRCLE_LIGHT) ? 1.0f : light_state.aspect;
                            m_view_params_cache.projection_type = ProjectionType::Perspective;
                            m_view_params_cache.perspective_fov_radians = 2.0f * atanf(tanf(DegToRad(light_state.fallsize) * 0.5f) * sqrt(aspect));
                            m_view_params_cache.focus_distance = light_object->GetTDist(t, m_view_params_cache.validity);
                        }
                        break;
                    case DIRECT_LGT:
                        {
                            m_view_params_cache.projection_type = ProjectionType::Orthographic;
                            m_view_params_cache.orthographic_aperture_width = 2.0f * std::max(light_state.hotsize, light_state.fallsize);
                        }
                        break;
                    default:
                        DbgAssert(false); 
                        break;
                    }		

                }
            }
        }
        else
        {
            // Use ViewParams
            m_view_params_cache.transform_tminus2 = Inverse(m_view_params.prevAffineTM);
            m_view_params_cache.transform.shutter_open = m_view_params_cache.transform.shutter_close = Inverse(m_view_params.affineTM);
            switch(m_view_params.projType) 
            {
            case PROJ_PERSPECTIVE:
                m_view_params_cache.projection_type = ProjectionType::Perspective;
                m_view_params_cache.perspective_fov_radians = m_view_params.fov;
                m_view_params_cache.focus_distance = m_view_params.distance;
                break;
            case PROJ_PARALLEL:
                m_view_params_cache.projection_type = ProjectionType::Orthographic;
                m_view_params_cache.orthographic_aperture_width = zoom_factor_to_aperture_width(m_view_params.zoom);
                break;
            default:
                DbgAssert(false);
                break;
            }

            // Clipping ranges: ignored for non-camera viewports, as they don't behave correctly: orthographic views are placed at the origin,
            // but we move them outside of the scene bounding box, and respecting the default near range (0.1) causes undefined behaviour... what is
            // that 0.1 value supposed to be defined against? Anyway, things don't work right unless we disable it.
            m_view_params_cache.clip_enabled = false;
            m_view_params_cache.clip_near = 0;
            m_view_params_cache.clip_far = std::numeric_limits<float>::max();
            m_view_params_cache.env_near = 0;
            m_view_params_cache.env_far = std::numeric_limits<float>::max();
        }

        // Apply blowup zoom
        const float camera_scale = 1.0f / m_blowup_zoom;
        m_view_params_cache.transform.shutter_open.PreScale(Point3(camera_scale, camera_scale, 1.0f));    
        m_view_params_cache.transform.shutter_close.PreScale(Point3(camera_scale, camera_scale, 1.0f));    

        // Derive default values for unevaluated parameters
        if(m_view_params_cache.film_width == 0.0f)
        {
            m_view_params_cache.film_width = GetCOREInterface()->GetRendApertureWidth() / GetMasterScale(UNITS_MILLIMETERS);
        }
        if((m_view_params_cache.lens_focal_length == 0.0f) && (m_view_params_cache.projection_type == ProjectionType::Perspective))
        {
            m_view_params_cache.lens_focal_length = (0.5f * m_view_params_cache.film_width) / tanf(m_view_params_cache.perspective_fov_radians * 0.5f);
        }

        // Setup the render view
        m_view_params_cache.render_view.update(*this, m_render_settings_container);
    }

    validity &= m_view_params_cache.validity;
}

void CameraContainer_Base::camera_node_deleted(INode& deleted_node) 
{
    // Check if this node is being used
    if(&deleted_node == m_explicit_camera_node)
    {
        m_explicit_camera_node = nullptr;
        Invalidate();
    }
    else if(&deleted_node == m_view_exp_camera_node)
    {
        m_view_exp_camera_node = nullptr;
        Invalidate();
    }
}

const View& CameraContainer_Base::GetView(const TimeValue t, Interval& validity) const
{
    // Update the cached parameters
    update_view_params_cache(t, validity);
    
    return m_view_params_cache.render_view;
}

ViewParams CameraContainer_Base::GetViewParams(const TimeValue t, Interval& validity) const
{
    // Update the cached parameters
    update_view_params_cache(t, validity);

    if(GetCameraNode() != nullptr)
    {
        // Initialize view params from camera parameters
        ViewParams view_params;
        view_params.affineTM = Inverse(m_view_params_cache.transform.shutter_open);
        view_params.prevAffineTM = Inverse(m_view_params_cache.transform_tminus2);
        view_params.projType = (m_view_params_cache.projection_type == ProjectionType::Perspective) ? PROJ_PERSPECTIVE : PROJ_PARALLEL;
        view_params.distance = m_view_params_cache.focus_distance;
        view_params.zoom = aperture_width_to_zoom_factor(m_view_params_cache.orthographic_aperture_width);
        view_params.fov = m_view_params_cache.perspective_fov_radians;
        view_params.hither = m_view_params_cache.clip_near;
        view_params.yon = m_view_params_cache.clip_far;
        view_params.nearRange = m_view_params_cache.env_near;
        view_params.farRange = m_view_params_cache.env_far;
        return view_params;
    }
    else
    {
        // Copy the existing view params 
        return m_view_params;
    }
}

MotionBlurSettings CameraContainer_Base::GetGlobalMotionBlurSettings(const TimeValue t, Interval& validity) const
{
    IPhysicalCamera* const physical_camera = GetPhysicalCamera(t);
    if((physical_camera != nullptr) && physical_camera->GetMotionBlurEnabled(t, validity))
    {
        return MotionBlurSettings(
            physical_camera->GetShutterDurationInFrames(t, validity) * GetTicksPerFrame(),
            physical_camera->GetShutterOffsetInFrames(t, validity) * GetTicksPerFrame());
    }
    else
    {
        return MotionBlurSettings();
    }
}

void CameraContainer_Base::RegisterChangeNotifier(IChangeNotifier& notifier) const
{
    m_change_notifiers.push_back(&notifier);
}

void CameraContainer_Base::UnregisterChangeNotifier(IChangeNotifier& notifier) const
{
    for(auto it = m_change_notifiers.begin(); it != m_change_notifiers.end(); ++it)
    {
        if(*it == &notifier)
        {
            m_change_notifiers.erase(it);
            return;
        }
    }

    // Shouldn't get here
    DbgAssert(false);
}

//==================================================================================================
// class CameraContainer_Base::RenderView
//==================================================================================================

CameraContainer_Base::RenderView::RenderView()
    : m_x_center(0.0f),
    m_y_center(0.0f),
    m_x_scale(0.0f),
    m_y_scale(0.0f)
{

}

void CameraContainer_Base::RenderView::update(const CameraContainer_Base& camera_container, const IRenderSettingsContainer& render_settings_container)
{
    const IPoint2 resolution = camera_container.GetResolution();
    screenW = resolution.x;
    screenH = resolution.y;
    worldToView = Inverse(camera_container.m_view_params_cache.transform.shutter_open);
    RemoveScaling(worldToView);
    projType = (camera_container.m_view_params_cache.projection_type == ProjectionType::Perspective) ? 0 : 1;
    fov = camera_container.m_view_params_cache.perspective_fov_radians;
    pixelSize = camera_container.m_pixel_aspect_ratio;
    affineTM = worldToView;
    flags = render_settings_container.GetDisplacementEnabled() ? RENDER_MESH_DISPLACEMENT_MAP : 0;

    // Cache values for ViewToScreen()
    m_x_center = screenW * 0.5f;
    m_y_center = screenH * 0.5f;
    m_x_scale = 
        (projType == 0)
        ? -m_x_center / tanf(0.5f * fov)
        : screenW / camera_container.m_view_params_cache.orthographic_aperture_width;
    m_y_scale = -pixelSize * m_x_scale;
}

Point2 CameraContainer_Base::RenderView::ViewToScreen(Point3 p)
{
    // The logic of this code is copied from mental ray and scanline.
    if(projType == 0)      // Perspective
    {
        return Point2(m_x_center + (m_x_scale * p.x / p.z), m_y_center + (m_y_scale * p.y / p.z));
    }
    else        // Parallel
    {
        return Point2(m_x_center + (m_x_scale * p.x), m_y_center + (m_y_scale * p.y));
    }
}

BOOL CameraContainer_Base::RenderView::CheckForRenderAbort()
{
    // No one in the Max source code calls this, so it's pointless to implement it...
    return FALSE;
}

//==================================================================================================
// struct CameraContainer_Base::ViewParamsCache
//==================================================================================================
CameraContainer_Base::ViewParamsCache::ViewParamsCache()
    : validity(NEVER),
    transform_tminus2(true),
    projection_type(ProjectionType::Perspective),
    perspective_fov_radians(0.0f),
    film_width(0.0f),
    lens_focal_length(0.0f),
    orthographic_aperture_width(0.0f),
    focus_distance(0.0f),
    aperture_radius(0.0f),
    dof_enabled(false),
    film_plane_offset(0.0f, 0.0f),
    clip_enabled(false),
    clip_near(0.0f),
    clip_far(std::numeric_limits<float>::max()),
    env_near(0.0f),
    env_far(std::numeric_limits<float>::max())
{

}

}}	// namespace
