/*
  ==============================================================================

    TrackPluginManager.h
    Created: 2026-01-31
    Author:  Zenith DAW

    Plugin chain management for Track.

  ==============================================================================
*/

#pragma once

#include "PluginChain.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

namespace zenith {

class TrackProcessor;

/**
 * @brief Owns plugin chain management for a Track.
 */
class TrackPluginManager {
public:
  TrackPluginManager(TrackProcessor &processor, double &sampleRate,
                     int &blockSize);

  void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
  void removePlugin(int pluginIndex);
  void clearPlugins();
  int getNumPlugins() const;
  juce::AudioPluginInstance *getPlugin(int index) const;

  std::vector<PluginChain::ParameterInfo> getPluginParameters(int pluginIndex) const;
  std::vector<PluginChain::ParameterInfo> getAllPluginParameters() const;
  void setPluginParameterValue(int pluginIndex, int paramIndex,
                               float normalizedValue);
  int getPluginNumParameters(int pluginIndex) const;
  juce::String getPluginParameterName(int pluginIndex, int paramIndex) const;

private:
  TrackProcessor &processor_;
  double &sampleRate_;
  int &blockSize_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackPluginManager)
};

} // namespace zenith
