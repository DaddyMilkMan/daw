/**
 * @file ExportJob.cpp
 * @brief Asynchronous export job implementation
 */

#include "ExportJob.h"
#include "AudioExporter.h"
#include "Engine.h"
#include "AudioRenderer.h"
#include "../dsp/Dither.h"

namespace zenith {

//==============================================================================
// Constructor
//==============================================================================

ExportJob::ExportJob(Engine& engine,
                     const ExportOptions& options,
                     ProgressCallback progress,
                     CompletionCallback completion)
    : juce::ThreadPoolJob("ExportJob"),
      engine_(engine),
      options_(options),
      progressCallback_(std::move(progress)),
      completionCallback_(std::move(completion))
{
}

//==============================================================================
// Main Job Entry Point
//==============================================================================

juce::ThreadPoolJob::JobStatus ExportJob::runJob()
{
    juce::Result result = performExport();
    
    // Report completion on message thread for thread safety
    if (completionCallback_)
    {
        juce::MessageManager::callAsync([callback = completionCallback_, result]() {
            callback(result);
        });
    }
    
    // Cleanup engine pointer
    ExportJob* expected = this;
    engine_.currentExportJob_.compare_exchange_strong(expected, nullptr);

    return jobHasFinished;
}

void ExportJob::cancel()
{
    shouldCancel_.store(true, std::memory_order_release);
}

//==============================================================================
// Progress Reporting
//==============================================================================

void ExportJob::reportProgress(float progress, const juce::String& status,
                               juce::int64& lastReportTime)
{
    juce::int64 now = juce::Time::getMillisecondCounter();
    
    if (now - lastReportTime >= kProgressReportIntervalMs)
    {
        currentProgress_.store(progress, std::memory_order_relaxed);
        
        if (progressCallback_)
        {
            // Dispatch to message thread for UI safety
            juce::MessageManager::callAsync(
                [callback = progressCallback_, progress, status]() {
                    callback(progress, status);
                });
        }
        
        lastReportTime = now;
    }
}

//==============================================================================
// Writer Creation with Error Handling
//==============================================================================

std::unique_ptr<juce::AudioFormatWriter> ExportJob::createWriter(
    const juce::File& outputFile,
    juce::AudioFormat* format,
    juce::String& outErrorMessage)
{
    // Delete existing file
    if (outputFile.existsAsFile())
    {
        if (!outputFile.deleteFile())
        {
            outErrorMessage = "Cannot overwrite existing file: " + outputFile.getFullPathName();
            return nullptr;
        }
    }
    
    // Check parent directory exists and is writable
    juce::File parentDir = outputFile.getParentDirectory();
    if (!parentDir.isDirectory())
    {
        outErrorMessage = "Output directory does not exist: " + parentDir.getFullPathName();
        return nullptr;
    }
    
    // Create output stream
    std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());
    if (!fileStream)
    {
        outErrorMessage = "Permission denied or disk full: " + outputFile.getFullPathName();
        return nullptr;
    }
    
    if (fileStream->failedToOpen())
    {
        outErrorMessage = "Failed to open file for writing: " + outputFile.getFullPathName();
        return nullptr;
    }
    
    // Create writer
    auto writerOptions = juce::AudioFormatWriterOptions()
        .withSampleRate(options_.sampleRate)
        .withNumChannels(2)
        .withBitsPerSample(options_.bitDepth);
    
    std::unique_ptr<juce::OutputStream> streamPtr(std::move(fileStream));
    std::unique_ptr<juce::AudioFormatWriter> writer(
        format->createWriterFor(streamPtr, writerOptions));
    
    if (!writer)
    {
        outErrorMessage = "Failed to create audio writer for format";
        return nullptr;
    }
    
    // Writer takes ownership of stream
    streamPtr.release();
    
    return writer;
}

//==============================================================================
// Main Export Implementation
//==============================================================================

juce::Result ExportJob::performExport()
{
    DBG("ExportJob: Starting async export to " + options_.outputFile.getFullPathName());
    
    // Validate options
    if (options_.sampleRate <= 0)
        return juce::Result::fail("Invalid sample rate: " + juce::String(options_.sampleRate));
    
    if (options_.bitDepth != 8 && options_.bitDepth != 16 && 
        options_.bitDepth != 24 && options_.bitDepth != 32)
        return juce::Result::fail("Invalid bit depth: " + juce::String(options_.bitDepth));
    
    // Get audio format
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    formatManager.registerFormat(new juce::FlacAudioFormat(), false);
    formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
    
    juce::String extension;
    switch (options_.format)
    {
        case ExportFormat::WAV:  extension = "wav";  break;
        case ExportFormat::FLAC: extension = "flac"; break;
        case ExportFormat::OGG:  extension = "ogg";  break;
        case ExportFormat::AIFF: extension = "aiff"; break;
    }
    
    juce::AudioFormat* format = formatManager.findFormatForFileExtension(extension);
    if (!format)
        return juce::Result::fail("Unsupported audio format: " + extension);
    
    // Create writer
    juce::String errorMessage;
    auto writer = createWriter(options_.outputFile, format, errorMessage);
    if (!writer)
        return juce::Result::fail(errorMessage);
    
    // Suspend engine to prevent race conditions
    engine_.suspendProcessing(true);
    
    // RAII helper to ensure we resume processing
    struct ScopedResume {
        Engine& e;
        ~ScopedResume() { e.suspendProcessing(false); }
    } resumer{engine_};
    
    // Setup dithering
    zenith::dsp::Dither dither;
    if (options_.enableDither)
    {
        dither.prepare(2);
        dither.setType(options_.ditherType);
    }
    
    // Determine duration
    double duration = options_.duration;
    if (duration <= 0.0)
    {
        duration = engine_.autoDetectProjectDuration();
    }
    
    DBG("ExportJob: Duration: " + juce::String(duration, 2) + "s @ " + 
        juce::String(options_.sampleRate) + " Hz");
    
    // Prepare buffers
    juce::AudioBuffer<float> buffer(2, kExportBlockSize);

    // Create Render Context
    AudioRenderContext context;
    context.prepare(options_.sampleRate, kExportBlockSize, 
                    engine_.getNumTracks(), engine_.getNumAuxBuses());
    
    juce::int64 startSample = static_cast<juce::int64>(options_.startTime * options_.sampleRate);
    juce::int64 totalSamples = static_cast<juce::int64>(options_.sampleRate * duration);
    juce::int64 samplesWritten = 0;
    juce::int64 lastProgressReport = juce::Time::getMillisecondCounter();
    
    // Main render loop
    while (samplesWritten < totalSamples)
    {
        // Check for cancellation
        if (shouldCancel_.load(std::memory_order_acquire))
        {
            DBG("ExportJob: Cancelled at " + juce::String(samplesWritten) + " samples");
            writer.reset();
            options_.outputFile.deleteFile();
            return juce::Result::fail("Export cancelled by user");
        }
        
        int numSamples = static_cast<int>(
            juce::jmin(static_cast<juce::int64>(kExportBlockSize),
                       totalSamples - samplesWritten));
        
        // Render audio block
        engine_.renderOfflineBlock(context, buffer, numSamples, startSample + samplesWritten);
        
        // Apply dithering
        if (options_.enableDither && options_.bitDepth < 32)
        {
            dither.process(buffer, options_.bitDepth);
        }
        
        // Write to file
        if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
        {
            writer.reset();
            options_.outputFile.deleteFile();
            return juce::Result::fail("Write error - disk may be full");
        }
        
        samplesWritten += numSamples;
        
        // Report progress
        float progress = static_cast<float>(samplesWritten) / static_cast<float>(totalSamples);
        reportProgress(progress, "Exporting audio...", lastProgressReport);
    }
    
    // Final progress report
    currentProgress_.store(1.0f, std::memory_order_relaxed);
    if (progressCallback_)
    {
        juce::MessageManager::callAsync([callback = progressCallback_]() {
            callback(1.0f, "Export complete!");
        });
    }
    
    DBG("ExportJob: Export complete - " + juce::String(samplesWritten) + " samples written");
    
    return juce::Result::ok();
}

} // namespace zenith
