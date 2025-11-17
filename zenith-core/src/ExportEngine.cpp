/**
 * @file ExportEngine.cpp
 * @brief Offline WAV export implementation
 */

#include "../include/ExportEngine.h"
#include "../include/Engine.h"

//==============================================================================
// Export Implementation
//==============================================================================

bool ExportEngine::exportToWav(
    Engine& engine,
    const juce::File& outputFile,
    double startTime,
    double endTime,
    juce::String& errorMessage)
{
    // ⚠️ MESSAGE THREAD ONLY - Verify we're on the correct thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    DBG("ExportEngine: Starting export to " + outputFile.getFullPathName());
    DBG("ExportEngine: Time range: " + juce::String(startTime, 2) + "s - " + juce::String(endTime, 2) + "s");

    // Validate parameters
    if (endTime <= startTime)
    {
        errorMessage = "Invalid time range: end time must be greater than start time";
        DBG("ExportEngine: " + errorMessage);
        return false;
    }

    if (!outputFile.getParentDirectory().exists())
    {
        errorMessage = "Output directory does not exist";
        DBG("ExportEngine: " + errorMessage);
        return false;
    }

    // Get engine settings
    const double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0)
    {
        errorMessage = "Invalid sample rate from engine";
        DBG("ExportEngine: " + errorMessage);
        return false;
    }

    // Calculate export duration and samples
    const double duration = endTime - startTime;
    const int64_t totalSamples = static_cast<int64_t>(duration * sampleRate);

    DBG("ExportEngine: Sample rate: " + juce::String(sampleRate, 0) + " Hz");
    DBG("ExportEngine: Duration: " + juce::String(duration, 2) + "s");
    DBG("ExportEngine: Total samples: " + juce::String(totalSamples));

    // Create WAV format writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());

    if (fileStream == nullptr)
    {
        errorMessage = "Failed to create output file";
        DBG("ExportEngine: " + errorMessage);
        return false;
    }

    // Create audio writer (16-bit PCM, stereo)
    const int numChannels = 2;
    const int bitsPerSample = 16;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(
            fileStream.get(),
            sampleRate,
            static_cast<unsigned int>(numChannels),
            bitsPerSample,
            {},  // metadata
            0    // quality option
        )
    );

    if (writer == nullptr)
    {
        errorMessage = "Failed to create WAV writer";
        DBG("ExportEngine: " + errorMessage);
        return false;
    }

    // Release file stream ownership (writer now owns it)
    fileStream.release();

    DBG("ExportEngine: WAV writer created successfully");

    // Render and write audio in chunks
    const int chunkSize = 4096;  // Process 4096 samples at a time
    juce::AudioBuffer<float> renderBuffer(numChannels, chunkSize);
    int64_t samplesWritten = 0;

    while (samplesWritten < totalSamples)
    {
        const int samplesToWrite = static_cast<int>(
            juce::jmin(static_cast<int64_t>(chunkSize), totalSamples - samplesWritten)
        );

        // Calculate current time position
        const double currentTime = startTime + (static_cast<double>(samplesWritten) / sampleRate);

        // Render audio for this chunk
        renderAudioOffline(renderBuffer, sampleRate, currentTime, samplesToWrite);

        // Write to file
        if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToWrite))
        {
            errorMessage = "Failed to write audio data to file";
            DBG("ExportEngine: " + errorMessage);
            return false;
        }

        samplesWritten += samplesToWrite;

        // Optional: Update progress (could add a callback here in the future)
        if (samplesWritten % (static_cast<int64_t>(sampleRate) * 2) == 0)  // Every 2 seconds
        {
            const double progress = (static_cast<double>(samplesWritten) / static_cast<double>(totalSamples)) * 100.0;
            DBG("ExportEngine: Progress: " + juce::String(progress, 1) + "%");
        }
    }

    // Flush and close writer
    writer.reset();

    DBG("ExportEngine: Export complete! Wrote " + juce::String(samplesWritten) + " samples");
    DBG("ExportEngine: Output file: " + outputFile.getFullPathName());
    DBG("ExportEngine: File size: " + juce::String(outputFile.getSize() / 1024) + " KB");

    return true;
}

//==============================================================================
// Audio Rendering (Offline)
//==============================================================================

void ExportEngine::renderAudioOffline(
    juce::AudioBuffer<float>& buffer,
    double sampleRate,
    double startTime,
    int numSamples)
{
    // Phase 0: Generate silence for now
    // TODO: In Phase 1, integrate with actual audio processing:
    //       - Render all tracks
    //       - Apply mixer settings
    //       - Process automation
    //       - Apply effects/plugins

    juce::ignoreUnused(sampleRate, startTime);

    buffer.clear(0, numSamples);

    // Optional: Generate a test tone for verification
    // Uncomment to test export with a 440 Hz sine wave:
    /*
    const double frequency = 440.0;
    const double amplitude = 0.25;
    static double phase = 0.0;
    const double phaseIncrement = frequency * 2.0 * juce::MathConstants<double>::pi / sampleRate;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = static_cast<float>(std::sin(phase) * amplitude);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            buffer.setSample(channel, sample, value);
        }
        phase += phaseIncrement;
        if (phase >= 2.0 * juce::MathConstants<double>::pi)
            phase -= 2.0 * juce::MathConstants<double>::pi;
    }
    */
}
