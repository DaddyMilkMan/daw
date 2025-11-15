/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"
#include "../include/ProjectState.h"
#include "../include/TrackAutomationSynchronizer.h"
#include "../include/TempoMapSynchronizer.h"

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

    // Stop tempo map synchronizer if running
    if (tempoMapSynchronizer)
    {
        tempoMapSynchronizer.reset();
    }

    projectState_ = state;

    // Create new automation synchronizer if we have a project state
    if (projectState_ != nullptr)
    {
        automationSynchronizer = std::make_unique<TrackAutomationSynchronizer>(*projectState_, *this);
        DBG("Engine: Created automation synchronizer");

        // Create tempo map synchronizer
        tempoMapSynchronizer = std::make_unique<TempoMapSynchronizer>(*projectState_, *this);
        tempoMapSynchronizer->initialize();
        DBG("Engine: Created tempo map synchronizer");
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
// Phase 15: Tempo Map Runtime
//==============================================================================

void Engine::setTempoMap(const juce::Array<ProjectState::TempoChangeSpec>& tempoChanges)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (tempoChanges.isEmpty())
    {
        DBG("Engine: Empty tempo map, using default 120 BPM");

        std::lock_guard<std::mutex> lock(tempoMapMutex_);
        tempoSegments_.clear();

        TempoSegment defaultSegment;
        defaultSegment.startBeat = 0.0;
        defaultSegment.bpm = 120.0;
        defaultSegment.secondsAtStartBeat = 0.0;
        defaultSegment.timeSigNumerator = 4;
        defaultSegment.timeSigDenominator = 4;
        tempoSegments_.push_back(defaultSegment);
        return;
    }

    // Precompute segments for RT-safe access
    std::vector<TempoSegment> newSegments;
    newSegments.reserve(tempoChanges.size());

    double accumulatedSeconds = 0.0;

    for (int i = 0; i < tempoChanges.size(); ++i)
    {
        const auto& change = tempoChanges[i];

        // If this isn't the first segment, calculate time elapsed since last change
        if (i > 0)
        {
            const auto& prevChange = tempoChanges[i - 1];
            double beatDelta = change.beatPosition - prevChange.beatPosition;
            double secondsDelta = (beatDelta / prevChange.bpm) * 60.0;
            accumulatedSeconds += secondsDelta;
        }

        TempoSegment segment;
        segment.startBeat = change.beatPosition;
        segment.bpm = change.bpm;
        segment.secondsAtStartBeat = accumulatedSeconds;
        segment.timeSigNumerator = change.timeSigNumerator;
        segment.timeSigDenominator = change.timeSigDenominator;
        newSegments.push_back(segment);
    }

    // Update the tempo segments (thread-safe swap)
    {
        std::lock_guard<std::mutex> lock(tempoMapMutex_);
        tempoSegments_ = std::move(newSegments);
    }

    DBG("Engine: Updated tempo map with " + juce::String(tempoSegments_.size()) + " segments");
}

double Engine::getTempoAtSample(juce::int64 samplePos) const noexcept
{
    std::lock_guard<std::mutex> lock(tempoMapMutex_);

    if (tempoSegments_.empty())
        return 120.0;

    double seconds = static_cast<double>(samplePos) / currentSampleRate.load();

    // Find the segment that applies at this time
    double currentBpm = tempoSegments_[0].bpm;
    for (const auto& segment : tempoSegments_)
    {
        if (seconds >= segment.secondsAtStartBeat)
            currentBpm = segment.bpm;
        else
            break;
    }

    return currentBpm;
}

double Engine::sampleToBeat(juce::int64 samplePos) const noexcept
{
    std::lock_guard<std::mutex> lock(tempoMapMutex_);

    if (tempoSegments_.empty())
    {
        // Fallback: 120 BPM
        double seconds = static_cast<double>(samplePos) / currentSampleRate.load();
        return (seconds / 60.0) * 120.0;
    }

    double seconds = static_cast<double>(samplePos) / currentSampleRate.load();
    double beat = 0.0;

    for (size_t i = 0; i < tempoSegments_.size(); ++i)
    {
        const auto& segment = tempoSegments_[i];

        // Calculate seconds at the next segment (or end of time if last segment)
        double nextSeconds = (i + 1 < tempoSegments_.size())
                           ? tempoSegments_[i + 1].secondsAtStartBeat
                           : seconds + 1000.0;  // Large number

        if (seconds <= nextSeconds || i + 1 >= tempoSegments_.size())
        {
            // Target is within this segment
            double secondsInSegment = seconds - segment.secondsAtStartBeat;
            double beatsInSegment = (secondsInSegment / 60.0) * segment.bpm;
            beat = segment.startBeat + beatsInSegment;
            break;
        }
    }

    return beat;
}

juce::int64 Engine::beatToSample(double beat) const noexcept
{
    std::lock_guard<std::mutex> lock(tempoMapMutex_);

    if (tempoSegments_.empty())
    {
        // Fallback: 120 BPM
        double seconds = (beat / 120.0) * 60.0;
        return static_cast<juce::int64>(seconds * currentSampleRate.load());
    }

    double seconds = 0.0;

    for (size_t i = 0; i < tempoSegments_.size(); ++i)
    {
        const auto& segment = tempoSegments_[i];

        // Calculate beat at the next segment (or target beat if last segment)
        double nextBeat = (i + 1 < tempoSegments_.size())
                        ? tempoSegments_[i + 1].startBeat
                        : beat + 1000.0;  // Large number

        if (beat <= nextBeat || i + 1 >= tempoSegments_.size())
        {
            // Target is within this segment
            double beatDelta = beat - segment.startBeat;
            double secondsDelta = (beatDelta / segment.bpm) * 60.0;
            seconds = segment.secondsAtStartBeat + secondsDelta;
            break;
        }
    }

    return static_cast<juce::int64>(seconds * currentSampleRate.load());
}

void Engine::setPlayheadPosition(juce::int64 samplePos)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    playbackPosition.store(samplePos);
    DBG("Engine: Set playhead to sample " + juce::String(samplePos));
}
