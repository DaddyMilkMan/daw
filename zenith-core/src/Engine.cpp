/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"

#ifdef _WIN32
    #include "../Source/win/WinRtAudioPriority.h"
#endif

#if ZENITH_ENABLE_PHASE1_AUDIO
    #include "../Source/engine/Track.h"
#endif

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
// W10: Track Management
//==============================================================================

int Engine::getNumTracks() const
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    return static_cast<int>(tracks_.size());
#else
    return 0;
#endif
}

zenith::Track* Engine::getTrack(int index)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();
#endif
    return nullptr;
}

void Engine::addTestTracks(int count)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    DBG("Engine: Adding " + juce::String(count) + " test tracks");

    for (int i = 0; i < count; ++i)
    {
        auto track = std::make_unique<zenith::Track>();
        track->setName("Track " + juce::String(i + 1));

        // Prepare track if audio is running
        if (currentSampleRate.load() > 0)
            track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());

        tracks_.push_back(std::move(track));
    }

    DBG("Engine: Total tracks: " + juce::String(tracks_.size()));
#else
    juce::ignoreUnused(count);
    DBG("Engine: W10 disabled - addTestTracks() requires ZENITH_ENABLE_PHASE1_AUDIO");
#endif
}

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void Engine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    DBG("Engine: Audio device starting...");

    // Update settings
    const double sampleRate = device->getCurrentSampleRate();
    const int blockSize = device->getCurrentBufferSizeSamples();

    currentSampleRate.store(sampleRate);
    currentBufferSize.store(blockSize);

    // Reset state
    phase = 0.0;
    playbackPosition.store(0);

#if ZENITH_ENABLE_PHASE1_AUDIO
    // W10: Pre-allocate mix buffer once, no RT allocs afterwards
    const int numOutputs = juce::jmax(1, device->getActiveOutputChannels().countNumberOfSetBits());
    mixBuffer_.setSize(numOutputs, blockSize, false, true, true);
    mixBuffer_.clear();

    // W10: Prepare all tracks (no RT allocs expected inside)
    for (auto& track : tracks_)
    {
        if (track)
            track->prepareToPlay(blockSize, sampleRate);
    }

    DBG("W10: Mix buffer allocated (" + juce::String(numOutputs) + " channels, "
        + juce::String(blockSize) + " samples)");
    DBG("W10: Prepared " + juce::String(tracks_.size()) + " tracks");
#endif

    DBG("Engine: Audio device started");
    DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
    DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
    DBG("  MMCSS will be registered on first audio callback");
}

void Engine::audioDeviceStopped()
{
    DBG("Engine: Audio device stopped");

#if ZENITH_ENABLE_PHASE1_AUDIO
    // W10: Release track resources
    for (auto& track : tracks_)
    {
        if (track)
            track->releaseResources();
    }

    // W10: Release mix buffer
    mixBuffer_.setSize(0, 0);

    DBG("W10: Released resources");
#endif
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

    // Windows: Register MMCSS on first callback (correct thread)
    // NOTE: Static construction happens once, not per-callback
    #ifdef _WIN32
        static MMCSSAudioPriority audioPriority(L"Pro Audio");
        juce::ignoreUnused(audioPriority);
    #endif

    juce::ignoreUnused(inputChannelData, numInputChannels, context);

    // ===================== W10 gated path =====================
#if ZENITH_ENABLE_PHASE1_AUDIO
    // Advance transport (simple free-run when playing)
    if (isPlaying_.load(std::memory_order_acquire))
        playbackPosition.fetch_add(numSamples, std::memory_order_acq_rel);

    // Size-check (device may change block at runtime)
    if (mixBuffer_.getNumChannels() != numOutputChannels || mixBuffer_.getNumSamples() != numSamples)
        mixBuffer_.setSize(juce::jmax(1, numOutputChannels), numSamples, false, true, true);

    // One clear per callback
    mixBuffer_.clear();

    // Mix all tracks (each track must NOT clear the buffer internally)
    // W13: Pass transport position for sample-accurate clip rendering
    const juce::int64 currentTransportPos = playbackPosition.load(std::memory_order_relaxed);

    for (auto& track : tracks_)
    {
        if (track)
            track->processBlock(mixBuffer_, numSamples, currentTransportPos);
    }

    // Master gain/pan (constant per block; no per-sample allocs or branches)
    const float g = masterGain_.load(std::memory_order_relaxed);
    const float p = masterPan_.load(std::memory_order_relaxed); // -1..1
    float gL = g, gR = g;

    if (numOutputChannels >= 2)
    {
        // Equal-power-ish pan law
        const float l = juce::jlimit(0.0f, 1.0f, 0.5f - 0.5f * p);
        const float r = juce::jlimit(0.0f, 1.0f, 0.5f + 0.5f * p);
        gL *= l * 2.0f;
        gR *= r * 2.0f;
    }

    // Apply to outputs
    if (numOutputChannels == 1)
    {
        // Mono output
        const float* inL = mixBuffer_.getReadPointer(0);
        juce::FloatVectorOperations::copyWithMultiply(outputChannelData[0], inL, gL, numSamples);

        #if JUCE_DEBUG
            // Compute peak for debug HUD
            lastPeakL_.store(juce::FloatVectorOperations::findMaximum(outputChannelData[0], numSamples),
                            std::memory_order_relaxed);
            lastPeakR_.store(lastPeakL_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        #endif
    }
    else if (numOutputChannels >= 2)
    {
        // Stereo output
        const float* inL = mixBuffer_.getReadPointer(0);
        const float* inR = mixBuffer_.getNumChannels() > 1 ? mixBuffer_.getReadPointer(1) : inL;

        juce::FloatVectorOperations::copyWithMultiply(outputChannelData[0], inL, gL, numSamples);
        juce::FloatVectorOperations::copyWithMultiply(outputChannelData[1], inR, gR, numSamples);

        #if JUCE_DEBUG
            // Compute peaks for debug HUD (absolute max per channel)
            lastPeakL_.store(std::abs(juce::FloatVectorOperations::findMinAndMax(outputChannelData[0], numSamples).getEnd()),
                            std::memory_order_relaxed);
            lastPeakR_.store(std::abs(juce::FloatVectorOperations::findMinAndMax(outputChannelData[1], numSamples).getEnd()),
                            std::memory_order_relaxed);
        #endif

        // Clear any extra channels
        for (int ch = 2; ch < numOutputChannels; ++ch)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
    }
#else
    // ===================== Existing test-tone path (unchanged) =====================
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
#endif
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
