#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace zenith {

/**
 * @class PluginChain
 * @brief Manages a sequence of plugins with RT-safe snapshot pattern.
 */
class PluginChain {
public:
  PluginChain();
  ~PluginChain();

  // Message thread only
  void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin,
                 double sampleRate, int blockSize);
  void removePlugin(int index);
  void clearPlugins();

  int getNumPlugins() const;
  juce::AudioPluginInstance *getPlugin(int index) const;

  // Audio thread safe
  void process(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi);
  void prepareToPlay(double sampleRate, int blockSize);
  void releaseResources();

private:
  struct PluginSnapshot {
    std::vector<std::shared_ptr<juce::AudioPluginInstance>> plugins;

    PluginSnapshot() = default;
    explicit PluginSnapshot(
        const std::vector<std::shared_ptr<juce::AudioPluginInstance>>
            &ownedPlugins) {
      plugins.reserve(ownedPlugins.size());
      for (const auto &p : ownedPlugins)
        plugins.push_back(p);
    }
  };

  void updateSnapshot();

  std::vector<std::shared_ptr<juce::AudioPluginInstance>> pluginsOwned_;
  
  // Audio thread reads this (lock-free)
  std::atomic<PluginSnapshot*> activeSnapshot_{nullptr};
  // Message thread owns this
  std::shared_ptr<PluginSnapshot> currentSnapshot_;

  double currentSampleRate_ = 0;
  int currentBlockSize_ = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChain)
};

} // namespace zenith
