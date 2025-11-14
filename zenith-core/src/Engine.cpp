/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"

// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "engine/Track.h"
#include "engine/Clip.h"
#include "engine/MixerChannel.h"

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
}

void Engine::stop()
{
    DBG("Engine: Stop");
    isPlaying_.store(false);
    enableTestTone_.store(false);
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

        // Phase 11: Prepare track for audio processing if engine is already running
        if (currentSampleRate.load() > 0)
        {
            track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
        }

        tracks_.push_back(std::move(track));
    }

    DBG("Engine: Total tracks: " + juce::String(tracks_.size()));
}

//==============================================================================
// Phase 11: Mixer Control (MESSAGE THREAD ONLY)
//==============================================================================

void Engine::setTrackVolume(int trackIndex, float volume)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size()))
    {
        tracks_[trackIndex]->setVolume(volume);
    }
}

void Engine::setTrackPan(int trackIndex, float pan)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size()))
    {
        tracks_[trackIndex]->setPan(pan);
    }
}

void Engine::setTrackMute(int trackIndex, bool muted)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size()))
    {
        tracks_[trackIndex]->setMuted(muted);
    }
}

void Engine::setTrackSolo(int trackIndex, bool solo)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size()))
    {
        tracks_[trackIndex]->setSolo(solo);
    }
}

void Engine::setTrackArmed(int trackIndex, bool armed)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size()))
    {
        tracks_[trackIndex]->setArmed(armed);
    }
}

//==============================================================================
// Phase 11: Metering (MESSAGE THREAD SAFE)
//==============================================================================

float Engine::getTrackLevel(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size()))
    {
        return tracks_[trackIndex]->getCurrentLevel();
    }
    return 0.0f;
}

float Engine::getTrackPeakLevel(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size()))
    {
        return tracks_[trackIndex]->getPeakLevel();
    }
    return 0.0f;
}

float Engine::getMasterLevel() const
{
    return masterLevel_.load();
}

float Engine::getMasterPeakLevel() const
{
    return masterPeakLevel_.load();
}

void Engine::resetPeakMeters()
{
    // Reset master peak
    masterPeakLevel_.store(0.0f);

    // Reset all track peaks (message thread only)
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    for (auto& track : tracks_)
    {
        if (track != nullptr)
        {
            track->resetPeakLevel();
        }
    }
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

    // Phase 11: Allocate master mix buffer (pre-allocate to avoid RT allocations)
    masterMixBuffer_.setSize(2, currentBufferSize.load());

    // Phase 11: Prepare all tracks for playback
    for (auto& track : tracks_)
    {
        if (track != nullptr)
        {
            track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
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

    // Phase 11: Release resources from all tracks
    for (auto& track : tracks_)
    {
        if (track != nullptr)
        {
            track->releaseResources();
        }
    }
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
    //
    // Phase 11: Process all tracks and mix them down to master output

    juce::ignoreUnused(inputChannelData, numInputChannels);

    // Clear master mix buffer
    masterMixBuffer_.clear();

    // Phase 11: Process each track and mix into master buffer
    // Note: Track::getNextAudioBlock handles mute, volume, pan, and metering internally
    const size_t numTracks = tracks_.size();

    for (size_t i = 0; i < numTracks; ++i)
    {
        auto& track = tracks_[i];
        if (track != nullptr)
        {
            // Create channel info for this track
            juce::AudioSourceChannelInfo trackInfo;
            trackInfo.buffer = &masterMixBuffer_;
            trackInfo.startSample = 0;
            trackInfo.numSamples = numSamples;

            // Get audio from track (this also updates track meters)
            track->getNextAudioBlock(trackInfo);
        }
    }

    // Phase 11: Update master metering from mixed buffer
    float maxLevel = 0.0f;
    const int numChannels = juce::jmin(2, masterMixBuffer_.getNumChannels());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* channelData = masterMixBuffer_.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float absValue = std::abs(channelData[i]);
            if (absValue > maxLevel)
            {
                maxLevel = absValue;
            }
        }
    }

    // Update master level with smoothing
    const float currentMasterLevel = masterLevel_.load();
    const float smoothingFactor = 0.3f;
    const float newMasterLevel = currentMasterLevel * (1.0f - smoothingFactor) + maxLevel * smoothingFactor;
    masterLevel_.store(newMasterLevel);

    // Update master peak
    if (maxLevel > masterPeakLevel_.load())
    {
        masterPeakLevel_.store(maxLevel);
    }

    // Copy master mix buffer to output
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] != nullptr)
        {
            if (channel < masterMixBuffer_.getNumChannels())
            {
                // Copy from master mix buffer
                juce::FloatVectorOperations::copy(
                    outputChannelData[channel],
                    masterMixBuffer_.getReadPointer(channel),
                    numSamples);
            }
            else
            {
                // Clear extra output channels
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
            }
        }
    }

    // Phase 0 test tone (disabled in Phase 11 - tracks are now the audio source)
    // Keep this for future debugging if needed
    bool testToneEnabled = enableTestTone_.load();
    if (testToneEnabled)
    {
        // Generate 440 Hz sine wave at -12 dB and ADD to output (for testing)
        const double sampleRate = currentSampleRate.load();
        const double frequency = 440.0;  // A4
        const double amplitude = 0.25;   // -12 dB
        const double phaseIncrement = frequency * 2.0 * juce::MathConstants<double>::pi / sampleRate;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float value = static_cast<float>(std::sin(phase) * amplitude);

            // Add to all output channels (not replace)
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                if (outputChannelData[channel] != nullptr)
                {
                    outputChannelData[channel][sample] += value;
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
