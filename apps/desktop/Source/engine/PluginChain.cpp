#include "PluginChain.h"

namespace zenith {

PluginChain::PluginChain() {
    currentSnapshot_ = std::make_shared<PluginSnapshot>();
    std::atomic_store_explicit(&activeSnapshot_, currentSnapshot_, std::memory_order_release);
}

PluginChain::~PluginChain() {
    releaseResources();
    pluginsOwned_.clear();
    currentSnapshot_ = std::make_shared<PluginSnapshot>();
    std::atomic_store_explicit(&activeSnapshot_, currentSnapshot_, std::memory_order_release);
}

void PluginChain::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin, double sampleRate, int blockSize) {
    if (!plugin) return;
    
    // Convert generic unique_ptr to shared_ptr
    std::shared_ptr<juce::AudioPluginInstance> sharedPlugin(plugin.release());
    
    if (sampleRate > 0) {
        sharedPlugin->prepareToPlay(sampleRate, blockSize);
        sharedPlugin->setNonRealtime(false);
    }
    
    pluginsOwned_.push_back(sharedPlugin);
    updateSnapshot();
}

void PluginChain::removePlugin(int index) {
    if (index >= 0 && index < (int)pluginsOwned_.size()) {
        auto plugin = pluginsOwned_[index];
        if (plugin) {
            plugin->releaseResources();
        }
        pluginsOwned_.erase(pluginsOwned_.begin() + index);
        updateSnapshot();
    }
}

void PluginChain::clearPlugins() {
    for (auto& p : pluginsOwned_) {
        if (p) p->releaseResources();
    }
    pluginsOwned_.clear();
    updateSnapshot();
}

int PluginChain::getNumPlugins() const { 
    return (int)pluginsOwned_.size(); 
}

juce::AudioPluginInstance* PluginChain::getPlugin(int index) const {
    if (index >= 0 && index < (int)pluginsOwned_.size()) {
        return pluginsOwned_[index].get();
    }
    return nullptr;
}

void PluginChain::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    // Load the shared_ptr atomically (thread-safe, acquires ownership)
    auto snapshot = std::atomic_load_explicit(&activeSnapshot_, std::memory_order_acquire);
    
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
    for (auto& p : pluginsOwned_) {
        p->releaseResources();
    }
}

void PluginChain::updateSnapshot() {
    // Create new snapshot from owned plugins
    auto newSnapshot = std::make_shared<PluginSnapshot>(pluginsOwned_);
    
    // Store as active snapshot atomically
    std::atomic_store_explicit(&activeSnapshot_, newSnapshot, std::memory_order_release);
    
    // Keep reference to prevent immediate deletion
    currentSnapshot_ = newSnapshot;
    
    // Simple trash management to keep objects alive while audio thread might be using them
    snapshotTrash_.push_back(std::atomic_load_explicit(&activeSnapshot_, std::memory_order_relaxed));
    if (snapshotTrash_.size() > 5) {
        snapshotTrash_.erase(snapshotTrash_.begin());
    }
}

} // namespace zenith
