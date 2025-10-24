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

// 3ds max sdk
#include <render.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/UnifiedRenderer.h>

class ViewExp;

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK;
using namespace MaxSDK::RenderingAPI;

// Functionality common to both interactive and offline versions of ICameraContainer
class CameraContainer_Base :
    public ICameraContainer,
    public MaxSDK::Util::Noncopyable
{
public:

	CameraContainer_Base(
        const UnifiedRenderer& renderer,
        const IRenderSettingsContainer& render_settings_container);
    virtual ~CameraContainer_Base();

    // Sets the camera to be used by the render
    void set_camera_node(INode* const camera_node);
    // Sets whether the camera node is to be used. This flag defaults to true.
    void set_use_camera(const bool use_camera);
    bool get_use_camera() const;

    // Sets the viewport which will be used iff no camera node has been specified or set_use_camera(false) has been called.
    void set_view(const ViewParams& view_params);
    void set_view(const ViewExp& view_exp);

    // Sets the down-resolution factor, used to reduce resolution to improve performance
    void set_down_res_factor(const unsigned int factor);
    // Returns the effective down-resolution factor, clamped to ensure the render resolution doesn't end up being 0
    unsigned int get_effective_down_res_factor() const;
    // Returns the full camera resolution, ignoring the down-res factor
    IPoint2 get_resolution_ignoring_down_res_factor() const;
    // Returns the full region, ignoring the down-res factor
    Box2 get_region_ignoring_down_res_factor() const;

    // -- inherited from ICameraContainer
    virtual INode* GetCameraNode() const override;
    virtual IPhysicalCamera* GetPhysicalCamera(const TimeValue t) const override;
    virtual MotionTransforms EvaluateCameraTransform(const TimeValue t, Interval& validity, const MotionBlurSettings* camera_motion_blur_settings) const override;
    virtual ProjectionType GetProjectionType(const TimeValue t, Interval& validity) const override;
    virtual float GetPerspectiveFOVRadians(const TimeValue t, Interval& validity) const override;
    virtual float GetOrthographicApertureWidth(const TimeValue t, Interval& validity) const override;
    virtual float GetOrthographicApertureHeight(const TimeValue t, Interval& validity) const override;
    virtual bool GetDOFEnabled(const TimeValue t, Interval& validity) const override;
    virtual float GetLensFocalLength(const TimeValue t, Interval& validity) const override;
    virtual float GetFocusPlaneDistance(const TimeValue t, Interval& validity) const override;
    virtual float GetLensApertureRadius(const TimeValue t, Interval& validity) const override;
    virtual bool GetClipEnabled(const TimeValue t, Interval& validity) const override;
    virtual float GetClipNear(const TimeValue t, Interval& validity) const override;
    virtual float GetClipFar(const TimeValue t, Interval& validity) const override;
    virtual IPoint2 GetResolution() const override;
    virtual float GetPixelAspectRatio() const override;
    virtual Bitmap* GetBitmap() const override;
    virtual Box2 GetRegion() const override;
    virtual Point2 GetImagePlaneOffset(const TimeValue t, Interval& validity) const override;
    virtual const View& GetView(const TimeValue t, Interval& validity) const override;
    virtual ViewParams GetViewParams(const TimeValue t, Interval& validity) const override;
    virtual MotionBlurSettings GetGlobalMotionBlurSettings(const TimeValue t, Interval& validity) const override;
    virtual void RegisterChangeNotifier(IChangeNotifier& notifier) const override final;
    virtual void UnregisterChangeNotifier(IChangeNotifier& notifier) const override final;

protected:

    virtual void notify_change();

    // To be called when we are notified that the camera node may have been deleted
    void camera_node_deleted(INode& deleted_node);

    // Internal methods for accessing parameters which are exposed differently by each implementation of this class
    void set_bitmap_internal(Bitmap* const bitmap);
    void set_region_internal(const Box2& region);
    void set_blowup_internal(const Point2& offset, const float zoom);

    // Invalidates the (cached) contents of the camera, notifies clients of the change.
    void Invalidate();

private:

    // Ensures all view parameters (m_view_params_cache) are updated at the given time.
    void update_view_params_cache(const TimeValue t, Interval& validity) const;

    // Fetches DOF parameters from the given camera
    void get_dof_parameters(const CameraObject& camera, bool& dof_enabled, float& aperture_fstop) const;

private:

    class RenderView : public View
    {
    public:

        RenderView();

        // Updates the contents of this class by reading the settings from the camera
        void update(const CameraContainer_Base& camera_container, const IRenderSettingsContainer& render_settings_container);

        // -- from View
        virtual Point2 ViewToScreen(Point3 p);
        virtual BOOL CheckForRenderAbort();

    private:

        // Cached values to speed up ViewToScreen()
        float m_x_center;
        float m_y_center;
        float m_x_scale;
        float m_y_scale;
    };

    const UnifiedRenderer& m_renderer;
    const IRenderSettingsContainer& m_render_settings_container;

    // This is the camera node which was explicitly specified through set_camera_node()
    INode* m_explicit_camera_node;
    // The is the camera node which was specified in a ViewExp. We keep this separate from m_explicit_camera_node, because which one gets used
    // depends on the state of the use_camera_node flag.
    INode* m_view_exp_camera_node;

    // The ViewParams, to be used iff the camera is null.
    ViewParams m_view_params;

    // Determines whether m_explicit_camera_node is to be used.
    bool m_use_explicit_camera;

    // Properties related to the bitmap/output.
    Bitmap* m_bitmap;
    IPoint2 m_bitmap_resolution;
    float m_pixel_aspect_ratio;
    // The down-resolution factor, used to reduce resolution and improve performance
    // Generally, don't use directly - use get_effective_down_res_factor() instead.
    unsigned int m_down_res_factor = 1;

    // Region to be rendered
    Box2 m_region;

    // Blowup render settings
    Point2 m_blowup_image_plane_offset_in_pixels;
    float m_blowup_zoom;

    // Cached properties fetched from the camera node/ViewParams, with time-based validity.
    // Mutable since those are re-build/re-cached as needed, but don't affect the true content of the camera container.
    mutable struct ViewParamsCache
    {
        // Default constructor initializes default values
        ViewParamsCache();

        Interval validity;
        Matrix3 transform_tminus2; // Transform at t-2, used for setting ViewParams::prevAffineTM
        MotionTransforms transform;
        ICameraContainer::ProjectionType projection_type;
        float perspective_fov_radians;
        float film_width;       // film width in system units
        float lens_focal_length;        // focal length in system units
        float orthographic_aperture_width;
        float focus_distance;
        float aperture_radius;      // aperture radius in system units
        bool dof_enabled;
        Point2 film_plane_offset;      // film planet offset in pixels, relative to upper-left corner
        // The View used with GetRenderMesh()
        RenderView render_view;
        bool clip_enabled;
        float clip_near;
        float clip_far;
        float env_near;
        float env_far;
    } m_view_params_cache;

    mutable std::vector<IChangeNotifier*> m_change_notifiers;
};

}}	// namespace
