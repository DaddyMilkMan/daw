/**
 * @file Engine.cpp
 * @brief Audio engine implementation - Core Logic
 */

#include "Engine.h"
#include "MixerController.h"
#include "PlatformAudioUtils.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include "TrackAutomationSynchronizer.h"
#include "ZenithLogger.h"
#include <algorithm>
#include <array>
#include <memory>

// Subsystem Includes
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

// Modular Components
#include "../engine/AudioRenderer.h"
#include "../engine/MeteringSystem.h"
#include "../engine/Metronome.h"
#include "../engine/RecordingManager.h"
#include "../engine/TransportController.h"

namespace zenith {

Engine::Engine() {
  DBG("Engine: Constructor");

  // Initialize Modular Components
  audioRenderer_ = std::make_unique<AudioRenderer>();
  recordingManager_ = std::make_unique<RecordingManager>();
  transportController_ = std::make_unique<TransportController>();
  metronome_ = std::make_unique<Metronome>();
  meteringSystem_ = std::make_unique<MeteringSystem>();
  mixerController_ = std::make_unique<MixerController>();
  DBG("Engine: Modular components initialized");

  // Initialize subsystems
  audioFilePool_ = std::make_unique<zenith::AudioFilePool>();
  pluginHost_ = std::make_unique<zenith::PluginHost>();
  pluginHost_->scanDefaultLocations();
  pluginEditorWindowManager_ =
      std::make_unique<zenith::PluginEditorWindowManager>();

  instrumentRegistry_ = std::make_unique<zenith::InstrumentRegistry>();
  zenith::registerBuiltInInstruments(*instrumentRegistry_);

  sessionDebugger_ = std::make_unique<ai::SessionDebuggerAgent>(*this);
  masteringAgent_ = std::make_unique<ai::AIMasteringAgent>(*this);

  tempoMap_ = std::make_unique<zenith::TempoMap>();
  transportController_->setTempoMap(tempoMap_.get());

  freezeManager_ = std::make_unique<TrackFreezeManager>();

  // Register Master Bus node in RoutingGraph
  RoutingGraph::Node masterNode;
  masterNode.id = "master";
  masterNode.name = "Master";
  masterNode.type = RoutingGraph::NodeType::Master;
  masterNode.channelCount = 2;
  routingGraph_.addNode(masterNode);

  updateTrackSnapshot();
}

Engine::~Engine() {
  DBG("Engine: Destructor");
  isShuttingDown_.store(true);
  disableMidiInput();
  shutdown();

  audioRenderer_.reset();
  recordingManager_.reset();
  transportController_.reset();
  meteringSystem_.reset();
  masteringAgent_.reset();
  audioFilePool_.reset();
}

//==============================================================================
// Initialization / Shutdown
//==============================================================================

void Engine::setProjectState(ProjectState *state) {
  DBG("Engine: Setting project state");

  if (automationSynchronizer) {
    automationSynchronizer->stop();
    automationSynchronizer.reset();
  }

  projectState_ = state;

  if (recordingManager_) {
    recordingManager_->setProjectState(projectState_);
  }

  if (projectState_ != nullptr) {
    automationSynchronizer =
        std::make_unique<TrackAutomationSynchronizer>(*projectState_, *this);
    syncWithProjectState();
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
    tracks_.clear();
    return;
  }

  tracks_.clear();
  auto &state = projectState_->getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid())
    return;

  const double sampleRate = currentSampleRate.load();
  const int bufferSize = currentBufferSize.load();

  auto beatsToSamples = [this, sampleRate](double beats) -> juce::int64 {
    return tempoMap_
               ? tempoMap_->beatsToSamples(beats, sampleRate)
               : static_cast<juce::int64>(beats * (60.0 / 120.0) * sampleRate);
  };

  for (auto trackNode : tracksNode) {
    juce::String trackName = trackNode[ProjectState::PROP_NAME].toString();
    juce::String trackType = trackNode[ProjectState::PROP_TYPE].toString();

    zenith::Track::Type actualType = zenith::Track::Type::Audio;
    if (trackType == "midi")
      actualType = zenith::Track::Type::MIDI;
    else if (trackType == "instrument")
      actualType = zenith::Track::Type::Instrument;
    else if (trackType == "bus")
      actualType = zenith::Track::Type::Bus;

    auto track = zenith::Track::create(trackName, actualType);
    track->setTrackId(trackNode[ProjectState::PROP_ID].toString());
    track->setVolume(trackNode[ProjectState::PROP_VOLUME]);
    track->setPan(trackNode[ProjectState::PROP_PAN]);
    track->setMuted(trackNode[ProjectState::PROP_MUTE]);
    track->setSolo(trackNode[ProjectState::PROP_SOLO]);

    if (sampleRate > 0)
      track->prepareToPlay(bufferSize, sampleRate);

    auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
    if (clipsNode.isValid()) {
      for (auto clipNode : clipsNode) {
        auto clip = std::make_unique<zenith::Clip>();
        clip->setStartPosition(
            beatsToSamples(clipNode[ProjectState::PROP_START]));
        clip->setLength(beatsToSamples(clipNode[ProjectState::PROP_LENGTH]));

        juce::String audioFilePath =
            clipNode[ProjectState::PROP_AUDIO_FILE].toString();
        if (audioFilePath.isNotEmpty()) {
          juce::File audioFile(audioFilePath);
          if (audioFile.existsAsFile()) {
            clip->setAudioFile(audioFile);
            clip->setType(zenith::Clip::Type::Audio);
          }
        }
        if (sampleRate > 0)
          clip->prepareToPlay(bufferSize, sampleRate);
        clip->setPlaying(true);
        track->addClip(std::move(clip));
      }
    }

    track->setTrackIndex((int)tracks_.size());
    tracks_.push_back(std::move(track));

    RoutingGraph::Node node;
    node.id = tracks_.back()->getTrackId();
    node.name = tracks_.back()->getName();
    node.type = RoutingGraph::NodeType::Track;
    routingGraph_.addNode(node);
    routingGraph_.connect(tracks_.back()->getTrackId(), "master", 1.0f);
  }

  updateTrackSnapshot();
}

bool Engine::initialize() {
  DBG("Engine: Initializing...");
  auto error = deviceManager.initialiseWithDefaultDevices(2, 2);
  if (error.isNotEmpty()) {
    DBG("Engine: Failed to initialize audio device: " + error);
  }

  PlatformAudioUtils::initializeAudioDeviceSetup(deviceManager);
  auto setup = deviceManager.getAudioDeviceSetup();

  DBG("Engine: Audio device initialized - " + setup.outputDeviceName);
  currentSampleRate.store(setup.sampleRate);
  currentBufferSize.store(setup.bufferSize);

  deviceManager.addAudioCallback(this);

  if (recordingManager_) {
    recordingManager_->setDeviceManager(&deviceManager);
    recordingManager_->prepare(setup.sampleRate);
  }

  if (metronome_) {
    metronome_->prepareToPlay(setup.sampleRate, setup.bufferSize);
  }

  enableMidiInput();

  if (sessionDebugger_) {
    sessionDebugger_->startMonitoring(500);
  }

  return true;
}

void Engine::shutdown() {
  DBG("Engine: Shutting down...");
  if (sessionDebugger_)
    sessionDebugger_->stopMonitoring();
  stop();
  deviceManager.removeAudioCallback(this);
  deviceManager.closeAudioDevice();
  if (audioFilePool_)
    audioFilePool_->clear();
}

//==============================================================================
// Audio Thread Logic
//==============================================================================

void Engine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  DBG("Engine: Audio device starting...");
  currentSampleRate.store(device->getCurrentSampleRate());
  currentBufferSize.store(device->getCurrentBufferSizeSamples());

  phase = 0.0;
  prepareTracks(device->getCurrentBufferSizeSamples(),
                device->getCurrentSampleRate());

  if (audioRenderer_) {
    audioRenderer_->prepare(currentSampleRate.load(), currentBufferSize.load(),
                            tracks_.size(), auxBuses_.size());
  }
}

void Engine::audioDeviceStopped() {
  for (auto &track : tracks_)
    if (track)
      track->releaseResources();
}

void Engine::audioDeviceIOCallbackWithContext(
    const float *const *input, int numIn, float *const *output, int numOut,
    int numSamples,
    const juce::AudioIODeviceCallbackContext &context) noexcept {

  for (int i = 0; i < numOut; ++i)
    if (output[i])
      juce::FloatVectorOperations::clear(output[i], numSamples);

  auto *snapshot = activeSnapshot_.load();
  if (!snapshot)
    return;

  processEvents();

  juce::MidiBuffer midiBuffer;
  midiFifo_.drainTo(midiBuffer, numSamples);

  if (transportController_ && transportController_->isPlaying()) {
    juce::AudioBuffer<float> buffer(const_cast<float **>(output), numOut,
                                    numSamples);
    juce::int64 currentPos = transportController_->getPlayheadSamples();

    if (audioRenderer_) {
      audioRenderer_->renderAudioGraph(
          buffer, numSamples, currentPos, snapshot->tracks, snapshot->auxBuses,
          routingGraph_, masterLimiter_, masterPlugins_, tempoMap_.get(),
          &midiBuffer);

      if (metronome_ && metronome_->isEnabled()) {
        metronome_->getNextAudioBlock(buffer, currentPos, true, *tempoMap_);
      }
    }
    transportController_->advancePlayhead(numSamples);
  }

  if (meteringSystem_) {
    juce::AudioBuffer<float> fullOutput(const_cast<float **>(output), numOut,
                                        numSamples);
    meteringSystem_->getAnalysisFifo().push(fullOutput, numSamples);
  }

  if (recordingManager_ && recordingManager_->isRecording()) {
    recordingManager_->captureAudio(input, numIn, numSamples,
                                    snapshot->lifecycle);
  }
}

void Engine::processAudioBlock(const float *const *input, int numIn,
                               float *const *output, int numOut,
                               int numSamples) noexcept {
  // Legacy wrapper if needed, logic moved to callback
}

//==============================================================================
// Event Queue
//==============================================================================

bool Engine::queueEvent(const zenith::EngineEvent &e) {
  int start1, size1, start2, size2;
  commandFifo_.prepareToWrite(1, start1, size1, start2, size2);
  if (size1 > 0) {
    commandBuffer_[start1] = e;
    commandFifo_.finishedWrite(1);
    return true;
  }
  return false;
}

void Engine::processEvents() noexcept {
  int start1, size1, start2, size2;
  commandFifo_.prepareToRead(commandFifo_.getNumReady(), start1, size1, start2,
                             size2);
  auto *snapshot = activeSnapshot_.load();
  if (size1 > 0)
    for (int i = 0; i < size1; ++i)
      applyEvent(commandBuffer_[start1 + i], snapshot);
  if (size2 > 0)
    for (int i = 0; i < size2; ++i)
      applyEvent(commandBuffer_[start2 + i], snapshot);
  commandFifo_.finishedRead(size1 + size2);
}

void Engine::applyEvent(const zenith::EngineEvent &e,
                        TrackSnapshot *snapshot) noexcept {
  if (!snapshot)
    return;
  if (e.type == zenith::EngineEvent::Type::SetPluginParam) {
    if (e.trackIndex >= 0 && e.trackIndex < (int)snapshot->tracks.size()) {
      if (auto *track = snapshot->tracks[e.trackIndex]) {
        if (auto *plugin = track->getPlugin(e.pluginIndex)) {
          auto params = plugin->getParameters();
          if (e.paramIndex >= 0 && e.paramIndex < (int)params.size())
            params[e.paramIndex]->setValueNotifyingHost(e.value);
        }
      }
    }
  } else if (e.type == zenith::EngineEvent::Type::SetTrackVolume) {
    if (e.trackIndex >= 0 && e.trackIndex < (int)snapshot->tracks.size())
      if (auto *track = snapshot->tracks[e.trackIndex])
        track->setVolume(e.value);
  }
}

//==============================================================================
// Rendering Wrappers
//==============================================================================

void Engine::renderAudioGraph(juce::AudioBuffer<float> &outputBuffer,
                              int numSamples, juce::int64 playheadPosition,
                              const std::vector<zenith::Track *> &tracks,
                              const std::vector<zenith::AuxBus *> &auxBuses,
                              const juce::MidiBuffer *incomingMidi) {
  if (audioRenderer_) {
    // Pass raw pointers (tracks, auxBuses) to AudioRenderer
    audioRenderer_->renderAudioGraph(outputBuffer, numSamples, playheadPosition,
                                     tracks, auxBuses, routingGraph_,
                                     masterLimiter_, masterPlugins_,
                                     tempoMap_.get(), incomingMidi);
  } else {
    outputBuffer.clear();
  }
}

void Engine::renderOfflineBlock(juce::AudioBuffer<float> &buffer,
                                int numSamples, juce::int64 position) {
  auto *snapshot = activeSnapshot_.load();
  if (snapshot)
    renderAudioGraph(buffer, numSamples, position, snapshot->tracks,
                     snapshot->auxBuses, nullptr);
}

//==============================================================================
// Accessors / Helpers
//==============================================================================

juce::String Engine::getAudioDeviceInfo() const {
  auto *device = deviceManager.getCurrentAudioDevice();
  return device ? device->getName() : "No Device";
}

double Engine::getCpuUsage() const { return cpuUsage_.load(); }
zenith::AudioFilePool &Engine::getAudioFilePool() { return *audioFilePool_; }
zenith::PluginHost &Engine::getPluginHost() noexcept { return *pluginHost_; }
int Engine::scanForPlugins() {
  return pluginHost_ ? pluginHost_->scanDefaultLocations() : 0;
}
zenith::PluginEditorWindowManager &
Engine::getPluginEditorWindowManager() noexcept {
  return *pluginEditorWindowManager_;
}
const zenith::TempoMap &Engine::getTempoMap() const noexcept {
  return *tempoMap_;
}
juce::AudioPluginFormatManager &Engine::getPluginFormatManager() {
  return pluginHost_->getFormatManager();
}
void Engine::registerFormats() {}
juce::ThreadPool &Engine::getThreadPool() { return threadPool; }

} // namespace zenith
