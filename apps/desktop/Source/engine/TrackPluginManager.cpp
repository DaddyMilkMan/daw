/*
  ==============================================================================

    TrackPluginManager.cpp
    Created: 2026-01-31
    Author:  Zenith DAW

  ==============================================================================
*/

#include "TrackPluginManager.h"
#include "TrackProcessor.h"

namespace zenith {

TrackPluginManager::TrackPluginManager(TrackProcessor &processor,
                                       double &sampleRate, int &blockSize)
    : processor_(processor), sampleRate_(sampleRate), blockSize_(blockSize) {}

void TrackPluginManager::addPlugin(
    std::unique_ptr<juce::AudioPluginInstance> plugin) {
  processor_.getPluginChain().addPlugin(std::move(plugin), sampleRate_,
                                        blockSize_);
}

void TrackPluginManager::removePlugin(int pluginIndex) {
  processor_.getPluginChain().removePlugin(pluginIndex);
}

void TrackPluginManager::clearPlugins() {
  processor_.getPluginChain().clearPlugins();
}

int TrackPluginManager::getNumPlugins() const {
  return processor_.getPluginChain().getNumPlugins();
}

juce::AudioPluginInstance *TrackPluginManager::getPlugin(int index) const {
  return processor_.getPluginChain().getPlugin(index);
}

std::vector<PluginChain::ParameterInfo>
TrackPluginManager::getPluginParameters(int pluginIndex) const {
  return processor_.getPluginChain().getAutomatableParameters(pluginIndex);
}

std::vector<PluginChain::ParameterInfo>
TrackPluginManager::getAllPluginParameters() const {
  return processor_.getPluginChain().getAllAutomatableParameters();
}

void TrackPluginManager::setPluginParameterValue(int pluginIndex, int paramIndex,
                                                 float normalizedValue) {
  processor_.getPluginChain().setParameterValue(pluginIndex, paramIndex,
                                                normalizedValue);
}

int TrackPluginManager::getPluginNumParameters(int pluginIndex) const {
  return processor_.getPluginChain().getNumParameters(pluginIndex);
}

juce::String TrackPluginManager::getPluginParameterName(int pluginIndex,
                                                        int paramIndex) const {
  return processor_.getPluginChain().getParameterName(pluginIndex, paramIndex);
}

} // namespace zenith
