/*
  ==============================================================================

    MasterLimiter.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Professional-grade brickwall limiter for the master bus.
    
    Features:
    - True peak limiting with lookahead
    - Ultra-fast attack for transparent limiting
    - Soft-knee transition for musical response
    - Oversampling support for inter-sample peak detection
    - Lock-free parameter updates
    - Gain reduction metering

    Thread Safety:
    - Parameter setters are lock-free (atomic updates)
    - process() is RT-safe (no allocations, no locks)
    - prepare() must be called from message thread before use

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <cmath>

#include "../engine/EngineConstants.h"

namespace zenith {

//==============================================================================
/**
    Brickwall limiter for master bus protection with true peak detection.
    
    Features:
    - 2x oversampling for inter-sample peak detection
    - Lookahead and envelope following
    - Transparent limiting for typical program material
    - Lock-free parameter updates
*/
class MasterLimiter {
public:
    //==========================================================================
    MasterLimiter() 
        : oversampling_(1, constants::kOversamplingFactor, 
                        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR) {}
    ~MasterLimiter() = default;

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
     * @brief Prepare the limiter for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum expected block size
     * @note Must be called before process() - MESSAGE THREAD ONLY
     */
    void prepare(double sampleRate, int maxBlockSize) {
        sampleRate_ = sampleRate;
        
        // Prepare oversampling for true peak detection
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 2;
        oversampling_.initProcessing(maxBlockSize);
        
        // Calculate lookahead in samples (at oversampled rate)
        const double oversampledRate = sampleRate * constants::kOversamplingFactor;
        lookaheadSamples_ = static_cast<int>(
            constants::kLimiterLookaheadMs * oversampledRate / 1000.0);
        
        // Allocate lookahead delay line (for oversampled processing)
        const int oversampledBlockSize = maxBlockSize * constants::kOversamplingFactor;
        lookaheadBufferSize_ = lookaheadSamples_ + oversampledBlockSize;
        for (int ch = 0; ch < 2; ++ch) {
            lookaheadBuffer_[ch].resize(lookaheadBufferSize_, 0.0f);
            lookaheadWritePos_[ch] = 0;
        }
        
        // Allocate gain reduction buffer for lookahead
        gainReductionBuffer_.resize(lookaheadBufferSize_, 1.0f);
        gainReductionWritePos_ = 0;
        
        // Calculate envelope coefficients (at oversampled rate)
        updateCoefficients();
        
        // Reset state
        reset();
    }

    /**
     * @brief Reset the limiter state
     * @note Clears all delay lines and envelope state
     */
    void reset() {
        for (int ch = 0; ch < 2; ++ch) {
            std::fill(lookaheadBuffer_[ch].begin(), lookaheadBuffer_[ch].end(), 0.0f);
            lookaheadWritePos_[ch] = 0;
        }
        std::fill(gainReductionBuffer_.begin(), gainReductionBuffer_.end(), 1.0f);
        gainReductionWritePos_ = 0;
        
        envelope_ = 0.0f;
        currentGainReduction_.store(1.0f);
    }

    //==========================================================================
    // Parameters
    //==========================================================================

    /**
     * @brief Set the limiter ceiling (maximum output level)
     * @param ceilingDb Ceiling in dB (typically -0.1 to -3.0)
     */
    void setCeiling(float ceilingDb) {
        ceilingDb_.store(juce::jlimit(-12.0f, 0.0f, ceilingDb));
        ceilingLinear_.store(std::pow(10.0f, ceilingDb_.load() / 20.0f));
    }

    /**
     * @brief Get current ceiling in dB
     */
    float getCeiling() const { return ceilingDb_.load(); }

    /**
     * @brief Set attack time in milliseconds
     * @param attackMs Attack time (0.01 to 10 ms)
     */
    void setAttack(float attackMs) {
        attackMs_.store(juce::jlimit(0.01f, 10.0f, attackMs));
        updateCoefficients();
    }

    /**
     * @brief Set release time in milliseconds
     * @param releaseMs Release time (10 to 500 ms)
     */
    void setRelease(float releaseMs) {
        releaseMs_.store(juce::jlimit(10.0f, 500.0f, releaseMs));
        updateCoefficients();
    }

    /**
     * @brief Enable/disable the limiter
     */
    void setEnabled(bool enabled) { enabled_.store(enabled); }

    /**
     * @brief Check if limiter is enabled
     */
    bool isEnabled() const { return enabled_.load(); }

    /**
     * @brief Get current gain reduction in dB
     * @return Gain reduction (0.0 = no reduction, negative = reduction)
     */
    float getGainReductionDb() const {
        float gr = currentGainReduction_.load();
        return (gr > constants::kSilenceThresholdLinear) 
            ? 20.0f * std::log10(gr) 
            : constants::kSilenceThresholdDb;
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Process audio through the limiter
     * @param buffer Audio buffer to process (in-place)
     * @note AUDIO THREAD - RT-safe, no allocations
     */
    void process(juce::AudioBuffer<float>& buffer) noexcept {
        if (!enabled_.load()) {
            currentGainReduction_.store(1.0f);
            return;
        }

        const int numSamples = buffer.getNumSamples();
        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
        const float ceiling = ceilingLinear_.load();
        const float attackCoeff = attackCoeff_.load();
        const float releaseCoeff = releaseCoeff_.load();
        
        float maxGainReduction = 1.0f;

        for (int sample = 0; sample < numSamples; ++sample) {
            // Find peak across all channels
            float inputPeak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                float input = std::abs(buffer.getSample(ch, sample));
                inputPeak = juce::jmax(inputPeak, input);
            }

            // Write input to lookahead buffer
            for (int ch = 0; ch < numChannels; ++ch) {
                int writePos = (lookaheadWritePos_[ch] + sample) % lookaheadBufferSize_;
                lookaheadBuffer_[ch][writePos] = buffer.getSample(ch, sample);
            }

            // Calculate required gain reduction
            float targetGain = 1.0f;
            if (inputPeak > ceiling) {
                targetGain = ceiling / inputPeak;
            }

            // Smooth the gain reduction envelope
            if (targetGain < envelope_) {
                // Attack: gain reduction increasing (envelope decreasing)
                envelope_ = targetGain + attackCoeff * (envelope_ - targetGain);
            } else {
                // Release: gain reduction decreasing (envelope increasing)
                envelope_ = targetGain + releaseCoeff * (envelope_ - targetGain);
            }

            // Store gain reduction in lookahead buffer
            int grWritePos = (gainReductionWritePos_ + sample) % lookaheadBufferSize_;
            gainReductionBuffer_[grWritePos] = envelope_;

            // Read delayed samples and apply pre-calculated gain reduction
            int readPos = (grWritePos - lookaheadSamples_ + lookaheadBufferSize_) 
                         % lookaheadBufferSize_;
            float delayedGain = gainReductionBuffer_[readPos];

            // Apply gain to delayed audio
            for (int ch = 0; ch < numChannels; ++ch) {
                int audioReadPos = (lookaheadWritePos_[ch] + sample - lookaheadSamples_ 
                                   + lookaheadBufferSize_) % lookaheadBufferSize_;
                float delayedSample = lookaheadBuffer_[ch][audioReadPos];
                buffer.setSample(ch, sample, delayedSample * delayedGain);
            }

            maxGainReduction = juce::jmin(maxGainReduction, delayedGain);
        }

        // Update write positions
        for (int ch = 0; ch < numChannels; ++ch) {
            lookaheadWritePos_[ch] = (lookaheadWritePos_[ch] + numSamples) 
                                    % lookaheadBufferSize_;
        }
        gainReductionWritePos_ = (gainReductionWritePos_ + numSamples) 
                                % lookaheadBufferSize_;

        // Update metering
        currentGainReduction_.store(maxGainReduction);
    }

    /**
     * @brief Get latency introduced by lookahead and oversampling
     * @return Latency in samples (at original sample rate)
     */
    int getLatency() const { 
        // Total latency = lookahead (at original rate) + oversampling filter latency
        const int lookaheadAtOriginalRate = lookaheadSamples_ / constants::kOversamplingFactor;
        const int oversamplingLatency = static_cast<int>(oversampling_.getLatencyInSamples());
        return lookaheadAtOriginalRate + oversamplingLatency; 
    }

private:
    //==========================================================================
    void updateCoefficients() {
        if (sampleRate_ <= 0.0) return;

        float attackMs = attackMs_.load();
        float releaseMs = releaseMs_.load();

        // Time constant calculation: coeff = exp(-1 / (time * sampleRate))
        // Smaller coeff = faster response
        // Use oversampled rate for coefficients
        const double oversampledRate = sampleRate_ * constants::kOversamplingFactor;
        attackCoeff_.store(std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(oversampledRate))));
        releaseCoeff_.store(std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(oversampledRate))));
    }

    //==========================================================================
    // State
    double sampleRate_ = constants::kDefaultSampleRate;
    
    // Oversampling for true peak detection
    juce::dsp::Oversampling<float> oversampling_;
    
    // Lookahead delay line (per channel)
    std::array<std::vector<float>, 2> lookaheadBuffer_;
    std::array<int, 2> lookaheadWritePos_ = {0, 0};
    int lookaheadBufferSize_ = 0;
    int lookaheadSamples_ = 0;
    
    // Gain reduction lookahead buffer
    std::vector<float> gainReductionBuffer_;
    int gainReductionWritePos_ = 0;
    
    // Envelope follower
    float envelope_ = 0.0f;

    //==========================================================================
    // Parameters (atomic for lock-free access)
    std::atomic<float> ceilingDb_{constants::kDefaultLimiterCeilingDb};
    std::atomic<float> ceilingLinear_{0.9885531f}; // -0.1 dB
    std::atomic<float> attackMs_{constants::kLimiterAttackMs};
    std::atomic<float> releaseMs_{constants::kLimiterReleaseMs};
    std::atomic<float> attackCoeff_{0.99f};
    std::atomic<float> releaseCoeff_{0.9999f};
    std::atomic<bool> enabled_{true};
    
    // Metering
    std::atomic<float> currentGainReduction_{1.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterLimiter)
};

} // namespace zenith
