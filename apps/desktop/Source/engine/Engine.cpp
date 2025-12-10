/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "Engine.h"
#include "../../include/TrackAutomationSynchronizer.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include <algorithm>     // For std::remove_if
#include <array>         // For RT-safe stack allocation in audio callback
#include <unordered_set> // For updateEnvelopeFollowers

// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "../ai/SessionDebuggerAgent.h"
#include "../ai/SampleHunterAgent.h"
#include "../engine/AudioFilePool.h"
#include "../engine/AuxBus.h"
#include "../engine/Clip.h"
#include "../engine/MixerChannel.h"
#include "../engine/PluginHost.h"
#include "../engine/Track.h"
#include "../instruments/InstrumentRegistry.h"
#include "../instruments/RegisterBuiltInInstruments.h"
#include "../ui/PluginEditorWindow.h"

//==============================================================================
namespace {
constexpr int kDefaultInputChannels = 2;
constexpr int kDefaultOutputChannels = 2;
constexpr int kDefaultTestTrackCount = 8;
constexpr int kSessionDebuggerIntervalMs = 500;
constexpr int kAudioThreadShutdownTimeoutMs = 1000;
constexpr int kFileWriteBufferSize = 32768; // 32KB
constexpr int kAutomationUpdateRateHz = 60;
constexpr int kMaxStereoChannels = 2;
constexpr int kMinMonoChannels = 1;
constexpr int kAudioDepth = 24;
} // namespace

//==============================================================================
namespace zenith {

Engine::Engine() {
  DBG("Engine: Constructor");

  // Phase 1.2: Initialize audio file pool
  audioFilePool_ = std::make_unique<zenith::AudioFilePool>();
  DBG("Engine: AudioFilePool created");

  // Phase 3: Initialize plugin host and editor window manager
  pluginHost_ = std::make_unique<zenith::PluginHost>();
  pluginEditorWindowManager_ =
      std::make_unique<zenith::PluginEditorWindowManager>();

  // Level 4: Initialize Instrument Registry
  instrumentRegistry_ = std::make_unique<zenith::InstrumentRegistry>();
  zenith::registerBuiltInInstruments(
      *instrumentRegistry_); // Register factories
  DBG("Engine: InstrumentRegistry initialized");

  // Initialize Session Debugger Agent (AI Technical Integrity)
  sessionDebugger_ = std::make_unique<ai::SessionDebuggerAgent>(*this);
  DBG("Engine: SessionDebuggerAgent initialized");

  // Phase 2D: Initialize audio recording infrastructure
  // Create background thread for audio file writing
  // Priority: normal priority, suitable for disk I/O
  audioWriterThread_ =
      std::make_unique<juce::TimeSliceThread>("Audio Writer Thread");
  audioWriterThread_->startThread(juce::Thread::Priority::normal);

  DBG("Engine: Audio recording infrastructure initialized");

  // Phase 15: Initialize tempo map
  tempoMap_ = std::make_unique<zenith::TempoMap>();
  DBG("Engine: TempoMap initialized");

  // Initialize track snapshot
  updateTrackSnapshot();
}

Engine::~Engine() {
  DBG("Engine: Destructor");

  // CODEX FIX P2: Set shutdown flag to prevent async callbacks
  isShuttingDown_.store(true);

  // Phase 2A: Disable MIDI input
  disableMidiInput();

  shutdown();

  // Phase 2D: Cleanup audio recording infrastructure
  // Stop writer thread
  if (audioWriterThread_ != nullptr) {
    audioWriterThread_->stopThread(
        kAudioThreadShutdownTimeoutMs); // Wait up to 1 second
    audioWriterThread_.reset();
  }

  // Clear audio file pool
  if (audioFilePool_ != nullptr) {
    audioFilePool_.reset();
  }

  DBG("Engine: Audio recording infrastructure cleaned up");
}

//==============================================================================
// Initialization / Shutdown
//==============================================================================

void Engine::setProjectState(ProjectState *state) {
  DBG("Engine: Setting project state");

  // Stop automation if running
  if (automationSynchronizer) {
    automationSynchronizer->stop();
    automationSynchronizer.reset();
  }

  projectState_ = state;

  // Create new automation synchronizer if we have a project state
  if (projectState_ != nullptr) {
    automationSynchronizer =
        std::make_unique<TrackAutomationSynchronizer>(*projectState_, *this);
    DBG("Engine: Created automation synchronizer");

    // Sync tracks with project state
    syncWithProjectState();

    // Sync tempo map
    syncTempoMap();
  }
}

void Engine::syncTempoMap() {
  if (projectState_ && tempoMap_) {
    tempoMap_->updateFromValueTree(projectState_->getTempoMap());
  }
}

void Engine::syncWithProjectState() {
  DBG("Engine: Syncing with project state");

  if (projectState_ == nullptr) {
    DBG("Engine: No project state, clearing tracks");
    tracks_.clear();
    return;
  }

  // Clear existing tracks
  tracks_.clear();

  // Get tracks from project state
  auto &state = projectState_->getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid()) {
    DBG("Engine: No tracks in project state");
    return;
  }

  const double sampleRate = currentSampleRate.load();
  const int bufferSize = currentBufferSize.load();
  const double tempo = projectState_->getTempo();

  // Helper: convert beats to samples
  // Helper: convert beats to samples using TempoMap
  auto beatsToSamples = [this, sampleRate](double beats) -> juce::int64 {
    if (tempoMap_) {
      return tempoMap_->beatsToSamples(beats, sampleRate);
    }
    // Fallback if no tempo map
    const double tempo = projectState_ ? projectState_->getTempo() : 120.0;
    const double secondsPerBeat = 60.0 / tempo;
    const double seconds = beats * secondsPerBeat;
    return static_cast<juce::int64>(seconds * sampleRate);
  };

  // Create engine tracks from project state
  for (auto trackNode : tracksNode) {
    juce::String trackName = trackNode[ProjectState::PROP_NAME].toString();
    juce::String trackType = trackNode[ProjectState::PROP_TYPE].toString();

    // Create track
    auto track = std::make_unique<zenith::Track>(
        trackName, trackType == "midi" ? zenith::Track::Type::MIDI
                                       : zenith::Track::Type::Audio);

    // Set track ID
    track->setTrackId(trackNode[ProjectState::PROP_ID].toString());

    // Set mixer properties
    track->setVolume(trackNode[ProjectState::PROP_VOLUME]);
    track->setPan(trackNode[ProjectState::PROP_PAN]);
    track->setMuted(trackNode[ProjectState::PROP_MUTE]);
    track->setSolo(trackNode[ProjectState::PROP_SOLO]);

    // Prepare track for playback
    if (sampleRate > 0) {
      track->prepareToPlay(bufferSize, sampleRate);
    }

    // Load clips
    auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
    if (clipsNode.isValid()) {
      for (auto clipNode : clipsNode) {
        // Create clip
        auto clip = std::make_unique<zenith::Track::Clip>();

        // Set basic properties
        double startBeats = clipNode[ProjectState::PROP_START];
        double lengthBeats = clipNode[ProjectState::PROP_LENGTH];

        clip->setStartPosition(beatsToSamples(startBeats));
        clip->setLength(beatsToSamples(lengthBeats));

        // Load audio file if present
        juce::String audioFilePath =
            clipNode[ProjectState::PROP_AUDIO_FILE].toString();
        if (audioFilePath.isNotEmpty()) {
          juce::File audioFile(audioFilePath);
          if (audioFile.existsAsFile()) {
            clip->setAudioFile(audioFile);
            clip->setType(zenith::Track::Clip::Type::Audio);
            DBG("Engine: Loaded audio file: " + audioFile.getFileName());
          } else {
            DBG("Engine: Warning - audio file not found: " + audioFilePath);
          }
        }

        // Prepare clip
        if (sampleRate > 0) {
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

    // Register with RoutingGraph
    RoutingGraph::Node node;
    node.id = tracks_.back()->getTrackId();
    node.name = tracks_.back()->getName();
    node.type = RoutingGraph::NodeType::Track;
    routingGraph_.addNode(node);
  }

  DBG("Engine: Synced " + juce::String(tracks_.size()) + " tracks");

  // Update snapshot for audio thread
  updateTrackSnapshot();
}

bool Engine::initialize() {
  DBG("Engine: Initializing...");

  // Initialize audio device manager
  auto error = deviceManager.initialiseWithDefaultDevices(
      kDefaultInputChannels, kDefaultOutputChannels); // 2 in, 2 out

  if (error.isNotEmpty()) {
    DBG("Engine: Failed to initialize audio device: " + error);
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon, "Audio Device Error",
        "Failed to initialize audio device:\n" + error, "OK");
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

  // C3: Optional debug seed (disabled by default; enable with
  // -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON)
#if defined(JUCE_DEBUG) && defined(ZENITH_ENGINE_SEED_DEBUG_TRACKS)
  DBG("Engine: Seeding debug tracks (ZENITH_ENGINE_SEED_DEBUG_TRACKS enabled)");
  addTestTracks(kDefaultTestTrackCount);
#endif

  // Start Session Debugger monitoring (AI Technical Integrity Agent)
  if (sessionDebugger_) {
    sessionDebugger_->startMonitoring(
        kSessionDebuggerIntervalMs); // Analyze every 500ms
    DBG("Engine: SessionDebugger monitoring started");
  }

  DBG("Engine: Initialization complete!");
  return true;
}

void Engine::shutdown() {
  DBG("Engine: Shutting down...");

  // Stop Session Debugger monitoring first
  if (sessionDebugger_) {
    sessionDebugger_->stopMonitoring();
    DBG("Engine: SessionDebugger monitoring stopped");
  }

  // Stop playback
  stop();

  // Remove audio callback
  deviceManager.removeAudioCallback(this);

  // Close audio device
  deviceManager.closeAudioDevice();

  // Clear audio file pool
  if (audioFilePool_) {
    audioFilePool_->clear();
  }

  DBG("Engine: Shutdown complete");
}

//==============================================================================
// Transport Controls
//==============================================================================

void Engine::play() {
  DBG("Engine: Play");
  isPlaying_.store(true);

  // Phase 1.3: Use new playhead system
  // If playhead is at or past loop end, reset to loop start or 0
  const juce::int64 loopEnd = loopEndSamples_.load();
  const juce::int64 loopStart = loopStartSamples_.load();
  const juce::int64 currentPos = playheadSamples_.load();

  if (loopEnd > 0 && currentPos >= loopEnd) {
    playheadSamples_.store(loopStart);
  }

  // Enable test tone for Phase 0 fallback (when no tracks)
  enableTestTone_.store(true);

  // Phase 13: Start automation synchronizer
  if (automationSynchronizer) {
    automationSynchronizer->start(60); // 60 Hz update rate
    DBG("Engine: Started automation synchronizer");
  }
}

void Engine::stop() {
  DBG("Engine: Stop");

  // Phase 2C: If recording, bake recordings into clips first
  if (isRecording_.load()) {
    stopRecording();
  }

  isPlaying_.store(false);
  enableTestTone_.store(false);

  // Phase 13: Stop automation synchronizer
  if (automationSynchronizer) {
    automationSynchronizer->stop();
    DBG("Engine: Stopped automation synchronizer");
  }
}

double Engine::getPlaybackPositionBeats() const {
  if (projectState_ == nullptr)
    return 0.0;

  const double tempo = projectState_->getTempo();
  const double sampleRate = currentSampleRate.load();
  const juce::int64 positionSamples = playheadSamples_.load();

  // Use TempoMap for accurate conversion
  if (tempoMap_) {
    const double seconds = static_cast<double>(positionSamples) / sampleRate;
    return tempoMap_->secondsToBeats(seconds, sampleRate);
  }

  // Fallback
  const double seconds = static_cast<double>(positionSamples) / sampleRate;
  const double beats = (seconds * tempo) / 60.0;

  return beats;
}

//==============================================================================
// Phase 2C/2D: MIDI and Audio Recording
//==============================================================================

void Engine::record() {
  DBG("Engine: Record");

  // Start playback if not already playing
  if (!isPlaying_.load()) {
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
    for (auto &trackRecording : midiRecording_.trackRecordings) {
      trackRecording.clear();
    }
  }

  // ==========================================================================
  // Phase 2D: Setup Audio recording
  // ==========================================================================

  // Create recordings directory
  juce::File recordingsDir;

  if (projectState_ != nullptr &&
      projectState_->getProjectFile().existsAsFile()) {
    // Use "Audio Files" directory next to project file
    recordingsDir =
        projectState_->getProjectFile().getSiblingFile("Audio Files");
  } else {
    // Fallback to Documents/ZenithDAW/Recordings
    recordingsDir =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("ZenithDAW/Recordings");
  }

  if (!recordingsDir.exists()) {
    recordingsDir.createDirectory();
  }

  // Create recording sessions for all armed audio tracks
  audioRecordingSessions_.clear();

  for (size_t i = 0; i < tracks_.size(); ++i) {
    auto &track = tracks_[i];

    // Skip if not armed or not an audio track
    if (!track->isArmed() || track->getType() != zenith::Track::Type::Audio)
      continue;

    DBG("Engine: Creating recording session for track " + juce::String(i) +
        " (" + track->getName() + ")");

    // ROAST FIX #4: Check for pre-prepared session
    bool foundPrepped = false;
    AudioRecordingSession sessionToUse;
    {
      const juce::ScopedLock sl(preppedSessionsLock_);
      auto it = std::find_if(
          preppedSessions_.begin(), preppedSessions_.end(),
          [i](const auto &s) { return s.trackIndex == static_cast<int>(i); });

      if (it != preppedSessions_.end()) {
        sessionToUse = std::move(*it);
        preppedSessions_.erase(it);
        foundPrepped = true;
      }
    }

    if (foundPrepped) {
      // Update start time and use prepped session
      sessionToUse.recordingStartSamples = recordStartSamples;
      audioRecordingSessions_.push_back(std::move(sessionToUse));
      DBG("Engine: Used pre-prepared recording session for track " +
          juce::String(i));
      continue;
    }

    // Fallback: Create synchronously (BLOCKING I/O)
    DBG("Engine: Creating recording session synchronously (fallback)");

    // Create unique filename with timestamp
    juce::String timestamp =
        juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    juce::String filename =
        track->getName().replaceCharacter(' ', '_') + "_" + timestamp + ".wav";
    juce::File recordFile = recordingsDir.getChildFile(filename);

    // CODEX P1 FIX: Respect actual input channel count instead of hardcoding
    // Get the number of active input channels from the device
    auto *device = deviceManager.getCurrentAudioDevice();
    const int deviceInputChannels =
        device ? device->getActiveInputChannels().countNumberOfSetBits() : 1;

    // For now: use mono (1 channel) or stereo (2 channels) based on device
    // capability Clamp to min(2, deviceInputChannels) to avoid exceeding device
    // capabilities
    const int numChannels = juce::jmin(
        kMaxStereoChannels, juce::jmax(kMinMonoChannels, deviceInputChannels));

    // Create WAV writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> fileStream(
        new juce::FileOutputStream(recordFile));

    if (!fileStream->openedOk()) {
      DBG("Engine: Failed to create output stream for " +
          recordFile.getFullPathName());
      continue;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(fileStream.release(), sampleRate,
                                  static_cast<unsigned int>(numChannels),
                                  kAudioDepth, // 24-bit depth
                                  {},          // Default metadata
                                  0            // Default quality
                                  ));

    if (writer == nullptr) {
      DBG("Engine: Failed to create audio writer for " +
          recordFile.getFullPathName());
      continue;
    }

    // Wrap in ThreadedWriter for RT-safe writing
    auto threadedWriter =
        std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            writer.release(), *audioWriterThread_,
            kFileWriteBufferSize // 32KB FIFO buffer
        );

    // Create session
    AudioRecordingSession session;
    session.writer = std::move(threadedWriter);
    session.file = recordFile;
    session.numChannels = numChannels;
    session.sampleRate = sampleRate;
    session.recordingStartSamples = recordStartSamples;
    session.trackIndex = static_cast<int>(i);
    // ROAST FIX #9: Capture input channel from track
    session.inputChannelIndex = tracks_[i]->getInputChannel();

    audioRecordingSessions_.push_back(std::move(session));

    DBG("Engine: Recording to " + recordFile.getFullPathName());
  }

  // ==========================================================================
  // CODEX FIX P1: Enable recording flag AFTER sessions are set up
  // This prevents the audio thread from accessing sessions before they're ready
  // ==========================================================================
  isRecording_.store(true);

  DBG("Engine: Recording started at sample " +
      juce::String(recordStartSamples));
  DBG("Engine: Audio sessions: " +
      juce::String(audioRecordingSessions_.size()));
}

void Engine::stopRecording() {
  DBG("Engine: Stop recording");

  // Stop accepting new samples immediately
  isRecording_.store(false);

  // ==========================================================================
  // CODEX FIX P2: Guard async callback against use-after-free
  // Check if we're shutting down before posting async operations
  // ==========================================================================
  if (isShuttingDown_.load()) {
    DBG("Engine: Shutdown in progress, skipping async recording cleanup");
    return;
  }

  // ==========================================================================
  // Phase 2C: Bake MIDI recordings into clips
  // ==========================================================================
  bakeMidiRecordingsIntoClips(true); // With quantization
  clearMidiRecordings();

  // ==========================================================================
  // Phase 2D: Process audio recordings asynchronously
  // This MUST be async because we need to flush writers on the message thread
  // ==========================================================================
  juce::MessageManager::callAsync([this]() {
    // Double-check we're not shutting down
    if (isShuttingDown_.load()) {
      DBG("Engine: Shutdown detected in async callback, aborting");
      return;
    }

    DBG("Engine: Flushing and closing " +
        juce::String(audioRecordingSessions_.size()) + " recording sessions");

    // Flush and close all writers, then create clips
    for (auto &session : audioRecordingSessions_) {
      // Flush and delete writer (triggers file close)
      session.writer.reset();

      DBG("Engine: Closed recording: " + session.file.getFullPathName());

      // Create audio clip from recording
      if (session.trackIndex >= 0 &&
          session.trackIndex < static_cast<int>(tracks_.size())) {
        auto &track = tracks_[session.trackIndex];
        bakeAudioRecordingIntoTrack(*track, session.file,
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

void Engine::toggleRecording() {
  if (isRecording()) {
    stopRecording();
  } else {
    record();
  }
}

//==============================================================================
// Phase 1.3: Transport Position & Looping
//==============================================================================

void Engine::setPlayheadSamples(juce::int64 position) {
  playheadSamples_.store(juce::jmax(juce::int64(0), position));
}

void Engine::setLooping(bool shouldLoop) {
  isLooping_.store(shouldLoop);
  DBG("Engine: Looping " + juce::String(shouldLoop ? "enabled" : "disabled"));
}

void Engine::setLoopRegion(juce::int64 start, juce::int64 end) {
  loopStartSamples_.store(juce::jmax(juce::int64(0), start));
  loopEndSamples_.store(juce::jmax(juce::int64(0), end));

  DBG("Engine: Loop region set: " + juce::String(start) + " - " +
      juce::String(end) + " samples");
}

//==============================================================================
// Audio Device Management
//==============================================================================

juce::String Engine::getAudioDeviceInfo() const {
  auto *device = deviceManager.getCurrentAudioDevice();

  if (device == nullptr)
    return "No device";

  auto name = device->getName();
  auto sampleRate = device->getCurrentSampleRate();
  auto bufferSize = device->getCurrentBufferSizeSamples();

  return name + " @ " + juce::String(sampleRate, 0) + " Hz, " +
         juce::String(bufferSize) + " samples";
}

//==============================================================================
// CPU Monitoring
//==============================================================================

double Engine::getCpuUsage() const {
  return deviceManager.getCpuUsage() * 100.0;
}

//==============================================================================
// C3: Minimal Engine Surface (compile-only, no audio wiring)
//==============================================================================

int Engine::getNumTracks() const noexcept {
  return static_cast<int>(tracks_.size());
}

const std::vector<std::shared_ptr<zenith::Track>> &
Engine::tracks() const noexcept {
  return tracks_;
}

void Engine::addTestTracks(int count) {
  if (count <= 0)
    return;

  DBG("Engine: Adding " + juce::String(count) + " test tracks");

  // Reserve capacity to avoid reallocations
  tracks_.reserve(tracks_.size() + static_cast<size_t>(count));

  for (int i = 0; i < count; ++i) {
    // Create track with default name and type
    // ROAST FIX #1: Use make_shared instead of make_unique
    auto track = std::make_shared<zenith::Track>(
        "Track " + juce::String(tracks_.size() + 1),
        zenith::Track::Type::Audio);

    // Phase 11: Prepare track for audio processing if engine is already running
    if (currentSampleRate.load() > 0) {
      track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
    }

    tracks_.push_back(track); // No std::move needed for shared_ptr

    // Register with RoutingGraph
    RoutingGraph::Node node;
    node.id = track->getTrackId();
    node.name = track->getName();
    node.type = RoutingGraph::NodeType::Track;
    routingGraph_.addNode(node);
  }

  DBG("Engine: Total tracks: " + juce::String(tracks_.size()));

  // Re-prepare tracks if audio device is already running
  auto *device = deviceManager.getCurrentAudioDevice();
  if (device != nullptr) {
    prepareTracks(device->getCurrentBufferSizeSamples(),
                  device->getCurrentSampleRate());
  }

  // Update snapshot for audio thread
  updateTrackSnapshot();
}

//==============================================================================
// Phase 1.2: Audio File Pool
//==============================================================================

zenith::AudioFilePool &Engine::getAudioFilePool() {
  jassert(audioFilePool_ != nullptr);
  return *audioFilePool_;
}

//==============================================================================
// Plugin Hosting (Phase 3: VST3 hosting MVP)
//==============================================================================

zenith::PluginHost &Engine::getPluginHost() noexcept {
  jassert(pluginHost_ != nullptr);
  return *pluginHost_;
}

int Engine::scanForPlugins() {
  if (pluginHost_ == nullptr) {
    DBG("Engine: PluginHost not initialized");
    return 0;
  }

  DBG("Engine: Scanning for plugins...");
  int count = pluginHost_->scanDefaultLocations();
  DBG("Engine: Plugin scan complete - found " + juce::String(count) +
      " plugins");

  return count;
}

zenith::PluginEditorWindowManager &
Engine::getPluginEditorWindowManager() noexcept {
  jassert(pluginEditorWindowManager_ != nullptr);
  return *pluginEditorWindowManager_;
}

// getInstrumentRegistry() is now defined inline in Engine.h

const zenith::TempoMap &Engine::getTempoMap() const noexcept {
  jassert(tempoMap_ != nullptr);
  return *tempoMap_;
}

//==============================================================================
// Flecs ECS Integration
//==============================================================================

//==============================================================================
// Phase 11: Mixer Control (MESSAGE THREAD ONLY)
//==============================================================================

void Engine::setTrackVolume(int trackIndex, float volume) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
    tracks_[trackIndex]->setVolume(volume);
  }
}

void Engine::setTrackPan(int trackIndex, float pan) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
    tracks_[trackIndex]->setPan(pan);
  }
}

void Engine::setTrackInputChannel(int trackIndex, int channelIndex) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
    tracks_[trackIndex]->setInputChannel(channelIndex);
  }
}

void Engine::setTrackMute(int trackIndex, bool muted) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
    tracks_[trackIndex]->setMuted(muted);
  }
}

void Engine::setTrackSolo(int trackIndex, bool solo) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
    tracks_[trackIndex]->setSolo(solo);
    updateSoloState();
  }
}

void Engine::setTrackArmed(int trackIndex, bool armed) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
    tracks_[trackIndex]->setArmed(armed);

    // ROAST FIX #4: Prepare recording asynchronously when armed
    if (armed) {
      prepareRecordingForTrack(trackIndex);
    }
  }
}

//==============================================================================
// Phase 11: Metering (MESSAGE THREAD SAFE)
//==============================================================================

float Engine::getTrackLevel(int trackIndex) const {
  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
    return tracks_[trackIndex]->getCurrentLevel();
  }
  return 0.0f;
}

float Engine::getTrackPeakLevel(int trackIndex) const {
  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
    return tracks_[trackIndex]->getPeakLevel();
  }
  return 0.0f;
}

float Engine::getMasterLevel() const { return masterLevel_.load(); }

float Engine::getMasterPeakLevel() const { return masterPeakLevel_.load(); }

void Engine::resetPeakMeters() {
  // Reset master peak
  masterPeakLevel_.store(0.0f);

  // Reset all track peaks (message thread only)
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  for (auto &track : tracks_) {
    if (track != nullptr) {
      track->resetPeakLevel();
    }
  }
}

juce::String Engine::createTrack(const juce::String &name,
                                 const juce::String &type) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (projectState_) {
    // Add track to ProjectState
    // This will trigger TrackStateSynchronizer::valueTreeChildAdded,
    // which will call Engine::addTrack()
    return projectState_->addTrack(name, type);
  } else {
    DBG("Engine: Creating track without ProjectState (fallback)");

    // Fallback: Create track directly in Engine
    // ROAST FIX #1: Use make_shared instead of make_unique
    zenith::Track::Type trackType = (type == "midi")
                                        ? zenith::Track::Type::MIDI
                                        : zenith::Track::Type::Audio;
    auto track = std::make_shared<zenith::Track>(name, trackType);

    // Generate a fake ID
    juce::String trackId = "track_" + juce::String(tracks_.size());
    track->setTrackId(trackId);

    addTrack(track); // No std::move for shared_ptr

    return trackId;
  }
}

// ROAST FIX #1: Accept shared_ptr instead of unique_ptr
void Engine::addTrack(std::shared_ptr<zenith::Track> track) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  jassert(track != nullptr);

  // Prepare the track if audio is already running
  if (currentSampleRate.load() > 0) {
    track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
  }

  juce::String name = track->getName();
  juce::String id = track->getTrackId();

  tracks_.push_back(track); // Shared_ptr, no move needed

  // Register with RoutingGraph
  RoutingGraph::Node node;
  node.id = id;
  node.name = name;
  node.type = RoutingGraph::NodeType::Track;

  // Determine proper node type based on track type
  if (track->getType() == Track::Type::Bus) {
    node.type = RoutingGraph::NodeType::Bus;
  } else if (track->getType() == Track::Type::Master) {
    node.type = RoutingGraph::NodeType::Master;
  }
  routingGraph_.addNode(node);

  DBG("Engine: Added track '" + name + "' (ID: " + id + ")");

  // Update Solo State (new track might need to be silenced if others are
  // soloed)
  updateSoloState();

  // Update snapshot for audio thread
  // ROAST FIX #1: Snapshot now holds shared_ptr, extending track lifetime
  updateTrackSnapshot();
}

void Engine::removeTrack(int index) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (index >= 0 && index < static_cast<int>(tracks_.size())) {
    juce::String name = tracks_[index]->getName();
    juce::String id = tracks_[index]->getTrackId();

    // Release resources
    tracks_[index]->releaseResources();

    // Remove from vector
    tracks_.erase(tracks_.begin() + index);

    // Remove from RoutingGraph
    routingGraph_.removeNode(id);

    DBG("Engine: Removed track '" + name + "' at index " + juce::String(index));

    // Update Solo State (removed track might have been the only soloed one)
    updateSoloState();

    // Update snapshot for audio thread
    updateTrackSnapshot();
  }
}

// Helper to ensure followers exist for active sources
void Engine::updateEnvelopeFollowers() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Identify all Modulation Sources
  std::unordered_set<std::string> neededSources;

  // Iterate all nodes to find modulation sources
  // (We use getProcessingOrder to get all active nodes)
  const auto &nodes = routingGraph_.getProcessingOrder();
  for (const auto &sourceId : nodes) {
    const auto connections = routingGraph_.getConnectionsFrom(sourceId);
    for (const auto &conn : connections) {
      if (conn.type == RoutingGraph::Connection::Type::Modulation) {
        neededSources.insert(sourceId.toStdString());
        break; // One modulation output is enough to need a follower
      }
    }
  }

  // Remove unused followers
  for (auto it = envelopeFollowers_.begin(); it != envelopeFollowers_.end();) {
    if (neededSources.find(it->first) == neededSources.end()) {
      it = envelopeFollowers_.erase(it);
    } else {
      ++it;
    }
  }

  // Create missing followers
  for (const auto &sourceId : neededSources) {
    if (envelopeFollowers_.find(sourceId) == envelopeFollowers_.end()) {
      envelopeFollowers_[sourceId] =
          std::make_shared<zenith::dsp::EnvelopeFollower>();
    }
  }
}

void Engine::updateTrackSnapshot() {
  // ==============================================================================
  // Phase 2: Topological Execution Plan Compilation (Message Thread)
  // ==============================================================================

  // 0. Update Envelope Followers (Message Thread)
  updateEnvelopeFollowers();

  // 1. Get Processing Order from RoutingGraph (Kahn's Algorithm result)
  // This ensures we process nodes in dependency order (inputs before outputs)
  auto processingOrder = routingGraph_.getProcessingOrder();

  std::vector<RenderNode> sequence;
  sequence.reserve(processingOrder.size());

  // Helper maps for ID -> Index lookups
  std::unordered_map<std::string, int> trackIdToIndex;
  std::unordered_map<std::string, int> auxIdToIndex;
  std::unordered_map<std::string, zenith::Track *> trackPtrs;
  std::unordered_map<std::string, zenith::AuxBus *> auxPtrs;

  // Build lookups
  for (size_t i = 0; i < tracks_.size(); ++i) {
    if (tracks_[i]) {
      auto id = tracks_[i]->getTrackId().toStdString();
      trackIdToIndex[id] = static_cast<int>(i);
      trackPtrs[id] = tracks_[i].get();
    }
  }
  for (size_t i = 0; i < auxBuses_.size(); ++i) {
    if (auxBuses_[i]) {
      auto id = auxBuses_[i]->getId().toStdString();
      auxIdToIndex[id] = static_cast<int>(i);
      auxPtrs[id] = auxBuses_[i].get();
    }
  }

  // 2. Build Render Nodes
  for (const auto &nodeId : processingOrder) {
    RenderNode node;
    std::string idStr = nodeId.toStdString();
    bool isValidNode = false;

    // Determine type (Track or Aux)
    if (trackIdToIndex.count(idStr)) {
      node.outputBufferIndex = trackIdToIndex[idStr];
      node.track = trackPtrs[idStr];
      isValidNode = true;
    } else if (auxIdToIndex.count(idStr)) {
      node.outputBufferIndex = auxIdToIndex[idStr];
      node.bus = auxPtrs[idStr];
      isValidNode = true;
    }

    if (!isValidNode)
      continue; // Unknown node (maybe removed?)

    // Attach Envelope Follower (Modulation Source)
    if (envelopeFollowers_.count(idStr)) {
      node.follower = envelopeFollowers_[idStr];
    }

    // 3. Resolve Inputs
    auto connections = routingGraph_.getConnectionsTo(nodeId);
    for (const auto &conn : connections) {
      if (conn.type == RoutingGraph::Connection::Type::Modulation)
        continue;

      MixOp op;
      op.gain = conn.gain;
      op.isFeedback = conn.isFeedback;
      std::string srcId = conn.sourceId.toStdString();

      // Check if input is Track
      if (trackIdToIndex.count(srcId)) {
        op.sourceBufferIndex = trackIdToIndex[srcId];
        op.isSourceAux = false;
        node.inputs.push_back(op);
      }
      // Check if input is Aux
      else if (auxIdToIndex.count(srcId)) {
        op.sourceBufferIndex = auxIdToIndex[srcId];
        op.isSourceAux = true;
        node.inputs.push_back(op);
      }
    }

    // Check Output (Master Routing)
    auto outConnections = routingGraph_.getConnectionsFrom(nodeId);
    for (const auto &conn : outConnections) {
      // Only consider Audio connections for Master summing
      if (conn.type == RoutingGraph::Connection::Type::Audio) {
        const auto *destNode = routingGraph_.getNode(conn.destId);
        // Verify destination is Master
        if (destNode && destNode->type == RoutingGraph::NodeType::Master) {
          node.masterGain = conn.gain;
        }
      }
    }

    sequence.push_back(node);
  }

  // Create new snapshot with sequence
  auto newSnapshot =
      std::make_shared<TrackSnapshot>(tracks_, auxBuses_, sequence);

  // Atomic swap (release semantics for the store)
  // The audio thread will see the new pointer immediately
  activeSnapshot_.store(newSnapshot.get());

  // Manage lifetime of old snapshots
  // We keep the previous snapshot alive in snapshotTrash_
  // because the audio thread might still be reading it.
  snapshotTrash_.push_back(currentSnapshotHolder_);

  // Update current holder to the new snapshot
  currentSnapshotHolder_ = newSnapshot;

  // Garbage collection: Keep last 5 snapshots
  // At 60Hz updates, this gives plenty of margin for the audio thread to finish
  if (snapshotTrash_.size() > 5) {
    snapshotTrash_.erase(snapshotTrash_.begin());
  }
}

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void Engine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  DBG("Engine: Audio device starting...");

  // Update settings
  currentSampleRate.store(device->getCurrentSampleRate());
  currentBufferSize.store(device->getCurrentBufferSizeSamples());

  // Reset state
  phase = 0.0;
  playheadSamples_.store(0); // Phase 1.3: Reset playhead

  // Phase 1: Prepare tracks for audio processing
  prepareTracks(device->getCurrentBufferSizeSamples(),
                device->getCurrentSampleRate());

  // Prepare track buffers for unified render path
  const int bufferSize = currentBufferSize.load();
  const int numTracks = static_cast<int>(tracks_.size());

  DBG("Engine: Preparing " + juce::String(numTracks) + " track buffers");

  trackBuffers_.clear();
  trackBuffers_.resize(numTracks);

  for (int i = 0; i < numTracks; ++i) {
    // Allocate stereo buffer for each track
    trackBuffers_[i].setSize(2, bufferSize);
    trackBuffers_[i].clear();

    // Prepare track for playback
    if (tracks_[i] != nullptr) {
      tracks_[i]->prepareToPlay(bufferSize, currentSampleRate.load());
    }
  }

  // Prepare master buffer
  masterBuffer_.setSize(2, bufferSize);
  masterBuffer_.clear();

  // Initialize Global LFOs
  for (int i = 0; i < kNumGlobalLFOs; ++i) {
    globalLFOs_[static_cast<size_t>(i)].setSampleRate(currentSampleRate.load());
    globalLFOs_[static_cast<size_t>(i)].reset();
  }

  // Initialize Macro Bank
  macroBank_.setSampleRate(currentSampleRate.load());

  // Prepare Aux Bus buffers
  auxBusBuffers_.clear();
  auxBusBuffers_.resize(auxBuses_.size());
  for (size_t i = 0; i < auxBusBuffers_.size(); ++i) {
    auxBusBuffers_[i].setSize(2, bufferSize);
    auxBusBuffers_[i].clear();

    if (auxBuses_[i]) {
      auxBuses_[i]->prepareToPlay(bufferSize, currentSampleRate.load());
    }
  }

  // Phase 11: Master buffer already allocated above

  // Phase 11: Prepare all tracks for playback
  for (auto &track : tracks_) {
    if (track != nullptr) {
      track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
    }
  }

  DBG("Engine: Audio device started");
  DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
  DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
  DBG("  Track Buffers: " + juce::String(trackBuffers_.size()));
  DBG("  Tracks Prepared: " + juce::String(tracks_.size()));
}

void Engine::audioDeviceStopped() {
  DBG("Engine: Audio device stopped");

  // Phase 11: Release resources from all tracks
  for (auto &track : tracks_) {
    if (track != nullptr) {
      track->releaseResources();
    }
  }
}

void Engine::audioDeviceIOCallbackWithContext(
    const float *const *inputChannelData, int numInputChannels,
    float *const *outputChannelData, int numOutputChannels, int numSamples,
    const juce::AudioIODeviceCallbackContext &context) noexcept {
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

  if (playing) {
    // SAMPLE-ACCURATE LOOPING:
    // If loop wrap happens mid-buffer, we need to split the processing
    const juce::int64 currentPos = playheadSamples_.load();
    const bool looping = isLooping_.load();
    const juce::int64 loopEnd = loopEndSamples_.load();
    const juce::int64 loopStart = loopStartSamples_.load();

    // Check if loop wrap occurs within this buffer
    if (looping && loopEnd > 0 && loopEnd > loopStart) {
      const juce::int64 bufferEndPos = currentPos + numSamples;

      if (bufferEndPos > loopEnd && currentPos < loopEnd) {
        // Loop wrap occurs within this buffer!
        // Split into two parts: before loop end, and after loop start

        const int samplesBeforeLoop = static_cast<int>(loopEnd - currentPos);
        const int samplesAfterLoop = numSamples - samplesBeforeLoop;

        // Store loop wrap offset for any systems that need it
        loopWrapSampleOffset_.store(samplesBeforeLoop);

        // Part 1: Process samples up to loop end
        if (samplesBeforeLoop > 0) {
          // Create temp buffer for first part
          juce::AudioBuffer<float> outputBuffer1(
              outputChannelData, numOutputChannels, samplesBeforeLoop);
          outputBuffer1.clear();

          // Render at current position
          juce::MidiBuffer localMidi1;
          midiFifo_.drainTo(localMidi1, samplesBeforeLoop);
          renderAudioGraph(outputBuffer1, samplesBeforeLoop, currentPos,
                           &localMidi1);
        }

        // Part 2: Process samples from loop start
        if (samplesAfterLoop > 0) {
          // RT-SAFE FIX: Use stack-allocated array instead of heap
          // allocation Maximum 32 channels should cover any reasonable
          // audio setup
          constexpr int kMaxChannels = 32;
          jassert(numOutputChannels <= kMaxChannels);

          std::array<float *, kMaxChannels> offsetOutputStack;
          for (int ch = 0; ch < numOutputChannels; ++ch) {
            offsetOutputStack[ch] = outputChannelData[ch] + samplesBeforeLoop;
          }

          juce::AudioBuffer<float> outputBuffer2(
              offsetOutputStack.data(), numOutputChannels, samplesAfterLoop);
          outputBuffer2.clear();

          // Render from loop start
          juce::MidiBuffer localMidi2;
          midiFifo_.drainTo(localMidi2, samplesAfterLoop);
          renderAudioGraph(outputBuffer2, samplesAfterLoop, loopStart,
                           &localMidi2);

          // No delete needed - stack allocation
        }

        // Update playhead to position after loop
        playheadSamples_.store(loopStart + samplesAfterLoop);
      } else {
        // No loop wrap in this buffer - normal processing
        loopWrapSampleOffset_.store(-1);
        processAudioBlock(inputChannelData, numInputChannels, outputChannelData,
                          numOutputChannels, numSamples);

        // Advance playhead with loop wrap check
        juce::int64 newPosition = currentPos + numSamples;
        if (newPosition >= loopEnd) {
          const juce::int64 loopLength = loopEnd - loopStart;
          if (loopLength > 0) {
            while (newPosition >= loopEnd) {
              newPosition -= loopLength;
            }
            if (newPosition < loopStart) {
              newPosition = loopStart;
            }
          }
        }
        playheadSamples_.store(newPosition);
      }
    } else {
      // No looping - simple processing
      loopWrapSampleOffset_.store(-1);
      processAudioBlock(inputChannelData, numInputChannels, outputChannelData,
                        numOutputChannels, numSamples);
      playheadSamples_.store(currentPos + numSamples);
    }
  } else {
    // Silent output when not playing
    for (int channel = 0; channel < numOutputChannels; ++channel) {
      if (outputChannelData[channel] != nullptr) {
        juce::FloatVectorOperations::clear(outputChannelData[channel],
                                           numSamples);
      }
    }
  }

  // Phase 2D: Process recording (can record even when not playing, but
  // typically we start playback)
  if (recording) {
    captureAudioInput(inputChannelData, numInputChannels, numSamples);
  }
}

//==============================================================================
// Real-time Event Queue
//==============================================================================

bool Engine::queueEvent(const zenith::EngineEvent &e) {
  int start1, size1, start2, size2;
  commandFifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
    commandBuffer_[start1] = e;
    commandFifo_.finishedWrite(1);
    return true;
  }

  return false; // Buffer full
}

void Engine::processEvents() noexcept {
  int start1, size1, start2, size2;
  commandFifo_.prepareToRead(commandFifo_.getNumReady(), start1, size1, start2,
                             size2);

  // Load snapshot once for event processing
  auto *snapshot = activeSnapshot_.load();

  if (size1 > 0) {
    for (int i = 0; i < size1; ++i) {
      const auto &e = commandBuffer_[start1 + i];
      // Process event based on snapshot
      // Note: snapshot is already acquired in processAudio
      // But we need to access the tracks safely.
      // Since we are in processAudio, we are safe to modify RT parameters
      // IF the track objects support it.
      // Zenith tracks generally use atomic parameters or critical
      // sections internally for parameters.

      if (e.type == zenith::EngineEvent::Type::SetPluginParam) {
        // Get thread-safe snapshot (we are in audio thread, so we read
        // snapshot) But wait, we need to apply this to the track. The
        // track pointer in snapshot is valid. Finding the track:
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          auto *track = snapshot->tracks[e.trackIndex];
          if (track) {
            auto *plugin = track->getPlugin(e.pluginIndex);
            if (plugin) {
              auto params = plugin->getParameters();
              if (e.paramIndex >= 0 && e.paramIndex < (int)params.size()) {
                // JUCE parameters are thread-safe
                params[e.paramIndex]->setValueNotifyingHost(e.value);
              }
            }
          }
        }
      } else if (e.type == zenith::EngineEvent::Type::SetTrackVolume) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          if (auto *track = snapshot->tracks[e.trackIndex]) {
            track->setVolume(e.value);
          }
        }
      } else if (e.type == zenith::EngineEvent::Type::SetTrackPan) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          if (auto *track = snapshot->tracks[e.trackIndex]) {
            track->setPan(e.value);
          }
        }
      } else if (e.type == zenith::EngineEvent::Type::SetTrackMute) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          if (auto *track = snapshot->tracks[e.trackIndex]) {
            track->setMuted(e.boolValue);
          }
        }
      } else if (e.type == zenith::EngineEvent::Type::SetTrackSolo) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          if (auto *track = snapshot->tracks[e.trackIndex]) {
            track->setSolo(e.boolValue);
          }
        }
      }
      // Implement other event types here...
    }
  }
  if (size2 > 0) {
    for (int i = 0; i < size2; ++i) {
      const auto &e = commandBuffer_[start2 + i];
      // (Duplicate logic for wrap-around - ideally factor this out)
      if (e.type == zenith::EngineEvent::Type::SetPluginParam) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          auto *track = snapshot->tracks[e.trackIndex];
          if (track) {
            auto *plugin = track->getPlugin(e.pluginIndex);
            if (plugin) {
              auto params = plugin->getParameters();
              if (e.paramIndex >= 0 && e.paramIndex < (int)params.size()) {
                params[e.paramIndex]->setValueNotifyingHost(e.value);
              }
            }
          }
        }
      } else if (e.type == zenith::EngineEvent::Type::SetTrackVolume) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          if (auto *track = snapshot->tracks[e.trackIndex]) {
            track->setVolume(e.value);
          }
        }
      } else if (e.type == zenith::EngineEvent::Type::SetTrackPan) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          if (auto *track = snapshot->tracks[e.trackIndex]) {
            track->setPan(e.value);
          }
        }
      } else if (e.type == zenith::EngineEvent::Type::SetTrackMute) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          if (auto *track = snapshot->tracks[e.trackIndex]) {
            track->setMuted(e.boolValue);
          }
        }
      } else if (e.type == zenith::EngineEvent::Type::SetTrackSolo) {
        if (snapshot && e.trackIndex >= 0 &&
            e.trackIndex < (int)snapshot->tracks.size()) {
          if (auto *track = snapshot->tracks[e.trackIndex]) {
            track->setSolo(e.boolValue);
          }
        }
      }
    }
  }

  commandFifo_.finishedRead(size1 + size2);
}

//==============================================================================
// Audio Processing (AUDIO THREAD)
//==============================================================================

void Engine::processAudioBlock(const float *const *inputChannelData,
                               int numInputChannels,
                               float *const *outputChannelData,
                               int numOutputChannels, int numSamples) noexcept {
  // ⚠️ AUDIO THREAD - REAL-TIME SAFE!
  //
  // CRITICAL WARNING: Accessing 'tracks_' (std::vector) here is NOT
  // thread-safe! If the message thread adds/removes tracks while this
  // runs, the vector may reallocate, causing a segfault.
  //
  // Phase 11: Process all tracks and mix them down to master output

  juce::ignoreUnused(inputChannelData, numInputChannels);

  // Wrap output buffer for unified render path
  juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels,
                                        numSamples);

  // Get current transport position
  juce::int64 position = playheadSamples_.load();

  // Update transport position for all clips in all tracks
  // Get thread-safe snapshot (Lock-free load)
  auto *snapshot = activeSnapshot_.load();

  // Process queued events (Parameter changes, etc.)
  processEvents();

  // Update transport position for all clips in all tracks
  if (snapshot) {
    for (auto *track : snapshot->tracks) {
      if (track == nullptr)
        continue;

      for (int i = 0; i < track->getNumClips(); ++i) {
        auto *clip = track->getClip(i);
        if (clip != nullptr) {
          clip->setTransportPosition(position);
        }
      }
    }
  }

  // Use unified render path

  // Thread-safe MIDI transfer:
  // 1. Create local buffer
  // 2. Drain FIFO into local buffer
  juce::MidiBuffer localMidi;
  midiFifo_.drainTo(localMidi, numSamples);

  // Fix: Correct argument order (numSamples, position) and pass local
  // MIDI
  renderAudioGraph(outputBuffer, numSamples, position, &localMidi);

  // Fallback: If no tracks or all tracks are silent, optionally enable
  // test tone (Only if explicitly enabled via enableTestTone_)
  bool testToneEnabled = enableTestTone_.load();

  if (testToneEnabled && tracks_.empty()) {
    // Generate 440 Hz sine wave at -12 dB (only if no tracks exist)
    const double sampleRate = currentSampleRate.load();
    const double frequency = 440.0; // A4
    const double amplitude = 0.25;  // -12 dB
    const double phaseIncrement =
        frequency * 2.0 * juce::MathConstants<double>::pi / sampleRate;

    // Use member variable phase (initialized in prepareToPlay)
    for (int sample = 0; sample < numSamples; ++sample) {
      float value = static_cast<float>(std::sin(phase) * amplitude);

      for (int channel = 0; channel < numOutputChannels; ++channel) {
        if (outputChannelData[channel] != nullptr) {
          outputChannelData[channel][sample] += value; // Add instead of replace
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

void Engine::prepareTracks(int samplesPerBlockExpected, double sampleRate) {
  DBG("Engine: Preparing " + juce::String(tracks_.size()) + " tracks");

  // Prepare each track
  for (auto &track : tracks_) {
    if (track != nullptr) {
      track->prepareToPlay(samplesPerBlockExpected, sampleRate);
      DBG("  Prepared: " + track->getName());
    }
  }
}

//==============================================================================
// Phase 2A: MIDI Input Handling
//==============================================================================

void Engine::enableMidiInput() {
  DBG("Engine: Enabling MIDI input...");

  // Get list of available MIDI input devices
  auto midiInputs = juce::MidiInput::getAvailableDevices();

  if (midiInputs.isEmpty()) {
    DBG("Engine: No MIDI input devices available");
    return;
  }

  // Open all available MIDI input devices (Omni mode)
  for (const auto &input : midiInputs) {
    DBG("Engine: Opening MIDI input: " + input.name);

    auto newInput = juce::MidiInput::openDevice(input.identifier, this);
    if (newInput != nullptr) {
      newInput->start();
      midiInputs_.push_back(std::move(newInput));
      DBG("Engine: MIDI input started: " + input.name);
    } else {
      DBG("Engine: Failed to open MIDI input: " + input.name);
    }
  }

  if (midiInputs_.empty()) {
    DBG("Engine: No MIDI inputs could be opened");
  }

  // Initialize MIDI recording buffers for all tracks
  {
    const juce::ScopedLock sl(midiRecordingLock_);
    midiRecording_.trackRecordings.resize(tracks_.size());
  }
}

void Engine::disableMidiInput() {
  if (!midiInputs_.empty()) {
    DBG("Engine: Stopping " + juce::String(midiInputs_.size()) +
        " MIDI inputs...");
    for (auto &input : midiInputs_) {
      if (input)
        input->stop();
    }
    midiInputs_.clear();
    DBG("Engine: MIDI inputs stopped");
  }
}

void Engine::handleIncomingMidiMessage(juce::MidiInput *source,
                                       const juce::MidiMessage &message) {
  juce::ignoreUnused(source);

// This runs on MIDI input thread (NOT audio thread or message thread)
// Buffer the message for processing in audio callback

// Debug log for MIDI activity
#if JUCE_DEBUG
  if (message.isNoteOn()) {
    DBG("MIDI In: Note On " + juce::String(message.getNoteNumber()) + " Vel " +
        juce::String(message.getVelocity()));
  }
#endif

  // Add message to FIFO (lock-free)
  midiFifo_.push(message);

  // LOCK-FREE MIDI RECORDING:
  // Instead of using a mutex, we use a lock-free FIFO to buffer recording
  // events. The message thread will drain this FIFO in stopRecording().
  if (isRecording_.load()) {
    // Get current playhead position
    const juce::int64 playhead = playheadSamples_.load();

    // Find all armed MIDI/Instrument tracks and queue recording events
    auto *snapshot = activeSnapshot_.load();
    if (snapshot != nullptr) {
      for (size_t i = 0; i < snapshot->tracks.size(); ++i) {
        auto *track = snapshot->tracks[i];
        if (track != nullptr && track->isArmed() &&
            (track->getType() == zenith::Track::Type::MIDI ||
             track->getType() == zenith::Track::Type::Instrument)) {
          // Queue event to lock-free FIFO
          int start1, size1, start2, size2;
          midiRecordFifo_.prepareToWrite(1, start1, size1, start2, size2);

          if (size1 > 0) {
            midiRecordBuffer_[start1] =
                MidiRecordEvent{message, static_cast<int>(i), playhead};
            midiRecordFifo_.finishedWrite(1);
          }
        }
      }
    }
  }
}

//==============================================================================

// Offline Export Implementation
//==============================================================================

void Engine::prepareBuffersForOfflineRender(int blockSize, int numChannels) {
  DBG("Engine: Preparing buffers for offline render - blockSize=" +
      juce::String(blockSize) + ", numChannels=" + juce::String(numChannels) +
      ", numTracks=" + juce::String(tracks_.size()));

  // Resize trackBuffers_ to match the number of tracks
  trackBuffers_.resize(tracks_.size());

  // Allocate each track buffer with the specified block size and channel
  // count
  for (size_t i = 0; i < trackBuffers_.size(); ++i) {
    trackBuffers_[i].setSize(numChannels, blockSize, false, true, false);
    trackBuffers_[i].clear();
  }

  // Resize Aux Bus buffers
  auxBusBuffers_.resize(auxBuses_.size());
  for (size_t i = 0; i < auxBusBuffers_.size(); ++i) {
    auxBusBuffers_[i].setSize(numChannels, blockSize, false, true, false);
    auxBusBuffers_[i].clear();
  }

  DBG("Engine: Buffers prepared successfully");
}

void Engine::renderAudioGraph(juce::AudioBuffer<float> &outputBuffer,
                              int numSamples, juce::int64 playheadPosition,
                              const juce::MidiBuffer *incomingMidi) {
  // 1. Clear Headers / Output
  outputBuffer.clear();

  // Clear all internal buffers (accumulators)
  for (auto &buf : trackBuffers_) {
    buf.clear();
  }
  for (auto &buf : auxBusBuffers_) {
    buf.clear();
  }

  // Get thread-safe snapshot (Lock-free load)
  auto *snapshot = activeSnapshot_.load();
  if (!snapshot)
    return;

  // 2. Iterate Topological Render Sequence
  for (const auto &node : snapshot->sequence) {

    // Determine Output Buffer
    juce::AudioBuffer<float> *destBuffer = nullptr;
    if (node.track) {
      if (node.outputBufferIndex >= 0 &&
          node.outputBufferIndex < (int)trackBuffers_.size()) {
        destBuffer = &trackBuffers_[node.outputBufferIndex];
      }
    } else if (node.bus) {
      if (node.outputBufferIndex >= 0 &&
          node.outputBufferIndex < (int)auxBusBuffers_.size()) {
        destBuffer = &auxBusBuffers_[node.outputBufferIndex];
      }
    }

    if (!destBuffer)
      continue;

    // 2a. Apply Modulation Inputs (Block-Rate Modulation)
    for (const auto &mod : node.modulationInputs) {
      if (node.track) {
        float value = mod.source.getValue(&globalLFOs_, &macroBank_,
                                          &persistentFollowers_);
        node.track->applyModulation(mod.targetPluginIndex, mod.targetParamIndex,
                                    value);
      }
    }

    // 3. Sum Inputs (Matrix Mixing)
    for (const auto &input : node.inputs) {
      if (input.isFeedback)
        continue;

      juce::AudioBuffer<float> *srcBuffer = nullptr;
      if (!input.isSourceAux) {
        if (input.sourceBufferIndex >= 0 &&
            input.sourceBufferIndex < (int)trackBuffers_.size()) {
          srcBuffer = &trackBuffers_[input.sourceBufferIndex];
        }
      } else {
        if (input.sourceBufferIndex >= 0 &&
            input.sourceBufferIndex < (int)auxBusBuffers_.size()) {
          srcBuffer = &auxBusBuffers_[input.sourceBufferIndex];
        }
      }

      if (srcBuffer) {
        for (int ch = 0; ch < destBuffer->getNumChannels(); ++ch) {
          int srcCh = ch % srcBuffer->getNumChannels();
          destBuffer->addFrom(ch, 0, *srcBuffer, srcCh, 0, numSamples,
                              input.gain);
        }
      }
    }

    // 4. Process Node (Generation + Effects)
    juce::AudioSourceChannelInfo bufferInfo(destBuffer, 0, numSamples);

    if (node.track) {
      // Compatibility: Pass empty aux buffers list as we handle summing
      // here
      std::vector<juce::AudioBuffer<float> *> compatibilityAux;

      // Pass tempo map if available
      const zenith::TempoMap *tMap = tempoMap_.get();

      node.track->getNextAudioBlock(bufferInfo, playheadPosition, incomingMidi,
                                    compatibilityAux, tMap);

      // 4b. Update Envelope Follower (Modulation Source)
      if (node.follower) {
        const float *input = destBuffer->getReadPointer(0);
        node.follower->process(input, numSamples);
      }

      // 5. Output to Master Device (Graph Routing)
      if (node.masterGain > 0.0f) {
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
          int srcCh = ch % destBuffer->getNumChannels();
          outputBuffer.addFrom(ch, 0, *destBuffer, srcCh, 0, numSamples,
                               node.masterGain);
        }
      } else if (node.track->getType() == zenith::Track::Type::Master) {
        // Fallback for Master track type
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
          int srcCh = ch % destBuffer->getNumChannels();
          outputBuffer.addFrom(ch, 0, *destBuffer, srcCh, 0, numSamples);
        }
      }
    } else if (node.bus) {
      node.bus->getNextAudioBlock(bufferInfo);

      // 4b. Update Envelope Follower for Bus (if any)
      if (node.follower) {
        const float *input = destBuffer->getReadPointer(0);
        node.follower->process(input, numSamples);
      }

      // 5. Output to Master (if routed)
      if (node.masterGain > 0.0f) {
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
          int srcCh = ch % destBuffer->getNumChannels();
          outputBuffer.addFrom(ch, 0, *destBuffer, srcCh, 0, numSamples,
                               node.masterGain);
        }
      }
    }
  }

  // Apply master bus effects on the final outputBuffer
  if (!masterPlugins_.empty()) {
    juce::MidiBuffer midi;
    for (auto &plugin : masterPlugins_) {
      if (plugin != nullptr && !plugin->isSuspended()) {
        plugin->processBlock(outputBuffer, midi);
      }
    }
  }
}

bool Engine::exportProjectToWav(const juce::File &outputFile, double sampleRate,
                                int bitDepth, double durationInSeconds) {
  DBG("Engine: Starting WAV export to " + outputFile.getFullPathName());
  DBG("  Sample Rate: " + juce::String(sampleRate) + " Hz");
  DBG("  Bit Depth: " + juce::String(bitDepth));
  DBG("  Duration: " + juce::String(durationInSeconds) + " seconds");

  // Validate parameters
  if (sampleRate <= 0.0) {
    DBG("Engine: Error - Invalid sample rate");
    return false;
  }

  if (bitDepth != 16 && bitDepth != 24 && bitDepth != 32) {
    DBG("Engine: Error - Invalid bit depth (must be 16, 24, or 32)");
    return false;
  }

  // Auto-detect duration from project content
  if (durationInSeconds <= 0.0) {
    // Scan all tracks for clip end times
    double maxEndTime = 0.0;
    for (const auto &track : tracks_) {
      if (track) {
        for (int i = 0; i < track->getNumClips(); ++i) {
          auto *clip = track->getClip(i);
          if (clip) {
            double rate = currentSampleRate.load() > 0
                              ? currentSampleRate.load()
                              : 44100.0;
            double clipEnd = static_cast<double>(clip->getStartPosition() +
                                                 clip->getLength()) /
                             rate;
            maxEndTime = std::max(maxEndTime, clipEnd);
          }
        }
      }
    }

    // Add 1 second buffer, minimum 10 seconds
    durationInSeconds = std::max(10.0, maxEndTime + 1.0);
    DBG("Engine: Auto-detected duration: " + juce::String(durationInSeconds) +
        " seconds");
  }

  // Calculate total samples
  const juce::int64 totalSamples =
      static_cast<juce::int64>(durationInSeconds * sampleRate);

  // Use 4096-sample blocks for efficient offline rendering
  // This is the block size mentioned in the bug report
  constexpr int offlineBlockSize = 4096;
  const int numChannels = 2; // Stereo output

  DBG("Engine: Using offline block size of " + juce::String(offlineBlockSize) +
      " samples");

  // CRITICAL: Prepare buffers for offline rendering BEFORE calling
  // renderBlock This fixes the bug where trackBuffers_ would be sized for
  // the audio device buffer (typically 512/1024) and all tracks would be
  // skipped when rendering 4096-sample blocks
  prepareBuffersForOfflineRender(offlineBlockSize, numChannels);

  // Create WAV file writer
  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::AudioFormatWriter> writer;

  writer.reset(wavFormat.createWriterFor(
      new juce::FileOutputStream(outputFile), sampleRate,
      static_cast<unsigned int>(numChannels), bitDepth, {}, // metadata
      0 // quality option (not used for WAV)
      ));

  if (writer == nullptr) {
    DBG("Engine: Error - Failed to create WAV writer");
    return false;
  }

  // Create render buffer
  juce::AudioBuffer<float> renderBuffer(numChannels, offlineBlockSize);

  // Render loop
  juce::int64 samplesRendered = 0;

  while (samplesRendered < totalSamples) {
    // Calculate how many samples to render in this block
    const int samplesToRender =
        static_cast<int>(juce::jmin(static_cast<juce::int64>(offlineBlockSize),
                                    totalSamples - samplesRendered));

    // Render this block
    // The renderAudioGraph() method will skip any track whose buffer is
    // too small But since we called prepareBuffersForOfflineRender() with
    // offlineBlockSize, all track buffers are >= offlineBlockSize, so no
    // tracks will be skipped
    renderAudioGraph(renderBuffer, samplesToRender, samplesRendered);

    // Write to file
    if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToRender)) {
      DBG("Engine: Error - Failed to write audio data");
      return false;
    }

    samplesRendered += samplesToRender;

    // Log progress every second
    if (samplesRendered % static_cast<juce::int64>(sampleRate) == 0) {
      double progress =
          static_cast<double>(samplesRendered) / totalSamples * 100.0;
      DBG("Engine: Export progress: " + juce::String(progress, 1) + "%");
    }
  }

  // Flush and close writer
  writer.reset();

  DBG("Engine: Export complete - " + juce::String(samplesRendered) +
      " samples written");
  return true;
}

//==============================================================================
// Phase 2D: Audio Recording (AUDIO THREAD)
//==============================================================================

void Engine::captureAudioInput(const float *const *inputChannelData,
                               int numInputChannels, int numSamples) noexcept {
  // ⚠️ AUDIO THREAD - MUST BE REAL-TIME SAFE!
  //
  // This function writes audio input to ThreadedWriter instances,
  // which use a lock-free FIFO. This is RT-safe.
  //
  // NO allocations, NO locks, NO system calls here!

  if (inputChannelData == nullptr || numInputChannels == 0)
    return;

  // Load track snapshot for safe access (Lock-free load)
  auto *snapshot = activeSnapshot_.load();
  // No lock needed!

  if (snapshot == nullptr)
    return;

  // Write to each active recording session
  for (auto &session : audioRecordingSessions_) {
    if (session.writer == nullptr)
      continue;

    // Get track input channel
    int inputChannel = 0; // Default to channel 0

    if (session.trackIndex >= 0 &&
        session.trackIndex < static_cast<int>(snapshot->tracks.size())) {
      auto *track = snapshot->tracks[session.trackIndex];
      if (track != nullptr) {
        inputChannel = track->getInputChannel();
      }
    }

    // Determine which input channel(s) to use for this session
    // If mono, use inputChannel. If stereo, use inputChannel and
    // inputChannel+1

    // For mono recording: write single channel
    if (session.numChannels == 1) {
      if (inputChannel < numInputChannels &&
          inputChannelData[inputChannel] != nullptr) {
        const float *channelData[1] = {inputChannelData[inputChannel]};
        session.writer->write(channelData, numSamples);
      }
    }
    // For stereo recording: write two channels
    else if (session.numChannels == 2) {
      int leftCh = inputChannel;
      int rightCh = inputChannel + 1;

      // Handle edge case where right channel is out of bounds
      // If input is mono (1 channel), duplicate it? Or silence?
      // If we have at least 2 input channels, try to map.

      const float *leftData =
          (leftCh < numInputChannels) ? inputChannelData[leftCh] : nullptr;
      const float *rightData =
          (rightCh < numInputChannels) ? inputChannelData[rightCh] : nullptr;

      // If right channel missing but left exists, maybe duplicate left?
      // For now, let's just use what we have, passing nullptr for missing
      // channels (writer handles it?) Actually ThreadedWriter::write
      // expects valid pointers.

      if (leftData && rightData) {
        const float *channelData[2] = {leftData, rightData};
        session.writer->write(channelData, numSamples);
      } else if (leftData) {
        // Duplicate mono to stereo
        const float *channelData[2] = {leftData, leftData};
        session.writer->write(channelData, numSamples);
      }
    }
  }
}

//==============================================================================
// Phase 2D: Audio Recording Helpers (MESSAGE THREAD)
//==============================================================================

void Engine::bakeAudioRecordingIntoTrack(zenith::Track &track,
                                         const juce::File &file,
                                         juce::int64 recordingStartSamples,
                                         double sampleRate) {
  DBG("Engine: Baking audio recording into track '" + track.getName() + "'");
  DBG("  File: " + file.getFullPathName());
  DBG("  Start: " + juce::String(recordingStartSamples) + " samples");

  if (!file.existsAsFile()) {
    DBG("Engine: Recording file does not exist!");
    return;
  }

  // Load file into AudioFilePool
  auto fileHandle = audioFilePool_->loadFile(file);

  if (fileHandle == nullptr || !fileHandle->isValid()) {
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

  // Sync with ProjectState if available
  if (projectState_ != nullptr) {
    // Find track ID
    juce::String trackId = track.getTrackId();
    if (trackId.isNotEmpty()) {
      // Create clip in ProjectState
      // Convert samples to beats
      double tempo = projectState_->getTempo();
      double startBeats = 0.0;
      double lengthBeats = 4.0; // Default

      if (tempoMap_) {
        startBeats =
            tempoMap_->samplesToBeats(recordingStartSamples, sampleRate);
        lengthBeats =
            tempoMap_->samplesToBeats(fileHandle->lengthInSamples, sampleRate);
      } else {
        // Fallback
        double secondsPerBeat = 60.0 / tempo;
        startBeats = (recordingStartSamples / sampleRate) / secondsPerBeat;
        lengthBeats =
            (fileHandle->lengthInSamples / sampleRate) / secondsPerBeat;
      }

      juce::String clipName = file.getFileNameWithoutExtension();

      // Create clip in ProjectState
      juce::String clipId = projectState_->createClip(
          trackId, "audio", recordingStartSamples, fileHandle->lengthInSamples,
          clipName, "Record Audio");

      if (clipId.isNotEmpty()) {
        projectState_->setClipAudioFile(trackId, clipId, file,
                                        "Record Audio File");
        DBG("Engine: Synced recording to ProjectState: " + clipId);
      }
    }
  }

  DBG("Engine: Audio clip created successfully");
  DBG("  Length: " + juce::String(fileHandle->lengthInSamples) + " samples (" +
      juce::String(fileHandle->lengthInSamples / sampleRate, 2) + " seconds)");
}

//==============================================================================
// Phase 2C: MIDI Recording Baking (MESSAGE THREAD)
//==============================================================================

void Engine::bakeMidiRecordingsIntoClips(bool quantize) {
  DBG("Engine: Baking MIDI recordings into clips (quantize=" +
      juce::String(quantize ? "true" : "false") + ")");

  // First, drain all events from the lock-free FIFO into recording
  // buffers
  drainMidiRecordFifo();

  // Get project tempo for quantization (default to 120 BPM if no project
  // state)
  const double tempo =
      (projectState_ != nullptr) ? projectState_->getTempo() : 120.0;

  // No lock needed - drainMidiRecordFifo already populated
  // midiRecording_.trackRecordings
  const juce::int64 recordStart = midiRecording_.recordingStartSamples;

  // Process each track's recording
  for (size_t i = 0;
       i < midiRecording_.trackRecordings.size() && i < tracks_.size(); ++i) {
    auto &recording = midiRecording_.trackRecordings[i];

    // Skip empty recordings
    if (recording.getNumEvents() == 0)
      continue;

    // Quantize if requested
    juce::MidiMessageSequence finalSequence = recording;
    if (quantize) {
      finalSequence =
          quantizeMidiSequence(recording, tempo, 0.25); // 1/16 note grid
    }

    // Ensure note-off events are properly matched
    finalSequence.updateMatchedPairs();

    // Calculate clip length from sequence end time
    const double endTimeSeconds = finalSequence.getEndTime();
    const juce::int64 clipLengthSamples =
        static_cast<juce::int64>(endTimeSeconds * currentSampleRate.load());

    // Create MIDI clip
    auto clip = std::make_unique<zenith::Track::Clip>();
    clip->setType(zenith::Track::Clip::Type::MIDI);
    clip->setName("MIDI Recording");
    clip->setMidiSequence(finalSequence);
    clip->setStartPosition(recordStart);
    clip->setLength(clipLengthSamples);

    // Add clip to track
    auto *track = tracks_[i].get();
    if (track != nullptr) {
      track->addClip(std::move(clip));
      DBG("Engine: Created MIDI clip on track " + juce::String(i) +
          " (start=" + juce::String(recordStart) +
          ", length=" + juce::String(clipLengthSamples) +
          ", events=" + juce::String(finalSequence.getNumEvents()) + ")");
    }
  }

  DBG("Engine: Baking complete");
}

void Engine::clearMidiRecordings() {
  // MESSAGE THREAD ONLY - no lock needed with lock-free FIFO pattern
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  for (auto &recording : midiRecording_.trackRecordings) {
    recording.clear();
  }

  midiRecording_.recordingStartSamples = 0;

  // Also clear the FIFO
  int numReady = midiRecordFifo_.getNumReady();
  if (numReady > 0) {
    int start1, size1, start2, size2;
    midiRecordFifo_.prepareToRead(numReady, start1, size1, start2, size2);
    midiRecordFifo_.finishedRead(numReady);
  }

  DBG("Engine: MIDI recordings cleared");
}

juce::MidiMessageSequence
Engine::quantizeMidiSequence(const juce::MidiMessageSequence &input,
                             double tempo, double quantizeGrid) {
  // Calculate grid spacing in seconds
  const double beatsPerSecond = tempo / 60.0;
  const double quarterNoteSeconds = 1.0 / beatsPerSecond;
  const double gridSeconds = quarterNoteSeconds * quantizeGrid;

  juce::MidiMessageSequence output;

  // Quantize each event
  for (int i = 0; i < input.getNumEvents(); ++i) {
    auto *event = input.getEventPointer(i);
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

juce::AudioPluginFormatManager &Engine::getPluginFormatManager() {
  return pluginHost_->getFormatManager();
}

void Engine::prepareRecordingForTrack(int trackIndex) {
  // Launch async task to prepare recording file
  juce::Thread::launch([this, trackIndex]() {
    DBG("Engine: Preparing recording for track " + juce::String(trackIndex) +
        "...");

    if (audioFilePool_ == nullptr || audioWriterThread_ == nullptr)
      return;

    // Get audio device info
    auto *device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
      return;

    double sampleRate = device->getCurrentSampleRate();
    auto activeInputChannels = device->getActiveInputChannels();
    int numChannels =
        activeInputChannels.countNumberOfSetBits(); // Total inputs
    // Note: We might want per-track channel count, but for now use total
    // inputs or stereo default ThreadedWriter needs to know how many
    // channels it accepts. processAudioRecording writes 2 channels max
    // usually.
    numChannels =
        2; // Force stereo for now to match processAudioRecording logic

    // Create recordings directory (need to do this here too)
    juce::File recordingsDir;
    if (projectState_ != nullptr &&
        projectState_->getProjectFile().existsAsFile()) {
      recordingsDir =
          projectState_->getProjectFile().getSiblingFile("Audio Files");
    } else {
      recordingsDir =
          juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
              .getChildFile("ZenithDAW/Recordings");
    }

    if (!recordingsDir.exists()) {
      recordingsDir.createDirectory();
    }

    // Generate filename
    juce::String trackName =
        "Track_" + juce::String(trackIndex); // Fallback name since we can't
                                             // access tracks_ safely
    juce::String timestamp =
        juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    juce::String filename = trackName + "_" + timestamp + ".wav";
    juce::File recordFile = recordingsDir.getChildFile(filename);

    // Create writer
    auto fileStream = std::make_unique<juce::FileOutputStream>(recordFile);
    if (fileStream->failedToOpen()) {
      DBG("Engine: Failed to prepare recording file");
      return;
    }

    juce::AudioFormatManager &formatManager =
        audioFilePool_->getFormatManager();
    auto *wavFormat = formatManager.findFormatForFileExtension("wav");

    if (wavFormat == nullptr)
      return;

    auto *writer = wavFormat->createWriterFor(fileStream.get(), sampleRate,
                                              numChannels, 24, {}, 0);
    if (writer == nullptr)
      return;

    fileStream.release();

    auto threadedWriter =
        std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            writer, *audioWriterThread_, 32768);

    AudioRecordingSession session;
    session.writer = std::move(threadedWriter);
    session.file = recordFile;
    session.numChannels = numChannels;
    session.sampleRate = sampleRate;
    session.trackIndex = trackIndex;
    // We can't set inputChannelIndex safely here, will be set in record()

    {
      const juce::ScopedLock sl(preppedSessionsLock_);
      // Remove old prep for this track
      preppedSessions_.erase(std::remove_if(preppedSessions_.begin(),
                                            preppedSessions_.end(),
                                            [trackIndex](const auto &s) {
                                              return s.trackIndex == trackIndex;
                                            }),
                             preppedSessions_.end());

      preppedSessions_.push_back(std::move(session));
    }

    DBG("Engine: Recording prepared for track " + juce::String(trackIndex));
  });
}

void Engine::registerFormats() {
  formatManager.registerBasicFormats();
  formatManager.registerFormat(new juce::FlacAudioFormat(), false);
  formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
}

bool Engine::exportProject(const ExportOptions &options) {
  DBG("Engine: Starting Advanced Export...");
  registerFormats();

  if (options.sampleRate <= 0)
    return false;

  if (options.bitDepth == 8 && options.format != ExportFormat::WAV) {
    DBG("Engine: 8-bit export is only supported for WAV format.");
    return false;
  }

  juce::AudioFormat *format = nullptr;
  switch (options.format) {
  case ExportFormat::WAV:
    format = formatManager.findFormatForFileExtension("wav");
    break;
  case ExportFormat::FLAC:
    format = formatManager.findFormatForFileExtension("flac");
    break;
  case ExportFormat::OGG:
    format = formatManager.findFormatForFileExtension("ogg");
    break;
  }

  if (!format)
    return false;

  // Create file stream
  auto fileStream =
      std::make_unique<juce::FileOutputStream>(options.outputFile);
  if (fileStream->failedToOpen())
    return false;

  std::unique_ptr<juce::AudioFormatWriter> writer(
      format->createWriterFor(fileStream.release(), options.sampleRate,
                              2,                    // Stereo
                              options.bitDepth, {}, // Metadata
                              0                     // Quality
                              ));

  if (!writer)
    return false;

  const int blockSize = 4096;
  juce::AudioBuffer<float> renderBuffer(2, blockSize);
  prepareBuffersForOfflineRender(blockSize, 2);

  if (options.enableDither)
    dither.prepare(2);

  // Use auto-detect duration if not specified
  double duration =
      options.duration > 0 ? options.duration : autoDetectProjectDuration();
  DBG("Engine: Export duration: " + juce::String(duration, 2) + " seconds");
  juce::int64 totalSamples =
      static_cast<juce::int64>(options.sampleRate * duration);
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples) {
    int numSamples =
        (int)juce::jmin((juce::int64)blockSize, totalSamples - samplesWritten);

    // Render Mix
    // Note: renderAudioGraph is the private method for rendering
    renderAudioGraph(renderBuffer, numSamples, samplesWritten, nullptr);

    // Apply Dithering
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(renderBuffer, options.bitDepth);
    }

    // Normalization (Simple Peak Limiter for now if enabled)
    if (options.normalize) {
      applyNormalization(renderBuffer, 1.0f, (float)options.normalizeDb);
    }

    if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples)) {
      return false;
    }

    samplesWritten += numSamples;
  }

  return true;
}

void Engine::applyNormalization(juce::AudioBuffer<float> &buffer, float maxPeak,
                                float targetDb) {
  juce::ignoreUnused(maxPeak);
  float targetLinear = juce::Decibels::decibelsToGain(targetDb);
  float blockPeak = buffer.getMagnitude(0, buffer.getNumSamples());
  if (blockPeak > targetLinear) {
    float gain = targetLinear / blockPeak;
    buffer.applyGain(gain);
  }
}

//==============================================================================
// Plugin Delay Compensation (PDC)
//==============================================================================

int Engine::getTrackLatency(int trackIndex) const {
  if (trackIndex >= 0 &&
      trackIndex < static_cast<int>(trackLatencies_.size())) {
    return trackLatencies_[trackIndex];
  }
  return 0;
}

int Engine::getMasterLatency() const { return masterLatency_; }

void Engine::recalculatePDC() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  DBG("Engine: Recalculating PDC...");

  // Calculate per-track latency
  trackLatencies_.resize(tracks_.size());
  int maxLatency = 0;

  for (size_t i = 0; i < tracks_.size(); ++i) {
    auto *track = tracks_[i].get();
    if (track == nullptr) {
      trackLatencies_[i] = 0;
      continue;
    }

    // Sum latency from all plugins in the track
    int trackLatency = 0;
    for (int p = 0; p < track->getNumPlugins(); ++p) {
      auto *plugin = track->getPlugin(p);
      if (plugin != nullptr) {
        trackLatency += plugin->getLatencySamples();
      }
    }

    trackLatencies_[i] = trackLatency;
    if (trackLatency > maxLatency) {
      maxLatency = trackLatency;
    }
  }

  // Calculate master bus latency
  masterLatency_ = 0;
  for (const auto &plugin : masterPlugins_) {
    if (plugin != nullptr) {
      masterLatency_ += plugin->getLatencySamples();
    }
  }

  maxTrackLatency_.store(maxLatency);

  DBG("Engine: PDC calculated - max track latency: " +
      juce::String(maxLatency) +
      " samples, master latency: " + juce::String(masterLatency_) + " samples");

  // Allocate/resize PDC delay buffers if PDC is enabled
  if (pdcEnabled_.load() && maxLatency > 0) {
    pdcDelayBuffers_.resize(tracks_.size());
    pdcDelayWritePos_.resize(tracks_.size(), 0);

    for (size_t i = 0; i < tracks_.size(); ++i) {
      // Calculate delay needed for this track (max - track's own latency)
      int delayNeeded = maxLatency - trackLatencies_[i];

      if (delayNeeded > 0) {
        // Allocate circular buffer for delay
        pdcDelayBuffers_[i].setSize(2, delayNeeded + currentBufferSize.load(),
                                    false, true, false);
        pdcDelayBuffers_[i].clear();
        pdcDelayWritePos_[i] = 0;
      } else {
        pdcDelayBuffers_[i].setSize(0, 0); // No delay needed
      }
    }
  }
}

void Engine::setPDCEnabled(bool enabled) {
  bool wasEnabled = pdcEnabled_.exchange(enabled);
  if (wasEnabled != enabled) {
    if (enabled) {
      recalculatePDC();
    }
    DBG("Engine: PDC " + juce::String(enabled ? "enabled" : "disabled"));
  }
}

void Engine::applyPDCDelay(juce::AudioBuffer<float> &buffer, int trackIndex,
                           int delaySamples) noexcept {
  // Apply circular buffer delay for PDC
  if (delaySamples <= 0 || trackIndex < 0 ||
      trackIndex >= static_cast<int>(pdcDelayBuffers_.size()))
    return;

  auto &delayBuffer = pdcDelayBuffers_[trackIndex];
  if (delayBuffer.getNumSamples() == 0)
    return;

  const int numSamples = buffer.getNumSamples();
  const int numChannels =
      juce::jmin(buffer.getNumChannels(), delayBuffer.getNumChannels());
  const int delayBufferSize = delayBuffer.getNumSamples();

  int writePos = pdcDelayWritePos_[trackIndex];

  for (int ch = 0; ch < numChannels; ++ch) {
    const float *src = buffer.getReadPointer(ch);
    float *dst = buffer.getWritePointer(ch);
    float *delayData = delayBuffer.getWritePointer(ch);

    int localWritePos = writePos;

    for (int i = 0; i < numSamples; ++i) {
      // Read from delay buffer
      int readPos =
          (localWritePos - delaySamples + delayBufferSize) % delayBufferSize;
      float delayedSample = delayData[readPos];

      // Write current sample to delay buffer
      delayData[localWritePos] = src[i];

      // Output delayed sample
      dst[i] = delayedSample;

      localWritePos = (localWritePos + 1) % delayBufferSize;
    }
  }

  pdcDelayWritePos_[trackIndex] = (writePos + numSamples) % delayBufferSize;
}

//==============================================================================
// Auto-Detect Export Duration
//==============================================================================

double Engine::autoDetectProjectDuration() const {
  double maxDuration = 0.0;
  const double sampleRate = currentSampleRate.load();

  if (sampleRate <= 0.0)
    return 10.0; // Fallback

  // Scan all tracks for the latest clip end position
  for (const auto &track : tracks_) {
    if (track == nullptr)
      continue;

    for (int i = 0; i < track->getNumClips(); ++i) {
      const auto *clip = track->getClip(i);
      if (clip != nullptr) {
        // Get clip end position in samples and convert to seconds
        juce::int64 clipEnd = clip->getStartPosition() + clip->getLength();
        double endSeconds = static_cast<double>(clipEnd) / sampleRate;

        if (endSeconds > maxDuration) {
          maxDuration = endSeconds;
        }
      }
    }
  }

  // Add a small tail (2 seconds) for reverb/delay tails
  if (maxDuration > 0.0) {
    maxDuration += 2.0;
  } else {
    maxDuration = 10.0; // Default if no clips
  }

  return maxDuration;
}

//==============================================================================
// Lock-Free MIDI Recording Drain (message thread)
//==============================================================================

void Engine::drainMidiRecordFifo() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Ensure recording buffers are sized correctly
  if (midiRecording_.trackRecordings.size() != tracks_.size()) {
    midiRecording_.trackRecordings.resize(tracks_.size());
  }

  const juce::int64 recordStart = midiRecording_.recordingStartSamples;
  const double sampleRate = currentSampleRate.load();

  // Drain all events from the lock-free FIFO
  int numReady = midiRecordFifo_.getNumReady();
  int start1, size1, start2, size2;
  midiRecordFifo_.prepareToRead(numReady, start1, size1, start2, size2);

  // Process first segment
  for (int i = 0; i < size1; ++i) {
    const auto &event = midiRecordBuffer_[start1 + i];

    if (event.trackIndex >= 0 &&
        event.trackIndex <
            static_cast<int>(midiRecording_.trackRecordings.size())) {
      // Calculate position relative to recording start
      double positionInSeconds =
          static_cast<double>(event.timestampSamples - recordStart) /
          sampleRate;

      juce::MidiMessage timestampedMsg(event.message);
      timestampedMsg.setTimeStamp(positionInSeconds);

      midiRecording_.trackRecordings[event.trackIndex].addEvent(timestampedMsg);
    }
  }

  // Process second segment (wrap-around)
  for (int i = 0; i < size2; ++i) {
    const auto &event = midiRecordBuffer_[start2 + i];

    if (event.trackIndex >= 0 &&
        event.trackIndex <
            static_cast<int>(midiRecording_.trackRecordings.size())) {
      double positionInSeconds =
          static_cast<double>(event.timestampSamples - recordStart) /
          sampleRate;

      juce::MidiMessage timestampedMsg(event.message);
      timestampedMsg.setTimeStamp(positionInSeconds);

      midiRecording_.trackRecordings[event.trackIndex].addEvent(timestampedMsg);
    }
  }

  midiRecordFifo_.finishedRead(numReady);
}

void Engine::updateSoloState() {
  bool anySolo = false;
  for (const auto &track : tracks_) {
    if (track && track->isSolo()) {
      anySolo = true;
      break;
    }
  }

  for (const auto &track : tracks_) {
    if (track) {
      if (anySolo) {
        // If any track is soloed, mute this track unless it is also
        // soloed
        track->setSilencedBySolo(!track->isSolo());
      } else {
        // No solo active, unmute everyone (from solo perspective)
        track->setSilencedBySolo(false);
      }
    }
  }
}

//==============================================================================
// Aux Bus Management
//==============================================================================

int Engine::createAuxBus(const juce::String &name) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Create new bus
  auto bus = std::make_shared<zenith::AuxBus>(name);

  // Initialize if engine is running
  if (currentSampleRate.load() > 0) {
    bus->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
  }

  auxBuses_.push_back(bus);
  int index = static_cast<int>(auxBuses_.size()) - 1;

  // Set ID (Use monotonic counter)
  juce::String id = "aux_" + juce::String(auxBusIdCounter.fetch_add(1));
  bus->setId(id);

  // Register with RoutingGraph
  RoutingGraph::Node node;
  node.id = id;
  node.name = name;
  node.type = RoutingGraph::NodeType::Bus;
  routingGraph_.addNode(node);

  // Ensure buffer exists
  if (index >= static_cast<int>(auxBusBuffers_.size())) {
    auxBusBuffers_.resize(index + 1);
    auxBusBuffers_[index].setSize(2, currentBufferSize.load());
    auxBusBuffers_[index].clear();
  }

  updateTrackSnapshot();

  DBG("Engine: Created Aux Bus '" + name + "' (ID: " + node.id + ")");
  return index;
}

void Engine::removeAuxBus(int auxIndex) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size())) {
    auto bus = auxBuses_[auxIndex];
    if (bus) {
      juce::String id = bus->getId();

      // Remove from RoutingGraph
      routingGraph_.removeNode(id);

      bus->releaseResources();
    }

    auxBuses_.erase(auxBuses_.begin() + auxIndex);

    updateTrackSnapshot();
  }
}

int Engine::getNumAuxBuses() const noexcept {
  return static_cast<int>(auxBuses_.size());
}
AuxBus *Engine::getAuxBus(int auxIndex) noexcept {
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
    return auxBuses_[auxIndex].get();
  return nullptr;
}

float Engine::getAuxBusLevel(int auxIndex) const {
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
    return auxBuses_[auxIndex]->getCurrentLevel();
  return 0.0f;
}

float Engine::getAuxBusPeakLevel(int auxIndex) const {
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
    return auxBuses_[auxIndex]->getPeakLevel();
  return 0.0f;
}

} // namespace zenith
