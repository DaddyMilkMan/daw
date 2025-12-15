/**
 * @file Engine.cpp
 * @brief Audio engine implementation
 */

#include "Engine.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include "TrackAutomationSynchronizer.h"
#include <algorithm> // For std::remove_if
#include <array>     // For RT-safe stack allocation in audio callback


// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "../ai/SessionDebuggerAgent.h"
#include "../engine/AudioFilePool.h"
#include "../engine/AuxBus.h"
#include "../engine/Clip.h"
#include "../engine/EngineConstants.h"
#include "../engine/MixerChannel.h"
#include "../engine/PluginHost.h"
#include "../engine/Track.h"
#include "../engine/TrackFreeze.h"
#include "../instruments/InstrumentRegistry.h"
#include "../instruments/RegisterBuiltInInstruments.h"
#include "PluginEditorWindow.h"

// Refactor 2025-12-09: Modular Components
#include "../engine/AudioRenderer.h"
#include "../engine/RecordingManager.h"
#include "../engine/TransportController.h"

//==============================================================================
namespace zenith {

Engine::Engine() {
  DBG("Engine: Constructor");

  // Initialize Modular Components
  audioRenderer_ = std::make_unique<AudioRenderer>();
  recordingManager_ = std::make_unique<RecordingManager>();
  transportController_ = std::make_unique<TransportController>();
  DBG("Engine: Modular components initialized");

  // Initialize audio file pool for sample caching
  audioFilePool_ = std::make_unique<zenith::AudioFilePool>();
  DBG("Engine: AudioFilePool created");

  // Initialize plugin host and editor window manager
  pluginHost_ = std::make_unique<zenith::PluginHost>();
  pluginHost_
      ->scanDefaultLocations(); // Load cached plugins, check for crash recovery
  DBG("Engine: PluginHost initialized with " +
      juce::String(pluginHost_->getKnownPlugins().getNumTypes()) +
      " cached plugins");
  pluginEditorWindowManager_ =
      std::make_unique<zenith::PluginEditorWindowManager>();

  // Initialize Instrument Registry (built-in synths, samplers, etc.)
  instrumentRegistry_ = std::make_unique<zenith::InstrumentRegistry>();
  zenith::registerBuiltInInstruments(
      *instrumentRegistry_); // Register factories
  DBG("Engine: InstrumentRegistry initialized");

  // Initialize Session Debugger Agent (AI Technical Integrity)
  sessionDebugger_ = std::make_unique<ai::SessionDebuggerAgent>(*this);
  DBG("Engine: SessionDebuggerAgent initialized");

  // Initialize Analysis FIFO (for visualizers like spectrum analyzer)
  analysisFifo_ = std::make_unique<zenith::StereoAudioFifo>(16384);
  DBG("Engine: Analysis FIFO initialized");

  // Initialize tempo map for beat/time conversions
  tempoMap_ = std::make_unique<zenith::TempoMap>();
  DBG("Engine: TempoMap initialized");

  // Wire up transport controller to tempo map
  transportController_->setTempoMap(tempoMap_.get());

  // Initialize TrackFreezeManager for CPU optimization
  freezeManager_ = std::make_unique<TrackFreezeManager>();
  DBG("Engine: TrackFreezeManager initialized");

  // Register Master Bus node in RoutingGraph
  // This is the final destination for all audio before output
  RoutingGraph::Node masterNode;
  masterNode.id = "master";
  masterNode.name = "Master";
  masterNode.type = RoutingGraph::NodeType::Master;
  masterNode.channelCount = 2;
  routingGraph_.addNode(masterNode);
  DBG("Engine: Master bus registered in RoutingGraph");

  // Initialize track snapshot
  updateTrackSnapshot();
}

Engine::~Engine() {
  DBG("Engine: Destructor");

  // Set shutdown flag to prevent async callbacks during destruction
  isShuttingDown_.store(true);

  // Disable MIDI input before shutdown
  disableMidiInput();

  shutdown();

  // Explicitly reset managers to ensure orderly shutdown
  audioRenderer_.reset();
  recordingManager_.reset();
  transportController_.reset();

  // Clear audio file pool
  if (audioFilePool_ != nullptr) {
    audioFilePool_.reset();
  }

  DBG("Engine: Cleanup complete");
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

  // Wire up recording manager with project state
  if (recordingManager_) {
    recordingManager_->setProjectState(projectState_);
    DBG("Engine: RecordingManager wired to ProjectState");
  }

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

    // Register with RoutingGraph and connect to master bus
    RoutingGraph::Node node;
    node.id = tracks_.back()->getTrackId();
    node.name = tracks_.back()->getName();
    node.type = RoutingGraph::NodeType::Track;
    routingGraph_.addNode(node);

    // Automatically route track to master bus
    routingGraph_.connect(tracks_.back()->getTrackId(), "master", 1.0f);
  }

  DBG("Engine: Synced " + juce::String(tracks_.size()) + " tracks");

  // Update snapshot for audio thread
  updateTrackSnapshot();
}

bool Engine::initialize() {
  DBG("Engine: Initializing...");

  // Initialize audio device manager
  auto error = deviceManager.initialiseWithDefaultDevices(2, 2); // 2 in, 2 out

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

  // Wire up recording manager with device manager for input channel info
  if (recordingManager_) {
    recordingManager_->setDeviceManager(&deviceManager);
    recordingManager_->prepare(setup.sampleRate);
    DBG("Engine: RecordingManager wired to DeviceManager");
  }

  // Enable MIDI input devices
  enableMidiInput();

  // Optional debug seed (disabled by default; enable with
  // -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON)
#if defined(JUCE_DEBUG) && defined(ZENITH_ENGINE_SEED_DEBUG_TRACKS)
  DBG("Engine: Seeding debug tracks (ZENITH_ENGINE_SEED_DEBUG_TRACKS enabled)");
  addTestTracks(8);
#endif

  // Start Session Debugger monitoring (AI Technical Integrity Agent)
  if (sessionDebugger_) {
    sessionDebugger_->startMonitoring(500); // Analyze every 500ms
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

//==============================================================================
// Transport Controls
//==============================================================================

void Engine::play() {
  DBG("Engine: Play");

  // Handle loop region - reset to loop start if past loop end
  // If playhead is at or past loop end, reset to loop start or 0
  const juce::int64 loopEnd = transportController_->getLoopEndSamples();
  const juce::int64 loopStart = transportController_->getLoopStartSamples();
  const juce::int64 currentPos = transportController_->getPlayheadSamples();

  if (loopEnd > 0 && currentPos >= loopEnd) {
    transportController_->setPlayheadSamples(loopStart);
  }

  // Enable test tone for Phase 0 fallback (when no tracks)
  enableTestTone_.store(true);

  transportController_->play();

  // Start automation synchronizer for parameter recording/playback
  if (automationSynchronizer) {
    automationSynchronizer->start(60); // 60 Hz update rate
    DBG("Engine: Started automation synchronizer");
  }
}

void Engine::stop() {
  DBG("Engine: Stop");

  if (transportController_) {
    transportController_->stop();
  }

  // Stop recording and bake recordings into clips
  if (isRecording()) {
    stopRecording();
  }

  enableTestTone_.store(false);

  // Stop automation synchronizer
  if (automationSynchronizer) {
    automationSynchronizer->stop();
    DBG("Engine: Stopped automation synchronizer");
  }
}

bool Engine::isPlaying() const {
  return transportController_ ? transportController_->isPlaying() : false;
}

bool Engine::isRecording() const {
  return recordingManager_ ? recordingManager_->isRecording() : false;
}

juce::int64 Engine::getPlayheadSamples() const {
  return transportController_ ? transportController_->getPlayheadSamples() : 0;
}

juce::int64 Engine::getPlaybackPosition() const {
  return transportController_ ? transportController_->getPlayheadSamples() : 0;
}

double Engine::getPlaybackPositionBeats() const {
  if (transportController_) {
    return transportController_->getPlayheadBeats();
  }
  return 0.0;
}

//==============================================================================
// MIDI and Audio Recording
//==============================================================================

void Engine::record() {
  DBG("Engine: Record");

  if (!transportController_->isPlaying()) {
    play();
  }

  // Create recordings directory
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

  // Set recording directory
  recordingManager_->setRecordingDirectory(recordingsDir);

  // Start recording on managed sessions
  recordingManager_->startRecording(transportController_->getPlayheadSamples(),
                                    tracks_);
  DBG("Engine: Recording started (Delegated)");
}

void Engine::stopRecording() {
  DBG("Engine: Stop recording");
  recordingManager_->stopRecording(tracks_);
}

void Engine::toggleRecording() {
  if (recordingManager_) {
    if (recordingManager_->isRecording()) {
      stopRecording();
    } else {
      record();
    }
  }
}

//==============================================================================
// Transport Position & Looping
//==============================================================================

void Engine::setPlayheadSamples(juce::int64 position) {
  if (transportController_) {
    transportController_->setPlayheadSamples(position);
  }
}

void Engine::setLooping(bool shouldLoop) {
  if (transportController_) {
    transportController_->setLooping(shouldLoop);
  }
}

void Engine::setLoopRegion(juce::int64 start, juce::int64 end) {
  if (transportController_) {
    transportController_->setLoopRegionSamples(start, end);
  }
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
// Track Management
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
    // Use shared_ptr to allow track references to outlive snapshot updates
    auto track = std::make_shared<zenith::Track>(
        "Track " + juce::String(tracks_.size() + 1),
        zenith::Track::Type::Audio);

    // Prepare track for audio processing if engine is already running
    if (currentSampleRate.load() > 0) {
      track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
    }

    tracks_.push_back(track); // No std::move needed for shared_ptr

    // Register track with RoutingGraph and connect to master bus
    RoutingGraph::Node node;
    node.id = track->getTrackId();
    node.name = track->getName();
    node.type = RoutingGraph::NodeType::Track;
    routingGraph_.addNode(node);

    // Automatically route track to master bus
    routingGraph_.connect(track->getTrackId(), "master", 1.0f);
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
// Audio File Pool
//==============================================================================

zenith::AudioFilePool &Engine::getAudioFilePool() {
  jassert(audioFilePool_ != nullptr);
  return *audioFilePool_;
}

//==============================================================================
// Plugin Hosting
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
// Mixer Control (MESSAGE THREAD ONLY)
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

    // Prepare recording asynchronously when armed for latency-free recording
    // start
    if (armed && recordingManager_) {
      // Determine directory
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
      if (!recordingsDir.exists())
        recordingsDir.createDirectory();

      // recordingManager_->prepareRecordingForTrack(*tracks_[trackIndex],
      //                                             trackIndex, recordingsDir);
    }
  }
}

//==============================================================================
// Metering (MESSAGE THREAD SAFE)
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

float Engine::getMasterLevel() const {
  return audioRenderer_ ? audioRenderer_->getMasterLevel() : 0.0f;
}

float Engine::getMasterPeakLevel() const {
  return audioRenderer_ ? audioRenderer_->getMasterPeakLevel() : 0.0f;
}

void Engine::resetPeakMeters() {
  // Reset master peak
  if (audioRenderer_)
    audioRenderer_->resetPeakMeters();

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
    // Use shared_ptr to allow track references to outlive snapshot updates
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

// Accept shared_ptr for RT-safe snapshot sharing across threads
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

  // Automatically route regular tracks to master bus
  // Bus and Master tracks handle their own routing
  if (track->getType() != Track::Type::Bus &&
      track->getType() != Track::Type::Master) {
    routingGraph_.connect(id, "master", 1.0f);
  }

  DBG("Engine: Added track '" + name + "' (ID: " + id + ")");

  // Update Solo State (new track might need to be silenced if others are
  // soloed)
  updateSoloState();

  // Update snapshot for audio thread
  // Snapshot holds shared_ptr, extending track lifetime across thread
  // boundaries
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

void Engine::updateTrackSnapshot() {
  // Create new snapshot
  // Include Aux Buses in snapshot for consistent audio thread access
  auto newSnapshot = std::make_shared<TrackSnapshot>(tracks_, auxBuses_);

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
  if (transportController_) {
    transportController_->setPlayheadSamples(0);
  }

  // Phase 1: Prepare tracks for audio processing
  prepareTracks(device->getCurrentBufferSizeSamples(),
                device->getCurrentSampleRate());

  // Prepare Aux Buses
  for (auto &bus : auxBuses_) {
    if (bus) {
      bus->prepareToPlay(device->getCurrentBufferSizeSamples(),
                         currentSampleRate.load());
    }
  }

  // Prepare AudioRenderer (Handles buffers, PDC, metering, limiter)
  if (audioRenderer_) {
    audioRenderer_->prepare(currentSampleRate.load(), currentBufferSize.load(),
                            tracks_.size(), auxBuses_.size());
  }

  // Prepare RecordingManager
  if (recordingManager_) {
    recordingManager_->prepare(currentSampleRate.load());
  }

  // Note: MasterLimiter is now managed by AudioRenderer

  DBG("Engine: Audio device started");
  DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
  DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
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

  juce::ignoreUnused(inputChannelData, numInputChannels, context);

  // Clean output buffers
  for (int i = 0; i < numOutputChannels; ++i) {
    if (outputChannelData[i]) {
      juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }
  }

  // Load snapshot for RT-safe access
  auto *snapshot = activeSnapshot_.load();
  if (!snapshot)
    return;

  // Process Events (Updates track parameters etc.)
  processEvents();

  if (transportController_ && transportController_->isPlaying()) {
    juce::int64 currentPos = transportController_->getPlayheadSamples();
    juce::int64 loopEnd = transportController_->getLoopEndSamples();
    juce::int64 loopStart = transportController_->getLoopStartSamples();
    bool looping = transportController_->isLooping();

    // Check for loop wrap
    bool wrapped = false;
    int samplesBeforeLoop = numSamples;

    if (looping && loopEnd > 0 && loopEnd > loopStart && currentPos < loopEnd &&
        (currentPos + numSamples) > loopEnd) {
      wrapped = true;
      samplesBeforeLoop = static_cast<int>(loopEnd - currentPos);
    }

    // Dummy containers for missing arguments

    // Pass 1
    if (samplesBeforeLoop > 0) {
      juce::AudioBuffer<float> buffer1(outputChannelData, numOutputChannels,
                                       samplesBeforeLoop);
      juce::MidiBuffer midi1;
      midiFifo_.drainTo(midi1, samplesBeforeLoop);

      if (audioRenderer_) {
        audioRenderer_->renderAudioGraph(
            buffer1, samplesBeforeLoop, currentPos, snapshot->lifecycle,
            snapshot->lifecycleAux, routingGraph_, masterLimiter_,
            masterPlugins_, // masterPlugins
            tempoMap_.get(), &midi1);
      }
    }

    // Pass 2 (Wrapped)
    if (wrapped) {
      int samplesAfter = numSamples - samplesBeforeLoop;
      if (samplesAfter > 0) {
        std::vector<float *> offsets(numOutputChannels);
        for (int ch = 0; ch < numOutputChannels; ++ch)
          offsets[ch] = outputChannelData[ch] + samplesBeforeLoop;

        juce::AudioBuffer<float> buffer2(offsets.data(), numOutputChannels,
                                         samplesAfter);
        juce::MidiBuffer midi2;
        midiFifo_.drainTo(midi2, samplesAfter);

        if (audioRenderer_) {
          audioRenderer_->renderAudioGraph(
              buffer2, samplesAfter, loopStart, snapshot->lifecycle,
              snapshot->lifecycleAux, routingGraph_, masterLimiter_,
              masterPlugins_, tempoMap_.get(), &midi2);
        }

        transportController_->setPlayheadSamples(loopStart + samplesAfter);
      }
    } else {
      transportController_->advancePlayhead(numSamples);
    }
  }

  if (recordingManager_ && recordingManager_->isRecording()) {
    recordingManager_->captureAudio(inputChannelData, numInputChannels,
                                    numSamples, snapshot->lifecycle);
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
      // Zenith tracks generally use atomic parameters or critical sections
      // internally for parameters.

      if (e.type == zenith::EngineEvent::Type::SetPluginParam) {
        // Get thread-safe snapshot (we are in audio thread, so we read
        // snapshot) But wait, we need to apply this to the track. The track
        // pointer in snapshot is valid. Finding the track:
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
  // thread-safe! If the message thread adds/removes tracks while this runs, the
  // vector may reallocate, causing a segfault.
  //
  // Phase 11: Process all tracks and mix them down to master output

  juce::ignoreUnused(inputChannelData, numInputChannels);

  // Wrap output buffer for unified render path
  juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels,
                                        numSamples);

  // Get current transport position
  juce::int64 position =
      transportController_ ? transportController_->getPlayheadSamples() : 0;

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

  // Fix: Correct argument order (numSamples, position) and pass local MIDI
  renderAudioGraph(outputBuffer, numSamples, position, &localMidi);

  // Push to Analysis FIFO (Stereo)
  if (analysisFifo_) {
    analysisFifo_->push(outputBuffer, numSamples);
  }

  // Fallback: If no tracks or all tracks are silent, optionally enable test
  // tone (Only if explicitly enabled via enableTestTone_)
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

void Engine::renderAudioGraph(juce::AudioBuffer<float> &outputBuffer,
                              int numSamples, juce::int64 playheadPosition,
                              const juce::MidiBuffer *incomingMidi) {
  if (audioRenderer_) {
    audioRenderer_->renderAudioGraph(outputBuffer, numSamples, playheadPosition,
                                     tracks_, auxBuses_, routingGraph_,
                                     masterLimiter_, masterPlugins_,
                                     tempoMap_.get(), incomingMidi);
  } else {
    outputBuffer.clear();
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
    }
  }

  if (audioRenderer_) {
    audioRenderer_->prepare(sampleRate, samplesPerBlockExpected, tracks_.size(),
                            auxBuses_.size());
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

  // Add message to FIFO for playback (lock-free)
  midiFifo_.push(message);

  // MIDI Recording: Capture MIDI for ALL armed MIDI/Instrument tracks
  // This enables multi-track MIDI recording from a single source
  if (recordingManager_ && recordingManager_->isRecording()) {
    juce::int64 currentPosition =
        transportController_ ? transportController_->getPlayheadSamples() : 0;

    // Route to ALL armed MIDI/Instrument tracks for multi-track recording
    // Use snapshot for RT-safe access to tracks
    auto *snapshot = activeSnapshot_.load();
    if (snapshot) {
      for (size_t i = 0; i < snapshot->tracks.size(); ++i) {
        auto *track = snapshot->tracks[i];
        if (track && track->isArmed()) {
          if (track->getType() == Track::Type::MIDI ||
              track->getType() == Track::Type::Instrument) {
            // Capture MIDI for this track (RT-safe, uses lock-free FIFO)
            recordingManager_->captureMidi(message, currentPosition,
                                           static_cast<int>(i));
          }
        }
      }
    }
  }
}

//==============================================================================

// Offline Export Implementation
//==============================================================================

bool Engine::exportProjectToWav(const juce::File &outputFile, double sampleRate,
                                int bitDepth, double durationInSeconds) {
  DBG("Engine: Starting WAV export to " + outputFile.getFullPathName());

  if (sampleRate <= 0.0)
    return false;
  if (bitDepth != 16 && bitDepth != 24 && bitDepth != 32)
    return false;

  // Auto-detect duration
  if (durationInSeconds <= 0.0) {
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
    durationInSeconds = std::max(10.0, maxEndTime + 1.0);
  }

  const juce::int64 totalSamples =
      static_cast<juce::int64>(durationInSeconds * sampleRate);
  constexpr int offlineBlockSize = 4096;
  const int numChannels = 2;

  // Prepare for export - message thread safe here
  prepareTracks(offlineBlockSize,
                sampleRate); // Prepare tracks for new rate/size
  if (audioRenderer_) {
    audioRenderer_->prepare(sampleRate, offlineBlockSize, tracks_.size(),
                            auxBuses_.size());
  }

  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
      new juce::FileOutputStream(outputFile), sampleRate,
      static_cast<unsigned int>(numChannels), bitDepth, {}, 0));

  if (!writer)
    return false;

  juce::AudioBuffer<float> renderBuffer(numChannels, offlineBlockSize);
  juce::int64 samplesRendered = 0;

  while (samplesRendered < totalSamples) {
    const int samplesToRender =
        static_cast<int>(juce::jmin(static_cast<juce::int64>(offlineBlockSize),
                                    totalSamples - samplesRendered));

    // Render using AudioRenderer
    if (audioRenderer_) {
      juce::MidiBuffer dummyMidi;
      audioRenderer_->renderAudioGraph(
          renderBuffer, samplesToRender, samplesRendered, tracks_, auxBuses_,
          routingGraph_, masterLimiter_, masterPlugins_, tempoMap_.get(),
          &dummyMidi);
    } else {
      renderBuffer.clear();
    }

    if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToRender)) {
      break; // Error
    }

    samplesRendered += samplesToRender;
  }

  writer.reset();

  // Restore state
  double originalRate = currentSampleRate.load();
  int originalSize = currentBufferSize.load();

  prepareTracks(originalSize, originalRate);
  if (audioRenderer_) {
    audioRenderer_->prepare(originalRate, originalSize, tracks_.size(),
                            auxBuses_.size());
  }

  return true;
}

//==============================================================================
// Phase 2D: Audio Recording (AUDIO THREAD)
//==============================================================================

juce::AudioPluginFormatManager &Engine::getPluginFormatManager() {
  return pluginHost_->getFormatManager();
}

/*  */

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
  if (audioRenderer_) {
    audioRenderer_->prepare(options.sampleRate, blockSize, tracks_.size(),
                            auxBuses_.size());
  }

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
  if (audioRenderer_) {
    // TODO: Expose per-track latency in AudioRenderer
    return 0; // audioRenderer_->getTrackLatency(trackIndex);
  }
  return 0;
}

int Engine::getMasterLatency() const {
  // TODO: Expose master latency in AudioRenderer
  return 0;
}

void Engine::setPDCEnabled(bool enabled) {
  if (audioRenderer_) {
    audioRenderer_->setPDCEnabled(enabled);
  }
}

//==============================================================================

//==============================================================================

double Engine::autoDetectProjectDuration() const {
  double maxDuration = 0.0;
  const double sampleRate = currentSampleRate.load();

  if (sampleRate <= 0.0)
    return 10.0; // Fallback

  // Scan all tracks for the latest clip end position
  for (const std::shared_ptr<zenith::Track> &track : tracks_) {
    if (!track)
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

// getPlayheadSamples() and getPlaybackPosition() are now inline in Engine.h

bool Engine::isLooping() const {
  return transportController_ ? transportController_->isLooping() : false;
}

juce::int64 Engine::getLoopStart() const {
  return transportController_ ? transportController_->getLoopStartSamples() : 0;
}

juce::int64 Engine::getLoopEnd() const {
  return transportController_ ? transportController_->getLoopEndSamples() : 0;
}

bool Engine::isPDCEnabled() const {
  return audioRenderer_ ? audioRenderer_->isPDCEnabled() : false;
}

int Engine::getMaxTrackLatency() const {
  return audioRenderer_ ? audioRenderer_->getMaxTrackLatency() : 0;
}

void Engine::recalculatePDC() {
  // PDC is handled by AudioRenderer during prepare/render
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
        // If any track is soloed, mute this track unless it is also soloed
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

  if (audioRenderer_) {
    audioRenderer_->prepare(currentSampleRate.load(), currentBufferSize.load(),
                            tracks_.size(), auxBuses_.size());
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

//==============================================================================
// Master Limiter API
//==============================================================================

void Engine::setMasterLimiterEnabled(bool enabled) {
  masterLimiter_.setEnabled(enabled);
  DBG("Engine: Master limiter " +
      juce::String(enabled ? "enabled" : "disabled"));
}

bool Engine::isMasterLimiterEnabled() const {
  return masterLimiter_.isEnabled();
}

void Engine::setMasterLimiterCeiling(float ceilingDb) {
  masterLimiter_.setCeiling(ceilingDb);
  DBG("Engine: Master limiter ceiling set to " + juce::String(ceilingDb, 1) +
      " dB");
}

float Engine::getMasterLimiterGainReduction() const {
  return masterLimiter_.getGainReductionDb();
}

int Engine::getMasterLimiterLatency() const {
  return masterLimiter_.getLatency();
}

//==============================================================================
// Track Freeze API (CPU Optimization)
//==============================================================================

bool Engine::freezeTrack(
    int trackIndex, std::function<void(float, const juce::String &)> progress) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) {
    DBG("Engine: freezeTrack - Invalid track index: " +
        juce::String(trackIndex));
    return false;
  }

  if (!freezeManager_) {
    DBG("Engine: freezeTrack - FreezeManager not initialized");
    return false;
  }

  // Determine freeze directory
  juce::File freezeDir;
  if (projectState_ && projectState_->getProjectFile().existsAsFile()) {
    freezeDir = projectState_->getProjectFile().getSiblingFile("Freeze Files");
  } else {
    freezeDir =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("ZenithDAW/Freeze");
  }

  return freezeManager_->freezeTrack(*tracks_[trackIndex], *this, freezeDir,
                                     progress);
}

bool Engine::unfreezeTrack(int trackIndex) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) {
    DBG("Engine: unfreezeTrack - Invalid track index: " +
        juce::String(trackIndex));
    return false;
  }

  if (!freezeManager_) {
    DBG("Engine: unfreezeTrack - FreezeManager not initialized");
    return false;
  }

  return freezeManager_->unfreezeTrack(*tracks_[trackIndex]);
}

bool Engine::isTrackFrozen(int trackIndex) const {
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) {
    return false;
  }

  return tracks_[trackIndex]->isFrozen();
}

} // namespace zenith
