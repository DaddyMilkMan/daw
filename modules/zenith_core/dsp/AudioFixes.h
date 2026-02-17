/*
    Critical Audio Bug Fixes for Zenith Ultra Synth
    Fixes for zipper noise, clicks, denormals, and aliasing
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <atomic>

namespace zenith {

//==============================================================================
// Denormal Protection
//==============================================================================

class DenormalProtection {
public:
    // Flush denormals to zero (SSE/NEON)
    static inline void flushDenormals() {
       #if JUCE_INTEL
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
       #elif JUCE_ARM
        // ARM doesn't have direct denormal control but we can add bias
       #endif
    }

    // Add DC offset to prevent denormals
    static inline float addDenormalOffset(float value) {
        return value + 1e-20f;
    }

    // Remove DC offset after processing
    static inline float removeDenormalOffset(float value) {
        return value - 1e-20f;
    }

    // Check if value is denormal or near-zero
    static inline bool isDenormal(float value) {
        return std::fpclassify(value) == FP_SUBNORMAL;
    }

    // Safe value with denormal check
    static inline float safeFloat(float value) {
        if (isDenormal(value) || std::isnan(value) || std::isinf(value)) {
            return 0.0f;
        }
        return value;
    }
};

//==============================================================================
// Smoothed Parameter (eliminates zipper noise)
//==============================================================================

class SmoothedParameter {
public:
    SmoothedParameter() = default;

    explicit SmoothedParameter(float sampleRate, float smoothingTimeMs = 20.0f)
        : sampleRate_(sampleRate) {
        setSmoothingTime(smoothingTimeMs);
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        calculateCoefficient();
    }

    void setSmoothingTime(float timeMs) {
        smoothingTimeMs_ = timeMs;
        calculateCoefficient();
    }

    void setTargetValue(float target) {
        target_ = target;
        if (immediateMode_) {
            current_ = target;
        }
    }

    float getNextValue() {
        // One-pole lowpass smoothing
        current_ += coefficient_ * (target_ - current_);

        // Prevent denormals
        return DenormalProtection::safeFloat(current_);
    }

    float getCurrentValue() const {
        return current_;
    }

    float getTargetValue() const {
        return target_;
    }

    bool isSmoothing() const {
        return std::abs(current_ - target_) > 0.0001f;
    }

    void resetToValue(float value) {
        current_ = value;
        target_ = value;
    }

    void setImmediateMode(bool immediate) {
        immediateMode_ = immediate;
    }

private:
    void calculateCoefficient() {
        if (sampleRate_ > 0.0f && smoothingTimeMs_ > 0.0f) {
            float timeSeconds = smoothingTimeMs_ * 0.001f;
            coefficient_ = 1.0f - std::exp(-1.0f / (timeSeconds * sampleRate_));
        } else {
            coefficient_ = 1.0f;
        }
    }

    float current_ = 0.0f;
    float target_ = 0.0f;
    float coefficient_ = 1.0f;
    float sampleRate_ = 44100.0f;
    float smoothingTimeMs_ = 20.0f;
    bool immediateMode_ = false;
};

//==============================================================================
// Click-Free Envelope
//==============================================================================

class ClickFreeEnvelope {
public:
    ClickFreeEnvelope() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        minAttackTime_ = 20.0f / sampleRate;  // Minimum 20 samples
        minReleaseTime_ = 20.0f / sampleRate;
    }

    void setAttack(float attackSeconds) {
        attackTime_ = std::max(attackSeconds, minAttackTime_);
        calculateAttackCoefficient();
    }

    void setDecay(float decaySeconds) {
        decayTime_ = std::max(decaySeconds, 0.001f);
        calculateDecayCoefficient();
    }

    void setSustain(float sustainLevel) {
        sustainLevel_ = juce::jlimit(0.0f, 1.0f, sustainLevel);
    }

    void setRelease(float releaseSeconds) {
        releaseTime_ = std::max(releaseSeconds, minReleaseTime_);
        calculateReleaseCoefficient();
    }

    void setAttackCurve(int curveType) {
        attackCurve_ = curveType;
    }

    void setDecayCurve(int curveType) {
        decayCurve_ = curveType;
    }

    void setReleaseCurve(int curveType) {
        releaseCurve_ = curveType;
    }

    void noteOn() {
        state_ = State::Attack;
        currentLevel_ = 0.0f;
        // Start from non-zero to avoid click
        if (previousLevel_ > 0.0001f) {
            currentLevel_ = previousLevel_ * 0.5f;
        }
    }

    void noteOff() {
        state_ = State::Release;
        releaseStartLevel_ = currentLevel_;
    }

    float getNextSample() {
        float increment = 0.0f;

        switch (state_) {
            case State::Idle:
                currentLevel_ = 0.0f;
                break;

            case State::Attack: {
                increment = attackCoefficient_;
                float target = 1.0f;

                // Apply curve
                if (attackCurve_ == 1) {  // Exponential
                    float remaining = 1.0f - currentLevel_;
                    currentLevel_ += remaining * increment;
                } else if (attackCurve_ == 2) {  // Logarithmic
                    float logLevel = std::log(1.0f + currentLevel_);
                    logLevel += increment * 0.5f;
                    currentLevel_ = std::exp(logLevel) - 1.0f;
                    currentLevel_ = std::min(currentLevel_, 1.0f);
                } else {  // Linear
                    currentLevel_ += increment;
                }

                if (currentLevel_ >= 0.999f) {
                    currentLevel_ = 1.0f;
                    state_ = State::Decay;
                }
                break;
            }

            case State::Decay: {
                float sustainDelta = currentLevel_ - sustainLevel_;
                currentLevel_ -= sustainDelta * decayCoefficient_;

                if (std::abs(currentLevel_ - sustainLevel_) < 0.001f) {
                    currentLevel_ = sustainLevel_;
                    state_ = State::Sustain;
                }
                break;
            }

            case State::Sustain:
                currentLevel_ = sustainLevel_;
                break;

            case State::Release: {
                float releaseDelta = currentLevel_;
                currentLevel_ -= releaseDelta * releaseCoefficient_;

                if (currentLevel_ <= 0.001f) {
                    currentLevel_ = 0.0f;
                    state_ = State::Idle;
                }
                break;
            }
        }

        previousLevel_ = currentLevel_;
        return DenormalProtection::safeFloat(currentLevel_);
    }

    bool isActive() const {
        return state_ != State::Idle;
    }

    float getCurrentLevel() const {
        return currentLevel_;
    }

    void reset() {
        state_ = State::Idle;
        currentLevel_ = 0.0f;
        previousLevel_ = 0.0f;
    }

private:
    enum class State { Idle, Attack, Decay, Sustain, Release };

    void calculateAttackCoefficient() {
        float samples = attackTime_ * sampleRate_;
        if (samples > 0.0f) {
            attackCoefficient_ = 1.0f / samples;
        } else {
            attackCoefficient_ = 1.0f;
        }
    }

    void calculateDecayCoefficient() {
        float samples = decayTime_ * sampleRate_;
        if (samples > 0.0f) {
            decayCoefficient_ = 1.0f / samples;
        } else {
            decayCoefficient_ = 1.0f;
        }
    }

    void calculateReleaseCoefficient() {
        float samples = releaseTime_ * sampleRate_;
        if (samples > 0.0f) {
            releaseCoefficient_ = 1.0f / samples;
        } else {
            releaseCoefficient_ = 1.0f;
        }
    }

    State state_ = State::Idle;
    float currentLevel_ = 0.0f;
    float previousLevel_ = 0.0f;
    float releaseStartLevel_ = 0.0f;

    float sampleRate_ = 44100.0f;
    float minAttackTime_ = 0.0005f;
    float minReleaseTime_ = 0.0005f;

    float attackTime_ = 0.01f;
    float decayTime_ = 0.3f;
    float sustainLevel_ = 0.7f;
    float releaseTime_ = 0.5f;

    int attackCurve_ = 1;   // 0=linear, 1=exponential, 2=log
    int decayCurve_ = 1;
    int releaseCurve_ = 1;

    float attackCoefficient_ = 0.001f;
    float decayCoefficient_ = 0.001f;
    float releaseCoefficient_ = 0.001f;
};

//==============================================================================
// Smooth PWM (click-free pulse width modulation)
//==============================================================================

class SmoothPWM {
public:
    SmoothPWM() = default;

    void setSampleRate(float sampleRate) {
        pwmSmoother.setSampleRate(sampleRate);
    }

    void setPulseWidth(float width) {
        // Clamp to valid range
        width = juce::jlimit(0.01f, 0.99f, width);

        // Only update if significant change
        if (std::abs(width - pwmSmoother.getTargetValue()) > 0.001f) {
            pwmSmoother.setTargetValue(width);
        }
    }

    float getCurrentPulseWidth() {
        return pwmSmoother.getNextValue();
    }

    float generatePulse(float phase) {
        float pw = pwmSmoother.getNextValue();

        // Soft transition to avoid click
        if (phase < pw) {
            return 1.0f;
        } else {
            // Soft knee at transition
            float edgeDist = std::abs(phase - pw);
            if (edgeDist < 0.01f) {
                // Smooth crossfade
                return 1.0f - (edgeDist / 0.01f) * 0.5f;
            }
            return -1.0f;
        }
    }

private:
    SmoothedParameter pwmSmoother{44100.0f, 10.0f};
};

//==============================================================================
// Anti-Aliasing Filter
//==============================================================================

class AntiAliasingFilter {
public:
    AntiAliasingFilter() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        // Set cutoff to Nyquist * 0.45
        setCutoff(sampleRate * 0.45f);
    }

    void setCutoff(float cutoff) {
        cutoff_ = juce::min(cutoff, sampleRate_ * 0.49f);
        calculateCoefficients();
    }

    void setResonance(float resonance) {
        resonance_ = juce::jlimit(0.0f, 1.0f, resonance);
        calculateCoefficients();
    }

    float processSample(float input) {
        // One-pole lowpass for anti-aliasing
        float output = z1_ + coefficient_ * (input - z1_);
        z1_ = output;

        return DenormalProtection::safeFloat(output);
    }

    void reset() {
        z1_ = 0.0f;
    }

private:
    void calculateCoefficients() {
        float wc = 2.0f * juce::MathConstants<float>::pi * cutoff_ / sampleRate_;
        coefficient_ = wc / (1.0f + wc);
    }

    float sampleRate_ = 44100.0f;
    float cutoff_ = 18000.0f;
    float resonance_ = 0.0f;
    float coefficient_ = 0.9f;
    float z1_ = 0.0f;
};

//==============================================================================
// Oversampling for anti-aliasing
//==============================================================================

class Oversampler {
public:
    enum class Factor {
        None = 1,
        x2 = 2,
        x4 = 4,
        x8 = 8
    };

    Oversampler() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateBuffer();
    }

    void setFactor(Factor factor) {
        factor_ = factor;
        updateBuffer();
    }

    void upsample(float input) {
        int factor = static_cast<int>(factor_);

        if (factor == 1) {
            buffer_[0] = input;
            return;
        }

        // Zero-stuffing and filtering
        buffer_[0] = input;

        for (int i = 1; i < factor; ++i) {
            buffer_[i] = 0.0f;  // Zero-stuffing
        }

        // Apply interpolation filter
        applyInterpolationFilter();
    }

    float downsample() {
        int factor = static_cast<int>(factor_);

        if (factor == 1) {
            return buffer_[0];
        }

        // Apply anti-aliasing filter
        applyAntiAliasingFilter();

        // Decimate
        return buffer_[0];
    }

    float* getBuffer() {
        return buffer_.data();
    }

    int getBufferSize() const {
        return static_cast<int>(factor_);
    }

private:
    void updateBuffer() {
        int size = static_cast<int>(factor_);
        buffer_.resize(size);
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    void applyInterpolationFilter() {
        // Simple linear interpolation for now
        // Could be upgraded to polyphase FIR
        int factor = static_cast<int>(factor_);

        for (int i = 1; i < factor; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(factor);
            buffer_[i] = buffer_[0] * (1.0f - t);  // Simple interpolation
        }
    }

    void applyAntiAliasingFilter() {
        // Simple moving average
        int factor = static_cast<int>(factor_);
        float sum = 0.0f;

        for (int i = 0; i < factor; ++i) {
            sum += buffer_[i];
        }

        buffer_[0] = sum / factor;
    }

    float sampleRate_ = 44100.0f;
    Factor factor_ = Factor::None;
    std::vector<float> buffer_;
};

//==============================================================================
// Modulation Smoother (prevents zipper noise in LFO/modulation)
//==============================================================================

class ModulationSmoother {
public:
    ModulationSmoother() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        recalculateCoefficients();
    }

    void setSmoothingTime(float timeMs) {
        smoothingTimeMs_ = timeMs;
        recalculateCoefficients();
    }

    float process(float input) {
        // Symmetrical smoothing for bipolar modulation
        float error = input - smoothed_;
        smoothed_ += coefficient_ * error;

        return DenormalProtection::safeFloat(smoothed_);
    }

    void reset() {
        smoothed_ = 0.0f;
    }

private:
    void recalculateCoefficients() {
        if (sampleRate_ > 0.0f && smoothingTimeMs_ > 0.0f) {
            float timeSeconds = smoothingTimeMs_ * 0.001f;
            coefficient_ = 1.0f - std::exp(-1.0f / (timeSeconds * sampleRate_));
        } else {
            coefficient_ = 1.0f;
        }
    }

    float sampleRate_ = 44100.0f;
    float smoothingTimeMs_ = 5.0f;
    float coefficient_ = 0.01f;
    float smoothed_ = 0.0f;
};

//==============================================================================
// Pop-free filter transitions
//==============================================================================

class SmoothFilterTransition {
public:
    SmoothFilterTransition() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        cutoffSmoother.setSampleRate(sampleRate);
        resonanceSmoother.setSampleRate(sampleRate);
    }

    void setTargetCutoff(float cutoff) {
        cutoffSmoother.setTargetValue(cutoff);
    }

    void setTargetResonance(float resonance) {
        resonanceSmoother.setTargetValue(resonance);
    }

    float getCurrentCutoff() {
        return cutoffSmoother.getNextValue();
    }

    float getCurrentResonance() {
        return resonanceSmoother.getNextValue();
    }

    bool isSmoothing() {
        return cutoffSmoother.isSmoothing() || resonanceSmoother.isSmoothing();
    }

    void reset() {
        cutoffSmoother.resetToValue(1000.0f);
        resonanceSmoother.resetToValue(0.0f);
    }

private:
    float sampleRate_ = 44100.0f;
    SmoothedParameter cutoffSmoother{44100.0f, 5.0f};
    SmoothedParameter resonanceSmoother{44100.0f, 10.0f};
};

//==============================================================================
// Unified Audio Fix Processor
//==============================================================================

class AudioFixProcessor {
public:
    AudioFixProcessor() {
        setSampleRate(44100.0f);
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;

        // Enable denormal protection
        DenormalProtection::flushDenormals();

        // Update all processors
        for (auto& param : smoothedParams_) {
            param->setSampleRate(sampleRate);
        }

        envelope.setSampleRate(sampleRate);
        pwm.setSampleRate(sampleRate);
        aaFilter.setSampleRate(sampleRate);
        oversampler.setSampleRate(sampleRate);
        modSmoother.setSampleRate(sampleRate);
        filterTransition.setSampleRate(sampleRate);
    }

    // Get a smoothed parameter
    SmoothedParameter* getSmoothedParameter(size_t index) {
        if (index >= smoothedParams_.size()) {
            smoothedParams_.push_back(std::make_unique<SmoothedParameter>(sampleRate_));
        }
        return smoothedParams_[index].get();
    }

    // Processors
    ClickFreeEnvelope envelope;
    SmoothPWM pwm;
    AntiAliasingFilter aaFilter;
    Oversampler oversampler;
    ModulationSmoother modSmoother;
    SmoothFilterTransition filterTransition;

private:
    float sampleRate_ = 44100.0f;
    std::vector<std::unique_ptr<SmoothedParameter>> smoothedParams_;
};

} // namespace zenith
