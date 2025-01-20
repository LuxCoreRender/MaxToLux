/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
	http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/
/**************************************************************************
*                                                                         *
* DESCRIPTION: Contains the Dll Entry stuff                               *
* AUTHOR: Omid Ghotbi (TAO) omid.ghotbi@gmail.com                         *
*                                                                         *
***************************************************************************/
/*
#pragma once

#include "Common.h"
#include "iparamm2.h"

FIRERENDER_NAMESPACE_BEGIN

/// IDs of different versions of the plugin. The ID is written in the saved files and can be read later during loading to
/// potentially run a legacy-porting script.
enum PbDescVersion
{
	FIREMAXVER_DELIVERABLE_2 = 1234,
	FIREMAXVER_LATEST = FIREMAXVER_DELIVERABLE_2,
};

#define RPR_GLOBAL_ILLUMINATION_SOLVER_PATH_TRACING 0x1
#define RPR_GLOBAL_ILLUMINATION_SOLVER_CACHEDGI 0x2

#define RPR_QUALITY_PRESET_PRODUCTION 0x1
#define RPR_QUALITY_PRESET_PREVIEW 0x2

enum TerminationCriteria
{
	enum_first = 1,
	Termination_Passes = 1,
	Termination_Time = 2,
	Termination_None = 3,
	Termination_PassesOrTime = 4,
	enum_last = 4
};

#define DEFAULT_RENDER_STAMP _T("Radeon ProRender for 3ds Max %b | %h | Time: %pt | Passes: %pp | Objects: %so | Lights: %sl")

std::tuple<float, float, float> GetRayCastConstants();

enum DenoiserType
{
	DenoiserNone = 0,
	DenoiserBilateral = 1,
	DenoiserLwr = 2,
	DenoiserEaw = 3,
	DenoiserMl = 4
};

enum Parameter : ParamID
{
	/// INT: Maximum number of global illumination light bounces.
	PARAM_MAX_RAY_DEPTH = 101,

	/// INT: Which Radeon ProRender render mode to use. Values correspond to fr_render_mode values from Radeon ProRender API.
	PARAM_RENDER_MODE = 102,

	/// INT: Which image filter to use for reconstruction. Values correspond to fr_aa_filter values from Radeon ProRender API.
	PARAM_IMAGE_FILTER = 103,

	/// INT: Time limit in seconds, 0 = no time limit. If both time and pass limits are set, render is terminated after either 
	/// of them is hit. In seconds.
	PARAM_TIME_LIMIT = 200,

	/// INT: Number of passes to make, 0 = no limit. If both time and pass limits are set, render is terminated after either 
	/// of them is hit.  VIRTUAL PARAMETER, equal to (PARAM_SAMPLES_MAX / PARAM_CONTEXT_ITERATIONS)
	PARAM_PASS_LIMIT = 201,

	/// INT: Maximum number of samples to make, 0 = no limit. If both time and pass limits are set, render is terminated after
	/// either of them is hit.  Each render iteration adds a number of samples equal to the context iterations value
	PARAM_SAMPLES_MAX = 202,

	/// INT: Minimum number of samples to make. After this number, adaptive sample will stop sampling pixels where noise is
	/// less than threshold
	PARAM_SAMPLES_MIN = 203,

	/// FLOAT: Adaptive Sampling noise threshold. Once pixels are below this amount of noise, not more samples are added.
	/// Set to 0 for no cutoff.
	PARAM_ADAPTIVE_NOISE_THRESHOLD = 204,

	/// FLOAT: Adaptive Sampling tile size.
	PARAM_ADAPTIVE_TILESIZE = 205,

	/// BOOL: Whether to use the depth of field effect
	OBSOLETE_PARAM_USE_DOF = 300,
	/// INT: How many camera aperture blades are simulated when doing DOF
	OBSOLETE_PARAM_CAMERA_BLADES = 301,
	/// WORLD: Focal distance for DOF used when rendering from a free view (i.e. not from camera view, because cameras have their 
	/// own focal distance)
	OBSOLETE_PARAM_CAMERA_FOCUS_DIST = 302,
	/// FLOAT: Camera aperture F-Stop number for DOF calculation
	OBSOLETE_PARAM_CAMERA_F_STOP = 303,
	/// WORLD: Camera sensor size for DOF calculation. It is in world units, so the number can be vastly different from the 
	/// "usual" value of 36
	OBSOLETE_PARAM_CAMERA_SENSOR_WIDTH = 304,
	/// FLOAT: Simple exposure - number that multiplies the final result
	OBSOLETE_PARAM_CAMERA_EXPOSURE = 305,
	/// FLOAT: Focal Length
	OBSOLETE_PARAM_CAMERA_FOCAL_LENGTH = 306,
	/// FLOAT: Camera FOV
	OBSOLETE_PARAM_CAMERA_FOV = 307,
	/// BOOL: overwrite DOF values
	OBSOLETE_PARAM_CAMERA_OVERWRITE_DOF_SETTINGS = 308,
	/// BOOL: for general settings
	OBSOLETE_PARAM_CAMERA_USE_FOV = 309,
	/// integer: type of camera
	OBSOLETE_PARAM_CAMERA_TYPE = 310,

	/// BOOL: If true, 3ds Max will be frozen except for the VFB during rendering. This prevets crashes from modifying the scene 
	/// while rendering
	PARAM_LOCK_MAX = 401,

	/// BOOL: If true, Radeon ProRender core will be told to use also CPU for rendering
	PARAM_USE_CPU = 402,

	/// INT: The number of GPUs Radeon ProRender code is told to use
	PARAM_GPU_COUNT = 403, // not used

	/// INT: How many rendering passes should be for material editor previews
	PARAM_MTL_PREVIEW_PASSES = 404,

	/// BOOL: If true, then any changes in camera view will cause the rendering to restart from the new angle
	PARAM_INTERACTIVE_MODE = 405, // not used

	/// FLOAT: Width of the image reconstruction filter in pixels
	PARAM_IMAGE_FILTER_WIDTH = 408,

	/// INT (milliseconds): The minimum period after which the VFB redraws
	PARAM_VFB_REFRESH = 410, // not used

	/// INT: Which tone mapping operator to use. It is updated in real time. Values are from enum fr_tonemapping_operator
	OBSOLETE_PARAM_TONEMAP_OPERATOR = 411,

	// Radeon ProRender tone mapping parameters.  All values are FLOAT. All changes to these values are updated in real time

	OBSOLETE_PARAM_TONEMAP_LINEAR_SCALE = 412,
	OBSOLETE_PARAM_TONEMAP_PHOTOLINEAR_SENSITIVITY = 413,
	OBSOLETE_PARAM_TONEMAP_PHOTOLINEAR_EXPOSURE = 414,
	OBSOLETE_PARAM_TONEMAP_PHOTOLINEAR_FSTOP = 415,
	OBSOLETE_PARAM_TONEMAP_REINHARD_PRESCALE = 416,
	OBSOLETE_PARAM_TONEMAP_REINHARD_POSTSCALE = 417,
	OBSOLETE_PARAM_TONEMAP_REINHARD_BURN = 418,

	/// FLOAT: How many AA samples will the core take during each rendering pass
	OBSOLETE_PARAM_AA_SAMPLE_COUNT = 419,

	/// FLOAT: Radeon ProRender core parameter
	OBSOLETE_PARAM_AA_GRID_SIZE = 420,

	PARAM_TRACEDUMP_BOOL = 422,

	PARAM_GLOBAL_ILLUMINATION_SOLVER = 423,
	PARAM_USE_PHOTOLINEAR_TONEMAP = 424,
	PARAM_QUALITY_PRESET = 425,
	PARAM_LIMIT_LIGHT_BOUNCE = 426,
	PARAM_MAX_LIGHT_BOUNCES = 427,
	PARAM_RENDER_DEVICE = 429,

	PARAM_USE_IRRADIANCE_CLAMP = 430,
	PARAM_IRRADIANCE_CLAMP = 431,

	OBSOLETE_PARAM_TONEMAP_PHOTOLINEAR_ISO = 432,

	PARAM_GPU_WAS_COMPATIBLE = 433,
	PARAM_GPU_INCOMPATIBILITY_WARNING_WAS_SHOWN = 434,

	OBSOLETE_PARAM_USE_MOTION_BLUR = 435,
	OBSOLETE_PARAM_MOTION_BLUR_SCALE = 436,

	PARAM_GPU_SELECTED_BY_USER = 437,

	PARAM_CONTEXT_ITERATIONS = 438,

	PARAM_WARNING_DONTSHOW = 501,
	PARAM_RENDER_LIMIT = 502, // VIRTUAL PARAM, time limited if PARAM_SAMPLES_MAX==0, pass limited if PARAM_TIME_LIMIT==0

	PARAM_TRACEDUMP_PATH = 503,

	OBSOLETE_PARAM_OVERRIDE_MAX_TONEMAPPERS = 504,
	OBSOLETE_PARAM_TONEMAP_WHITE_BALANCE = 505,
	OBSOLETE_PARAM_TONEMAP_CONTRAST = 506,
	OBSOLETE_PARAM_TONEMAP_SHUTTER_SPEED = 507,
	OBSOLETE_PARAM_TONEMAP_SENSOR_WIDTH = 508,

	PARAM_TEXMAP_RESOLUTION = 551,

	// BACKGROUND TRANSIENT PROPERTIES (to access/animate BgManagerMax properties)
	TRPARAM_BG_USE = 552,
	TRPARAM_BG_TYPE = 553, // IBL, SUNSKY
	TRPARAM_BG_IBL_SOLIDCOLOR = 554,
	TRPARAM_BG_IBL_INTENSITY = 555,
	TRPARAM_BG_IBL_MAP = 556,
	TRPARAM_BG_IBL_BACKPLATE = 557,
	TRPARAM_BG_IBL_REFLECTIONMAP = 558,
	TRPARAM_BG_IBL_REFRACTIONMAP = 559,
	TRPARAM_BG_IBL_MAP_USE = 560,
	TRPARAM_BG_IBL_BACKPLATE_USE = 561,
	TRPARAM_BG_IBL_REFLECTIONMAP_USE = 562,
	TRPARAM_BG_IBL_REFRACTIONMAP_USE = 563,

	TRPARAM_BG_SKY_TYPE = 564, // ANALYTICAL, DATETIME
	TRPARAM_BG_SKY_AZIMUTH = 565,
	TRPARAM_BG_SKY_ALTITUDE = 566,
	TRPARAM_BG_SKY_ALBEDO = 567,
	TRPARAM_BG_SKY_TURBIDITY = 568,
	TRPARAM_BG_SKY_HOURS = 569,
	TRPARAM_BG_SKY_MINUTES = 570,
	TRPARAM_BG_SKY_SECONDS = 571,
	TRPARAM_BG_SKY_DAY = 572,
	TRPARAM_BG_SKY_MONTH = 573,
	TRPARAM_BG_SKY_YEAR = 574,
	TRPARAM_BG_SKY_TIMEZONE = 575,
	TRPARAM_BG_SKY_LATITUDE = 576,
	TRPARAM_BG_SKY_LONGITUDE = 577,
	TRPARAM_BG_SKY_DAYLIGHTSAVING = 578,

	TRPARAM_BG_SKY_HAZE = 579,
	TRPARAM_BG_SKY_GLOW = 580,
	TRPARAM_BG_SKY_REDBLUE_SHIFT = 581,
	TRPARAM_BG_SKY_GROUND_COLOR = 582,
	TRPARAM_BG_SKY_DISCSIZE = 583,
	TRPARAM_BG_SKY_HORIZON_HEIGHT = 584,
	TRPARAM_BG_SKY_HORIZON_BLUR = 585,
	TRPARAM_BG_SKY_INTENSITY = 586,

	// GROUND TRANSIENT PROPERTIES (to access/animate BgManagerMax properties)
	TRPARAM_GROUND_ACTIVE = 587,
	TRPARAM_GROUND_RADIUS = 588,
	TRPARAM_GROUND_GROUND_HEIGHT = 589,
	TRPARAM_GROUND_SHADOWS = 590,
	TRPARAM_GROUND_REFLECTIONS_STRENGTH = 591,
	TRPARAM_GROUND_REFLECTIONS = 592,
	TRPARAM_GROUND_REFLECTIONS_COLOR = 593,
	TRPARAM_GROUND_REFLECTIONS_ROUGHNESS = 594,

	TRPARAM_BG_SKY_BACKPLATE = 595,
	TRPARAM_BG_SKY_REFLECTIONMAP = 596,
	TRPARAM_BG_SKY_REFRACTIONMAP = 597,
	TRPARAM_BG_SKY_BACKPLATE_USE = 598,
	TRPARAM_BG_SKY_REFLECTIONMAP_USE = 599,
	TRPARAM_BG_SKY_REFRACTIONMAP_USE = 600,

	TRPARAM_BG_ENABLEALPHA = 601,

	// Render stamp
	PARAM_STAMP_ENABLED = 602,
	PARAM_STAMP_TEXT = 603,

	// override max's material preview shapes with our own matball
	PARAM_OVERRIDE_MAX_PREVIEW_SHAPES = 604,

	TRPARAM_BG_SKY_FILTER_COLOR = 605,
	TRPARAM_BG_SKY_SHADOW_SOFTNESS = 606, // not used
	TRPARAM_BG_SKY_SATURATION = 607,

	PARAM_USE_TEXTURE_COMPRESSION = 620,

	/// INT: How many AA samples will the core take during each rendering pass
	PARAM_AA_SAMPLE_COUNT, // Obsolete parameter; not removed for scenes backward compatibility

	/// INT: Radeon ProRender core parameter
	PARAM_AA_GRID_SIZE, // Obsolete parameter; not removed for scenes backward compatibility

	// (sky) ground albedo
	TRPARAM_BG_SKY_GROUND_ALBEDO,

	// (Quality) Raycast Epsilon
	PARAM_QUALITY_RAYCAST_EPSILON,

	// Denoiser
	PARAM_DENOISER_ENABLED = 700,
	PARAM_DENOISER_TYPE = 701,
	PARAM_DENOISER_BILATERAL_RADIUS = 710,
	PARAM_DENOISER_LWR_SAMPLES = 720,
	PARAM_DENOISER_LWR_RADIUS = 721,
	PARAM_DENOISER_LWR_BANDWIDTH = 722,
	PARAM_DENOISER_EAW_COLOR = 730,
	PARAM_DENOISER_EAW_NORMAL = 731,
	PARAM_DENOISER_EAW_DEPTH = 732,
	PARAM_DENOISER_EAW_OBJECTID = 733,
};

/// Descriptor of the rendering plugin parameter block. It specifies all parameters and their options the rendering plugin has
extern ParamBlockDesc2 FIRE_MAX_PBDESC;

/// Accessort for Background and Ground transient properties 
class BgAccessor : public PBAccessor
{
	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) override;
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override;
	BOOL KeyFrameAtTime(ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override;
	ParamID TranslateParamID(ParamID id);
};

/// Accessort for Sampling transient properties 
class SamplingAccessor : public PBAccessor
{
	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) override;
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override;
	BOOL KeyFrameAtTime(ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override;
};

FIRERENDER_NAMESPACE_END
*/