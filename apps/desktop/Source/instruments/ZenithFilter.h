/*
  ==============================================================================

    ZenithFilter.h
    Created: 2025-12-06
    Author:  Zenith DAW

    Multimode synth filter for ZenithPolySynth with selectable analog models
    (SVF, Moog ladder, Korg MS-20, Oberheim SEM, Roland TB-303).

  ==============================================================================
*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace zenith {

/**
    Multimode filter with selectable analog character models.

    State is held per-instance (not per-call) so the filter integrators
    persist across samples — required for any of the models to actually filter.
*/
class ZenithFilter {
public:
  ZenithFilter() = default;

  void setType(FilterType type) { type_ = type; }
  void setModel(FilterModelType model) { model_ = model; }
  void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
  void setCutoff(float cutoffHz) { cutoff_ = juce::jlimit(20.0f, 20000.0f, cutoffHz); }
  void setResonance(float resonance) { resonance_ = juce::jlimit(0.0f, 1.0f, resonance); }
  void setDrive(float drive) { drive_ = juce::jlimit(0.0f, 1.0f, drive); }
  void setKeyTrack(float amount) { keyTrackAmount_ = amount; }
  void setOversampling(int factor) { oversamplingFactor_ = juce::jlimit(1, 4, factor); }

  float getResonance() const { return resonance_; }

  /** Clears all integrator state. */
  void reset();

  /**
   * @brief Process one sample.
   * @param input    Input sample
   * @param midiNote Note number driving optional key-tracking (default A4-area)
   * @return Filtered sample
   */
  float processSample(float input, float midiNote = 60.0f);

  /** Process a whole buffer in place. */
  void process(juce::AudioBuffer<float>& buffer, float midiNote = 60.0f);

private:
  float calculateCutoffWithKeyTrack(float midiNote);
  float applyDrive(float sample);
  float processSVF(float input);
  float processMoog(float input);
  float processMS20(float input);
  float processSEM(float input);
  float processTB303(float input);

  FilterType type_ = FilterType::Lowpass;
  FilterModelType model_ = FilterModelType::SVF;

  double sampleRate_ = 44100.0;
  float cutoff_ = 1000.0f;
  float resonance_ = 0.0f;
  float drive_ = 0.0f;
  float keyTrackAmount_ = 0.0f;
  int oversamplingFactor_ = 1;

  // Persistent per-model integrator state. Previously these lived in
  // function-local structs and were zeroed every sample, so the analog
  // models could not filter at all. They are now members.
  struct SVFState { double low = 0.0, high = 0.0, band = 0.0; };
  struct LadderState { double s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0; };
  struct MS20State { double hp = 0.0, lp1 = 0.0, lp2 = 0.0; };
  struct SEMState { double s1 = 0.0, s2 = 0.0; };

  SVFState svf_;
  LadderState moog_;
  LadderState tb303_;
  MS20State ms20_;
  SEMState sem_;
};

} // namespace zenith
