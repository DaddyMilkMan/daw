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
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {

// Forward declarations
class WingmanSynthBridge;
class Arpeggiator;
class StepLFO;
class ZenithPolySynthParameterManager;
struct CachedSynthParameters;

//==============================================================================
class WingmanSynthBridge;

//==============================================================================
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

  // Accessor for Wingman bridge
  WingmanSynthBridge* getWingmanBridge() { return wingmanBridge_; }

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
  static const juce::String &UnisonSpread;
  static const juce::String &UnisonPanRandom;

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

  // Arpeggiator Parameters
  static const juce::String &ArpEnable;
  static const juce::String &ArpMode;
  static const juce::String &ArpRate;
  static const juce::String &ArpSync;
  static const juce::String &ArpSyncRate;
  static const juce::String &ArpGate;
  static const juce::String &ArpOctaves;
  static const juce::String &ArpSwing;
  static const juce::String &ArpHold;

  // Step LFO Parameters
  static const juce::String &StepLFO1Enable;
  static const juce::String &StepLFO1Steps;
  static const juce::String &StepLFO1Rate;
  static const juce::String &StepLFO1Sync;
  static const juce::String &StepLFO1Smoothing;
  static const juce::String &StepLFO2Enable;
  static const juce::String &StepLFO2Steps;
  static const juce::String &StepLFO2Rate;
  static const juce::String &StepLFO2Sync;
  static const juce::String &StepLFO2Smoothing;
  static const juce::String &StepLFO3Enable;
  static const juce::String &StepLFO3Steps;
  static const juce::String &StepLFO3Rate;
  static const juce::String &StepLFO3Sync;
  static const juce::String &StepLFO3Smoothing;
  static const juce::String &StepLFO4Enable;
  static const juce::String &StepLFO4Steps;
  static const juce::String &StepLFO4Rate;
  static const juce::String &StepLFO4Sync;
  static const juce::String &StepLFO4Smoothing;

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

  // Step LFO Access (for modulation matrix)
  float getStepLFOOutput(int index) const;
  void getStepLFOOutputs(float outputs[4]) const;
  Arpeggiator* getArpeggiator() { return arpeggiator_.get(); }

  // Processing helpers
  void updateArpeggiatorParameters(const CachedSynthParameters& params);
  void updateStepLFOParameters(const CachedSynthParameters& params);
  void processArpeggiator(juce::MidiBuffer& midiMessages, int numSamples);
  void processStepLFOs(int numSamples);
  void updateVoicesWithStepLFOs();

 private:
  juce::SpinLock voiceLock_;
  juce::MPESynthesiser synthesiser_;
  juce::Atomic<double> currentBpm_{120.0};
  juce::AudioProcessorValueTreeState parameters_;

  // The new parameter manager
  ZenithPolySynthParameterManager paramManager_;

  // Global Effects Chain
  ZenithEffects effects_;

  // Modulation Matrix Storage (Global for UI, applied to voices)
  juce::SpinLock modMatrixLock_;
  juce::Array<ModulationSlot> globalModMatrix_;

  // Visualizer Buffer
  juce::AbstractFifo visualizerFifo_{4096};
  juce::Array<float> visualizerBuffer_;

  // Arpeggiator and Step LFOs
  std::unique_ptr<Arpeggiator> arpeggiator_;
  std::array<std::unique_ptr<StepLFO>, 4> stepLFOs_;
  std::array<float, 4> stepLFOOutputs_{0.0f, 0.0f, 0.0f, 0.0f};
  std::array<juce::uint64, 4> stepLFOSampleCount_{0, 0, 0, 0};

  // Wingman integration
  friend class WingmanSynthBridge;
  WingmanSynthBridge* wingmanBridge_;

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
