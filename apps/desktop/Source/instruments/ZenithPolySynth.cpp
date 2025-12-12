/*
  ==============================================================================

    ZenithPolySynth.cpp
    Refactored: 2025-12-09
    Author:  Zenith DAW

    PHASE 3 PRO UPGRADE: Added Wavetables, Oscillator Shape, and Ladder Filter.

  ==============================================================================
*/

#include "ZenithPolySynth.h"
#include "../ui/skia/ZenithPolySynthUI.h"
#include "ZenithPolySynthVoice.h"


namespace zenith {

//==============================================================================
// Parameter IDs
//==============================================================================

const juce::String ZenithPolySynthProcessor::Osc1Wave = "osc1_wave";
const juce::String ZenithPolySynthProcessor::Osc1Detune = "osc1_detune";
const juce::String ZenithPolySynthProcessor::Osc1Mix = "osc1_mix";
const juce::String ZenithPolySynthProcessor::Osc2Wave = "osc2_wave";
const juce::String ZenithPolySynthProcessor::Osc2Detune = "osc2_detune";
const juce::String ZenithPolySynthProcessor::Osc2Mix = "osc2_mix";
const juce::String ZenithPolySynthProcessor::Osc3Wave = "osc3_wave";
const juce::String ZenithPolySynthProcessor::Osc3Detune = "osc3_detune";
const juce::String ZenithPolySynthProcessor::Osc3Mix = "osc3_mix";

// New Shape Parameters
const juce::String ZenithPolySynthProcessor::Osc1Shape = "osc1_shape";
const juce::String ZenithPolySynthProcessor::Osc2Shape = "osc2_shape";
const juce::String ZenithPolySynthProcessor::Osc3Shape = "osc3_shape";

const juce::String ZenithPolySynthProcessor::NoiseLevel = "noise_level";
const juce::String ZenithPolySynthProcessor::SubOscLevel = "sub_level";
const juce::String ZenithPolySynthProcessor::FilterEnvAmount = "filter_env_amt";

const juce::String ZenithPolySynthProcessor::UnisonVoices = "unison_voices";
const juce::String ZenithPolySynthProcessor::UnisonDetune = "unison_detune";

const juce::String ZenithPolySynthProcessor::FilterTypeParam = "filter_type";
const juce::String ZenithPolySynthProcessor::FilterCutoff = "filter_cutoff";
const juce::String ZenithPolySynthProcessor::FilterResonance = "filter_res";
const juce::String ZenithPolySynthProcessor::FilterDrive = "filter_drive";

const juce::String ZenithPolySynthProcessor::AmpAttack = "amp_attack";
const juce::String ZenithPolySynthProcessor::AmpDecay = "amp_decay";
const juce::String ZenithPolySynthProcessor::AmpSustain = "amp_sustain";
const juce::String ZenithPolySynthProcessor::AmpRelease = "amp_release";

const juce::String ZenithPolySynthProcessor::ModAttack = "mod_attack";
const juce::String ZenithPolySynthProcessor::ModDecay = "mod_decay";
const juce::String ZenithPolySynthProcessor::ModSustain = "mod_sustain";
const juce::String ZenithPolySynthProcessor::ModRelease = "mod_release";

const juce::String ZenithPolySynthProcessor::LFO1Rate = "lfo1_rate";
const juce::String ZenithPolySynthProcessor::LFO1Amount = "lfo1_amount";
const juce::String ZenithPolySynthProcessor::LFO1Target = "lfo1_target";
const juce::String ZenithPolySynthProcessor::LFO1Waveform = "lfo1_waveform";

const juce::String ZenithPolySynthProcessor::LFO2Rate = "lfo2_rate";
const juce::String ZenithPolySynthProcessor::LFO2Amount = "lfo2_amount";
const juce::String ZenithPolySynthProcessor::LFO2Target = "lfo2_target";
const juce::String ZenithPolySynthProcessor::LFO2Waveform = "lfo2_waveform";

const juce::String ZenithPolySynthProcessor::GlideTime = "glide_time";
const juce::String ZenithPolySynthProcessor::MonoMode = "mono_mode";
const juce::String ZenithPolySynthProcessor::MasterGain = "master_gain";

const juce::String ZenithPolySynthProcessor::MaxVoices = "max_voices";
const juce::String ZenithPolySynthProcessor::QualitySetting = "quality";

const juce::String ZenithPolySynthProcessor::FilterKeyTrackParam =
    "filter_keytrack";
const juce::String ZenithPolySynthProcessor::PitchBendRange =
    "pitch_bend_range";
const juce::String ZenithPolySynthProcessor::SubOscOctave = "sub_osc_octave";
const juce::String ZenithPolySynthProcessor::VelocityCurve = "velocity_curve";

const juce::String ZenithPolySynthProcessor::Osc2Sync = "osc2_sync";
const juce::String ZenithPolySynthProcessor::Osc2FM = "osc2_fm";
const juce::String ZenithPolySynthProcessor::RingMod = "ring_mod";
const juce::String ZenithPolySynthProcessor::FilterModel = "filter_model";

const juce::String ZenithPolySynthProcessor::DistortionAmount = "dist_amount";
const juce::String ZenithPolySynthProcessor::ChorusAmount = "chorus_amount";
const juce::String ZenithPolySynthProcessor::ReverbAmount = "reverb_amount";

const juce::String ZenithPolySynthProcessor::DelayTime = "delay_time";
const juce::String ZenithPolySynthProcessor::DelayFeedback = "delay_feedback";
const juce::String ZenithPolySynthProcessor::DelayMix = "delay_mix";
const juce::String ZenithPolySynthProcessor::DelaySync = "delay_sync";
const juce::String ZenithPolySynthProcessor::DelaySyncRate = "delay_sync_rate";

const juce::String ZenithPolySynthProcessor::LFO1Sync = "lfo1_sync";
const juce::String ZenithPolySynthProcessor::LFO1SyncRate = "lfo1_sync_rate";
const juce::String ZenithPolySynthProcessor::LFO1Retr = "lfo1_retr";
const juce::String ZenithPolySynthProcessor::LFO2Sync = "lfo2_sync";
const juce::String ZenithPolySynthProcessor::LFO2SyncRate = "lfo2_sync_rate";
const juce::String ZenithPolySynthProcessor::LFO2Retr = "lfo2_retr";

//==============================================================================
// ZenithPolySynthProcessor
//==============================================================================

ZenithPolySynthProcessor::ZenithPolySynthProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "PARAMS", createParameterLayout()) {
  for (int i = 0; i < currentMaxVoices_; ++i) {
    synthesiser_.addVoice(new ZenithPolySynthVoice());
  }
  synthesiser_.addSound(new ZenithPolySynthSound());
}

ZenithPolySynthProcessor::~ZenithPolySynthProcessor() {}

void ZenithPolySynthProcessor::prepareToPlay(double sampleRate,
                                             int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  synthesiser_.setCurrentPlaybackSampleRate(sampleRate);
  effects_.setSampleRate(sampleRate);
  effects_.reset();

  visualizerFifo_.reset();
  std::fill(visualizerBuffer_.begin(), visualizerBuffer_.end(), 0.0f);

  for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
    if (auto *voice =
            dynamic_cast<ZenithPolySynthVoice *>(synthesiser_.getVoice(i))) {
      voice->setSampleRate(sampleRate);
    }
  }
}

void ZenithPolySynthProcessor::releaseResources() {}
//==============================================================================
void ZenithPolySynthProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                            juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  // Get BPM
  if (auto *ph = getPlayHead()) {
    if (auto pos = ph->getPosition()) {
      if (pos->getBpm())
        currentBpm_ = *pos->getBpm();
    }
  }

  // Update voice parameters before processing
  updateVoiceParameters();

  buffer.clear();
  synthesiser_.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

  // Effects
  if (buffer.getNumChannels() == 2) {
    auto *left = buffer.getWritePointer(0);
    auto *right = buffer.getWritePointer(1);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      effects_.process(left[i], right[i]);
    }
  } else if (buffer.getNumChannels() > 0) {
    auto *left = buffer.getWritePointer(0);
    float dummyRight = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      effects_.process(left[i], dummyRight);
    }
  }

  if (buffer.getNumChannels() > 0) {
    const float *channelData = buffer.getReadPointer(0);
    pushToVisualizer(channelData, buffer.getNumSamples());
  }
}

juce::AudioProcessorEditor *ZenithPolySynthProcessor::createEditor() {
  return new ZenithPolySynthUI(*this);
}

void ZenithPolySynthProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = parameters_.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void ZenithPolySynthProcessor::setStateInformation(const void *data,
                                                   int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));
  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(parameters_.state.getType()))
      parameters_.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithPolySynthProcessor::createParameterLayout() {
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

  // Filter
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
<<<<<<< HEAD
      FilterType, "Filter Type",
=======
      FilterTypeParam, "Filter Type",
>>>>>>> master
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
<<<<<<< HEAD
      FilterKeyTrack, "Filter Key Track",
=======
      FilterKeyTrackParam, "Filter Key Track",
>>>>>>> master
      juce::StringArray{"Off", "50%", "100%"}, 0));

  // Envelopes
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

  // LFOs
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

  // Performance
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

  params.push_back(std::make_unique<juce::AudioParameterInt>(
      MaxVoices, "Max Voices", 1, 32, 16));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      QualitySetting, "Quality", juce::StringArray{"Low", "Medium", "High"},
      1));

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

  return {params.begin(), params.end()};
}

void ZenithPolySynthProcessor::updateVoiceParameters() {
  // Read all params atomic
  int osc1Wave =
      static_cast<int>(parameters_.getRawParameterValue(Osc1Wave)->load());
  float osc1Detune = parameters_.getRawParameterValue(Osc1Detune)->load();
  float osc1Mix = parameters_.getRawParameterValue(Osc1Mix)->load();
  float osc1Shape = parameters_.getRawParameterValue(Osc1Shape)->load();

  int osc2Wave =
      static_cast<int>(parameters_.getRawParameterValue(Osc2Wave)->load());
  float osc2Detune = parameters_.getRawParameterValue(Osc2Detune)->load();
  float osc2Mix = parameters_.getRawParameterValue(Osc2Mix)->load();
  float osc2Shape = parameters_.getRawParameterValue(Osc2Shape)->load();

  int osc3Wave =
      static_cast<int>(parameters_.getRawParameterValue(Osc3Wave)->load());
  float osc3Detune = parameters_.getRawParameterValue(Osc3Detune)->load();
  float osc3Mix = parameters_.getRawParameterValue(Osc3Mix)->load();
  float osc3Shape = parameters_.getRawParameterValue(Osc3Shape)->load();

  float noiseLevel = parameters_.getRawParameterValue(NoiseLevel)->load();
  float subOscLevel = parameters_.getRawParameterValue(SubOscLevel)->load();
  int subOscOctave =
      static_cast<int>(parameters_.getRawParameterValue(SubOscOctave)->load());

  int unisonVoices =
      static_cast<int>(parameters_.getRawParameterValue(UnisonVoices)->load());
  float unisonDetune = parameters_.getRawParameterValue(UnisonDetune)->load();

  bool osc2Sync = parameters_.getRawParameterValue(Osc2Sync)->load() > 0.5f;
  float osc2FM = parameters_.getRawParameterValue(Osc2FM)->load();
  float ringMod = parameters_.getRawParameterValue(RingMod)->load();
  int filterModel =
      static_cast<int>(parameters_.getRawParameterValue(FilterModel)->load());

<<<<<<< HEAD
  int filterType =
      static_cast<int>(parameters_.getRawParameterValue(FilterType)->load());
=======
  int filterType = static_cast<int>(
      parameters_.getRawParameterValue(FilterTypeParam)->load());
>>>>>>> master
  float filterCutoff = parameters_.getRawParameterValue(FilterCutoff)->load();
  float filterResonance =
      parameters_.getRawParameterValue(FilterResonance)->load();
  float filterDrive = parameters_.getRawParameterValue(FilterDrive)->load();
  float filterEnvAmount =
      parameters_.getRawParameterValue(FilterEnvAmount)->load();
  int filterKeyTrack = static_cast<int>(
<<<<<<< HEAD
      parameters_.getRawParameterValue(FilterKeyTrack)->load());
=======
      parameters_.getRawParameterValue(FilterKeyTrackParam)->load());
>>>>>>> master

  float ampAttack = parameters_.getRawParameterValue(AmpAttack)->load();
  float ampDecay = parameters_.getRawParameterValue(AmpDecay)->load();
  float ampSustain = parameters_.getRawParameterValue(AmpSustain)->load();
  float ampRelease = parameters_.getRawParameterValue(AmpRelease)->load();

  float modAttack = parameters_.getRawParameterValue(ModAttack)->load();
  float modDecay = parameters_.getRawParameterValue(ModDecay)->load();
  float modSustain = parameters_.getRawParameterValue(ModSustain)->load();
  float modRelease = parameters_.getRawParameterValue(ModRelease)->load();

  float lfo1Rate = parameters_.getRawParameterValue(LFO1Rate)->load();
  float lfo1Amount = parameters_.getRawParameterValue(LFO1Amount)->load();
  int lfo1Target =
      static_cast<int>(parameters_.getRawParameterValue(LFO1Target)->load());
  int lfo1Waveform =
      static_cast<int>(parameters_.getRawParameterValue(LFO1Waveform)->load());
  bool lfo1Sync = parameters_.getRawParameterValue(LFO1Sync)->load() > 0.5f;
  int lfo1SyncIdx =
      static_cast<int>(parameters_.getRawParameterValue(LFO1SyncRate)->load());
  bool lfo1RetrVal = parameters_.getRawParameterValue(LFO1Retr)->load() > 0.5f;

  float lfo2Rate = parameters_.getRawParameterValue(LFO2Rate)->load();
  float lfo2Amount = parameters_.getRawParameterValue(LFO2Amount)->load();
  int lfo2Target =
      static_cast<int>(parameters_.getRawParameterValue(LFO2Target)->load());
  int lfo2Waveform =
      static_cast<int>(parameters_.getRawParameterValue(LFO2Waveform)->load());
  bool lfo2Sync = parameters_.getRawParameterValue(LFO2Sync)->load() > 0.5f;
  int lfo2SyncIdx =
      static_cast<int>(parameters_.getRawParameterValue(LFO2SyncRate)->load());
  bool lfo2RetrVal = parameters_.getRawParameterValue(LFO2Retr)->load() > 0.5f;

  float glideTime = parameters_.getRawParameterValue(GlideTime)->load();
  bool monoMode = parameters_.getRawParameterValue(MonoMode)->load() > 0.5f;
  float masterGain = parameters_.getRawParameterValue(MasterGain)->load();
  int pitchBendRange = static_cast<int>(
      parameters_.getRawParameterValue(PitchBendRange)->load());
  float velocityCurve = parameters_.getRawParameterValue(VelocityCurve)->load();
  int qualitySetting = static_cast<int>(
      parameters_.getRawParameterValue(QualitySetting)->load());

  float delayTime = parameters_.getRawParameterValue(DelayTime)->load();
  float delayFeedback = parameters_.getRawParameterValue(DelayFeedback)->load();
  float delayMix = parameters_.getRawParameterValue(DelayMix)->load();
  bool delaySync = parameters_.getRawParameterValue(DelaySync)->load() > 0.5f;
  int delaySyncIdx =
      static_cast<int>(parameters_.getRawParameterValue(DelaySyncRate)->load());

  effects_.setDistortion(
      parameters_.getRawParameterValue(DistortionAmount)->load());
  effects_.setChorus(parameters_.getRawParameterValue(ChorusAmount)->load());
  effects_.setReverb(parameters_.getRawParameterValue(ReverbAmount)->load());
  effects_.setDelay(delayTime, delayFeedback, delayMix);
  effects_.setBpm(currentBpm_);
  effects_.setDelaySync(delaySync, static_cast<SyncRate>(delaySyncIdx + 1));

  for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
    if (auto *voice =
            dynamic_cast<ZenithPolySynthVoice *>(synthesiser_.getVoice(i))) {
      voice->setOsc1Waveform(static_cast<OscillatorWaveform>(osc1Wave));
      voice->setOsc2Waveform(static_cast<OscillatorWaveform>(osc2Wave));
      voice->setOsc3Waveform(static_cast<OscillatorWaveform>(osc3Wave));

      voice->setOsc1Detune(osc1Detune);
      voice->setOsc2Detune(osc2Detune);
      voice->setOsc3Detune(osc3Detune);

      voice->setOsc1Mix(osc1Mix);
      voice->setOsc2Mix(osc2Mix);
      voice->setOsc3Mix(osc3Mix);

      voice->setOsc1Shape(osc1Shape);
      voice->setOsc2Shape(osc2Shape);
      voice->setOsc3Shape(osc3Shape);

      voice->setSubOscLevel(subOscLevel);
      voice->setSubOscOctave(subOscOctave == 0 ? -2 : -1);

      voice->setNoiseLevel(noiseLevel);

      voice->setUnisonVoices(unisonVoices);
      voice->setUnisonDetune(unisonDetune);

      voice->setOsc2Sync(osc2Sync);
      voice->setOsc2FM(osc2FM);
      voice->setRingMod(ringMod);
      voice->setFilterModel(filterModel);

<<<<<<< HEAD
      voice->setFilterType(static_cast<zenith::FilterType>(filterType));
=======
      voice->setFilterType(static_cast<FilterType>(filterType));
>>>>>>> master
      voice->setFilterCutoff(filterCutoff);
      voice->setFilterResonance(filterResonance);
      voice->setFilterDrive(filterDrive);
      voice->setFilterEnvAmount(filterEnvAmount);
<<<<<<< HEAD
      voice->setFilterKeyTrack(
          static_cast<zenith::FilterKeyTrack>(filterKeyTrack));
=======
      voice->setFilterKeyTrack(static_cast<FilterKeyTrack>(filterKeyTrack));
>>>>>>> master

      voice->setAmpEnvelope(ampAttack, ampDecay, ampSustain, ampRelease);
      voice->setModEnvelope(modAttack, modDecay, modSustain, modRelease);

      voice->setLFO1(lfo1Rate, lfo1Amount, static_cast<LFOTarget>(lfo1Target),
                     static_cast<LFOWaveform>(lfo1Waveform));
      voice->setLFO1Sync(lfo1Sync, static_cast<SyncRate>(lfo1SyncIdx + 1),
                         lfo1RetrVal);
      voice->setLFO2(lfo2Rate, lfo2Amount, static_cast<LFOTarget>(lfo2Target),
                     static_cast<LFOWaveform>(lfo2Waveform));
      voice->setLFO2Sync(lfo2Sync, static_cast<SyncRate>(lfo2SyncIdx + 1),
                         lfo2RetrVal);
      voice->setBpm(currentBpm_);

      voice->setGlideTime(glideTime);
      voice->setMonoMode(monoMode);
      voice->setMasterGain(masterGain);
      voice->setPitchBendRange(pitchBendRange);
      voice->setVelocityCurve(velocityCurve);
      voice->setQualityPreset(static_cast<QualityPreset>(qualitySetting));
    }
  }
}

float ZenithPolySynthProcessor::getModulationMatrix(
    ModulationSource src, ModulationDestination dst) const {
  for (const auto &slot : globalModMatrix_) {
    if (slot.source == src && slot.destination == dst)
      return slot.amount;
  }
  return 0.0f;
}

void ZenithPolySynthProcessor::setModulationMatrix(ModulationSource src,
                                                   ModulationDestination dst,
                                                   float amount) {
  for (auto &slot : globalModMatrix_) {
    if (slot.source == src && slot.destination == dst) {
      slot.amount = amount;
      return;
    }
  }
  for (auto &slot : globalModMatrix_) {
    if (slot.source == ModulationSource::None) {
      slot.source = src;
      slot.destination = dst;
      slot.amount = amount;
      return;
    }
  }
}

int ZenithPolySynthProcessor::readFromVisualizer(float *buffer,
                                                 int numSamples) {
  int read = 0;
  auto numReadable = visualizerFifo_.getNumReady();
  if (numReadable > 0) {
    int start1, size1, start2, size2;
    visualizerFifo_.prepareToRead(std::min(numReadable, numSamples), start1,
                                  size1, start2, size2);
    if (size1 > 0)
      memcpy(buffer, visualizerBuffer_.data() + start1, size1 * sizeof(float));
    if (size2 > 0)
      memcpy(buffer + size1, visualizerBuffer_.data() + start2,
             size2 * sizeof(float));
    visualizerFifo_.finishedRead(size1 + size2);
    read = size1 + size2;
  }
  return read;
}

void ZenithPolySynthProcessor::pushToVisualizer(const float *buffer,
                                                int numSamples) {
  int start1, size1, start2, size2;
  visualizerFifo_.prepareToWrite(numSamples, start1, size1, start2, size2);
  if (size1 > 0)
    memcpy(visualizerBuffer_.data() + start1, buffer, size1 * sizeof(float));
  if (size2 > 0)
    memcpy(visualizerBuffer_.data() + start2, buffer + size1,
           size2 * sizeof(float));
  visualizerFifo_.finishedWrite(size1 + size2);
}

void ZenithPolySynthProcessor::updateVoiceCount() {
  int targetVoices =
      static_cast<int>(parameters_.getRawParameterValue(MaxVoices)->load());
  if (targetVoices != currentMaxVoices_) {
    while (synthesiser_.getNumVoices() > targetVoices)
      synthesiser_.removeVoice(synthesiser_.getNumVoices() - 1);
    while (synthesiser_.getNumVoices() < targetVoices)
      synthesiser_.addVoice(new ZenithPolySynthVoice());
    currentMaxVoices_ = targetVoices;
  }
}

ZenithPolySynth::ZenithPolySynth()
    : InstrumentBase(std::make_unique<ZenithPolySynthProcessor>(),
                     createMetadata()) {
  registerPresets();
}

InstrumentMetadata ZenithPolySynth::createMetadata() {
  InstrumentMetadata meta;
  meta.instrumentId = "zenith_poly_synth";
  meta.name = "Zenith Poly Synth";
  meta.category = "Synthesizer";
  meta.description = "Flagship Virtual Analog & Wavetable Synthesizer.";
  meta.tags = {"analog", "poly", "flagship", "fm", "wavetable"};
  return meta;
}

void ZenithPolySynth::registerPresets() {}

} // namespace zenith
