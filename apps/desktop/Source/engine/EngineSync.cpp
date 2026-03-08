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
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
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
// NOTE: Latency data is stored in liveContext_, not AudioRenderer members.
//==============================================================================

// These are now defined in EngineMixing.cpp to avoid ODR violations.
// setPDCEnabled, isPDCEnabled also moved to EngineMixing.cpp

} // namespace zenith

