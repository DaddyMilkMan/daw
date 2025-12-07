/*
  ==============================================================================

    ZenithPolySynthVoice.h
    Created: 2025-12-06
    Author:  Zenith DAW

    Polyphonic voice for ZenithPolySynth.

  ==============================================================================
*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include "ZenithOscillator.h"
#include "ZenithFilter.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

// Forward declaration of Sound
class ZenithPolySynthSound : public juce::SynthesiserSound {
public:
  ZenithPolySynthSound() {}
  bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
  bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

/**
    Voice for ZenithPolySynth - RT-safe polyphonic voice
*/
class ZenithPolySynthVoice : public juce::SynthesiserVoice {
public:
  ZenithPolySynthVoice();
  ~ZenithPolySynthVoice() override = default;

  bool canPlaySound(juce::SynthesiserSound *sound) override;
  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;
  void stopNote(float velocity, bool allowTailOff) override;
  void pitchWheelMoved(int newPitchWheelValue) override;
  void controllerMoved(int controllerNumber, int newControllerValue) override;
  void channelPressureChanged(int newChannelPressureValue) override;
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

  //==========================================================================
  // Parameter setters (called from message thread or via atomic parameters)
  //==========================================================================
  void setOsc1Waveform(OscillatorWaveform waveform) {
    osc1_.setWaveform(waveform);
  }
  void setOsc2Waveform(OscillatorWaveform waveform) {
    osc2_.setWaveform(waveform);
  }
  void setOsc3Waveform(OscillatorWaveform waveform) {
    osc3_.setWaveform(waveform);
  }

  void setOsc1Detune(float cents) { osc1_.setDetune(cents); }
  void setOsc2Detune(float cents) { osc2_.setDetune(cents); }
  void setOsc3Detune(float cents) { osc3_.setDetune(cents); }

  void setOsc1Mix(float mix) { osc1Mix_ = mix; }
  void setOsc2Mix(float mix) { osc2Mix_ = mix; }
  void setOsc3Mix(float mix) { osc3Mix_ = mix; }

  void setUnisonVoices(int voices) {
    unisonVoices_ = juce::jlimit(1, 7, voices);
  }
  void setUnisonDetune(float cents) { unisonDetune_ = cents; }

  void setFilterType(FilterType type) { filter1_.setType(type); }
  void setFilterCutoff(float cutoff) { filterCutoff_ = cutoff; }
  void setFilterResonance(float resonance) { filter1_.setResonance(resonance); }
  void setFilterDrive(float drive) { filter1_.setDrive(drive); }

  void setFilter2Type(FilterType type) { filter2_.setType(type); }
  void setFilter2Cutoff(float cutoff) { filter2Cutoff_ = cutoff; }
  void setFilter2Resonance(float resonance) {
    filter2_.setResonance(resonance);
  }
  void setFilterRouting(bool serial) { filterSerial_ = serial; }

  void setAmpEnvelope(float attack, float decay, float sustain, float release);
  void setModEnvelope(float attack, float decay, float sustain, float release);

  void setLFO1(float rate, float amount, LFOTarget target);
  void setLFO2(float rate, float amount, LFOTarget target);

  void setGlideTime(float glideTimeSeconds) { glideTime_ = glideTimeSeconds; }
  void setMonoMode(bool mono) { monoMode_ = mono; }
  void setQualityPreset(QualityPreset quality) { qualityPreset_ = quality; }

  void setSampleRate(double sampleRate);

  //==========================================================================
  // Modulation Matrix Control
  //==========================================================================

  void setModulationSlot(int slotIndex, ModulationSource source,
                         ModulationDestination destination, float amount);

  void setModWheel(float value) { modWheel_ = juce::jlimit(0.0f, 1.0f, value); }
  void setAftertouch(float value) {
    aftertouch_ = juce::jlimit(0.0f, 1.0f, value);
  }

  float getCurrentAmplitude() const { return currentAmplitude_; }

private:
  // Oscillators
  ZenithOscillator osc1_, osc2_, osc3_;
  std::array<ZenithOscillator, 7> unisonOscillators_;

  // Filters
  ZenithFilter filter1_, filter2_;

  // Envelopes
  juce::ADSR ampEnvelope_;
  juce::ADSR modEnvelope_;

  // LFOs
  double lfo1Phase_ = 0.0;
  double lfo2Phase_ = 0.0;
  float lfo1Value_ = 0.0f;
  float lfo2Value_ = 0.0f;

  // Parameters
  float osc1Mix_ = 1.0f;
  float osc2Mix_ = 0.0f;
  float osc3Mix_ = 0.0f;

  int unisonVoices_ = 1;
  float unisonDetune_ = 0.0f;

  float filterCutoff_ = 1000.0f;
  float filter2Cutoff_ = 1000.0f;
  bool filterSerial_ = true;

  juce::ADSR::Parameters ampEnvParams_;
  juce::ADSR::Parameters modEnvParams_;

  float lfo1Rate_ = 1.0f;
  float lfo1Amount_ = 0.0f;
  LFOTarget lfo1Target_ = LFOTarget::FilterCutoff;

  float lfo2Rate_ = 1.0f;
  float lfo2Amount_ = 0.0f;
  LFOTarget lfo2Target_ = LFOTarget::FilterCutoff;

  float glideTime_ = 0.0f;
  bool monoMode_ = false;
  QualityPreset qualityPreset_ = QualityPreset::Medium;

  // Modulation Matrix
  std::array<ModulationSlot, 8> modulationMatrix_;
  ModulationState modulationState_;

  // Performance state
  float currentFrequency_ = 440.0f;
  float targetFrequency_ = 440.0f;
  float velocity_ = 0.0f;
  float modWheel_ = 0.0f;
  float aftertouch_ = 0.0f;
  float pitchBend_ = 0.0f;
  float currentAmplitude_ = 0.0f;
  float oscShape_ = 0.5f; // Global shape parameter for square waves

  // Internal helpers
  void updateFrequency();
  void computeModulation();
  float getModulationSourceValue(ModulationSource source);
};

} // namespace zenith
