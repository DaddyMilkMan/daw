/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Engine.h"
#include "ProjectState.h"
#include "TempoMap.h"

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

