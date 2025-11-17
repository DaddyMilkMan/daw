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

        // Sync tracks with project state
        syncWithProjectState();
    }
}

void Engine::syncWithProjectState()
{
    DBG("Engine: Syncing with project state");

    if (projectState_ == nullptr)
    {
        DBG("Engine: No project state, clearing tracks");
        tracks_.clear();
        return;
    }

    // Clear existing tracks
    tracks_.clear();

    // Get tracks from project state
    auto& state = projectState_->getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
    {
        DBG("Engine: No tracks in project state");
        return;
    }

    const double sampleRate = currentSampleRate.load();
    const int bufferSize = currentBufferSize.load();
    const double tempo = projectState_->getTempo();

    // Helper: convert beats to samples
    auto beatsToSamples = [tempo, sampleRate](double beats) -> juce::int64
    {
        const double secondsPerBeat = 60.0 / tempo;
        const double seconds = beats * secondsPerBeat;
        return static_cast<juce::int64>(seconds * sampleRate);
    };

    // Create engine tracks from project state
    for (auto trackNode : tracksNode)
    {
        juce::String trackName = trackNode[ProjectState::PROP_NAME].toString();
        juce::String trackType = trackNode[ProjectState::PROP_TYPE].toString();

        // Create track
        auto track = std::make_unique<zenith::Track>(
            trackName,
            trackType == "midi" ? zenith::Track::Type::MIDI : zenith::Track::Type::Audio);

        // Set mixer properties
        track->setVolume(trackNode[ProjectState::PROP_VOLUME]);
        track->setPan(trackNode[ProjectState::PROP_PAN]);
        track->setMuted(trackNode[ProjectState::PROP_MUTE]);
        track->setSolo(trackNode[ProjectState::PROP_SOLO]);

        // Prepare track for playback
        if (sampleRate > 0)
        {
            track->prepareToPlay(bufferSize, sampleRate);
        }

        // Load clips
        auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
        if (clipsNode.isValid())
        {
            for (auto clipNode : clipsNode)
            {
                // Create clip
                auto clip = std::make_unique<zenith::Track::Clip>();

                // Set basic properties
                double startBeats = clipNode[ProjectState::PROP_START];
                double lengthBeats = clipNode[ProjectState::PROP_LENGTH];

                clip->setStartPosition(beatsToSamples(startBeats));
                clip->setLength(beatsToSamples(lengthBeats));

                // Load audio file if present
                juce::String audioFilePath = clipNode[ProjectState::PROP_AUDIO_FILE].toString();
                if (audioFilePath.isNotEmpty())
                {
                    juce::File audioFile(audioFilePath);
                    if (audioFile.existsAsFile())
                    {
                        clip->setAudioFile(audioFile);
                        clip->setType(zenith::Track::Clip::Type::Audio);
                        DBG("Engine: Loaded audio file: " + audioFile.getFileName());
                    }
                    else
                    {
                        DBG("Engine: Warning - audio file not found: " + audioFilePath);
                    }
                }

                // Prepare clip
                if (sampleRate > 0)
                {
                    clip->prepareToPlay(bufferSize, sampleRate);
                }

                // Set clip as playing (so it's active during playback)
                clip->setPlaying(true);

                // Add clip to track
                track->addClip(std::move(clip));
            }
        }

        // Add track to engine
        tracks_.push_back(std::move(track));
    }

    DBG("Engine: Synced " + juce::String(tracks_.size()) + " tracks");
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

    // Reset playback position
    playbackPosition.store(0);

    // Phase 13: Stop automation synchronizer
    if (automationSynchronizer)
    {
        automationSynchronizer->stop();
        DBG("Engine: Stopped automation synchronizer");
    }
}

double Engine::getPlaybackPositionBeats() const
{
    if (projectState_ == nullptr)
        return 0.0;

    const double tempo = projectState_->getTempo();
    const double sampleRate = currentSampleRate.load();
    const juce::int64 positionSamples = playbackPosition.load();

    // Convert samples to beats
    const double seconds = static_cast<double>(positionSamples) / sampleRate;
    const double beats = (seconds * tempo) / 60.0;

    return beats;
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

    // Allocate mix buffer
    mixBuffer.setSize(2, device->getCurrentBufferSizeSamples());

    // Prepare tracks for playback
    const double sampleRate = device->getCurrentSampleRate();
    const int bufferSize = device->getCurrentBufferSizeSamples();

    for (auto& track : tracks_)
    {
        if (track != nullptr)
        {
            track->prepareToPlay(bufferSize, sampleRate);
        }
    }

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

    // Clear output first
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] != nullptr)
        {
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
        }
    }

    // Check if we should use test tone (for compatibility)
    bool testToneEnabled = enableTestTone_.load();

    // If we have tracks, process them
    if (!tracks_.empty() && !testToneEnabled)
    {
        // Get current playback position
        const juce::int64 currentPosition = playbackPosition.load();

        // Clear mix buffer
        mixBuffer.clear();

        // Process each track
        for (auto& track : tracks_)
        {
            if (track == nullptr)
                continue;

            // Update transport position for all clips in this track
            for (int i = 0; i < track->getNumClips(); ++i)
            {
                auto* clip = track->getClip(i);
                if (clip != nullptr)
                {
                    clip->setTransportPosition(currentPosition);
                }
            }

            // Get audio from track
            juce::AudioSourceChannelInfo info(&mixBuffer, 0, numSamples);
            track->getNextAudioBlock(info);

            // Mix track into output
            for (int channel = 0; channel < juce::jmin(numOutputChannels, mixBuffer.getNumChannels()); ++channel)
            {
                if (outputChannelData[channel] != nullptr)
                {
                    juce::FloatVectorOperations::add(
                        outputChannelData[channel],
                        mixBuffer.getReadPointer(channel),
                        numSamples);
                }
            }

            // Clear mix buffer for next track
            mixBuffer.clear();
        }
    }
    else if (testToneEnabled)
    {
        // Fallback: Generate test tone (for testing without project)
        const double sampleRate = currentSampleRate.load();
        const double frequency = 440.0;  // A4
        const double amplitude = 0.25;   // -12 dB
        const double phaseIncrement = frequency * 2.0 * juce::MathConstants<double>::pi / sampleRate;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float value = static_cast<float>(std::sin(phase) * amplitude);

            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                if (outputChannelData[channel] != nullptr)
                {
                    outputChannelData[channel][sample] = value;
                }
            }

            phase += phaseIncrement;
            if (phase >= 2.0 * juce::MathConstants<double>::pi)
                phase -= 2.0 * juce::MathConstants<double>::pi;
        }
    }
}
