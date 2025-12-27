#include "AutomationManager.h"
#include "RealTimeGarbageCollector.h"

namespace zenith {

AutomationManager::AutomationManager() {
    currentSnapshot_ = std::make_shared<AutomationSnapshot>();
    activeSnapshot_.store(currentSnapshot_.get());
}

AutomationManager::~AutomationManager() {
    lanesOwned_.clear();
    currentSnapshot_ = std::make_shared<AutomationSnapshot>();
    activeSnapshot_.store(currentSnapshot_.get());
}

void AutomationManager::addLane(const juce::String& paramId, std::shared_ptr<AutomationLane> lane) {
    lanesOwned_[paramId] = lane;
    updateSnapshot();
}

void AutomationManager::clearLanes() {
    lanesOwned_.clear();
    updateSnapshot();
}

const std::shared_ptr<AutomationLane> AutomationManager::getLane(const juce::String& paramId) const {
    const AutomationSnapshot* snapshot = activeSnapshot_.load(std::memory_order_acquire);
    if (!snapshot) return nullptr;
    
    auto it = snapshot->lanes.find(paramId);
    if (it != snapshot->lanes.end()) return it->second;
    return nullptr;
}

void AutomationManager::updateSnapshot() {
    auto newSnapshot = std::make_shared<AutomationSnapshot>(lanesOwned_);
    activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
    RealTimeGarbageCollector::getInstance().deferDelete(currentSnapshot_);
    currentSnapshot_ = newSnapshot;
}

} // namespace zenith
