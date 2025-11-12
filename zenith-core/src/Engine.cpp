/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"

#ifdef _WIN32
    #include "../Source/win/WinRtAudioPriority.h"
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
    DBG("  MMCSS will be registered on first audio callback");
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

    // Windows: Register MMCSS on first callback (correct thread)
    // NOTE: Static construction happens once, not per-callback
    #ifdef _WIN32
        static MMCSSAudioPriority audioPriority(L"Pro Audio");
        juce::ignoreUnused(audioPriority);
    #endif

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

#if ZENITH_ENABLE_PHASE1_AUDIO
    // Phase 1: Drain scheduled events for this block
    const int64_t blockStart = transportSamples_.load(std::memory_order_relaxed);
    const int64_t blockEnd = blockStart + numSamples;
    drainScheduledEvents(blockStart, blockEnd, numSamples);

    // Update transport position
    transportSamples_.fetch_add(numSamples, std::memory_order_relaxed);
#endif
}

#if ZENITH_ENABLE_PHASE1_AUDIO
//==============================================================================
// Phase 1: Sample-Accurate Scheduling Implementation
//==============================================================================

bool Engine::scheduleClipStart(int trackIndex, int64_t clipId, int64_t startSample)
{
    TransportEvent ev(TransportEventType::ClipStart, startSample, trackIndex, clipId);

    if (!eventQ_.tryPush(ev))
    {
        droppedEvents_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    return true;
}

bool Engine::scheduleClipStop(int trackIndex, int64_t clipId, int64_t stopSample)
{
    TransportEvent ev(TransportEventType::ClipStop, stopSample, trackIndex, clipId);

    if (!eventQ_.tryPush(ev))
    {
        droppedEvents_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    return true;
}

void Engine::seekSamples(int64_t targetSample)
{
    // Update transport position
    transportSamples_.store(targetSample, std::memory_order_release);

    // Purge all queued events < targetSample (they're now stale)
    // This runs on message thread, so it's safe to drain the queue
    TransportEvent ev;
    while (eventQ_.tryPop(ev))
    {
        if (ev.whenSamples >= targetSample)
        {
            // This event is still valid, but we just popped it
            // Try to push it back (best effort, may drop if queue is full)
            (void)eventQ_.tryPush(ev);
            break;
        }
        // else: event is in the past, discard it
    }
}

void Engine::drainScheduledEvents(int64_t blockStart, int64_t blockEnd, int numSamples)
{
    // ⚠️ AUDIO THREAD - REAL-TIME SAFE!
    // Uses peek-then-pop to avoid losing future events

    TransportEvent ev;
    while (eventQ_.tryPeek(ev))
    {
        // If event is beyond this block, leave it in queue
        if (ev.whenSamples >= blockEnd)
            break;

        // Event is for this block or past, consume it
        (void)eventQ_.tryPop(ev);

        // If event is in this block, process it
        if (ev.whenSamples >= blockStart)
        {
            const int offset = static_cast<int>(ev.whenSamples - blockStart);

            // Boundary check (should never trigger if logic is correct)
            jassert(offset >= 0 && offset < numSamples);

            // TODO (W10.3): Route to Mixer
            // mixer_.onScheduledEvent(ev, offset);

            // Temporary: Silent acknowledgment
            // In dev builds, we can verify events are being processed
            // In release builds, this compiles to nothing
            (void)offset;
        }
        // else: Event is overdue (< blockStart)
        // Policy: Drop silently (could also clamp to offset 0)
    }
}

#endif // ZENITH_ENABLE_PHASE1_AUDIO
