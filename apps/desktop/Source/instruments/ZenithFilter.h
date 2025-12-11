/*
  ==============================================================================

    ZenithFilter.h
    Created: 2025-12-06
    Author:  Zenith DAW

    Filter component for ZenithPolySynth.

  ==============================================================================
*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace zenith {

/**
    Multimode filter with smoothed parameters
*/
class ZenithFilter {
public:
  ZenithFilter() = default;

  void setType(FilterType type) { type_ = type; }
  void setSampleRate(double sampleRate);
  void setCutoff(float cutoffHz);
  void setResonance(float resonance);
  void setDrive(float drive) { drive_ = drive; }
  void reset();
  float getResonance() const { return resonanceSmoothed_.getTargetValue(); }

  /**
   * @brief Process one sample
   * @param input Input sample
   * @return Filtered sample
   */
  float processSample(float input);

  void setModel(int model) { model_ = model; } // 0=SVF, 1=Ladder

private:
  FilterType type_ = FilterType::Lowpass;
  int model_ = 0; // 0=SVF, 1=Ladder
  double sampleRate_ = 44100.0;

  // Smoothed parameters to avoid zipper noise
  juce::SmoothedValue<float> cutoffSmoothed_;
  juce::SmoothedValue<float> resonanceSmoothed_;
  float drive_ = 1.0f;

  // SVF State
  float ic1eq_ = 0.0f, ic2eq_ = 0.0f;

  // Ladder State (Huovilainen / Stilson)
  double l_z1 = 0, l_z2 = 0, l_z3 = 0, l_z4 = 0;

  float processSVF(float input);
  float processLadder(float input);
};

} // namespace zenith
