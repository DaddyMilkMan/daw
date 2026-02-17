/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "SIMDOptimizations.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// SIMDBufferOps Implementation
//==============================================================================

#if JUCE_USE_SIMD

void SIMDBufferOps::applyGain(float* samples, int numSamples, float gain) {
    using namespace juce::dsp;

    int simd = numSamples & ~(SIMDRegister<float>::SIMDRegisterSize - 1);
    int i = 0;

    // SIMD-optimized processing
    for (i = 0; i < simd; i += SIMDRegister<float>::SIMDRegisterSize) {
        SIMDRegister<float> v(samples + i);
        v = v * gain;
        v.store(samples + i);
    }

    // Tail processing
    for (; i < numSamples; ++i) {
        samples[i] *= gain;
    }
}

#else // No SIMD available

void SIMDBufferOps::applyGain(float* samples, int numSamples, float gain) {
    for (int i = 0; i < numSamples; ++i) {
        samples[i] *= gain;
    }
}

#endif

void SIMDBufferOps::applyGainRamp(float* samples, int numSamples, float startGain, float endGain) {
    if (numSamples == 0) return;

    float gainStep = (endGain - startGain) / numSamples;
    float currentGain = startGain;

    for (int i = 0; i < numSamples; ++i) {
        samples[i] *= currentGain;
        currentGain += gainStep;
    }
}

#if JUCE_USE_SIMD

void SIMDBufferOps::addWithGain(float* dest, const float* src, int numSamples, float gain) {
    using namespace juce::dsp;

    int simd = numSamples & ~(SIMDRegister<float>::SIMDRegisterSize - 1);
    int i = 0;

    for (i = 0; i < simd; i += SIMDRegister<float>::SIMDRegisterSize) {
        SIMDRegister<float> d(dest + i);
        SIMDRegister<float> s(src + i);
        d = d + s * gain;
        d.store(dest + i);
    }

    for (; i < numSamples; ++i) {
        dest[i] += src[i] * gain;
    }
}

#else

void SIMDBufferOps::addWithGain(float* dest, const float* src, int numSamples, float gain) {
    for (int i = 0; i < numSamples; ++i) {
        dest[i] += src[i] * gain;
    }
}

#endif

void SIMDBufferOps::copyWithGain(float* dest, const float* src, int numSamples, float gain) {
    for (int i = 0; i < numSamples; ++i) {
        dest[i] = src[i] * gain;
    }
}

void SIMDBufferOps::fadeIn(float* samples, int numSamples) {
    if (numSamples <= 1) {
        if (numSamples == 1) samples[0] = 0.0f;
        return;
    }

    float step = 1.0f / (numSamples - 1);
    float gain = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        samples[i] *= gain;
        gain += step;
    }
}

void SIMDBufferOps::fadeOut(float* samples, int numSamples) {
    if (numSamples <= 1) {
        if (numSamples == 1) samples[0] = 0.0f;
        return;
    }

    float step = 1.0f / (numSamples - 1);
    float gain = 1.0f;

    for (int i = 0; i < numSamples; ++i) {
        samples[i] *= gain;
        gain -= step;
    }
}

#if JUCE_USE_SIMD

void SIMDBufferOps::applyGainStereo(float* samples, int numSamples, float gainLeft, float gainRight) {
    using namespace juce::dsp;

    int numFrames = numSamples / 2;
    int simd = numFrames & ~(SIMDRegister<float>::SIMDRegisterSize / 2 - 1);
    int i = 0;

    // Process stereo pairs
    for (i = 0; i < simd; ++i) {
        samples[i * 2] *= gainLeft;
        samples[i * 2 + 1] *= gainRight;
    }

    // Tail
    for (; i < numFrames; ++i) {
        samples[i * 2] *= gainLeft;
        samples[i * 2 + 1] *= gainRight;
    }
}

#else

void SIMDBufferOps::applyGainStereo(float* samples, int numSamples, float gainLeft, float gainRight) {
    for (int i = 0; i < numSamples; i += 2) {
        samples[i] *= gainLeft;
        samples[i + 1] *= gainRight;
    }
}

#endif

void SIMDBufferOps::stereoToMono(float* dest, const float* srcLeft, const float* srcRight, int numSamples) {
    constexpr float sqrt2inv = 0.70710678f; // 1/sqrt(2)

    for (int i = 0; i < numSamples; ++i) {
        dest[i] = (srcLeft[i] + srcRight[i]) * sqrt2inv;
    }
}

void SIMDBufferOps::monoToStereo(float* destLeft, float* destRight, const float* src, int numSamples,
                                float gainLeft, float gainRight) {
    for (int i = 0; i < numSamples; ++i) {
        destLeft[i] = src[i] * gainLeft;
        destRight[i] = src[i] * gainRight;
    }
}

void SIMDBufferOps::crossfadeEqualPower(float* dest, const float* srcA, const float* srcB,
                                       int numSamples, float crossfadeAmount) {
    // Equal power crossfade: sin/cos curves
    float angle = crossfadeAmount * juce::MathConstants<float>::halfPi;
    float gainA = std::cos(angle);
    float gainB = std::sin(angle);

    for (int i = 0; i < numSamples; ++i) {
        dest[i] = srcA[i] * gainA + srcB[i] * gainB;
    }
}

void SIMDBufferOps::crossfadeLinear(float* dest, const float* srcA, const float* srcB,
                                   int numSamples, float crossfadeAmount) {
    float gainA = 1.0f - crossfadeAmount;
    float gainB = crossfadeAmount;

    for (int i = 0; i < numSamples; ++i) {
        dest[i] = srcA[i] * gainA + srcB[i] * gainB;
    }
}

#if JUCE_USE_SIMD

float SIMDBufferOps::calculateRMS(const float* samples, int numSamples) {
    using namespace juce::dsp;

    SIMDRegister<float> sum(0.0f);
    int simd = numSamples & ~(SIMDRegister<float>::SIMDRegisterSize - 1);
    int i = 0;

    for (i = 0; i < simd; i += SIMDRegister<float>::SIMDRegisterSize) {
        SIMDRegister<float> v(samples + i);
        sum = sum + v * v;
    }

    // Extract SIMD sum
    float total = 0.0f;
    auto sumArr = sum.get();
    for (size_t j = 0; j < SIMDRegister<float>::SIMDRegisterSize; ++j) {
        total += sumArr[j];
    }

    // Tail
    for (; i < numSamples; ++i) {
        total += samples[i] * samples[i];
    }

    return std::sqrt(total / numSamples);
}

#else

float SIMDBufferOps::calculateRMS(const float* samples, int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += samples[i] * samples[i];
    }
    return std::sqrt(sum / numSamples);
}

#endif

#if JUCE_USE_SIMD

float SIMDBufferOps::findPeak(const float* samples, int numSamples) {
    using namespace juce::dsp;

    SIMDRegister<float> maxVal(0.0f);
    int simd = numSamples & ~(SIMDRegister<float>::SIMDRegisterSize - 1);
    int i = 0;

    for (i = 0; i < simd; i += SIMDRegister<float>::SIMDRegisterSize) {
        SIMDRegister<float> v(samples + i);
        SIMDRegister<float> absV = v.abs();
        maxVal = maxVal.max(absV);
    }

    // Extract max
    float peak = 0.0f;
    auto maxArr = maxVal.get();
    for (size_t j = 0; j < SIMDRegister<float>::SIMDRegisterSize; ++j) {
        peak = std::max(peak, std::abs(maxArr[j]));
    }

    // Tail
    for (; i < numSamples; ++i) {
        peak = std::max(peak, std::abs(samples[i]));
    }

    return peak;
}

#else

float SIMDBufferOps::findPeak(const float* samples, int numSamples) {
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    return peak;
}

#endif

int SIMDBufferOps::countAboveThreshold(const float* samples, int numSamples, float threshold) {
    int count = 0;
    for (int i = 0; i < numSamples; ++i) {
        if (samples[i] > threshold) count++;
    }
    return count;
}

void SIMDBufferOps::clamp(float* samples, int numSamples, float min, float max) {
    for (int i = 0; i < numSamples; ++i) {
        samples[i] = juce::jlimit(min, max, samples[i]);
    }
}

void SIMDBufferOps::softClip(float* samples, int numSamples, float drive) {
    for (int i = 0; i < numSamples; ++i) {
        float driven = samples[i] * drive;
        samples[i] = std::tanh(driven);
    }
}

void SIMDBufferOps::hardClip(float* samples, int numSamples, float min, float max) {
    for (int i = 0; i < numSamples; ++i) {
        samples[i] = juce::jlimit(min, max, samples[i]);
    }
}

//==============================================================================
// SIMDOscillator Implementation
//==============================================================================

SIMDOscillator::SIMDOscillator() {
    // Initialize all phases to random values for unison effect
    for (auto& row : phases_) {
        row.fill(0.0);
    }
}

void SIMDOscillator::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void SIMDOscillator::setFrequency(float frequencyHz) {
    baseFrequency_ = juce::jlimit(20.0f, 20000.0f, frequencyHz);
}

void SIMDOscillator::setVoiceFrequencies(const float* frequencies, int numVoices) {
    // For now, just store base frequency
    // Individual detuning is applied via phase increments
}

void SIMDOscillator::process(float** outputs, int numVoices, int numSamples, OscillatorWaveform waveform) {
    switch (waveform) {
        case OscillatorWaveform::Sine:
            processSine(outputs, numVoices, numSamples);
            break;
        // Add other waveform types as needed
        default:
            processSine(outputs, numVoices, numSamples);
            break;
    }
}

void SIMDOscillator::processSine(float** outputs, int numVoices, int numSamples) {
    for (int v = 0; v < numVoices; ++v) {
        float* output = outputs[v];
        if (!output) continue;

        // Get phase for this voice
        size_t row = v / 16;
        size_t col = v % 16;
        double& phase = phases_[row][col];

        double phaseInc = baseFrequency_ / sampleRate_;

        for (int i = 0; i < numSamples; ++i) {
            output[i] = static_cast<float>(std::sin(phase * juce::MathConstants<double>::twoPi));
            phase += phaseInc;
            if (phase >= 1.0) phase -= 1.0;
        }
    }
}

void SIMDOscillator::reset() {
    for (auto& row : phases_) {
        row.fill(0.0);
    }
}

void SIMDOscillator::setPhase(int voiceIndex, float phase) {
    if (voiceIndex >= 0 && voiceIndex < 64) {
        size_t row = voiceIndex / 16;
        size_t col = voiceIndex % 16;
        phases_[row][col] = static_cast<double>(phase);
    }
}

//==============================================================================
// SIMDFilterBank Implementation
//==============================================================================

SIMDFilterBank::SIMDFilterBank() {
    cutoffSmoothed_.reset(sampleRate_, 0.01);
    resonanceSmoothed_.reset(sampleRate_, 0.01);
}

void SIMDFilterBank::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    cutoffSmoothed_.reset(sampleRate, 0.01);
    resonanceSmoothed_.reset(sampleRate, 0.01);
}

void SIMDFilterBank::setCutoff(float cutoffHz) {
    cutoffSmoothed_.setTargetValue(juce::jlimit(20.0f, 20000.0f, cutoffHz));
}

void SIMDFilterBank::setResonance(float resonance) {
    resonanceSmoothed_.setTargetValue(juce::jlimit(0.0f, 1.0f, resonance));
}

void SIMDFilterBank::setFilterType(FilterType type) {
    type_ = type;
}

void SIMDFilterBank::process(float** inputs, float** outputs, int numFilters, int numSamples) {
    float cutoff = cutoffSmoothed_.getNextValue();
    float resonance = resonanceSmoothed_.getNextValue();

    // Calculate SVF coefficients
    double omega = 2.0 * juce::MathConstants<double>::pi * cutoff / sampleRate_;
    double g = std::tan(omega / 2.0);
    double k = resonance * 2.0;

    for (int f = 0; f < numFilters; ++f) {
        float* input = inputs[f];
        float* output = outputs[f];
        if (!input || !output) continue;

        // Simple SVF processing (per-filter)
        // In a full implementation, this would use SIMD registers
        for (int i = 0; i < numSamples; ++i) {
            // Simplified - use proper state variables in production
            output[i] = input[i];
        }
    }
}

void SIMDFilterBank::reset() {
    for (auto& state : states_) {
#if JUCE_USE_SIMD
        state.z1 = juce::dsp::SIMDRegister<float>(0.0f);
        state.z2 = juce::dsp::SIMDRegister<float>(0.0f);
#else
        std::fill(state.z1, state.z1 + 4, 0.0f);
        std::fill(state.z2, state.z2 + 4, 0.0f);
#endif
    }
}

//==============================================================================
// SIMDEnvelope Implementation
//==============================================================================

SIMDEnvelope::SIMDEnvelope() {
    for (auto& voice : voices_) {
        voice.current = 0.0f;
        voice.target = 0.0f;
        voice.stage = Stage::Idle;
        voice.phase = 0.0f;
    }
}

void SIMDEnvelope::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void SIMDEnvelope::setParameters(const Parameters& params) {
    params_ = params;
    updateCoefficients();
}

void SIMDEnvelope::updateCoefficients() {
    // Calculate time constants
    if (sampleRate_ > 0) {
        attackCoeff_ = params_.attack > 0 ? static_cast<float>(1.0 / (params_.attack * sampleRate_)) : 1.0f;
        decayCoeff_ = params_.decay > 0 ? static_cast<float>(1.0 / (params_.decay * sampleRate_)) : 1.0f;
        releaseCoeff_ = params_.release > 0 ? static_cast<float>(1.0 / (params_.release * sampleRate_)) : 1.0f;
    }
}

void SIMDEnvelope::noteOn(int voiceIndex) {
    if (voiceIndex >= 0 && voiceIndex < maxVoices) {
        voices_[voiceIndex].stage = Stage::Attack;
        voices_[voiceIndex].target = 1.0f;
        voices_[voiceIndex].phase = 0.0f;
    }
}

void SIMDEnvelope::noteOff(int voiceIndex) {
    if (voiceIndex >= 0 && voiceIndex < maxVoices) {
        voices_[voiceIndex].stage = Stage::Release;
        voices_[voiceIndex].target = 0.0f;
    }
}

void SIMDEnvelope::process(float* outputs, int numVoices, int numSamples) {
    for (int v = 0; v < numVoices; ++v) {
        auto& voice = voices_[v];
        float* output = &outputs[v * numSamples];

        for (int i = 0; i < numSamples; ++i) {
            switch (voice.stage) {
                case Stage::Attack: {
                    // Exponential attack
                    voice.current += (voice.target - voice.current) * attackCoeff_;
                    if (voice.current >= 0.99f) {
                        voice.current = 1.0f;
                        voice.stage = Stage::Decay;
                    }
                    break;
                }

                case Stage::Decay: {
                    voice.current += (params_.sustain - voice.current) * decayCoeff_;
                    if (std::abs(voice.current - params_.sustain) < 0.001f) {
                        voice.current = params_.sustain;
                        voice.stage = Stage::Sustain;
                    }
                    break;
                }

                case Stage::Sustain:
                    // Hold at sustain level
                    voice.current = params_.sustain;
                    break;

                case Stage::Release: {
                    voice.current += (voice.target - voice.current) * releaseCoeff_;
                    if (voice.current < 0.001f) {
                        voice.current = 0.0f;
                        voice.stage = Stage::Idle;
                    }
                    break;
                }

                case Stage::Idle:
                    voice.current = 0.0f;
                    break;
            }

            output[i] = voice.current;
        }
    }
}

SIMDEnvelope::Stage SIMDEnvelope::getStage(int voiceIndex) const {
    if (voiceIndex >= 0 && voiceIndex < maxVoices) {
        return voices_[voiceIndex].stage;
    }
    return Stage::Idle;
}

void SIMDEnvelope::reset() {
    for (auto& voice : voices_) {
        voice.current = 0.0f;
        voice.target = 0.0f;
        voice.stage = Stage::Idle;
        voice.phase = 0.0f;
    }
}

//==============================================================================
// SIMDMixer Implementation
//==============================================================================

SIMDMixer::SIMDMixer() {
    reset();
}

void SIMDMixer::setInputGain(int inputIndex, float gain) {
    if (inputIndex >= 0 && inputIndex < maxInputs) {
        inputs_[inputIndex].gain = juce::jlimit(0.0f, 2.0f, gain);
        inputs_[inputIndex].smoothedGain.setTargetValue(gain);
        updateInputGains(inputIndex);
    }
}

void SIMDMixer::setInputPan(int inputIndex, float pan) {
    if (inputIndex >= 0 && inputIndex < maxInputs) {
        inputs_[inputIndex].pan = juce::jlimit(0.0f, 1.0f, pan);
        inputs_[inputIndex].smoothedPan.setTargetValue(pan);
        updateInputGains(inputIndex);
    }
}

void SIMDMixer::setMasterGain(float gain) {
    masterGain_.setTargetValue(juce::jlimit(0.0f, 2.0f, gain));
}

void SIMDMixer::setStereoWidth(float width) {
    stereoWidth_ = juce::jlimit(-1.0f, 1.0f, width);
}

void SIMDMixer::updateInputGains(int inputIndex) {
    auto& input = inputs_[inputIndex];

    // Equal power pan law
    float angle = input.pan * juce::MathConstants<float>::halfPi * 0.5f;
    input.leftGain = std::cos(angle) * input.gain;
    input.rightGain = std::sin(angle) * input.gain;
}

void SIMDMixer::process(const float* const* inputs, int numInputs,
                        float* leftOutput, float* rightOutput, int numSamples) {
    // Clear outputs
    juce::FloatVectorOperations::fill(leftOutput, 0.0f, numSamples);
    juce::FloatVectorOperations::fill(rightOutput, 0.0f, numSamples);

    float masterGain = masterGain_.getNextValue();

    for (int i = 0; i < numInputs && i < maxInputs; ++i) {
        if (!inputs[i]) continue;

        auto& inputState = inputs_[i];
        float leftGain = inputState.leftGain * masterGain;
        float rightGain = inputState.rightGain * masterGain;

        // Add to mix with smoothing
        juce::FloatVectorOperations::addWithMultiply(leftOutput, inputs[i], leftGain, numSamples);
        juce::FloatVectorOperations::addWithMultiply(rightOutput, inputs[i], rightGain, numSamples);
    }

    // Apply stereo width
    if (stereoWidth_ != 1.0f) {
        for (int i = 0; i < numSamples; ++i) {
            float mid = (leftOutput[i] + rightOutput[i]) * 0.5f;
            float side = (leftOutput[i] - rightOutput[i]) * 0.5f * stereoWidth_;
            leftOutput[i] = mid + side;
            rightOutput[i] = mid - side;
        }
    }
}

void SIMDMixer::reset() {
    masterGain_.reset(44100.0, 0.01);
    masterGain_.setTargetValue(1.0f);

    for (auto& input : inputs_) {
        input.gain = 1.0f;
        input.pan = 0.5f;
        input.smoothedGain.reset(44100.0, 0.01);
        input.smoothedPan.reset(44100.0, 0.01);
        updateInputGains(static_cast<int>(&input - inputs_.data()));
    }
}

//==============================================================================
// SIMDDSP Implementation
//==============================================================================

void SIMDDSP::sin(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = std::sin(input[i]);
    }
}

void SIMDDSP::cos(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = std::cos(input[i]);
    }
}

void SIMDDSP::tanh(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = std::tanh(input[i]);
    }
}

void SIMDDSP::pow(const float* input, float exponent, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = std::pow(input[i], exponent);
    }
}

void SIMDDSP::exp(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = std::exp(input[i]);
    }
}

void SIMDDSP::log(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = std::log(input[i]);
    }
}

void SIMDDSP::sqrt(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = std::sqrt(input[i]);
    }
}

void SIMDDSP::lerp(const float* a, const float* b, float* output, int numSamples, float t) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = a[i] + (b[i] - a[i]) * t;
    }
}

void SIMDDSP::multiply(const float* a, const float* b, float* output, int numSamples) {
    juce::FloatVectorOperations::multiply(output, a, b, numSamples);
}

void SIMDDSP::add(const float* a, const float* b, float* output, int numSamples) {
    juce::FloatVectorOperations::add(output, a, b, numSamples);
}

void SIMDDSP::scaleAdd(const float* a, float scale, const float* b, float* output, int numSamples) {
    juce::FloatVectorOperations::copy(output, b, numSamples);
    juce::FloatVectorOperations::addWithMultiply(output, a, scale, numSamples);
}

void SIMDDSP::applyHannWindow(float* samples, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float phase = static_cast<float>(i) / (numSamples - 1);
        float window = 0.5f * (1.0f - std::cos(phase * juce::MathConstants<float>::twoPi));
        samples[i] *= window;
    }
}

void SIMDDSP::applyHammingWindow(float* samples, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float phase = static_cast<float>(i) / (numSamples - 1);
        float window = 0.54f - 0.46f * std::cos(phase * juce::MathConstants<float>::twoPi);
        samples[i] *= window;
    }
}

void SIMDDSP::applyBlackmanWindow(float* samples, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float phase = static_cast<float>(i) / (numSamples - 1);
        float window = 0.42f
                    - 0.5f * std::cos(phase * juce::MathConstants<float>::twoPi)
                    + 0.08f * std::cos(2.0f * phase * juce::MathConstants<float>::twoPi);
        samples[i] *= window;
    }
}

void SIMDDSP::generateHannWindow(float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float phase = static_cast<float>(i) / (numSamples - 1);
        output[i] = 0.5f * (1.0f - std::cos(phase * juce::MathConstants<float>::twoPi));
    }
}

void SIMDDSP::flushDenormals(float* samples, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        if (isDenormal(samples[i])) {
            samples[i] = 0.0f;
        }
    }
}

//==============================================================================
// CPUFeatures Implementation
//==============================================================================

juce::String CPUFeatures::getFeatureString() {
    juce::String features;

    if (hasSSE()) features << "SSE ";
    if (hasSSE2()) features << "SSE2 ";
    if (hasSSE3()) features << "SSE3 ";
    if (hasSSE41()) features << "SSE4.1 ";
    if (hasSSE42()) features << "SSE4.2 ";
    if (hasAVX()) features << "AVX ";
    if (hasAVX2()) features << "AVX2 ";
    if (hasAVX512()) features << "AVX512 ";
    if (hasNEON()) features << "NEON ";

    if (features.isEmpty())
        features = "None detected";

    return features.trimEnd();
}

int CPUFeatures::getSIMDWidth() {
    if (hasAVX512()) return 16;  // 16 floats
    if (hasAVX() || hasNEON()) return 4;  // 4 floats (or 8 with AVX2)
    if (hasSSE()) return 4;     // 4 floats
    return 1;                   // Scalar
}

//==============================================================================
// SIMDDispatcher Implementation
//==============================================================================

std::unique_ptr<SIMDBufferOps> SIMDDispatcher::createBufferOps() {
    return std::make_unique<SIMDBufferOps>();
}

std::unique_ptr<SIMDOscillator> SIMDDispatcher::createOscillator() {
    return std::make_unique<SIMDOscillator>();
}

} // namespace zenith
