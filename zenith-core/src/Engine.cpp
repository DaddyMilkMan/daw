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

    // Prepare track buffers for unified render path
    const int bufferSize = currentBufferSize.load();
    const int numTracks = static_cast<int>(tracks_.size());

    DBG("Engine: Preparing " + juce::String(numTracks) + " track buffers");

    trackBuffers_.clear();
    trackBuffers_.resize(numTracks);

    for (int i = 0; i < numTracks; ++i)
    {
        // Allocate stereo buffer for each track
        trackBuffers_[i].setSize(2, bufferSize);
        trackBuffers_[i].clear();

        // Prepare track for playback
        if (tracks_[i] != nullptr)
        {
            tracks_[i]->prepareToPlay(bufferSize, currentSampleRate.load());
        }
    }

    // Prepare master buffer
    masterBuffer_.setSize(2, bufferSize);
    masterBuffer_.clear();

    DBG("Engine: Audio device started");
    DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
    DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
    DBG("  Track Buffers: " + juce::String(trackBuffers_.size()));
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
// Unified Render Path
//==============================================================================

void Engine::renderBlock(juce::AudioBuffer<float>& outputBuffer,
                         juce::int64 transportPosition,
                         int numSamples)
{
    // ⚠️ CAN RUN ON AUDIO THREAD - MUST BE REAL-TIME SAFE!
    //
    // This is the single unified render path used by:
    // 1. Real-time audio callback (with live transport position)
    // 2. Offline export (with offline transport position)

    juce::ignoreUnused(transportPosition);  // TODO: Pass to clips for timeline-based rendering

    // Clear output buffer
    outputBuffer.clear();

    // Render each track and mix into master
    const int numTracks = static_cast<int>(tracks_.size());

    for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
    {
        auto* track = tracks_[trackIdx].get();

        if (track == nullptr)
            continue;

        // Skip disabled or muted tracks (Track checks this internally, but we can optimize)
        // Note: We still call getNextAudioBlock even if muted, to maintain plugin state

        // Ensure track buffer is sized correctly
        if (trackIdx >= static_cast<int>(trackBuffers_.size()))
            continue;  // Safety check; should not happen if audioDeviceAboutToStart() was called

        auto& trackBuffer = trackBuffers_[trackIdx];

        // Ensure buffer is large enough
        if (trackBuffer.getNumSamples() < numSamples)
            continue;  // Safety check

        // Clear track buffer
        trackBuffer.clear();

        // Get audio from track (clips + plugins + gain/pan)
        juce::AudioSourceChannelInfo info(&trackBuffer, 0, numSamples);
        track->getNextAudioBlock(info);

        // Mix track into master output
        // Support both mono and stereo tracks
        const int trackChannels = trackBuffer.getNumChannels();
        const int outputChannels = outputBuffer.getNumChannels();

        if (trackChannels == 1 && outputChannels >= 2)
        {
            // Mono track -> Stereo output (copy to both L/R)
            outputBuffer.addFrom(0, 0, trackBuffer, 0, 0, numSamples);  // L
            outputBuffer.addFrom(1, 0, trackBuffer, 0, 0, numSamples);  // R
        }
        else if (trackChannels >= 2 && outputChannels >= 2)
        {
            // Stereo track -> Stereo output
            outputBuffer.addFrom(0, 0, trackBuffer, 0, 0, numSamples);  // L
            outputBuffer.addFrom(1, 0, trackBuffer, 1, 0, numSamples);  // R
        }
        else if (trackChannels == 1 && outputChannels == 1)
        {
            // Mono track -> Mono output
            outputBuffer.addFrom(0, 0, trackBuffer, 0, 0, numSamples);
        }
        // else: Channel count mismatch, skip this track
    }

    // TODO: Apply master effects here (Phase N)
    // TODO: Apply master volume/limiter (Phase N)
}

bool Engine::exportProjectToWav(const juce::String& outputFilePath,
                                double durationSeconds,
                                double sampleRate)
{
    DBG("Engine: Exporting to WAV: " + outputFilePath);
    DBG("  Duration: " + juce::String(durationSeconds) + " seconds");

    // Use current engine sample rate if not specified
    if (sampleRate <= 0.0)
        sampleRate = currentSampleRate.load();

    DBG("  Sample Rate: " + juce::String(sampleRate) + " Hz");

    // Calculate total samples
    const juce::int64 totalSamples = static_cast<juce::int64>(durationSeconds * sampleRate);
    const int blockSize = 4096;  // Use larger block size for offline rendering

    // Create output file
    juce::File outputFile(outputFilePath);
    outputFile.deleteFile();  // Remove existing file

    // Create WAV writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;

    writer.reset(wavFormat.createWriterFor(
        new juce::FileOutputStream(outputFile),
        sampleRate,
        2,  // Stereo
        16, // 16-bit
        {},
        0));

    if (writer == nullptr)
    {
        DBG("Engine: Failed to create WAV writer");
        return false;
    }

    // Prepare tracks for offline rendering
    for (auto& track : tracks_)
    {
        if (track != nullptr)
        {
            track->prepareToPlay(blockSize, sampleRate);
        }
    }

    // Create offline render buffer
    juce::AudioBuffer<float> offlineBuffer(2, blockSize);

    // Render loop
    juce::int64 currentPosition = 0;

    while (currentPosition < totalSamples)
    {
        const int samplesThisBlock = juce::jmin(
            blockSize,
            static_cast<int>(totalSamples - currentPosition));

        // Clear buffer
        offlineBuffer.clear();

        // Render this block using the SAME renderBlock() function as realtime!
        renderBlock(offlineBuffer, currentPosition, samplesThisBlock);

        // Write to WAV file
        writer->writeFromAudioSampleBuffer(offlineBuffer, 0, samplesThisBlock);

        currentPosition += samplesThisBlock;
    }

    // Flush and close
    writer.reset();

    DBG("Engine: Export complete!");
    DBG("  Wrote " + juce::String(totalSamples) + " samples");

    return true;
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

    // Wrap output buffer for unified render path
    juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);

    // Get current transport position
    juce::int64 position = playbackPosition.load();

    // Use unified render path
    renderBlock(outputBuffer, position, numSamples);

    // Fallback: If no tracks or all tracks are silent, optionally enable test tone
    // (Only if explicitly enabled via enableTestTone_)
    bool testToneEnabled = enableTestTone_.load();

    if (testToneEnabled && tracks_.empty())
    {
        // Generate 440 Hz sine wave at -12 dB (only if no tracks exist)
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
                    outputChannelData[channel][sample] += value;  // Add instead of replace
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
