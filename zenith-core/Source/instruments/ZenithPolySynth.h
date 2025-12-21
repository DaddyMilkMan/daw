/*
  ==============================================================================

    ZenithPolySynth.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Header for ZenithPolySynth - multi-oscillator subtractive synthesizer.

  ==============================================================================
*/

#pragma once

#include "Instrument.h"
#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

//==============================================================================
/**
    Waveform types for oscillators
*/
enum class OscillatorWaveform {
  Sine = 0,
  Saw,
  Square,
  Triangle,
  Noise,
  Supersaw,
  NumWaveforms
};

/**
    Filter types
*/
enum class FilterType { Lowpass = 0, Bandpass, Highpass, NumTypes };

/**
    Quality preset for CPU optimization
*/
enum class QualityPreset {
  Low = 0, // Max 3 unison voices, optimized for CPU
  Medium,  // Max 5 unison voices, balanced
  High,    // Max 7 unison voices, full quality
  NumPresets
};

/**
    LFO Waveform Types
*/
enum class LFOWaveform {
  Sine = 0,
  Triangle,
  Square,
  SawUp,
  SawDown,
  SampleHold,
  NumWaveforms
};

/**
    Modulation sources available in the matrix
*/
enum class ModulationSource {
  None = 0,   // No modulation
  LFO1,       // Low-frequency oscillator 1 (sine wave, -1 to +1)
  LFO2,       // Low-frequency oscillator 2 (sine wave, -1 to +1)
  Env1,       // Amplitude envelope (0 to 1, ADSR)
  Env2,       // Modulation envelope (0 to 1, ADSR)
  Velocity,   // Note-on velocity (0 to 1)
  ModWheel,   // MIDI mod wheel CC#1 (0 to 1)
  Aftertouch, // MIDI channel pressure (0 to 1)
  NumSources
};

/**
    Modulation destinations available in the matrix
*/
enum class ModulationDestination {
  None = 0,        // No destination
  FilterCutoff,    // Filter cutoff frequency
  FilterResonance, // Filter resonance/Q
  Osc1Pitch,       // Oscillator 1 pitch (semitones)
  Osc2Pitch,       // Oscillator 2 pitch (semitones)
  Osc3Pitch,       // Oscillator 3 pitch (semitones)
  WavetablePos,    // Wavetable/phase position (0 to 1)
  Pan,             // Stereo panning (-1 left to +1 right)
  Volume,          // Output volume/gain
  Osc1Mix,         // Oscillator 1 mix level
  Osc2Mix,         // Oscillator 2 mix level
  Osc3Mix,         // Oscillator 3 mix level
  OscShape,        // Oscillator Shape/PulseWidth (0 to 1)
  NumDestinations
};

/**
    Single modulation routing slot
*/
struct ModulationSlot {
  ModulationSource source = ModulationSource::None;
  ModulationDestination destination = ModulationDestination::None;
  float amount = 0.0f; // Modulation depth/amount (-1 to +1)

  bool isActive() const {
    return source != ModulationSource::None &&
           destination != ModulationDestination::None;
  }
};

/**
    RT-safe modulation state per voice
*/
struct ModulationState {
  // Pre-computed modulation amounts for each destination
  std::array<float, static_cast<size_t>(ModulationDestination::NumDestinations)>
      values;

  ModulationState() { reset(); }

  void reset() { values.fill(0.0f); }

  float get(ModulationDestination dest) const {
    return values[static_cast<size_t>(dest)];
  }

  void set(ModulationDestination dest, float value) {
    values[static_cast<size_t>(dest)] = value;
  }

  void add(ModulationDestination dest, float value) {
    values[static_cast<size_t>(dest)] += value;
  }
};

//==============================================================================
/**
    Single oscillator with multiple waveforms and detune
*/
class ZenithOscillator {
public:
  ZenithOscillator() = default;

  void setWaveform(OscillatorWaveform waveform) { waveform_ = waveform; }
  OscillatorWaveform getWaveform() const { return waveform_; }
  void setDetune(float detuneCents) { detuneCents_ = detuneCents; }
  void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
  void reset() { phase_ = 0.0; }
  void randomizePhase() { phase_ = random_.nextFloat(); }

  /**
   * @brief Generate next sample
   * @param frequency Base frequency in Hz
   * @param shape Shape parameter (Pulse Width for Square, etc.)
   * @return Sample value in range [-1, 1]
   */
  float getNextSample(float frequency, float shape = 0.5f);

private:
  OscillatorWaveform waveform_ = OscillatorWaveform::Saw;
  double phase_ = 0.0;
  double sampleRate_ = 44100.0;
  float detuneCents_ = 0.0f;
  juce::Random random_;

  float processSine(float frequency);
  float processSaw(float frequency);
  float processSquare(float frequency, float pulseWidth);
  float processTriangle(float frequency);
  float processNoise();
};

//==============================================================================
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

private:
  FilterType type_ = FilterType::Lowpass;
  double sampleRate_ = 44100.0;

  // Smoothed parameters to avoid zipper noise
  juce::SmoothedValue<float> cutoffSmoothed_;
  juce::SmoothedValue<float> resonanceSmoothed_;

  float drive_ = 1.0f;

  // State variables filter implementation
  float v0_ = 0.0f, v1_ = 0.0f, v2_ = 0.0f;
  float ic1eq_ = 0.0f, ic2eq_ = 0.0f;

  // Optimization cache
  float lastCutoff_ = -1.0f;
  float lastG_ = 0.0f;
};

//==============================================================================
/**
    Per-Voice Effects Chain
*/
class ZenithEffects {
public:
  ZenithEffects() = default;

  void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
  void reset() {
    // Clear delay buffers
    delayBufferL_.fill(0.0f);
    delayBufferR_.fill(0.0f);
    delayPos_ = 0;
    chorusPhase_ = 0.0f; // Reset chorus LFO phase for consistency
  }

  void setDistortion(float amount) { distortionAmount_ = amount; }
  void setChorus(float amount) { chorusAmount_ = amount; }

  void process(float &left, float &right);

private:
  double sampleRate_ = 44100.0;
  float distortionAmount_ = 0.0f;
  float chorusAmount_ = 0.0f;

  // Chorus LFO
  float chorusPhase_ = 0.0f;

  // Real Chorus (Delay Line)
  std::array<float, 2048> delayBufferL_ = {0.0f};
  std::array<float, 2048> delayBufferR_ = {0.0f};
  int delayPos_ = 0;
};

//==============================================================================
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

  void setSubOscLevel(float level) { subOscLevel_ = level; }
  void setNoiseLevel(float level) { noiseLevel_ = level; }

  void setFilterType(FilterType type) { filter1_.setType(type); }
  void setFilterCutoff(float cutoff) { filterCutoff_ = cutoff; }
  void setFilterResonance(float resonance) { 
    filterResonance_ = resonance;
    filter1_.setResonance(resonance); 
  }
  void setFilterDrive(float drive) { filter1_.setDrive(drive); }
  void setFilterEnvAmount(float amount) { filterEnvAmount_ = amount; }

  void setFilter2Type(FilterType type) { filter2_.setType(type); }
  void setFilter2Cutoff(float cutoff) { filter2Cutoff_ = cutoff; }
  void setFilter2Resonance(float resonance) {
    filter2_.setResonance(resonance);
  }
  void setFilter2Drive(float drive) { filter2_.setDrive(drive); }
  void setFilterRouting(bool serial) { filterSerial_ = serial; }

  void setDistortion(float amount) { effects_.setDistortion(amount); }
  void setChorus(float amount) { effects_.setChorus(amount); }

  void setAmpEnvelope(float attack, float decay, float sustain, float release);
  void setModEnvelope(float attack, float decay, float sustain, float release);

  void setLFO1(float rate, float amount) {
    lfo1Rate_ = rate;
    lfo1Amount_ = amount;
  }
  void setLFO2(float rate, float amount) {
    lfo2Rate_ = rate;
    lfo2Amount_ = amount;
  }

  void setLFO1Waveform(LFOWaveform waveform) { lfo1Waveform_ = waveform; }
  void setLFO2Waveform(LFOWaveform waveform) { lfo2Waveform_ = waveform; }

  void setGlideTime(float glideTimeSeconds) { glideTime_ = glideTimeSeconds; }
  void setMonoMode(bool mono) { monoMode_ = mono; }
  void setQualityPreset(QualityPreset quality) { qualityPreset_ = quality; }

  void setSampleRate(double sampleRate);

  //==========================================================================
  // Modulation Matrix Control
  //==========================================================================

  void setModulationAmount(ModulationSource source, ModulationDestination destination, float amount);
  float getModulationAmount(ModulationSource source, ModulationDestination destination) const;

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

  // Effects
  ZenithEffects effects_;

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

  float subOscLevel_ = 0.0f; // Sub oscillator level (0 to 1)
  double subOscPhase_ = 0.0; // Sub oscillator phase

  float noiseLevel_ = 0.0f; // Noise level (0 to 1)
  juce::Random noiseRandom_; // Random generator for noise

  float filterCutoff_ = 1000.0f;
  float filterResonance_ = 0.5f; // Base resonance value
  float filterEnvAmount_ = 0.0f; // Filter envelope amount (0 to 1)
  float filter2Cutoff_ = 1000.0f;
  bool filterSerial_ = true;

  juce::ADSR::Parameters ampEnvParams_;
  juce::ADSR::Parameters modEnvParams_;

  float lfo1Rate_ = 1.0f;
  float lfo1Amount_ = 0.0f;
  LFOWaveform lfo1Waveform_ = LFOWaveform::Sine;
  float lfo1SampleHold_ = 0.0f;
  double lastLfo1Phase_ = 0.0;

  float lfo2Rate_ = 1.0f;
  float lfo2Amount_ = 0.0f;
  LFOWaveform lfo2Waveform_ = LFOWaveform::Sine;
  float lfo2SampleHold_ = 0.0f;
  double lastLfo2Phase_ = 0.0;

  float glideTime_ = 0.0f;
  bool monoMode_ = false;
  QualityPreset qualityPreset_ = QualityPreset::Medium;

  // Modulation Matrix (Dense)
  // [Source][Destination] -> Amount (-1.0 to 1.0)
  float modulationMatrix_[static_cast<int>(ModulationSource::NumSources)][static_cast<int>(ModulationDestination::NumDestinations)];
  ModulationState modulationState_;

  // Performance state
  float currentFrequency_ = 440.0f;
  float targetFrequency_ = 440.0f;
  int currentMidiNote_ = -1; // Store MIDI note for pitch wheel
  float pitchBendRange_ = 2.0f; // Pitch bend range in semitones
  float velocity_ = 0.0f;
  float modWheel_ = 0.0f;
  float aftertouch_ = 0.0f;
  float currentAmplitude_ = 0.0f;
  float oscShape_ = 0.5f; // Global shape parameter for square waves
  
  // Cached envelope values for modulation (to avoid calling getNextSample twice)
  float currentAmpEnv_ = 0.0f;
  float currentModEnv_ = 0.0f;

  // Internal helpers
  void updateFrequency();
  void computeModulation();
  float getModulationSourceValue(ModulationSource source);
};

//==============================================================================
/**
    Sound for ZenithPolySynth
*/
class ZenithPolySynthSound : public juce::SynthesiserSound {
public:
  ZenithPolySynthSound() {}

  bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
  bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

//==============================================================================
/**
    Main Processor Class
*/
class ZenithPolySynthProcessor : public juce::Synthesiser,
                                 public juce::AudioProcessor {
public:
  ZenithPolySynthProcessor();
  ~ZenithPolySynthProcessor() override;

  // AudioProcessor overrides
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  // Editor
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  // Metadata
  const juce::String getName() const override { return "Zenith Poly Synth"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  // Program handling
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int /*index*/) override {}
  const juce::String getProgramName(int /*index*/) override {
    return "Default";
  }
  void changeProgramName(int /*index*/,
                         const juce::String & /*newName*/) override {}

  // State save/load
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  // Modulation Matrix Access
  void setModulationMatrix(ModulationSource src, ModulationDestination dst, float amount);
  float getModulationMatrix(ModulationSource src, ModulationDestination dst) const;

  // Visualizer Data
  void pushToVisualizer(const juce::AudioBuffer<float>& buffer);
  int readFromVisualizer(float* dest, int numSamples);

  // Parameter layout
  juce::AudioProcessorValueTreeState &getParameters() { return parameters_; }

  // Parameter IDs
  static const juce::String Osc1Wave;
  static const juce::String Osc1Detune;
  static const juce::String Osc1Mix;
  static const juce::String Osc2Wave;
  static const juce::String Osc2Detune;
  static const juce::String Osc2Mix;
  static const juce::String Osc3Wave;
  static const juce::String Osc3Detune;
  static const juce::String Osc3Mix;

  static const juce::String UnisonVoices;
  static const juce::String UnisonDetune;

  static const juce::String FilterType;
  static const juce::String FilterCutoff;
  static const juce::String FilterResonance;
  static const juce::String FilterDrive;

  static const juce::String AmpAttack;
  static const juce::String AmpDecay;
  static const juce::String AmpSustain;
  static const juce::String AmpRelease;

  static const juce::String ModAttack;
  static const juce::String ModDecay;
  static const juce::String ModSustain;
  static const juce::String ModRelease;

  static const juce::String LFO1Rate;
  static const juce::String LFO1Amount;

  static const juce::String LFO2Rate;
  static const juce::String LFO2Amount;

  static const juce::String GlideTime;
  static const juce::String MonoMode;
  static const juce::String MasterGain;

  static const juce::String MaxVoices;
  static const juce::String QualitySetting;

private:
  juce::AudioProcessorValueTreeState parameters_;

  // Internal state
  int currentMaxVoices_ = 16;
  int maxActiveVoices_ = 0;
  double maxBlockProcessingTime_ = 0.0;

  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  void updateVoiceParameters();
  juce::SynthesiserVoice *
  findFreeVoice(juce::SynthesiserSound *soundToPlay, int midiChannel,
                int midiNoteNumber, bool stealIfNoneAvailable) const override;

#if JUCE_DEBUG
    return nullptr;
  }

  static InstrumentMetadata createMetadata();

private:
  void registerPresets();
};

} // namespace zenith
