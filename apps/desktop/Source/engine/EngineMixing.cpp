/**
 * @file EngineMixing.cpp
 * @brief Mixer control, metering, aux bus routines, and master effects
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "../engine/AudioRenderer.h"
#include "../engine/AuxBus.h"
#include "../engine/MeteringSystem.h"
#include "../engine/Metronome.h"
#include "../engine/RecordingManager.h"
#include "../engine/Track.h"
#include "../engine/TrackFreeze.h"
#include "../engine/TransportController.h"
#include "Engine.h"
#include "ProjectState.h"

namespace zenith {

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
  return meteringSystem_
             ? meteringSystem_->getLevel(MeteringSystem::MeterMode::Peak)
             : 0.0f;
}

float Engine::getMasterPeakLevel() const {
  return meteringSystem_ ? meteringSystem_->getPeak() : 0.0f;
}

void Engine::resetPeakMeters() {
  // Reset master peak
  if (meteringSystem_) {
    meteringSystem_->resetPeak();
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

  bus->setBusIndex(static_cast<int>(auxBuses_.size()));
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
    renderContext_.prepare(currentSampleRate.load(), currentBufferSize.load(),
                          tracks_.size(), auxBuses_.size());
  }

  updateTrackSnapshot();

  DBG("Engine: Created Aux Bus '" + name + "' (ID: " + node.id + ") at index " + juce::String(index));
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

    // Re-index remaining buses
    for (int i = 0; i < static_cast<int>(auxBuses_.size()); ++i) {
        auxBuses_[i]->setBusIndex(i);
    }

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
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  masterLimiter_.setEnabled(enabled);
  DBG("Engine: Master limiter " +
      juce::String(enabled ? "enabled" : "disabled"));
}

bool Engine::isMasterLimiterEnabled() const {
  return masterLimiter_.isEnabled();
}

void Engine::setMasterLimiterCeiling(float ceilingDb) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
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

void Engine::setSidechainSource(int destTrackIndex, int pluginIndex, int sourceTrackIndex) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (destTrackIndex < 0 || destTrackIndex >= static_cast<int>(tracks_.size())) {
        DBG("Engine: Invalid sidechain destination track index: " + juce::String(destTrackIndex));
        return;
    }

    Track* sourceTrack = nullptr;
    if (sourceTrackIndex >= 0 && sourceTrackIndex < static_cast<int>(tracks_.size())) {
        sourceTrack = tracks_[sourceTrackIndex].get();
    } else if (sourceTrackIndex != -1) {
        DBG("Engine: Invalid sidechain source track index: " + juce::String(sourceTrackIndex));
        return;
    }

    tracks_[destTrackIndex]->setPluginSidechainSource(pluginIndex, sourceTrack);
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

double Engine::getCpuUsage() const { return deviceManager.getCpuUsage(); }

void Engine::cancelFreeze() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (freezeManager_) {
    freezeManager_->cancelFreeze();
    DBG("Engine: Cancelled active freeze operation");
  }
}



//==============================================================================
// Plugin Delay Compensation (PDC)
//==============================================================================

void Engine::recalculatePDC() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  
  if (audioRenderer_) {
      // Convert shared_ptr vector to raw pointer vector for calculatePDC
      std::vector<zenith::Track*> trackPtrs;
      trackPtrs.reserve(tracks_.size());
      for (const auto& t : tracks_) {
          trackPtrs.push_back(t.get());
      }
      
      int maxLatency = audioRenderer_->calculatePDC(renderContext_, trackPtrs);
      DBG("Engine: PDC Recalculated. Max latency: " + juce::String(maxLatency) + " samples");
  }
}

int Engine::getTrackLatency(int trackIndex) const {
  // Read from Live Context
  if (trackIndex >= 0 && trackIndex < static_cast<int>(renderContext_.trackLatencies.size())) {
      return renderContext_.trackLatencies[trackIndex];
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
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    if (audioRenderer_) {
        audioRenderer_->setPDCEnabled(enabled);
        DBG("Engine: PDC " + juce::String(enabled ? "Enabled" : "Disabled"));
    }
}

bool Engine::isPDCEnabled() const {
    if (audioRenderer_) {
        return audioRenderer_->isPDCEnabled();
    }
    return false;
}

int Engine::getMaxTrackLatency() const {
    return renderContext_.maxTrackLatency;
}

} // namespace zenith
