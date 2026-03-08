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
#include <vector>

namespace zenith {
namespace effects {

//==============================================================================
/**
 * Professional Convolution Reverb
 *
 * Features:
 * - Impulse response (IR) loading (WAV, AIFF)
 * - Built-in IR library (Hall, Room, Plate, Chamber, Special)
 * - Pre-delay (0-200ms)
 * - Decay time (10% - 200% of IR length)
 * - Size (0% - 200%)
 * - High-cut damping
 * - Low-cut damping
 * - Early reflections mix
 * - Wet reverb mix
 * - Stereo width
 * - Reverse IR capability
 * - EQ for reverb tail (3-band)
 *
 * Designed to compete with:
 * - Logic Pro: Space Designer
 * - Ableton Live: Hybrid Reverb (convolution section)
 * - Valhalla VintageVerb ($50)
 * - Lexicon PCM Native Reverb ($299)
 */
class ConvolutionReverb : public engine::ParameterizedEffect
{
public:
    //==============================================================================
    ConvolutionReverb();
    ~ConvolutionReverb() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Convolution Reverb"; }
    engine::EffectType getType() const override { return engine::EffectType::Reverb; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Impulse Response management
    bool loadImpulseResponse(const juce::File& file);
    bool loadImpulseResponse(const juce::String& builtinName);
    void loadImpulseResponseFromMemory(const juce::String& name,
                                      const std::vector<float>& data,
                                      int sampleRate);

    juce::String getCurrentImpulseName() const;
    int getCurrentImpulseLength() const;  // in samples
    float getCurrentImpulseLengthSeconds() const;
    int getCurrentImpulseSampleRate() const;

    // Built-in impulse responses
    static juce::StringArray getBuiltinImpulseNames();
    static juce::StringArray getImpulseCategories();

    //==============================================================================
    // Controls
    void setPreDelay(float milliseconds);
    float getPreDelay() const { return preDelay_.load(); }

    void setDecayTime(float percentage);  // 10% - 200% of IR length
    float getDecayTime() const { return decayTime_.load(); }

    void setSize(float percentage);  // 0% - 200%
    float getSize() const { return size_.load(); }

    void setDensity(float percentage);  // 0% - 100%
    float getDensity() const { return density_.load(); }

    void setHighCutDamping(float frequencyHz);  // High-cut for tail
    float getHighCutDamping() const { return highCutDamping_.load(); }

    void setLowCutDamping(float frequencyHz);  // Low-cut for tail
    float getLowCutDamping() const { return lowCutDamping_.load(); }

    void setEarlyReflectionsMix(float mix);  // 0.0 - 1.0
    float getEarlyReflectionsMix() const { return earlyReflectionsMix_.load(); }

    void setWetMix(float mix);  // 0.0 - 1.0
    float getWetMix() const { return wetMix_.load(); }

    void setStereoWidth(float width);  // 0.0 (mono) - 1.0 (stereo) - 2.0 (super stereo)
    float getStereoWidth() const { return stereoWidth_.load(); }

    void setReverseEnabled(bool enabled);
    bool isReverseEnabled() const { return reverseEnabled_.load(); }

    //==============================================================================
    // EQ for reverb tail
    void setEQLow(float dB);
    float getEQLow() const { return eqLowParam_.load(); }

    void setEQMid(float dB);
    float getEQMid() const { return eqMidParam_.load(); }

    void setEQHigh(float dB);
    float getEQHigh() const { return eqHighParam_.load(); }

    //==============================================================================
    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

private:
    //==============================================================================
    // Convolution engine
    juce::dsp::Convolution convolution_;

    // Pre-delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayLine_;

    // EQ for tail
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> eqLow_;
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> eqMid_;
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> eqHigh_;

    // Damping filters
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> highCutDampingFilter_;
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> lowCutDampingFilter_;

    // Wet/dry buffer
    juce::AudioBuffer<float> wetBuffer_;
    juce::AudioBuffer<float> earlyReflectionsBuffer_;

    // Current impulse response info
    juce::String currentImpulseName_;
    int currentImpulseLength_ = 0;
    int currentImpulseSampleRate_ = 44100;

    // Parameters (atomic for thread safety)
    std::atomic<float> preDelay_{20.0f};          // ms
    std::atomic<float> decayTime_{100.0f};        // percentage
    std::atomic<float> size_{100.0f};             // percentage
    std::atomic<float> density_{100.0f};          // percentage
    std::atomic<float> highCutDamping_{10000.0f}; // Hz
    std::atomic<float> lowCutDamping_{100.0f};    // Hz
    std::atomic<float> earlyReflectionsMix_{0.3f};
    std::atomic<float> wetMix_{0.5f};
    std::atomic<float> stereoWidth_{1.0f};
    std::atomic<bool> reverseEnabled_{false};

    // EQ parameters (stored separately from the filter objects)
    std::atomic<float> eqLowParam_{0.0f};   // dB
    std::atomic<float> eqMidParam_{0.0f};   // dB
    std::atomic<float> eqHighParam_{0.0f};  // dB

    // Sample rate and block size
    double sampleRate_ = 44100.0;
    int maxSamplesPerBlock_ = 512;
    int maxPreDelaySamples_ = 0;

    //==============================================================================
    void updateFilters();
    void applyStereoWidth(juce::AudioBuffer<float>& buffer);

    // Built-in impulse responses (procedurally generated)
    struct ImpulseResponseInfo {
        juce::String name;
        juce::String category;
        float decayTime;  // seconds
        float preDelay;   // seconds
        int sampleRate;
    };

    std::vector<float> generateBuiltinImpulse(const ImpulseResponseInfo& info);
    std::vector<float> generateHallImpulse(float decayTime, int sampleRate);
    std::vector<float> generateRoomImpulse(float decayTime, int sampleRate);
    std::vector<float> generatePlateImpulse(float decayTime, int sampleRate);
    std::vector<float> generateChamberImpulse(float decayTime, int sampleRate);
    std::vector<float> generateSpringImpulse(float decayTime, int sampleRate);
    std::vector<float> generateReverseImpulse(float decayTime, int sampleRate);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionReverb)
};

//==============================================================================
/**
 * Factory for creating convolution reverb
 */
class ConvolutionReverbFactory
{
public:
    static std::unique_ptr<ConvolutionReverb> create()
    {
        return std::make_unique<ConvolutionReverb>();
    }
};

} // namespace effects
} // namespace zenith
