```cpp
/**
 * @file Engine.cpp
 * @brief Audio engine implementation - Core Logic
 */

#include "PlatformAudioUtils.h"
#include "ZenithLogger.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include "TrackAutomationSynchronizer.h"
#include <algorithm> // For std::remove_if
#include <array>     // For RT-safe stack allocation in audio callback

// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "../ai/AIMasteringAgent.h"
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
<<<<<<< HEAD
#include "../engine/RecordingManager.h"
#include "../engine/TransportController.h"
=======
>>>>>>> origin/master
#include "../engine/MeteringSystem.h"
#include "../engine/Metronome.h"
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
  metronome_ = std::make_unique<Metronome>();
  meteringSystem_ = std::make_unique<MeteringSystem>();
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

  // Initialize AI Mastering Agent
  masteringAgent_ = std::make_unique<ai::AIMasteringAgent>(*this);
  DBG("Engine: AIMasteringAgent initialized");

  // MeteringSystem handles analysis FIFO internally

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
  meteringSystem_.reset();
  masteringAgent_.reset();

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

    // Create track via Factory
    zenith::Track::Type actualType = zenith::Track::Type::Audio;
    if (trackType == "midi")
      actualType = zenith::Track::Type::MIDI;
    else if (trackType == "instrument")
      actualType = zenith::Track::Type::Instrument;
    else if (trackType == "bus")
      actualType = zenith::Track::Type::Bus;

    auto track = zenith::Track::create(trackName, actualType);

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
        auto clip = std::make_unique<zenith::Clip>();

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
            clip->setType(zenith::Clip::Type::Audio);
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
    track->setTrackIndex((int)tracks_.size());
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

  // 1. Initialize Audio Device Manager with default devices
  auto error = deviceManager.initialiseWithDefaultDevices(2, 2); // 2 in, 2 out

  if (error.isNotEmpty()) {
    DBG("Engine: Failed to initialize audio device: " + error);
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon, "Audio Device Error",
        "Failed to initialize audio device:\n" + error, "OK");
    // Don't return false yet, try platform-specific fallback
  }

  // 2. Initialize Audio Devices with platform-specific fallbacks
  PlatformAudioUtils::initializeAudioDeviceSetup(deviceManager);

  // Get current device setup (might have changed due to fallback)
  auto setup = deviceManager.getAudioDeviceSetup();

          }
      }
  }
  DBG("===============================================================================================");
  
  // Refresh setup if it changed during fallback
  setup = deviceManager.getAudioDeviceSetup();
#endif

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

  // Prepare Metronome
  if (metronome_) {
    metronome_->prepareToPlay(setup.sampleRate, setup.bufferSize);
    // Sync metronome state with TransportController
    metronome_->setEnabled(transportController_->isMetronomeEnabled());
    metronome_->setLevel(transportController_->getMetronomeLevel());
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
<<<<<<< HEAD
=======
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

  // Handle Count-In / Pre-Roll
  if (metronome_ && metronome_->getCountInBars() > 0 && transportController_) {
    if (tempoMap_) {
      int numerator = tempoMap_->getTimeSignatureNumerator();
      double countInBeats =
          static_cast<double>(metronome_->getCountInBars() * numerator);
      double currentBeats = transportController_->getPlayheadBeats();
      double startBeats = currentBeats - countInBeats;

      juce::int64 startSamples =
          tempoMap_->beatsToSamples(startBeats, currentSampleRate.load());
      transportController_->setPlayheadSamples(startSamples);

      DBG("Engine: Pre-Roll active. Shifted from " +
          juce::String(currentBeats) + " to " + juce::String(startBeats) +
          " beats.");
    }
  }

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
  // Note: We pass the *current* playhead samples (which might include pre-roll
  // subtraction) This means the recording will start "early" on the timeline,
  // effectively capturing the count-in. This is a "Pre-Roll" recording
  // behavior.
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

void Engine::panic() {
  DBG("Engine: PANIC triggered!");

  // 1. Stop Transport
  stop();

  // 2. Iterate all tracks (message thread is safe)
  for (const auto &track : tracks_) {
    if (track) {
      // Clear any pending MIDI events in the track
      // (Track doesn't expose a method for this yet, assuming implementation
      // needed later)

      // Mute temporarily to stop audio output immediately
      // track->setMuted(true); // Maybe too aggressive?

      // Allow reverb tails to fade naturally or kill them?
      // Panic usually implies immediate silence.
      // Ideally we would send MIDI CC 123 (All Notes Off) and 120 (All Sound
      // Off) but we need a mechanism to inject MIDI into the track. For now, we
      // will rely on stop() stopping the engine processing primarily.
    }
  }
}

void Engine::setSidechainSource(int destTrackIndex, int pluginIndex,
                                int sourceTrackIndex) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (destTrackIndex < 0 || destTrackIndex >= tracks_.size())
    return;
  if (sourceTrackIndex < 0 || sourceTrackIndex >= tracks_.size())
    return;

  auto &destTrack = tracks_[destTrackIndex];
  auto &sourceTrack = tracks_[sourceTrackIndex];

  DBG("Engine: Routing Sidechain: " << sourceTrack->getName() << " -> "
                                    << destTrack->getName() << " (Plugin "
                                    << pluginIndex << ")");

  // Connect in routing graph (Stub Logic for Phase 2)
  if (destTrack && sourceTrack) {
    // routingGraph_.connect(sourceTrack->getTrackId(),
    // destTrack->getTrackId(), 1.0f); Note: Real implementation needs to target
    // specific plugin inputs, not just track mix.
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
>>>>>>> origin/master
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
<<<<<<< HEAD
=======
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
    // Create track via Factory
    auto track =
        zenith::Track::create("Track " + juce::String(tracks_.size() + 1),
                              zenith::Track::Type::Audio);

    // Prepare track for audio processing if engine is already running
    if (currentSampleRate.load() > 0) {
      track->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
    }

    track->setTrackIndex((int)tracks_.size());
    tracks_.push_back(std::move(
        track)); // Corrected: transfer ownership from unique_ptr to shared_ptr

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
>>>>>>> origin/master
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

const zenith::TempoMap &Engine::getTempoMap() const noexcept {
  jassert(tempoMap_ != nullptr);
  return *tempoMap_;
}

juce::AudioPluginFormatManager &Engine::getPluginFormatManager() {
  return pluginHost_->getFormatManager();
}

<<<<<<< HEAD
void Engine::registerFormats() {
  // Bug 27: JUCE FormatManager takes ownership of registered formats
  formatManager.registerBasicFormats();
  formatManager.registerFormat(new juce::FlacAudioFormat(), false);
  formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
=======
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

      recordingManager_->prepareRecordingForTrack(
          *tracks_[trackIndex], trackIndex, recordingsDir); // Rebuild fix
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
  return meteringSystem_ ? meteringSystem_->getMasterLevel() : 0.0f;
}

float Engine::getMasterPeakLevel() const {
  return meteringSystem_ ? meteringSystem_->getMasterPeak() : 0.0f;
}

void Engine::resetPeakMeters() {
  // Reset master peak
  if (meteringSystem_) {
    meteringSystem_->resetMasterPeak();
  }
  if (audioRenderer_) {
    audioRenderer_->resetPeakMeters();
  }
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
    auto track =
        std::shared_ptr<zenith::Track>(zenith::Track::create(name, trackType));

    // Use atomic counter for ID generation
    juce::String trackId = "track_" + juce::String(nextTrackId_++);
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

  track->setTrackIndex((int)tracks_.size());
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

  // [DSP Optimization] Update routing graph snapshot with direct pointers for
  // fast lookup
  {
    std::unordered_map<juce::String, Track *> trackMap;
    for (const auto &track : tracks_) {
      if (track)
        trackMap[track->getTrackId()] = track.get();
    }

    std::unordered_map<juce::String, AuxBus *> auxBusMap;
    for (const auto &bus : auxBuses_) {
      if (bus)
        auxBusMap[bus->getId()] = bus.get();
    }

    routingGraph_.updateSnapshotWithPointers(trackMap, auxBusMap);
  }

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
>>>>>>> origin/master
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

  // Midi Buffer for rendering (populated from FIFO)
  juce::MidiBuffer midiBuffer;
  midiFifo_.drainTo(midiBuffer, numSamples);

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

    // Pass 1
    if (samplesBeforeLoop > 0) {
      // Use proxy buffer to avoid allocation
      juce::AudioBuffer<float> buffer1(outputChannelData, numOutputChannels,
                                       samplesBeforeLoop);
<<<<<<< HEAD
      
      // Extract MIDI for this section (simple split not supported by MidiBuffer, using full buffer for now or empty?)
      // For correctness in looping, we should split MIDI, but for now passing empty/full might be acceptable if events are sparse.
      // Ideally handled by AudioRenderer splitting. Here we pass the full drained midi to the first block or handle properly.
      // AudioRenderer::renderAudioGraph is responsible for processing MIDI. 
=======

      juce::MidiBuffer midi1;
      midiFifo_.drainTo(midi1, samplesBeforeLoop);
>>>>>>> origin/master

      if (audioRenderer_) {
        // NOTE: We pass 'midiBuffer' (full buffer) to first pass. 
        // This is a simplification; ideally we split MIDI events based on timestamp.
        audioRenderer_->renderAudioGraph(
            buffer1, samplesBeforeLoop, currentPos, snapshot->tracks,
<<<<<<< HEAD
            snapshot->auxBuses, routingGraph_, masterLimiter_,
            masterPlugins_, tempoMap_.get(), &midiBuffer);
=======
            snapshot->auxBuses, routingGraph_, masterLimiter_, masterPlugins_,
            tempoMap_.get(), &midi1, inputChannelData, numInputChannels);
>>>>>>> origin/master

            // Mix Metronome (Pass 1)
            if (metronome_) {
              metronome_->getNextAudioBlock(buffer1, currentPos, true,
                                            *tempoMap_);
            }
      }
    }

    // Pass 2 (Wrapped)
    if (wrapped) {
      int samplesAfter = numSamples - samplesBeforeLoop;
      if (samplesAfter > 0) {
        // Use stack array for channel pointers to avoid heap allocation
        jassert(numOutputChannels <= 32 &&
                "Audio callback has a hardcoded limit of 32 channels");
        float *offsets[32]; // Max 32 channels supported
        int safeNumChannels = juce::jmin(numOutputChannels, 32);

        for (int ch = 0; ch < safeNumChannels; ++ch)
          if (outputChannelData[ch])
            offsets[ch] = outputChannelData[ch] + samplesBeforeLoop;

        juce::AudioBuffer<float> buffer2(offsets, safeNumChannels,
                                         samplesAfter);
        
        juce::MidiBuffer emptyMidi; // No MIDI in wrapped part for now

        if (audioRenderer_) {
          audioRenderer_->renderAudioGraph(
              buffer2, samplesAfter, loopStart, snapshot->tracks,
<<<<<<< HEAD
              snapshot->auxBuses, routingGraph_, masterLimiter_,
              masterPlugins_, tempoMap_.get(), &emptyMidi);
=======
              snapshot->auxBuses, routingGraph_, masterLimiter_, masterPlugins_,
              tempoMap_.get(), &midi2, offsets, safeNumChannels); // Using offset inputs
>>>>>>> origin/master

              // Mix Metronome (Pass 2)
              if (metronome_) {
                metronome_->getNextAudioBlock(buffer2, loopStart, true,
                                              *tempoMap_);
              }
        }

        transportController_->setPlayheadSamples(loopStart + samplesAfter);
      }
    } else {
      transportController_->advancePlayhead(numSamples);
    }
  } else {
    // Not playing? 
    // We could process reverb tails here if we wanted.
  }

  // Analysis / Visualizers
  // We need to push the final output to the analysis FIFO.
  // Reconstruct full buffer wrapper
  juce::AudioBuffer<float> fullOutput(outputChannelData, numOutputChannels, numSamples);
  if (meteringSystem_) {
      meteringSystem_->getAnalysisFifo().push(fullOutput, numSamples);
  }

  // Audio Recording
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

  auto *snapshot = activeSnapshot_.load();

  if (size1 > 0) {
    for (int i = 0; i < size1; ++i) {
      applyEvent(commandBuffer_[start1 + i], snapshot);
    }
  }
  if (size2 > 0) {
    for (int i = 0; i < size2; ++i) {
      applyEvent(commandBuffer_[start2 + i], snapshot);
    }
  }

  commandFifo_.finishedRead(size1 + size2);
}

void Engine::applyEvent(const zenith::EngineEvent &e,
                        TrackSnapshot *snapshot) noexcept {
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

<<<<<<< HEAD
// Retain legacy public render API for other consumers if any (but AudioRenderer does the work)
=======
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

  // Update transport position for all clips in all tracks (Delegated to
  // AudioRenderer)
  if (snapshot && audioRenderer_) {
    audioRenderer_->updateClipPositions(snapshot->tracks, position);
  }

  // REMOVED: Fall-through renderAudioGraph call that caused double processing
  // The logic inside transportController_->isPlaying() block now handles all
  // rendering.

  // Push to Analysis FIFO (Stereo)
  if (meteringSystem_) {
    meteringSystem_->getAnalysisFifo().push(outputBuffer, numSamples);
  }

  // Fallback: If no tracks or all tracks are silent, optionally enable test
  // tone (Only if explicitly enabled via enableTestTone_)
  // Using snapshot->tracks to check emptiness safely
  bool testToneEnabled = enableTestTone_.load();
  bool tracksEmpty = snapshot ? snapshot->tracks.empty() : true;

  if (testToneEnabled && tracksEmpty) {
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

>>>>>>> origin/master
void Engine::renderAudioGraph(juce::AudioBuffer<float> &outputBuffer,
                              int numSamples, juce::int64 playheadPosition,
                              const std::vector<zenith::Track *> &tracks,
                              const std::vector<zenith::AuxBus *> &auxBuses,
                              const juce::MidiBuffer *incomingMidi) {
  if (audioRenderer_) {
    audioRenderer_->renderAudioGraph(outputBuffer, numSamples, playheadPosition,
                                     tracks, auxBuses, routingGraph_,
                                     masterLimiter_, masterPlugins_,
                                     tempoMap_.get(), incomingMidi, nullptr, 0);
  } else {
    outputBuffer.clear();
  }
}

<<<<<<< HEAD
// Legacy processAudioBlock - keep as private helper if needed or just remove, but to match summary:
void Engine::processAudioBlock(const float *const *inputChannelData,
                               int numInputChannels,
                               float *const *outputChannelData,
                               int numOutputChannels, int numSamples) noexcept {
    // Legacy method - delegated to audioDeviceIOCallbackWithContext logic via loop
    // But since IO callback is the updated one, this might be unused.
    // However, keeping it as an empty shell or redirecting to ensure signature match if vtable requires it.
    // Engine declares it private.
    juce::ignoreUnused(inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples);
=======
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
  DBG("Engine: Disabling MIDI inputs...");

  for (auto &input : midiInputs_) {
    if (input)
      input->stop();
  }
  midiInputs_.clear();
  DBG("Engine: MIDI inputs stopped");
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

      // Build raw pointer vectors for AudioRenderer
      std::vector<Track *> trackPtrs;
      trackPtrs.reserve(tracks_.size());
      for (const auto &t : tracks_)
        trackPtrs.push_back(t.get());

      std::vector<AuxBus *> auxPtrs;
      auxPtrs.reserve(auxBuses_.size());
      for (const auto &a : auxBuses_)
        auxPtrs.push_back(a.get());

      audioRenderer_->renderAudioGraph(renderBuffer, samplesToRender,
                                       samplesRendered, trackPtrs, auxPtrs,
                                       routingGraph_, masterLimiter_,
                                       masterPlugins_, tempoMap_.get(),
                                       &dummyMidi, nullptr, 0);
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

    // Render Mix - create raw pointer vectors for export
    std::vector<zenith::Track *> trackPtrs;
    std::vector<zenith::AuxBus *> auxPtrs;
    for (const auto &t : tracks_) {
      if (t)
        trackPtrs.push_back(t.get());
    }
    for (const auto &a : auxBuses_) {
      if (a)
        auxPtrs.push_back(a.get());
    }
    renderAudioGraph(renderBuffer, numSamples, samplesWritten, trackPtrs,
                     auxPtrs, nullptr);

    // Apply Dithering
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(renderBuffer, options.bitDepth);
    }

    // Normalization (2-Pass: Find Peak -> Apply Gain)
    // Normalization (Offline Render Refactor required for full track)
    // NOTE: Per-block normalization is WRONG for full track export.
    // Correct implementation requires render-to-temp-file -> scan ->
    // write-to-final This is disabled pending a full offline-render refactor.
    // See: applyNormalization() for when this gets properly implemented.
    (void)options.normalize; // Suppress unused warning

    if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples)) {
      return false;
    }

    samplesWritten += numSamples;
  }

  return true;
}

void Engine::applyNormalization(juce::AudioBuffer<float> &buffer, float maxPeak,
                                float targetDb) {
  if (maxPeak <= 0.00001f)
    return;

  float targetLinear = juce::Decibels::decibelsToGain(targetDb);
  float gain = targetLinear / maxPeak;
  buffer.applyGain(gain);
}

//==============================================================================
// Plugin Delay Compensation (PDC)
//==============================================================================

int Engine::getTrackLatency(int trackIndex) const {
  if (audioRenderer_) {
    return audioRenderer_->getTrackLatency(trackIndex);
  }
  return 0;
}

int Engine::getMasterLatency() const {
  if (audioRenderer_) {
    return audioRenderer_->getMasterLatency();
  }
  return 0;
}

void Engine::setPDCEnabled(bool enabled) {
  if (audioRenderer_) {
    audioRenderer_->setPDCEnabled(enabled);
  }
}

//==============================================================================

//==============================================================================

//==============================================================================
// TrackSnapshot Implementation
//==============================================================================

Engine::TrackSnapshot::TrackSnapshot(
    const std::vector<std::shared_ptr<zenith::Track>> &ownedTracks,
    const std::vector<std::shared_ptr<zenith::AuxBus>> &ownedBuses) {
  this->tracks.reserve(ownedTracks.size());
  this->lifecycle.reserve(ownedTracks.size());
  for (const auto &track : ownedTracks) {
    if (track != nullptr) {
      this->tracks.push_back(track.get());
      this->lifecycle.push_back(track); // Increment refcount
      this->trackMap[track->getTrackId().toStdString()] = track.get();
    }
  }

  this->auxBuses.reserve(ownedBuses.size());
  this->lifecycleAux.reserve(ownedBuses.size());
  for (const auto &bus : ownedBuses) {
    if (bus != nullptr) {
      this->auxBuses.push_back(bus.get());
      this->lifecycleAux.push_back(bus);
      this->auxBusMap[bus->getId().toStdString()] = bus.get();
    }
  }
}

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
  if (audioRenderer_) {
    // Build raw pointer vector for AudioRenderer
    std::vector<Track *> trackPtrs;
    trackPtrs.reserve(tracks_.size());
    for (const auto &t : tracks_)
      trackPtrs.push_back(t.get());

    audioRenderer_->calculatePDC(trackPtrs);
  }
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

void Engine::cancelFreeze() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (freezeManager_) {
    freezeManager_->cancelFreeze();
  }
}

//==============================================================================
// Metronome
//==============================================================================

void Engine::toggleMetronome() {
  if (transportController_) {
    bool newState = !transportController_->isMetronomeEnabled();
    transportController_->setMetronomeEnabled(newState);
    if (metronome_) {
      metronome_->setEnabled(newState);
    }
  }
}

bool Engine::isMetronomeEnabled() const {
  return transportController_ ? transportController_->isMetronomeEnabled()
                              : false;
}

void Engine::setMetronomeLevel(float level) {
  if (transportController_) {
    transportController_->setMetronomeLevel(level);
    if (metronome_) {
      metronome_->setLevel(level);
    }
  }
>>>>>>> origin/master
}

//==============================================================================
juce::ThreadPool &Engine::getThreadPool() { return threadPool; }

} // namespace zenith
