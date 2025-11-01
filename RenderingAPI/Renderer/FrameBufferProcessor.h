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

// max sdk
#include <RenderingAPI/Renderer/IFrameBufferProcessor.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/IRenderingProcess.h>
#include <noncopyable.h>
#include <box2.h>
#include <toneop.h>

class Bitmap;

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

// Generic implementation of IFrameBufferProcessor, for storing potentially tone mapped data into any Bitmap
class FrameBufferProcessor : public IFrameBufferProcessor
{
public:

    FrameBufferProcessor();
    ~FrameBufferProcessor();

    // -- inherited from IFrameBufferProcessor
    virtual bool ProcessFrameBuffer(const bool process_tone_operator, const TimeValue t, IFrameBufferReader& frame_buffer_access) override;

protected:

    // Returns the bitmap to which the frame buffer data is to be stored
    virtual Bitmap* get_frame_buffer_bitmap() = 0;

    virtual IRenderSessionContext& get_render_session_context() = 0;

private:

    class MainThreadProcessor;

private:

};

// The main thread job that does the actual frame buffer processing
class FrameBufferProcessor::MainThreadProcessor : 
    private IRenderingProcess::IMainThreadJob,
    public MaxSDK::Util::Noncopyable
{
public:

    // The tone operator is fully processed in the constructor
    MainThreadProcessor(
        Bitmap& bitmap,
        IFrameBufferReader& frame_buffer_access,
        const bool process_tone_operator,
        const TimeValue t, 
        const IRenderSessionContext& render_session_context);

    // Returns whether the job ran without error
    bool was_successful() const;

private:

    // Abstracts the processing the frame buffer with a tone operator using either ToneOperator::ScaledToRGB() or ThreadedProcessor::ScaledToRGB()
    template<typename T> void process_frame_buffer(T* const tone_operator);

    // -- inherited from IMainThreadJob
    virtual void ExecuteMainThreadJob();

private:

    const bool m_process_tone_operator;
    const TimeValue m_t;
    const IRenderSessionContext& m_render_session_context;
    Bitmap& m_bitmap;
    IFrameBufferReader& m_frame_buffer_access;

    std::unique_ptr<const ToneOperator::ThreadedProcessor> m_tone_operator_processor;

	bool m_error_encountered;
};

}}	// namespace 

