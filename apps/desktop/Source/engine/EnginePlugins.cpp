/**
 * @file EnginePlugins.cpp
 * @brief Master plugin management and RCU snapshot logic
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "Engine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "RealTimeGarbageCollector.h"

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
