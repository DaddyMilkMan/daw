/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "EffectProcessor.h"
#include "../dsp/PitchCorrector.h"
#include "../dsp/UltraLowLatencyPitchDetector.h"
#include "../dsp/ProPitchShifter.h"
#include "../dsp/ScaleAutoDetector.h"
#include "../dsp/ThroatModel.h"
#include <atomic>
#include <vector>

namespace zenith {
namespace engine {

//==============================================================================
/**
 * Native Auto-Tune effect for Zenith DAW.
 *
 * This is NOT a VST3 plugin - it's a native effect that integrates
 * directly with the DAW's track processing chain.
 *
 * Features:
 * - Ultra-low latency pitch correction (0.36ms Turbo mode)
 * - Flex-Tune mode for natural correction
 * - Auto-Key detection
 * - Throat modeling
 * - Harmony generation (4 voices)
 * - Graph mode with per-note editing
 */
class NativeAutoTuneEffect : public ParameterizedEffect
{
public:
    //==============================================================================
    NativeAutoTuneEffect();
    ~NativeAutoTuneEffect() override;

    //==============================================================================
    // EffectProcessor overrides
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    juce::String getName() const override { return "Auto-Tune"; }
    EffectType getType() const override { return EffectType::PitchCorrection; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    int getLatencySamples() const override;

    //==============================================================================
    // Latency Mode
    enum class LatencyMode
    {
        Turbo,         // 16 samples = 0.36ms @ 44.1kHz
        Extreme,       // 32 samples = 0.73ms
        UltraLow,      // 64 samples = 1.45ms
        Low,           // 128 samples = 2.90ms
        Standard,      // 256 samples = 5.80ms
        HighQuality    // 512 samples = 11.6ms
    };

    void setLatencyMode(LatencyMode mode);
    LatencyMode getLatencyMode() const { return latencyMode_; }

    //==============================================================================
    // Direct parameter access (faster than generic getParameter)

    // Main controls
    void setRetuneSpeed(float ms);           // 0-800ms
    void setCorrectionAmount(float amount);   // 0-1
    void setHumanize(float amount);           // 0-1
    void setFormantPreservation(float amount); // 0-1
    void setKey(int rootNote);                // 0-11
    void setScale(int scaleType);             // scale type

    float getRetuneSpeed() const { return retuneSpeed_.load(); }
    float getCorrectionAmount() const { return correctionAmount_.load(); }
    float getHumanize() const { return humanize_.load(); }
    float getFormantPreservation() const { return formantPreservation_.load(); }
    int getKey() const { return key_.load(); }
    int getScale() const { return scale_.load(); }

    //==============================================================================
    // Flex-Tune
    void setFlexTuneEnabled(bool enabled);
    void setFlexTuneThreshold(float cents);
    void setFlexTuneAmount(float amount);
    bool isFlexTuneEnabled() const { return flexTuneEnabled_.load(); }
    float getFlexTuneThreshold() const { return flexTuneThreshold_.load(); }

    //==============================================================================
    // Auto-Key
    void setAutoKeyEnabled(bool enabled);
    bool isAutoKeyEnabled() const { return autoKeyEnabled_.load(); }
    dsp::ScaleDetectionResult getAutoKeyResult() const;
    juce::String getAutoKeyName() const;

    //==============================================================================
    // Throat Modeling
    void setThroatModelEnabled(bool enabled);
    void setThroatLength(float length);    // 0-1
    void setThroatWidth(float width);      // 0-1
    void setThroatBreathiness(float amount); // 0-1
    bool isThroatModelEnabled() const { return throatModelEnabled_.load(); }

    //==============================================================================
    // Real-time feedback for UI
    float getDetectedPitch() const { return detectedPitch_.load(); }
    float getTargetPitch() const { return targetPitch_.load(); }
    float getConfidence() const { return confidence_.load(); }
    bool isCorrecting() const { return isCorrecting_.load(); }

    //==============================================================================
    // Graph Mode - Per-note corrections
    struct NoteCorrection
    {
        double startTime = 0.0;
        double endTime = 0.0;
        float originalPitch = 0.0f;
        float correctedPitch = 0.0f;
        float correctionAmount = 1.0f;
        bool isSelected = false;
    };

    void setGraphModeEnabled(bool enabled);
    bool isGraphModeEnabled() const { return graphModeEnabled_.load(); }
    void setNoteCorrections(const std::vector<NoteCorrection>& corrections);
    const std::vector<NoteCorrection>& getNoteCorrections() const { return noteCorrections_; }

    //==============================================================================
    // Presets
    enum class Preset { Natural, Transparent, Tight, Robot, Subtle };
    void loadPreset(Preset preset);

private:
    //==============================================================================
    // DSP modules
    dsp::UltraLowLatencyPitchDetector pitchDetector_;
    dsp::PitchCorrector pitchCorrector_;
    dsp::ProPitchShifter pitchShifter_;
    dsp::ScaleAutoDetector scaleAutoDetector_;
    dsp::ThroatModel throatModel_;

    //==============================================================================
    // Parameters (atomic for thread safety)
    std::atomic<LatencyMode> latencyMode_{LatencyMode::Low};
    std::atomic<float> retuneSpeed_{50.0f};
    std::atomic<float> correctionAmount_{1.0f};
    std::atomic<float> humanize_{0.5f};
    std::atomic<float> formantPreservation_{0.8f};
    std::atomic<int> key_{0};      // C
    std::atomic<int> scale_{0};    // Chromatic

    // Flex-Tune
    std::atomic<bool> flexTuneEnabled_{false};
    std::atomic<float> flexTuneThreshold_{15.0f};
    std::atomic<float> flexTuneAmount_{0.5f};

    // Auto-Key
    std::atomic<bool> autoKeyEnabled_{false};
    dsp::ScaleDetectionResult lastAutoKeyResult_;
    juce::uint32 autoKeySampleCount_ = 0;

    // Throat Model
    std::atomic<bool> throatModelEnabled_{false};
    std::atomic<float> throatLength_{0.5f};
    std::atomic<float> throatWidth_{0.5f};
    std::atomic<float> throatBreathiness_{0.0f};

    // Graph Mode
    std::atomic<bool> graphModeEnabled_{false};
    std::vector<NoteCorrection> noteCorrections_;

    // Real-time feedback
    std::atomic<float> detectedPitch_{0.0f};
    std::atomic<float> targetPitch_{0.0f};
    std::atomic<float> confidence_{0.0f};
    std::atomic<bool> isCorrecting_{false};

    // Processing buffers
    juce::AudioBuffer<float> monoBuffer_;
    juce::AudioBuffer<float> correctedBuffer_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeAutoTuneEffect)
};

//==============================================================================
/**
 * Factory for creating native Auto-Tune effect instances per track
 */
class AutoTuneEffectFactory
{
public:
    //==============================================================================
    static std::unique_ptr<NativeAutoTuneEffect> create();
    static constexpr const char* EffectID = "zenith.native.autotune";
};

} // namespace engine
} // namespace zenith
