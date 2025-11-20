/**
 * @file ExportEngine.cpp
 * @brief Export engine implementation
 */

#include "ExportEngine.h"
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"

//==============================================================================
ExportEngine::ExportEngine()
{
    DBG("ExportEngine: Constructor");
}

ExportEngine::~ExportEngine()
{
    DBG("ExportEngine: Destructor");
}

//==============================================================================
// Export Implementation
//==============================================================================

bool ExportEngine::exportToWav(
    Engine& engine,
    const juce::File& outputFile,
    double startSeconds,
    double endSeconds,
    ProjectState* projectState,
    juce::String& errorMessage)
{
    // MESSAGE THREAD ONLY
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    DBG("ExportEngine: Starting export to " + outputFile.getFullPathName());
    DBG("  Start: " + juce::String(startSeconds) + "s");
    DBG("  End: " + juce::String(endSeconds) + "s");

    // Auto-detect end time if needed
    if (endSeconds <= 0.0)
    {
        if (projectState != nullptr)
        {
            double projectLength = calculateProjectLength(projectState);
            if (projectLength > 0.0)
            {
                endSeconds = projectLength + 0.5; // Add 0.5s safety pad
                DBG("  Auto-detected end: " + juce::String(endSeconds) + "s");
            }
            else
            {
                // No clips found, use default 10 seconds
                endSeconds = 10.0;
                DBG("  No clips found, using default: " + juce::String(endSeconds) + "s");
            }
        }
        else
        {
            // No project state, use default 10 seconds
            endSeconds = 10.0;
            DBG("  No project state, using default: " + juce::String(endSeconds) + "s");
        }
    }

    // Validate parameters
    if (startSeconds < 0.0)
    {
        errorMessage = "Start time cannot be negative";
        DBG("ExportEngine: ERROR - " + errorMessage);
        return false;
    }

    if (endSeconds <= startSeconds)
    {
        errorMessage = "End time must be greater than start time";
        DBG("ExportEngine: ERROR - " + errorMessage);
        return false;
    }

    // Get sample rate from engine
    sampleRate_ = engine.getSampleRate();
    if (sampleRate_ <= 0.0)
    {
        sampleRate_ = 44100.0; // Fallback
        DBG("ExportEngine: WARNING - Invalid sample rate, using 44100 Hz");
    }

    // Calculate total samples to render
    int64_t startSample = static_cast<int64_t>(startSeconds * sampleRate_);
    int64_t endSample = static_cast<int64_t>(endSeconds * sampleRate_);
    int64_t totalSamples = endSample - startSample;

    DBG("  Sample rate: " + juce::String(sampleRate_) + " Hz");
    DBG("  Total samples: " + juce::String(totalSamples));

    // Create output file
    outputFile.deleteFile(); // Delete if exists
    outputFile.create();

    // Create WAV format writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> fileStream(
        new juce::FileOutputStream(outputFile));

    if (fileStream->failedToOpen())
    {
        errorMessage = "Failed to create output file: " + outputFile.getFullPathName();
        DBG("ExportEngine: ERROR - " + errorMessage);
        return false;
    }

    // Create audio format writer (24-bit PCM, stereo)
    const int numChannels = 2;
    const int bitsPerSample = 24;
    const juce::StringPairArray metadataValues;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(
            fileStream.get(),
            sampleRate_,
            numChannels,
            bitsPerSample,
            metadataValues,
            0)); // quality hint (0 = default)

    if (writer == nullptr)
    {
        errorMessage = "Failed to create WAV writer";
        DBG("ExportEngine: ERROR - " + errorMessage);
        return false;
    }

    // Release ownership - writer now owns the stream
    fileStream.release();

    // Store original playback state
    bool wasPlaying = engine.isPlaying();

    // Stop playback during export
    if (wasPlaying)
    {
        engine.stop();
        DBG("ExportEngine: Stopped playback for export");
    }

    // Render in blocks
    const int blockSize = 512;
    juce::AudioBuffer<float> renderBuffer(numChannels, blockSize);
    int64_t samplesRendered = 0;

    DBG("ExportEngine: Rendering...");

    while (samplesRendered < totalSamples)
    {
        // Calculate samples to render in this block
        int samplesToRender = static_cast<int>(
            std::min(static_cast<int64_t>(blockSize),
                     totalSamples - samplesRendered));

        // Clear buffer
        renderBuffer.clear();

        // Render this block
        renderBlock(engine, renderBuffer, samplesToRender);

        // Write to file
        if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToRender))
        {
            errorMessage = "Failed to write audio data to file";
            DBG("ExportEngine: ERROR - " + errorMessage);
            writer.reset();
            outputFile.deleteFile();
            return false;
        }

        samplesRendered += samplesToRender;

        // Optional: Progress logging (every second of audio)
        if (samplesRendered % static_cast<int64_t>(sampleRate_) == 0)
        {
            double progress = static_cast<double>(samplesRendered) / totalSamples * 100.0;
            DBG("  Progress: " + juce::String(progress, 1) + "%");
        }
    }

    // Flush and close writer
    writer.reset();

    DBG("ExportEngine: Export complete!");
    DBG("  File: " + outputFile.getFullPathName());
    DBG("  Size: " + juce::String(outputFile.getSize() / 1024) + " KB");

    // Restore playback state if needed
    if (wasPlaying)
    {
        engine.play();
        DBG("ExportEngine: Restored playback");
    }

    return true;
}

//==============================================================================
// Helper Methods
//==============================================================================

double ExportEngine::calculateProjectLength(ProjectState* projectState)
{
    if (projectState == nullptr)
        return 0.0;

    // Get the state tree
    const auto& state = projectState->getState();

    // Find TRACKS node
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return 0.0;

    // Scan all clips to find the maximum end time
    double maxEndBeats = 0.0;

    for (int trackIdx = 0; trackIdx < tracksNode.getNumChildren(); ++trackIdx)
    {
        auto trackNode = tracksNode.getChild(trackIdx);
        if (!trackNode.hasType(ProjectState::ID_TRACK))
            continue;

        // Find CLIPS node
        auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
        if (!clipsNode.isValid())
            continue;

        for (int clipIdx = 0; clipIdx < clipsNode.getNumChildren(); ++clipIdx)
        {
            auto clipNode = clipsNode.getChild(clipIdx);
            if (!clipNode.hasType(ProjectState::ID_CLIP))
                continue;

            // Get clip start and length (in beats)
            double startBeats = clipNode.getProperty(ProjectState::PROP_START, 0.0);
            double lengthBeats = clipNode.getProperty(ProjectState::PROP_LENGTH, 0.0);
            double endBeats = startBeats + lengthBeats;

            if (endBeats > maxEndBeats)
                maxEndBeats = endBeats;
        }
    }

    if (maxEndBeats <= 0.0)
        return 0.0;

    // Convert beats to seconds using project tempo
    double tempo = projectState->getTempo();
    if (tempo <= 0.0)
        tempo = 120.0; // Fallback

    double secondsPerBeat = 60.0 / tempo;
    double lengthSeconds = maxEndBeats * secondsPerBeat;

    DBG("ExportEngine: Calculated project length: " + juce::String(lengthSeconds) + "s");
    DBG("  Max end: " + juce::String(maxEndBeats) + " beats");
    DBG("  Tempo: " + juce::String(tempo) + " BPM");

    return lengthSeconds;
}

void ExportEngine::renderBlock(
    Engine& engine,
    juce::AudioBuffer<float>& outputBuffer,
    int numSamples)
{
    // For now, we'll use the engine's test tone generator
    // In a real implementation, this would call the engine's mixing pipeline
    //
    // Since Engine::processAudio() is private, we need Engine to provide
    // a public method for offline rendering. For this implementation,
    // we'll generate a simple test tone directly here.
    //
    // TODO: Add Engine::renderOffline() method to properly integrate with
    // the actual mixing pipeline.

    const int numChannels = outputBuffer.getNumChannels();

    // Generate a simple 440 Hz test tone (same as Engine's test tone)
    // This matches the current Engine behavior
    // Use member variable instead of static to reset between exports
    const double frequency = 440.0;  // A4
    const double amplitude = 0.25;   // -12 dB
    const double phaseIncrement = frequency * 2.0 * juce::MathConstants<double>::pi / sampleRate_;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = static_cast<float>(std::sin(renderPhase_) * amplitude);

        // Write to all output channels
        for (int channel = 0; channel < numChannels; ++channel)
        {
            outputBuffer.setSample(channel, sample, value);
        }

        // Increment phase
        renderPhase_ += phaseIncrement;

        // Wrap phase to avoid precision issues
        if (renderPhase_ >= 2.0 * juce::MathConstants<double>::pi)
            renderPhase_ -= 2.0 * juce::MathConstants<double>::pi;
    }

    // Note: In a production implementation, this would:
    // 1. Set up a transport position
    // 2. Call Engine's track mixing code
    // 3. Apply master effects
    // 4. Handle automation
    // But for now, the test tone matches Engine's current behavior
}
