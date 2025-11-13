/**
 * @file OfflineRender.cpp
 * @brief Implementation of offline rendering
 */

#include "../../include/render/OfflineRender.h"
#include "../../include/Engine.h"
#include "../../include/playback/ArrangementPlaybackController.h"

namespace zenith
{
    //==========================================================================
    // Helper Functions
    //==========================================================================

    /**
     * @brief Compute the length of the project in samples
     *
     * Finds the end position of the last non-muted clip across all tracks.
     *
     * @param project Project to analyze
     * @return Length in samples (0 if no audible clips)
     */
    static juce::int64 computeProjectLengthSamples(const ProjectModel& project)
    {
        juce::int64 maxEnd = 0;

        for (const auto& track : project.tracks)
        {
            for (const auto& clip : track.clips)
            {
                // Skip muted clips
                if (clip.muted)
                    continue;

                const auto start = juce::jmax<juce::int64>(0, clip.startSample);
                const auto len   = juce::jmax<juce::int64>(0, clip.lengthSamples);
                const auto end   = start + len;

                if (end > maxEnd)
                    maxEnd = end;
            }
        }

        return maxEnd;
    }

    //==========================================================================
    // Public API
    //==========================================================================

    juce::Result renderProjectToWav(const ProjectModel& project,
                                    const juce::File& outputFile,
                                    double sampleRate,
                                    int blockSize,
                                    double tailSeconds)
    {
        // MESSAGE THREAD ONLY

        DBG("OfflineRender: Starting export to " + outputFile.getFullPathName());

        // 0. Basic validation
        if (sampleRate <= 0.0)
        {
            return juce::Result::fail("Invalid sample rate for export: " +
                                      juce::String(sampleRate, 1) + " Hz");
        }

        if (project.sampleRate <= 0.0)
        {
            return juce::Result::fail("Project has invalid sample rate: " +
                                      juce::String(project.sampleRate, 1) + " Hz");
        }

        // v0.1: No resampling allowed
        if (std::abs(project.sampleRate - sampleRate) > 1e-3)
        {
            return juce::Result::fail(
                "Export sample rate must match project sample rate (v0.1). " +
                "Project: " + juce::String(project.sampleRate, 1) + " Hz, " +
                "Export: " + juce::String(sampleRate, 1) + " Hz");
        }

        if (blockSize <= 0)
            blockSize = 1024;

        if (tailSeconds < 0.0)
            tailSeconds = 0.0;

        // 1. Compute total samples to render
        const auto projectLengthSamples = computeProjectLengthSamples(project);

        if (projectLengthSamples <= 0)
        {
            return juce::Result::fail("Project has no audible clips to export");
        }

        const auto tailSamples  = static_cast<juce::int64>(std::llround(tailSeconds * sampleRate));
        const auto totalSamples = projectLengthSamples + tailSamples;

        DBG("OfflineRender: Project length = " + juce::String(projectLengthSamples) + " samples");
        DBG("OfflineRender: Tail = " + juce::String(tailSamples) + " samples");
        DBG("OfflineRender: Total = " + juce::String(totalSamples) + " samples (" +
            juce::String(static_cast<double>(totalSamples) / sampleRate, 2) + " seconds)");

        // 2. Ensure output directory exists
        auto parentDir = outputFile.getParentDirectory();

        if (!parentDir.exists())
        {
            DBG("OfflineRender: Creating parent directory: " + parentDir.getFullPathName());

            if (!parentDir.createDirectory())
            {
                return juce::Result::fail("Failed to create export directory: " +
                                          parentDir.getFullPathName());
            }
        }

        // 3. Create temporary engine and playback controller
        DBG("OfflineRender: Creating engine and loading project");

        Engine engine;
        engine.prepareOffline(sampleRate, blockSize, 2); // 2 channels (stereo)

        ArrangementPlaybackController playback(engine);
        playback.setProject(project); // Load model → engine (decodes PCM, sets tracks/clips)

        // Ensure transport starts at 0
        engine.seekSamples(0);

        // 4. Create WAV writer (32-bit float, stereo)
        DBG("OfflineRender: Creating WAV writer");

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> outStream(outputFile.createOutputStream());

        if (outStream == nullptr || !outStream->openedOk())
        {
            return juce::Result::fail("Failed to open output file: " +
                                      outputFile.getFullPathName());
        }

        const int numChannels = 2;
        const int bitsPerSample = 32; // 32-bit float

        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(outStream.release(),
                                      sampleRate,
                                      static_cast<unsigned int>(numChannels),
                                      bitsPerSample,
                                      {}, // metadata
                                      0   // quality (ignored for WAV)
            )
        );

        if (writer == nullptr)
        {
            return juce::Result::fail("Failed to create WAV writer");
        }

        // 5. Allocate offline buffer
        juce::AudioBuffer<float> blockBuffer(numChannels, blockSize);

        // 6. Render loop
        DBG("OfflineRender: Starting render loop");

        juce::int64 samplesRendered = 0;

        while (samplesRendered < totalSamples)
        {
            const int remaining = static_cast<int>(juce::jmin<juce::int64>(blockSize, totalSamples - samplesRendered));

            blockBuffer.clear();

            // Drive engine offline
            engine.processOfflineBlock(blockBuffer, remaining);

            // Write to disk
            if (!writer->writeFromAudioSampleBuffer(blockBuffer, 0, remaining))
            {
                return juce::Result::fail("Failed while writing audio data to disk at sample " +
                                          juce::String(samplesRendered));
            }

            samplesRendered += remaining;

            // Optional progress logging (every 10%)
            if ((samplesRendered % (totalSamples / 10 + 1)) == 0)
            {
                const double progress = 100.0 * static_cast<double>(samplesRendered) /
                                        static_cast<double>(totalSamples);
                DBG("OfflineRender: Progress: " + juce::String(progress, 1) + "%");
            }
        }

        // 7. Flush and close
        writer.reset(); // Ensures file is flushed and closed

        DBG("OfflineRender: Export complete - " + juce::String(samplesRendered) + " samples written");

        return juce::Result::ok();
    }
}
