/**
 * @file OfflineRender.cpp
 * @brief Implementation of offline rendering to WAV
 */

#include <render/OfflineRender.h>
#include <playback/ArrangementPlaybackController.h>
#include <Engine.h>

namespace zenith
{

//==============================================================================
// Helper Functions
//==============================================================================

/**
 * @brief Compute the total length of the project in samples
 * @param project Project to analyze
 * @return Length in samples (last clip end position)
 */
static juce::int64 computeProjectLengthSamples(const ProjectModel& project)
{
    juce::int64 maxEnd = 0;

    for (const auto& track : project.tracks)
    {
        for (const auto& clip : track.clips)
        {
            if (clip.muted)
                continue;

            // If lengthSamples == 0, we'll use the file length (handled by playback controller)
            // For computing project length, use the clip's specified length or 0
            const auto length = clip.lengthSamples;
            const auto clipEnd = clip.startSample + length;

            if (clipEnd > maxEnd)
                maxEnd = clipEnd;
        }
    }

    return maxEnd;
}

//==============================================================================
// Main Offline Render Function
//==============================================================================

juce::Result renderProjectToWav(const ProjectModel& project,
                                const juce::File& outputFile,
                                double sampleRate,
                                int blockSize,
                                double tailSeconds)
{
    // MESSAGE THREAD ONLY

    DBG("========================================");
    DBG("OfflineRender: Starting WAV export...");
    DBG("========================================");

    //==========================================================================
    // 1. Validate Parameters
    //==========================================================================

    if (sampleRate <= 0.0)
        return juce::Result::fail("Invalid sample rate: " + juce::String(sampleRate));

    if (blockSize <= 0)
        return juce::Result::fail("Invalid block size: " + juce::String(blockSize));

    if (tailSeconds < 0.0)
        return juce::Result::fail("Invalid tail seconds: " + juce::String(tailSeconds));

    // v0.1: No resampling - project SR must match export SR
    if (std::abs(project.sampleRate - sampleRate) > 1e-3)
    {
        return juce::Result::fail(
            "Project sample rate (" + juce::String(project.sampleRate) +
            ") does not match export sample rate (" + juce::String(sampleRate) +
            "). No resampling in v0.1.");
    }

    //==========================================================================
    // 2. Compute Render Length
    //==========================================================================

    const juce::int64 projectLengthSamples = computeProjectLengthSamples(project);
    const juce::int64 tailSamples = static_cast<juce::int64>(std::round(tailSeconds * sampleRate));
    const juce::int64 totalSamples = projectLengthSamples + tailSamples;

    if (totalSamples <= 0)
        return juce::Result::fail("Project has no audio to render (all clips muted or zero length).");

    DBG("OfflineRender: Project length: " + juce::String(projectLengthSamples) + " samples");
    DBG("OfflineRender: Tail: " + juce::String(tailSamples) + " samples");
    DBG("OfflineRender: Total: " + juce::String(totalSamples) + " samples");
    DBG("OfflineRender: Duration: " + juce::String(totalSamples / sampleRate, 2) + " seconds");

    //==========================================================================
    // 3. Set Up Engine (Throwaway Instance)
    //==========================================================================

    DBG("OfflineRender: Creating engine instance...");

    Engine engine;
    const int numChannels = 2;  // Stereo output

    engine.prepareOffline(sampleRate, blockSize, numChannels);

    //==========================================================================
    // 4. Load Project into Engine
    //==========================================================================

    DBG("OfflineRender: Loading project into engine...");

    ArrangementPlaybackController playback(engine);
    playback.setProject(project);

    // Ensure transport is at 0 (should already be, but be explicit)
    engine.stop();
    engine.seekSamples(0);

    //==========================================================================
    // 5. Set Up WAV Writer
    //==========================================================================

    DBG("OfflineRender: Opening output file: " + outputFile.getFullPathName());

    // Ensure parent directory exists
    auto parentDir = outputFile.getParentDirectory();
    if (!parentDir.exists())
    {
        if (!parentDir.createDirectory())
            return juce::Result::fail("Failed to create output directory: " + parentDir.getFullPathName());
    }

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> outStream(outputFile.createOutputStream());

    if (!outStream || !outStream->openedOk())
        return juce::Result::fail("Failed to open output file for writing: " + outputFile.getFullPathName());

    const int bitsPerSample = 32;  // 32-bit float

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outStream.get(), sampleRate,
                                 static_cast<unsigned int>(numChannels),
                                 bitsPerSample, {}, 0));

    if (writer == nullptr)
        return juce::Result::fail("Failed to create WAV writer for output file.");

    // Writer now owns the stream
    outStream.release();

    //==========================================================================
    // 6. Offline Render Loop
    //==========================================================================

    DBG("OfflineRender: Rendering audio...");

    juce::AudioBuffer<float> blockBuffer(numChannels, blockSize);
    juce::int64 rendered = 0;

    while (rendered < totalSamples)
    {
        const int remaining = static_cast<int>(std::min<juce::int64>(blockSize, totalSamples - rendered));

        // Clear buffer
        blockBuffer.clear();

        // Process one offline block using engine
        engine.processOfflineBlock(blockBuffer, remaining);

        // Write to WAV file
        if (!writer->writeFromAudioSampleBuffer(blockBuffer, 0, remaining))
            return juce::Result::fail("Failed while writing audio data to WAV file at sample " + juce::String(rendered));

        rendered += remaining;

        // Progress logging (every ~1 second of audio)
        if (rendered % static_cast<juce::int64>(sampleRate) == 0 || rendered == totalSamples)
        {
            const double progress = 100.0 * static_cast<double>(rendered) / static_cast<double>(totalSamples);
            DBG("OfflineRender: Progress: " + juce::String(progress, 1) + "% (" +
                juce::String(rendered) + " / " + juce::String(totalSamples) + " samples)");
        }
    }

    //==========================================================================
    // 7. Finalize and Close
    //==========================================================================

    writer->flush();
    writer.reset();  // Close file

    DBG("========================================");
    DBG("OfflineRender: Export complete!");
    DBG("  Output: " + outputFile.getFullPathName());
    DBG("  Size: " + juce::String(outputFile.getSize() / 1024) + " KB");
    DBG("========================================");

    return juce::Result::ok();
}

} // namespace zenith
