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
 * Professional Multiband Compressor
 *
 * Features:
 * - 4-band frequency splitter (Linkwitz-Riley crossover)
 * - Independent compression per band
 * - Band solo/mute for auditioning
 * - Per-band threshold, ratio, attack, release, knee, makeup gain
 * - Band linking option
 * - Wet/dry mix
 * - Gain reduction metering per band
 *
 * Designed to compete with:
 * - Logic Pro: Multiband Compressor
 * - Ableton Live: Multiband Dynamics (via Utility)
 * - FabFilter Pro-MB ($199)
 */
class MultibandCompressor : public engine::ParameterizedEffect
{
public:
    //==============================================================================
    MultibandCompressor();
    ~MultibandCompressor() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Multiband Compressor"; }
    engine::EffectType getType() const override { return engine::EffectType::Dynamics; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Band controls
    void setCrossoverFrequency(int crossoverIndex, float frequencyHz);
    float getCrossoverFrequency(int crossoverIndex) const;

    void setBandEnabled(int bandIndex, bool enabled);
    bool isBandEnabled(int bandIndex) const;

    void setBandSolo(int bandIndex, bool solo);
    bool isBandSolo(int bandIndex) const;

    void setBandMute(int bandIndex, bool mute);
    bool isBandMute(int bandIndex) const;

    // Per-band compression parameters
    void setThreshold(int bandIndex, float dB);
    float getThreshold(int bandIndex) const;

    void setRatio(int bandIndex, float ratio);
    float getRatio(int bandIndex) const;

    void setAttack(int bandIndex, float milliseconds);
    float getAttack(int bandIndex) const;

    void setRelease(int bandIndex, float milliseconds);
    float getRelease(int bandIndex) const;

    void setKnee(int bandIndex, float dB);
    float getKnee(int bandIndex) const;

    void setMakeupGain(int bandIndex, float dB);
    float getMakeupGain(int bandIndex) const;

    // Global controls
    void setWetDryMix(float mix);  // 0.0 = dry, 1.0 = wet
    float getWetDryMix() const { return wetDryMix_.load(); }

    void setOutputGain(float dB);
    float getOutputGain() const { return outputGain_.load(); }

    void setBandsLinked(bool linked);
    bool areBandsLinked() const { return bandsLinked_.load(); }

    // Metering
    float getGainReduction(int bandIndex) const;
    float getInputLevel(int bandIndex) const;
    float getOutputLevel(int bandIndex) const;

    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

private:
    //==============================================================================
    static constexpr int numBands = 4;
    static constexpr int numCrossovers = 3;  // 3 crossovers = 4 bands

    struct CompressorBand
    {
        // Parameters (atomic for thread safety)
        std::atomic<bool> enabled{true};
        std::atomic<bool> solo{false};
        std::atomic<bool> mute{false};

        std::atomic<float> threshold{-20.0f};  // dB
        std::atomic<float> ratio{4.0f};         // 1:1 to inf:1
        std::atomic<float> attack{10.0f};       // ms
        std::atomic<float> release{100.0f};     // ms
        std::atomic<float> knee{6.0f};          // dB
        std::atomic<float> makeupGain{0.0f};    // dB

        // Metering (non-atomic, updated in process thread)
        float gainReduction = 0.0f;
        float inputLevel = -100.0f;
        float outputLevel = -100.0f;

        // DSP
        juce::dsp::Compressor<float> compressor;
    };

    // Crossover (Linkwitz-Riley 4th order)
    struct Crossover
    {
        std::atomic<float> frequency{0.0f};  // Hz

        // Lowpass for lower band
        juce::dsp::ProcessorDuplicator<
            juce::dsp::IIR::Filter<float>,
            juce::dsp::IIR::Coefficients<float>> lowpass;

        // Highpass for higher band
        juce::dsp::ProcessorDuplicator<
            juce::dsp::IIR::Filter<float>,
            juce::dsp::IIR::Coefficients<float>> highpass;
    };

    std::array<CompressorBand, numBands> bands_;
    std::array<Crossover, numCrossovers> crossovers_;

    // Band buffers (for intermediate processing)
    juce::AudioBuffer<float> bandBuffers_[numBands];

    // Global parameters
    std::atomic<float> wetDryMix_{1.0f};
    std::atomic<float> outputGain_{0.0f};
    std::atomic<bool> bandsLinked_{false};

    // Dry signal (for parallel mixing)
    juce::AudioBuffer<float> dryBuffer_;

    // Sample rate and block size
    double sampleRate_ = 44100.0;
    int maxSamplesPerBlock_ = 512;

    //==============================================================================
    void updateCrossover(int crossoverIndex);
    void updateCompressor(int bandIndex);

    float calculateGainReduction(float inputLevel, const CompressorBand& band);

    // Calculate level in dB
    static float levelToDecibels(float level)
    {
        return juce::Decibels::gainToDecibels(level + 1e-6f);
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultibandCompressor)
};

//==============================================================================
/**
 * Factory for creating multiband compressor
 */
class MultibandCompressorFactory
{
public:
    static std::unique_ptr<MultibandCompressor> create()
    {
        return std::make_unique<MultibandCompressor>();
    }
};

} // namespace effects
} // namespace zenith
