/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../engine/EffectProcessor.h"
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <array>

namespace zenith {
namespace effects {

//==============================================================================
/**
 * Professional Dynamic Equalizer
 *
 * Features:
 * - 4-8 bands of dynamic EQ
 * - Frequency-selective compression (sidechained per band)
 * - Per-band threshold, ratio, attack, release, knee, makeup gain
 * - Band linkage (couple bands together)
 * - Threshold overlay on spectrum
 * - Auto-threshold (learn from signal)
 * - External sidechain support per band
 * - Frequency graph with gain reduction metering
 *
 * Designed to compete with:
 * - FabFilter Pro-Q 3 Dynamic EQ ($249)
 * - Oxford Dynamic EQ ($299)
 * - iZotope Neutron 3 EQ ($129)
 */
class DynamicEQ : public engine::ParameterizedEffect
{
public:
    //==============================================================================
    DynamicEQ();
    ~DynamicEQ() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Dynamic EQ"; }
    engine::EffectType getType() const override { return engine::EffectType::Filter; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Band controls
    static constexpr int maxBands = 8;
    static constexpr int defaultBands = 4;

    struct DynamicBand
    {
        // Filter settings
        float frequency = 1000.0f;
        float q = 2.0f;
        bool enabled = true;
        bool solo = false;

        // Compressor settings
        float threshold = -20.0f;  // dB
        float ratio = 4.0f;         // 1:1 to inf:1
        float attack = 10.0f;        // ms
        float release = 100.0f;      // ms
        float knee = 6.0f;           // dB
        float makeup = 0.0f;         // dB

        // Sidechain source
        bool externalSidechain = false;
        int sidechainChannel = 0;     // Channel for external sidechain

        // Metering
        float gainReduction = 0.0f;
        float inputLevel = -100.0f;
        float outputLevel = -100.0f;

        // State
        bool autoThreshold = false;
        float learnedThreshold = -20.0f;
    };

    void setNumBands(int num);
    int getNumBands() const { return numBands_; }

    void setBand(int index, const DynamicBand& band);
    DynamicBand getBand(int index) const;

    // Per-band parameters
    void setBandFrequency(int band, float frequency);
    void setBandQ(int band, float q);
    void setBandEnabled(int band, bool enabled);
    void setBandSolo(int band, bool solo);

    // Compression parameters
    void setThreshold(int band, float dB);
    void setRatio(int band, float ratio);
    void setAttack(int band, float ms);
    void setRelease(int band, float ms);
    void setKnee(int band, float dB);
    void setMakeupGain(int band, float dB);

    // Sidechain
    void setExternalSidechain(int band, bool enabled);
    void setSidechainChannel(int band, int channel);

    // Auto-threshold
    void setAutoThresholdEnabled(int band, bool enabled);
    void learnThreshold(int band);
    bool isLearningThreshold(int band) const;

    // Metering
    float getGainReduction(int band) const;
    float getInputLevel(int band) const;
    float getOutputLevel(int band) const;
    float getBandLevel(int band, float frequency, float q) const;

    // Band linkage
    void setBandsLinked(bool linked);
    bool areBandsLinked() const { return bandsLinked_; }
    void setLinkGroup(int band, int group);  // 0-3 link groups

    // Global controls
    void setWetDryMix(float mix);  // 0.0 = dry, 1.0 = wet
    float getWetDryMix() const { return wetDryMix_.load(); }

    void setOutputGain(float dB);
    float getOutputGain() const { return outputGain_.load(); }

    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

private:
    //==============================================================================
    // Dynamic EQ processing
    void processBand(int bandIndex, juce::AudioBuffer<float>& buffer,
                     const juce::AudioBuffer<float>* sidechain);

    // Filter design
    void updateFilters(int bandIndex);

    // Calculate level in specific band
    float calculateBandLevel(const juce::AudioBuffer<float>& buffer,
                             int bandIndex);

    // Threshold learning
    void learnThresholdForBand(int band);

    //==============================================================================
    // Bands
    std::array<DynamicBand, maxBands> bands_;
    int numBands_ = defaultBands;

    // Per-band filters (for isolating frequency ranges)
    struct BandFilters
    {
        juce::dsp::ProcessorDuplicator<
            juce::dsp::IIR::Filter<float>,
            juce::dsp::IIR::Coefficients<float>> bandpass;

        juce::dsp::ProcessorDuplicator<
            juce::dsp::IIR::Filter<float>,
            juce::dsp::IIR::Coefficients<float>> sidechainFilter;
    };

    std::array<BandFilters, maxBands> bandFilters_;

    // Per-band compressors
    struct BandCompressor
    {
        juce::dsp::Compressor<float> compressor;
        juce::dsp::BallisticsFilter<float> envelopeFollower;
    };

    std::array<BandCompressor, maxBands> bandCompressors_;

    // Band buffers
    juce::AudioBuffer<float> bandBuffers_[maxBands];

    // Link groups (0-3, 4 bands per group)
    std::array<int, maxBands> linkGroups_{0, 0, 1, 1, 2, 2, 3, 3};
    bool bandsLinked_ = false;

    // Learning state
    std::array<bool, maxBands> learningThreshold_{false};

    // Global parameters
    std::atomic<float> wetDryMix_{1.0f};
    std::atomic<float> outputGain_{0.0f};

    // Sample rate and block size
    double sampleRate_ = 44100.0;
    int maxSamplesPerBlock_ = 512;

    //==============================================================================
    void setupDefaultBands();
    void updateAllFilters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicEQ)
};

//==============================================================================
/**
 * Factory for creating dynamic EQ
 */
class DynamicEQFactory
{
public:
    static std::unique_ptr<DynamicEQ> create()
    {
        return std::make_unique<DynamicEQ>();
    }
};

} // namespace effects
} // namespace zenith
