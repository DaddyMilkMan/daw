/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"

#ifdef _WIN32
    #include "../Source/win/WinRtAudioPriority.h"
#endif

#if ZENITH_ENABLE_PHASE1_AUDIO
    #include <vector>
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
    // Phase 1: Sample-accurate event scheduling + mixer rendering
    // SPSC Safety: This runs ONLY when isPlaying_ == true (enforced by caller)
    // seekSamples() can only run when isPlaying_ == false (enforced by jassert)
    // Therefore, audio thread (consumer) and message thread (producer) never
    // conflict on eventQ_ - SPSC contract is maintained

    const int64_t blockStart = transportSamples_.load(std::memory_order_relaxed);
    const int64_t blockEnd = blockStart + numSamples;

    // Drain events into dueEvents_ array
    drainScheduledEvents(blockStart, blockEnd, numSamples);

    // Segment loop: Process audio between events
    int segmentStart = 0;

    for (int i = 0; i < numDueEvents_; ++i)
    {
        const auto& due = dueEvents_[i];
        const int segEnd = due.offsetInBlock;
        const int segLen = segEnd - segmentStart;

        // Process audio segment up to this event
        if (segLen > 0)
        {
            mixer_.processSegment(mixBuffer_, segmentStart, segLen, blockStart + segmentStart);
            segmentStart = segEnd;
        }

        // Handle transport event at sample-accurate offset
        mixer_.handleTransportEventRT(due.ev, due.offsetInBlock, blockStart + due.offsetInBlock);
    }

    // Process remaining segment after last event
    if (segmentStart < numSamples)
    {
        mixer_.processSegment(mixBuffer_, segmentStart, numSamples - segmentStart, blockStart + segmentStart);
    }

    // Copy mix buffer to output
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] != nullptr)
        {
            const int srcCh = juce::jmin(channel, mixBuffer_.getNumChannels() - 1);
            if (srcCh >= 0)
            {
                const float* src = mixBuffer_.getReadPointer(srcCh, 0);
                juce::FloatVectorOperations::copy(outputChannelData[channel], src, numSamples);
            }
        }
    }

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
    // CRITICAL: You MUST be stopped (not playing) to safely rewrite the queue
    // SPSC contract: audio thread is consumer, message thread is producer
    // If audio callback is running, it's consuming from eventQ_ concurrently
    // This would violate SPSC and cause undefined behavior
    jassert(!isPlaying_.load(std::memory_order_acquire));

    // Update transport position
    transportSamples_.store(targetSample, std::memory_order_release);

    // Purge all queued events < targetSample (they're now stale)
    // This runs on MESSAGE THREAD - safe to allocate and drain entire queue
    // ONLY SAFE BECAUSE AUDIO THREAD IS NOT CONSUMING (playback stopped)

    // Drain entire queue to temp buffer, preserving chronological order
    std::vector<TransportEvent> futureEvents;
    futureEvents.reserve(kEventRingCap);  // Avoid reallocations

    TransportEvent ev;
    while (eventQ_.tryPop(ev))
    {
        if (ev.whenSamples >= targetSample)
        {
            futureEvents.push_back(ev);  // Keep future events
        }
        // else: event is in the past (< targetSample), discard it
    }

    // Re-push all future events in original chronological order
    for (const auto& e : futureEvents)
    {
        if (!eventQ_.tryPush(e))
        {
            // Queue full (shouldn't happen with correct sizing, but be defensive)
            droppedEvents_.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void Engine::drainScheduledEvents(int64_t blockStart, int64_t blockEnd, int numSamples)
{
    // ⚠️ AUDIO THREAD - REAL-TIME SAFE!
    // Uses peek-then-pop to avoid losing future events
    // Fills dueEvents_ array for segment loop processing

    numDueEvents_ = 0;

    TransportEvent ev;
    while (eventQ_.tryPeek(ev))
    {
        // If event is beyond this block, leave it in queue
        if (ev.whenSamples >= blockEnd)
            break;

        // Event is for this block or past, consume it
        (void)eventQ_.tryPop(ev);

        // If event is in this block, add to due events
        if (ev.whenSamples >= blockStart)
        {
            const int offset = static_cast<int>(ev.whenSamples - blockStart);

            // Boundary check (should never trigger if logic is correct)
            jassert(offset >= 0 && offset < numSamples);

            // Add to due events array if space available
            if (numDueEvents_ < kMaxEventsPerBlock)
            {
                dueEvents_[numDueEvents_].ev = ev;
                dueEvents_[numDueEvents_].offsetInBlock = offset;
                numDueEvents_++;
            }
            // else: Dropped (too many events in one block)
            // TODO: Increment droppedEvents_ counter
        }
        // else: Event is overdue (< blockStart)
        // Policy: Drop silently (could also clamp to offset 0)
    }
}

#endif // ZENITH_ENABLE_PHASE1_AUDIO
