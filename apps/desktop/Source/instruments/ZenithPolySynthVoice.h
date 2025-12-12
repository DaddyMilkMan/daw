/*
  ==============================================================================

    ZenithPolySynthVoice.h
    Created: 2025-12-06
    Author:  Zenith DAW

    Polyphonic voice for ZenithPolySynth.

  ==============================================================================
*/

#pragma once

#include "ZenithFilter.h"
#include "ZenithOscillator.h"
#include "ZenithPolySynthDefs.h"
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

  void setLFO1(float rate, float amount, LFOTarget target,
               LFOWaveform waveform);
  void setLFO2(float rate, float amount, LFOTarget target,
               LFOWaveform waveform);

  void setGlideTime(float glideTimeSeconds) { glideTime_ = glideTimeSeconds; }
  void setMonoMode(bool mono) { monoMode_ = mono; }
  void setQualityPreset(QualityPreset quality) { qualityPreset_ = quality; }

  // New Phase 1 Fixes
  void setFilterEnvAmount(float amount) { filterEnvAmount_ = amount; }
  void setFilterKeyTrack(FilterKeyTrack mode) { filterKeyTrack_ = mode; }
  void setPitchBendRange(int semitones) {
    pitchBendRange_ = juce::jlimit(1, 24, semitones);
  }
  void setSubOscLevel(float level) {
    subOscLevel_ = juce::jlimit(0.0f, 1.0f, level);
  }
  void setSubOscOctave(int octave) {
    subOscOctave_ = juce::jlimit(-2, -1, octave);
  }
  void setNoiseLevel(float level) {
    noiseLevel_ = juce::jlimit(0.0f, 1.0f, level);
  }
  void setVelocityCurve(float curve) {
    velocityCurve_ = juce::jlimit(0.0f, 2.0f, curve);
  }
  void setMasterGain(float gain) {
    masterGain_.setTargetValue(juce::jlimit(0.0f, 2.0f, gain));
  }
  void setOsc1Mix(float mix) { osc1Mix_.setTargetValue(mix); }
  void setOsc2Mix(float mix) { osc2Mix_.setTargetValue(mix); }
  void setOsc3Mix(float mix) { osc3Mix_.setTargetValue(mix); }
  void setOsc1Detune(float d) { osc1Detune_ = d; }
  void setOsc2Detune(float d) { osc2Detune_ = d; }
  void setOsc3Detune(float d) { osc3Detune_ = d; }

  // Store previous frequency for glide
  void storePreviousFrequency() { previousFrequency_ = currentFrequency_; }

  void setSampleRate(double sampleRate);
  void setBpm(double bpm) { bpm_ = bpm; }
  void setLFO1Sync(bool sync, SyncRate rate, bool retr) {
    lfo1Sync_ = sync;
    lfo1SyncRate_ = rate;
    lfo1Retr_ = retr;
  }
  void setLFO2Sync(bool sync, SyncRate rate, bool retr) {
    lfo2Sync_ = sync;
    lfo2SyncRate_ = rate;
    lfo2Retr_ = retr;
  }
  // Flagship Setters (public for ZenithPolySynth access)
  void setOsc2Sync(bool sync) { osc2Sync_ = sync; }
  void setOsc2FM(float amount) { osc2FM_ = amount; }
  void setRingMod(float amount) { ringMod_ = amount; }
  void setFilterModel(FilterModelType model) { filterModel_ = static_cast<int>(model); }

  // Oscillator shape setters (public for ZenithPolySynth access)
  void setOsc1Shape(float shape) { osc1Shape_.setTargetValue(shape); }
  void setOsc2Shape(float shape) { osc2Shape_.setTargetValue(shape); }
  void setOsc3Shape(float shape) { osc3Shape_.setTargetValue(shape); }

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
  double bpm_ = 120.0; // BPM for Sync
  float lfo1Value_ = 0.0f;
  float lfo2Value_ = 0.0f;

  // Parameters
  juce::SmoothedValue<float> osc1Mix_;
  juce::SmoothedValue<float> osc2Mix_;
  juce::SmoothedValue<float> osc3Mix_;

  int unisonVoices_ = 1;
  float unisonDetune_ = 0.0f;

  // Per-oscillator detune in cents
  float osc1Detune_ = 0.0f;
  float osc2Detune_ = 0.0f;
  float osc3Detune_ = 0.0f;

  float filterCutoff_ = 1000.0f;
  float filter2Cutoff_ = 1000.0f;
  bool filterSerial_ = true;

  juce::ADSR::Parameters ampEnvParams_;
  juce::ADSR::Parameters modEnvParams_;

  float lfo1Rate_ = 1.0f;
  float lfo1Amount_ = 0.0f;
  LFOTarget lfo1Target_ = LFOTarget::FilterCutoff;
  LFOWaveform lfo1Waveform_ = LFOWaveform::Sine;
  bool lfo1Sync_ = false;
  SyncRate lfo1SyncRate_ = SyncRate::_1_4;
  bool lfo1Retr_ = true;
  float lfo1SHValue_ = 0.0f; // Sample & Hold cached value

  float lfo2Rate_ = 1.0f;
  float lfo2Amount_ = 0.0f;
  LFOTarget lfo2Target_ = LFOTarget::FilterCutoff;
  LFOWaveform lfo2Waveform_ = LFOWaveform::Sine;
  bool lfo2Sync_ = false;
  SyncRate lfo2SyncRate_ = SyncRate::_1_4;
  bool lfo2Retr_ = true;
  float lfo2SHValue_ = 0.0f; // Sample & Hold cached value

  float glideTime_ = 0.0f;
  bool monoMode_ = false;
  QualityPreset qualityPreset_ = QualityPreset::Medium;

  // New Phase 1 Fix Parameters
  float filterEnvAmount_ =
      0.5f; // Filter envelope depth (-1 to +1 normalized to 0-1)
  FilterKeyTrack filterKeyTrack_ = FilterKeyTrack::Off;
  int pitchBendRange_ = 2;     // Semitones (standard is ±2)
  float subOscLevel_ = 0.0f;   // Sub-oscillator level
  int subOscOctave_ = -1;      // -1 = one octave down, -2 = two octaves down
  float noiseLevel_ = 0.0f;    // White noise level
  float velocityCurve_ = 1.0f; // 0.5 = soft, 1.0 = linear, 2.0 = hard
  juce::SmoothedValue<float> masterGain_; // Master output gain
  int midiNoteNumber_ = 60;               // Current MIDI note for key tracking

  // Modulation Matrix
  std::array<ModulationSlot, 8> modulationMatrix_;
  ModulationState modulationState_;

  // Performance state
  float currentFrequency_ = 440.0f;
  float targetFrequency_ = 440.0f;
  float previousFrequency_ =
      440.0f; // For glide - preserves last note frequency
  float velocity_ = 0.0f;
  float modWheel_ = 0.0f;
  float aftertouch_ = 0.0f;
  float pitchBend_ = 0.0f;
  float currentAmplitude_ = 0.0f;

  // Per-oscillator shape/wavetable position
  juce::SmoothedValue<float> osc1Shape_;
  juce::SmoothedValue<float> osc2Shape_;
  juce::SmoothedValue<float> osc3Shape_;

  // Sub-oscillator (generates one octave below osc1)
  ZenithOscillator subOsc_;

  // Noise generator
  juce::Random noiseRandom_;

  // Flagship State
  bool osc2Sync_ = false;
  float osc2FM_ = 0.0f;
  float ringMod_ = 0.0f;
  int filterModel_ = 0; // 0=SVF

  // Previous phases for sync detection
  double prevOsc1Phase_ = 0.0;

  // Internal helpers
  void updateFrequency();
  void computeModulation();
  float getModulationSourceValue(ModulationSource source);
  float computeLFOValue(double phase, LFOWaveform waveform, float &shValue);
};

} // namespace zenith
