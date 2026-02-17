/*
  ==============================================================================

    ZenithPolySynth.h
    Created: 2025-11-18
    Refactored: 2025-12-11
    Author:  Zenith DAW

    Header for ZenithPolySynth - multi-oscillator subtractive synthesizer.
    REFACTORED: Uses ZenithPolySynthParameterManager.

  ==============================================================================
*/

#pragma once

#include "Instrument.h"
#include "ZenithEffects.h"
#include "ZenithPolySynthDefs.h"
#include "ZenithPolySynthParameterManager.h"
#include "MacroController.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
/**
    Main Processor Class
*/
class ZenithPolySynthProcessor : public juce::AudioProcessor {
public:
  ZenithPolySynthProcessor();
  ~ZenithPolySynthProcessor() override;

  juce::AbstractFifo &getVisualizerFifo() { return visualizerFifo_; }

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

  // Accessor for the manager
  ZenithPolySynthParameterManager &getParameterManager() {
    return paramManager_;
  }

  // Parameter ID Aliases (for backward compatibility)
  static const juce::String &Osc1Wave;
  static const juce::String &Osc1Detune;
  static const juce::String &Osc1Mix;
  static const juce::String &Osc1Shape;
  static const juce::String &Osc2Wave;
  static const juce::String &Osc2Detune;
  static const juce::String &Osc2Mix;
  static const juce::String &Osc2Shape;
  static const juce::String &Osc3Wave;
  static const juce::String &Osc3Detune;
  static const juce::String &Osc3Mix;
  static const juce::String &Osc3Shape;

  static const juce::String &NoiseLevel;
  static const juce::String &SubOscLevel;
  static const juce::String &SubOscOctave;
  static const juce::String &FilterEnvAmount;

  static const juce::String &UnisonVoices;
  static const juce::String &UnisonDetune;

  static const juce::String
      &FilterType; // Renamed from FilterTypeParam to match manager
  static const juce::String &FilterCutoff;
  static const juce::String &FilterResonance;
  static const juce::String &FilterDrive;
  static const juce::String &FilterKeyTrack; // Renamed from FilterKeyTrackParam
  static const juce::String &FilterModel;

  static const juce::String &AmpAttack;
  static const juce::String &AmpDecay;
  static const juce::String &AmpSustain;
  static const juce::String &AmpRelease;

  static const juce::String &ModAttack;
  static const juce::String &ModDecay;
  static const juce::String &ModSustain;
  static const juce::String &ModRelease;

  static const juce::String &LFO1Rate;
  static const juce::String &LFO1Amount;
  static const juce::String &LFO1Target;
  static const juce::String &LFO1Waveform;
  static const juce::String &LFO1Sync;
  static const juce::String &LFO1SyncRate;
  static const juce::String &LFO1Retr;

  static const juce::String &LFO2Rate;
  static const juce::String &LFO2Amount;
  static const juce::String &LFO2Target;
  static const juce::String &LFO2Waveform;
  static const juce::String &LFO2Sync;
  static const juce::String &LFO2SyncRate;
  static const juce::String &LFO2Retr;

  // LFO 3 Parameters
  static const juce::String &LFO3Rate;
  static const juce::String &LFO3Amount;
  static const juce::String &LFO3Target;
  static const juce::String &LFO3Waveform;
  static const juce::String &LFO3Sync;
  static const juce::String &LFO3SyncRate;
  static const juce::String &LFO3Retr;

  // Envelope 3 Parameters
  static const juce::String &Env3Attack;
  static const juce::String &Env3Decay;
  static const juce::String &Env3Sustain;
  static const juce::String &Env3Release;

  // Macro Control Parameters
  static const juce::String &Macro1;
  static const juce::String &Macro2;
  static const juce::String &Macro3;
  static const juce::String &Macro4;

  static const juce::String &GlideTime;
  static const juce::String &MonoMode;
  static const juce::String &MasterGain;
  static const juce::String &PitchBendRange;
  static const juce::String &VelocityCurve;

  static const juce::String &MaxVoices;
  static const juce::String &QualitySetting;

  // Flagship Features
  static const juce::String &Osc2Sync;
  static const juce::String &Osc2FM;
  static const juce::String &RingMod;

  // Effects Parameters
  static const juce::String &DistortionAmount;
  static const juce::String &ChorusAmount;
  static const juce::String &ReverbAmount;
  static const juce::String &DelayTime;
  static const juce::String &DelayFeedback;
  static const juce::String &DelayMix;
  static const juce::String &DelaySync;
  static const juce::String &DelaySyncRate;

  // Modulation Matrix Access
  float getModulationMatrix(ModulationSource src,
                            ModulationDestination dst) const;
  void setModulationMatrix(ModulationSource src, ModulationDestination dst,
                           float amount);

  // Visualizer Access
  int readFromVisualizer(float *buffer, int numSamples);
  void pushToVisualizer(const float *buffer, int numSamples);

  // Global Effects Access
  void setDistortion(float amount) { effects_.setDistortion(amount); }
  void setChorus(float amount) { effects_.setChorus(amount); }
  void setReverb(float amount) { effects_.setReverb(amount); }

  // Macro Control Access
  MacroController& getMacroController() { return macroController_; }
  void processMacros() { macroController_.processSmoothing(256); }
  void applyMacrosToParameters() { macroController_.applyToParameters(parameters_); }

private:
  juce::SpinLock voiceLock_;
  mutable juce::SpinLock modMatrixLock_;
  juce::MPESynthesiser synthesiser_;
  std::atomic<double> currentBpm_{120.0};
  juce::AudioProcessorValueTreeState parameters_;

  // The new parameter manager
  ZenithPolySynthParameterManager paramManager_;

  // Global Effects Chain
  ZenithEffects effects_;

  // Macro Controller
  MacroController macroController_;

  // Modulation Matrix Storage (Global for UI, applied to voices)
  // Modulation Matrix Storage (Global for UI, applied to voices)
  std::array<ModulationSlot, 64> globalModMatrix_;

  // Visualizer Buffer
  juce::AbstractFifo visualizerFifo_{4096};
  std::vector<float> visualizerBuffer_{4096};

  // Internal state
  static constexpr int DEFAULT_VOICE_COUNT = 16;
  int currentMaxVoices_ = DEFAULT_VOICE_COUNT;
  int currentBlockSize_ = 512;  // Roast Fix #5: Track buffer size for dynamic changes
  int maxActiveVoices_ = 0;
  double maxBlockProcessingTime_ = 0.0;

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
