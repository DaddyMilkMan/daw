/*
  ==============================================================================

    ZenithTransientShaper.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Transient Shaper for controlling Attack and Sustain.

  ==============================================================================
*/

#pragma once

#include "../plugins/ZenithPlugin.h"
#include <juce_dsp/juce_dsp.h>

namespace zenith {

class ZenithTransientShaper : public ZenithPlugin {
public:
  ZenithTransientShaper();
  ~ZenithTransientShaper() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  const juce::String getName() const override {
    return "Zenith Transient Shaper";
  }

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

private:
  std::atomic<float> *attackGain = nullptr;
  std::atomic<float> *sustainGain = nullptr;

  // Envelope followers
  // We use two followers with different constants to detect transients
  float fastEnvelope[2] = {0.0f, 0.0f}; // Stereo
  float slowEnvelope[2] = {0.0f, 0.0f};

  float sampleRate_ = 44100.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTransientShaper)
};

} // namespace zenith
