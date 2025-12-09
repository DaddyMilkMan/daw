/*
  ==============================================================================

    SettingsComponent.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

    Implementation of the comprehensive Settings Panel.
  ==============================================================================
*/

#include "../../Source/ui/SettingsComponent.h"
#include "../../Source/engine/PluginHost.h"
#include "../../Source/ui/ZenithLookAndFeel.h"
#include "../../include/Engine.h"
#include "../../include/ui/SettingsComponent.h" // For the main SettingsComponent class

namespace zenith {

//==============================================================================
// AudioSettingsTab Implementation
//==============================================================================
// Implemented inline in header for simplicity.

//==============================================================================
// MidiSettingsTab Implementation
//==============================================================================
// Implemented inline in header for simplicity.

//==============================================================================
// PluginSettingsTab Implementation
//==============================================================================
// Implemented inline in header for simplicity.

//==============================================================================
// DisplaySettingsTab Implementation
//==============================================================================
// Implemented inline in header for simplicity.

//==============================================================================
// AiSettingsTab Implementation
//==============================================================================
// Implemented inline in header for simplicity.

//==============================================================================
// ColorPickerButton Implementation
//==============================================================================
// Implemented inline in header for simplicity.

//==============================================================================
// AppearanceSettingsTab Implementation
//==============================================================================
// Implemented inline in header for simplicity.

//==============================================================================
// SettingsComponent Implementation
//==============================================================================

SettingsComponent::SettingsComponent(Engine& engine)
    : audioTab(engine), 
      midiTab(engine), 
      pluginTab(engine.getPluginHost()), 
      displayTab(),
      aiTab(),
      appearanceTab()
{
    setLookAndFeel(&ZenithLookAndFeel::getInstance());
    addAndMakeVisible(tabs);
    tabs.addTab("Audio", juce::Colours::darkgrey, &audioTab, false);
    tabs.addTab("MIDI", juce::Colours::darkgrey, &midiTab, false);
    tabs.addTab("Plugins", juce::Colours::darkgrey, &pluginTab, false);
    tabs.addTab("Display", juce::Colours::darkgrey, &displayTab, false);
    tabs.addTab("Appearance", juce::Colours::darkgrey, &appearanceTab, false);
    tabs.addTab("AI", juce::Colours::darkgrey, &aiTab, false);
}

SettingsComponent::~SettingsComponent() {
    setLookAndFeel(nullptr);
}

void SettingsComponent::resized() {
    tabs.setBounds(getLocalBounds());
}

} // namespace zenith
