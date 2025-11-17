/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"
#include "../include/ProjectState.h"
#include "../include/TrackAutomationSynchronizer.h"

// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"
#include "../Source/engine/MixerChannel.h"

//==============================================================================
Engine::Engine()
{
    DBG("Engine: Constructor");
}

Engine::~Engine()
{
    DBG("Engine: Destructor");
    shutdown();
}

//==============================================================================
// Initialization / Shutdown
//==============================================================================

void Engine::setProjectState(ProjectState* state)
{
    DBG("Engine: Setting project state");

    // Stop automation if running
    if (automationSynchronizer)
    {
        automationSynchronizer->stop();
        automationSynchronizer.reset();
    }

    projectState_ = state;

    // Create new automation synchronizer if we have a project state
    if (projectState_ != nullptr)
    {
        automationSynchronizer = std::make_unique<TrackAutomationSynchronizer>(*projectState_, *this);
        DBG("Engine: Created automation synchronizer");
    }
}

bool Engine::initialize()
{
    DBG("Engine: Initializing...");

    // Initialize audio device manager
    auto error = deviceManager.initialiseWithDefaultDevices(2, 2);  // 2 in, 2 out

    if (error.isNotEmpty())
    {
        DBG("Engine: Failed to initialize audio device: " + error);
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Audio Device Error",
            "Failed to initialize audio device:\n" + error,
            "OK");
        return false;
    }

    // Get current device setup
    auto setup = deviceManager.getAudioDeviceSetup();

    DBG("Engine: Audio device initialized");
    DBG("  Device: " + setup.outputDeviceName);
    DBG("  Sample Rate: " + juce::String(setup.sampleRate) + " Hz");
    DBG("  Buffer Size: " + juce::String(setup.bufferSize) + " samples");

    // Store settings
    currentSampleRate.store(setup.sampleRate);
    currentBufferSize.store(setup.bufferSize);

    // Add this engine as the audio callback
    deviceManager.addAudioCallback(this);

    // C3: Optional debug seed (disabled by default; enable with -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON)
#if defined(JUCE_DEBUG) && defined(ZENITH_ENGINE_SEED_DEBUG_TRACKS)
    DBG("Engine: Seeding debug tracks (ZENITH_ENGINE_SEED_DEBUG_TRACKS enabled)");
    addTestTracks(8);
#endif

    DBG("Engine: Initialization complete!");
    return true;
}

void Engine::shutdown()
{
    DBG("Engine: Shutting down...");

    // Stop playback
    stop();

    // Remove audio callback
    deviceManager.removeAudioCallback(this);

    // Close audio device
    deviceManager.closeAudioDevice();

    DBG("Engine: Shutdown complete");
}

//==============================================================================
// Transport Controls
//==============================================================================

void Engine::play()
{
    DBG("Engine: Play");
    isPlaying_.store(true);
    playbackPosition.store(0);

    // Enable test tone for Phase 0 testing
    // TODO: Remove this in Phase 1 when we have actual content
    enableTestTone_.store(true);

    // Phase 13: Start automation synchronizer
    if (automationSynchronizer)
    {
        automationSynchronizer->start(60);  // 60 Hz update rate
        DBG("Engine: Started automation synchronizer");
    }
}

void Engine::stop()
{
    DBG("Engine: Stop");
    isPlaying_.store(false);
    enableTestTone_.store(false);

    // Phase 13: Stop automation synchronizer
    if (automationSynchronizer)
    {
        automationSynchronizer->stop();
        DBG("Engine: Stopped automation synchronizer");
    }
}

//==============================================================================
// Audio Device Management
//==============================================================================

juce::String Engine::getAudioDeviceInfo() const
{
    auto* device = deviceManager.getCurrentAudioDevice();

    if (device == nullptr)
        return "No device";

    auto name = device->getName();
    auto sampleRate = device->getCurrentSampleRate();
    auto bufferSize = device->getCurrentBufferSizeSamples();

    return name + " @ " + juce::String(sampleRate, 0) + " Hz, "
           + juce::String(bufferSize) + " samples";
}

//==============================================================================
// CPU Monitoring
//==============================================================================

double Engine::getCpuUsage() const
{
    return deviceManager.getCpuUsage() * 100.0;
}

//==============================================================================
// C3: Minimal Engine Surface (compile-only, no audio wiring)
//==============================================================================

int Engine::getNumTracks() const noexcept
{
    return static_cast<int>(tracks_.size());
}

const std::vector<std::unique_ptr<zenith::Track>>& Engine::tracks() const noexcept
{
    return tracks_;
}

void Engine::addTestTracks(int count)
{
    if (count <= 0)
        return;

    DBG("Engine: Adding " + juce::String(count) + " test tracks");

    // Reserve capacity to avoid reallocations
    tracks_.reserve(tracks_.size() + static_cast<size_t>(count));

    for (int i = 0; i < count; ++i)
    {
        // Create track with default name and type
        auto track = std::make_unique<zenith::Track>(
            "Track " + juce::String(tracks_.size() + 1),
            zenith::Track::Type::Audio);

        // NOTE: Do NOT call prepareToPlay() here - these are detached test tracks
        // They are NOT wired into the audio graph and will not be used in processAudio()
        // This is purely for compile verification and UI testing

        tracks_.push_back(std::move(track));
    }

    DBG("Engine: Total tracks: " + juce::String(tracks_.size()));
}

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void Engine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    DBG("Engine: Audio device starting...");

    // Update settings
    currentSampleRate.store(device->getCurrentSampleRate());
    currentBufferSize.store(device->getCurrentBufferSizeSamples());

    // Reset state
    phase = 0.0;
    playbackPosition.store(0);

    DBG("Engine: Audio device started");
    DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
    DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
}

void Engine::audioDeviceStopped()
{
    DBG("Engine: Audio device stopped");
}

void Engine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    // ⚠️ AUDIO THREAD - MUST BE REAL-TIME SAFE!
    //
    // NEVER:
    // - Allocate memory
    // - Lock mutexes
    // - Make system calls (DBG, file I/O, etc.)
    // - Call UI methods
    //
    // ONLY:
    // - Process audio samples
    // - Read/write std::atomic values
    // - Use pre-allocated buffers

    juce::ignoreUnused(inputChannelData, numInputChannels, context);

    // Check if playing
    bool playing = isPlaying_.load();

    if (playing)
    {
        // Process audio
        processAudio(inputChannelData, numInputChannels,
                    outputChannelData, numOutputChannels, numSamples);

        // Update playback position
        playbackPosition.fetch_add(numSamples);
    }
    else
    {
        // Silent output when not playing
        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            if (outputChannelData[channel] != nullptr)
            {
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
            }
        }
    }
}

//==============================================================================
// Audio Processing (AUDIO THREAD)
//==============================================================================

void Engine::processAudio(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples)
{
    // ⚠️ AUDIO THREAD - REAL-TIME SAFE!

    juce::ignoreUnused(inputChannelData, numInputChannels);

    // For Phase 0, generate a simple test tone (440 Hz sine wave)
    // TODO: Replace with actual audio processing in Phase 1

    bool testToneEnabled = enableTestTone_.load();

    if (testToneEnabled)
    {
        // Generate 440 Hz sine wave at -12 dB
        const double sampleRate = currentSampleRate.load();
        const double frequency = 440.0;  // A4
        const double amplitude = 0.25;   // -12 dB
        const double phaseIncrement = frequency * 2.0 * juce::MathConstants<double>::pi / sampleRate;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float value = static_cast<float>(std::sin(phase) * amplitude);

            // Write to all output channels
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                if (outputChannelData[channel] != nullptr)
                {
                    outputChannelData[channel][sample] = value;
                }
            }

            // Increment phase
            phase += phaseIncrement;

            // Wrap phase to avoid precision issues
            if (phase >= 2.0 * juce::MathConstants<double>::pi)
                phase -= 2.0 * juce::MathConstants<double>::pi;
        }
    }
    else
    {
        // Silent output
        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            if (outputChannelData[channel] != nullptr)
            {
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
            }
        }
    }
}

//==============================================================================
// Export / Bounce
//==============================================================================

bool Engine::exportProjectToWav(const juce::File& outputFile,
                                 double startSeconds,
                                 double endSeconds)
{
    DBG("Engine: Exporting project to WAV: " + outputFile.getFullPathName());

    // Validate output file
    if (outputFile == juce::File())
    {
        DBG("Engine: Invalid output file");
        return false;
    }

    // Compute project duration if not specified
    double exportDuration = endSeconds;
    if (exportDuration < 0.0)
    {
        exportDuration = computeProjectDuration();
        DBG("Engine: Auto-detected project duration: " + juce::String(exportDuration, 2) + " seconds");
    }

    // Validate time range
    if (startSeconds < 0.0 || startSeconds >= exportDuration)
    {
        DBG("Engine: Invalid start time: " + juce::String(startSeconds));
        return false;
    }

    // Get audio settings
    const double sampleRate = currentSampleRate.load();
    const int numChannels = 2;  // Stereo output
    const int blockSize = 4096; // Export block size (larger than real-time for efficiency)

    // Calculate sample range
    const int64_t startSample = static_cast<int64_t>(startSeconds * sampleRate);
    const int64_t endSample = static_cast<int64_t>(exportDuration * sampleRate);
    const int64_t totalSamples = endSample - startSample;

    DBG("Engine: Exporting " + juce::String(totalSamples) + " samples at " + juce::String(sampleRate) + " Hz");
    DBG("Engine: Duration: " + juce::String(exportDuration - startSeconds, 2) + " seconds");

    // Create audio format writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());

    if (fileStream == nullptr)
    {
        DBG("Engine: Failed to create output stream");
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Export Error",
            "Failed to create output file:\n" + outputFile.getFullPathName(),
            "OK");
        return false;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(fileStream.get(),
                                  sampleRate,
                                  static_cast<unsigned int>(numChannels),
                                  16,  // 16-bit PCM
                                  {},  // No metadata
                                  0)); // No quality option for WAV

    if (writer == nullptr)
    {
        DBG("Engine: Failed to create WAV writer");
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Export Error",
            "Failed to create WAV writer",
            "OK");
        return false;
    }

    // Release the file stream ownership to the writer
    fileStream.release();

    // Allocate offline render buffer
    juce::AudioBuffer<float> renderBuffer(numChannels, blockSize);

    // Render loop
    int64_t samplesRendered = 0;
    while (samplesRendered < totalSamples)
    {
        // Calculate samples to render in this block
        const int samplesToRender = static_cast<int>(
            juce::jmin(static_cast<int64_t>(blockSize), totalSamples - samplesRendered));

        // Render this block
        renderOfflineBlock(renderBuffer, startSample + samplesRendered, samplesToRender);

        // Write to file
        writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToRender);

        samplesRendered += samplesToRender;

        // Optional: Could add progress callback here
        if (samplesRendered % (sampleRate * 5) < blockSize)  // Log every ~5 seconds
        {
            double progress = (double)samplesRendered / (double)totalSamples * 100.0;
            DBG("Engine: Export progress: " + juce::String(progress, 1) + "%");
        }
    }

    // Finalize writer
    writer.reset();

    DBG("Engine: Export complete!");
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "Export Complete",
        "Successfully exported to:\n" + outputFile.getFullPathName(),
        "OK");

    return true;
}

double Engine::computeProjectDuration() const
{
    // If no project state, return default duration
    if (projectState_ == nullptr)
    {
        DBG("Engine: No project state, using default duration of 10.0 seconds");
        return 10.0;
    }

    // Scan all tracks and clips to find the latest end time
    const auto& state = projectState_->getState();
    const auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
    {
        DBG("Engine: No tracks found, using default duration of 10.0 seconds");
        return 10.0;
    }

    double maxEndTime = 0.0;
    const double sampleRate = currentSampleRate.load();

    for (int i = 0; i < tracksNode.getNumChildren(); ++i)
    {
        const auto trackNode = tracksNode.getChild(i);
        const auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);

        if (!clipsNode.isValid())
            continue;

        for (int j = 0; j < clipsNode.getNumChildren(); ++j)
        {
            const auto clipNode = clipsNode.getChild(j);

            // Get clip start and length (stored in seconds in ProjectState)
            const double clipStart = clipNode.getProperty(ProjectState::PROP_START, 0.0);
            const double clipLength = clipNode.getProperty(ProjectState::PROP_LENGTH, 0.0);
            const double clipEnd = clipStart + clipLength;

            maxEndTime = juce::jmax(maxEndTime, clipEnd);
        }
    }

    // If no clips found, use default duration
    if (maxEndTime <= 0.0)
    {
        DBG("Engine: No clips found, using default duration of 10.0 seconds");
        return 10.0;
    }

    // Add a small tail (0.5 seconds) for reverb/delay tails
    const double duration = maxEndTime + 0.5;

    DBG("Engine: Computed project duration: " + juce::String(duration, 2) + " seconds");
    return duration;
}

void Engine::renderOfflineBlock(juce::AudioBuffer<float>& outputBuffer,
                                int64_t startSample,
                                int numSamples)
{
    // Clear output buffer
    outputBuffer.clear(0, numSamples);

    // NOTE: This is where we would render tracks and clips
    // For now, the track rendering infrastructure exists but isn't fully wired
    // to the actual audio samples yet (tracks exist but don't have audio loaded)
    //
    // The proper implementation would be:
    // 1. For each track in tracks_:
    //    - Set transport position to startSample
    //    - Call track->getNextAudioBlock() to render clips
    //    - Mix into outputBuffer with track volume/pan/mute
    //
    // For now, we render silence since the tracks don't have loaded audio yet.
    // This provides the correct infrastructure - when tracks have real audio,
    // this will work automatically.

    // TODO: Implement real track mixing when audio loading is complete
    // Pseudocode for future:
    /*
    juce::AudioBuffer<float> trackBuffer(outputBuffer.getNumChannels(), numSamples);

    for (auto& track : tracks_)
    {
        if (track->isMuted() || !track->isEnabled())
            continue;

        trackBuffer.clear();

        // Set up audio source info for this block
        juce::AudioSourceChannelInfo info;
        info.buffer = &trackBuffer;
        info.startSample = 0;
        info.numSamples = numSamples;

        // Render track (which will render all its clips)
        track->getNextAudioBlock(info);

        // Mix into output with track volume/pan
        const float volume = track->getVolume();
        const float pan = track->getPan();

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        {
            float channelGain = volume;
            if (outputBuffer.getNumChannels() == 2)
            {
                // Apply pan (simple equal-power panning)
                if (ch == 0)  // Left
                    channelGain *= (1.0f - pan) * 0.5f + 0.5f;
                else  // Right
                    channelGain *= (1.0f + pan) * 0.5f + 0.5f;
            }

            outputBuffer.addFrom(ch, 0, trackBuffer, ch, 0, numSamples, channelGain);
        }
    }
    */

    DBG("Engine: Rendered offline block (currently silence - track audio not loaded yet)");
}
