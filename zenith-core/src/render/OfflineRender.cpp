/**
 * @file OfflineRender.cpp
 * @brief OfflineRender implementation
 */

#include "../../include/render/OfflineRender.h"
#include "../../include/Engine.h"
#include "../../include/model/ProjectPlaybackContext.h"
#include <cmath>

namespace zenith {

namespace {

/**
 * @brief Compute total project length in samples
 * @param project Project data
 * @return Length in samples (end of last unmuted clip)
 */
int64_t computeProjectLengthSamples(const ProjectModel& project)
{
    int64_t maxEnd = 0;

    for (const auto& track : project.tracks)
    {
        // Skip muted tracks
        if (track.muted)
            continue;

        for (const auto& clip : track.clips)
        {
            // Skip muted clips
            if (clip.muted)
                continue;

            // Compute clip end position
            int64_t clipEnd = clip.startSample + clip.lengthSamples;
            if (clipEnd > maxEnd)
                maxEnd = clipEnd;
        }
    }

    return maxEnd;
}

/**
 * @brief Create appropriate audio format for export
 * @param options Export options
 * @return Audio format instance (caller owns)
 */
std::unique_ptr<juce::AudioFormat> createAudioFormat(const ExportOptions& options)
{
    switch (options.format)
    {
        case ExportFormat::WAV:
            return std::make_unique<juce::WavAudioFormat>();

        case ExportFormat::AIFF:
            return std::make_unique<juce::AiffAudioFormat>();

        default:
            return std::make_unique<juce::WavAudioFormat>();
    }
}

} // anonymous namespace

//==============================================================================
// Public API
//==============================================================================

juce::Result renderProjectToFile(const ProjectModel& project,
                                 const juce::File& outputFile,
                                 double sampleRate,
                                 const ExportOptions& options)
{
    // Validate export options
    auto validationError = options.validate();
    if (validationError.isNotEmpty())
        return juce::Result::fail(validationError);

    // Validate parameters
    if (sampleRate <= 0.0)
        return juce::Result::fail("Invalid sample rate for export.");

    // v0.1: No resampling support
    if (std::abs(project.sampleRate - sampleRate) > 1e-3)
    {
        return juce::Result::fail(
            "Project sample rate (" + juce::String(project.sampleRate) +
            " Hz) does not match export sample rate (" + juce::String(sampleRate) +
            " Hz). Resampling not supported in v0.1."
        );
    }

    // Compute render length
    const int64_t lengthSamples = computeProjectLengthSamples(project);
    const int64_t tailSamples = static_cast<int64_t>(std::llround(options.tailSeconds * sampleRate));
    const int64_t totalSamples = lengthSamples + tailSamples;

    if (totalSamples <= 0)
        return juce::Result::fail("Project has no audio to render (no unmuted clips).");

    DBG("Offline render: " << totalSamples << " samples ("
        << (totalSamples / sampleRate) << " seconds) @ "
        << options.bitsPerSample << "-bit " << options.getFormatName());

    // Ensure output directory exists
    auto parentDir = outputFile.getParentDirectory();
    if (!parentDir.exists() && !parentDir.createDirectory())
    {
        return juce::Result::fail(
            "Failed to create output directory: " + parentDir.getFullPathName()
        );
    }

#if ZENITH_ENABLE_PHASE1_AUDIO
    // Create temporary engine for offline rendering
    Engine offlineEngine;

    // Initialize engine (offline mode - no audio device)
    if (!offlineEngine.initialize())
    {
        return juce::Result::fail("Failed to initialize offline engine.");
    }

    // Load project into engine via playback context
    ProjectPlaybackContext playback(offlineEngine);
    playback.loadFromModel(project, outputFile.getParentDirectory());

    // Seek to start (t=0)
    offlineEngine.seekSamples(0);

    // Create appropriate audio format
    auto audioFormat = createAudioFormat(options);
    if (!audioFormat)
    {
        return juce::Result::fail("Failed to create audio format writer.");
    }

    // Create output stream
    std::unique_ptr<juce::FileOutputStream> outStream(outputFile.createOutputStream());

    if (!outStream || !outStream->openedOk())
    {
        return juce::Result::fail(
            "Failed to open output file for writing: " + outputFile.getFullPathName()
        );
    }

    const int numChannels = 2; // v0.1: Always stereo

    // Create audio format writer
    std::unique_ptr<juce::AudioFormatWriter> writer(
        audioFormat->createWriterFor(
            outStream.get(),
            sampleRate,
            static_cast<unsigned int>(numChannels),
            options.bitsPerSample,
            {},  // metadata (empty)
            0    // quality hint (0 = default quality)
        )
    );

    if (writer == nullptr)
    {
        return juce::Result::fail(
            "Failed to create audio writer for: " + outputFile.getFullPathName() +
            " (format: " + options.getFormatName() +
            ", bit depth: " + juce::String(options.bitsPerSample) + ")"
        );
    }

    // Writer now owns the stream
    outStream.release();

    // Allocate processing buffer
    juce::AudioBuffer<float> blockBuffer(numChannels, options.blockSize);

    // Offline render loop
    int64_t rendered = 0;
    while (rendered < totalSamples)
    {
        // Compute samples to process in this block
        const int remaining = static_cast<int>(
            juce::jmin<int64_t>(options.blockSize, totalSamples - rendered)
        );

        // Clear buffer
        blockBuffer.clear();

        // Process audio via engine's offline path
        offlineEngine.processBlockOffline(blockBuffer, remaining);

        // Write to audio file
        if (!writer->writeFromAudioSampleBuffer(blockBuffer, 0, remaining))
        {
            return juce::Result::fail(
                "Error while writing audio data to: " + outputFile.getFullPathName()
            );
        }

        rendered += remaining;

        // Progress feedback (v0.1: just log every 10%)
        if (rendered % (totalSamples / 10 + 1) == 0 && rendered > 0)
        {
            int percent = static_cast<int>((rendered * 100) / totalSamples);
            DBG("Offline render progress: " << percent << "%");
        }
    }

    // Flush and close
    writer->flush();

    DBG("Offline render complete: " << outputFile.getFullPathName()
        << " (" << options.getFormatName() << ", "
        << options.bitsPerSample << "-bit)");

    return juce::Result::ok();

#else
    juce::ignoreUnused(project, outputFile, sampleRate, options);
    return juce::Result::fail("Phase 1 audio not enabled (ZENITH_ENABLE_PHASE1_AUDIO=OFF)");
#endif
}

} // namespace zenith
