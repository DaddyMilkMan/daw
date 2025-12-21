/*
  ==============================================================================

    ConsoleEmulation.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Analog console emulation for MixerChannel (Saturation, Crosstalk, Noise).

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>

namespace zenith {
namespace effects {

class ConsoleEmulation {
public:
  enum class Mode {
    Clean,
    Vintage, // Warm saturation + roll-off
    Modern,  // Slight saturation, bright
    Tube     // Heavy harmonics
  };

  ConsoleEmulation() = default;

  void prepare(juce::dsp::ProcessSpec &spec);
  void reset();

  // Process a block of audio (in-place)
  void process(juce::AudioBuffer<float> &buffer);

<<<<<<< HEAD
  void updateFilters();

  // Parameters
  void setMode(Mode newMode) {
    if (mode != newMode) {
      mode = newMode;
      updateFilters();
    }
  }
=======
  // Parameters
  void setMode(Mode newMode) { mode = newMode; }
>>>>>>> origin/master
  void setDrive(float newDrive) { drive = juce::jlimit(0.0f, 1.0f, newDrive); }
  void setCharacter(float newChar) {
    character = juce::jlimit(0.0f, 1.0f, newChar);
  }

private:
  Mode mode = Mode::Clean;
  float drive = 0.0f;     // 0.0 to 1.0
  float character = 0.0f; // 0.0 to 1.0 (mix or intensity)

  float sampleRate = 44100.0f;

  // Filters for tonal shaping
  juce::dsp::IIR::Filter<float> lowPass;
  juce::dsp::IIR::Filter<float> highPass;

  float applySaturation(float input, float driveAmount);
};

} // namespace effects
} // namespace zenith
