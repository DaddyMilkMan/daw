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
