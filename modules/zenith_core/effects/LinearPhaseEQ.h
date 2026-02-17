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
 * Professional Linear Phase Equalizer
 *
 * Features:
 * - 16 bands of EQ
 * - Linear phase mode (zero phase distortion)
 * - Minimum phase mode (lower latency)
 * - Filter types: Low-cut, High-cut, Low-shelf, High-shelf, Bell, Bandpass, Notch
 * - Frequency: 20Hz - 20kHz
 * - Gain: ±24dB
 * - Q: 0.1 - 100
 * - Filter order: 512, 1024, 2048, 4096
 * - Spectrum analyzer overlay
 * - Band solo/mute
 * - EQ curve import/export
 * - Match EQ capability
 *
 * Designed to compete with:
 * - Logic Pro: Linear Phase EQ
 * - FabFilter Pro-Q 3 ($149)
 * - Waves Q-Equal ($99)
 */
class LinearPhaseEQ : public engine::ParameterizedEffect
{
public:
    //==============================================================================
    LinearPhaseEQ();
    ~LinearPhaseEQ() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Linear Phase EQ"; }
    engine::EffectType getType() const override { return engine::EffectType::Filter; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Phase mode
    enum class PhaseMode
    {
        Linear,     // Zero phase distortion, higher latency
        Minimum     // Lower latency, some phase shift
    };

    void setPhaseMode(PhaseMode mode);
    PhaseMode getPhaseMode() const { return phaseMode_; }

    void setFilterOrder(int order);  // 512, 1024, 2048, 4096
    int getFilterOrder() const { return filterOrder_; }

    //==============================================================================
    // Filter types (must be declared before Band struct)
    enum class FilterType
    {
        Lowcut,
        Highcut,
        Bell,
        Lowshelf,
        Highshelf,
        Bandpass,
        Notch
    };

    //==============================================================================
    // Band controls
    static constexpr int maxBands = 16;

    struct Band
    {
        FilterType type = FilterType::Bell;
        float frequency = 1000.0f;
        float gain = 0.0f;         // For bell/shelf
        float q = 1.0f;            // Bandwidth
        bool enabled = true;
        bool solo = false;
    };

    void setBand(int index, const Band& band);
    Band getBand(int index) const;

    void setNumBands(int num);
    int getNumBands() const { return numBands_; }

    void setBandEnabled(int index, bool enabled);
    bool isBandEnabled(int index) const;

    void setBandSolo(int index, bool solo);
    bool isBandSolo(int index) const;

    // Per-band parameters
    void setBandType(int index, FilterType type);
    void setBandFrequency(int index, float frequency);
    void setBandGain(int index, float gain);
    void setBandQ(int index, float q);

    //==============================================================================
    // Spectrum analyzer
    void setSpectrumAnalyzerEnabled(bool enabled);
    bool isSpectrumAnalyzerEnabled() const { return spectrumAnalyzerEnabled_; }

    const float* getSpectrumData() const { return spectrumData_.data(); }
    int getSpectrumSize() const { return static_cast<int>(spectrumData_.size()); }

    //==============================================================================
    // Match EQ (learn from reference track)
    void startMatchEQ();
    void stopMatchEQ();
    bool isMatchingEQ() const { return matchingEQ_; }

    void setMatchEQAmount(float amount);  // 0.0 - 1.0 (how much to match)
    float getMatchEQAmount() const { return matchEQAmount_; }

    //==============================================================================
    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

    //==============================================================================
    // EQ curve export/import
    juce::String exportEQCurve() const;  // XML format
    bool importEQCurve(const juce::String& xml);

private:
    //==============================================================================
    // Filter design
    void designFilters();
    void designLinearPhaseFilter();
    void designMinimumPhaseFilter();

    // FIR filter design using window method
    void designFIRFilter(int bandIndex);

    // Window functions for FIR design
    float kaiserWindow(float x, float beta);
    float blackmanWindow(float x);

    // Spectrum analysis
    void updateSpectrumAnalyzer(const juce::AudioBuffer<float>& buffer);

    // Match EQ processing
    void processMatchEQ(const juce::AudioBuffer<float>& buffer);
    void analyzeSpectrum(const juce::AudioBuffer<float>& buffer, float* spectrum);
    void calculateMatchCurve(const float* targetSpectrum, const float* currentSpectrum);

    //==============================================================================
    // Parameters
    std::atomic<bool> spectrumAnalyzerEnabled_{true};
    std::atomic<bool> matchingEQ_{false};
    std::atomic<float> matchEQAmount_{0.5f};

    // Phase mode and filter order
    PhaseMode phaseMode_ = PhaseMode::Linear;
    int filterOrder_ = 2048;  // Default FIR length

    // Bands
    std::array<Band, maxBands> bands_;
    int numBands_ = 8;

    // Default bands
    void setupDefaultBands();

    // FIR filters for each band
    struct FIRFilter
    {
        std::vector<float> coefficients;
        int delay = 0;  // Group delay for linear phase alignment
    };

    std::array<FIRFilter, maxBands> firFilters_;

    // Convolution for FIR filtering
    juce::dsp::Convolution convolution_;

    // Minimum phase filters (IIR cascades)
    struct IIRFilterCascade
    {
        juce::dsp::ProcessorDuplicator<
            juce::dsp::IIR::Filter<float>,
            juce::dsp::IIR::Coefficients<float>> filter;
    };

    std::array<IIRFilterCascade, maxBands> iirFilters_;

    // Spectrum analyzer
    static constexpr int spectrumSize = 512;
    std::array<float, spectrumSize> spectrumData_{};
    juce::dsp::FFT fft_{9};  // 2^9 = 512 points
    std::vector<float> fftBuffer_;

    // Match EQ state
    static constexpr int matchSpectrumSize = 256;
    std::array<float, matchSpectrumSize> targetSpectrum_{};
    std::array<float, matchSpectrumSize> matchedSpectrum_{};
    bool haveTargetSpectrum_ = false;

    // Sample rate and block size
    double sampleRate_ = 44100.0;
    int maxSamplesPerBlock_ = 512;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LinearPhaseEQ)
};

//==============================================================================
/**
 * Factory for creating linear phase EQ
 */
class LinearPhaseEQFactory
{
public:
    static std::unique_ptr<LinearPhaseEQ> create()
    {
        return std::make_unique<LinearPhaseEQ>();
    }
};

} // namespace effects
} // namespace zenith
