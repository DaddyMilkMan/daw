/*
  ==============================================================================

    ZenithPolySynthParameterManager.h
    Created: 2025-12-11
    Author:  Zenith DAW

    Dedicated parameter management for ZenithPolySynth.
    Extracted from ZenithPolySynthProcessor to improve modularity.

  ==============================================================================
*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

// Forward declarations
class ZenithPolySynthVoice;
class ZenithEffects;

//==============================================================================
/**
    Cached snapshot of all synth parameters.
    Used for real-time safe parameter reading.
*/
struct CachedSynthParameters {
  // Oscillators
  int osc1Wave = 1;
  float osc1Detune = 0.0f;
  float osc1Mix = 1.0f;
  float osc1Shape = 0.5f;

  int osc2Wave = 1;
  float osc2Detune = 0.0f;
  float osc2Mix = 0.0f;
  float osc2Shape = 0.5f;

  int osc3Wave = 0;
  float osc3Detune = 0.0f;
  float osc3Mix = 0.0f;
  float osc3Shape = 0.5f;

  // Sub & Noise
  float noiseLevel = 0.0f;
  float subOscLevel = 0.0f;
  int subOscOctave = 1;

  // Unison
  int unisonVoices = 1;
  float unisonDetune = 10.0f;

  // Flagship Features
  bool osc2Sync = false;
  float osc2FM = 0.0f;
  float ringMod = 0.0f;
  int filterModel = 0;

  // Filter
  int filterType = 0;
  float filterCutoff = 20000.0f;
  float filterResonance = 0.0f;
  float filterDrive = 1.0f;
  float filterEnvAmount = 0.5f;
  int filterKeyTrack = 0;

  // Amp Envelope
  float ampAttack = 0.01f;
  float ampDecay = 0.1f;
  float ampSustain = 1.0f;
  float ampRelease = 0.1f;

  // Mod Envelope
  float modAttack = 0.01f;
  float modDecay = 0.3f;
  float modSustain = 0.0f;
  float modRelease = 0.1f;

  // LFO 1
  float lfo1Rate = 1.0f;
  float lfo1Amount = 0.0f;
  int lfo1Target = 0;
  int lfo1Waveform = 0;
  bool lfo1Sync = false;
  int lfo1SyncIdx = 4;
  bool lfo1Retrigger = true;

  // LFO 2
  float lfo2Rate = 1.0f;
  float lfo2Amount = 0.0f;
  int lfo2Target = 0;
  int lfo2Waveform = 0;
  bool lfo2Sync = false;
  int lfo2SyncIdx = 4;
  bool lfo2Retrigger = true;

  // Performance
  float glideTime = 0.0f;
  bool monoMode = false;
  float masterGain = 0.8f;
  int pitchBendRange = 2;
  float velocityCurve = 1.0f;
  int qualitySetting = 1;

  // Delay
  float delayTime = 0.5f;
  float delayFeedback = 0.5f;
  float delayMix = 0.0f;
  bool delaySync = false;
  int delaySyncIdx = 4;

  // Effects
  float distortionAmount = 0.0f;
  float chorusAmount = 0.0f;
  float reverbAmount = 0.0f;

  // Voice count
  int maxVoices = 16;
};

//==============================================================================
/**
    Manages all parameter creation, value fetching, and voice updates
    for the ZenithPolySynth.

    Responsibilities:
    - Define and create the full parameter layout
    - Fetch all parameter values atomically into a cache
    - Apply cached parameters to synth voices
    - Apply effect parameters to the effects chain
*/
class ZenithPolySynthParameterManager {
public:
  //==========================================================================
  // Parameter ID Constants
  //==========================================================================

  // Oscillators
  static const juce::String Osc1Wave;
  static const juce::String Osc1Detune;
  static const juce::String Osc1Mix;
  static const juce::String Osc1Shape;
  static const juce::String Osc2Wave;
  static const juce::String Osc2Detune;
  static const juce::String Osc2Mix;
  static const juce::String Osc2Shape;
  static const juce::String Osc3Wave;
  static const juce::String Osc3Detune;
  static const juce::String Osc3Mix;
  static const juce::String Osc3Shape;

  // Sub & Noise
  static const juce::String NoiseLevel;
  static const juce::String SubOscLevel;
  static const juce::String SubOscOctave;

  // Unison
  static const juce::String UnisonVoices;
  static const juce::String UnisonDetune;

  // Filter
  static const juce::String FilterType;
  static const juce::String FilterCutoff;
  static const juce::String FilterResonance;
  static const juce::String FilterDrive;
  static const juce::String FilterEnvAmount;
  static const juce::String FilterKeyTrack;
  static const juce::String FilterModel;

  // Envelopes
  static const juce::String AmpAttack;
  static const juce::String AmpDecay;
  static const juce::String AmpSustain;
  static const juce::String AmpRelease;
  static const juce::String ModAttack;
  static const juce::String ModDecay;
  static const juce::String ModSustain;
  static const juce::String ModRelease;

  // LFOs
  static const juce::String LFO1Rate;
  static const juce::String LFO1Amount;
  static const juce::String LFO1Target;
  static const juce::String LFO1Waveform;
  static const juce::String LFO1Sync;
  static const juce::String LFO1SyncRate;
  static const juce::String LFO1Retr;
  static const juce::String LFO2Rate;
  static const juce::String LFO2Amount;
  static const juce::String LFO2Target;
  static const juce::String LFO2Waveform;
  static const juce::String LFO2Sync;
  static const juce::String LFO2SyncRate;
  static const juce::String LFO2Retr;

  // Performance
  static const juce::String GlideTime;
  static const juce::String MonoMode;
  static const juce::String MasterGain;
  static const juce::String PitchBendRange;
  static const juce::String VelocityCurve;

  // System
  static const juce::String MaxVoices;
  static const juce::String QualitySetting;

  // Flagship Features
  static const juce::String Osc2Sync;
  static const juce::String Osc2FM;
  static const juce::String RingMod;

  // Effects
  static const juce::String DistortionAmount;
  static const juce::String ChorusAmount;
  static const juce::String ReverbAmount;
  static const juce::String DelayTime;
  static const juce::String DelayFeedback;
  static const juce::String DelayMix;
  static const juce::String DelaySync;
  static const juce::String DelaySyncRate;

  //==========================================================================
  // Constructor / Destructor
  //==========================================================================

  explicit ZenithPolySynthParameterManager(
      juce::AudioProcessorValueTreeState &apvts);
  ~ZenithPolySynthParameterManager() = default;

  //==========================================================================
  // Parameter Layout Creation
  //==========================================================================

  /**
      Creates the full parameter layout for the synth.
      Called once during processor construction.
  */
  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

  //==========================================================================
  // Parameter Fetching
  //==========================================================================

  /**
      Fetches all parameter values atomically into the cached struct.
      Call this once per audio block before applying to voices.
  */
  void fetchAllParameters();

  /**
      Returns a const reference to the cached parameters.
      Valid after calling fetchAllParameters().
  */
  const CachedSynthParameters &getCachedParameters() const {
    return cachedParams_;
  }

  //==========================================================================
  // Voice Parameter Application
  //==========================================================================

  /**
      Applies all cached parameters to a single voice.
      @param voice The voice to update
      @param bpm Current tempo for sync features
  */
  void applyToVoice(ZenithPolySynthVoice &voice, double bpm) const;

  /**
      Applies effect parameters to the effects chain.
      @param effects The effects chain to update
      @param bpm Current tempo for sync features
  */
  void applyToEffects(ZenithEffects &effects, double bpm) const;

  //==========================================================================
  // Utility
  //==========================================================================

  /**
      Returns the target voice count from parameters.
  */
  int getTargetVoiceCount() const { return cachedParams_.maxVoices; }

private:
  juce::AudioProcessorValueTreeState &parameters_;
  CachedSynthParameters cachedParams_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthParameterManager)
};

} // namespace zenith
