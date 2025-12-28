/*

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
          true // isBuffered
      );
    } else {
      oversampler_ = nullptr;
    }

    // We must call updateSampleRate to propagate the new rate
    // Note: This calls updateSampleRate recursively but we released lock?
    // No, we hold lock. updateSampleRate also takes lock. Recursive lock is
    // needed? juce::CriticalSection IS recursive.
  }

  // Call updateSampleRate to propagate the new rate (Bug Fix)
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
