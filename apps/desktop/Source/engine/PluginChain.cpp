/*
  ==============================================================================

    PluginChain.cpp
    Created: 2025
    Author:  Zenith DAW

  ==============================================================================
*/

#include "PluginChain.h"
#include "RealTimeGarbageCollector.h"
#include <algorithm>

namespace zenith {

PluginChain::PluginChain() {
  currentSnapshot_ = std::make_shared<PluginSnapshot>();
  activeSnapshot_.store(currentSnapshot_.get());
}

PluginChain::~PluginChain() {
  clearPlugins();
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
    auto* pluginPtr = pluginsOwned_[index].get();

    // Remove bindings for this plugin
    bindingsOwned_.erase(
        std::remove_if(bindingsOwned_.begin(), bindingsOwned_.end(),
            [pluginPtr](const auto& b) { return b->getPlugin() == pluginPtr; }),
        bindingsOwned_.end());

    pluginsOwned_[index]->releaseResources();
    pluginsOwned_.erase(pluginsOwned_.begin() + index);
    updateSnapshot();
  }
}

void PluginChain::clearPlugins() {
  bindingsOwned_.clear();
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
                          juce::MidiBuffer &midi,
                          const juce::AudioBuffer<float> *sidechain) {
  const PluginSnapshot *snapshot =
      activeSnapshot_.load(std::memory_order_acquire);
  if (!snapshot)
    return;

  const int numSamples = buffer.getNumSamples();
  const int numMainChannels = buffer.getNumChannels();

  // Process parameter automation
  for (const auto& binding : snapshot->bindings) {
    if (binding) binding->applyAutomation(numSamples);
  }

  for (const auto &plugin : snapshot->plugins) {
    if (plugin && !plugin->isSuspended()) {
      // Check if plugin has sidechain inputs and sidechain data is available
      const int numTotalInputChannels = plugin->getTotalNumInputChannels();
      
      // If the plugin has more inputs than our main bus, assume the rest are sidechain/aux
      if (sidechain != nullptr && numTotalInputChannels > numMainChannels) {
        // Ensure proxy buffer is big enough (try to resize without reallocating)
        if (sidechainProxyBuffer_.getNumChannels() < numTotalInputChannels || 
            sidechainProxyBuffer_.getNumSamples() < numSamples) {
            sidechainProxyBuffer_.setSize(numTotalInputChannels, numSamples, false, true, true);
        }

        // Safety clamp: If resize failed (due to no allocation allowed), we must not write beyond bounds
        const int validProxyChannels = sidechainProxyBuffer_.getNumChannels();

        // Copy main channels (clamped)
        const int numMainToCopy = juce::jmin(numMainChannels, validProxyChannels);
        for (int i = 0; i < numMainToCopy; ++i)
          sidechainProxyBuffer_.copyFrom(i, 0, buffer, i, 0, numSamples);
          
        // Copy sidechain channels
        const int remainingProxyChannels = validProxyChannels - numMainChannels;
        if (remainingProxyChannels > 0) {
            const int numSidechainChansToCopy = juce::jmin(sidechain->getNumChannels(),
                                                           numTotalInputChannels - numMainChannels);
            const int safeSidechainCopyCount = juce::jmin(numSidechainChansToCopy, remainingProxyChannels);
            
            for (int i = 0; i < safeSidechainCopyCount; ++i)
              sidechainProxyBuffer_.copyFrom(numMainChannels + i, 0, *sidechain, i, 0,
                                         numSamples);
                                         
            // Clear any remaining unconnected channels
            for (int i = numMainChannels + safeSidechainCopyCount; i < validProxyChannels; ++i)
               sidechainProxyBuffer_.clear(i, 0, numSamples);
        }

        plugin->processBlock(sidechainProxyBuffer_, midi);

        // Copy back main channels
        for (int i = 0; i < numMainToCopy; ++i)
          buffer.copyFrom(i, 0, sidechainProxyBuffer_, i, 0, numSamples);
      } else {
        plugin->processBlock(buffer, midi);
      }
    }
  }
}

void PluginChain::prepareToPlay(double sampleRate, int blockSize) {
  currentSampleRate_ = sampleRate;
  currentBlockSize_ = blockSize;

  // Initial maxChannels based on current configuration if known, or start small and grow
  int maxChannels = 2; // Default minimum
  for (auto &p : pluginsOwned_) {
    p->prepareToPlay(sampleRate, blockSize);
    p->setNonRealtime(false);
    maxChannels = juce::jmax(maxChannels, p->getTotalNumInputChannels(), p->getTotalNumOutputChannels());
  }
  
  // Prepare bindings
  for (auto& b : bindingsOwned_) {
      b->prepare(sampleRate);
  }

  // Pre-allocate to global max channels (32) to prevent allocation in process()
  int allocationChannels = std::max(32, maxChannels);
  sidechainProxyBuffer_.setSize(allocationChannels, blockSize);
  sidechainProxyBuffer_.clear();
}

void PluginChain::releaseResources() {
  for (auto &p : pluginsOwned_)
    p->releaseResources();
}

void PluginChain::updateSnapshot() {
  auto newSnapshot = std::make_shared<PluginSnapshot>(pluginsOwned_, bindingsOwned_);
  activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  
  RealTimeGarbageCollector::getInstance().deferDelete(currentSnapshot_);
  currentSnapshot_ = newSnapshot;
}

std::shared_ptr<PluginAutomationBinding> PluginChain::findBinding(int pluginIndex, int paramIndex) {
    auto* plugin = getPlugin(pluginIndex);
    if (!plugin) return nullptr;
    
    for (const auto& b : bindingsOwned_) {
        if (b->getPlugin() == plugin && b->getParameterIndex() == paramIndex)
            return b;
    }
    return nullptr;
}

//==============================================================================
// Plugin Parameter Automation Support
//==============================================================================

int PluginChain::getNumParameters(int pluginIndex) const {
  if (auto* plugin = getPlugin(pluginIndex)) {
    return static_cast<int>(plugin->getParameters().size());
  }
  return 0;
}

juce::AudioProcessorParameter* PluginChain::getParameter(int pluginIndex,
                                                          int paramIndex) const {
  if (auto* plugin = getPlugin(pluginIndex)) {
    auto& params = plugin->getParameters();
    if (paramIndex >= 0 && paramIndex < static_cast<int>(params.size())) {
      return params[paramIndex];
    }
  }
  return nullptr;
}

juce::String PluginChain::getParameterName(int pluginIndex, int paramIndex) const {
  if (auto* param = getParameter(pluginIndex, paramIndex)) {
    return param->getName(64);
  }
  return {};
}

void PluginChain::setParameterValue(int pluginIndex, int paramIndex,
                                     float normalizedValue) {
    auto binding = findBinding(pluginIndex, paramIndex);
    if (binding) {
        binding->setTargetValue(normalizedValue);
    } else {
        auto* plugin = getPlugin(pluginIndex);
        if (plugin) {
            auto newBinding = std::make_shared<PluginAutomationBinding>(plugin, paramIndex);
            newBinding->prepare(currentSampleRate_ > 0 ? currentSampleRate_ : 44100.0);
            newBinding->setTargetValue(normalizedValue);
            bindingsOwned_.push_back(newBinding);
            updateSnapshot();
        }
    }
}

std::vector<PluginChain::ParameterInfo>
PluginChain::getAutomatableParameters(int pluginIndex) const {
  std::vector<ParameterInfo> result;

  if (auto* plugin = getPlugin(pluginIndex)) {
    auto& params = plugin->getParameters();
    result.reserve(params.size());

    for (int i = 0; i < static_cast<int>(params.size()); ++i) {
      auto* param = params[i];
      if (param == nullptr)
        continue;

      if (param->isMetaParameter())
        continue;

      ParameterInfo info;
      info.pluginIndex = pluginIndex;
      info.paramIndex = i;
      info.name = param->getName(64);
      info.label = param->getLabel();
      info.defaultValue = param->getDefaultValue();

      // Without RTTI (-fno-rtti), we cannot use dynamic_cast.
      // Assume normalized 0-1 range for all parameters.
      info.minValue = 0.0f;
      info.maxValue = 1.0f;

      result.push_back(info);
    }
  }

  return result;
}

std::vector<PluginChain::ParameterInfo>
PluginChain::getAllAutomatableParameters() const {
  std::vector<ParameterInfo> result;

  for (int pluginIdx = 0; pluginIdx < getNumPlugins(); ++pluginIdx) {
    auto pluginParams = getAutomatableParameters(pluginIdx);
    result.insert(result.end(), pluginParams.begin(), pluginParams.end());
  }

  return result;
}

} // namespace zenith
