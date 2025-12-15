/*
  ==============================================================================

    ZenithPolySynth.cpp
    Refactored: 2025-12-11
    Author:  Zenith DAW

    REFACTORED: Parameter management extracted to
    ZenithPolySynthParameterManager. This file now contains a much cleaner
    processor with delegation to the manager. Parameter IDs are aliases
    to the manager's constants.

  ==============================================================================
*/

#include "ZenithPolySynth.h"
#include "ZenithPolySynthUI.h"
#include "ContentPaths.h"
#include "ZenithPolySynthVoice.h"
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
// Parameter IDs
//==============================================================================
const juce::String &ZenithPolySynthProcessor::Osc1Wave =
    ZenithPolySynthParameterManager::Osc1Wave;
const juce::String &ZenithPolySynthProcessor::Osc1Detune =
    ZenithPolySynthParameterManager::Osc1Detune;
const juce::String &ZenithPolySynthProcessor::Osc1Mix =
    ZenithPolySynthParameterManager::Osc1Mix;
const juce::String &ZenithPolySynthProcessor::Osc1Shape =
    ZenithPolySynthParameterManager::Osc1Shape;

const juce::String &ZenithPolySynthProcessor::Osc2Wave =
    ZenithPolySynthParameterManager::Osc2Wave;
const juce::String &ZenithPolySynthProcessor::Osc2Detune =
    ZenithPolySynthParameterManager::Osc2Detune;
const juce::String &ZenithPolySynthProcessor::Osc2Mix =
    ZenithPolySynthParameterManager::Osc2Mix;
const juce::String &ZenithPolySynthProcessor::Osc2Shape =
    ZenithPolySynthParameterManager::Osc2Shape;

const juce::String &ZenithPolySynthProcessor::Osc3Wave =
    ZenithPolySynthParameterManager::Osc3Wave;
const juce::String &ZenithPolySynthProcessor::Osc3Detune =
    ZenithPolySynthParameterManager::Osc3Detune;
const juce::String &ZenithPolySynthProcessor::Osc3Mix =
    ZenithPolySynthParameterManager::Osc3Mix;
const juce::String &ZenithPolySynthProcessor::Osc3Shape =
    ZenithPolySynthParameterManager::Osc3Shape;

const juce::String &ZenithPolySynthProcessor::NoiseLevel =
    ZenithPolySynthParameterManager::NoiseLevel;
const juce::String &ZenithPolySynthProcessor::SubOscLevel =
    ZenithPolySynthParameterManager::SubOscLevel;
const juce::String &ZenithPolySynthProcessor::SubOscOctave =
    ZenithPolySynthParameterManager::SubOscOctave;
const juce::String &ZenithPolySynthProcessor::FilterEnvAmount =
    ZenithPolySynthParameterManager::FilterEnvAmount;

const juce::String &ZenithPolySynthProcessor::UnisonVoices =
    ZenithPolySynthParameterManager::UnisonVoices;
const juce::String &ZenithPolySynthProcessor::UnisonDetune =
    ZenithPolySynthParameterManager::UnisonDetune;

const juce::String &ZenithPolySynthProcessor::FilterType =
    ZenithPolySynthParameterManager::FilterType;
const juce::String &ZenithPolySynthProcessor::FilterCutoff =
    ZenithPolySynthParameterManager::FilterCutoff;
const juce::String &ZenithPolySynthProcessor::FilterResonance =
    ZenithPolySynthParameterManager::FilterResonance;
const juce::String &ZenithPolySynthProcessor::FilterDrive =
    ZenithPolySynthParameterManager::FilterDrive;
const juce::String &ZenithPolySynthProcessor::FilterKeyTrack =
    ZenithPolySynthParameterManager::FilterKeyTrack;
const juce::String &ZenithPolySynthProcessor::FilterModel =
    ZenithPolySynthParameterManager::FilterModel;

const juce::String &ZenithPolySynthProcessor::AmpAttack =
    ZenithPolySynthParameterManager::AmpAttack;
const juce::String &ZenithPolySynthProcessor::AmpDecay =
    ZenithPolySynthParameterManager::AmpDecay;
const juce::String &ZenithPolySynthProcessor::AmpSustain =
    ZenithPolySynthParameterManager::AmpSustain;
const juce::String &ZenithPolySynthProcessor::AmpRelease =
    ZenithPolySynthParameterManager::AmpRelease;

const juce::String &ZenithPolySynthProcessor::ModAttack =
    ZenithPolySynthParameterManager::ModAttack;
const juce::String &ZenithPolySynthProcessor::ModDecay =
    ZenithPolySynthParameterManager::ModDecay;
const juce::String &ZenithPolySynthProcessor::ModSustain =
    ZenithPolySynthParameterManager::ModSustain;
const juce::String &ZenithPolySynthProcessor::ModRelease =
    ZenithPolySynthParameterManager::ModRelease;

const juce::String &ZenithPolySynthProcessor::LFO1Rate =
    ZenithPolySynthParameterManager::LFO1Rate;
const juce::String &ZenithPolySynthProcessor::LFO1Amount =
    ZenithPolySynthParameterManager::LFO1Amount;
const juce::String &ZenithPolySynthProcessor::LFO1Target =
    ZenithPolySynthParameterManager::LFO1Target;
const juce::String &ZenithPolySynthProcessor::LFO1Waveform =
    ZenithPolySynthParameterManager::LFO1Waveform;
const juce::String &ZenithPolySynthProcessor::LFO1Sync =
    ZenithPolySynthParameterManager::LFO1Sync;
const juce::String &ZenithPolySynthProcessor::LFO1SyncRate =
    ZenithPolySynthParameterManager::LFO1SyncRate;
const juce::String &ZenithPolySynthProcessor::LFO1Retr =
    ZenithPolySynthParameterManager::LFO1Retr;

const juce::String &ZenithPolySynthProcessor::LFO2Rate =
    ZenithPolySynthParameterManager::LFO2Rate;
const juce::String &ZenithPolySynthProcessor::LFO2Amount =
    ZenithPolySynthParameterManager::LFO2Amount;
const juce::String &ZenithPolySynthProcessor::LFO2Target =
    ZenithPolySynthParameterManager::LFO2Target;
const juce::String &ZenithPolySynthProcessor::LFO2Waveform =
    ZenithPolySynthParameterManager::LFO2Waveform;
const juce::String &ZenithPolySynthProcessor::LFO2Sync =
    ZenithPolySynthParameterManager::LFO2Sync;
const juce::String &ZenithPolySynthProcessor::LFO2SyncRate =
    ZenithPolySynthParameterManager::LFO2SyncRate;
const juce::String &ZenithPolySynthProcessor::LFO2Retr =
    ZenithPolySynthParameterManager::LFO2Retr;

const juce::String &ZenithPolySynthProcessor::GlideTime =
    ZenithPolySynthParameterManager::GlideTime;
const juce::String &ZenithPolySynthProcessor::MonoMode =
    ZenithPolySynthParameterManager::MonoMode;
const juce::String &ZenithPolySynthProcessor::MasterGain =
    ZenithPolySynthParameterManager::MasterGain;
const juce::String &ZenithPolySynthProcessor::PitchBendRange =
    ZenithPolySynthParameterManager::PitchBendRange;
const juce::String &ZenithPolySynthProcessor::VelocityCurve =
    ZenithPolySynthParameterManager::VelocityCurve;

const juce::String &ZenithPolySynthProcessor::MaxVoices =
    ZenithPolySynthParameterManager::MaxVoices;
const juce::String &ZenithPolySynthProcessor::QualitySetting =
    ZenithPolySynthParameterManager::QualitySetting;

const juce::String &ZenithPolySynthProcessor::Osc2Sync =
    ZenithPolySynthParameterManager::Osc2Sync;
const juce::String &ZenithPolySynthProcessor::Osc2FM =
    ZenithPolySynthParameterManager::Osc2FM;
const juce::String &ZenithPolySynthProcessor::RingMod =
    ZenithPolySynthParameterManager::RingMod;

const juce::String &ZenithPolySynthProcessor::DistortionAmount =
    ZenithPolySynthParameterManager::DistortionAmount;
const juce::String &ZenithPolySynthProcessor::ChorusAmount =
    ZenithPolySynthParameterManager::ChorusAmount;
const juce::String &ZenithPolySynthProcessor::ReverbAmount =
    ZenithPolySynthParameterManager::ReverbAmount;
const juce::String &ZenithPolySynthProcessor::DelayTime =
    ZenithPolySynthParameterManager::DelayTime;
const juce::String &ZenithPolySynthProcessor::DelayFeedback =
    ZenithPolySynthParameterManager::DelayFeedback;
const juce::String &ZenithPolySynthProcessor::DelayMix =
    ZenithPolySynthParameterManager::DelayMix;
const juce::String &ZenithPolySynthProcessor::DelaySync =
    ZenithPolySynthParameterManager::DelaySync;
const juce::String &ZenithPolySynthProcessor::DelaySyncRate =
    ZenithPolySynthParameterManager::DelaySyncRate;

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

void ZenithPolySynth::registerPresets() {
  auto contentRoot = ContentPaths::getInstance().getContentRoot();
  auto presetDir =
      contentRoot.getChildFile("Presets").getChildFile("ZenithPolySynth");

  if (!presetDir.exists()) {
    DBG("ZenithPolySynth presets directory not found: "
        << presetDir.getFullPathName());
    return;
  }

  // Find all bank files
  auto bankFiles =
      presetDir.findChildFiles(juce::File::findFiles, false, "*_bank.json");

  for (const auto &bankFile : bankFiles) {
    juce::String jsonString = bankFile.loadFileAsString();
    auto result = juce::JSON::parse(jsonString);

    if (!result.isObject())
      continue;

    auto *bankObj = result.getDynamicObject();
    if (!bankObj)
      continue;

    auto presetsVar = bankObj->getProperty("presets");
    if (!presetsVar.isArray())
      continue;

    auto *presetsArray = presetsVar.getArray();
    for (const auto &presetVar : *presetsArray) {
      if (!presetVar.isObject())
        continue;
      auto *presetObj = presetVar.getDynamicObject();

      juce::String id = presetObj->getProperty("id").toString();
      juce::String name = presetObj->getProperty("name").toString();
      auto parametersVar = presetObj->getProperty("parameters");

      std::map<juce::String, float> values;

      if (parametersVar.isObject()) {
        auto *paramsObj = parametersVar.getDynamicObject();
        for (auto &prop : paramsObj->getProperties()) {
          juce::String key = prop.name.toString();
          float val = static_cast<float>(static_cast<double>(prop.value));

          // Mapping logic from simplified JSON to internal parameters

          static const std::map<juce::String, juce::String> paramMap = {
              {"filter_cutoff", ZenithPolySynthParameterManager::FilterCutoff},
              {"filter_resonance",
               ZenithPolySynthParameterManager::FilterResonance},
              {"attack", ZenithPolySynthParameterManager::AmpAttack},
              {"decay", ZenithPolySynthParameterManager::AmpDecay},
              {"sustain", ZenithPolySynthParameterManager::AmpSustain},
              {"release", ZenithPolySynthParameterManager::AmpRelease}};

          if (key == "osc_type") {
            // Map 0, 1, 2, ... into normalized range for 7 choices
            // 0 -> 0/6, 1 -> 1/6, etc.
            constexpr float numOscWaveforms = 7.0f;
            values[ZenithPolySynthParameterManager::Osc1Wave] =
                val / (numOscWaveforms - 1.0f);
          } else {
            auto it = paramMap.find(key);
            if (it != paramMap.end()) {
              values[it->second] = val;
            }
          }
        }
      }
      registerPreset(id, name, values);
    }
  }
}

} // namespace zenith
