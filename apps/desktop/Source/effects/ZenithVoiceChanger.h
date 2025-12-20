/*
  ==============================================================================

    ZenithVoiceChanger.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Plugin wrapper for DSPVoiceChanger.

  ==============================================================================
*/

#pragma once

#include "../dsp/DSPVoiceChanger.h"
#include "../plugins/ZenithPlugin.h"


namespace zenith {

class ZenithVoiceChanger : public ZenithPlugin {
public:
  ZenithVoiceChanger();
  ~ZenithVoiceChanger() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  const juce::String getName() const override { return "Zenith Voice Changer"; }

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

private:
  std::atomic<float> *characterParam = nullptr;

  DSPVoiceChanger voiceChanger;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithVoiceChanger)
};

} // namespace zenith
