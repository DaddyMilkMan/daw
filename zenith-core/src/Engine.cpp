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
// Offline Export Implementation
//==============================================================================

void Engine::prepareBuffersForOfflineRender(int blockSize, int numChannels)
{
    DBG("Engine: Preparing buffers for offline render - blockSize=" + juce::String(blockSize) +
        ", numChannels=" + juce::String(numChannels) +
        ", numTracks=" + juce::String(tracks_.size()));

    // Resize trackBuffers_ to match the number of tracks
    trackBuffers_.resize(tracks_.size());

    // Allocate each track buffer with the specified block size and channel count
    for (size_t i = 0; i < trackBuffers_.size(); ++i)
    {
        trackBuffers_[i].setSize(numChannels, blockSize, false, true, false);
        trackBuffers_[i].clear();
    }

    DBG("Engine: Buffers prepared successfully");
}

void Engine::renderBlock(juce::AudioBuffer<float>& outputBuffer,
                        int numSamples,
                        juce::int64 playheadPosition)
{
    juce::ignoreUnused(playheadPosition);

    // Clear output buffer
    outputBuffer.clear();

    // Validate buffer sizes to prevent the bug described in the issue
    // Skip any track whose preallocated buffer is smaller than the requested block
    for (size_t trackIdx = 0; trackIdx < tracks_.size(); ++trackIdx)
    {
        // Check if we have a buffer for this track
        if (trackIdx >= trackBuffers_.size())
        {
            DBG("Engine::renderBlock - Warning: No buffer allocated for track " + juce::String(trackIdx));
            continue;
        }

        auto& trackBuffer = trackBuffers_[trackIdx];

        // THIS IS THE CRITICAL CHECK mentioned in lines 353-357 of the bug report
        // If the preallocated buffer is smaller than the requested block, skip this track
        if (trackBuffer.getNumSamples() < numSamples)
        {
            DBG("Engine::renderBlock - Warning: Track " + juce::String(trackIdx) +
                " buffer size (" + juce::String(trackBuffer.getNumSamples()) +
                ") is smaller than requested block size (" + juce::String(numSamples) +
                ") - SKIPPING TRACK");
            continue;
        }

        // Clear track buffer
        trackBuffer.clear();

        // TODO: When tracks have actual audio content, render it here
        // For now, tracks don't have clips or audio sources yet (Phase 0)
        // In future phases:
        // - Get track clips that overlap playheadPosition
        // - Render clip audio into trackBuffer
        // - Apply track volume, pan, mute, solo
        // - Apply track effects chain

        // Mix track buffer into output buffer
        for (int channel = 0; channel < juce::jmin(outputBuffer.getNumChannels(),
                                                    trackBuffer.getNumChannels()); ++channel)
        {
            outputBuffer.addFrom(channel, 0,
                               trackBuffer.getReadPointer(channel),
                               numSamples);
        }
    }

    // TODO: Apply master bus effects when implemented
}

bool Engine::exportProjectToWav(const juce::File& outputFile,
                                double sampleRate,
                                int bitDepth,
                                double durationInSeconds)
{
    DBG("Engine: Starting WAV export to " + outputFile.getFullPathName());
    DBG("  Sample Rate: " + juce::String(sampleRate) + " Hz");
    DBG("  Bit Depth: " + juce::String(bitDepth));
    DBG("  Duration: " + juce::String(durationInSeconds) + " seconds");

    // Validate parameters
    if (sampleRate <= 0.0)
    {
        DBG("Engine: Error - Invalid sample rate");
        return false;
    }

    if (bitDepth != 16 && bitDepth != 24 && bitDepth != 32)
    {
        DBG("Engine: Error - Invalid bit depth (must be 16, 24, or 32)");
        return false;
    }

    // Auto-detect duration if not specified
    // For Phase 0, use 10 seconds as default
    // TODO: In future phases, detect from project content (clips, automation, etc.)
    if (durationInSeconds <= 0.0)
    {
        durationInSeconds = 10.0;  // Default duration
        DBG("Engine: Auto-detected duration: " + juce::String(durationInSeconds) + " seconds");
    }

    // Calculate total samples
    const juce::int64 totalSamples = static_cast<juce::int64>(durationInSeconds * sampleRate);

    // Use 4096-sample blocks for efficient offline rendering
    // This is the block size mentioned in the bug report
    constexpr int offlineBlockSize = 4096;
    const int numChannels = 2;  // Stereo output

    DBG("Engine: Using offline block size of " + juce::String(offlineBlockSize) + " samples");

    // CRITICAL: Prepare buffers for offline rendering BEFORE calling renderBlock
    // This fixes the bug where trackBuffers_ would be sized for the audio device
    // buffer (typically 512/1024) and all tracks would be skipped when rendering
    // 4096-sample blocks
    prepareBuffersForOfflineRender(offlineBlockSize, numChannels);

    // Create WAV file writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;

    writer.reset(wavFormat.createWriterFor(
        new juce::FileOutputStream(outputFile),
        sampleRate,
        static_cast<unsigned int>(numChannels),
        bitDepth,
        {},  // metadata
        0    // quality option (not used for WAV)
    ));

    if (writer == nullptr)
    {
        DBG("Engine: Error - Failed to create WAV writer");
        return false;
    }

    // Create render buffer
    juce::AudioBuffer<float> renderBuffer(numChannels, offlineBlockSize);

    // Render loop
    juce::int64 samplesRendered = 0;

    while (samplesRendered < totalSamples)
    {
        // Calculate how many samples to render in this block
        const int samplesToRender = static_cast<int>(
            juce::jmin(static_cast<juce::int64>(offlineBlockSize),
                      totalSamples - samplesRendered));

        // Render this block
        // The renderBlock() method will skip any track whose buffer is too small
        // But since we called prepareBuffersForOfflineRender() with offlineBlockSize,
        // all track buffers are >= offlineBlockSize, so no tracks will be skipped
        renderBlock(renderBuffer, samplesToRender, samplesRendered);

        // Write to file
        if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToRender))
        {
            DBG("Engine: Error - Failed to write audio data");
            return false;
        }

        samplesRendered += samplesToRender;

        // Log progress every second
        if (samplesRendered % static_cast<juce::int64>(sampleRate) == 0)
        {
            double progress = static_cast<double>(samplesRendered) / totalSamples * 100.0;
            DBG("Engine: Export progress: " + juce::String(progress, 1) + "%");
        }
    }

    // Flush and close writer
    writer.reset();

    DBG("Engine: Export complete - " + juce::String(samplesRendered) + " samples written");
    return true;
}
