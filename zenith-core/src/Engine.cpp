/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"

// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "engine/Track.h"
#include "engine/Clip.h"
#include "engine/MixerChannel.h"
#include "engine/AudioFilePool.h"

//==============================================================================
Engine::Engine()
    : audioFilePool_(std::make_unique<zenith::AudioFilePool>())
{
    DBG("Engine: Constructor");
    DBG("Engine: AudioFilePool created");
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

    // Clear audio file pool
    if (audioFilePool_)
    {
        audioFilePool_->clear();
    }

    DBG("Engine: Shutdown complete");
}

//==============================================================================
// Transport Controls
//==============================================================================

void Engine::play()
{
    DBG("Engine: Play");
    isPlaying_.store(true);

    // Phase 1.3: Use new playhead system
    // If playhead is at or past loop end, reset to loop start or 0
    const juce::int64 loopEnd = loopEndSamples_.load();
    const juce::int64 loopStart = loopStartSamples_.load();
    const juce::int64 currentPos = playheadSamples_.load();

    if (loopEnd > 0 && currentPos >= loopEnd)
    {
        playheadSamples_.store(loopStart);
    }

    // Keep legacy playbackPosition in sync for now
    playbackPosition.store(playheadSamples_.load());

    // Enable test tone for Phase 0 fallback (when no tracks)
    enableTestTone_.store(true);
}

void Engine::stop()
{
    DBG("Engine: Stop");
    isPlaying_.store(false);
    enableTestTone_.store(false);
}

//==============================================================================
// Phase 1.3: Transport Position & Looping
//==============================================================================

void Engine::setPlayheadSamples(juce::int64 position)
{
    playheadSamples_.store(juce::jmax(int64_t(0), position));
    playbackPosition.store(playheadSamples_.load());  // Keep legacy in sync
}

void Engine::setLooping(bool shouldLoop)
{
    isLooping_.store(shouldLoop);
    DBG("Engine: Looping " + juce::String(shouldLoop ? "enabled" : "disabled"));
}

void Engine::setLoopRegion(juce::int64 start, juce::int64 end)
{
    loopStartSamples_.store(juce::jmax(int64_t(0), start));
    loopEndSamples_.store(juce::jmax(int64_t(0), end));

    DBG("Engine: Loop region set: " + juce::String(start) + " - " + juce::String(end) + " samples");
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

        // Phase 1: Tracks are now wired to audio processing!
        // They will be prepared when audioDeviceAboutToStart() is called

        tracks_.push_back(std::move(track));
    }

    DBG("Engine: Total tracks: " + juce::String(tracks_.size()));

    // Re-prepare tracks if audio device is already running
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device != nullptr)
    {
        prepareTracks(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());
    }
}

//==============================================================================
// Phase 1.2: Audio File Pool
//==============================================================================

zenith::AudioFilePool& Engine::getAudioFilePool()
{
    jassert(audioFilePool_ != nullptr);
    return *audioFilePool_;
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
    playheadSamples_.store(0);  // Phase 1.3: Reset playhead

    // Phase 1: Prepare tracks for audio processing
    prepareTracks(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());

    DBG("Engine: Audio device started");
    DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
    DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
    DBG("  Tracks Prepared: " + juce::String(tracks_.size()));
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

        // Phase 1.3: Advance playhead with looping support
        juce::int64 newPosition = playheadSamples_.load() + numSamples;
        const bool looping = isLooping_.load();
        const juce::int64 loopEnd = loopEndSamples_.load();
        const juce::int64 loopStart = loopStartSamples_.load();

        if (looping && loopEnd > 0 && newPosition >= loopEnd)
        {
            // Handle loop wrap
            const juce::int64 loopLength = loopEnd - loopStart;
            if (loopLength > 0)
            {
                // Wrap position within loop
                while (newPosition >= loopEnd)
                {
                    newPosition -= loopLength;
                }
                // Ensure we're not before loop start
                if (newPosition < loopStart)
                {
                    newPosition = loopStart;
                }
            }
        }

        playheadSamples_.store(newPosition);

        // Keep legacy position in sync for backward compat
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

    // Phase 1: Mix all tracks into output
    // Clear output buffer first
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] != nullptr)
        {
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
        }
    }

    // Check if we have tracks to process
    if (tracks_.empty())
    {
        // Phase 0 fallback: test tone if enabled
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
        return;
    }

    // Phase 1: Process each track and mix into output
    // Using pre-allocated mixBuffer_ to avoid RT allocations
    for (const auto& track : tracks_)
    {
        if (track == nullptr)
            continue;

        // Clear mix buffer for this track
        mixBuffer_.clear();

        // Prepare channel info for track processing
        juce::AudioSourceChannelInfo channelInfo(&mixBuffer_, 0, numSamples);

        // Get audio from track (processes all clips, plugins, mixer)
        track->getNextAudioBlock(channelInfo);

        // Mix track output into main output buffer
        const int channelsToMix = juce::jmin(numOutputChannels, mixBuffer_.getNumChannels());

        for (int channel = 0; channel < channelsToMix; ++channel)
        {
            if (outputChannelData[channel] != nullptr)
            {
                juce::FloatVectorOperations::add(
                    outputChannelData[channel],
                    mixBuffer_.getReadPointer(channel),
                    numSamples);
            }
        }
    }
}

//==============================================================================
// Track Management (MESSAGE THREAD)
//==============================================================================

void Engine::prepareTracks(int samplesPerBlockExpected, double sampleRate)
{
    // Allocate mix buffer for track processing (message thread, NOT RT critical)
    // Size: stereo (2 channels) x block size
    const int maxChannels = 2;  // Stereo for now
    mixBuffer_.setSize(maxChannels, samplesPerBlockExpected, false, true, false);

    DBG("Engine: Preparing " + juce::String(tracks_.size()) + " tracks");

    // Prepare each track
    for (auto& track : tracks_)
    {
        if (track != nullptr)
        {
            track->prepareToPlay(samplesPerBlockExpected, sampleRate);
            DBG("  Prepared: " + track->getName());
        }
    }
}
