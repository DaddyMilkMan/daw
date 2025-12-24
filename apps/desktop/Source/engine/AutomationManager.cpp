#include "AutomationManager.h"
#include "RealTimeGarbageCollector.h"
#include <memory>

namespace zenith {

AutomationManager::AutomationManager() {
  auto initialSnapshot = std::make_shared<AutomationSnapshot>();
  activeSnapshot_.store(initialSnapshot, std::memory_order_release);
}

AutomationManager::~AutomationManager() {
  lanesOwned_.clear();
  activeSnapshot_.store(nullptr);
}

void AutomationManager::addLane(const juce::String &paramId,
                                std::shared_ptr<AutomationLane> lane) {
  lanesOwned_[paramId] = lane;
  updateSnapshot();
}

void AutomationManager::clearLanes() {
  lanesOwned_.clear();
  updateSnapshot();
}

const std::shared_ptr<AutomationLane>
AutomationManager::getLane(const juce::String &paramId) const {
  auto snapshot = activeSnapshot_.load(std::memory_order_acquire);
  if (!snapshot)
    return nullptr;

  auto it = snapshot->lanes.find(paramId);
  if (it != snapshot->lanes.end()) {
    return it->second;
  }
  return nullptr;
}

void AutomationManager::updateSnapshot() {
  auto oldSnapshot = activeSnapshot_.load(std::memory_order_acquire);
  auto newSnapshot = std::make_shared<AutomationSnapshot>(lanesOwned_);
  activeSnapshot_.store(newSnapshot, std::memory_order_release);

  if (oldSnapshot) {
    RealTimeGarbageCollector::getInstance().push(oldSnapshot);
  }
}

} // namespace zenith
