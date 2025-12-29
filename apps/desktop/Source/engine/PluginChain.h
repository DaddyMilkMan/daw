#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <string>
#include <vector>
#include "PluginAutomationBinding.h"

namespace zenith {

// Forward declaration for test friend
/**
 * @class PluginChain
 * @brief Manages a sequence of plugins with RT-safe snapshot pattern.
 */
// Forward declare test class
namespace tests { class PluginBufferingTests; }

class PluginChain {
  friend class ::zenith::tests::PluginBufferingTests;
public:
  //============================================================================
  /**
   * @struct ParameterInfo
   * @brief Information about an automatable plugin parameter
   */
  struct ParameterInfo {
    int pluginIndex = -1;
    int paramIndex = -1;
    juce::String name;
    juce::String label;
    float defaultValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
  };

  //============================================================================
  PluginChain();
  ~PluginChain();

  // Message thread only
  void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin,
                 double sampleRate, int blockSize);
  void removePlugin(int index);
  void clearPlugins();

  int getNumPlugins() const;
  juce::AudioPluginInstance *getPlugin(int index) const;
  const std::vector<std::shared_ptr<juce::AudioPluginInstance>>& getPlugins() const { return pluginsOwned_; }

  // Audio thread safe
  void process(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi,
               const juce::AudioBuffer<float> *sidechain = nullptr);
  void prepareToPlay(double sampleRate, int blockSize);
  void releaseResources();

  //============================================================================
  // Plugin Parameter Automation Support
  //============================================================================

  /**
   * @brief Get number of parameters for a plugin
   * @param pluginIndex Index of the plugin in the chain
   * @return Number of parameters, or 0 if invalid
   */
  int getNumParameters(int pluginIndex) const;

  /**
   * @brief Get parameter object
   * @param pluginIndex Index of the plugin
   * @param paramIndex Index of the parameter
   * @return Pointer to parameter, or nullptr if invalid
   */
  juce::AudioProcessorParameter* getParameter(int pluginIndex, int paramIndex) const;

  /**
   * @brief Get parameter name
   * @param pluginIndex Index of the plugin
   * @param paramIndex Index of the parameter
   * @return Parameter name, or empty string if invalid
   */
  juce::String getParameterName(int pluginIndex, int paramIndex) const;

  /**
   * @brief Set parameter value (MESSAGE THREAD, RT-safe delivery)
   * @param pluginIndex Index of the plugin
   * @param paramIndex Index of the parameter
   * @param normalizedValue Value in range [0.0, 1.0]
   * @note Uses setValueNotifyingHost for proper plugin notification
   */
  void setParameterValue(int pluginIndex, int paramIndex, float normalizedValue);

  /**
   * @brief Get all automatable parameters for a plugin
   * @param pluginIndex Index of the plugin
   * @return Vector of ParameterInfo for each automatable parameter
   */
  std::vector<ParameterInfo> getAutomatableParameters(int pluginIndex) const;

  /**
   * @brief Get all automatable parameters for all plugins in the chain
   * @return Vector of ParameterInfo for all automatable parameters
   */
  std::vector<ParameterInfo> getAllAutomatableParameters() const;

private:
  struct PluginSnapshot {
    std::vector<std::shared_ptr<juce::AudioPluginInstance>> plugins;
    
    std::vector<std::shared_ptr<PluginAutomationBinding>> bindings;
    std::shared_ptr<juce::AudioBuffer<float>> sidechainBuffer;

    PluginSnapshot() = default;
    explicit PluginSnapshot(
        const std::vector<std::shared_ptr<juce::AudioPluginInstance>>& ownedPlugins,
        const std::vector<std::shared_ptr<PluginAutomationBinding>>& ownedBindings,
        std::shared_ptr<juce::AudioBuffer<float>> buffer) 
      : sidechainBuffer(std::move(buffer)) {
      plugins.reserve(ownedPlugins.size());
      for (const auto &p : ownedPlugins)
        plugins.push_back(p);
        
      bindings.reserve(ownedBindings.size());
      for (const auto &b : ownedBindings)
        bindings.push_back(b);
    }
  };

  void updateSnapshot();

  std::vector<std::shared_ptr<juce::AudioPluginInstance>> pluginsOwned_;
  std::vector<std::shared_ptr<PluginAutomationBinding>> bindingsOwned_; // Master list of bindings
  
  std::atomic<const PluginSnapshot *> activeSnapshot_{nullptr};
  std::shared_ptr<PluginSnapshot> currentSnapshot_;

  double currentSampleRate_ = 0;
  int currentBlockSize_ = 0;
  
  // Helper to find existing binding
  std::shared_ptr<PluginAutomationBinding> findBinding(int pluginIndex, int paramIndex);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChain)
};


} // namespace zenith
