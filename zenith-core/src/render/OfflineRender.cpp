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

} // anonymous namespace

//==============================================================================
// Public API
//==============================================================================

juce::Result renderProjectToWav(const ProjectModel& project,
                                const juce::File& outputFile,
                                double sampleRate,
                                int blockSize,
                                double tailSeconds)
{
    // Validate parameters
    if (sampleRate <= 0.0)
        return juce::Result::fail("Invalid sample rate for export.");

    if (blockSize <= 0)
        return juce::Result::fail("Invalid block size for export.");

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
    const int64_t tailSamples = static_cast<int64_t>(std::llround(tailSeconds * sampleRate));
    const int64_t totalSamples = lengthSamples + tailSamples;

    if (totalSamples <= 0)
        return juce::Result::fail("Project has no audio to render (no unmuted clips).");

    DBG("Offline render: " << totalSamples << " samples ("
        << (totalSamples / sampleRate) << " seconds)");

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

    // Schedule all clip events from the beginning
    // (ProjectPlaybackContext already scheduled them during loadFromModel)

    // Create WAV writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> outStream(outputFile.createOutputStream());

    if (!outStream || !outStream->openedOk())
    {
        return juce::Result::fail(
            "Failed to open output file for writing: " + outputFile.getFullPathName()
        );
    }

    const int numChannels = 2; // v0.1: Always stereo
    const int bitsPerSample = 32; // 32-bit float

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(
            outStream.get(),
            sampleRate,
            static_cast<unsigned int>(numChannels),
            bitsPerSample,
            {},  // metadata (empty)
            0    // quality hint (ignored for WAV)
        )
    );

    if (writer == nullptr)
    {
        return juce::Result::fail(
            "Failed to create WAV writer for: " + outputFile.getFullPathName()
        );
    }

    // Writer now owns the stream
    outStream.release();

    // Allocate processing buffer
    juce::AudioBuffer<float> blockBuffer(numChannels, blockSize);

    // Offline render loop
    int64_t rendered = 0;
    while (rendered < totalSamples)
    {
        // Compute samples to process in this block
        const int remaining = static_cast<int>(
            juce::jmin<int64_t>(blockSize, totalSamples - rendered)
        );

        // Clear buffer
        blockBuffer.clear();

        // Process audio via engine's offline path
        offlineEngine.processBlockOffline(blockBuffer, remaining);

        // Write to WAV file
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

    DBG("Offline render complete: " << outputFile.getFullPathName());
    return juce::Result::ok();

#else
    juce::ignoreUnused(project, outputFile, sampleRate, blockSize, tailSeconds);
    return juce::Result::fail("Phase 1 audio not enabled (ZENITH_ENABLE_PHASE1_AUDIO=OFF)");
#endif
}

} // namespace zenith
