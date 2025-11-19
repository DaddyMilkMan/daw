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
#include "../Source/engine/AudioFilePool.h"
#include "../Source/engine/PluginHost.h"
#include "../Source/ui/PluginEditorWindow.h"

//==============================================================================
Engine::Engine()
{
    DBG("Engine: Constructor");

    // Phase 1.2: Initialize audio file pool
    audioFilePool_ = std::make_unique<zenith::AudioFilePool>();
    DBG("Engine: AudioFilePool created");

    // Phase 3: Initialize plugin host and editor window manager
    pluginHost_ = std::make_unique<zenith::PluginHost>();
    pluginEditorWindowManager_ = std::make_unique<zenith::PluginEditorWindowManager>();

    // Phase 2D: Initialize audio recording infrastructure
    // Create background thread for audio file writing
    // Priority 5 = normal priority, suitable for disk I/O
    audioWriterThread_ = std::make_unique<juce::TimeSliceThread>("Audio Writer Thread");
    audioWriterThread_->startThread(5);

    DBG("Engine: Audio recording infrastructure initialized");
}

Engine::~Engine()
{
    DBG("Engine: Destructor");

    // CODEX FIX P2: Set shutdown flag to prevent async callbacks
    isShuttingDown_.store(true);

    // Phase 2A: Disable MIDI input
    disableMidiInput();

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

        // Sync tracks with project state
        syncWithProjectState();
    }
}

void Engine::syncWithProjectState()
{
    DBG("Engine: Syncing with project state");

    if (projectState_ == nullptr)
    {
        DBG("Engine: No project state, clearing tracks");
        tracks_.clear();
        return;
    }

    // Clear existing tracks
    tracks_.clear();

    // Get tracks from project state
    auto& state = projectState_->getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
    {
        DBG("Engine: No tracks in project state");
        return;
    }

    const double sampleRate = currentSampleRate.load();
    const int bufferSize = currentBufferSize.load();
    const double tempo = projectState_->getTempo();

    // Helper: convert beats to samples
    auto beatsToSamples = [tempo, sampleRate](double beats) -> juce::int64
    {
        const double secondsPerBeat = 60.0 / tempo;
        const double seconds = beats * secondsPerBeat;
        return static_cast<juce::int64>(seconds * sampleRate);
    };

    // Create engine tracks from project state
    for (auto trackNode : tracksNode)
    {
        juce::String trackName = trackNode[ProjectState::PROP_NAME].toString();
        juce::String trackType = trackNode[ProjectState::PROP_TYPE].toString();

        // Create track
        auto track = std::make_unique<zenith::Track>(
            trackName,
            trackType == "midi" ? zenith::Track::Type::MIDI : zenith::Track::Type::Audio);

        // Set mixer properties
        track->setVolume(trackNode[ProjectState::PROP_VOLUME]);
        track->setPan(trackNode[ProjectState::PROP_PAN]);
        track->setMuted(trackNode[ProjectState::PROP_MUTE]);
        track->setSolo(trackNode[ProjectState::PROP_SOLO]);

        // Prepare track for playback
        if (sampleRate > 0)
        {
            track->prepareToPlay(bufferSize, sampleRate);
        }

        // Load clips
        auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
        if (clipsNode.isValid())
        {
            for (auto clipNode : clipsNode)
            {
                // Create clip
                auto clip = std::make_unique<zenith::Track::Clip>();

                // Set basic properties
                double startBeats = clipNode[ProjectState::PROP_START];
                double lengthBeats = clipNode[ProjectState::PROP_LENGTH];

                clip->setStartPosition(beatsToSamples(startBeats));
                clip->setLength(beatsToSamples(lengthBeats));

                // Load audio file if present
                juce::String audioFilePath = clipNode[ProjectState::PROP_AUDIO_FILE].toString();
                if (audioFilePath.isNotEmpty())
                {
                    juce::File audioFile(audioFilePath);
                    if (audioFile.existsAsFile())
                    {
                        clip->setAudioFile(audioFile);
                        clip->setType(zenith::Track::Clip::Type::Audio);
                        DBG("Engine: Loaded audio file: " + audioFile.getFileName());
                    }
                    else
                    {
                        DBG("Engine: Warning - audio file not found: " + audioFilePath);
                    }
                }

                // Prepare clip
                if (sampleRate > 0)
                {
                    clip->prepareToPlay(bufferSize, sampleRate);
                }

                // Set clip as playing (so it's active during playback)
                clip->setPlaying(true);

                // Add clip to track
                track->addClip(std::move(clip));
            }
        }

        // Add track to engine
        tracks_.push_back(std::move(track));
    }

    DBG("Engine: Synced " + juce::String(tracks_.size()) + " tracks");
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

    // Phase 2A: Enable MIDI input
    enableMidiInput();

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

    // Enable test tone for Phase 0 fallback (when no tracks)
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

    // Phase 2C: If recording, bake recordings into clips first
    if (isRecording_.load())
    {
        stopRecording();
    }

    isPlaying_.store(false);
    enableTestTone_.store(false);

    // Phase 13: Stop automation synchronizer
    if (automationSynchronizer)
    {
        automationSynchronizer->stop();
        DBG("Engine: Stopped automation synchronizer");
    }
}

double Engine::getPlaybackPositionBeats() const
{
    if (projectState_ == nullptr)
        return 0.0;

    const double tempo = projectState_->getTempo();
    const double sampleRate = currentSampleRate.load();
    const juce::int64 positionSamples = playheadSamples_.load();

    // Convert samples to beats
    const double seconds = static_cast<double>(positionSamples) / sampleRate;
    const double beats = (seconds * tempo) / 60.0;

    return beats;
}

//==============================================================================
// Phase 2C/2D: MIDI and Audio Recording
//==============================================================================

void Engine::record()
{
    DBG("Engine: Record");

    // Start playback if not already playing
    if (!isPlaying_.load())
    {
        play();
    }

    // Get current sample rate and start position
    const double sampleRate = currentSampleRate.load();
    const juce::int64 recordStartSamples = playheadSamples_.load();

    // ==========================================================================
    // Phase 2C: Setup MIDI recording
    // ==========================================================================
    {
        const juce::ScopedLock sl(midiRecordingLock_);
        midiRecording_.recordingStartSamples = recordStartSamples;

        // Resize recording buffers to match track count
        midiRecording_.trackRecordings.resize(tracks_.size());

        // Clear all track recordings
        for (auto& trackRecording : midiRecording_.trackRecordings)
        {
            trackRecording.clear();
        }
    }

    // ==========================================================================
    // Phase 2D: Setup Audio recording
    // ==========================================================================

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

    // ==========================================================================
    // CODEX FIX P1: Enable recording flag AFTER sessions are set up
    // This prevents the audio thread from accessing sessions before they're ready
    // ==========================================================================
    isRecording_.store(true);

    DBG("Engine: Recording started at sample " + juce::String(recordStartSamples));
    DBG("Engine: Audio sessions: " + juce::String(audioRecordingSessions_.size()));
}

void Engine::stopRecording()
{
    DBG("Engine: Stop recording");

    // Stop accepting new samples immediately
    isRecording_.store(false);

    // ==========================================================================
    // CODEX FIX P2: Guard async callback against use-after-free
    // Check if we're shutting down before posting async operations
    // ==========================================================================
    if (isShuttingDown_.load())
    {
        DBG("Engine: Shutdown in progress, skipping async recording cleanup");
        return;
    }

    // ==========================================================================
    // Phase 2C: Bake MIDI recordings into clips
    // ==========================================================================
    bakeMidiRecordingsIntoClips(true);  // With quantization
    clearMidiRecordings();

    // ==========================================================================
    // Phase 2D: Process audio recordings asynchronously
    // This MUST be async because we need to flush writers on the message thread
    // ==========================================================================
    juce::MessageManager::callAsync([this]()
    {
        // Double-check we're not shutting down
        if (isShuttingDown_.load())
        {
            DBG("Engine: Shutdown detected in async callback, aborting");
            return;
        }

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

    DBG("Engine: Recording stopped, processing clips");
}

//==============================================================================
// Phase 1.3: Transport Position & Looping
//==============================================================================

void Engine::setPlayheadSamples(juce::int64 position)
{
    playheadSamples_.store(juce::jmax(juce::int64(0), position));
}

void Engine::setLooping(bool shouldLoop)
{
    isLooping_.store(shouldLoop);
    DBG("Engine: Looping " + juce::String(shouldLoop ? "enabled" : "disabled"));
}

void Engine::setLoopRegion(juce::int64 start, juce::int64 end)
{
    loopStartSamples_.store(juce::jmax(juce::int64(0), start));
    loopEndSamples_.store(juce::jmax(juce::int64(0), end));

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
// Plugin Hosting (Phase 3: VST3 hosting MVP)
//==============================================================================

zenith::PluginHost& Engine::getPluginHost() noexcept
{
    jassert(pluginHost_ != nullptr);
    return *pluginHost_;
}

int Engine::scanForPlugins()
{
    if (pluginHost_ == nullptr)
    {
        DBG("Engine: PluginHost not initialized");
        return 0;
    }

    DBG("Engine: Scanning for plugins...");
    int count = pluginHost_->scanDefaultLocations();
    DBG("Engine: Plugin scan complete - found " + juce::String(count) + " plugins");

    return count;
}

zenith::PluginEditorWindowManager& Engine::getPluginEditorWindowManager() noexcept
{
    jassert(pluginEditorWindowManager_ != nullptr);
    return *pluginEditorWindowManager_;
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
    playheadSamples_.store(0);  // Phase 1.3: Reset playhead

    // Phase 1: Prepare tracks for audio processing
    prepareTracks(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());

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
    bool recording = isRecording_.load();

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

    // Phase 2D: Process recording (can record even when not playing, but typically we start playback)
    if (recording)
    {
        processAudioRecording(inputChannelData, numInputChannels, numSamples);
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
    juce::int64 position = playheadSamples_.load();

    // Update transport position for all clips in all tracks
    for (auto& track : tracks_)
    {
        if (track == nullptr)
            continue;

        for (int i = 0; i < track->getNumClips(); ++i)
        {
            auto* clip = track->getClip(i);
            if (clip != nullptr)
            {
                clip->setTransportPosition(position);
            }
        }
    }

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

            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                if (outputChannelData[channel] != nullptr)
                {
                    outputChannelData[channel][sample] += value;  // Add instead of replace
                }
            }

            phase += phaseIncrement;
            if (phase >= 2.0 * juce::MathConstants<double>::pi)
                phase -= 2.0 * juce::MathConstants<double>::pi;
        }
    }
}

//==============================================================================
// Track Management (MESSAGE THREAD)
//==============================================================================

void Engine::prepareTracks(int samplesPerBlockExpected, double sampleRate)
{
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

//==============================================================================
// Phase 2A: MIDI Input Handling
//==============================================================================

void Engine::enableMidiInput()
{
    DBG("Engine: Enabling MIDI input...");

    // Get list of available MIDI input devices
    auto midiInputs = juce::MidiInput::getAvailableDevices();

    if (midiInputs.isEmpty())
    {
        DBG("Engine: No MIDI input devices available");
        return;
    }

    // Open the first available MIDI input device
    const auto& firstInput = midiInputs[0];
    DBG("Engine: Opening MIDI input: " + firstInput.name);

    midiInput_ = juce::MidiInput::openDevice(firstInput.identifier, this);

    if (midiInput_ != nullptr)
    {
        midiInput_->start();
        DBG("Engine: MIDI input started: " + firstInput.name);
    }
    else
    {
        DBG("Engine: Failed to open MIDI input");
    }

    // Initialize MIDI recording buffers for all tracks
    {
        const juce::ScopedLock sl(midiRecordingLock_);
        midiRecording_.trackRecordings.resize(tracks_.size());
    }
}

void Engine::disableMidiInput()
{
    if (midiInput_ != nullptr)
    {
        DBG("Engine: Stopping MIDI input...");
        midiInput_->stop();
        midiInput_.reset();
        DBG("Engine: MIDI input stopped");
    }
}

void Engine::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    juce::ignoreUnused(source);

    // This runs on MIDI input thread (NOT audio thread or message thread)
    // Buffer the message for processing in audio callback

    // Add message to incoming buffer with current timestamp
    {
        const juce::ScopedLock sl(midiInputLock_);
        incomingMidiBuffer_.addEvent(message, 0);  // Will be time-adjusted in audio callback
    }

    // Phase 2A: MIDI recording - if recording is active, store with playhead timestamp
    if (isRecording_.load())
    {
        const juce::ScopedLock sl(midiRecordingLock_);

        // Get current playhead position
        const juce::int64 playhead = playheadSamples_.load();
        const juce::int64 recordStart = midiRecording_.recordingStartSamples;

        // Calculate position relative to recording start
        const double positionInSeconds = static_cast<double>(playhead - recordStart) / currentSampleRate.load();

        // Add to all armed MIDI/Instrument tracks
        for (size_t i = 0; i < tracks_.size() && i < midiRecording_.trackRecordings.size(); ++i)
        {
            auto* track = tracks_[i].get();
            if (track != nullptr && track->isArmed() &&
                (track->getType() == zenith::Track::Type::MIDI ||
                 track->getType() == zenith::Track::Type::Instrument))
            {
                // Create timestamped message
                juce::MidiMessage timestampedMessage(message);
                timestampedMessage.setTimeStamp(positionInSeconds);

                // Add to track's recording buffer
                midiRecording_.trackRecordings[i].addEvent(timestampedMessage);
            }
        }
    }
}

//==============================================================================
// Phase 2D: Audio Recording (AUDIO THREAD)
//==============================================================================

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
// Phase 2D: Audio Recording Helpers (MESSAGE THREAD)
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

//==============================================================================
// Phase 2C: MIDI Recording Baking (MESSAGE THREAD)
//==============================================================================

void Engine::bakeMidiRecordingsIntoClips(bool quantize)
{
    DBG("Engine: Baking MIDI recordings into clips (quantize=" + juce::String(quantize ? "true" : "false") + ")");

    // Get project tempo for quantization (default to 120 BPM if no project state)
    const double tempo = (projectState_ != nullptr) ? projectState_->getTempo() : 120.0;

    // Copy recording data while locked (minimize lock time)
    std::vector<juce::MidiMessageSequence> recordingsCopy;
    juce::int64 recordStart = 0;

    {
        const juce::ScopedLock sl(midiRecordingLock_);
        recordingsCopy = midiRecording_.trackRecordings;
        recordStart = midiRecording_.recordingStartSamples;
    }

    // Process each track's recording (unlocked)
    for (size_t i = 0; i < recordingsCopy.size() && i < tracks_.size(); ++i)
    {
        auto& recording = recordingsCopy[i];

        // Skip empty recordings
        if (recording.getNumEvents() == 0)
            continue;

        // Quantize if requested
        juce::MidiMessageSequence finalSequence = recording;
        if (quantize)
        {
            finalSequence = quantizeMidiSequence(recording, tempo, 0.25);  // 1/16 note grid
        }

        // Ensure note-off events are properly matched
        finalSequence.updateMatchedPairs();

        // Calculate clip length from sequence end time
        const double endTimeSeconds = finalSequence.getEndTime();
        const juce::int64 clipLengthSamples = static_cast<juce::int64>(
            endTimeSeconds * currentSampleRate.load());

        // Create MIDI clip
        auto clip = std::make_unique<zenith::Track::Clip>();
        clip->setType(zenith::Track::Clip::Type::MIDI);
        clip->setName("MIDI Recording");
        clip->setMidiSequence(finalSequence);
        clip->setStartPosition(recordStart);
        clip->setLength(clipLengthSamples);

        // Add clip to track
        auto* track = tracks_[i].get();
        if (track != nullptr)
        {
            track->addClip(std::move(clip));
            DBG("Engine: Created MIDI clip on track " + juce::String(i) +
                " (start=" + juce::String(recordStart) +
                ", length=" + juce::String(clipLengthSamples) +
                ", events=" + juce::String(finalSequence.getNumEvents()) + ")");
        }
    }

    DBG("Engine: Baking complete");
}

void Engine::clearMidiRecordings()
{
    const juce::ScopedLock sl(midiRecordingLock_);

    for (auto& recording : midiRecording_.trackRecordings)
    {
        recording.clear();
    }

    midiRecording_.recordingStartSamples = 0;

    DBG("Engine: MIDI recordings cleared");
}

juce::MidiMessageSequence Engine::quantizeMidiSequence(
    const juce::MidiMessageSequence& input,
    double tempo,
    double quantizeGrid)
{
    // Calculate grid spacing in seconds
    const double beatsPerSecond = tempo / 60.0;
    const double quarterNoteSeconds = 1.0 / beatsPerSecond;
    const double gridSeconds = quarterNoteSeconds * quantizeGrid;

    juce::MidiMessageSequence output;

    // Quantize each event
    for (int i = 0; i < input.getNumEvents(); ++i)
    {
        auto* event = input.getEventPointer(i);
        if (event == nullptr)
            continue;

        double timestamp = event->message.getTimeStamp();

        // Quantize to nearest grid point
        double quantized = std::round(timestamp / gridSeconds) * gridSeconds;

        // Ensure non-negative timestamps
        quantized = juce::jmax(0.0, quantized);

        // Create quantized message
        juce::MidiMessage msg(event->message);
        msg.setTimeStamp(quantized);
        output.addEvent(msg);
    }

    // Update note-on/note-off pairing after quantization
    output.updateMatchedPairs();

    return output;
}
