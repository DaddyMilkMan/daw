/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux
    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
    SPDX-License-Identifier: Apache-2.0 
*/
void ParametricEQ::updateBandCoefficients(Band& band) {
    float omega = 2.0f * juce::MathConstants<float>::pi * band.frequency / static_cast<float>(sampleRate_);
    float sn = std::sin(omega);
    float cs = std::cos(omega);
    float alpha = 0.0f;
    float A = std::pow(10.0f, band.gain / 40.0f); // For shelf/bell gain
    // Calculate alpha based on Q
    switch (band.type) {
        case FilterType::Bell:
        case FilterType::BandPass:
        case FilterType::Notch:
            alpha = sn / (2.0f * band.q);
            break;
        case FilterType::LowShelf:
        case FilterType::HighShelf:
            alpha = sn / 2.0f * std::sqrt((A + 1.0f/A) * (1.0f/band.q - 1.0f) + 2.0f);
            break;
        default:
            alpha = sn / 2.0f;
            break;
    }
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    switch (band.type) {
        case FilterType::Bell: {
            float beta = std::sqrt(A) * band.q;
            b0 = 1.0f + alpha * A;
            b1 = -2.0f * cs;
            b2 = 1.0f - alpha * A;
            a0 = 1.0f + alpha / A;
            a1 = -2.0f * cs;
            a2 = 1.0f - alpha / A;
            break;
        }
        case FilterType::LowShelf: {
            b0 = A * ((A + 1.0f) - (A - 1.0f) * cs + 2.0f * std::sqrt(A) * alpha);
            b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
            b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - 2.0f * std::sqrt(A) * alpha);
            a0 = (A + 1.0f) + (A - 1.0f) * cs + 2.0f * std::sqrt(A) * alpha;
            a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
            a2 = (A + 1.0f) + (A - 1.0f) * cs - 2.0f * std::sqrt(A) * alpha;
            break;
        }
        case FilterType::HighShelf: {
            b0 = A * ((A + 1.0f) + (A - 1.0f) * cs + 2.0f * std::sqrt(A) * alpha);
            b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
            b2 = A * ((A + 1.0f) + (A - 1.0f) * cs - 2.0f * std::sqrt(A) * alpha);
            a0 = (A + 1.0f) - (A - 1.0f) * cs + 2.0f * std::sqrt(A) * alpha;
            a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
            a2 = (A + 1.0f) - (A - 1.0f) * cs - 2.0f * std::sqrt(A) * alpha;
            break;
        }
        case FilterType::LowPass12:
            b0 = (1.0f - cs) / 2.0f;
            b1 = 1.0f - cs;
            b2 = (1.0f - cs) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cs;
            a2 = 1.0f - alpha;
            break;
        case FilterType::LowPass24:
            // Cascade two 2nd-order sections (simplified)
            b0 = (1.0f - cs) / 2.0f;
            b1 = 1.0f - cs;
            b2 = (1.0f - cs) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cs;
            a2 = 1.0f - alpha;
            break;
        case FilterType::HighPass12:
            b0 = (1.0f + cs) / 2.0f;
            b1 = -(1.0f + cs);
            b2 = (1.0f + cs) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cs;
            a2 = 1.0f - alpha;
            break;
        case FilterType::HighPass24:
            b0 = (1.0f + cs) / 2.0f;
            b1 = -(1.0f + cs);
            b2 = (1.0f + cs) / 2.0f;
    // Normalize by a0
    band.b0 = b0 / a0;
    band.b1 = b1 / a0;
    band.b2 = b2 / a0;
    band.a1 = a1 / a0;
    band.a2 = a2 / a0;
}
float ParametricEQ::processBand(float input, Band& band) {
    auto& state = leftState_[std::distance(bands_.data(), &band) % maxBands];
    float output = band.b0 * input + band.b1 * state.x1 + band.b2 * state.x2
                   - band.a1 * state.y1 - band.a2 * state.y2;
    state.x2 = state.x1;
    state.x1 = input;
    state.y2 = state.y1;
    state.y1 = output;
    return output;
}
float ParametricEQ::processBandStereo(float input, Band& band, size_t bandIndex) {
    auto& state = rightState_[bandIndex % maxBands];
    float output = band.b0 * input + band.b1 * state.x1 + band.b2 * state.x2
                   - band.a1 * state.y1 - band.a2 * state.y2;
    state.x2 = state.x1;
    state.x1 = input;
    state.y2 = state.y1;
    state.y1 = output;
    return output;
}
//==============================================================================
// LIMITER IMPLEMENTATION
//==============================================================================
Limiter::Limiter() {
    delayBuffer_.resize(256, 0.0f);
}
void Limiter::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    // Calculate look-ahead buffer size
    int lookAheadSamples = static_cast<int>(lookaheadMs_ * sampleRate / 1000.0);
    delaySize_ = lookAheadSamples + samplesPerBlock + 100;
    delayBuffer_.resize(delaySize_, 0.0f);
    updateCoefficients();
}
void Limiter::reset() {
    std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);
    delayIndex_ = 0;
    envelope_ = 0.0f;
    gainReductionDb_ = 0.0f;
    inputLevelDb_ = -100.0f;
    outputLevelDb_ = -100.0f;
}

//==============================================================================
// RING MODULATOR IMPLEMENTATION
//==============================================================================

RingModulator::RingModulator() {
    reset();
}

void RingModulator::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    reset();
}

void RingModulator::reset() {
    carrierPhaseL_ = 0.0f;
    carrierPhaseR_ = 0.0f;
    filterZ1L_ = filterZ2L_ = 0.0f;
    filterZ1R_ = filterZ2R_ = 0.0f;
}

void RingModulator::process(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* left = buffer.getWritePointer(0);
    float* right = numChannels > 1 ? buffer.getWritePointer(1) : left;

    for (int i = 0; i < numSamples; ++i) {
        float inputL = left[i];
        float inputR = right[i];

        // Calculate carrier frequencies with stereo detune
        float detune = stereoDetune_ * 0.01f;  // Convert cents to ratio
        float carrierFreqL = carrierFreq_ * ratio_ * (1.0f + detune);
        float carrierFreqR = carrierFreq_ * ratio_ * (1.0f - detune);

        // Generate carrier signals
        float carrierL = generateCarrier(carrierPhaseL_, carrierWave_);
        float carrierR = generateCarrier(carrierPhaseR_, carrierWave_);

        // Update carrier phases
        carrierPhaseL_ += carrierFreqL / static_cast<float>(sampleRate_);
        carrierPhaseR_ += carrierFreqR / static_cast<float>(sampleRate_);
        if (carrierPhaseL_ > 1.0f) carrierPhaseL_ -= 1.0f;
        if (carrierPhaseR_ > 1.0f) carrierPhaseR_ -= 1.0f;

        // Apply waveshaping to carrier
        if (waveshapeAmount_ > 0.0f) {
            carrierL = std::tanh(carrierL * (1.0f + waveshapeAmount_ * 3.0f));
            carrierR = std::tanh(carrierR * (1.0f + waveshapeAmount_ * 3.0f));
        }

        // Ring modulate
        float modulatedL = inputL * carrierL;
        float modulatedR = inputR * carrierR;

        // Apply tone filter to tame harshness
        float filteredL = applyFilter(modulatedL, filterZ1L_, filterZ2L_);
        float filteredR = applyFilter(modulatedR, filterZ1R_, filterZ2R_);

        // Mix with original
        left[i] = inputL * (1.0f - mix_) + filteredL * mix_;
        right[i] = inputR * (1.0f - mix_) + filteredR * mix_;
    }
}

float RingModulator::generateCarrier(float phase, CarrierWaveform wave) const {
    switch (wave) {
        case CarrierWaveform::Sine:
            return std::sin(phase * juce::MathConstants<float>::twoPi);

        case CarrierWaveform::Triangle:
            return 2.0f * std::abs(2.0f * (phase - std::floor(phase + 0.5f))) - 1.0f;

        case CarrierWaveform::Sawtooth:
            return 2.0f * (phase - std::floor(phase + 0.5f));

        case CarrierWaveform::Square:
            return (std::sin(phase * juce::MathConstants<float>::twoPi) >= 0.0f) ? 1.0f : -1.0f;

        case CarrierWaveform::Complex: {
            float out = 0.0f;
            out += std::sin(phase * juce::MathConstants<float>::twoPi);
            out += 0.5f * std::sin(phase * juce::MathConstants<float>::twoPi * 2.0f);
            out += 0.3f * std::sin(phase * juce::MathConstants<float>::twoPi * 3.0f);
            out += 0.2f * std::sin(phase * juce::MathConstants<float>::twoPi * 4.0f);
            return out * 0.5f;
        }

        case CarrierWaveform::VocalFormant: {
            float out = std::sin(phase * juce::MathConstants<float>::twoPi);
            out += 0.5f * std::sin(phase * juce::MathConstants<float>::twoPi * 6.0f) * std::exp(-20.0f * std::abs(phase - 0.5f));
            out += 0.3f * std::sin(phase * juce::MathConstants<float>::twoPi * 12.0f) * std::exp(-30.0f * std::abs(phase - 0.7f));
            return out * 0.6f;
        }

        default:
            return std::sin(phase * juce::MathConstants<float>::twoPi);
    }
}

float RingModulator::applyFilter(float input, float& z1, float& z2) const {
    // SVF-based tone control
    float omega = 2.0f * juce::MathConstants<float>::pi * filterFreq_ / static_cast<float>(sampleRate_);
    float g = std::tan(omega / 4.0f);
    float k = 1.0f - (filterRes_ * 0.25f);

    float hp = input - k * z1 - z2;
    float bp = g * hp + z1;
    float lp = g * bp + z2;

    z1 = bp;
    z2 = lp;

    return lp * 0.7f + bp * 0.3f;
}

//==============================================================================
// FREQUENCY SHIFTER IMPLEMENTATION
//==============================================================================

FrequencyShifter::FrequencyShifter() {
    reset();
}

void FrequencyShifter::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    reset();
}

void FrequencyShifter::reset() {
    oscPhaseL_ = 0.0f;
    oscPhaseR_ = 0.0f;
}

void FrequencyShifter::process(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* left = buffer.getWritePointer(0);
    float* right = numChannels > 1 ? buffer.getWritePointer(1) : left;

    // Calculate oscillator phase increment for frequency shift
    float phaseIncL = shiftAmount_ / static_cast<float>(sampleRate_);
    float phaseIncR = (shiftAmount_ + stereoSpread_) / static_cast<float>(sampleRate_);

    for (int i = 0; i < numSamples; ++i) {
        float inputL = left[i];
        float inputR = right[i];

        // Generate quadrature oscillators
        float oscL0 = std::cos(oscPhaseL_ * juce::MathConstants<float>::twoPi);
        float oscL90 = std::sin(oscPhaseL_ * juce::MathConstants<float>::twoPi);

        float oscR0 = std::cos(oscPhaseR_ * juce::MathConstants<float>::twoPi);
        float oscR90 = std::sin(oscPhaseR_ * juce::MathConstants<float>::twoPi);

        oscPhaseL_ += phaseIncL;
        oscPhaseR_ += phaseIncR;
        if (oscPhaseL_ > 1.0f) oscPhaseL_ -= 1.0f;
        if (oscPhaseR_ > 1.0f) oscPhaseR_ -= 1.0f;

        // Simplified frequency shifting using SSB modulation
        float shiftedL = inputL * oscL0 - inputL * 0.5f * oscL90;  // Approximate 90-degree shift
        float shiftedR = inputR * oscR0 - inputR * 0.5f * oscR90;

        // Apply mode
        float outputL = 0.0f;
        float outputR = 0.0f;

        switch (mode_) {
            case Mode::Up:
                outputL = inputL * 0.0f + shiftedL * 1.0f;  // Simplified
                outputR = inputR * 0.0f + shiftedR * 1.0f;
                break;

            case Mode::Down:
                outputL = inputL * oscL0 + inputL * 0.5f * oscL90;
                outputR = inputR * oscR0 + inputR * 0.5f * oscR90;
                break;

            case Mode::Both:
                outputL = shiftedL;
                outputR = inputR * oscR0 + inputR * 0.5f * oscR90;
                break;

            case Mode::Ring:
                outputL = inputL * oscL0;
                outputR = inputR * oscR0;
                break;

            case Mode::ThroughZero:
                if (shiftAmount_ >= 0) {
                    outputL = shiftedL;
                    outputR = shiftedR;
                } else {
                    outputL = inputL * oscL0 + inputL * 0.5f * oscL90;
                    outputR = inputR * oscR0 + inputR * 0.5f * oscR90;
                }
                break;
        }

        // Mix
        left[i] = inputL * (1.0f - mix_) + outputL * mix_;
        right[i] = inputR * (1.0f - mix_) + outputR * mix_;
    }
}

//==============================================================================
// GRANULAR FX IMPLEMENTATION
//==============================================================================

GranularFX::GranularFX() {
    inputBufferSize_ = 96000;
    inputBuffer_.resize(inputBufferSize_, 0.0f);
    inputWriteIndex_ = 0;

    delayBufferL_.resize(maxDelaySize, 0.0f);
    delayBufferR_.resize(maxDelaySize, 0.0f);

    reset();
}

void GranularFX::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    reset();
}

void GranularFX::reset() {
    for (auto& grain : grains_) {
        grain.reset();
    }
    inputBuffer_.fill(0.0f);
    inputWriteIndex_ = 0;

    delayBufferL_.fill(0.0f);
    delayBufferR_.fill(0.0f);
    delaySize_ = 0;
    delayIndex_ = 0;

    grainsSinceLastTrigger_ = 0.0f;
}

void GranularFX::process(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* left = buffer.getWritePointer(0);
    float* right = numChannels > 1 ? buffer.getWritePointer(1) : left;

    // Write input to circular buffer
    for (int i = 0; i < numSamples; ++i) {
        inputBuffer_[inputWriteIndex_] = left[i];
        inputWriteIndex_ = (inputWriteIndex_ + 1) % inputBufferSize_;
    }

    float grainsPerSample = density_ / static_cast<float>(sampleRate_);

    for (int i = 0; i < numSamples; ++i) {
        float sampleL = left[i];
        float sampleR = right[i];

        // Process grains
        float grainOutputL = 0.0f;
        float grainOutputR = 0.0f;
        int activeGrains = 0;

        for (auto& grain : grains_) {
            if (grain.active) {
                float gOut = processGrain(grain);
                grainOutputL += gOut * grain.amplitude * (1.0f - grain.pan);
                grainOutputR += gOut * grain.amplitude * grain.pan;
                activeGrains++;

                if (grain.position >= grain.size) {
                    grain.reset();
                } else {
                    grain.position++;
                }
            }
        }

        // Trigger new grains
        grainsSinceLastTrigger_ += grainsPerSample;

        while (grainsSinceLastTrigger_ >= 1.0f) {
            grainsSinceLastTrigger_ -= 1.0f;
            triggerNewGrain(left + i, numSamples);
        }

        // Mode-specific processing
        switch (mode_) {
            case Mode::Texture:
                sampleL = sampleL * (1.0f - mix_) + grainOutputL * mix_ * texture_;
                sampleR = sampleR * (1.0f - mix_) + grainOutputR * mix_ * texture_;
                break;

            case Mode::PitchDelay:
                processPitchDelay(sampleL, grainOutputL, sampleR);
                break;

            case Mode::Shimmer:
                processShimmer(sampleL, grainOutputL);
                sampleR = sampleL;
                break;

            case Mode::Freeze:
                if (freeze_) {
                    sampleL = grainOutputL * 0.5f;
                    sampleR = grainOutputR * 0.5f;
                }
                break;

            case Mode::Stretch:
                sampleL = sampleL * 0.7f + grainOutputL * 0.3f;
                sampleR = sampleR * 0.7f + grainOutputR * 0.3f;
                break;

            case Mode::Scrub:
                sampleL = grainOutputL * 0.5f;
                sampleR = grainOutputR * 0.5f;
                break;

            case Mode::Reverse:
                sampleL = sampleL * (1.0f - mix_) + grainOutputL * mix_;
                sampleR = sampleR * (1.0f - mix_) + grainOutputR * mix_;
                break;
        }

        // Apply feedback
        if (feedback_ > 0.0f) {
            sampleL += sampleR * feedback_;
            sampleR += sampleL * feedback_ * 0.5f;
        }

        left[i] = sampleL;
        right[i] = sampleR;
    }
}

void GranularFX::triggerGrain() {
    triggerNewGrain(nullptr, 0);
}

void GranularFX::triggerNewGrain(const float* source, int sourceSize) {
    for (auto& grain : grains_) {
        if (!grain.active) {
            float grainSizeSamples = grainSize_ * sampleRate_ / 1000.0f;
            grainSizeSamples = juce::jlimit(1.0f, 4096.0f, grainSizeSamples);
            grain.size = static_cast<int>(grainSizeSamples);

            int startPos = (inputWriteIndex_ - static_cast<int>(grainSizeSamples * 0.5) + inputBufferSize_) % inputBufferSize_;

            float pitch = std::pow(2.0f, position_ / 12.0f);

            for (int i = 0; i < grain.size && i < 4096; ++i) {
                int srcPos = (startPos + static_cast<int>(i * pitch)) % inputBufferSize_;
                grain.buffer[i] = inputBuffer_[srcPos] * windowSample(static_cast<float>(i) / grain.size, windowType_);
            }

            grain.position = 0;
            grain.pitchRatio = pitch;
            grain.amplitude = 1.0f / std::sqrt(static_cast<float>(density_) / 10.0f + 1.0f);
            grain.pan = 0.5f;
            grain.active = true;
            break;
        }
    }
}

float GranularFX::processGrain(Grain& grain) {
    float pos = grain.position * grain.pitchRatio;
    int idx0 = static_cast<int>(pos) % 4096;
    int idx1 = (idx0 + 1) % 4096;
    float frac = pos - std::floor(pos);

    return grain.buffer[idx0] + frac * (grain.buffer[idx1] - grain.buffer[idx0]);
}

float GranularFX::windowSample(float phase, int type) const {
    switch (type) {
        case 0: return 1.0f;
        case 1: return phase * (2.0f - phase) * 4.0f;
        case 2: return 0.5f * (1.0f - std::cos(phase * juce::MathConstants<float>::twoPi));
        case 3: return 0.54f - 0.46f * std::cos(phase * juce::MathConstants<float>::twoPi);
        default: return 0.5f * (1.0f - std::cos(phase * juce::MathConstants<float>::twoPi));
    }
}

void GranularFX::processShimmer(float& sample, float grainSample) {
    delaySize_ = maxDelaySize;
    delayBufferL_[delayIndex_] = sample + grainSample;
    delayIndex_ = (delayIndex_ + 1) % delaySize_;

    int readPos = (delayIndex_ - static_cast<int>(sampleRate_ * 0.5)) % delaySize_;
    float delayed = delayBufferL_[readPos];

    int readPos2 = (readPos - static_cast<int>(sampleRate_ * 0.25)) % delaySize_;
    float pitched = delayBufferL_[readPos2];

    sample = sample * 0.5f + (delayed + pitched) * 0.25f;
}

void GranularFX::processPitchDelay(float& sample, float grainSample, float& sampleR) {
    delaySize_ = maxDelaySize;
    delayBufferL_[delayIndex_] = sample;
    delayBufferR_[delayIndex_] = sampleR;
    delayIndex_ = (delayIndex_ + 1) % delaySize_;

    int pitchSamples = static_cast<int>(sampleRate_ * std::pow(2.0f, position_ / 12.0));
    int readPos = (delayIndex_ - pitchSamples + delaySize_) % delaySize_;

    sample = delayBufferL_[readPos] * mix_ + sample * (1.0f - mix_);
    sampleR = delayBufferR_[readPos] * mix_ + sampleR * (1.0f - mix_);
}

//==============================================================================
// ADVANCED VOCODER IMPLEMENTATION
//==============================================================================

AdvancedVocoder::AdvancedVocoder() {
    tempBuffer_.setSize(2, 4096);
}

void AdvancedVocoder::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    updateBandFrequencies();

    for (auto& band : bands_) {
        band.reset();
    }

    highpassState_ = 0.0f;
    sibilanceEnvelope_ = 0.0f;
}

void AdvancedVocoder::reset() {
    for (auto& band : bands_) {
        band.reset();
    }
    highpassState_ = 0.0f;
    sibilanceEnvelope_ = 0.0f;
    carrierPhase_ = 0.0f;
}

void AdvancedVocoder::process(juce::AudioBuffer<float>& modulator, const juce::AudioBuffer<float>& carrier) {
    const int numSamples = modulator.getNumSamples();

    const float* modL = modulator.getReadPointer(0);
    processSibilance(modL, numSamples);

    for (int bandIdx = 0; bandIdx < numBands_; ++bandIdx) {
        auto& band = bands_[bandIdx];

        for (int i = 0; i < numSamples; ++i) {
            float modSample = modulator.getSample(0, i);
            float bandEnergy = band.analysisFilter.process(modSample);
            band.envelope = band.envelope * 0.995f + bandEnergy * 0.005f;

            if (sibilance_ > 0.0f && bandIdx >= numBands_ / 2) {
                band.envelope *= (1.0f + sibilance_ * sibilanceEnvelope_);
            }

            float carrierSample = 0.0f;
            if (useSynthCarrier_) {
                carrierSample = generateSynthCarrier(carrierPhase_);
                carrierPhase_ += 440.0f * std::pow(2.0f, (carrierPitch_ - 69.0f) / 12.0) / sampleRate_;
            } else if (carrier && carrier->getNumChannels() > 0) {
                carrierSample = carrier->getSample(0, i);
            }

            float modulated = carrierSample * band.envelope * depth_;
            float output = band.synthesisFilter.process(modulated);
            applyCharacter(output, bandIdx);
        }
    }
}

float AdvancedVocoder::generateSynthCarrier(float phase) {
    float out = std::sin(phase * juce::MathConstants<float>::twoPi);
    out += 0.5f * std::sin(phase * juce::MathConstants<float>::twoPi * 2.0f);
    out += 0.3f * std::sin(phase * juce::MathConstants<float>::twoPi * 4.0f);
    return out * 0.5f;
}

void AdvancedVocoder::updateBandFrequencies() {
    float startFreq = 100.0f;
    float endFreq = 8000.0f;

    for (int i = 0; i < maxBands; ++i) {
        if (i < numBands_) {
            float freq = startFreq * std::pow(endFreq / startFreq, static_cast<float>(i) / numBands_);
            float q = 4.0f;

            bands_[i].analysisFilter.setParameters(freq, q, sampleRate_);
            bands_[i].synthesisFilter.setParameters(freq, q, sampleRate_);
        }
    }
}

void AdvancedVocoder::processSibilance(const float* input, int numSamples) {
    float hpCoeff = 0.99f;

    for (int i = 0; i < numSamples; ++i) {
        float hp = input[i] - highpassState_;
        highpassState_ = input[i] + hpCoeff * hp;

        float env = std::abs(hp);
        sibilanceEnvelope_ = sibilanceEnvelope_ * 0.999f + env * 0.001f;
    }
}

void AdvancedVocoder::applyCharacter(float& sample, int bandIndex) {
    if (character_ > 0.5f) {
        if (bandIndex >= numBands_ * 0.3f && bandIndex <= numBands_ * 0.6f) {
            sample *= 1.0f + (character_ - 0.5f) * 0.5f;
        }
    }

    if (bandIndex >= 2 && bandIndex <= 4) {
        sample *= 1.0f + character_ * 0.3f;
    }
}

void AdvancedVocoder::BandpassFilter::setParameters(float freq, float q, double sampleRate) {
    float omega = 2.0f * juce::MathConstants<double>::pi * freq / sampleRate;
    this->q = q;

    float alpha = std::sin(omega) / (2.0f * q);
    float g = 1.0f / (1.0f + alpha);

    f1 = g;
    f2 = 0.0f;
}

float AdvancedVocoder::BandpassFilter::process(float input) {
    float bp = f1 * (input - y1);
    y1 = y1 + bp;

    return y1;
}

} // namespace zenith
'
