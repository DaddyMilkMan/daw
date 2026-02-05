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
#include "../ui/instruments/ZenithPolySynthUI.h"
#include "../ai/WingmanSynthBridge.h"
#include "ContentPaths.h"
#include "ZenithPolySynthVoice.h"
#include "ZenithPolySynth/sequencer/Arpeggiator.h"
#include "ZenithPolySynth/lfos/StepLFO.h"
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
const juce::String &ZenithPolySynthProcessor::UnisonSpread =
    ZenithPolySynthParameterManager::UnisonSpread;
const juce::String &ZenithPolySynthProcessor::UnisonPanRandom =
    ZenithPolySynthParameterManager::UnisonPanRandom;

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

// Arpeggiator Parameters
const juce::String &ZenithPolySynthProcessor::ArpEnable =
    ZenithPolySynthParameterManager::ArpEnable;
const juce::String &ZenithPolySynthProcessor::ArpMode =
    ZenithPolySynthParameterManager::ArpMode;
const juce::String &ZenithPolySynthProcessor::ArpRate =
    ZenithPolySynthParameterManager::ArpRate;
const juce::String &ZenithPolySynthProcessor::ArpSync =
    ZenithPolySynthParameterManager::ArpSync;
const juce::String &ZenithPolySynthProcessor::ArpSyncRate =
    ZenithPolySynthParameterManager::ArpSyncRate;
const juce::String &ZenithPolySynthProcessor::ArpGate =
    ZenithPolySynthParameterManager::ArpGate;
const juce::String &ZenithPolySynthProcessor::ArpOctaves =
    ZenithPolySynthParameterManager::ArpOctaves;
const juce::String &ZenithPolySynthProcessor::ArpSwing =
    ZenithPolySynthParameterManager::ArpSwing;
const juce::String &ZenithPolySynthProcessor::ArpHold =
    ZenithPolySynthParameterManager::ArpHold;

// Step LFO Parameters
const juce::String &ZenithPolySynthProcessor::StepLFO1Enable =
    ZenithPolySynthParameterManager::StepLFO1Enable;
const juce::String &ZenithPolySynthProcessor::StepLFO1Steps =
    ZenithPolySynthParameterManager::StepLFO1Steps;
const juce::String &ZenithPolySynthProcessor::StepLFO1Rate =
    ZenithPolySynthParameterManager::StepLFO1Rate;
const juce::String &ZenithPolySynthProcessor::StepLFO1Sync =
    ZenithPolySynthParameterManager::StepLFO1Sync;
const juce::String &ZenithPolySynthProcessor::StepLFO1Smoothing =
    ZenithPolySynthParameterManager::StepLFO1Smoothing;
const juce::String &ZenithPolySynthProcessor::StepLFO2Enable =
    ZenithPolySynthParameterManager::StepLFO2Enable;
const juce::String &ZenithPolySynthProcessor::StepLFO2Steps =
    ZenithPolySynthParameterManager::StepLFO2Steps;
const juce::String &ZenithPolySynthProcessor::StepLFO2Rate =
    ZenithPolySynthParameterManager::StepLFO2Rate;
const juce::String &ZenithPolySynthProcessor::StepLFO2Sync =
    ZenithPolySynthParameterManager::StepLFO2Sync;
const juce::String &ZenithPolySynthProcessor::StepLFO2Smoothing =
    ZenithPolySynthParameterManager::StepLFO2Smoothing;
const juce::String &ZenithPolySynthProcessor::StepLFO3Enable =
    ZenithPolySynthParameterManager::StepLFO3Enable;
const juce::String &ZenithPolySynthProcessor::StepLFO3Steps =
    ZenithPolySynthParameterManager::StepLFO3Steps;
const juce::String &ZenithPolySynthProcessor::StepLFO3Rate =
    ZenithPolySynthParameterManager::StepLFO3Rate;
const juce::String &ZenithPolySynthProcessor::StepLFO3Sync =
    ZenithPolySynthParameterManager::StepLFO3Sync;
const juce::String &ZenithPolySynthProcessor::StepLFO3Smoothing =
    ZenithPolySynthParameterManager::StepLFO3Smoothing;
const juce::String &ZenithPolySynthProcessor::StepLFO4Enable =
    ZenithPolySynthParameterManager::StepLFO4Enable;
const juce::String &ZenithPolySynthProcessor::StepLFO4Steps =
    ZenithPolySynthParameterManager::StepLFO4Steps;
const juce::String &ZenithPolySynthProcessor::StepLFO4Rate =
    ZenithPolySynthParameterManager::StepLFO4Rate;
const juce::String &ZenithPolySynthProcessor::StepLFO4Sync =
    ZenithPolySynthParameterManager::StepLFO4Sync;
const juce::String &ZenithPolySynthProcessor::StepLFO4Smoothing =
    ZenithPolySynthParameterManager::StepLFO4Smoothing;

//==============================================================================
// ZenithPolySynthProcessor
//==============================================================================

ZenithPolySynthProcessor::ZenithPolySynthProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "PARAMS",
                  ZenithPolySynthParameterManager::createParameterLayout()),
      paramManager_(parameters_) {
  
  // Initialize Wingman bridge AFTER all other members
  wingmanBridge_ = new WingmanSynthBridge(*this);

  // Initialize Arpeggiator
  arpeggiator_ = std::make_unique<Arpeggiator>();
  arpeggiator_->setBPM(currentBpm_.get());

  // Initialize Step LFOs
  for (int i = 0; i < 4; ++i) {
    stepLFOs_[i] = std::make_unique<StepLFO>();
    stepLFOs_[i]->setBPM(currentBpm_.get());
  }
  
  for (int i = 0; i < currentMaxVoices_; ++i) {
    // Bug 21: addVoice takes ownership of the voice object
    synthesiser_.addVoice(new ZenithPolySynthVoice());
  }
  // Enable MPE (disable legacy mode)
  synthesiser_.enableLegacyMode(false);
  // Default zone layout (all channels)
  synthesiser_.setZoneLayout(juce::MPEZoneLayout());
}

ZenithPolySynthProcessor::~ZenithPolySynthProcessor() {
  // Cleanup Wingman bridge BEFORE other members
  if (wingmanBridge_ != nullptr) {
    delete wingmanBridge_;
    wingmanBridge_ = nullptr;
  }
}

void ZenithPolySynthProcessor::prepareToPlay(double sampleRate,
                                             int samplesPerBlock) {
  // Roast Fix #5: Use samplesPerBlock for proper buffer sizing
  currentBlockSize_ = samplesPerBlock;
  synthesiser_.setCurrentPlaybackSampleRate(sampleRate);
  effects_.setSampleRate(sampleRate);
  effects_.setBlockSize(samplesPerBlock);  // Propagate buffer size to effects
  effects_.reset();

  // Initialize StepLFOs with sample rate
  for (int i = 0; i < 4; ++i) {
    if (stepLFOs_[i]) {
      stepLFOs_[i]->setSampleRate(sampleRate);
    }
  }

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
        currentBpm_.set(*pos->getBpm());
    }
  }

  // Update voice parameters before processing
  updateVoiceParameters();

  // Process Step LFOs - do this before voice rendering so voices can read current values
  processStepLFOs(buffer.getNumSamples());

  // Update voices with current StepLFO outputs
  updateVoicesWithStepLFOs();

  // Process Arpeggiator - transforms MIDI before sending to synthesiser
  processArpeggiator(midiMessages, buffer.getNumSamples());

  buffer.clear();

  {
      // Bug 8 Fix: usage of voiceLock_
      const juce::SpinLock::ScopedLockType sl(voiceLock_);
      synthesiser_.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
  }

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
  paramManager_.applyToEffects(effects_, currentBpm_.get());

  // Apply parameters to each voice
  for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
    // Bug 75: Use static_cast for performance in audio path (type guaranteed by constructor)
    if (auto *voice =
            static_cast<ZenithPolySynthVoice *>(synthesiser_.getVoice(i))) {
      paramManager_.applyToVoice(*voice, currentBpm_.get());
    }
  }
}

//==============================================================================
// Modulation Matrix
//==============================================================================

float ZenithPolySynthProcessor::getModulationMatrix(
    ModulationSource src, ModulationDestination dst) const {
  const juce::SpinLock::ScopedLockType sl(modMatrixLock_);
  for (const auto &slot : globalModMatrix_) {
    if (slot.source == src && slot.destination == dst)
      return slot.amount;
  }
  return 0.0f;
}

void ZenithPolySynthProcessor::setModulationMatrix(ModulationSource src,
                                                   ModulationDestination dst,
                                                   float amount) {
  const juce::SpinLock::ScopedLockType sl(modMatrixLock_);
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
// Step LFO Access
//==============================================================================

float ZenithPolySynthProcessor::getStepLFOOutput(int index) const {
    if (index >= 0 && index < 4) {
        return stepLFOOutputs_[index];
    }
    return 0.0f;
}

void ZenithPolySynthProcessor::getStepLFOOutputs(float outputs[4]) const {
    for (int i = 0; i < 4; ++i) {
        outputs[i] = stepLFOOutputs_[i];
    }
}

//==============================================================================
// Arpeggiator Parameter Updates
//==============================================================================

void ZenithPolySynthProcessor::updateArpeggiatorParameters(const CachedSynthParameters& params) {
    if (!arpeggiator_) return;

    arpeggiator_->setMode(static_cast<zenith::ArpMode>(params.arpMode));
    arpeggiator_->setRate(params.arpRate);
    arpeggiator_->setSyncRate(static_cast<zenith::ArpSyncRate>(params.arpSyncRate));
    arpeggiator_->setBPM(currentBpm_.get());
    arpeggiator_->setGate(params.arpGate);
    arpeggiator_->setOctaveRange(params.arpOctaves);
    arpeggiator_->setSwing(params.arpSwing);
    arpeggiator_->setHoldMode(params.arpHold);
}

//==============================================================================
// Step LFO Parameter Updates
//==============================================================================

void ZenithPolySynthProcessor::updateStepLFOParameters(const CachedSynthParameters& params) {
    for (int i = 0; i < 4; ++i) {
        if (!stepLFOs_[i]) continue;

        bool enable = false;
        int steps = 16;
        float rate = 1.0f;
        bool sync = false;
        float smoothing = 0.0f;

        switch (i) {
        case 0:
            enable = params.stepLFO1Enable;
            steps = params.stepLFO1Steps;
            rate = params.stepLFO1Rate;
            sync = params.stepLFO1Sync;
            smoothing = params.stepLFO1Smoothing;
            break;
        case 1:
            enable = params.stepLFO2Enable;
            steps = params.stepLFO2Steps;
            rate = params.stepLFO2Rate;
            sync = params.stepLFO2Sync;
            smoothing = params.stepLFO2Smoothing;
            break;
        case 2:
            enable = params.stepLFO3Enable;
            steps = params.stepLFO3Steps;
            rate = params.stepLFO3Rate;
            sync = params.stepLFO3Sync;
            smoothing = params.stepLFO3Smoothing;
            break;
        case 3:
            enable = params.stepLFO4Enable;
            steps = params.stepLFO4Steps;
            rate = params.stepLFO4Rate;
            sync = params.stepLFO4Sync;
            smoothing = params.stepLFO4Smoothing;
            break;
        }

        stepLFOs_[i]->setNumSteps(steps);
        stepLFOs_[i]->setRate(rate);
        stepLFOs_[i]->setSyncRate(static_cast<zenith::ArpSyncRate>(sync ? 4 : 0)); // Default to 1/4 if sync
        stepLFOs_[i]->setBPM(currentBpm_.get());
        stepLFOs_[i]->setSmoothing(smoothing);
    }
}

//==============================================================================
// Arpeggiator Processing
//==============================================================================

void ZenithPolySynthProcessor::processArpeggiator(juce::MidiBuffer& midiMessages, int numSamples) {
    const auto& params = paramManager_.getCachedParameters();

    if (!params.arpEnable) {
        // When arpeggiator is disabled, just clear active notes
        if (arpeggiator_) {
            arpeggiator_->reset();
        }
        return;
    }

    if (!arpeggiator_) return;

    // Update arpeggiator with latest parameters
    updateArpeggiatorParameters(params);

    // Track original note events for the arpeggiator
    juce::MidiBuffer originalMessages = midiMessages;

    // Clear the buffer - arpeggiator will generate new messages
    midiMessages.clear();

    // First, process incoming MIDI to update arpeggiator state
    for (const auto metadata : originalMessages) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            arpeggiator_->noteOn(msg.getNoteNumber(), msg.getVelocity());
        } else if (msg.isNoteOff()) {
            arpeggiator_->noteOff(msg.getNoteNumber());
        } else {
            // Pass through non-note messages
            midiMessages.addEvent(msg, metadata.samplePosition);
        }
    }

    // Let the arpeggiator generate new note messages
    double sampleRate = getSampleRate();
    arpeggiator_->process(midiMessages, sampleRate, numSamples);
}

//==============================================================================
// Step LFO Processing
//==============================================================================

void ZenithPolySynthProcessor::processStepLFOs(int numSamples) {
    const auto& params = paramManager_.getCachedParameters();
    updateStepLFOParameters(params);

    // Create a temp buffer for processing
    std::array<float, 32> tempBuffer;
    int samplesToProcess = juce::jmin(static_cast<int>(tempBuffer.size()), numSamples);

    for (int i = 0; i < 4; ++i) {
        if (!stepLFOs_[i]) continue;

        bool enabled = false;
        switch (i) {
        case 0: enabled = params.stepLFO1Enable; break;
        case 1: enabled = params.stepLFO2Enable; break;
        case 2: enabled = params.stepLFO3Enable; break;
        case 3: enabled = params.stepLFO4Enable; break;
        }

        if (!enabled) {
            stepLFOOutputs_[i] = 0.0f;
            continue;
        }

        // Process the LFO and get the last output value
        stepLFOs_[i]->processBlock(tempBuffer.data(), samplesToProcess);
        stepLFOOutputs_[i] = stepLFOs_[i]->getCurrentOutput();
    }
}

//==============================================================================
// Update Voices with Step LFO Values
//==============================================================================

void ZenithPolySynthProcessor::updateVoicesWithStepLFOs() {
    // Get current StepLFO outputs and pass to all voices
    float lfoOutputs[4];
    getStepLFOOutputs(lfoOutputs);

    for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
        if (auto* voice = static_cast<ZenithPolySynthVoice*>(synthesiser_.getVoice(i))) {
            voice->setStepLFOValues(lfoOutputs);
        }
    }
}

//==============================================================================
// Voice Count Management
//==============================================================================

void ZenithPolySynthProcessor::updateVoiceCount() {
  int targetVoices = paramManager_.getTargetVoiceCount();
  if (targetVoices != currentMaxVoices_) {
    const juce::SpinLock::ScopedLockType sl(voiceLock_);
    while (synthesiser_.getNumVoices() > targetVoices)
      synthesiser_.removeVoice(synthesiser_.getNumVoices() - 1);
    while (synthesiser_.getNumVoices() < targetVoices)
      // Bug 21: addVoice takes ownership
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

    if (!result.isObject()) {
      DBG("ZenithPolySynth: Warning - failed to parse preset bank: " +
          bankFile.getFileName());
      continue;
    }

    auto *bankObj = result.getDynamicObject();
    if (!bankObj) {
      DBG("ZenithPolySynth: Warning - invalid bank object in: " +
          bankFile.getFileName());
      continue;
    }

    auto presetsVar = bankObj->getProperty("presets");
    if (!presetsVar.isArray()) {
      DBG("ZenithPolySynth: Warning - no 'presets' array in: " +
          bankFile.getFileName());
      continue;
    }

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
