/**
 * @file EngineSync.cpp
 * @brief Tempo, timeline synchronization, and PDC management
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "Engine.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include "../engine/AudioRenderer.h"
#include "../engine/Track.h"

namespace zenith {

//==============================================================================
// Tempo and Timeline Sync
//==============================================================================

void Engine::syncTempoMap() {
  if (projectState_ && tempoMap_) {
    tempoMap_->updateFromValueTree(projectState_->getTempoMap());
  }
}

const zenith::TempoMap &Engine::getTempoMap() const noexcept {
  jassert(tempoMap_ != nullptr);
  return *tempoMap_;
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

} // namespace zenith
