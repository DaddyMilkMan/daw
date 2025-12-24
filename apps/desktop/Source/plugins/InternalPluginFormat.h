/*
  ==============================================================================

    InternalPluginFormat.h
    Created: 2025-12-24
    Author:  Zenith DAW

    Format for hosting internal Zenith processors and instruments.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

class InternalPluginFormat : public juce::AudioPluginFormat {
public:
  InternalPluginFormat();
  ~InternalPluginFormat() override;

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
  bool doesPluginStillExist(const juce::PluginDescription &) override {
    return true;
  }
  juce::StringArray searchPathsForPlugins(const juce::FileSearchPath &, bool,
                                          bool) override {
    return {};
  }
  juce::FileSearchPath getDefaultLocationsToSearch() override { return {}; }

protected:
  void createPluginInstance(const juce::PluginDescription &description,
                            double initialSampleRate, int initialBufferSize,
                            PluginCreationCallback callback) override;

  bool requiresUnblockedMessageThreadDuringCreation(
      const juce::PluginDescription &) const override {
    return false;
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternalPluginFormat)
};

} // namespace zenith
