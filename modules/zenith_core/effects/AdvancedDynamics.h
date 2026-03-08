/*
    AdvancedDynamics.h - Professional Dynamics Suite

    Complete dynamics processing featuring:
    - Adaptive Limiter (True Peak, mastering-grade)
    - Tube Compressor (Variable-Mu, Fairchild 670 style)
    - Bus Compressor (SSL G-Series style)

    Designed to compete with:
    - FabFilter Pro-L 2 ($199)
    - UAD Fairchild 670 ($299)
    - Waves SSL E-Channel ($299)
    - IK Multimedia T-RackS 4 ($149)

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#pragma once

#include "../engine/EffectProcessor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <array>

namespace zenith {
namespace engine {

//==============================================================================
// DYNAMICS TYPE ENUM
//==============================================================================

/**
 * @enum DynamicsType
 * @brief Professional dynamics processor types
 */
enum class DynamicsType
{
    //==========================================================================
    // Limiters
    //==========================================================================
    AdaptiveLimiter,     // True peak limiter (mastering)
    BrickwallLimiter,     // Zero overshoot limiter

    //==========================================================================
    // Compressors
    //==========================================================================
    TubeCompressor,       // Variable-Mu (Fairchild style)
    BusCompressor,        // SSL G-Series style
    FETCompressor,        // 1176 FET style
    VCACompressor,        // DBX 160 style
    OptoCompressor,       // LA-2A style

    //==========================================================================
    // Specialty
    //==========================================================================
    DeEsser,             // Sibilance reduction
    TransientShaper,     // Attack/sustain shaping
    ParallelCompressor   // NY-style compression
};

//==============================================================================
// PROFESSIONAL DYNAMICS SUITE
//==============================================================================

/**
 * @class AdvancedDynamics
 * @brief Complete professional dynamics processor
 *
 * Features:
 * - Adaptive Limiter with True Peak detection (4x oversampling)
 * - Tube Compressor with variable-mu circuit modeling
 * - Bus Compressor with SSL-style punch
 * - Multiple compressor types (FET, VCA, Opto)
 * - Sidechain filtering (HPF, LPF, bandpass)
 * - Parallel compression (NY compression)
 * - Advanced metering (GR, input/output, peak hold)
 * - Auto-release for mastering
 * - Channel linking (peak, average, unlink)
 *
 * Designed to compete with:
 * - FabFilter Pro-L 2 ($199)
 * - UAD Fairchild 670 ($299)
 * - Waves SSL E-Channel ($299)
 * - IK Multimedia T-RackS 4 ($149)
 */
class AdvancedDynamics : public EffectProcessor
{
public:
    //==============================================================================
    // Link mode for stereo linking
    enum class LinkMode {
        Peak,        // Link based on peak channel
        Average,     // Link based on average of both channels
        Left,        // Use left channel only
        Right,       // Use right channel only
        Mid,         // Link based on mid signal
        Side         // Link based on side signal
    };

    //==============================================================================
    AdvancedDynamics();
    ~AdvancedDynamics() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Advanced Dynamics"; }
    EffectType getType() const override { return EffectType::Dynamics; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Dynamics type
    void setDynamicsType(DynamicsType type);
    DynamicsType getDynamicsType() const { return dynamicsType_.load(); }

    //==============================================================================
    // Main compressor/limiter controls
    void setThreshold(float dB);  // -60dB to 0dB
    float getThreshold() const { return threshold_.load(); }

    void setRatio(float ratio);  // 1.0 to ∞ (20.0 for limiting)
    float getRatio() const { return ratio_.load(); }

    void setKnee(float dB);  // 0dB (hard) to 12dB (soft)
    float getKnee() const { return knee_.load(); }

    void setAttack(float ms);  // 0.01ms to 100ms
    float getAttack() const { return attack_.load(); }

    void setRelease(float ms);  // 1ms to 2000ms
    float getRelease() const { return release_.load(); }

    void setMakeupGain(float dB);  // -20dB to 20dB
    float getMakeupGain() const { return makeupGain_.load(); }

    void setWetDryMix(float mix);  // 0.0 to 1.0
    float getWetDryMix() const { return wetDryMix_.load(); }

    //==============================================================================
    // Limiter-specific controls
    void setCeiling(float dB);  // -20dB to 0dB (max output level)
    float getCeiling() const { return ceiling_.load(); }

    void setReleaseAuto(bool autoRelease);
    bool isReleaseAuto() const { return releaseAuto_.load(); }

    void setOversampling(int factor);  // 1x, 2x, 4x, 8x
    int getOversampling() const { return oversampling_.load(); }

    void setLinkMode(LinkMode mode);  // Peak, Average, Unlinked
    LinkMode getLinkMode() const { return linkMode_.load(); }

    //==============================================================================
    // Tube compressor-specific controls
    void setWarmth(float warmth);  // 0.0 to 1.0 (harmonic distortion)
    float getWarmth() const { return warmth_.load(); }

    void setProgramDependent(bool enabled);  // Program-dependent attack/release
    bool isProgramDependent() const { return programDependent_.load(); }

    //==============================================================================
    // Sidechain filtering
    enum class SidechainFilterType { Off, HighPass, LowPass, BandPass };

    void setSidechainFilterEnabled(bool enabled);
    bool isSidechainFilterEnabled() const { return sidechainFilterEnabled_.load(); }

    void setSidechainFilterType(SidechainFilterType type);
    SidechainFilterType getSidechainFilterType() const { return sidechainFilterType_.load(); }

    void setSidechainFilterFrequency(float Hz);
    float getSidechainFilterFrequency() const { return sidechainFilterFreq_.load(); }

    //==============================================================================
    // Parallel compression (NY compression)
    void setParallelEnabled(bool enabled);
    bool isParallelEnabled() const { return parallelEnabled_.load(); }

    void setParallelThreshold(float dB);  // Threshold for parallel path
    float getParallelThreshold() const { return parallelThreshold_.load(); }

    void setPunchDetection(bool enabled);  // Auto-detect transients
    bool isPunchDetectionEnabled() const { return punchDetection_.load(); }

    //==============================================================================
    // Metering
    float getGainReduction() const { return gainReduction_.load(); }
    float getInputLevel() const { return inputLevel_.load(); }
    float getOutputLevel() const { return outputLevel_.load(); }
    float getPeakLevel() const { return peakLevel_.load(); }

    //==============================================================================
    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

private:
    //==============================================================================
    // Processing methods
    float processCompressor(float input, int channel);
    float processLimiter(float input, int channel);
    float processTubeCompressor(float input, int channel);
    float processBusCompressor(float input, int channel);
    float processFETCompressor(float input, int channel);
    float processVCACompressor(float input, int channel);
    float processOptoCompressor(float input, int channel);
    float processParallelCompressor(float input, int channel);

    // Compression algorithms
    float computeGainReduction(float inputLevel, int channel);
    float applyKnee(float level, float threshold, float knee);
    float applyRatio(float overshoot, float ratio);

    // Tube compressor (variable-mu)
    float applyVariableMu(float input, float controlVoltage);
    float generateControlVoltage(float inputLevel);

    // Limiter (True Peak)
    float processTruePeak(float input, int channel);
    float oversampleAndLimit(float input);

    // Sidechain filtering
    float applySidechainFilter(float sample);

    // Metering
    void updateMetering(float input, float output, float gr);

    //==============================================================================
    // Parameters (atomic for thread safety)
    std::atomic<DynamicsType> dynamicsType_{DynamicsType::BusCompressor};
    std::atomic<float> threshold_{-20.0f};
    std::atomic<float> ratio_{4.0f};
    std::atomic<float> knee_{6.0f};
    std::atomic<float> attack_{10.0f};
    std::atomic<float> release_{100.0f};
    std::atomic<float> makeupGain_{0.0f};
    std::atomic<float> wetDryMix_{1.0f};

    // Limiter parameters
    std::atomic<float> ceiling_{-0.1f};
    std::atomic<bool> releaseAuto_{false};
    std::atomic<int> oversampling_{4};  // 4x for True Peak
    std::atomic<LinkMode> linkMode_{LinkMode::Peak};

    // Tube compressor parameters
    std::atomic<float> warmth_{0.3f};
    std::atomic<bool> programDependent_{true};

    // Sidechain filter
    std::atomic<bool> sidechainFilterEnabled_{false};
    std::atomic<SidechainFilterType> sidechainFilterType_{SidechainFilterType::HighPass};
    std::atomic<float> sidechainFilterFreq_{150.0f};

    // Parallel compression
    std::atomic<bool> parallelEnabled_{false};
    std::atomic<float> parallelThreshold_{-20.0f};
    std::atomic<bool> punchDetection_{false};

    // Metering
    std::atomic<float> gainReduction_{0.0f};
    std::atomic<float> inputLevel_{-100.0f};
    std::atomic<float> outputLevel_{-100.0f};
    std::atomic<float> peakLevel_{-100.0f};

    //==============================================================================
    // DSP components

    // Per-channel envelopes (for compression)
    struct EnvelopeFollower
    {
        float envelope = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;

        void setAttack(float ms, double sampleRate)
        {
            attackCoeff = std::exp(-1.0f / (sampleRate * ms / 1000.0));
        }

        void setRelease(float ms, double sampleRate)
        {
            releaseCoeff = std::exp(-1.0f / (sampleRate * ms / 1000.0));
        }

        float process(float input)
        {
            float target = std::abs(input);
            if (target > envelope)
                envelope = target + (envelope - target) * attackCoeff;
            else
                envelope = target + (envelope - target) * releaseCoeff;
            return envelope;
        }

        void reset() { envelope = 0.0f; }
    };

    std::array<EnvelopeFollower, 2> envelopeFollowers_;

    // Sidechain filter
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> sidechainFilter_;

    // Oversampler for True Peak limiter
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler_;

    // Gain smoothing
    struct GainSmoother
    {
        float currentGain = 1.0f;
        float targetGain = 1.0f;
        float smoothingCoeff = 0.0f;

        void setSmoothing(float ms, double sampleRate)
        {
            smoothingCoeff = std::exp(-1.0f / (sampleRate * ms / 1000.0));
        }

        float process(float target)
        {
            targetGain = target;
            currentGain = targetGain + (currentGain - targetGain) * smoothingCoeff;
            return currentGain;
        }

        void reset() { currentGain = 1.0f; targetGain = 1.0f; }
    };

    std::array<GainSmoother, 2> gainSmoothers_;

    // Peak hold for metering
    struct PeakHolder
    {
        float peak = 0.0f;
        float holdTime = 0.0f;
        int holdCounter = 0;

        float process(float input, int sampleRate)
        {
            float absInput = std::abs(input);
            if (absInput > peak)
            {
                peak = absInput;
                holdCounter = static_cast<int>(sampleRate * holdTime / 1000.0);
            }
            else if (holdCounter > 0)
            {
                holdCounter--;
            }
            else
            {
                peak *= 0.999f;  // Slow decay
            }
            return peak;
        }

        void reset() { peak = 0.0f; holdCounter = 0; }
    };

    PeakHolder peakHolder_;

    // Sample rate
    double sampleRate_ = 44100.0;
    int maxSamplesPerBlock_ = 512;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedDynamics)
};

} // namespace engine
} // namespace zenith
