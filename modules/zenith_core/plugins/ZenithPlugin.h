/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// ZenithPlugin.h


#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

class ZenithPlugin : public juce::AudioPluginInstance {
public:
  //==============================================================================
  explicit ZenithPlugin(
      juce::AudioProcessorValueTreeState::ParameterLayout layout);
  ~ZenithPlugin() override;

  //==============================================================================
  // Standard AudioProcessor Overrides
  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  // Derived classes implement processBlock, but we provide helpers
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override = 0;

  //==============================================================================
  // Editor (UI)
  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }

  //==============================================================================
  // Persistence
  //==============================================================================
  const juce::String
  getName() const override = 0; // Pure virtual: plugins must name themselves

  void
  fillInPluginDescription(juce::PluginDescription &description) const override {
    description.name = getName();
    description.descriptiveName = getName();
    description.pluginFormatName = "Zenith Internal";
    description.category = "Effect";
    description.manufacturerName = "Zenith DAW";
    description.version = "1.0.0";
    description.fileOrIdentifier =
        "zenith.internal." + getName().replace(" ", "").toLowerCase();
    description.uniqueId = description.fileOrIdentifier.hashCode();
    description.isInstrument = false;
  }

  bool acceptsMidi() const override {
    return true;
  } // Default to true for robust chaining
  bool producesMidi() const override { return true; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override {}
  const juce::String getProgramName(int index) override { return {}; }
  void changeProgramName(int index, const juce::String &newName) override {}

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==============================================================================
  // Zenith Specific helpers
  //==============================================================================
  juce::AudioProcessorValueTreeState &getAPVTS() { return apvts; }
  const juce::AudioProcessorValueTreeState &getAPVTS() const { return apvts; }

protected:
  // Derived classes must pass layout to constructor
  // virtual createParameterLayout() removed to prevent ctor virtual call issues

  // The state!
  // Note: We initialize this in the constructor body after
  // createParameterLayout is ready
  juce::AudioProcessorValueTreeState apvts;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPlugin)
};

} // namespace zenith
