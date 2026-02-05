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
    EnginePlugins.cpp - Master plugin management and RCU snapshot logic
    Note: This is a modular component of Engine - declarations remain in Engine.h
*/


#include <memory>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace zenith {

void Engine::updateMasterPluginSnapshot() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  auto newSnapshot = std::make_shared<MasterPluginSnapshot>();

  {
    const juce::ScopedLock sl(masterPluginLock_);
    newSnapshot->plugins = masterPlugins_;
  }

  // Atomic swap
  activeMasterPluginsSnapshot_.store(newSnapshot.get());

  // Manage lifetime
  RealTimeGarbageCollector::getInstance().deferDelete(currentMasterPluginsSnapshotHolder_);
  currentMasterPluginsSnapshotHolder_ = newSnapshot;
}

void Engine::addMasterPlugin(std::shared_ptr<juce::AudioPluginInstance> plugin) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (!plugin)
    return;

  {
    const juce::ScopedLock sl(masterPluginLock_);
    masterPlugins_.push_back(plugin);
  }

  updateMasterPluginSnapshot();
}

void Engine::removeMasterPlugin(int index) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  {
    const juce::ScopedLock sl(masterPluginLock_);
    if (index >= 0 && index < static_cast<int>(masterPlugins_.size())) {
      masterPlugins_.erase(masterPlugins_.begin() + index);
    }
  }

  updateMasterPluginSnapshot();
}

} // namespace zenith
