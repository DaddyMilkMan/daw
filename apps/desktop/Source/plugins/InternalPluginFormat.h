/*
  ==============================================================================

    InternalPluginFormat.h
    Created: 2025-12-19
    Author:  Zenith DAW

    Custom JUCE Plugin Format to host internal Zenith plugins (EQ, Comp, etc.)
    as if they were VSTs.

  ==============================================================================
*/

#pragma once

#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <memory>


namespace zenith {

class InternalPluginFormat : public juce::AudioPluginFormat {
public:
  InternalPluginFormat();
  ~InternalPluginFormat() override;

  //==============================================================================
  juce::String getName() const override { return "Zenith Internal"; }
  bool canScanForPlugins() const override { return false; }
  bool isTrivialToScan() const override { return true; }

  void findAllTypesForFile(juce::OwnedArray<juce::PluginDescription> &results,
                           const juce::String &fileOrIdentifier) override;

  bool
  fileMightContainThisPluginType(const juce::String &fileOrIdentifier) override;

  juce::String
  getNameOfPluginFromIdentifier(const juce::String &fileOrIdentifier) override;

  bool pluginNeedsRescanning(const juce::PluginDescription &) override {
    return false;
  }

  bool doesPluginStillExist(const juce::PluginDescription &desc) override;

  juce::StringArray searchPathsForPlugins(const juce::FileSearchPath &, bool,
                                          bool) override {
    return {};
  }
  juce::FileSearchPath getDefaultLocationsToSearch() override { return {}; }

  void createPluginInstance(const juce::PluginDescription &desc,
                            double initialSampleRate, int initialBlockSize,
                            PluginCreationCallback callback) override;

  // Synchronous creation (optional but good to keep)
  std::unique_ptr<juce::AudioPluginInstance>
  createInstanceFromDescription(const juce::PluginDescription &desc,
                                double initialSampleRate, int initialBlockSize,
                                juce::String &errorMessage);

  bool requiresUnblockedMessageThreadDuringCreation(
      const juce::PluginDescription &) const override {
    return false;
  }

  //==============================================================================
  // Registration Helpers
  using Output = std::unique_ptr<juce::AudioPluginInstance>;
  using Creator = std::function<Output()>;

  void registerPlugin(const juce::String &identifier, Creator creator);

  // Static singleton access for registration from other translation units
  static InternalPluginFormat &getInstance();

private:
  std::map<juce::String, Creator> creators;
  std::map<juce::String, juce::PluginDescription> descriptions;

  void registerBuiltInPlugins(); // Called in ctor

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternalPluginFormat)
};

} // namespace zenith
