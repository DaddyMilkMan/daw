/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"
#include "../include/Track.h"

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

    // Enable test tone for Phase 0 testing
    // TODO: Remove this in Phase 1 when we have actual content
    enableTestTone_.store(true);
}

void Engine::pause()
{
    DBG("Engine: Pause");
    isPlaying_.store(false);
    // Keep playbackPosition - don't reset
}

void Engine::stop()
{
    DBG("Engine: Stop");
    isPlaying_.store(false);
    enableTestTone_.store(false);
    playbackPosition.store(0);
}

void Engine::seekSamples(SamplePos targetSample)
{
    // MESSAGE THREAD ONLY - must not be called while playing
    jassert(!isPlaying_.load());

    DBG("Engine: Seek to sample " + juce::String(targetSample));
    playbackPosition.store(targetSample);
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
// Track Management
//==============================================================================

void Engine::setNumTracks(int numTracks)
{
    // MESSAGE THREAD ONLY
    DBG("Engine: Setting number of tracks to " + juce::String(numTracks));

    tracks_.clear();
    tracks_.reserve(numTracks);

    for (int i = 0; i < numTracks; ++i)
    {
        auto trackName = "Track " + juce::String(i + 1);
        tracks_.push_back(std::make_unique<Track>(trackName, true)); // true = audio track
    }
}

Track* Engine::getTrack(int index)
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();

    return nullptr;
}

const Track* Engine::getTrack(int index) const
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();

    return nullptr;
}

//==============================================================================
// Offline Rendering
//==============================================================================

void Engine::prepareOffline(double sampleRate, int blockSize, int numChannels)
{
    // MESSAGE THREAD ONLY - no RT constraints

    DBG("Engine: Preparing for offline rendering - " +
        juce::String(sampleRate, 1) + " Hz, " +
        juce::String(blockSize) + " samples, " +
        juce::String(numChannels) + " channels");

    offlineSampleRate_ = sampleRate;
    offlineBlockSize_ = blockSize;
    offlineNumChannels_ = numChannels;

    // Allocate offline mix buffer
    offlineMixBuffer_.setSize(numChannels, blockSize);
    offlineMixBuffer_.clear();

    // Store settings for tracks to use
    currentSampleRate.store(sampleRate);
    currentBufferSize.store(blockSize);

    // Reset transport
    playbackPosition.store(0);
    isPlaying_.store(false);
    phase = 0.0;

    DBG("Engine: Offline preparation complete");
}

void Engine::processOfflineBlock(juce::AudioBuffer<float>& outBuffer, int numSamples)
{
    // MESSAGE THREAD ONLY (offline rendering, no RT constraints)

    jassert(numSamples <= offlineBlockSize_);
    jassert(outBuffer.getNumChannels() == offlineNumChannels_);

    // Clear offline mix buffer
    offlineMixBuffer_.clear();

    // Get current transport position
    const double transportPositionSeconds =
        static_cast<double>(playbackPosition.load()) / offlineSampleRate_;

    // Process all tracks (reuse existing Track::processAudioBlock)
    for (auto& track : tracks_)
    {
        if (track == nullptr)
            continue;

        // Create AudioSourceChannelInfo for this track
        juce::AudioSourceChannelInfo channelInfo;
        channelInfo.buffer = &offlineMixBuffer_;
        channelInfo.startSample = 0;
        channelInfo.numSamples = numSamples;

        // Process track (this calls all clips and mixes)
        track->processAudioBlock(channelInfo, transportPositionSeconds);
    }

    // Advance transport
    playbackPosition.fetch_add(numSamples);

    // Copy mixed audio to output buffer
    const int numCh = juce::jmin(outBuffer.getNumChannels(), offlineMixBuffer_.getNumChannels());
    for (int ch = 0; ch < numCh; ++ch)
    {
        outBuffer.copyFrom(ch, 0, offlineMixBuffer_, ch, 0, numSamples);
    }
}

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void Engine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    // ⚠️ AUDIO THREAD CONTEXT
    // This is called from the audio thread before streaming starts
    // - No heap allocations
    // - No logging or String creation
    // - No locks or UI calls

    // Update settings
    currentSampleRate.store(device->getCurrentSampleRate());
    currentBufferSize.store(device->getCurrentBufferSizeSamples());

    // Reset state
    phase = 0.0;
    playbackPosition.store(0);

    // RT-SAFETY: No logging on audio thread!
    // Device info is available via getAudioDeviceInfo() on message thread
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
