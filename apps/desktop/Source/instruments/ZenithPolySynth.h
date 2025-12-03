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
    LFO target parameters (legacy - now part of modulation matrix)
*/
enum class LFOTarget {
  FilterCutoff = 0,
  Osc1Pitch,
  Osc2Pitch,
  Osc1Mix,
  Osc2Mix,
  NumTargets
};

//==============================================================================
/**
    Modulation Matrix System
*/

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
  None = 0,
  Osc1Pitch,
  Osc2Pitch,
  Osc3Pitch,
  Osc1Mix,
  Osc2Mix,
  Osc3Mix,
  FilterCutoff,
  FilterResonance,
  AmpGain,
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
  float processSupersaw(float frequency);

  // Supersaw state
  std::array<double, 7> supersawPhases_ = {0.0};
  std::array<float, 7> supersawDetunes_ = {0.0f};
  std::array<float, 7> supersawRatios_ = {1.0f}; // Precalculated frequency multipliers
  bool supersawInit_ = false;

  void updateSupersawRatios();
  // PolyBLEP anti-aliasing helper
  // t: current phase (0..1)
  // dt: phase increment per sample
  inline float poly_blep(float t, float dt) {
      if (t < dt) {
          t /= dt;
          return t + t - t * t - 1.0f;
      } else if (t > 1.0f - dt) {
          t = (t - 1.0f) / dt;
          return t * t + t + t + 1.0f;
      }
      return 0.0f;
  }
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
    chorusPhase_ = 0.0f;
  }

  void setDistortion(float amount) { distortionAmount_ = amount; }
  void setChorus(float amount) { chorusAmount_ = amount; }
  void setReverb(float amount) { reverbAmount_ = amount; }

  void process(float &left, float &right);

private:
  double sampleRate_ = 44100.0;
  float distortionAmount_ = 0.0f;
  float chorusAmount_ = 0.0f;
  float reverbAmount_ = 0.0f;

  // Chorus LFO
  float chorusPhase_ = 0.0f;

  // Real Chorus (Delay Line)
  std::array<float, 2048> delayBufferL_ = {0.0f};
  std::array<float, 2048> delayBufferR_ = {0.0f};
  int delayPos_ = 0;
  
  // Reverb (Comb filters + Allpass)
  // Simple implementation: 4 combs, 2 allpass
  struct Comb {
      std::vector<float> buffer;
      int pos = 0;
      float feedback = 0.84f;
      float damp = 0.2f;
      float val = 0.0f;
      
      void resize(int size) { buffer.resize(size, 0.0f); }
      float process(float input) {
          if (buffer.empty()) return input;
          float output = buffer[pos];
          val = output * (1.0f - damp) + val * damp;
          buffer[pos] = input + val * feedback;
          pos = (pos + 1) % buffer.size();
          return output;
      }
  };
  
  struct Allpass {
      std::vector<float> buffer;
      int pos = 0;
      float feedback = 0.5f;
      
      void resize(int size) { buffer.resize(size, 0.0f); }
      float process(float input) {
          if (buffer.empty()) return input;
          float bufOut = buffer[pos];
          float output = -input + bufOut;
          buffer[pos] = input + (bufOut * feedback);
          pos = (pos + 1) % buffer.size();
          return output;
      }
  };
  
  std::array<Comb, 4> combs_;
  std::array<Allpass, 2> allpasses_;
  bool reverbInit_ = false;
  
  void initReverb() {
      if (reverbInit_) return;
      // Tunings for 44.1kHz (scaled by SR in setSampleRate)
      // Standard Schroeder/Moorer values
      int tunings[] = { 1116, 1188, 1277, 1356 }; 
      for (int i=0; i<4; ++i) combs_[i].resize(static_cast<int>(tunings[i] * (sampleRate_ / 44100.0)));
      
      int allpassTunings[] = { 225, 556 };
      for (int i=0; i<2; ++i) allpasses_[i].resize(static_cast<int>(allpassTunings[i] * (sampleRate_ / 44100.0)));
      
      reverbInit_ = true;
  }
  
public:
    bool hasTail() const {
        // Simple check: if effects are enabled, assume tail is active for a while
        // Ideally we would check signal levels, but for now we prevent early cutoff
        return (reverbAmount_ > 0.0f || chorusAmount_ > 0.0f);
    }
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
  
  // Fast Tanh Approximation (Padé)
  inline float fast_tanh(float x) {
      if (x < -3.0f) return -1.0f;
      if (x > 3.0f) return 1.0f;
      float x2 = x * x;
      return x * (27.0f + x2) / (27.0f + 9.0f * x2);
  }
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
class ZenithPolySynthProcessor : public juce::AudioProcessor {
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
  bool hasEditor() const override { return true; }

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
  static const juce::String NoiseLevel;
  static const juce::String SubOscLevel;
  static const juce::String FilterEnvAmount;

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
  static const juce::String LFO1Target;

  static const juce::String LFO2Rate;
  static const juce::String LFO2Amount;
  static const juce::String LFO2Target;

  static const juce::String GlideTime;
  static const juce::String MonoMode;
  static const juce::String MasterGain;

  static const juce::String MaxVoices;
  static const juce::String QualitySetting;

  // Modulation Matrix Access
  float getModulationMatrix(ModulationSource src, ModulationDestination dst) const;
  void setModulationMatrix(ModulationSource src, ModulationDestination dst, float amount);

  // Visualizer Access
  int readFromVisualizer(float* buffer, int numSamples);
  void pushToVisualizer(const float* buffer, int numSamples);
  
  // Global Effects Access
  void setDistortion(float amount) { effects_.setDistortion(amount); }
  void setChorus(float amount) { effects_.setChorus(amount); }
  void setReverb(float amount) { effects_.setReverb(amount); }

private:
  juce::Synthesiser synthesiser_;
  juce::AudioProcessorValueTreeState parameters_;
  
  // Global Effects Chain (Moved from Voice to Processor)
  ZenithEffects effects_;

  // Modulation Matrix Storage (Global for UI, applied to voices)
  std::array<ModulationSlot, 64> globalModMatrix_; // Simplified storage

  // Visualizer Buffer
  juce::AbstractFifo visualizerFifo_{ 4096 };
  std::vector<float> visualizerBuffer_{ 4096 };

  // Internal state
  int currentMaxVoices_ = 16;
  int maxActiveVoices_ = 0;
  double maxBlockProcessingTime_ = 0.0;

  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  void updateVoiceParameters();
  void updateVoiceCount();
};

class ZenithPolySynth : public InstrumentBase {
public:
  ZenithPolySynth();
  ~ZenithPolySynth() override = default;

  juce::AudioProcessorValueTreeState *getParameterState() {
    if (auto *proc =
            dynamic_cast<ZenithPolySynthProcessor *>(getAudioProcessor())) {
      return &proc->getParameters();
    }
    return nullptr;
  }

  static InstrumentMetadata createMetadata();

private:
  void registerPresets();
};

} // namespace zenith
