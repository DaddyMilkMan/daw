/*
  ==============================================================================

    ZenithPolySynth.cpp
    Refactored: 2025-12-11
    Author:  Zenith DAW

    REFACTORED: Parameter management extracted to
  ZenithPolySynthParameterManager. This file now contains a much cleaner
  processor with delegation to the manager.

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
      parameters_(*this, nullptr, "PARAMS",
                  ZenithPolySynthParameterManager::createParameterLayout()),
      paramManager_(parameters_) {
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

//==============================================================================
// Voice Parameter Updates - Now delegated to parameter manager
//==============================================================================

void ZenithPolySynthProcessor::updateVoiceParameters() {
  // Fetch all parameters atomically via the manager
  paramManager_.fetchAllParameters();

  // Apply effect parameters
  paramManager_.applyToEffects(effects_, currentBpm_);

  // Apply parameters to each voice
  for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
    if (auto *voice =
            dynamic_cast<ZenithPolySynthVoice *>(synthesiser_.getVoice(i))) {
      paramManager_.applyToVoice(*voice, currentBpm_);
    }
  }
}

//==============================================================================
// Modulation Matrix
//==============================================================================

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

//==============================================================================
// Visualizer
//==============================================================================

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

//==============================================================================
// Voice Count Management
//==============================================================================

void ZenithPolySynthProcessor::updateVoiceCount() {
  int targetVoices = paramManager_.getTargetVoiceCount();
  if (targetVoices != currentMaxVoices_) {
    while (synthesiser_.getNumVoices() > targetVoices)
      synthesiser_.removeVoice(synthesiser_.getNumVoices() - 1);
    while (synthesiser_.getNumVoices() < targetVoices)
      synthesiser_.addVoice(new ZenithPolySynthVoice());
    currentMaxVoices_ = targetVoices;
  }
}

//==============================================================================
// ZenithPolySynth Instrument Wrapper
//==============================================================================

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
