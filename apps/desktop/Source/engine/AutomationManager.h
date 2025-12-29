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
