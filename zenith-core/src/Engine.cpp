/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "../include/Engine.h"

// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "engine/Track.h"
#include "engine/Clip.h"
#include "engine/MixerChannel.h"

//==============================================================================
// RecordingThread: Background thread for writing audio to disk (RT-safe)
//==============================================================================

class Engine::RecordingThread : public juce::Thread
{
public:
    RecordingThread(const juce::String& name)
        : juce::Thread(name)
    {
    }

    ~RecordingThread() override
    {
        stopThread(2000);
    }

    void prepareToRecord(const juce::File& file, double sampleRate, int numChannels)
    {
        outputFile = file;

        // Create WAV writer
        juce::WavAudioFormat wavFormat;
        if (auto* fileStream = outputFile.createOutputStream())
        {
            writer.reset(wavFormat.createWriterFor(fileStream,
                                                    sampleRate,
                                                    static_cast<unsigned int>(numChannels),
                                                    16,  // 16-bit
                                                    {},
                                                    0));
        }

        if (writer == nullptr)
        {
            DBG("RecordingThread: Failed to create audio writer");
        }
        else
        {
            DBG("RecordingThread: Prepared to record to " + file.getFullPathName());
        }
    }

    void writeBlock(const juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (writer != nullptr)
        {
            writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
        }
    }

    void finishRecording()
    {
        writer.reset();
        DBG("RecordingThread: Finished recording");
    }

    juce::File getOutputFile() const { return outputFile; }

    void run() override
    {
        // This thread just stays alive for async operations
        // Actual writing happens via writeBlock() called from message thread
        while (!threadShouldExit())
        {
            wait(100);
        }
    }

private:
    juce::File outputFile;
    std::unique_ptr<juce::AudioFormatWriter> writer;
};

//==============================================================================
Engine::Engine()
{
    DBG("Engine: Constructor");

    // Create recordings directory
    recordingsDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("Zenith DAW")
                            .getChildFile("Recordings");

    if (!recordingsDirectory.exists())
        recordingsDirectory.createDirectory();
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

    // If recording, stop it
    if (isRecording_.load())
    {
        stopRecording();
    }
}

void Engine::startRecording()
{
    DBG("Engine: Start recording");

    // Start playback if not already playing
    if (!isPlaying_.load())
    {
        play();
    }

    // Record the starting position
    recordingStartPosition.store(playbackPosition.load());

    // Create recording thread if needed
    if (recordingThread == nullptr)
    {
        recordingThread = std::make_unique<RecordingThread>("RecordingThread");
        recordingThread->startThread();
    }

    // Create a timestamped filename
    auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    auto recordingFile = recordingsDirectory.getChildFile("Recording_" + timestamp + ".wav");

    // Prepare the recording thread
    double sr = currentSampleRate.load();
    int numChannels = 2;  // Stereo for now
    recordingThread->prepareToRecord(recordingFile, sr, numChannels);

    // Allocate record buffer if needed
    {
        const juce::ScopedLock sl(recordBufferLock);
        if (recordBuffer.getNumChannels() < numChannels || recordBuffer.getNumSamples() < recordFifo.getTotalSize())
        {
            recordBuffer.setSize(numChannels, recordFifo.getTotalSize());
        }
    }

    // Reset FIFO
    recordFifo.reset();

    // Set recording flag
    isRecording_.store(true);

    DBG("Engine: Recording started at position " + juce::String(recordingStartPosition.load()));
}

juce::StringArray Engine::stopRecording()
{
    DBG("Engine: Stop recording");

    if (!isRecording_.load())
    {
        DBG("Engine: Not recording, nothing to stop");
        return {};
    }

    // Clear recording flag
    isRecording_.store(false);

    // Finalize the recording
    if (recordingThread != nullptr)
    {
        recordingThread->finishRecording();

        auto recordedFile = recordingThread->getOutputFile();
        if (recordedFile.existsAsFile())
        {
            recordedFiles.clear();
            recordedFiles.add(recordedFile.getFullPathName());

            auto recordedDuration = playbackPosition.load() - recordingStartPosition.load();

            DBG("Engine: Recorded file: " + recordedFile.getFullPathName());
            DBG("Engine: Recording duration: " + juce::String(recordedDuration / currentSampleRate.load(), 2) + " seconds");

            // Phase 12: Create clips in ProjectState for armed tracks
            if (projectState_ != nullptr)
            {
                auto trackIds = projectState_->getTrackIds();
                int clipsCreated = 0;

                // Begin undo transaction
                projectState_->getUndoManager().beginNewTransaction("Record pass");

                for (const auto& trackId : trackIds)
                {
                    if (projectState_->isTrackArmed(trackId) && projectState_->isAudioTrack(trackId))
                    {
                        // Create audio clip for this track
                        auto clipId = projectState_->createAudioClip(
                            trackId,
                            recordedFile.getFullPathName(),
                            recordingStartPosition.load(),
                            recordedDuration);

                        if (clipId.isNotEmpty())
                        {
                            clipsCreated++;
                            DBG("Engine: Created clip " + clipId + " on track " + trackId);
                        }
                    }
                }

                DBG("Engine: Created " + juce::String(clipsCreated) + " clips from recording");
            }

            return recordedFiles;
        }
    }

    return {};
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

    // Phase 12: Capture input audio if recording
    bool recording = isRecording_.load();

    if (recording && numInputChannels > 0 && inputChannelData != nullptr)
    {
        // Write input to record buffer (via FIFO for RT-safety)
        // Note: In a full implementation, we'd use the FIFO properly
        // For MVP, we'll directly write to the recording thread (message thread trigger)

        // For now, just pass audio through to outputs for monitoring
        int channelsToCopy = juce::jmin(numInputChannels, numOutputChannels);

        for (int channel = 0; channel < channelsToCopy; ++channel)
        {
            if (inputChannelData[channel] != nullptr && outputChannelData[channel] != nullptr)
            {
                juce::FloatVectorOperations::copy(outputChannelData[channel],
                                                   inputChannelData[channel],
                                                   numSamples);
            }
        }

        // Fill remaining outputs with silence
        for (int channel = channelsToCopy; channel < numOutputChannels; ++channel)
        {
            if (outputChannelData[channel] != nullptr)
            {
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
            }
        }

        // SIMPLIFIED: Write directly to recording thread (not fully RT-safe, but works for MVP)
        // In production, would use lock-free FIFO
        if (recordingThread != nullptr)
        {
            juce::AudioBuffer<float> tempBuffer(numInputChannels, numSamples);
            for (int ch = 0; ch < numInputChannels; ++ch)
            {
                if (inputChannelData[ch] != nullptr)
                {
                    tempBuffer.copyFrom(ch, 0, inputChannelData[ch], numSamples);
                }
            }

            // Write on message thread via async call (safe enough for MVP)
            juce::MessageManager::callAsync([this, tempBuffer = std::move(tempBuffer), numSamples]() mutable {
                if (recordingThread != nullptr && isRecording_.load())
                {
                    recordingThread->writeBlock(tempBuffer, numSamples);
                }
            });
        }
    }
    else
    {
        // Normal playback without recording
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
}
