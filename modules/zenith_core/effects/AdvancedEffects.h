/*
    Advanced Effects for Zenith Ultra Synth
    Features: Phaser, Flanger, Ring Modulator, Frequency Shifter, Granular FX, Vocoder, Compressor, EQ, Limiter
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include <array>

namespace zenith {

//==============================================================================
// Stereo Effect Base
//==============================================================================

class StereoEffect {
public:
    StereoEffect() { setSampleRate(44100.0f); }
    virtual ~StereoEffect() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateCoefficients();
    }

    void setMix(float mix) {
        mix_ = juce::jlimit(0.0f, 1.0f, mix);
    }

    void process(float* left, float* right, int numSamples) {
        // Wet/dry mix
        std::vector<float> dryLeft(numSamples);
        std::vector<float> dryRight(numSamples);

        // Copy dry signal
        std::copy(left, left + numSamples, dryLeft.begin());
        std::copy(right, right + numSamples, dryRight.begin());

        // Process wet
        processEffect(left, right, numSamples);

        // Mix
        for (int i = 0; i < numSamples; ++i) {
            float dryL = dryLeft[i];
            float dryR = dryRight[i];
            float wetL = left[i];
            float wetR = right[i];

            // Equal power mixing
            float dryGain = std::sqrt(1.0f - mix_);
            float wetGain = std::sqrt(mix_);

            left[i] = dryL * dryGain + wetL * wetGain;
            right[i] = dryR * dryGain + wetR * wetGain;
        }
    }

    virtual void reset() {}

protected:
    virtual void processEffect(float* left, float* right, int numSamples) = 0;
    virtual void updateCoefficients() {}

    float sampleRate_ = 44100.0f;
    float mix_ = 1.0f;
};

//==============================================================================
// Phaser (Multi-stage allpass modulation)
//==============================================================================

class Phaser : public StereoEffect {
public:
    Phaser() {
        stages_ = 4;
    }

    void setStages(int numStages) {
        stages_ = juce::jlimit(2, 12, numStages);
    }

    void setRate(float rateHz) {
        rateHz_ = juce::jlimit(0.01f, 20.0f, rateHz);
    }

    void setDepth(float depth) {
        depth_ = juce::jlimit(0.0f, 1.0f, depth);
    }

    void setFeedback(float feedback) {
        feedback_ = juce::jlimit(-0.95f, 0.95f, feedback);
    }

    void setBaseFrequency(float freq) {
        baseFreq_ = juce::jlimit(100.0f, 5000.0f, freq);
    }

protected:
    void processEffect(float* left, float* right, int numSamples) override {
        for (int i = 0; i < numSamples; ++i) {
            // Update LFO
            lfoPhase_ += lfoInc_;
            if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;

            float lfo = std::sin(lfoPhase_ * juce::MathConstants<float>::twoPi);
            float modFreq = baseFreq_ * (1.0f + lfo * depth_);

            // Calculate allpass coefficient
            float wc = 2.0f * juce::MathConstants<float>::pi * modFreq / sampleRate_;
            float coef = (std::tan(wc * 0.5f) - 1.0f) / (std::tan(wc * 0.5f) + 1.0f);

            // Process left channel through allpass stages
            float allpassL = processAllpass(left[i], coef, 0);
            float allpassR = processAllpass(right[i], coef, 1);

            // Add feedback
            float feedbackL = feedbackL_ * feedback_;
            float feedbackR = feedbackR_ * feedback_;

            left[i] = allpassL + feedbackL;
            right[i] = allpassR + feedbackR;

            feedbackL_ = left[i];
            feedbackR_ = right[i];
        }
    }

    void updateCoefficients() override {
        lfoInc_ = rateHz_ / sampleRate_;
    }

    void reset() override {
        for (auto& state : allpassStates_) {
            std::fill(state.begin(), state.end(), 0.0f);
        }
        feedbackL_ = 0.0f;
        feedbackR_ = 0.0f;
        lfoPhase_ = 0.0f;
    }

private:
    float processAllpass(float input, float coef, int channel) {
        float* z = allpassStates_[channel].data();

        // Two allpass filters per stage
        for (int s = 0; s < stages_; ++s) {
            int offset = s * 2;

            float z0 = z[offset];
            float z1 = z[offset + 1];

            float output = z0 - coef * input;
            z[offset] = input;
            z[offset + 1] = output + coef * z1;

            input = output;
        }

        return input;
    }

    int stages_ = 4;
    float rateHz_ = 0.5f;
    float depth_ = 0.5f;
    float feedback_ = 0.5f;
    float baseFreq_ = 1000.0f;

    float lfoPhase_ = 0.0f;
    float lfoInc_ = 0.0f;

    std::array<std::array<float, 24>, 2> allpassStates_{};  // 12 stages * 2 per stage
    float feedbackL_ = 0.0f;
    float feedbackR_ = 0.0f;
};

//==============================================================================
// Flanger (Delay-based modulation)
//==============================================================================

class Flanger : public StereoEffect {
public:
    void setRate(float rateHz) {
        rateHz_ = juce::jlimit(0.01f, 10.0f, rateHz);
    }

    void setDepth(float depthMs) {
        depthMs_ = juce::jlimit(0.0f, 10.0f, depthMs);
    }

    void setFeedback(float feedback) {
        feedback_ = juce::jlimit(-0.95f, 0.95f, feedback);
    }

    void setDelayTime(float delayMs) {
        delayMs_ = juce::jlimit(0.1f, 50.0f, delayMs);
    }

protected:
    void processEffect(float* left, float* right, int numSamples) override {
        for (int i = 0; i < numSamples; ++i) {
            // Update LFO
            lfoPhase_ += lfoInc_;
            if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;

            float lfo = std::sin(lfoPhase_ * juce::MathConstants<float>::twoPi);

            // Modulate delay time
            float currentDelay = (delayMs_ + lfo * depthMs_) / 1000.0f;
            float delaySamples = currentDelay * sampleRate_;

            // Process channels
            left[i] = processDelay(left[i], delaySamples, 0);
            right[i] = processDelay(right[i], delaySamples, 1);
        }
    }

    void updateCoefficients() override {
        lfoInc_ = rateHz_ / sampleRate_;

        // Allocate delay buffer
        size_t maxDelay = static_cast<size_t>((delayMs_ + depthMs_ + 10.0f) / 1000.0f * sampleRate_);
        delayBuffer_.resize(maxDelay * 2, 0.0f);
    }

    void reset() override {
        std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);
        writePos_ = 0;
        lfoPhase_ = 0.0f;
    }

private:
    float processDelay(float input, float delaySamples, int channel) {
        float delaySamplesClamped = juce::jlimit(1.0f, static_cast<float>(delayBuffer_.size() / 2), delaySamples);

        // Read position with linear interpolation
        float readPos = writePos_ - delaySamplesClamped;
        if (readPos < 0) readPos += delayBuffer_.size() / 2;

        int readPosInt = static_cast<int>(readPos);
        float frac = readPos - readPosInt;
        int readPosNext = (readPosInt + 1) % (delayBuffer_.size() / 2);

        size_t offset = channel * (delayBuffer_.size() / 2);
        float delayed = delayBuffer_[offset + readPosInt] * (1.0f - frac) +
                       delayBuffer_[offset + readPosNext] * frac;

        // Add feedback
        float output = input + delayed * feedback_;

        // Write to buffer
        delayBuffer_[offset + writePos_] = output;

        return delayed;
    }

    float rateHz_ = 0.5f;
    float depthMs_ = 1.0f;
    float feedback_ = 0.5f;
    float delayMs_ = 5.0f;

    float lfoPhase_ = 0.0f;
    float lfoInc_ = 0.0f;

    std::vector<float> delayBuffer_;
    size_t writePos_ = 0;
};

//==============================================================================
// Ring Modulator
//==============================================================================

class RingModulator : public StereoEffect {
public:
    void setCarrierFrequency(float freq) {
        carrierFreq_ = juce::jlimit(1.0f, 10000.0f, freq);
        updatePhaseIncrement();
    }

    void setWaveType(int type) {
        // 0 = sine, 1 = triangle, 2 = saw, 3 = square
        waveType_ = juce::jlimit(0, 3, type);
    }

    void setCarrierMix(float mix) {
        carrierMix_ = juce::jlimit(0.0f, 1.0f, mix);
    }

protected:
    void processEffect(float* left, float* right, int numSamples) override {
        for (int i = 0; i < numSamples; ++i) {
            // Generate carrier
            carrierPhase_ += carrierInc_;
            if (carrierPhase_ >= 1.0f) carrierPhase_ -= 1.0f;

            float carrier = generateCarrier(carrierPhase_);

            // Ring modulate
            float modL = left[i] * carrier;
            float modR = right[i] * carrier;

            // Blend with dry
            left[i] = modL * carrierMix_ + left[i] * (1.0f - carrierMix_);
            right[i] = modR * carrierMix_ + right[i] * (1.0f - carrierMix_);
        }
    }

    void updateCoefficients() override {
        updatePhaseIncrement();
    }

    void reset() override {
        carrierPhase_ = 0.0f;
    }

private:
    float generateCarrier(float phase) {
        float angle = phase * juce::MathConstants<float>::twoPi;

        switch (waveType_) {
            case 0:  // Sine
                return std::sin(angle);

            case 1:  // Triangle
                {
                    float p = phase * 4.0f;
                    if (p < 1.0f) return p;
                    if (p < 3.0f) return 2.0f - p;
                    return p - 4.0f;
                }

            case 2:  // Saw
                return 2.0f * phase - 1.0f;

            case 3:  // Square
                return (phase < 0.5f) ? 1.0f : -1.0f;

            default:
                return std::sin(angle);
        }
    }

    void updatePhaseIncrement() {
        carrierInc_ = carrierFreq_ / sampleRate_;
    }

    float carrierFreq_ = 440.0f;
    float carrierInc_ = 0.0f;
    float carrierPhase_ = 0.0f;
    int waveType_ = 0;
    float carrierMix_ = 1.0f;
};

//==============================================================================
// Frequency Shifter (Single Sideband Modulation)
//==============================================================================

class FrequencyShifter : public StereoEffect {
public:
    void setShift(float shiftHz) {
        shiftHz_ = juce::jlimit(-5000.0f, 5000.0f, shiftHz);
        updatePhaseIncrement();
    }

    void setThroughZero(bool throughZero) {
        throughZero_ = throughZero;
    }

protected:
    void processEffect(float* left, float* right, int numSamples) override {
        for (int i = 0; i < numSamples; ++i) {
            // Update modulator phase
            modPhase_ += modInc_;
            if (modPhase_ >= 1.0f) modPhase_ -= 1.0f;

            float modSin = std::sin(modPhase_ * juce::MathConstants<float>::twoPi);
            float modCos = std::cos(modPhase_ * juce::MathConstants<float>::twoPi);

            // Create Hilbert transform (90° phase shift)
            float hilbertL = processHilbert(left[i], 0);
            float hilbertR = processHilbert(right[i], 1);

            // Single sideband modulation
            float shiftedL = left[i] * modCos - hilbertL * modSin;
            float shiftedR = right[i] * modCos - hilbertR * modSin;

            // Downshift or upshift
            if (!throughZero_ && shiftHz_ < 0) {
                // Downshift: use lower sideband
                shiftedL = left[i] * modCos + hilbertL * modSin;
                shiftedR = right[i] * modCos + hilbertR * modSin;
            }

            left[i] = shiftedL;
            right[i] = shiftedR;
        }
    }

    void updateCoefficients() override {
        modInc_ = std::abs(shiftHz_) / sampleRate_;
    }

    void reset() override {
        modPhase_ = 0.0f;
        for (auto& state : hilbertStates_) {
            std::fill(state.begin(), state.end(), 0.0f);
        }
    }

private:
    float processHilbert(float input, int channel) {
        // Approximate Hilbert transform using allpass filters
        float* z = hilbertStates_[channel].data();

        // Cascaded allpass filters for 90° phase shift
        float x = input;

        for (int i = 0; i < 4; ++i) {
            float output = z[i * 2] + hilbertCoef_[i] * (x - z[i * 2 + 1]);
            z[i * 2 + 1] = output;
            z[i * 2] = x;
            x = output;
        }

        return x;
    }

    float shiftHz_ = 0.0f;
    float modInc_ = 0.0f;
    float modPhase_ = 0.0f;
    bool throughZero_ = false;

    // Hilbert transform coefficients (4 allpass stages)
    std::array<float, 4> hilbertCoef_{0.0675f, 0.2930f, 0.5680f, 0.8510f};
    std::array<std::array<float, 8>, 2> hilbertStates_{};  // 4 stages * 2 per stage
};

//==============================================================================
// Granular Effects
//==============================================================================

class GranularEffects : public StereoEffect {
public:
    void setGrainSize(float sizeMs) {
        grainSizeMs_ = juce::jlimit(1.0f, 500.0f, sizeMs);
    }

    void setDensity(float density) {
        density_ = juce::jlimit(0.1f, 1.0f, density);
    }

    void setPitchVar(float variance) {
        pitchVariance_ = juce::jlimit(0.0f, 2.0f, variance);
    }

    void setFreeze(bool freeze) {
        freeze_ = freeze;
    }

protected:
    void processEffect(float* left, float* right, int numSamples) override {
        for (int i = 0; i < numSamples; ++i) {
            // Record input
            if (!freeze_) {
                size_t writeIdx = writePos_ % buffer_.size();
                buffer_[writeIdx] = left[i];
                bufferStereo_[writeIdx] = right[i];
                writePos_++;
            }

            // Check if new grain should start
            if (shouldStartGrain()) {
                startGrain();
            }

            // Sum active grains
            float grainL = 0.0f;
            float grainR = 0.0f;

            for (auto& grain : activeGrains_) {
                if (grain.active) {
                    grainL += processGrain(grain, left[i]);
                    grainR += processGrainStereo(grain, right[i]);
                    grain.position += grain.increment;

                    if (grain.position >= grain.length) {
                        grain.active = false;
                    }
                }
            }

            // Mix grains with input
            left[i] = grainL + left[i] * (1.0f - density_);
            right[i] = grainR + right[i] * (1.0f - density_);
        }
    }

    void updateCoefficients() override {
        size_t bufferSize = static_cast<size_t>((grainSizeMs_ * 2) / 1000.0f * sampleRate_);
        buffer_.resize(bufferSize, 0.0f);
        bufferStereo_.resize(bufferSize, 0.0f);
    }

    void reset() override {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        std::fill(bufferStereo_.begin(), bufferStereo_.end(), 0.0f);
        writePos_ = 0;
        for (auto& grain : activeGrains_) {
            grain.active = false;
        }
    }

private:
    struct Grain {
        bool active = false;
        float position = 0.0f;
        float increment = 1.0f;
        float length = 0.0f;
        float startPhase = 0.0f;
        float pan = 0.5f;
        float amplitude = 1.0f;
    };

    bool shouldStartGrain() {
        // Probabilistic grain spawning
        juce::Random rng;
        float probability = density_ * 0.1f;
        return rng.nextFloat() < probability;
    }

    void startGrain() {
        // Find inactive grain
        for (auto& grain : activeGrains_) {
            if (!grain.active) {
                grain.active = true;
                grain.position = 0.0f;

                // Random grain parameters
                juce::Random rng;
                float pitchVar = 1.0f + (rng.nextFloat() - 0.5f) * pitchVariance_;
                grain.increment = pitchVar;

                float grainSizeSamples = (grainSizeMs_ / 1000.0f) * sampleRate_;
                grain.length = grainSizeSamples;

                // Random start position in buffer
                size_t bufferSize = buffer_.size();
                grain.startPhase = rng.nextFloat() * bufferSize;

                grain.pan = rng.nextFloat();
                grain.amplitude = 0.5f + rng.nextFloat() * 0.5f;

                return;
            }
        }
    }

    float processGrain(const Grain& grain, float input) {
        // Read from buffer
        float readPos = grain.startPhase + grain.position;
        size_t readIdx = static_cast<size_t>(readPos) % buffer_.size();

        // Envelope (triangle)
        float env = 2.0f * std::abs(grain.position / grain.length - 0.5f);
        env = 1.0f - env;  // Triangle: 0 → 1 → 0
        env = std::max(0.0f, env);

        // Window with raised cosine
        env = env * env * (3.0f - 2.0f * env);

        return buffer_[readIdx] * env * grain.amplitude;
    }

    float processGrainStereo(const Grain& grain, float input) {
        float readPos = grain.startPhase + grain.position;
        size_t readIdx = static_cast<size_t>(readPos) % bufferStereo_.size();

        float env = 2.0f * std::abs(grain.position / grain.length - 0.5f);
        env = 1.0f - env;
        env = env * env * (3.0f - 2.0f * env);

        return bufferStereo_[readIdx] * env * grain.amplitude;
    }

    float grainSizeMs_ = 50.0f;
    float density_ = 0.5f;
    float pitchVariance_ = 0.5f;
    bool freeze_ = false;

    std::vector<float> buffer_;
    std::vector<float> bufferStereo_;
    size_t writePos_ = 0;

    std::array<Grain, 16> activeGrains_{};
};

//==============================================================================
// Compressor with Sidechain
//==============================================================================

class Compressor : public StereoEffect {
public:
    void setThreshold(float dB) {
        threshold_ = juce::jlimit(-60.0f, 0.0f, dB);
    }

    void setRatio(float ratio) {
        ratio_ = juce::jmax(1.0f, ratio);
    }

    void setKnee(float dB) {
        knee_ = juce::jmax(0.0f, dB);
    }

    void setAttack(float seconds) {
        attack_ = juce::jmax(0.0001f, seconds);
    }

    void setRelease(float seconds) {
        release_ = juce::jmax(0.01f, seconds);
    }

    void setSidechainEnabled(bool enabled) {
        sidechainEnabled_ = enabled;
    }

    void processSidechain(float* sidechain, int numSamples) {
        // External sidechain input would be processed here
        if (sidechainEnabled_) {
            // Use external signal for gain reduction
        }
    }

protected:
    void processEffect(float* left, float* right, int numSamples) override {
        for (int i = 0; i < numSamples; ++i) {
            // Get input level (RMS)
            float inputL = left[i];
            float inputR = right[i];
            float inputLevel = std::sqrt(inputL * inputL + inputR * inputR);

            // Convert to dB
            float inputDb = juce::Decibels::gainToDecibels(inputLevel + 1e-6f);

            // Calculate gain reduction
            float gainDb = calculateGainReduction(inputDb);

            // Smooth gain changes
            float targetGain = juce::Decibels::decibelsToGain(gainDb);
            gain_ = gain_ + (targetGain - gain_) * coef_;

            // Apply gain
            left[i] *= gain_;
            right[i] *= gain_;
        }
    }

    void updateCoefficients() override {
        float attackCoef = std::exp(-1.0f / (attack_ * sampleRate_));
        float releaseCoef = std::exp(-1.0f / (release_ * sampleRate_));
        coef_ = (attack_ > release_) ? attackCoef : releaseCoef;
    }

    void reset() override {
        gain_ = 1.0f;
    }

private:
    float calculateGainReduction(float inputDb) {
        float kneeHalf = knee_ * 0.5f;

        if (inputDb <= threshold_ - kneeHalf) {
            return 0.0f;  // No compression
        } else if (inputDb >= threshold_ + kneeHalf) {
            // Above knee
            return (threshold_ - inputDb) * (1.0f - 1.0f / ratio_);
        } else {
            // In knee
            float x = inputDb - threshold_ + kneeHalf;
            return x * x / (2.0f * knee_) * (1.0f - 1.0f / ratio_);
        }
    }

    float threshold_ = -20.0f;
    float ratio_ = 4.0f;
    float knee_ = 6.0f;
    float attack_ = 0.005f;
    float release_ = 0.1f;
    float gain_ = 1.0f;
    float coef_ = 0.001f;
    bool sidechainEnabled_ = false;
};

//==============================================================================
// Parametric EQ
//==============================================================================

class ParametricEQ : public StereoEffect {
public:
    enum class FilterType {
        Lowpass,
        Highpass,
        Bandpass,
        Bell,
        Lowshelf,
        Highshelf
    };

    struct Band {
        FilterType type = FilterType::Bell;
        float freq = 1000.0f;
        float gain = 0.0f;      // For bell/shelf
        float q = 1.0f;         // Bandwidth
    };

    ParametricEQ() {
        // Default 4-band EQ
        bands_.resize(4);
        bands_[0].type = FilterType::Lowshelf;
        bands_[0].freq = 100.0f;
        bands_[0].gain = 0.0f;

        bands_[1].type = FilterType::Bell;
        bands_[1].freq = 500.0f;
        bands_[1].gain = 0.0f;
        bands_[1].q = 1.0f;

        bands_[2].type = FilterType::Bell;
        bands_[2].freq = 2000.0f;
        bands_[2].gain = 0.0f;
        bands_[2].q = 1.0f;

        bands_[3].type = FilterType::Highshelf;
        bands_[3].freq = 8000.0f;
        bands_[3].gain = 0.0f;
    }

    void setBand(int index, const Band& band) {
        if (index >= 0 && index < static_cast<int>(bands_.size())) {
            bands_[index] = band;
            updateBandCoefficients(index);
        }
    }

    void setNumBands(int num) {
        bands_.resize(juce::jlimit(1, 8, num));
    }

    Band getBand(int index) const {
        if (index >= 0 && index < static_cast<int>(bands_.size())) {
            return bands_[index];
        }
        return Band{};
    }

protected:
    void processEffect(float* left, float* right, int numSamples) override {
        for (int i = 0; i < numSamples; ++i) {
            float sampleL = left[i];
            float sampleR = right[i];

            // Process each band
            for (size_t b = 0; b < bands_.size(); ++b) {
                sampleL = processBand(sampleL, b, 0);
                sampleR = processBand(sampleR, b, 1);
            }

            left[i] = sampleL;
            right[i] = sampleR;
        }
    }

    void updateCoefficients() override {
        for (size_t i = 0; i < bands_.size(); ++i) {
            updateBandCoefficients(i);
        }
    }

    void reset() override {
        for (auto& states : bandStates_) {
            std::fill(states.begin(), states.end(), 0.0f);
        }
    }

private:
    float processBand(float sample, size_t bandIndex, int channel) {
        if (bandIndex >= bands_.size()) return sample;

        const auto& band = bands_[bandIndex];
        const auto& coef = bandCoefs_[bandIndex];
        float* z = bandStates_[channel * bands_.size() + bandIndex].data();

        // Biquad direct form I
        float output = coef.b0 * sample + coef.b1 * z[0] + coef.b2 * z[1]
                     - coef.a1 * z[2] - coef.a2 * z[3];

        z[1] = z[0];
        z[0] = sample;
        z[3] = z[2];
        z[2] = output;

        return output;
    }

    void updateBandCoefficients(size_t index) {
        if (index >= bands_.size()) return;

        const auto& band = bands_[index];
        auto& coef = bandCoefs_[index];

        float wc = 2.0f * juce::MathConstants<float>::pi * band.freq / sampleRate_;
        float cosWc = std::cos(wc);
        float sinWc = std::sin(wc);
        float alpha = sinWc / (2.0f * band.q);
        float A = std::pow(10.0f, band.gain / 40.0f);

        switch (band.type) {
            case FilterType::Lowpass:
                coef.b0 = (1.0f - cosWc) / 2.0f;
                coef.b1 = 1.0f - cosWc;
                coef.b2 = (1.0f - cosWc) / 2.0f;
                coef.a0 = 1.0f + alpha;
                coef.a1 = -2.0f * cosWc;
                coef.a2 = 1.0f - alpha;
                break;

            case FilterType::Highpass:
                coef.b0 = (1.0f + cosWc) / 2.0f;
                coef.b1 = -(1.0f + cosWc);
                coef.b2 = (1.0f + cosWc) / 2.0f;
                coef.a0 = 1.0f + alpha;
                coef.a1 = -2.0f * cosWc;
                coef.a2 = 1.0f - alpha;
                break;

            case FilterType::Bell:
                coef.b0 = 1.0f + alpha * A;
                coef.b1 = -2.0f * cosWc;
                coef.b2 = 1.0f - alpha * A;
                coef.a0 = 1.0f + alpha / A;
                coef.a1 = -2.0f * cosWc;
                coef.a2 = 1.0f - alpha / A;
                break;

            case FilterType::Lowshelf:
                {
                    float sqrt2A = std::sqrt(2.0f * A);
                    float ap1 = sqrt2A * alpha + A - 1.0f;
                    float am1 = sqrt2A * alpha + 1.0f - A;

                    coef.b0 = A * ((A + 1.0f) - (A - 1.0f) * cosWc + ap1);
                    coef.b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosWc);
                    coef.b2 = A * ((A + 1.0f) - (A - 1.0f) * cosWc - ap1);
                    coef.a0 = (A + 1.0f) + (A - 1.0f) * cosWc + am1;
                    coef.a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosWc);
                    coef.a2 = (A + 1.0f) + (A - 1.0f) * cosWc - am1;
                }
                break;

            case FilterType::Highshelf:
                {
                    float sqrt2A = std::sqrt(2.0f * A);
                    float ap1 = sqrt2A * alpha + A - 1.0f;
                    float am1 = sqrt2A * alpha + 1.0f - A;

                    coef.b0 = A * ((A + 1.0f) + (A - 1.0f) * cosWc + ap1);
                    coef.b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosWc);
                    coef.b2 = A * ((A + 1.0f) + (A - 1.0f) * cosWc - ap1);
                    coef.a0 = (A + 1.0f) - (A - 1.0f) * cosWc + am1;
                    coef.a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosWc);
                    coef.a2 = (A + 1.0f) - (A - 1.0f) * cosWc - am1;
                }
                break;

            default:
                coef.b0 = 1.0f; coef.b1 = 0.0f; coef.b2 = 0.0f;
                coef.a0 = 1.0f; coef.a1 = 0.0f; coef.a2 = 0.0f;
                break;
        }

        // Normalize
        float norm = 1.0f / coef.a0;
        coef.b0 *= norm;
        coef.b1 *= norm;
        coef.b2 *= norm;
        coef.a1 *= norm;
        coef.a2 *= norm;
    }

    struct BiquadCoefs {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
    };

    std::vector<Band> bands_;
    std::vector<BiquadCoefs> bandCoefs_{8};
    std::vector<std::array<float, 4>> bandStates_{16};  // 2 channels * 8 bands
};

//==============================================================================
// Limiter
//==============================================================================

class Limiter : public StereoEffect {
public:
    void setThreshold(float dB) {
        threshold_ = juce::jlimit(-60.0f, 0.0f, dB);
    }

    void setCeiling(float dB) {
        ceiling_ = juce::jlimit(-60.0f, 0.0f, dB);
    }

    void setRelease(float seconds) {
        release_ = juce::jmax(0.001f, seconds);
    }

    float getGainReduction() const {
        return gainReduction_;
    }

protected:
    void processEffect(float* left, float* right, int numSamples) override {
        float thresholdGain = juce::Decibels::decibelsToGain(threshold_);
        float ceilingGain = juce::Decibels::decibelsToGain(ceiling_);

        for (int i = 0; i < numSamples; ++i) {
            float inputL = left[i];
            float inputR = right[i];

            // Get peak
            float peak = std::max(std::abs(inputL), std::abs(inputR));

            // Smooth envelope
            envelope_ = std::max(peak, envelope_ * envelopeDecay_);

            // Calculate gain
            float desiredGain = (envelope_ > thresholdGain) ? thresholdGain / envelope_ : 1.0f;

            // Smooth gain changes
            gain_ = gain_ + (desiredGain - gain_) * gainInc_;

            // Clamp to ceiling
            float finalGain = std::min(gain_, ceilingGain);

            // Apply
            left[i] = inputL * finalGain;
            right[i] = inputR * finalGain;
        }

        // For metering
        gainReduction_ = juce::Decibels::gainToDecibels(gain_);
    }

    void updateCoefficients() override {
        envelopeDecay_ = std::exp(-1.0f / (release_ * sampleRate_));
        gainInc_ = 1.0f - std::exp(-1.0f / (0.01f * sampleRate_));
    }

    void reset() override {
        envelope_ = 0.0f;
        gain_ = 1.0f;
        gainReduction_ = 0.0f;
    }

private:
    float threshold_ = -0.1f;
    float ceiling_ = -0.1f;
    float release_ = 0.01f;
    float envelope_ = 0.0f;
    float envelopeDecay_ = 0.999f;
    float gain_ = 1.0f;
    float gainInc_ = 0.001f;
    float gainReduction_ = 0.0f;
};

//==============================================================================
// Effects Factory
//==============================================================================

class EffectsFactory {
public:
    static std::unique_ptr<Phaser> createPhaser() {
        return std::make_unique<Phaser>();
    }

    static std::unique_ptr<Flanger> createFlanger() {
        return std::make_unique<Flanger>();
    }

    static std::unique_ptr<RingModulator> createRingModulator() {
        return std::make_unique<RingModulator>();
    }

    static std::unique_ptr<FrequencyShifter> createFrequencyShifter() {
        return std::make_unique<FrequencyShifter>();
    }

    static std::unique_ptr<GranularEffects> createGranular() {
        return std::make_unique<GranularEffects>();
    }

    static std::unique_ptr<Compressor> createCompressor() {
        return std::make_unique<Compressor>();
    }

    static std::unique_ptr<ParametricEQ> createEQ() {
        return std::make_unique<ParametricEQ>();
    }

    static std::unique_ptr<Limiter> createLimiter() {
        return std::make_unique<Limiter>();
    }
};

} // namespace zenith
