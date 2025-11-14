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
#include "../include/ProjectState.h"

//==============================================================================
Engine::Engine()
{
    DBG("Engine: Constructor");

    // Phase 2D: Initialize audio recording infrastructure
    audioFilePool_ = std::make_unique<zenith::AudioFilePool>();

    // Create background thread for audio file writing
    // Priority 5 = normal priority, suitable for disk I/O
    audioWriterThread_ = std::make_unique<juce::TimeSliceThread>("Audio Writer Thread");
    audioWriterThread_->startThread(5);

    DBG("Engine: Audio recording infrastructure initialized");
}

Engine::~Engine()
{
    DBG("Engine: Destructor");
    shutdown();

    // Phase 2D: Cleanup audio recording infrastructure
    // Stop writer thread
    if (audioWriterThread_ != nullptr)
    {
        audioWriterThread_->stopThread(1000);  // Wait up to 1 second
        audioWriterThread_.reset();
    }

    // Clear audio file pool
    if (audioFilePool_ != nullptr)
    {
        audioFilePool_.reset();
    }

    DBG("Engine: Audio recording infrastructure cleaned up");
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

    // Stop recording if active
    if (isRecording_.load())
    {
        stopRecording();
    }
}

//==============================================================================
// Phase 2D: Recording Controls
//==============================================================================

void Engine::record()
{
    DBG("Engine: Record");

    // Start playback if not already playing
    if (!isPlaying_.load())
    {
        play();
    }

    // Set recording flag
    isRecording_.store(true);

    // Get current sample rate
    const double sampleRate = currentSampleRate.load();

    // Store recording start position
    const juce::int64 recordStartSamples = playheadSamples_.load();

    // Create recordings directory
    // TODO: Use project path when available; for now use a temp directory
    juce::File recordingsDir = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory).getChildFile("ZenithDAW/Recordings");

    if (!recordingsDir.exists())
    {
        recordingsDir.createDirectory();
    }

    // Create recording sessions for all armed audio tracks
    audioRecordingSessions_.clear();

    for (size_t i = 0; i < tracks_.size(); ++i)
    {
        auto& track = tracks_[i];

        // Skip if not armed or not an audio track
        if (!track->isArmed() || track->getType() != zenith::Track::Type::Audio)
            continue;

        DBG("Engine: Creating recording session for track " + juce::String(i) +
            " (" + track->getName() + ")");

        // Create unique filename with timestamp
        juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        juce::String filename = track->getName().replaceCharacter(' ', '_') +
                               "_" + timestamp + ".wav";
        juce::File recordFile = recordingsDir.getChildFile(filename);

        // Determine number of channels for this track
        // TODO: Implement proper input routing matrix
        // For now: assume mono recording (1 channel per track)
        const int numChannels = 1;

        // Create WAV writer
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> fileStream(
            new juce::FileOutputStream(recordFile));

        if (!fileStream->openedOk())
        {
            DBG("Engine: Failed to create output stream for " + recordFile.getFullPathName());
            continue;
        }

        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(
                fileStream.release(),
                sampleRate,
                static_cast<unsigned int>(numChannels),
                24,  // 24-bit depth
                {},  // Default metadata
                0    // Default quality
            ));

        if (writer == nullptr)
        {
            DBG("Engine: Failed to create audio writer for " + recordFile.getFullPathName());
            continue;
        }

        // Wrap in ThreadedWriter for RT-safe writing
        auto threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            writer.release(),
            *audioWriterThread_,
            32768  // 32KB FIFO buffer
        );

        // Create session
        AudioRecordingSession session;
        session.writer = std::move(threadedWriter);
        session.file = recordFile;
        session.numChannels = numChannels;
        session.sampleRate = sampleRate;
        session.recordingStartSamples = recordStartSamples;
        session.trackIndex = static_cast<int>(i);

        audioRecordingSessions_.push_back(std::move(session));

        DBG("Engine: Recording to " + recordFile.getFullPathName());
    }

    if (audioRecordingSessions_.empty())
    {
        DBG("Engine: No armed audio tracks - recording cancelled");
        isRecording_.store(false);
    }
    else
    {
        DBG("Engine: Recording started with " +
            juce::String(audioRecordingSessions_.size()) + " sessions");
    }
}

void Engine::stopRecording()
{
    DBG("Engine: Stop recording");

    // Stop accepting new samples
    isRecording_.store(false);

    // Process on message thread (this should already be message thread)
    juce::MessageManager::callAsync([this]()
    {
        DBG("Engine: Flushing and closing " +
            juce::String(audioRecordingSessions_.size()) + " recording sessions");

        // Flush and close all writers, then create clips
        for (auto& session : audioRecordingSessions_)
        {
            // Flush and delete writer (triggers file close)
            session.writer.reset();

            DBG("Engine: Closed recording: " + session.file.getFullPathName());

            // Create audio clip from recording
            if (session.trackIndex >= 0 &&
                session.trackIndex < static_cast<int>(tracks_.size()))
            {
                auto& track = tracks_[session.trackIndex];
                bakeAudioRecordingIntoTrack(
                    *track,
                    session.file,
                    session.recordingStartSamples,
                    session.sampleRate);
            }
        }

        // Clear sessions
        audioRecordingSessions_.clear();

        DBG("Engine: All recording sessions processed");
    });
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
    bool recording = isRecording_.load();

    if (playing)
    {
        // Process audio
        processAudio(inputChannelData, numInputChannels,
                    outputChannelData, numOutputChannels, numSamples);

        // Update playback position
        playbackPosition.fetch_add(numSamples);

        // Update playhead (for recording alignment)
        playheadSamples_.fetch_add(numSamples);
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

    // Process recording (can record even when not playing, but typically we start playback)
    if (recording)
    {
        processAudioRecording(inputChannelData, numInputChannels, numSamples);
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

void Engine::processAudioRecording(
    const float* const* inputChannelData,
    int numInputChannels,
    int numSamples)
{
    // ⚠️ AUDIO THREAD - MUST BE REAL-TIME SAFE!
    //
    // This function writes audio input to ThreadedWriter instances,
    // which use a lock-free FIFO. This is RT-safe.
    //
    // NO allocations, NO locks, NO system calls here!

    if (inputChannelData == nullptr || numInputChannels == 0)
        return;

    // Write to each active recording session
    for (auto& session : audioRecordingSessions_)
    {
        if (session.writer == nullptr)
            continue;

        // TODO: Implement proper input routing matrix
        // For now: simple mapping - session track index maps to input channel
        // If we have more sessions than input channels, they'll share channels

        // Determine which input channel(s) to use for this session
        // Simplified: track index % numInputChannels
        const int inputChannel = session.trackIndex % numInputChannels;

        if (inputChannel >= numInputChannels || inputChannelData[inputChannel] == nullptr)
            continue;

        // For mono recording: write single channel
        if (session.numChannels == 1)
        {
            // Write samples to the threaded writer
            // This is RT-safe - just pushes to a FIFO
            const float* channelData[1] = { inputChannelData[inputChannel] };
            session.writer->write(channelData, numSamples);
        }
        // For stereo recording: write two channels
        else if (session.numChannels == 2 && numInputChannels >= 2)
        {
            const float* channelData[2] = {
                inputChannelData[0],
                inputChannelData[1]
            };
            session.writer->write(channelData, numSamples);
        }
    }
}

//==============================================================================
// Phase 2D: Recording Helpers (MESSAGE THREAD)
//==============================================================================

void Engine::bakeAudioRecordingIntoTrack(
    zenith::Track& track,
    const juce::File& file,
    juce::int64 recordingStartSamples,
    double sampleRate)
{
    DBG("Engine: Baking audio recording into track '" + track.getName() + "'");
    DBG("  File: " + file.getFullPathName());
    DBG("  Start: " + juce::String(recordingStartSamples) + " samples");

    if (!file.existsAsFile())
    {
        DBG("Engine: Recording file does not exist!");
        return;
    }

    // Load file into AudioFilePool
    auto fileHandle = audioFilePool_->loadFile(file);

    if (fileHandle == nullptr || !fileHandle->isValid())
    {
        DBG("Engine: Failed to load recording into AudioFilePool");
        return;
    }

    // Create a new audio clip
    auto clip = std::make_unique<zenith::Track::Clip>();
    clip->setType(zenith::Track::Clip::Type::Audio);
    clip->setName(file.getFileNameWithoutExtension());

    // Set timeline position
    clip->setStartPosition(recordingStartSamples);

    // Set clip length from file
    clip->setLength(fileHandle->lengthInSamples);

    // Load audio data into clip
    clip->setAudioFile(file);

    // Add clip to track
    track.addClip(std::move(clip));

    DBG("Engine: Audio clip created successfully");
    DBG("  Length: " + juce::String(fileHandle->lengthInSamples) + " samples (" +
        juce::String(fileHandle->lengthInSamples / sampleRate, 2) + " seconds)");
}
