/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "ZenithEffects.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// REVERB
//==============================================================================

ZenithReverb::ZenithReverb() {
    params_.roomSize = 0.5f;
    params_.damping = 0.5f;
    params_.wetLevel = 0.33f;
    params_.dryLevel = 0.67f;
    params_.width = 1.0f;
    reverb_.setParameters(params_);
}

void ZenithReverb::setSampleRate(double sr) {
    reverb_.setSampleRate(sr);
}

void ZenithReverb::setRoomSize(float size) {
    params_.roomSize = juce::jlimit(0.0f, 1.0f, size);
    reverb_.setParameters(params_);
}

void ZenithReverb::setDamping(float damping) {
    params_.damping = juce::jlimit(0.0f, 1.0f, damping);
    reverb_.setParameters(params_);
}

void ZenithReverb::setWidth(float width) {
    params_.width = juce::jlimit(0.0f, 1.0f, width);
    reverb_.setParameters(params_);
}

void ZenithReverb::setMix(float mix) {
    params_.wetLevel = juce::jlimit(0.0f, 1.0f, mix);
    params_.dryLevel = 1.0f - mix;
    reverb_.setParameters(params_);
}

void ZenithReverb::setPreDelay(float delayMs) {
    preDelaySamples_ = static_cast<int>((delayMs / 1000.0f) * sampleRate_);
}

void ZenithReverb::setDecay(float decay) {
    params_.damping = juce::jlimit(0.0f, 1.0f, 1.0f - decay);
    reverb_.setParameters(params_);
}

void ZenithReverb::process(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return;

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    reverb_.processStereo(left, right, numSamples);
}

void ZenithReverb::reset() {
    reverb_.reset();
    preDelayBuffer_[0] = 0.0f;
    preDelayBuffer_[1] = 0.0f;
}

//==============================================================================
// DELAY
//==============================================================================

ZenithDelay::ZenithDelay() {
    sampleRate_ = 44100.0;
    time_ = 0.25f;
    feedback_ = 0.5f;
    mix_ = 0.3f;
    pingPong_ = false;
    lastLeft_ = 0.0f;
    lastRight_ = 0.0f;
}

void ZenithDelay::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void ZenithDelay::setTime(float timeSeconds) {
    time_ = juce::jlimit(0.0f, 2.0f, timeSeconds);
}

void ZenithDelay::setFeedback(float fb) {
    feedback_ = juce::jlimit(0.0f, 0.95f, fb);
}

void ZenithDelay::setMix(float mix) {
    mix_ = juce::jlimit(0.0f, 1.0f, mix);
}

void ZenithDelay::setPingPong(bool pp) {
    pingPong_ = pp;
}

void ZenithDelay::setSync(bool sync) {
    sync_ = sync;
}

void ZenithDelay::setSyncRate(float noteLength) {
    syncRate_ = noteLength;
}

void ZenithDelay::process(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return;

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    int delaySamples = static_cast<int>(time_ * sampleRate_);

    float dryLevel = 1.0f - mix_;
    float wetLevel = mix_;

    for (int i = 0; i < numSamples; ++i) {
        float delayedL = lastLeft_;
        float delayedR = lastRight_;

        // Write input to delay
        delayLineL_->push(left[i] + lastLeft_ * feedback_);
        delayLineR_->push(right[i] + lastRight_ * feedback_);

        // Ping-pong: cross-feedback
        if (pingPong_) {
            delayLineR_->push(right[i] + lastRight_ * feedback_);
        }

        // Output with wet/dry mix
        left[i] = left[i] * dryLevel + delayedL * wetLevel;
        right[i] = right[i] * dryLevel + delayedR * wetLevel;

        lastLeft_ = delayedL;
        lastRight_ = delayedR;
    }
}

void ZenithDelay::reset() {
    lastLeft_ = 0.0f;
    lastRight_ = 0.0f;
}

//==============================================================================
// CHORUS
//==============================================================================

ZenithChorus::ZenithChorus() {
    sampleRate_ = 44100.0;
    numVoices_ = 4;
    rate_ = 0.5f;
    depth_ = 0.5f;
    mix_ = 0.5f;
    spread_ = 0.5f;
}

void ZenithChorus::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void ZenithChorus::setRate(float rateHz) {
    rate_ = juce::jlimit(0.1f, 10.0f, rateHz);
}

void ZenithChorus::setDepth(float depth) {
    depth_ = juce::jlimit(0.0f, 1.0f, depth);
}

void ZenithChorus::setVoices(int voices) {
    numVoices_ = juce::jlimit(1, MAX_VOICES, voices);
}

void ZenithChorus::setMix(float mix) {
    mix_ = juce::jlimit(0.0f, 1.0f, mix);
}

void ZenithChorus::setSpread(float spread) {
    spread_ = juce::jlimit(0.0f, 1.0f, spread);

    for (int i = 0; i < numVoices_; ++i) {
        float basePan = (static_cast<float>(i) / static_cast<float>(numVoices_ - 1)) * 2.0f - 1.0f;
        voices_[i].pan = basePan * spread_;
    }
}

void ZenithChorus::process(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return;

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    float maxDelayMs = 10.0f;
    int maxDelaySamples = static_cast<int>(maxDelayMs * sampleRate_ / 1000.0f);

    for (int i = 0; i < numSamples; ++i) {
        float input = (left[i] + right[i]) * 0.5f;
        float wetLeft = 0.0f;
        float wetRight = 0.0f;

        for (int v = 0; v < numVoices_; ++v) {
            double phaseInc = rate_ / sampleRate_;
            voices_[v].phase = std::fmod(voices_[v].phase + phaseInc, 1.0);

            float lfoValue = static_cast<float>(std::sin(
                voices_[v].phase * juce::MathConstants<double>::twoPi));

            int delaySamples = static_cast<int>(
                (0.5f + 0.5f * lfoValue * depth_) * maxDelaySamples);

            float voiceSample = input;

            float pan = voices_[v].pan;
            float leftGain = (pan <= 0.0f) ? 1.0f : std::cos(pan * juce::MathConstants<float>::pi * 0.25f);
            float rightGain = (pan >= 0.0f) ? 1.0f : std::sin(-pan * juce::MathConstants<float>::pi * 0.25f);

            wetLeft += voiceSample * leftGain * voices_[v].gain;
            wetRight += voiceSample * rightGain * voices_[v].gain;
        }

        float dryLevel = 1.0f - mix_;
        float wetLevel = mix_;

        left[i] = left[i] * dryLevel + wetLeft * wetLevel;
        right[i] = right[i] * dryLevel + wetRight * wetLevel;
    }
}

void ZenithChorus::reset() {
    for (auto& voice : voices_) {
        voice.phase = 0.0;
    }
}

//==============================================================================
// PHASER
//==============================================================================

ZenithPhaser::ZenithPhaser() {
    sampleRate_ = 44100.0;
    numStages_ = 4;
    rate_ = 0.5f;
    depth_ = 0.5f;
    feedback_ = 0.5f;
    mix_ = 0.5f;
    lfoPhase_ = 0.0;
    lastFeedbackSample_ = 0.0f;
}

void ZenithPhaser::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void ZenithPhaser::setRate(float rateHz) {
    rate_ = juce::jlimit(0.1f, 10.0f, rateHz);
}

void ZenithPhaser::setDepth(float depth) {
    depth_ = juce::jlimit(0.0f, 1.0f, depth);
}

void ZenithPhaser::setFeedback(float fb) {
    feedback_ = juce::jlimit(-0.95f, 0.95f, fb);
}

void ZenithPhaser::setStages(int stages) {
    numStages_ = juce::jlimit(2, MAX_STAGES, stages);
}

void ZenithPhaser::setMix(float mix) {
    mix_ = juce::jlimit(0.0f, 1.0f, mix);
}

void ZenithPhaser::process(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return;

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        // Update LFO
        double phaseInc = rate_ / sampleRate_;
        lfoPhase_ = std::fmod(lfoPhase_ + phaseInc, 1.0);

        float lfo = static_cast<float>(std::sin(lfoPhase_ * juce::MathConstants<double>::twoPi));

        // Calculate modulation amount
        float mod = 0.5f + 0.5f * lfo * depth_;

        float input = (left[i] + right[i]) * 0.5f + lastFeedbackSample_ * feedback_;

        // Process through allpass stages
        float output = input;

        for (int s = 0; s < numStages_; ++s) {
            float coeff = 0.3f + 0.2f * mod;

            float& stateL = allpassState_[s][0];
            float& stateR = allpassState_[s][1];

            float delayedL = stateL;
            float delayedR = stateR;

            stateL = output + coeff * delayedL;
            stateR = output + coeff * delayedR;

            output = delayedL;
        }

        lastFeedbackSample_ = output;

        float wetLevel = mix_;
        float dryLevel = 1.0f - mix_;

        left[i] = left[i] * dryLevel + output * wetLevel;
        right[i] = right[i] * dryLevel + output * wetLevel;
    }
}

void ZenithPhaser::reset() {
    allpassState_.fill({});
    lastFeedbackSample_ = 0.0f;
}

//==============================================================================
// DISTORTION
//==============================================================================

ZenithDistortion::ZenithDistortion() = default;

void ZenithDistortion::setType(Type type) {
    type_ = type;
}

void ZenithDistortion::setDrive(float drive) {
    drive_ = juce::jlimit(0.0f, 1.0f, drive);
}

void ZenithDistortion::setTone(float tone) {
    tone_ = juce::jlimit(0.0f, 1.0f, tone);
}

void ZenithDistortion::setMix(float mix) {
    mix_ = juce::jlimit(0.0f, 1.0f, mix);
}

void ZenithDistortion::process(juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* channel = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float input = channel[i];

            // Apply drive
            float driveGain = 1.0f + drive_ * 20.0f;
            float driven = input * driveGain;

            // Apply distortion
            float distorted = applyDistortion(driven);

            // Apply tone filter (simple lowpass)
            float toneFreq = 500.0f + tone_ * 4500.0f;
            float toneCoeff = toneFreq / (toneFreq + static_cast<float>(sampleRate_));
            float toneFiltered = toneZ1_ + toneCoeff * (distorted - toneZ1_);
            toneZ1_ = toneFiltered;

            // Mix dry/wet
            channel[i] = input * (1.0f - mix_) + toneFiltered * mix_;
        }
    }
}

void ZenithDistortion::reset() {
    toneZ1_ = 0.0f;
}

float ZenithDistortion::applyDistortion(float input) const {
    switch (type_) {
        case Type::SoftClip:
            return softClip(input);
        case Type::HardClip:
            return hardClip(input);
        case Type::Bitcrush: {
            float bits = 1.0f + (1.0f - drive_) * 15.0f;
            float levels = std::pow(2.0f, bits);
            return std::floor(input * levels) / levels;
        }
        case Type::Wavefold:
            return wavefold(input, 1.0f + drive_ * 5.0f);
        case Type::HalfWave:
            return (input > 0.0f) ? input : 0.0f;
        case Type::FullWave:
            return std::abs(input);
        default:
            return input;
    }
}

float ZenithDistortion::softClip(float x) {
    return std::tanh(x);
}

float ZenithDistortion::hardClip(float x) {
    return juce::jmax(-1.0f, juce::jmin(1.0f, x));
}

float ZenithDistortion::wavefold(float x, float amount) {
    float threshold = 1.0f / amount;
    while (std::abs(x) > threshold) {
        if (x > threshold) {
            x = threshold - (x - threshold);
        } else if (x < -threshold) {
            x = -threshold - (x + threshold);
        }
    }
    return x;
}

//==============================================================================
// COMPRESSOR
//==============================================================================

ZenithCompressor::ZenithCompressor() = default;

void ZenithCompressor::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void ZenithCompressor::setThreshold(float thresholdDb) {
    threshold_ = juce::jlimit(-60.0f, 0.0f, thresholdDb);
}

void ZenithCompressor::setRatio(float ratio) {
    ratio_ = juce::jlimit(1.0f, 20.0f, ratio);
}

void ZenithCompressor::setKnee(float kneeDb) {
    knee_ = juce::jlimit(0.0f, 24.0f, kneeDb);
}

void ZenithCompressor::setAttack(float attackMs) {
    attackMs_ = juce::jmax(0.1f, attackMs);
}

void ZenithCompressor::setRelease(float releaseMs) {
    releaseMs_ = juce::jmax(1.0f, releaseMs);
}

void ZenithCompressor::setMakeupGain(float makeupDb) {
    makeupDb_ = makeupDb;
}

void ZenithCompressor::setAutoMakeup(bool autoMakeup) {
    autoMakeup_ = autoMakeup;
}

void ZenithCompressor::setSidechainEnabled(bool enabled) {
    sidechainEnabled_ = enabled;
}

void ZenithCompressor::setSidechainSource(SidechainSource source) {
    sidechainSource_ = source;
}

void ZenithCompressor::setSidechainFilterFreq(float freq) {
    sidechainFilterFreq_ = juce::jlimit(20.0f, 2000.0f, freq);
}

void ZenithCompressor::process(juce::AudioBuffer<float>& buffer,
                                 const juce::AudioBuffer<float>* sidechain) {
    float attackCoeff = std::exp(-1.0f / (attackMs_ * sampleRate_ / 1000.0f));
    float releaseCoeff = std::exp(-1.0f / (releaseMs_ * sampleRate_ / 1000.0f));

    const juce::AudioBuffer<float>* detectBuffer = (sidechainEnabled_ && sidechain) ? sidechain : &buffer;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* channel = buffer.getWritePointer(ch);
        const float* detectChannel = detectBuffer->getReadPointer(std::min(ch, detectBuffer->getNumChannels() - 1));

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float input = channel[i];
            float detectSample = detectChannel[i];

            // Apply HPF to sidechain for ducking
            float hpfCoeff = sidechainFilterFreq_ / (sidechainFilterFreq + sampleRate_);
            float filtered = sidechainFilterZ1_ + hpfCoeff * (detectSample - sidechainFilterZ1_);
            sidechainFilterZ1_ = filtered;

            float inputDb = 20.0f * std::log10(std::abs(filtered) + 1e-6f);

            // Calculate gain with soft knee
            float gain = calculateGain(inputDb);

            // Smooth gain
            float coeff = (gain < envelope_) ? attackCoeff : releaseCoeff;
            envelope_ += coeff * (gain - envelope_);

            // Apply gain
            float makeupGain = juce::Decibels::decibelsToGain(
                autoMakeup_ ? (-threshold_ / ratio_ / 2.0f) : makeupDb_);
            channel[i] = input * envelope_ * makeupGain;
        }
    }

    gainReduction_ = envelope_;
}

void ZenithCompressor::process(juce::AudioBuffer<float>& buffer) {
    process(buffer, nullptr);
}

void ZenithCompressor::reset() {
    envelope_ = 1.0f;
    sidechainFilterZ1_ = 0.0f;
}

float ZenithCompressor::calculateGain(float inputDb) {
    float kneeHalf = knee_ * 0.5f;

    if (inputDb <= threshold_ - kneeHalf) {
        return 1.0f;
    } else if (inputDb >= threshold_ + kneeHalf) {
        float overshoot = inputDb - threshold_;
        return juce::Decibels::decibelsToGain(-overshoot * (1.0f - 1.0f / ratio_));
    } else {
        float x = (inputDb - threshold_ + kneeHalf) / knee_;
        float gainDb = -x * x * ((1.0f / ratio_) - 1.0f) * kneeHalf * 0.25f;
        return juce::Decibels::decibelsToGain(gainDb);
    }
}

//==============================================================================
// LIMITER
//==============================================================================

ZenithLimiter::ZenithLimiter() = default;

void ZenithLimiter::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void ZenithLimiter::setThreshold(float thresholdDb) {
    threshold_ = juce::jlimit(-20.0f, 0.0f, thresholdDb);
    ceiling_ = juce::jmin(ceiling_, threshold_);
}

void ZenithLimiter::setRelease(float releaseMs) {
    releaseMs = juce::jmax(1.0f, releaseMs);
}

void ZenithLimiter::setCeiling(float ceilingDb) {
    ceiling_ = juce::jlimit(-20.0f, 0.0f, ceilingDb);
}

void ZenithLimiter::process(juce::AudioBuffer<float>& buffer) {
    float thresholdGain = juce::Decibels::decibelsToGain(threshold_);
    float ceilingGain = juce::Decibels::decibelsToGain(ceiling_);
    float releaseCoeff = std::exp(-1.0f / (releaseMs_ * sampleRate_ / 1000.0f));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* channel = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float input = std::abs(channel[i]);

            // Calculate gain reduction needed
            float targetGain = (input > thresholdGain) ? thresholdGain / input : 1.0f;

            // Smooth gain
            envelope_ += releaseCoeff * (targetGain - envelope_);

            // Apply gain reduction and ceiling
            channel[i] = juce::jmax(-ceilingGain, juce::jmin(ceilingGain, channel[i] * envelope_));
        }
    }

    gainReduction_ = juce::Decibels::gainToDecibels(envelope_);
}

void ZenithLimiter::reset() {
    envelope_ = 1.0f;
}

//==============================================================================
// EFFECTS CHAIN
//==============================================================================

ZenithEffectsChain::ZenithEffectsChain() = default;

void ZenithEffectsChain::setSampleRate(double sr) {
    reverb_.setSampleRate(sr);
    delay_.setSampleRate(sr);
    chorus_.setSampleRate(sr);
    phaser_.setSampleRate(sr);
    distortion_.reset();
    compressor_.setSampleRate(sr);
    limiter_.setSampleRate(sr);
}

void ZenithEffectsChain::reset() {
    reverb_.reset();
    delay_.lastLeft_ = 0.0f;
    delay_.lastRight_ = 0.0f;
    chorus_.reset();
    phaser_.reset();
    distortion_.reset();
    compressor_.reset();
    limiter_.reset();
}

void ZenithEffectsChain::setEffectBypass(EffectType type, bool bypassed) {
    switch (type) {
        case EffectType::Reverb: reverbBypass_ = bypassed; break;
        case EffectType::Delay: delayBypass_ = bypassed; break;
        case EffectType::Chorus: chorusBypass_ = bypassed; break;
        case EffectType::Phaser: phaserBypass_ = bypassed; break;
        case EffectType::Distortion: distortionBypass_ = bypassed; break;
        case EffectType::Compressor: compressorBypass_ = bypassed; break;
        default: break;
    }
}

bool ZenithEffectsChain::isEffectBypassed(EffectType type) const {
    switch (type) {
        case EffectType::Reverb: return reverbBypass_;
        case EffectType::Delay: return delayBypass_;
        case EffectType::Chorus: return chorusBypass_;
        case EffectType::Phaser: return phaserBypass_;
        case EffectType::Distortion: return distortionBypass_;
        case EffectType::Compressor: return compressorBypass_;
        default: return false;
    }
}

ZenithReverb& ZenithEffectsChain::getReverb() { return reverb_; }
ZenithDelay& ZenithEffectsChain::getDelay() { return delay_; }
ZenithChorus& ZenithEffectsChain::getChorus() { return chorus_; }
ZenithPhaser& ZenithEffectsChain::getPhaser() { return phaser_; }
ZenithDistortion& ZenithEffectsChain::getDistortion() { return distortion_; }
ZenithCompressor& ZenithEffectsChain::getCompressor() { return compressor_; }
ZenithLimiter& ZenithEffectsChain::getLimiter() { return limiter_; }

void ZenithEffectsChain::process(juce::AudioBuffer<float>& buffer,
                                juce::AudioBuffer<float>* /* osc1Buffer */,
                                juce::AudioBuffer<float>* /* osc2Buffer */,
                                juce::AudioBuffer<float>* /* osc3Buffer */,
                                const juce::AudioBuffer<float>* /* sidechain */) {
    juce::ignoreUnused(osc1Buffer, osc2Buffer, osc3Buffer, sidechain);

    // Process in series (bypass where needed)
    if (!reverbBypass_) reverb_.process(buffer);
    if (!delayBypass_) delay_.process(buffer);
    if (!chorusBypass_) chorus_.process(buffer);
    if (!phaserBypass_) phaser_.process(buffer);
    if (!distortionBypass_) distortion_.process(buffer);
    if (!compressorBypass_) compressor_.process(buffer);

    // Limiter always runs (safety)
    limiter_.process(buffer);
}

} // namespace zenith
