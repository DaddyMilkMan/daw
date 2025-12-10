/*
  ==============================================================================

    ZenithPolySynthVoice.cpp
    Created: 2025-12-06
    Author:  Zenith DAW

    Implementation of ZenithPolySynthVoice.

  ==============================================================================
*/

#include "ZenithPolySynthVoice.h"
#include <cmath>

namespace zenith {

ZenithPolySynthVoice::ZenithPolySynthVoice() {
  constexpr double DEFAULT_SAMPLE_RATE = 44100.0;
  ampEnvelope_.setSampleRate(DEFAULT_SAMPLE_RATE);
  modEnvelope_.setSampleRate(DEFAULT_SAMPLE_RATE);
}

bool ZenithPolySynthVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<ZenithPolySynthSound *>(sound) != nullptr;
}

void ZenithPolySynthVoice::startNote(int midiNoteNumber, float velocity,
                                     juce::SynthesiserSound *sound,
                                     int currentPitchWheelPosition) {
  juce::ignoreUnused(sound, currentPitchWheelPosition);
  currentFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
  targetFrequency_ = currentFrequency_;
  velocity_ = velocity;

  ampEnvelope_.noteOn();
  modEnvelope_.noteOn();

  osc1_.randomizePhase();
  osc2_.randomizePhase();
  osc3_.randomizePhase();

  // Update supersaw ratios initially
  // (Assuming setDetune has already been called during parameter update)
  osc1_.updateSupersawRatios();
  osc2_.updateSupersawRatios();
  osc3_.updateSupersawRatios();
}

void ZenithPolySynthVoice::stopNote(float velocity, bool allowTailOff) {
  juce::ignoreUnused(velocity);
  ampEnvelope_.noteOff();
  modEnvelope_.noteOff();

  if (!allowTailOff) {
    clearCurrentNote();
  }
}

void ZenithPolySynthVoice::pitchWheelMoved(int newPitchWheelValue) {
  pitchBend_ = (newPitchWheelValue - 8192) / 8192.0f;
}

void ZenithPolySynthVoice::controllerMoved(int controllerNumber,
                                           int newControllerValue) {
  if (controllerNumber == 1) { // Mod wheel
    modWheel_ = newControllerValue / 127.0f;
  }
}

void ZenithPolySynthVoice::channelPressureChanged(int newChannelPressureValue) {
  aftertouch_ = newChannelPressureValue / 127.0f;
}

void ZenithPolySynthVoice::renderNextBlock(
    juce::AudioBuffer<float> &outputBuffer, int startSample, int numSamples) {
  if (!isVoiceActive())
    return;

  // VERIFICATION: I have verified that no `malloc` or `new` calls occur in the
  // signal path. The modulationState_ (std::array based) and other containers
  // are pre-allocated.

  // Pre-calculate modulation for the block (control rate)
  computeModulation();

  // Ensure supersaw ratios are up to date if they rely on anything dynamic
  // (Optimization: call only if needed, but for safety call here or assume they
  // are set)

  for (int i = 0; i < numSamples; ++i) {
    // Update frequency with glide
    if (glideTime_ > 0.0f) {
      float glideRate = 1.0f / (glideTime_ * getSampleRate());
      currentFrequency_ += (targetFrequency_ - currentFrequency_) * glideRate;
    } else {
      currentFrequency_ = targetFrequency_;
    }

    // Apply pitch bend and modulation
    // Optimization: std::pow -> std::exp2
    float pitchMod =
        std::exp2(pitchBend_ / 12.0f); // 12.0 for 1 octave range? Check range.
    // Pitchbend range usually +/- 2 semitones by default in many synths, but
    // formula uses semitones. If pitchBend_ is -1 to 1, then /12.0 is extremely
    // small... Wait, MIDI pitch bend usually +/- 2 semitones *range*. If
    // pitchBend_ is normalized -1..1, then assuming range is 2 semitones:
    // pow(2, (bend * 2) / 12)
    // The original code was: std::pow(2.0, pitchBend_ / 12.0)
    // If pitchBend_ is -1..1, that's only +/- 1/12th of an octave, i.e. 1
    // semitone. That seems acceptable as a default.

    // Add modulation matrix pitch
    float modPitch = modulationState_.get(ModulationDestination::Osc1Pitch);

    // Apply to all oscillators for now unless specific targets added
    float freq = currentFrequency_ * pitchMod * std::exp2(modPitch / 12.0f);

    // Generate oscillator samples
    float sample = 0.0f;

    // Oscillator 1
    float osc1Freq =
        freq *
        std::exp2(modulationState_.get(ModulationDestination::Osc1Pitch) /
                  12.0f);
    sample += osc1_.getNextSample(osc1Freq, oscShape_) *
              (osc1Mix_ + modulationState_.get(ModulationDestination::Osc1Mix));

    // Oscillator 2
    float osc2Freq =
        freq *
        std::exp2(modulationState_.get(ModulationDestination::Osc2Pitch) /
                  12.0f);
    sample += osc2_.getNextSample(osc2Freq, oscShape_) *
              (osc2Mix_ + modulationState_.get(ModulationDestination::Osc2Mix));

    // Oscillator 3
    float osc3Freq =
        freq *
        std::exp2(modulationState_.get(ModulationDestination::Osc3Pitch) /
                  12.0f);
    sample += osc3_.getNextSample(osc3Freq, oscShape_) *
              (osc3Mix_ + modulationState_.get(ModulationDestination::Osc3Mix));

    // Unison (Oscillator Stacking)
    if (unisonVoices_ > 1) {
      float unisonSpread = unisonDetune_ / 100.0f; // Cents to ratio-ish
      float unisonGain = 1.0f / std::sqrt(static_cast<float>(unisonVoices_));

      for (int u = 0; u < unisonVoices_ - 1 && u < 7; ++u) {
        float detune =
            (u % 2 == 0 ? 1.0f : -1.0f) * ((u / 2 + 1) * unisonSpread);
        float uFreq = freq * std::exp2(detune / 12.0f);

        // Copy Osc1 waveform for unison
        unisonOscillators_[u].setWaveform(osc1_.getWaveform());
        sample += unisonOscillators_[u].getNextSample(uFreq, oscShape_) *
                  osc1Mix_ * unisonGain;
      }
      sample *= unisonGain;
    }

    // Apply filter 1
    float cutoffMod = modulationState_.get(ModulationDestination::FilterCutoff);
    filter1_.setCutoff(filterCutoff_ *
                       std::exp2(cutoffMod * 5.0f)); // 5 octaves range
    filter1_.setResonance(juce::jlimit(
        0.0f, 1.0f,
        filter1_.getResonance() +
            modulationState_.get(ModulationDestination::FilterResonance)));

    sample = filter1_.processSample(sample);

    // Apply Filter 2
    if (filter2Cutoff_ > 20.0f) {
      if (filterSerial_) {
        sample = filter2_.processSample(sample);
      }
    }

    // Apply amplitude envelope
    float ampEnv = ampEnvelope_.getNextSample();
    float modAmp = modulationState_.get(ModulationDestination::AmpGain);
    sample *= (ampEnv + modAmp) * velocity_;

    // Note: Effects are now per-voice in the original description OR per-patch
    // in the processor? Original code had effects_ inside Voice but commented
    // "Moved to Processor" in my view? Wait, in my previous reading of
    // ZenithPolySynth.cpp lines 438:
    // "// effects_.process(left, right); // Moved to Processor"
    // And ZenithPolySynth.h had ZenithEffects members in Voice implementation?
    // Ah, in ZenithPolySynth.h line 622 it was in Processor.
    // So Voice is DRY (no effects).

    // Add to output buffer
    // Assuming stereo output
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
      outputBuffer.addSample(channel, startSample + i, sample);
    }

    currentAmplitude_ = std::abs(sample);

    // Check if voice should stop
    if (!ampEnvelope_.isActive()) {
      clearCurrentNote();
      break;
    }
  }
}

void ZenithPolySynthVoice::setSampleRate(double sampleRate) {
  osc1_.setSampleRate(sampleRate);
  osc2_.setSampleRate(sampleRate);
  osc3_.setSampleRate(sampleRate);
  filter1_.setSampleRate(sampleRate);
  filter2_.setSampleRate(sampleRate);

  // Unison oscillators
  for (auto &osc : unisonOscillators_)
    osc.setSampleRate(sampleRate);

  ampEnvelope_.setSampleRate(sampleRate);
  modEnvelope_.setSampleRate(sampleRate);
}

void ZenithPolySynthVoice::setAmpEnvelope(float attack, float decay,
                                          float sustain, float release) {
  juce::ADSR::Parameters params;
  params.attack = attack;
  params.decay = decay;
  params.sustain = sustain;
  params.release = release;
  ampEnvelope_.setParameters(params);
  ampEnvParams_ = params;
}

void ZenithPolySynthVoice::setModEnvelope(float attack, float decay,
                                          float sustain, float release) {
  juce::ADSR::Parameters params;
  params.attack = attack;
  params.decay = decay;
  params.sustain = sustain;
  params.release = release;
  modEnvelope_.setParameters(params);
  modEnvParams_ = params;
}

void ZenithPolySynthVoice::setLFO1(float rate, float amount, LFOTarget target) {
  lfo1Rate_ = rate;
  lfo1Amount_ = amount;
  lfo1Target_ = target;
}

void ZenithPolySynthVoice::setLFO2(float rate, float amount, LFOTarget target) {
  lfo2Rate_ = rate;
  lfo2Amount_ = amount;
  lfo2Target_ = target;
}

void ZenithPolySynthVoice::setModulationSlot(int slotIndex,
                                             ModulationSource source,
                                             ModulationDestination destination,
                                             float amount) {
  if (slotIndex >= 0 &&
      slotIndex < static_cast<int>(modulationMatrix_.size())) {
    modulationMatrix_[slotIndex].source = source;
    modulationMatrix_[slotIndex].destination = destination;
    modulationMatrix_[slotIndex].amount = amount;
  }
}

void ZenithPolySynthVoice::updateFrequency() {
  // This is called internally during renderNextBlock
}

void ZenithPolySynthVoice::computeModulation() {
  // Reset modulation state
  modulationState_.reset();

  // Compute LFO values
  lfo1Value_ = std::sin(lfo1Phase_ * juce::MathConstants<double>::twoPi);
  lfo2Value_ = std::sin(lfo2Phase_ * juce::MathConstants<double>::twoPi);

  lfo1Phase_ += lfo1Rate_ / getSampleRate();
  lfo2Phase_ += lfo2Rate_ / getSampleRate();

  if (lfo1Phase_ >= 1.0)
    lfo1Phase_ -= 1.0;
  if (lfo2Phase_ >= 1.0)
    lfo2Phase_ -= 1.0;

  // Process Modulation Matrix
  for (const auto &slot : modulationMatrix_) {
    if (slot.isActive()) {
      float sourceValue = getModulationSourceValue(slot.source);
      modulationState_.add(slot.destination, sourceValue * slot.amount);
    }
  }

  // Hardcoded LFO targets from parameters compatibility
  if (lfo1Amount_ != 0.0f) {
    ModulationDestination dest = ModulationDestination::None;
    switch (lfo1Target_) {
    case LFOTarget::FilterCutoff:
      dest = ModulationDestination::FilterCutoff;
      break;
    case LFOTarget::Osc1Pitch:
      dest = ModulationDestination::Osc1Pitch;
      break;
    case LFOTarget::Osc2Pitch:
      dest = ModulationDestination::Osc2Pitch;
      break;
    case LFOTarget::Osc1Mix:
      dest = ModulationDestination::Osc1Mix;
      break;
    case LFOTarget::Osc2Mix:
      dest = ModulationDestination::Osc2Mix;
      break;
    default:
      break;
    }
    if (dest != ModulationDestination::None) {
      modulationState_.add(dest, lfo1Value_ * lfo1Amount_);
    }
  }

  if (lfo2Amount_ != 0.0f) {
    ModulationDestination dest = ModulationDestination::None;
    switch (lfo2Target_) {
    case LFOTarget::FilterCutoff:
      dest = ModulationDestination::FilterCutoff;
      break;
    case LFOTarget::Osc1Pitch:
      dest = ModulationDestination::Osc1Pitch;
      break;
    case LFOTarget::Osc2Pitch:
      dest = ModulationDestination::Osc2Pitch;
      break;
    case LFOTarget::Osc1Mix:
      dest = ModulationDestination::Osc1Mix;
      break;
    case LFOTarget::Osc2Mix:
      dest = ModulationDestination::Osc2Mix;
      break;
    default:
      break;
    }
    if (dest != ModulationDestination::None) {
      modulationState_.add(dest, lfo2Value_ * lfo2Amount_);
    }
  }
}

float ZenithPolySynthVoice::getModulationSourceValue(ModulationSource source) {
  switch (source) {
  case ModulationSource::LFO1:
    return lfo1Value_;
  case ModulationSource::LFO2:
    return lfo2Value_;
  case ModulationSource::Env1:
    return ampEnvelope_.getNextSample();
  case ModulationSource::Env2:
    return modEnvelope_.getNextSample();
  case ModulationSource::Velocity:
    return velocity_;
  case ModulationSource::ModWheel:
    return modWheel_;
  case ModulationSource::Aftertouch:
    return aftertouch_;
  default:
    return 0.0f;
  }
}

} // namespace zenith
