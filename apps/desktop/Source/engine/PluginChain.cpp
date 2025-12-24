#include "PluginChain.h"
#include "RealTimeGarbageCollector.h"

namespace zenith {

PluginChain::PluginChain() {
  currentSnapshot_ = std::make_shared<PluginSnapshot>();
  activeSnapshot_.store(currentSnapshot_.get(), std::memory_order_release);
}

PluginChain::~PluginChain() {
  pluginsOwned_.clear();
  // No need to clear activeSnapshot_ atomic, but we should ensure currentSnapshot_ is released
  // safely if other threads are accessing it? 
  // Destructor should only run when no other threads are accessing this object.
  activeSnapshot_.store(nullptr);
}

void PluginChain::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin,
                            double sampleRate, int blockSize) {
  if (!plugin)
    return;

  auto sharedPlugin =
      std::shared_ptr<juce::AudioPluginInstance>(plugin.release());
  if (sampleRate > 0) {
    sharedPlugin->prepareToPlay(sampleRate, blockSize);
    sharedPlugin->setNonRealtime(false);
  }

  pluginsOwned_.push_back(sharedPlugin);
  updateSnapshot();
}

void PluginChain::removePlugin(int index) {
  if (index >= 0 && index < (int)pluginsOwned_.size()) {
    pluginsOwned_[index]->releaseResources();
    pluginsOwned_.erase(pluginsOwned_.begin() + index);
    updateSnapshot();
  }
}

void PluginChain::clearPlugins() {
  for (auto &p : pluginsOwned_)
    p->releaseResources();
  pluginsOwned_.clear();
  updateSnapshot();
}

int PluginChain::getNumPlugins() const { return (int)pluginsOwned_.size(); }

juce::AudioPluginInstance *PluginChain::getPlugin(int index) const {
  if (index >= 0 && index < (int)pluginsOwned_.size())
    return pluginsOwned_[index].get();
  return nullptr;
}

void PluginChain::process(juce::AudioBuffer<float> &buffer,
                          juce::MidiBuffer &midi) {
  auto *snapshot = activeSnapshot_.load(std::memory_order_acquire);
  if (!snapshot)
    return;

  for (const auto &plugin : snapshot->plugins) {
    if (plugin && !plugin->isSuspended()) {
      plugin->processBlock(buffer, midi);
    }
  }
}

void PluginChain::prepareToPlay(double sampleRate, int blockSize) {
  currentSampleRate_ = sampleRate;
  currentBlockSize_ = blockSize;

  for (auto &p : pluginsOwned_) {
    p->prepareToPlay(sampleRate, blockSize);
    p->setNonRealtime(false);
  }
}

void PluginChain::releaseResources() {
  for (auto &p : pluginsOwned_)
    p->releaseResources();
}

void PluginChain::updateSnapshot() {
  auto newSnapshot = std::make_shared<PluginSnapshot>(pluginsOwned_);
  
  // Update atomic pointer
  activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  
  // Defer deletion of old snapshot
  if (currentSnapshot_) {
    RealTimeGarbageCollector::getInstance().push(currentSnapshot_);
  }
  
  currentSnapshot_ = newSnapshot;
}

} // namespace zenith
