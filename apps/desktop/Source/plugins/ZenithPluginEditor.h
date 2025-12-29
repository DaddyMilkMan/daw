/*
  ==============================================================================

    ZenithPluginEditor.h
    Created: 2025-12-19
    Author:  Zenith DAW

    Standard UI container for Zenith Plugins.
    Provides a header with Title, Bypass, and generic styling.

  ==============================================================================
*/

#pragma once

#include "ZenithPlugin.h"
#include <juce_audio_processors/juce_audio_processors.h>


namespace zenith {

class ZenithPluginEditor : public juce::AudioProcessorEditor {
public:
  explicit ZenithPluginEditor(ZenithPlugin &p);
  ~ZenithPluginEditor() override;

  //==============================================================================
  void paint(juce::Graphics &g) override;
  void resized() override;

protected:
  ZenithPlugin &pluginProc;

  // Future: Common header components (Title, Bypass Button)
  // juce::Label titleLabel;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPluginEditor)
};

} // namespace zenith
