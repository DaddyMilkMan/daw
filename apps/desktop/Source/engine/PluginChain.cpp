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

void PluginChain::insertPluginAt(
    int index, std::unique_ptr<juce::AudioPluginInstance> plugin,
    double sampleRate, int blockSize) {
  if (!plugin)
    return;

  auto sharedPlugin = std::shared_ptr<juce::AudioPluginInstance>(plugin.release());
  if (sampleRate > 0) {
    sharedPlugin->prepareToPlay(sampleRate, blockSize);
    sharedPlugin->setNonRealtime(false);
  }

  const int clampedIndex = juce::jlimit(0, static_cast<int>(pluginsOwned_.size()), index);
  pluginsOwned_.insert(pluginsOwned_.begin() + clampedIndex, std::move(sharedPlugin));
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
        auto* scBuffer = snapshot->sidechainBuffer.get();
        
        // RT-Safe Check: Ensure we have a valid buffer large enough for this plugin
        // Due to "High Watermark" allocation in updateSnapshot, this should always be true
        // unless the plugin dynamicallly increased channel count beyond expectation (unsupported by this safety fixes without lock)
        if (scBuffer != nullptr && 
            scBuffer->getNumChannels() >= numTotalInputChannels && 
            scBuffer->getNumSamples() >= numSamples) {
            
            // Create a stack-based proxy buffer that views the pre-allocated memory
            // with the correct number of channels for this specific plugin
            juce::AudioBuffer<float> proxy(scBuffer->getArrayOfWritePointers(), 
                                         numTotalInputChannels, 
                                         numSamples);

            // Copy main channels
            for (int i = 0; i < numMainChannels; ++i)
              proxy.copyFrom(i, 0, buffer, i, 0, numSamples);
              
            // Copy sidechain channels
            const int numSidechainChansToCopy = juce::jmin(sidechain->getNumChannels(),
                                                           numTotalInputChannels - numMainChannels);
            
            for (int i = 0; i < numSidechainChansToCopy; ++i)
              proxy.copyFrom(numMainChannels + i, 0, *sidechain, i, 0, numSamples);
                                         
            // Clear any remaining unconnected channels to ensure clean inputs
            for (int i = numMainChannels + numSidechainChansToCopy; i < numTotalInputChannels; ++i)
               proxy.clear(i, 0, numSamples);

            plugin->processBlock(proxy, midi);

            // Copy back main channels (outputs)
            for (int i = 0; i < numMainChannels; ++i)
              buffer.copyFrom(i, 0, proxy, i, 0, numSamples);

        } else {
             // Fallback: Buffer too small (should not happen with correct prepareToPlay/updateSnapshot logic)
             // Process without sidechain to avoid crash/alloc
             plugin->processBlock(buffer, midi);
        }
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

  // Update snapshot to ensure sidechain buffers are resized if maxChannels increased
  updateSnapshot();
}

void PluginChain::releaseResources() {
  for (auto &p : pluginsOwned_)
    p->releaseResources();
}

void PluginChain::updateSnapshot() {
  // 1. Calculate requirements
  int maxChannels = 2; 
  // Ensure we allocate at least enough for typical usage, or respect current block size
  int requiredSamples = currentBlockSize_ > 0 ? currentBlockSize_ : 4096;

  for (const auto& p : pluginsOwned_) {
     // Check both input and output count to be safe, though sidechain is usually input-focused
     maxChannels = std::max(maxChannels, p->getTotalNumInputChannels());
  }

  // 2. High Watermark Allocation Strategy
  std::shared_ptr<juce::AudioBuffer<float>> chainBuffer;
  int currentCapacityChannels = 0;
  int currentCapacitySamples = 0;

  if (currentSnapshot_ && currentSnapshot_->sidechainBuffer) {
      currentCapacityChannels = currentSnapshot_->sidechainBuffer->getNumChannels();
      currentCapacitySamples = currentSnapshot_->sidechainBuffer->getNumSamples();
  }

  // "High Watermark": Capacity only grows.
  int targetChannels = std::max(maxChannels, currentCapacityChannels);
  int targetSamples = std::max(requiredSamples, currentCapacitySamples);

  bool reallocate = true;
  if (currentSnapshot_ && currentSnapshot_->sidechainBuffer) {
      // If the current buffer meets the TARGET requirements (which it should if it defines the capacity), reuse it.
      // But if target > current, we must reallocate.
      if (currentCapacityChannels >= targetChannels && currentCapacitySamples >= targetSamples) {
          chainBuffer = currentSnapshot_->sidechainBuffer;
          reallocate = false;
      }
  }

  if (reallocate) {
      chainBuffer = std::make_shared<juce::AudioBuffer<float>>(targetChannels, targetSamples);
      chainBuffer->clear();
  }

  auto newSnapshot = std::make_shared<PluginSnapshot>(pluginsOwned_, bindingsOwned_, chainBuffer);
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
