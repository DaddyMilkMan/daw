#include "PluginChain.h"

namespace zenith {

PluginChain::PluginChain() {
    currentSnapshot_ = std::make_shared<PluginSnapshot>();
    activeSnapshot_.store(currentSnapshot_.get());
}

PluginChain::~PluginChain() {
    pluginsOwned_.clear();
    currentSnapshot_ = std::make_shared<PluginSnapshot>();
    activeSnapshot_.store(currentSnapshot_.get());
}

void PluginChain::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin, double sampleRate, int blockSize) {
    if (!plugin) return;
    
    auto sharedPlugin = std::shared_ptr<juce::AudioPluginInstance>(plugin.release());
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
    for (auto& p : pluginsOwned_) p->releaseResources();
    pluginsOwned_.clear();
    updateSnapshot();
}

int PluginChain::getNumPlugins() const { return (int)pluginsOwned_.size(); }

juce::AudioPluginInstance* PluginChain::getPlugin(int index) const {
    if (index >= 0 && index < (int)pluginsOwned_.size()) return pluginsOwned_[index].get();
    return nullptr;
}

void PluginChain::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    const PluginSnapshot* snapshot = activeSnapshot_.load(std::memory_order_acquire);
    if (!snapshot) return;
    
    for (const auto& plugin : snapshot->plugins) {
        if (plugin && !plugin->isSuspended()) {
            plugin->processBlock(buffer, midi);
        }
    }
}

void PluginChain::prepareToPlay(double sampleRate, int blockSize) {
    currentSampleRate_ = sampleRate;
    currentBlockSize_ = blockSize;
    
    for (auto& p : pluginsOwned_) {
        p->prepareToPlay(sampleRate, blockSize);
        p->setNonRealtime(false);
    }
}

void PluginChain::releaseResources() {
    for (auto& p : pluginsOwned_) p->releaseResources();
}

void PluginChain::updateSnapshot() {
    auto newSnapshot = std::make_shared<PluginSnapshot>(pluginsOwned_);
    activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
    snapshotTrash_.push_back(currentSnapshot_);
    currentSnapshot_ = newSnapshot;
    
    while (snapshotTrash_.size() > 10) {
        snapshotTrash_.erase(snapshotTrash_.begin());
    }
}

} // namespace zenith
