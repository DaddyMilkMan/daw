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
#include "../Source/engine/RecordingManager.h"

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

    // Initialize recording manager
    recordingManager_ = std::make_unique<zenith::RecordingManager>();
    DBG("Engine: Recording manager initialized");

    // Start timer for processing recorded audio (50 Hz = 20ms interval)
    startTimer(20);

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

    // Stop playback and recording
    stop();
    if (isRecording())
        stopRecording();

    // Stop timer
    stopTimer();

    // Remove audio callback
    deviceManager.removeAudioCallback(this);

    // Close audio device
    deviceManager.closeAudioDevice();

    // Clean up recording manager
    recordingManager_.reset();

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

void Engine::startRecording()
{
    if (!recordingManager_)
    {
        DBG("Engine: Cannot start recording - RecordingManager not initialized");
        return;
    }

    if (isRecording_.load())
    {
        DBG("Engine: Already recording");
        return;
    }

    // Get current transport position
    const auto currentSample = playbackPosition.load();
    const double sampleRate = currentSampleRate.load();

    // TODO: Get current beat from tempo map (for now use sample-based positioning)
    const double currentBeat = 0.0;

    // Start recording
    recordingManager_->startRecording(sampleRate, currentBeat, currentSample);
    isRecording_.store(true);

    DBG("Engine: Started recording at sample " + juce::String(currentSample));
}

void Engine::stopRecording()
{
    if (!recordingManager_)
    {
        DBG("Engine: Cannot stop recording - RecordingManager not initialized");
        return;
    }

    if (!isRecording_.load())
    {
        DBG("Engine: Not recording");
        return;
    }

    // Stop recording flag first (audio thread will stop writing)
    isRecording_.store(false);

    // Collect armed tracks
    std::vector<zenith::Track*> armedTracks;
    for (auto& track : tracks_)
    {
        if (track && track->isArmed())
        {
            armedTracks.push_back(track.get());
        }
    }

    if (armedTracks.empty())
    {
        DBG("Engine: No armed tracks - recording will not create clips");
    }

    // Stop recording and create clips
    const int clipsCreated = recordingManager_->stopRecording(projectState_, armedTracks);

    DBG("Engine: Stopped recording, created " + juce::String(clipsCreated) + " clips");
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
    const double sampleRate = device->getCurrentSampleRate();
    const int bufferSize = device->getCurrentBufferSizeSamples();
    currentSampleRate.store(sampleRate);
    currentBufferSize.store(bufferSize);

    // Reset state
    phase = 0.0;
    playbackPosition.store(0);

    // Phase U3: Allocate mix buffer for track mixing
    trackMixBuffer_.setSize(2, bufferSize, false, true, true); // 2 channels, clear on allocation

    // Phase U3: Prepare all tracks for playback
    for (auto& track : tracks_)
    {
        if (track)
        {
            track->prepareToPlay(bufferSize, sampleRate);
        }
    }

    DBG("Engine: Audio device started");
    DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
    DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
    DBG("  Tracks prepared: " + juce::String(tracks_.size()));
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

    juce::ignoreUnused(context);

    // Check if playing
    bool playing = isPlaying_.load();
    bool recording = isRecording_.load();

    // Push input to recording manager if recording
    if (recording && recordingManager_)
    {
        recordingManager_->pushInputFromAudioCallback(
            inputChannelData,
            numInputChannels,
            numSamples);
    }
    else
    {
        juce::ignoreUnused(inputChannelData, numInputChannels);
    }

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

    // Clear output buffer first
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] != nullptr)
        {
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
        }
    }

    // Phase U3: Mix all tracks (if any exist)
    if (!tracks_.empty())
    {
        // Get current playback position
        const int64_t currentPosition = playbackPosition.load();

        for (auto& track : tracks_)
        {
            if (!track)
                continue;

            // Update transport position for all clips in this track
            for (size_t i = 0; i < track->getNumClips(); ++i)
            {
                auto* clip = track->getClip(i);
                if (clip)
                {
                    clip->setTransportPosition(currentPosition);
                    clip->setPlaying(true);
                }
            }

            // Clear the pre-allocated mix buffer
            trackMixBuffer_.clear();

            // Get audio from this track into mix buffer
            juce::AudioSourceChannelInfo info(&trackMixBuffer_, 0, numSamples);
            track->getNextAudioBlock(info);

            // Mix track audio into output
            const int channelsToMix = juce::jmin(numOutputChannels, trackMixBuffer_.getNumChannels());
            for (int channel = 0; channel < channelsToMix; ++channel)
            {
                if (outputChannelData[channel] != nullptr)
                {
                    juce::FloatVectorOperations::add(
                        outputChannelData[channel],
                        trackMixBuffer_.getReadPointer(channel),
                        numSamples);
                }
            }
        }
    }
    else
    {
        // Fallback: Phase 0 test tone (only if no tracks exist and test tone is enabled)
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
    }
}

//==============================================================================
// Timer Callback (MESSAGE THREAD)
//==============================================================================

void Engine::timerCallback()
{
    // Process recorded audio from ring buffer (runs on message thread)
    if (recordingManager_)
    {
        recordingManager_->processRecordedAudio();
    }
}
