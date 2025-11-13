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
// Transport Controls (Phase 0 - gated)
//==============================================================================

#if !defined(ZENITH_ENABLE_PHASE1_AUDIO) || !ZENITH_ENABLE_PHASE1_AUDIO
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
#endif

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
// W10.2: Transport & scheduler (gated)
//==============================================================================

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
void Engine::play()  noexcept { isPlaying_.store(true,  std::memory_order_release); }
void Engine::pause() noexcept { isPlaying_.store(false, std::memory_order_release); }
void Engine::stop() noexcept {
    pause();
    seekSamples(0);
}
void Engine::seekSamples(int64_t absolute) noexcept {
    // SPSC discipline: only seek when not playing (queue owned by message thread when paused)
    jassert(!isPlaying_.load(std::memory_order_acquire));
    transportSamples_.store(absolute, std::memory_order_release);
    // TODO: If seeking while events queued, may want to flush/rewrite eventQ_
}

bool Engine::scheduleClipStart(int trackIndex, int clipId, int64_t atSample) noexcept {
    TransportEvent ev{ TransportEvent::Type::StartClip, trackIndex, clipId, atSample };
    return eventQ_.push(ev);
}
bool Engine::scheduleClipStop(int trackIndex, int clipId, int64_t atSample) noexcept {
    TransportEvent ev{ TransportEvent::Type::StopClip, trackIndex, clipId, atSample };
    return eventQ_.push(ev);
}

//==============================================================================
// Track Management (MESSAGE THREAD ONLY)
//==============================================================================

void Engine::setNumTracks(int numTracks)
{
    const int current = mixer_.getNumTracks();

    if (numTracks == current)
        return;

    if (numTracks > current)
    {
        // Add tracks
        for (int i = current; i < numTracks; ++i)
        {
            mixer_.addTrack("Track " + juce::String(i + 1));
        }
    }
    else
    {
        // Remove tracks from the end
        for (int i = current - 1; i >= numTracks; --i)
        {
            mixer_.removeTrack(i);
        }
    }
}

int Engine::getNumTracks() const
{
    return mixer_.getNumTracks();
}

AudioTrack* Engine::getTrack(int index)
{
    return mixer_.getTrack(index);
}

const AudioTrack* Engine::getTrack(int index) const
{
    return const_cast<Mixer&>(mixer_).getTrack(index);
}

//==============================================================================
// W10.3: Drain scheduled events for segment loop
//==============================================================================

Engine::DueEvent* Engine::drainScheduledEvents(int64_t blockStart, int64_t blockEnd, int& outCount) noexcept {
    outCount = 0;
    TransportEvent ev;

    // Drain all events due in [blockStart, blockEnd)
    // Use peek-then-pop pattern to avoid consuming future events
    while (outCount < kMaxEventsPerBlock && eventQ_.tryPeek(ev)) {
        // If event is in the future, stop draining (leave it in queue)
        if (ev.whenSamples >= blockEnd) {
            break;
        }

        // Pop the event (we're consuming it)
        if (!eventQ_.tryPop(ev)) {
            // Shouldn't happen (peek succeeded but pop failed), but handle gracefully
            break;
        }

        // If event is late (before blockStart), drop it
        // Future work: could dispatch as immediate (offset=0)
        if (ev.whenSamples < blockStart) {
            continue;
        }

        // Event is due in this block - calculate offset and add to array
        const int offset = (int) juce::jlimit<int64_t>(0, blockEnd - blockStart - 1, ev.whenSamples - blockStart);
        dueEvents_[outCount].ev = ev;
        dueEvents_[outCount].offsetInBlock = offset;
        ++outCount;
    }

    // Note: Events are already in chronological order from the queue
    // No need to sort if queue maintains order (SPSC ring preserves order)
    return dueEvents_;
}

#if JUCE_DEBUG
//==============================================================================
// Debug Metrics (MESSAGE THREAD - RT-safe reading)
//==============================================================================

Engine::Phase1DebugMetrics Engine::getPhase1DebugMetrics() const noexcept
{
    Phase1DebugMetrics metrics;

    // Read transport state (atomics, no locks)
    metrics.transportSamples = transportSamples_.load(std::memory_order_relaxed);
    metrics.isPlaying = isPlaying_.load(std::memory_order_relaxed);
    metrics.sampleRate = currentSampleRate.load(std::memory_order_relaxed);

    // Read mixer state (requires lock, but we're on message thread)
    metrics.numTracks = mixer_.getNumTracks();

    // Active voices: sum across all tracks
    // Note: This requires iterating tracks, which is message-thread safe
    metrics.activeVoices = 0;
    for (int i = 0; i < metrics.numTracks; ++i)
    {
        auto* track = mixer_.getTrack(i);
        if (track != nullptr)
        {
            // Count active voices in this track
            // We'll need a method on AudioTrack to expose this
            // For now, leave at 0 (TODO: add getActiveVoiceCount() to AudioTrack)
        }
    }

    // Scheduler stats
    metrics.queuedEvents = (int) eventQ_.size();
    metrics.droppedEvents = 0;  // TODO: Add if tracking dropped events

    // Peak levels (atomics, RT writes / message reads)
    metrics.peakL = debugPeakL_.load(std::memory_order_relaxed);
    metrics.peakR = debugPeakR_.load(std::memory_order_relaxed);

    return metrics;
}
#endif // JUCE_DEBUG
#endif // ZENITH_ENABLE_PHASE1_AUDIO

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
#if !defined(ZENITH_ENABLE_PHASE1_AUDIO) || !ZENITH_ENABLE_PHASE1_AUDIO
    playbackPosition.store(0);
#endif

    // W10: Prepare mixer and buffers
    prepareToPlay(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());

    DBG("Engine: Audio device started");
    DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
    DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
    DBG("  MMCSS will be registered on first audio callback");
}

void Engine::audioDeviceStopped()
{
    DBG("Engine: Audio device stopped");
    releaseResources();
}

//==============================================================================
// W10: Prepare/Release (allocate mix buffers when flag ON)
//==============================================================================

void Engine::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected, sampleRate);

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
    // Allocate mix buffer for Phase 1 audio engine
    auto* dev = deviceManager.getCurrentAudioDevice();
    const int outs = dev ? std::max(1, dev->getActiveOutputChannels().countNumberOfSetBits()) : 2;
    mixBuffer_.setSize(outs, std::max(1, samplesPerBlockExpected), false, true, true);

    // Prepare mixer
    mixer_.prepare(sampleRate, samplesPerBlockExpected);

    // W10.2: Reset transport + queues on (re)prepare
    transportSamples_.store(0, std::memory_order_relaxed);
    isPlaying_.store(false, std::memory_order_relaxed);
    dueEventsScratch_.clear();

    DBG("Engine: Phase 1 mixer prepared (" + juce::String(outs) + " ch, "
        + juce::String(samplesPerBlockExpected) + " samples)");
#endif
}

void Engine::releaseResources()
{
#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
    mixBuffer_.setSize(0, 0);
    mixer_.release();
    dueEventsScratch_.clear();
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

#if !defined(ZENITH_ENABLE_PHASE1_AUDIO) || !ZENITH_ENABLE_PHASE1_AUDIO
    // Phase 0: Check if playing
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
#else
    // Phase 1: Always process (transport managed in processAudio)
    processAudio(inputChannelData, numInputChannels,
                outputChannelData, numOutputChannels, numSamples);
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

    // Clear outputs first
    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
    //==========================================================================
    // W10: Phase 1 mixer path (flag ON)
    //==========================================================================

    // W10.2/W10.3: Transport window for this block
    int64_t blockStart = transportSamples_.load(std::memory_order_relaxed);
    int64_t blockEnd   = blockStart + numSamples;

    // Ensure mix buffer matches device config
    if (mixBuffer_.getNumChannels() != numOutputChannels || mixBuffer_.getNumSamples() < numSamples)
        mixBuffer_.setSize(std::max(1, numOutputChannels), std::max(1, numSamples), false, true, true);

    // Clear mix buffer
    mixBuffer_.clear();

    // W10.3: Drain scheduled events due in [blockStart, blockEnd)
    int eventCount = 0;
    auto* events = drainScheduledEvents(blockStart, blockEnd, eventCount);

    // W10.3: Segment loop - render audio between events for sample-accurate timing
    int cursor = 0;
    for (int i = 0; i < eventCount; ++i) {
        const int eventOffset = events[i].offsetInBlock;

        // Render segment before this event
        if (eventOffset > cursor) {
            const int segmentLen = eventOffset - cursor;
            mixer_.processSegment(mixBuffer_, blockStart + cursor, cursor, segmentLen);
            cursor = eventOffset;
        }

        // Handle event at exact sample offset
        mixer_.handleTransportEventRT(events[i].ev, eventOffset);
    }

    // Render final segment after last event (or entire block if no events)
    if (cursor < numSamples) {
        const int segmentLen = numSamples - cursor;
        mixer_.processSegment(mixBuffer_, blockStart + cursor, cursor, segmentLen);
    }

    // Copy mix buffer → device outputs
    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
        {
            const int srcCh = std::min(ch, mixBuffer_.getNumChannels() - 1);
            juce::FloatVectorOperations::copy(
                outputChannelData[ch],
                mixBuffer_.getReadPointer(srcCh),
                numSamples
            );
        }
    }

#if JUCE_DEBUG
    // Update peak meters for debug HUD (RT writes, message thread reads)
    if (numOutputChannels >= 1 && outputChannelData[0] != nullptr)
    {
        float peakL = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peakL = std::max(peakL, std::abs(outputChannelData[0][i]));
        debugPeakL_.store(peakL, std::memory_order_relaxed);
    }
    if (numOutputChannels >= 2 && outputChannelData[1] != nullptr)
    {
        float peakR = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peakR = std::max(peakR, std::abs(outputChannelData[1][i]));
        debugPeakR_.store(peakR, std::memory_order_relaxed);
    }
#endif

    // Advance transport only while playing
    if (isPlaying_.load(std::memory_order_relaxed))
        transportSamples_.store(blockEnd, std::memory_order_relaxed);

#else
    //==========================================================================
    // Phase 0: Test tone fallback (flag OFF)
    //==========================================================================

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
#endif
}
