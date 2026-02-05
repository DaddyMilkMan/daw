/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ZenithPolySynthParameterManager.cpp
    Created: 2025-12-11
    Author:  Zenith DAW

    Implementation of parameter management for ZenithPolySynth.

  ==============================================================================

*/

#include "ZenithPolySynthParameterManager.h"
#include "ZenithEffects.h"
#include "ZenithPolySynthVoice.h"
// Forced compilation check

namespace zenith {

//==============================================================================
// Parameter ID Definitions
//==============================================================================

// Oscillators
const juce::String ZenithPolySynthParameterManager::Osc1Wave = "osc1_wave";
const juce::String ZenithPolySynthParameterManager::Osc1Detune = "osc1_detune";
const juce::String ZenithPolySynthParameterManager::Osc1Mix = "osc1_mix";
const juce::String ZenithPolySynthParameterManager::Osc1Shape = "osc1_shape";
const juce::String ZenithPolySynthParameterManager::Osc2Wave = "osc2_wave";
const juce::String ZenithPolySynthParameterManager::Osc2Detune = "osc2_detune";
const juce::String ZenithPolySynthParameterManager::Osc2Mix = "osc2_mix";
const juce::String ZenithPolySynthParameterManager::Osc2Shape = "osc2_shape";
const juce::String ZenithPolySynthParameterManager::Osc3Wave = "osc3_wave";
const juce::String ZenithPolySynthParameterManager::Osc3Detune = "osc3_detune";
const juce::String ZenithPolySynthParameterManager::Osc3Mix = "osc3_mix";
const juce::String ZenithPolySynthParameterManager::Osc3Shape = "osc3_shape";

// Sub & Noise
const juce::String ZenithPolySynthParameterManager::NoiseLevel = "noise_level";
const juce::String ZenithPolySynthParameterManager::SubOscLevel = "sub_level";
const juce::String ZenithPolySynthParameterManager::SubOscOctave =
    "sub_osc_octave";

// Unison
const juce::String ZenithPolySynthParameterManager::UnisonVoices =
    "unison_voices";
const juce::String ZenithPolySynthParameterManager::UnisonDetune =
    "unison_detune";
const juce::String ZenithPolySynthParameterManager::UnisonSpread =
    "unison_spread";
const juce::String ZenithPolySynthParameterManager::UnisonPanRandom =
    "unison_pan_random";

// Filter
const juce::String ZenithPolySynthParameterManager::FilterType = "filter_type";
const juce::String ZenithPolySynthParameterManager::FilterCutoff =
    "filter_cutoff";
const juce::String ZenithPolySynthParameterManager::FilterResonance =
    "filter_res";
const juce::String ZenithPolySynthParameterManager::FilterDrive =
    "filter_drive";
const juce::String ZenithPolySynthParameterManager::FilterEnvAmount =
    "filter_env_amt";
const juce::String ZenithPolySynthParameterManager::FilterKeyTrack =
    "filter_keytrack";
const juce::String ZenithPolySynthParameterManager::FilterModel =
    "filter_model";

// Envelopes
const juce::String ZenithPolySynthParameterManager::AmpAttack = "amp_attack";
const juce::String ZenithPolySynthParameterManager::AmpDecay = "amp_decay";
const juce::String ZenithPolySynthParameterManager::AmpSustain = "amp_sustain";
const juce::String ZenithPolySynthParameterManager::AmpRelease = "amp_release";
const juce::String ZenithPolySynthParameterManager::ModAttack = "mod_attack";
const juce::String ZenithPolySynthParameterManager::ModDecay = "mod_decay";
const juce::String ZenithPolySynthParameterManager::ModSustain = "mod_sustain";
const juce::String ZenithPolySynthParameterManager::ModRelease = "mod_release";

// LFOs
const juce::String ZenithPolySynthParameterManager::LFO1Rate = "lfo1_rate";
const juce::String ZenithPolySynthParameterManager::LFO1Amount = "lfo1_amount";
const juce::String ZenithPolySynthParameterManager::LFO1Target = "lfo1_target";
const juce::String ZenithPolySynthParameterManager::LFO1Waveform =
    "lfo1_waveform";
const juce::String ZenithPolySynthParameterManager::LFO1Sync = "lfo1_sync";
const juce::String ZenithPolySynthParameterManager::LFO1SyncRate =
    "lfo1_sync_rate";
const juce::String ZenithPolySynthParameterManager::LFO1Retr = "lfo1_retr";
const juce::String ZenithPolySynthParameterManager::LFO2Rate = "lfo2_rate";
const juce::String ZenithPolySynthParameterManager::LFO2Amount = "lfo2_amount";
const juce::String ZenithPolySynthParameterManager::LFO2Target = "lfo2_target";
const juce::String ZenithPolySynthParameterManager::LFO2Waveform =
    "lfo2_waveform";
const juce::String ZenithPolySynthParameterManager::LFO2Sync = "lfo2_sync";
const juce::String ZenithPolySynthParameterManager::LFO2SyncRate =
    "lfo2_sync_rate";
const juce::String ZenithPolySynthParameterManager::LFO2Retr = "lfo2_retr";

// Performance
const juce::String ZenithPolySynthParameterManager::GlideTime = "glide_time";
const juce::String ZenithPolySynthParameterManager::MonoMode = "mono_mode";
const juce::String ZenithPolySynthParameterManager::MasterGain = "master_gain";
const juce::String ZenithPolySynthParameterManager::PitchBendRange =
    "pitch_bend_range";
const juce::String ZenithPolySynthParameterManager::VelocityCurve =
    "velocity_curve";

// System
const juce::String ZenithPolySynthParameterManager::MaxVoices = "max_voices";
const juce::String ZenithPolySynthParameterManager::QualitySetting = "quality";

// Flagship Features
const juce::String ZenithPolySynthParameterManager::Osc2Sync = "osc2_sync";
const juce::String ZenithPolySynthParameterManager::Osc2FM = "osc2_fm";
const juce::String ZenithPolySynthParameterManager::RingMod = "ring_mod";

// Effects
const juce::String ZenithPolySynthParameterManager::DistortionAmount =
    "dist_amount";
const juce::String ZenithPolySynthParameterManager::ChorusAmount =
    "chorus_amount";
const juce::String ZenithPolySynthParameterManager::ReverbAmount =
    "reverb_amount";
const juce::String ZenithPolySynthParameterManager::DelayTime = "delay_time";
const juce::String ZenithPolySynthParameterManager::DelayFeedback =
    "delay_feedback";
const juce::String ZenithPolySynthParameterManager::DelayMix = "delay_mix";
const juce::String ZenithPolySynthParameterManager::DelaySync = "delay_sync";
const juce::String ZenithPolySynthParameterManager::DelaySyncRate =
    "delay_sync_rate";

// Arpeggiator
const juce::String ZenithPolySynthParameterManager::ArpEnable = "arp_enable";
const juce::String ZenithPolySynthParameterManager::ArpMode = "arp_mode";
const juce::String ZenithPolySynthParameterManager::ArpRate = "arp_rate";
const juce::String ZenithPolySynthParameterManager::ArpSync = "arp_sync";
const juce::String ZenithPolySynthParameterManager::ArpSyncRate = "arp_sync_rate";
const juce::String ZenithPolySynthParameterManager::ArpGate = "arp_gate";
const juce::String ZenithPolySynthParameterManager::ArpOctaves = "arp_octaves";
const juce::String ZenithPolySynthParameterManager::ArpSwing = "arp_swing";
const juce::String ZenithPolySynthParameterManager::ArpHold = "arp_hold";

// Step LFOs
const juce::String ZenithPolySynthParameterManager::StepLFO1Enable = "steplfo1_enable";
const juce::String ZenithPolySynthParameterManager::StepLFO1Steps = "steplfo1_steps";
const juce::String ZenithPolySynthParameterManager::StepLFO1Rate = "steplfo1_rate";
const juce::String ZenithPolySynthParameterManager::StepLFO1Sync = "steplfo1_sync";
const juce::String ZenithPolySynthParameterManager::StepLFO1Smoothing = "steplfo1_smoothing";
const juce::String ZenithPolySynthParameterManager::StepLFO2Enable = "steplfo2_enable";
const juce::String ZenithPolySynthParameterManager::StepLFO2Steps = "steplfo2_steps";
const juce::String ZenithPolySynthParameterManager::StepLFO2Rate = "steplfo2_rate";
const juce::String ZenithPolySynthParameterManager::StepLFO2Sync = "steplfo2_sync";
const juce::String ZenithPolySynthParameterManager::StepLFO2Smoothing = "steplfo2_smoothing";
const juce::String ZenithPolySynthParameterManager::StepLFO3Enable = "steplfo3_enable";
const juce::String ZenithPolySynthParameterManager::StepLFO3Steps = "steplfo3_steps";
const juce::String ZenithPolySynthParameterManager::StepLFO3Rate = "steplfo3_rate";
const juce::String ZenithPolySynthParameterManager::StepLFO3Sync = "steplfo3_sync";
const juce::String ZenithPolySynthParameterManager::StepLFO3Smoothing = "steplfo3_smoothing";
const juce::String ZenithPolySynthParameterManager::StepLFO4Enable = "steplfo4_enable";
const juce::String ZenithPolySynthParameterManager::StepLFO4Steps = "steplfo4_steps";
const juce::String ZenithPolySynthParameterManager::StepLFO4Rate = "steplfo4_rate";
const juce::String ZenithPolySynthParameterManager::StepLFO4Sync = "steplfo4_sync";
const juce::String ZenithPolySynthParameterManager::StepLFO4Smoothing = "steplfo4_smoothing";

//==============================================================================
// Constructor
//==============================================================================

ZenithPolySynthParameterManager::ZenithPolySynthParameterManager(
    juce::AudioProcessorValueTreeState &apvts)
    : parameters_(apvts) {}

//==============================================================================
// Parameter Layout Creation
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithPolySynthParameterManager::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  juce::StringArray waves = {"Sine",  "Saw",      "Square",   "Triangle",
                             "Noise", "Supersaw", "Wavetable"};

  // =========================================================================
  // OSCILLATORS
  // =========================================================================
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      Osc1Wave, "Osc 1 Wave", waves, 1));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc1Detune, "Osc 1 Detune", -100.0f, 100.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc1Mix, "Osc 1 Mix", 0.0f, 1.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc1Shape, "Osc 1 Shape", 0.0f, 1.0f, 0.5f));

  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      Osc2Wave, "Osc 2 Wave", waves, 1));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc2Detune, "Osc 2 Detune", -100.0f, 100.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc2Mix, "Osc 2 Mix", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc2Shape, "Osc 2 Shape", 0.0f, 1.0f, 0.5f));

  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      Osc3Wave, "Osc 3 Wave", waves, 0));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc3Detune, "Osc 3 Detune", -100.0f, 100.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc3Mix, "Osc 3 Mix", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc3Shape, "Osc 3 Shape", 0.0f, 1.0f, 0.5f));

  // Sub-oscillator & Noise
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      SubOscLevel, "Sub Osc Level", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      SubOscOctave, "Sub Osc Octave", juce::StringArray{"-2 Oct", "-1 Oct"},
      1));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      NoiseLevel, "Noise Level", 0.0f, 1.0f, 0.0f));

  // Unison
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      UnisonVoices, "Unison Voices", 1, 7, 1));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      UnisonDetune, "Unison Detune", 0.0f, 100.0f, 10.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      UnisonSpread, "Unison Spread", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      UnisonPanRandom, "Unison Pan Random", false));

  // Flagship Features
  params.push_back(
      std::make_unique<juce::AudioParameterBool>(Osc2Sync, "Sync 2->1", false));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      Osc2FM, "FM 1->2", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      RingMod, "Ring Mod", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      FilterModel, "Filter Model",
      juce::StringArray{"State Variable", "Ladder"}, 0));

  // =========================================================================
  // FILTER
  // =========================================================================
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      FilterType, "Filter Type",
      juce::StringArray{"LowPass", "BandPass", "HighPass"}, 0));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      FilterCutoff, "Filter Cutoff",
      juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      FilterResonance, "Filter Resonance", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      FilterDrive, "Filter Drive", 1.0f, 10.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      FilterEnvAmount, "Filter Env Amount", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      FilterKeyTrack, "Filter Key Track",
      juce::StringArray{"Off", "50%", "100%"}, 0));

  // =========================================================================
  // ENVELOPES
  // =========================================================================
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      AmpAttack, "Amp Attack", 0.001f, 10.0f, 0.01f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      AmpDecay, "Amp Decay", 0.001f, 10.0f, 0.1f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      AmpSustain, "Amp Sustain", 0.0f, 1.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      AmpRelease, "Amp Release", 0.001f, 10.0f, 0.1f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ModAttack, "Mod Attack", 0.001f, 10.0f, 0.01f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ModDecay, "Mod Decay", 0.001f, 10.0f, 0.3f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ModSustain, "Mod Sustain", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ModRelease, "Mod Release", 0.001f, 10.0f, 0.1f));

  // =========================================================================
  // LFOs
  // =========================================================================
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      LFO1Rate, "LFO 1 Rate", 0.01f, 50.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      LFO1Amount, "LFO 1 Amount", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      LFO1Target, "LFO 1 Target",
      juce::StringArray{"Cutoff", "Osc1 Pitch", "Osc2 Pitch", "Osc1 Mix",
                        "Osc2 Mix", "Amp", "Osc1 Shape"},
      0));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      LFO1Waveform, "LFO 1 Wave",
      juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      LFO1Sync, "LFO 1 Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      LFO1SyncRate, "LFO 1 Sync Rate",
      juce::StringArray{"1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1",
                        "2/1", "4/1"},
      4));
  params.push_back(
      std::make_unique<juce::AudioParameterBool>(LFO1Retr, "LFO 1 Retr", true));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      LFO2Rate, "LFO 2 Rate", 0.01f, 50.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      LFO2Amount, "LFO 2 Amount", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      LFO2Target, "LFO 2 Target",
      juce::StringArray{"Cutoff", "Osc1 Pitch", "Osc2 Pitch", "Osc1 Mix",
                        "Osc2 Mix", "Amp", "Osc1 Shape"},
      0));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      LFO2Waveform, "LFO 2 Wave",
      juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      LFO2Sync, "LFO 2 Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      LFO2SyncRate, "LFO 2 Sync Rate",
      juce::StringArray{"1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1",
                        "2/1", "4/1"},
      4));
  params.push_back(
      std::make_unique<juce::AudioParameterBool>(LFO2Retr, "LFO 2 Retr", true));

  // =========================================================================
  // PERFORMANCE
  // =========================================================================
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      GlideTime, "Glide Time", 0.0f, 2.0f, 0.0f));
  params.push_back(
      std::make_unique<juce::AudioParameterBool>(MonoMode, "Mono Mode", false));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      MasterGain, "Master Gain", 0.0f, 2.0f, 0.8f));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      PitchBendRange, "Pitch Bend Range", 1, 24, 2));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      VelocityCurve, "Velocity Curve", 0.25f, 4.0f, 1.0f));

  // =========================================================================
  // SYSTEM
  // =========================================================================
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      MaxVoices, "Max Voices", 1, 32, 16));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      QualitySetting, "Quality", juce::StringArray{"Low", "Medium", "High"},
      1));

  // =========================================================================
  // EFFECTS
  // =========================================================================
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      DistortionAmount, "Distortion", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ChorusAmount, "Chorus", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ReverbAmount, "Reverb", 0.0f, 1.0f, 0.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      DelayTime, "Delay Time", 0.01f, 2.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      DelayFeedback, "Delay Feedback", 0.0f, 0.95f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      DelayMix, "Delay Mix", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      DelaySync, "Delay Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      DelaySyncRate, "Delay Rate",
      juce::StringArray{"1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1",
                        "2/1", "4/1"},
      4));

  // =========================================================================
  // ARPEGGIATOR
  // =========================================================================
  juce::StringArray arpModes = {"Up", "Down", "Up/Down", "Random", "Chord", "Order", "As Played"};
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      ArpEnable, "Arp Enable", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      ArpMode, "Arp Mode", arpModes, 0));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ArpRate, "Arp Rate", 0.1f, 50.0f, 4.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      ArpSync, "Arp Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      ArpSyncRate, "Arp Sync Rate",
      juce::StringArray{"1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1",
                        "2/1", "4/1"},
      4));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ArpGate, "Arp Gate", 0.01f, 1.0f, 0.8f));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      ArpOctaves, "Arp Octaves", 1, 4, 1));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      ArpSwing, "Arp Swing", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      ArpHold, "Arp Hold", false));

  // =========================================================================
  // STEP LFOS
  // =========================================================================
  juce::StringArray smoothModes = {"Step", "Linear", "Cubic"};

  // Step LFO 1
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      StepLFO1Enable, "Step LFO 1 Enable", false));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      StepLFO1Steps, "Step LFO 1 Steps", 1, 64, 16));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      StepLFO1Rate, "Step LFO 1 Rate", 0.01f, 50.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      StepLFO1Sync, "Step LFO 1 Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      StepLFO1Smoothing, "Step LFO 1 Smoothing", smoothModes, 0));

  // Step LFO 2
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      StepLFO2Enable, "Step LFO 2 Enable", false));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      StepLFO2Steps, "Step LFO 2 Steps", 1, 64, 16));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      StepLFO2Rate, "Step LFO 2 Rate", 0.01f, 50.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      StepLFO2Sync, "Step LFO 2 Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      StepLFO2Smoothing, "Step LFO 2 Smoothing", smoothModes, 0));

  // Step LFO 3
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      StepLFO3Enable, "Step LFO 3 Enable", false));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      StepLFO3Steps, "Step LFO 3 Steps", 1, 64, 16));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      StepLFO3Rate, "Step LFO 3 Rate", 0.01f, 50.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      StepLFO3Sync, "Step LFO 3 Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      StepLFO3Smoothing, "Step LFO 3 Smoothing", smoothModes, 0));

  // Step LFO 4
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      StepLFO4Enable, "Step LFO 4 Enable", false));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      StepLFO4Steps, "Step LFO 4 Steps", 1, 64, 16));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      StepLFO4Rate, "Step LFO 4 Rate", 0.01f, 50.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterBool>(
      StepLFO4Sync, "Step LFO 4 Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      StepLFO4Smoothing, "Step LFO 4 Smoothing", smoothModes, 0));

  return {params.begin(), params.end()};
}

//==============================================================================
// Parameter Fetching
//==============================================================================

void ZenithPolySynthParameterManager::fetchAllParameters() {
  // Oscillators
  cachedParams_.osc1Wave =
      static_cast<int>(parameters_.getRawParameterValue(Osc1Wave)->load());
  cachedParams_.osc1Detune =
      parameters_.getRawParameterValue(Osc1Detune)->load();
  cachedParams_.osc1Mix = parameters_.getRawParameterValue(Osc1Mix)->load();
  cachedParams_.osc1Shape = parameters_.getRawParameterValue(Osc1Shape)->load();

  cachedParams_.osc2Wave =
      static_cast<int>(parameters_.getRawParameterValue(Osc2Wave)->load());
  cachedParams_.osc2Detune =
      parameters_.getRawParameterValue(Osc2Detune)->load();
  cachedParams_.osc2Mix = parameters_.getRawParameterValue(Osc2Mix)->load();
  cachedParams_.osc2Shape = parameters_.getRawParameterValue(Osc2Shape)->load();

  cachedParams_.osc3Wave =
      static_cast<int>(parameters_.getRawParameterValue(Osc3Wave)->load());
  cachedParams_.osc3Detune =
      parameters_.getRawParameterValue(Osc3Detune)->load();
  cachedParams_.osc3Mix = parameters_.getRawParameterValue(Osc3Mix)->load();
  cachedParams_.osc3Shape = parameters_.getRawParameterValue(Osc3Shape)->load();

  // Sub & Noise
  cachedParams_.noiseLevel =
      parameters_.getRawParameterValue(NoiseLevel)->load();
  cachedParams_.subOscLevel =
      parameters_.getRawParameterValue(SubOscLevel)->load();
  cachedParams_.subOscOctave =
      static_cast<int>(parameters_.getRawParameterValue(SubOscOctave)->load());

  // Unison
  cachedParams_.unisonVoices =
      static_cast<int>(parameters_.getRawParameterValue(UnisonVoices)->load());
  cachedParams_.unisonDetune =
      parameters_.getRawParameterValue(UnisonDetune)->load();
  cachedParams_.unisonSpread =
      parameters_.getRawParameterValue(UnisonSpread)->load();
  cachedParams_.unisonPanRandom =
      parameters_.getRawParameterValue(UnisonPanRandom)->load() != 0.0f;

  // Flagship Features
  cachedParams_.osc2Sync =
      parameters_.getRawParameterValue(Osc2Sync)->load() > 0.5f;
  cachedParams_.osc2FM = parameters_.getRawParameterValue(Osc2FM)->load();
  cachedParams_.ringMod = parameters_.getRawParameterValue(RingMod)->load();
  cachedParams_.filterModel =
      static_cast<int>(parameters_.getRawParameterValue(FilterModel)->load());

  // Filter
  cachedParams_.filterType =
      static_cast<int>(parameters_.getRawParameterValue(FilterType)->load());
  cachedParams_.filterCutoff =
      parameters_.getRawParameterValue(FilterCutoff)->load();
  cachedParams_.filterResonance =
      parameters_.getRawParameterValue(FilterResonance)->load();
  cachedParams_.filterDrive =
      parameters_.getRawParameterValue(FilterDrive)->load();
  cachedParams_.filterEnvAmount =
      parameters_.getRawParameterValue(FilterEnvAmount)->load();
  cachedParams_.filterKeyTrack = static_cast<int>(
      parameters_.getRawParameterValue(FilterKeyTrack)->load());

  // Amp Envelope
  cachedParams_.ampAttack = parameters_.getRawParameterValue(AmpAttack)->load();
  cachedParams_.ampDecay = parameters_.getRawParameterValue(AmpDecay)->load();
  cachedParams_.ampSustain =
      parameters_.getRawParameterValue(AmpSustain)->load();
  cachedParams_.ampRelease =
      parameters_.getRawParameterValue(AmpRelease)->load();

  // Mod Envelope
  cachedParams_.modAttack = parameters_.getRawParameterValue(ModAttack)->load();
  cachedParams_.modDecay = parameters_.getRawParameterValue(ModDecay)->load();
  cachedParams_.modSustain =
      parameters_.getRawParameterValue(ModSustain)->load();
  cachedParams_.modRelease =
      parameters_.getRawParameterValue(ModRelease)->load();

  // LFO 1
  cachedParams_.lfo1Rate = parameters_.getRawParameterValue(LFO1Rate)->load();
  cachedParams_.lfo1Amount =
      parameters_.getRawParameterValue(LFO1Amount)->load();
  cachedParams_.lfo1Target =
      static_cast<int>(parameters_.getRawParameterValue(LFO1Target)->load());
  cachedParams_.lfo1Waveform =
      static_cast<int>(parameters_.getRawParameterValue(LFO1Waveform)->load());
  cachedParams_.lfo1Sync =
      parameters_.getRawParameterValue(LFO1Sync)->load() > 0.5f;
  cachedParams_.lfo1SyncIdx =
      static_cast<int>(parameters_.getRawParameterValue(LFO1SyncRate)->load());
  cachedParams_.lfo1Retrigger =
      parameters_.getRawParameterValue(LFO1Retr)->load() > 0.5f;

  // LFO 2
  cachedParams_.lfo2Rate = parameters_.getRawParameterValue(LFO2Rate)->load();
  cachedParams_.lfo2Amount =
      parameters_.getRawParameterValue(LFO2Amount)->load();
  cachedParams_.lfo2Target =
      static_cast<int>(parameters_.getRawParameterValue(LFO2Target)->load());
  cachedParams_.lfo2Waveform =
      static_cast<int>(parameters_.getRawParameterValue(LFO2Waveform)->load());
  cachedParams_.lfo2Sync =
      parameters_.getRawParameterValue(LFO2Sync)->load() > 0.5f;
  cachedParams_.lfo2SyncIdx =
      static_cast<int>(parameters_.getRawParameterValue(LFO2SyncRate)->load());
  cachedParams_.lfo2Retrigger =
      parameters_.getRawParameterValue(LFO2Retr)->load() > 0.5f;

  // Performance
  cachedParams_.glideTime = parameters_.getRawParameterValue(GlideTime)->load();
  cachedParams_.monoMode =
      parameters_.getRawParameterValue(MonoMode)->load() > 0.5f;
  cachedParams_.masterGain =
      parameters_.getRawParameterValue(MasterGain)->load();
  cachedParams_.pitchBendRange = static_cast<int>(
      parameters_.getRawParameterValue(PitchBendRange)->load());
  cachedParams_.velocityCurve =
      parameters_.getRawParameterValue(VelocityCurve)->load();
  cachedParams_.qualitySetting = static_cast<int>(
      parameters_.getRawParameterValue(QualitySetting)->load());

  // Delay
  cachedParams_.delayTime = parameters_.getRawParameterValue(DelayTime)->load();
  cachedParams_.delayFeedback =
      parameters_.getRawParameterValue(DelayFeedback)->load();
  cachedParams_.delayMix = parameters_.getRawParameterValue(DelayMix)->load();
  cachedParams_.delaySync =
      parameters_.getRawParameterValue(DelaySync)->load() > 0.5f;
  cachedParams_.delaySyncIdx =
      static_cast<int>(parameters_.getRawParameterValue(DelaySyncRate)->load());

  // Effects
  cachedParams_.distortionAmount =
      parameters_.getRawParameterValue(DistortionAmount)->load();
  cachedParams_.chorusAmount =
      parameters_.getRawParameterValue(ChorusAmount)->load();
  cachedParams_.reverbAmount =
      parameters_.getRawParameterValue(ReverbAmount)->load();

  // System
  cachedParams_.maxVoices =
      static_cast<int>(parameters_.getRawParameterValue(MaxVoices)->load());

  // Arpeggiator
  cachedParams_.arpEnable =
      parameters_.getRawParameterValue(ArpEnable)->load() > 0.5f;
  cachedParams_.arpMode =
      static_cast<int>(parameters_.getRawParameterValue(ArpMode)->load());
  cachedParams_.arpRate = parameters_.getRawParameterValue(ArpRate)->load();
  cachedParams_.arpSync =
      parameters_.getRawParameterValue(ArpSync)->load() > 0.5f;
  cachedParams_.arpSyncRate =
      static_cast<int>(parameters_.getRawParameterValue(ArpSyncRate)->load());
  cachedParams_.arpGate = parameters_.getRawParameterValue(ArpGate)->load();
  cachedParams_.arpOctaves =
      static_cast<int>(parameters_.getRawParameterValue(ArpOctaves)->load());
  cachedParams_.arpSwing = parameters_.getRawParameterValue(ArpSwing)->load();
  cachedParams_.arpHold =
      parameters_.getRawParameterValue(ArpHold)->load() > 0.5f;

  // Step LFO 1
  cachedParams_.stepLFO1Enable =
      parameters_.getRawParameterValue(StepLFO1Enable)->load() > 0.5f;
  cachedParams_.stepLFO1Steps =
      static_cast<int>(parameters_.getRawParameterValue(StepLFO1Steps)->load());
  cachedParams_.stepLFO1Rate = parameters_.getRawParameterValue(StepLFO1Rate)->load();
  cachedParams_.stepLFO1Sync =
      parameters_.getRawParameterValue(StepLFO1Sync)->load() > 0.5f;
  cachedParams_.stepLFO1Smoothing = parameters_.getRawParameterValue(StepLFO1Smoothing)->load();

  // Step LFO 2
  cachedParams_.stepLFO2Enable =
      parameters_.getRawParameterValue(StepLFO2Enable)->load() > 0.5f;
  cachedParams_.stepLFO2Steps =
      static_cast<int>(parameters_.getRawParameterValue(StepLFO2Steps)->load());
  cachedParams_.stepLFO2Rate = parameters_.getRawParameterValue(StepLFO2Rate)->load();
  cachedParams_.stepLFO2Sync =
      parameters_.getRawParameterValue(StepLFO2Sync)->load() > 0.5f;
  cachedParams_.stepLFO2Smoothing = parameters_.getRawParameterValue(StepLFO2Smoothing)->load();

  // Step LFO 3
  cachedParams_.stepLFO3Enable =
      parameters_.getRawParameterValue(StepLFO3Enable)->load() > 0.5f;
  cachedParams_.stepLFO3Steps =
      static_cast<int>(parameters_.getRawParameterValue(StepLFO3Steps)->load());
  cachedParams_.stepLFO3Rate = parameters_.getRawParameterValue(StepLFO3Rate)->load();
  cachedParams_.stepLFO3Sync =
      parameters_.getRawParameterValue(StepLFO3Sync)->load() > 0.5f;
  cachedParams_.stepLFO3Smoothing = parameters_.getRawParameterValue(StepLFO3Smoothing)->load();

  // Step LFO 4
  cachedParams_.stepLFO4Enable =
      parameters_.getRawParameterValue(StepLFO4Enable)->load() > 0.5f;
  cachedParams_.stepLFO4Steps =
      static_cast<int>(parameters_.getRawParameterValue(StepLFO4Steps)->load());
  cachedParams_.stepLFO4Rate = parameters_.getRawParameterValue(StepLFO4Rate)->load();
  cachedParams_.stepLFO4Sync =
      parameters_.getRawParameterValue(StepLFO4Sync)->load() > 0.5f;
  cachedParams_.stepLFO4Smoothing = parameters_.getRawParameterValue(StepLFO4Smoothing)->load();
}

//==============================================================================
// Voice Parameter Application
//==============================================================================

void ZenithPolySynthParameterManager::applyToVoice(ZenithPolySynthVoice &voice,
                                                   double bpm) const {
  const auto &p = cachedParams_;

  // Oscillators
  voice.setOsc1Waveform(static_cast<OscillatorWaveform>(p.osc1Wave));
  voice.setOsc2Waveform(static_cast<OscillatorWaveform>(p.osc2Wave));
  voice.setOsc3Waveform(static_cast<OscillatorWaveform>(p.osc3Wave));

  voice.setOsc1Detune(p.osc1Detune);
  voice.setOsc2Detune(p.osc2Detune);
  voice.setOsc3Detune(p.osc3Detune);

  voice.setOsc1Mix(p.osc1Mix);
  voice.setOsc2Mix(p.osc2Mix);
  voice.setOsc3Mix(p.osc3Mix);

  voice.setOsc1Shape(p.osc1Shape);
  voice.setOsc2Shape(p.osc2Shape);
  voice.setOsc3Shape(p.osc3Shape);

  // Sub & Noise
  voice.setSubOscLevel(p.subOscLevel);
  voice.setSubOscOctave(p.subOscOctave == 0 ? -2 : -1);
  voice.setNoiseLevel(p.noiseLevel);

  // Unison
  voice.setUnisonVoices(p.unisonVoices);
  voice.setUnisonDetune(p.unisonDetune);
  voice.setUnisonSpread(p.unisonSpread);
  voice.setUnisonPanRandom(p.unisonPanRandom);

  // Flagship Features
  voice.setOsc2Sync(p.osc2Sync);
  voice.setOsc2FM(p.osc2FM);
  voice.setRingMod(p.ringMod);
  voice.setFilterModel(static_cast<FilterModelType>(p.filterModel));

  // Filter
  voice.setFilterType(static_cast<zenith::FilterType>(p.filterType));
  voice.setFilterCutoff(p.filterCutoff);
  voice.setFilterResonance(p.filterResonance);
  voice.setFilterDrive(p.filterDrive);
  voice.setFilterEnvAmount(p.filterEnvAmount);
  voice.setFilterKeyTrack(
      static_cast<zenith::FilterKeyTrack>(p.filterKeyTrack));

  // Envelopes
  voice.setAmpEnvelope(p.ampAttack, p.ampDecay, p.ampSustain, p.ampRelease);
  voice.setModEnvelope(p.modAttack, p.modDecay, p.modSustain, p.modRelease);

  // LFOs
  voice.setLFO1(p.lfo1Rate, p.lfo1Amount, static_cast<LFOTarget>(p.lfo1Target),
                static_cast<LFOWaveform>(p.lfo1Waveform));
  voice.setLFO1Sync(p.lfo1Sync, static_cast<SyncRate>(p.lfo1SyncIdx + 1),
                    p.lfo1Retrigger);
  voice.setLFO2(p.lfo2Rate, p.lfo2Amount, static_cast<LFOTarget>(p.lfo2Target),
                static_cast<LFOWaveform>(p.lfo2Waveform));
  voice.setLFO2Sync(p.lfo2Sync, static_cast<SyncRate>(p.lfo2SyncIdx + 1),
                    p.lfo2Retrigger);
  voice.setBpm(bpm);

  // Performance
  voice.setGlideTime(p.glideTime);
  voice.setMonoMode(p.monoMode);
  voice.setMasterGain(p.masterGain);
  voice.setPitchBendRange(p.pitchBendRange);
  voice.setVelocityCurve(p.velocityCurve);
  voice.setQualityPreset(static_cast<QualityPreset>(p.qualitySetting));
}

//==============================================================================
// Effects Parameter Application
//==============================================================================

void ZenithPolySynthParameterManager::applyToEffects(ZenithEffects &effects,
                                                     double bpm) const {
  const auto &p = cachedParams_;

  effects.setDistortion(p.distortionAmount);
  effects.setChorus(p.chorusAmount);
  effects.setReverb(p.reverbAmount);
  effects.setDelay(p.delayTime, p.delayFeedback, p.delayMix);
  effects.setBpm(bpm);
  effects.setDelaySync(p.delaySync, static_cast<SyncRate>(p.delaySyncIdx + 1));
}

} // namespace zenith
