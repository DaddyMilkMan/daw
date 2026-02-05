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

/*
    ==============================================================================
    Original file header:
*/

#pragma once

#include "AutomationLane.h"
#include <atomic>
#include <juce_core/juce_core.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace zenith {


/**
 * @class AutomationManager
 * @brief Manages automation lanes with RT-safe snapshot pattern.
 */
class AutomationManager {
public:
  AutomationManager();
  ~AutomationManager();

  // Message thread only
  void addLane(const juce::String &paramId,
               std::shared_ptr<AutomationLane> lane);
  void clearLanes();

  // Audio thread safe
  const std::shared_ptr<AutomationLane>
  getLane(const juce::String &paramId) const;

private:
  struct AutomationSnapshot {
    std::unordered_map<juce::String, std::shared_ptr<AutomationLane>> lanes;

    AutomationSnapshot() = default;
    explicit AutomationSnapshot(
        const std::unordered_map<juce::String, std::shared_ptr<AutomationLane>>
            &ownedLanes) {
      lanes = ownedLanes;
    }
  };

  void updateSnapshot();

  std::unordered_map<juce::String, std::shared_ptr<AutomationLane>> lanesOwned_;
  std::shared_ptr<AutomationSnapshot> currentSnapshot_;
  std::atomic<const AutomationSnapshot*> activeSnapshot_{nullptr};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationManager)
};

} // namespace zenith
