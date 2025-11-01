//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include "FrameBufferProcessor.h"

// max sdk
#include <RenderingAPI/Renderer/ICameraContainer.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <bitmap.h>
// std
#include <algorithm>

#undef min
#undef max

namespace Max
{;
namespace RenderingAPI
{;

//==================================================================================================
// class FrameBufferProcessor
//==================================================================================================

FrameBufferProcessor::FrameBufferProcessor()
{

}

FrameBufferProcessor::~FrameBufferProcessor()
{

}

bool FrameBufferProcessor::ProcessFrameBuffer(const bool process_tone_operator, const TimeValue t, IFrameBufferReader& frame_buffer_access) 
{
    Bitmap* const bitmap = get_frame_buffer_bitmap();
    if(DbgVerify(bitmap != nullptr))
    {
        IRenderSessionContext& session_context = get_render_session_context();
        MainThreadProcessor main_thread_processor(
            *bitmap, 
            frame_buffer_access, 
            process_tone_operator,
            t, 
            session_context);

        return main_thread_processor.was_successful();
    }
    else
    {
        return false;
    }
}

//==================================================================================================
// class FrameBufferProcessor::MainThreadProcessor
//==================================================================================================

FrameBufferProcessor::MainThreadProcessor::MainThreadProcessor(
    Bitmap& bitmap,
    IFrameBufferReader& frame_buffer_access,
    const bool process_tone_operator,
    const TimeValue t, 
    const IRenderSessionContext& render_session_context)

    : m_process_tone_operator(process_tone_operator),
    m_t(t),
    m_render_session_context(render_session_context),
    m_bitmap(bitmap),
    m_frame_buffer_access(frame_buffer_access),
    m_error_encountered(false)
{
    // Execute the main thread job, which either executes the tone operator or fetches the ThreadProcessor that allows us to run the tone operator
    // in a non-main thread.
    if(render_session_context.GetRenderingProcess().RunJobFromMainThread(*this))
    {
        // If the tone operator can be processed in a non-main thread, process it now. Otherwise, it was already processed
        // by the main thread job executed above
        if(m_tone_operator_processor != nullptr)
        {
            process_frame_buffer(m_tone_operator_processor.get());
        }
    }
    else
    {
        // Abort requested - but don't care. The bitmap won't update, but that's OK - we'll let the render abort normally after this.
    }
}

void FrameBufferProcessor::MainThreadProcessor::ExecuteMainThreadJob()
{
    ToneOperator* const tone_op = m_process_tone_operator ? m_render_session_context.GetRenderSettings().GetToneOperator() : nullptr;

    // Setup the tone operator
    if(tone_op != nullptr)
    {
        const float pixel_aspect = m_bitmap.Aspect();
        INode* const camera_node = m_render_session_context.GetCamera().GetCameraNode();
        Interval dummy_interval;
        tone_op->Update(m_t, camera_node, m_frame_buffer_access.GetResolution(), pixel_aspect, dummy_interval);

        // Check if we can execute the tone operator in a non-main thread
        m_tone_operator_processor.reset(tone_op->AllocateThreadedProcessor());
    }

    // If we have a tone operator processor, then we'll process the frame buffer in the render thread. But if we have a regular tone operator,
    // we need to call it from the main thread, i.e. here.
    if(m_tone_operator_processor == nullptr)
    {
        process_frame_buffer(tone_op);
    }
}

namespace
{
    void Generic_ScaledToRGB(const ToneOperator& tone_op, float energy[3], const Point2& xyCoord)
    {
        tone_op.ScaledToRGB(energy, xyCoord);
    }
    void Generic_ScaledToRGB(const ToneOperator::ThreadedProcessor& tone_op, float energy[3], const Point2& xyCoord)
    {
        tone_op.ScaledToRGB(energy, xyCoord);
    }
}

template<typename T> 
void FrameBufferProcessor::MainThreadProcessor::process_frame_buffer(T* const tone_operator)
{
    // Retrieve source and target resolutions
    const IPoint2 target_resolution(m_bitmap.Width(), m_bitmap.Height());
    const IPoint2 source_resolution = m_frame_buffer_access.GetResolution();
    const Box2 source_region = m_frame_buffer_access.GetRegion();

    const int begin_x = source_region.x();
    const int begin_y = source_region.y();
    const int end_x = std::min(source_resolution.x, begin_x + source_region.w());
    const int end_y = std::min(source_resolution.y, begin_y + source_region.h());

    if(DbgVerify((begin_y < end_y) && (begin_x < end_x)))
    {
        // Process the tone operator on the bitmap, one line at a time
        const unsigned int line_size = (end_x - begin_x);
        std::vector<BMM_Color_fl> pixel_line(line_size);
        for(int y = begin_y; y < end_y; ++y)
        {
            if(DbgVerify(m_frame_buffer_access.GetPixelLine(y, begin_x, line_size, pixel_line.data())))
            {
                if(tone_operator != nullptr)
                {
                    for(size_t x_pixel_line = 0; x_pixel_line < pixel_line.size(); ++x_pixel_line)
                    {
                        BMM_Color_fl& pixel = pixel_line[x_pixel_line];
                        float energy[3] = {pixel.r, pixel.g, pixel.b};
                        Generic_ScaledToRGB(*tone_operator, energy, Point2(x_pixel_line + begin_x + 0.5f, y + 0.5f));
                        pixel = BMM_Color_fl(energy[0], energy[1], energy[2], pixel.a);
                    }
                }

                // Commit the tone mapped pixel line back to the bitmap
                // (while supporting differing resolutions between source and target frame buffers)
                if(target_resolution == source_resolution)
                {
                    DbgVerify(m_bitmap.PutPixels(begin_x, y, static_cast<int>(pixel_line.size()), pixel_line.data()));
                }
                else
                {
                    // If source and target resolutions don't match, then we perform a very crude resizing by simply remapping the pixel values.
                    // This will work fine if and only if the source resolution is smaller, and ideally an integer multiplier of the target resolution
                    // (which is exactly what we need to properly support adaptive down-resolution).
                    const float resolution_factor_x = float(target_resolution.x) / float(source_resolution.x);
                    const float resolution_factor_y = float(target_resolution.y) / float(source_resolution.y);

                    for(int target_y = static_cast<int>(y * resolution_factor_y);
                        target_y < static_cast<int>((y + 1) * resolution_factor_y);
                        ++target_y)
                    {
                        if(target_y < target_resolution.y)
                        {
                            for(int source_pixel_index = 0; source_pixel_index < pixel_line.size(); ++source_pixel_index)
                            {
                                for(int target_x = static_cast<int>((begin_x + source_pixel_index) * resolution_factor_x);
                                    target_x < static_cast<int>((begin_x + source_pixel_index + 1) * resolution_factor_x);
                                    ++target_x)
                                {
                                    if(target_x < target_resolution.x)
                                    {
                                        DbgVerify(m_bitmap.PutPixels(target_x, target_y, 1, &(pixel_line[source_pixel_index])));
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // Error
                //!! TODO: Report error
                m_error_encountered = true;
            }
        }
    }
}

bool FrameBufferProcessor::MainThreadProcessor::was_successful() const
{
    return !m_error_encountered;
}

}}	// namespace 

