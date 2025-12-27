/*
  ==============================================================================

    PluginChain.cpp
    Created: 2025
    Author:  Zenith DAW

  ==============================================================================
*/

#include "PluginChain.h"
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

  // Process parameter automation
  for (const auto& binding : snapshot->bindings) {
    if (binding) binding->applyAutomation(numSamples);
  }

  for (const auto &plugin : snapshot->plugins) {
    if (plugin && !plugin->isSuspended()) {
      // Check if plugin has sidechain inputs and sidechain data is available
      const int numInputChannels = plugin->getTotalNumInputChannels();
      const int numMainInputs = 2; // Assuming stereo main

      if (sidechain != nullptr && numInputChannels > numMainInputs) {
        if (sidechainProxyBuffer_.getNumChannels() < numInputChannels || 
            sidechainProxyBuffer_.getNumSamples() < numSamples) {
            sidechainProxyBuffer_.setSize(numInputChannels, numSamples, false, true, true);
        }

        for (int i = 0;
             i < juce::jmin(buffer.getNumChannels(), numInputChannels); ++i)
          sidechainProxyBuffer_.copyFrom(i, 0, buffer, i, 0, numSamples);

        for (int i = 0; i < juce::jmin(sidechain->getNumChannels(),
                                       numInputChannels - numMainInputs);
             ++i)
          sidechainProxyBuffer_.copyFrom(numMainInputs + i, 0, *sidechain, i, 0,
                                     numSamples);

        plugin->processBlock(sidechainProxyBuffer_, midi);

        for (int i = 0; i < buffer.getNumChannels(); ++i)
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

  int maxChannels = 2;
  for (auto &p : pluginsOwned_) {
    p->prepareToPlay(sampleRate, blockSize);
    p->setNonRealtime(false);
    maxChannels = juce::jmax(maxChannels, p->getTotalNumInputChannels(), p->getTotalNumOutputChannels());
  }
  
  // Prepare bindings
  for (auto& b : bindingsOwned_) {
      b->prepare(sampleRate);
  }

  sidechainProxyBuffer_.setSize(maxChannels, blockSize, false, true, true);
}

void PluginChain::releaseResources() {
  for (auto &p : pluginsOwned_)
    p->releaseResources();
}

void PluginChain::updateSnapshot() {
  auto newSnapshot = std::make_shared<PluginSnapshot>(pluginsOwned_, bindingsOwned_);
  activeSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  
  snapshotTrash_.push_back(currentSnapshot_);
  currentSnapshot_ = newSnapshot;
  
  if (snapshotTrash_.size() > 10)
      snapshotTrash_.erase(snapshotTrash_.begin());
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

      if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param)) {
        auto range = ranged->getNormalisableRange();
        info.minValue = range.start;
        info.maxValue = range.end;
      }

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
