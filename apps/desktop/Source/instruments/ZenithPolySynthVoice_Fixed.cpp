/*
  ==============================================================================

    ZenithPolySynthVoice.cpp
    Refactored: 2025-12-09
    Author:  Zenith DAW

    Implementation of ZenithPolySynthVoice.
    Features: Flagship Wavetables, Moog Ladder Filter, FM, Sync.
    Fixes: LFO Rate Mod, Osc Shape Mod, Detune Tuning, Smoothed Params.

  ==============================================================================
*/

#include "ZenithPolySynthVoice.h"
#include <cmath>

namespace zenith {

static float getFrequencyForSyncRate(SyncRate rate, double bpm) {
  if (bpm <= 0.0)
    bpm = 120.0;
  double beats = 1.0;
  switch (rate) {
  case SyncRate::_1_64:
    beats = 0.0625;
    break;
  case SyncRate::_1_32:
    beats = 0.125;
    break;
  case SyncRate::_1_16:
    beats = 0.25;
    break;
  case SyncRate::_1_8:
    beats = 0.5;
    break;
  case SyncRate::_1_4:
    beats = 1.0;
    break;
  case SyncRate::_1_2:
    beats = 2.0;
    break;
  case SyncRate::_1_1:
    beats = 4.0;
    break;
  case SyncRate::_2_1:
    beats = 8.0;
    break;
  case SyncRate::_4_1:
    beats = 16.0;
    break;
  default:
    beats = 1.0;
    break;
  }
  return static_cast<float>(bpm / (60.0 * beats));
}

ZenithPolySynthVoice::ZenithPolySynthVoice() {
  baseSampleRate_ = 44100.0;
  oversamplingFactor_ = 1;

  constexpr double DEFAULT_SAMPLE_RATE = 44100.0;
  ampEnvelope_.setSampleRate(DEFAULT_SAMPLE_RATE);
  modEnvelope_.setSampleRate(DEFAULT_SAMPLE_RATE);
  subOsc_.setWaveform(OscillatorWaveform::Sine);

  // Initialize Smoothed Values Defaults
  osc1Mix_.setCurrentAndTargetValue(1.0f);
  osc2Mix_.setCurrentAndTargetValue(0.0f);
  osc3Mix_.setCurrentAndTargetValue(0.0f);
  masterGain_.setCurrentAndTargetValue(0.8f);
  osc1Shape_.setCurrentAndTargetValue(0.5f);
  osc2Shape_.setCurrentAndTargetValue(0.5f);
  osc3Shape_.setCurrentAndTargetValue(0.5f);

  // Pre-allocate buffers for RT safety
  // Assumes stereo (2 channels) and a reasonable max block size
  // Max needed = maxBlockSize * maxOversampling (e.g. 4x)
  // We'll allocate for 4x oversampling at 4096 samples block size to be safe
  oversamplingBuffer_.setSize(2, 4096 * 4);
  downsamplingBuffer_.setSize(2, 4096);
}

void ZenithPolySynthVoice::noteStarted() {
  auto note = getCurrentlyPlayingNote();
  midiNoteNumber_ = note.initialNote;

  if (monoMode_ && isActive()) {
    previousFrequency_ = currentFrequency_;
  } else {
    previousFrequency_ = note.getFrequencyInHertz();
    currentFrequency_ = previousFrequency_;
    osc1_.randomizePhase();
    osc2_.randomizePhase();
    osc3_.randomizePhase();

    subOsc_.randomizePhase();
    if (lfo1Retr_)
      lfo1Phase_ = 0.0;
    if (lfo2Retr_)
      lfo2Phase_ = 0.0;
    modulationState_.reset();
  }

  targetFrequency_ = note.getFrequencyInHertz();
  velocity_ = std::pow(note.noteOnVelocity.asUnsignedFloat(), velocityCurve_);

  // Initialize MPE values
  aftertouch_ = note.pressure.asUnsignedFloat();
  timbre_ = note.timbre.asUnsignedFloat();

  // Pitch bend calculation for Mod Matrix (normalized approx)
  // ZenithPolySynth uses pitchBendRange_ parameter, we can normalize against it
  pitchBend_ = note.totalPitchbendInSemitones / (float)pitchBendRange_;

  ampEnvelope_.noteOn();
  modEnvelope_.noteOn();

  osc1_.updateSupersawRatios();
  osc2_.updateSupersawRatios();
  osc3_.updateSupersawRatios();

  prevOsc1Phase_ = osc1_.getPhase();
}

void ZenithPolySynthVoice::noteStopped(bool allowTailOff) {
  ampEnvelope_.noteOff();
  modEnvelope_.noteOff();
  if (!allowTailOff)
    clearCurrentNote();
}

void ZenithPolySynthVoice::notePressureChanged() {
  aftertouch_ = getCurrentlyPlayingNote().pressure.asUnsignedFloat();
}

void ZenithPolySynthVoice::notePitchbendChanged() {
  auto note = getCurrentlyPlayingNote();
  targetFrequency_ = note.getFrequencyInHertz();
  // Normalize for modulation source usage
  pitchBend_ = note.totalPitchbendInSemitones / (float)pitchBendRange_;
}

void ZenithPolySynthVoice::noteTimbreChanged() {
  timbre_ = getCurrentlyPlayingNote().timbre.asUnsignedFloat();
}

void ZenithPolySynthVoice::noteKeyStateChanged() {
  // Handle key state changes if needed
}

float ZenithPolySynthVoice::computeLFOValue(double phase, LFOWaveform waveform,
                                            float &shValue) {
  switch (waveform) {
  case LFOWaveform::Sine:
    return static_cast<float>(
        std::sin(phase * juce::MathConstants<double>::twoPi));
  case LFOWaveform::Triangle: {
    double t = std::fmod(phase, 1.0);
    if (t < 0.25)
      return static_cast<float>(t * 4.0);
    if (t < 0.75)
      return static_cast<float>(2.0 - t * 4.0);
    return static_cast<float>((t - 1.0) * 4.0);
  }
  case LFOWaveform::Saw:
    return static_cast<float>(1.0 - 2.0 * std::fmod(phase, 1.0));
  case LFOWaveform::Square:
    return (std::fmod(phase, 1.0) < 0.5) ? 1.0f : -1.0f;
  case LFOWaveform::SampleAndHold:
    return shValue;
  default:
    return 0.0f;
  }
}

// RT-Safe Wrapper with Oversampling support
void ZenithPolySynthVoice::renderNextBlock(
    juce::AudioBuffer<float> &outputBuffer, int startSample, int numSamples) {

  // Acquire lock to prevent race with setQualityPreset
  juce::ScopedLock sl(oversamplerLock_);

  // If oversampling is disabled or invalid
  if (oversamplingFactor_ <= 1 || oversampler_ == nullptr) {
    renderInnerBlock(outputBuffer, startSample, numSamples);
    return;
  }

  // Oversampled processing
  // Ensure we don't exceed pre-allocated buffer limits
  // If request is too large, we process in chunks
  int samplesProcessed = 0;

  while (samplesProcessed < numSamples) {
    int chunk = juce::jmin(numSamples - samplesProcessed,
                           maxBlockSize_); // Limit to max safe block
    int upsampledChunk = chunk * oversamplingFactor_;

    // Safety check against buffer sizes
    if (chunk > downsamplingBuffer_.getNumSamples() ||
        upsampledChunk > oversamplingBuffer_.getNumSamples()) {
      // Buffer size exceeded - this indicates a configuration bug.
      // In debug builds we want to catch this, but in production we gracefully
      // fall back to non-oversampled rendering to avoid audio glitches.
      #if JUCE_DEBUG
      DBG("[ZenithPolySynthVoice] WARNING: Oversampling buffer too small. "
          "chunk=" + juce::String(chunk) + ", maxBlock=" + juce::String(maxBlockSize_) +
          ". Falling back to non-oversampled processing.");
      #endif
      // Fallback: render non-oversampled to avoid crash/silence
      renderInnerBlock(outputBuffer, startSample + samplesProcessed,
                       numSamples - samplesProcessed);
      return;
    }

    // 1. Render synth logic into upsampled buffer
    // Note: Internal components (Oscs, Filters) are already configured for
    // baseRate * factor
    oversamplingBuffer_.clear(0, upsampledChunk);
    renderInnerBlock(oversamplingBuffer_, 0, upsampledChunk);

    // 2. Downsample
    juce::dsp::AudioBlock<float> upBlock(oversamplingBuffer_);
    // Slice only the valid part
    juce::dsp::AudioBlock<float> validUpBlock =
        upBlock.getSubBlock(0, upsampledChunk);

    juce::dsp::AudioBlock<float> downBlock(downsamplingBuffer_);
    juce::dsp::AudioBlock<float> validDownBlock =
        downBlock.getSubBlock(0, chunk);

    oversampler_->processSamplesDown(validDownBlock);

    // 3. Mix into output buffer
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      outputBuffer.addFrom(ch, startSample + samplesProcessed,
                           downsamplingBuffer_, ch, 0, chunk);
    }

    samplesProcessed += chunk;
  }
}

void ZenithPolySynthVoice::renderInnerBlock(
    juce::AudioBuffer<float> &outputBuffer, int startSample, int numSamples) {
  if (!isActive())
    return;

  filter1_.setModel(static_cast<FilterModelType>(filterModel_));
  filter2_.setModel(static_cast<FilterModelType>(filterModel_));

  const double sampleRate = getSampleRate();
  const double baseLfo1Inc =
      (lfo1Sync_ ? getFrequencyForSyncRate(lfo1SyncRate_, bpm_) : lfo1Rate_) /
      sampleRate;
  const double baseLfo2Inc =
      (lfo2Sync_ ? getFrequencyForSyncRate(lfo2SyncRate_, bpm_) : lfo2Rate_) /
      sampleRate;

  // Get write pointers once to avoid overhead in loop
  auto *leftOut = outputBuffer.getWritePointer(0, startSample);
  auto *rightOut = outputBuffer.getNumChannels() > 1
                       ? outputBuffer.getWritePointer(1, startSample)
                       : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    // Envelopes
    float env1 = ampEnvelope_.getNextSample();
    float env2 = modEnvelope_.getNextSample();

    // LFO Rate Modulation
    double rateMod1 =
        std::exp2(modulationState_.get(ModulationDestination::LFO1Rate) * 3.0f);
    double rateMod2 =
        std::exp2(modulationState_.get(ModulationDestination::LFO2Rate) * 3.0f);

    // LFO 1
    lfo1Phase_ += baseLfo1Inc * rateMod1;
    if (lfo1Phase_ >= 1.0) {
      lfo1Phase_ -= 1.0;
      if (lfo1Waveform_ == LFOWaveform::SampleAndHold)
        lfo1SHValue_ =
            juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }
    lfo1Value_ = computeLFOValue(lfo1Phase_, lfo1Waveform_, lfo1SHValue_);

    // LFO 2
    lfo2Phase_ += baseLfo2Inc * rateMod2;
    if (lfo2Phase_ >= 1.0) {
      lfo2Phase_ -= 1.0;
      if (lfo2Waveform_ == LFOWaveform::SampleAndHold)
        lfo2SHValue_ =
            juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }
    lfo2Value_ = computeLFOValue(lfo2Phase_, lfo2Waveform_, lfo2SHValue_);

    // Reset & Route Mod Matrix
    modulationState_.reset();

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
      case LFOTarget::AmpGain:
        dest = ModulationDestination::AmpGain;
        break;
      case LFOTarget::Osc1Shape:
        dest = ModulationDestination::Osc1Shape;
        break;
      default:
        break;
      }
      if (dest != ModulationDestination::None)
        modulationState_.add(dest, lfo1Value_ * lfo1Amount_);
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
      case LFOTarget::AmpGain:
        dest = ModulationDestination::AmpGain;
        break;
      case LFOTarget::Osc1Shape:
        dest = ModulationDestination::Osc1Shape;
        break;
      default:
        break;
      }
      if (dest != ModulationDestination::None)
        modulationState_.add(dest, lfo2Value_ * lfo2Amount_);
    }

    // Mod Matrix Slots
    for (const auto &slot : modulationMatrix_) {
      if (!slot.isActive())
        continue;
      float val = 0.0f;
      switch (slot.source) {
      case ModulationSource::LFO1:
        val = lfo1Value_;
        break;
      case ModulationSource::LFO2:
        val = lfo2Value_;
        break;
      case ModulationSource::Env1:
        val = env1;
        break;
      case ModulationSource::Env2:
        val = env2;
        break;
      case ModulationSource::Velocity:
        val = velocity_;
        break;
      case ModulationSource::ModWheel:
        val = modWheel_;
        break;
      case ModulationSource::Aftertouch:
        val = aftertouch_;
        break;
      default:
        break;
      }
      modulationState_.add(slot.destination, val * slot.amount);
    }

    // DSP
    if (glideTime_ > 0.0f && currentFrequency_ != targetFrequency_) {
      float glideCoeff =
          std::exp(-1.0f / (glideTime_ * static_cast<float>(sampleRate)));
      currentFrequency_ = targetFrequency_ +
                          (currentFrequency_ - targetFrequency_) * glideCoeff;
      if (std::abs(currentFrequency_ - targetFrequency_) < 0.01f)
        currentFrequency_ = targetFrequency_;
    } else {
      currentFrequency_ = targetFrequency_;
    }

    float baseFreq = currentFrequency_;

    // Osc Shape & Mod
    float sh1 = juce::jlimit(
        0.0f, 1.0f,
        osc1Shape_.getNextValue() +
            modulationState_.get(ModulationDestination::Osc1Shape));
    float sh2 = juce::jlimit(
        0.0f, 1.0f,
        osc2Shape_.getNextValue() +
            modulationState_.get(ModulationDestination::Osc2Shape));
    float sh3 = juce::jlimit(
        0.0f, 1.0f,
        osc3Shape_.getNextValue() +
            modulationState_.get(ModulationDestination::Osc3Shape));

    // Osc 1 (Includes Detune + Mod)
    float osc1Freq =
        baseFreq *
        std::exp2((osc1Detune_ / 100.0f +
                   modulationState_.get(ModulationDestination::Osc1Pitch)) /
                  12.0f);
    double p1_before = osc1_.getPhase();
    float osc1Sample = osc1_.getNextSample(osc1Freq, sh1);
    double p1_after = osc1_.getPhase();
    if (osc2Sync_ && p1_after < p1_before)
      osc2_.resetPhase();

    // Osc 2
    float osc2Freq =
        baseFreq *
        std::exp2((osc2Detune_ / 100.0f +
                   modulationState_.get(ModulationDestination::Osc2Pitch)) /
                  12.0f);
    if (osc2FM_ > 0.0f) {
      float fmAmountHz = osc2FM_ * 3000.0f;
      osc2Freq += osc1Sample * fmAmountHz;
      if (osc2Freq < 1.0f)
        osc2Freq = 1.0f;
    }
    float osc2Sample = osc2_.getNextSample(osc2Freq, sh2);
    if (ringMod_ > 0.0f) {
      float ringSample = osc1Sample * osc2Sample;
      osc2Sample = osc2Sample * (1.0f - ringMod_) + ringSample * ringMod_;
    }

    // Osc 3
    float osc3Freq =
        baseFreq *
        std::exp2((osc3Detune_ / 100.0f +
                   modulationState_.get(ModulationDestination::Osc3Pitch)) /
                  12.0f);
    float osc3Sample = osc3_.getNextSample(osc3Freq, sh3);

    // Mix
    float sample = 0.0f;
    sample +=
        osc1Sample * (osc1Mix_.getNextValue() +
                      modulationState_.get(ModulationDestination::Osc1Mix));
    sample +=
        osc2Sample * (osc2Mix_.getNextValue() +
                      modulationState_.get(ModulationDestination::Osc2Mix));
    sample +=
        osc3Sample * (osc3Mix_.getNextValue() +
                      modulationState_.get(ModulationDestination::Osc3Mix));

    // Sub/Noise
    if (subOscLevel_ > 0.0f) {
      subOsc_.setWaveform(osc1_.getWaveform());
      float subFreq = baseFreq * std::exp2(static_cast<float>(subOscOctave_));
      sample += subOsc_.getNextSample(subFreq, 0.5f) * subOscLevel_;
    }
    if (noiseLevel_ > 0.0f)
      sample += (noiseRandom_.nextFloat() * 2.0f - 1.0f) * noiseLevel_;

    // Unison
    if (unisonVoices_ > 1) {
      float unisonSpread = unisonDetune_ / 100.0f;
      float unisonGain = 1.0f / std::sqrt(static_cast<float>(unisonVoices_));
      for (int u = 0; u < unisonVoices_ - 1 && u < 7; ++u) {
        float detune =
            (u % 2 == 0 ? 1.0f : -1.0f) * ((u / 2 + 1) * unisonSpread);
        float uFreq = baseFreq * std::exp2(detune / 12.0f);
        unisonOscillators_[u].setWaveform(osc1_.getWaveform());
        sample += unisonOscillators_[u].getNextSample(uFreq, sh1) *
                  osc1Mix_.getCurrentValue() * unisonGain;
      }
      sample *= unisonGain;
    }

    // Filter
    float envDepthNormalized = (filterEnvAmount_ - 0.5f) * 2.0f;
    float envMod = env2 * envDepthNormalized * 5.0f;

    float keyTrackMod = 0.0f;
    if (filterKeyTrack_ != FilterKeyTrack::Off) {
      float semitonesFromC4 = static_cast<float>(midiNoteNumber_ - 60);
      float trackAmount =
          (filterKeyTrack_ == FilterKeyTrack::Full) ? 1.0f : 0.5f;
      keyTrackMod = semitonesFromC4 * trackAmount / 12.0f;
    }

    float cutoffMod = modulationState_.get(ModulationDestination::FilterCutoff);
    float modulatedCutoff =
        filterCutoff_ * std::exp2(cutoffMod + envMod + keyTrackMod);
    modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);
    filter1_.setCutoff(modulatedCutoff);

    float resMod = modulationState_.get(ModulationDestination::FilterResonance);
    filter1_.setResonance(
        juce::jlimit(0.0f, 1.0f, filter1_.getResonance() + resMod));

    sample = filter1_.processSample(sample);

    if (filter2Cutoff_ > 20.0f && filterSerial_) {
      filter2_.setCutoff(filter2Cutoff_);
      sample = filter2_.processSample(sample);
    }

    // Amp
    float modAmp = modulationState_.get(ModulationDestination::AmpGain);
    sample *= (env1 + modAmp) * velocity_ * masterGain_.getNextValue();

    // Add to buffer (replacing old addSample loop for cleaner code)
    leftOut[i] += sample;
    if (rightOut)
      rightOut[i] += sample;

    currentAmplitude_ = std::abs(sample);
    if (!ampEnvelope_.isActive()) {
      clearCurrentNote();
      break;
    }
  }
}

void ZenithPolySynthVoice::setSampleRate(double sampleRate) {
  baseSampleRate_ = sampleRate;
  updateSampleRate();
}

void ZenithPolySynthVoice::updateSampleRate() {
  double rate = baseSampleRate_ * oversamplingFactor_;
  this->setCurrentSampleRate(rate);

  osc1_.setSampleRate(rate);
  osc2_.setSampleRate(rate);
  osc3_.setSampleRate(rate);
  subOsc_.setSampleRate(rate);
  filter1_.setSampleRate(rate);
  filter2_.setSampleRate(rate);
  for (auto &osc : unisonOscillators_)
    osc.setSampleRate(rate);
  ampEnvelope_.setSampleRate(rate);
  modEnvelope_.setSampleRate(rate);

  osc1Shape_.reset(rate, 0.05);
  osc2Shape_.reset(rate, 0.05);
  osc3Shape_.reset(rate, 0.05);
  osc1Mix_.reset(rate, 0.05);
  osc2Mix_.reset(rate, 0.05);
  osc3Mix_.reset(rate, 0.05);
  masterGain_.reset(rate, 0.05);

  {
    // Resize buffers safely (Message thread or prepare step)
    // Assuming maxBlockSize_ is sufficient, otherwise we resize larger
    juce::ScopedLock sl(oversamplerLock_);
    int requiredUpSize = maxBlockSize_ * oversamplingFactor_;
    if (oversamplingBuffer_.getNumSamples() < requiredUpSize) {
      oversamplingBuffer_.setSize(2, requiredUpSize);
    }
    if (downsamplingBuffer_.getNumSamples() < maxBlockSize_) {
      downsamplingBuffer_.setSize(2, maxBlockSize_);
    }

    // Update oversampler if factor > 1
    if (oversampler_ && oversamplingFactor_ > 1) {
      oversampler_->initProcessing(requiredUpSize);
    }
  }
}

void ZenithPolySynthVoice::setQualityPreset(QualityPreset quality) {
  if (qualityPreset_ == quality)
    return;

  qualityPreset_ = quality;

  int newFactor = 1;
  if (quality == QualityPreset::Medium)
    newFactor = 2;
  else if (quality == QualityPreset::High)
    newFactor = 4; // Ultra could be 4x or 8x

  if (newFactor != oversamplingFactor_) {
    // PROTECT the switch
    juce::ScopedLock sl(oversamplerLock_);

    oversamplingFactor_ = newFactor;
    if (oversamplingFactor_ > 1) {
      oversampler_ = std::make_unique<juce::dsp::Oversampling<float>>(
          2,                                   // numChannels
          (int)std::log2(oversamplingFactor_), // factorLog2
          juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, // filter
          true                                        // isBuffered
      );
    } else {
      oversampler_ = nullptr;
    }

  }

  // Call updateSampleRate outside the if to ensure logic runs
  updateSampleRate();
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

void ZenithPolySynthVoice::setLFO1(float rate, float amount, LFOTarget target,
                                   LFOWaveform waveform) {
  lfo1Rate_ = rate;
  lfo1Amount_ = amount;
  lfo1Target_ = target;
  lfo1Waveform_ = waveform;
}

void ZenithPolySynthVoice::setLFO2(float rate, float amount, LFOTarget target,
                                   LFOWaveform waveform) {
  lfo2Rate_ = rate;
  lfo2Amount_ = amount;
  lfo2Target_ = target;
  lfo2Waveform_ = waveform;
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

void ZenithPolySynthVoice::updateFrequency() {}

void ZenithPolySynthVoice::computeModulation() {}

float ZenithPolySynthVoice::getModulationSourceValue(ModulationSource source) {
  switch (source) {
  case ModulationSource::LFO1:
    return lfo1Value_;
  case ModulationSource::LFO2:
    return lfo2Value_;
  case ModulationSource::Velocity:
    return velocity_;
  case ModulationSource::ModWheel:
    return modWheel_;
  case ModulationSource::Timbre:
    return timbre_;
  default:
    return 0.0f;
  }
}

} // namespace zenith
