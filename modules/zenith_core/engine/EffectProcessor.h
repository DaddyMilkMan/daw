/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>
#include <vector>

namespace zenith {
namespace engine {

//==============================================================================
/**
 * Effect type enumeration for categorization
 */
enum class EffectType {
    PitchCorrection,
    EQ,
    Dynamics,
    Modulation,
    Delay,
    Reverb,
    Distortion,
    Filter,
    Utility,
    Other
};

//==============================================================================
/**
 * Base class for native DAW effects (NOT VST3 plugins).
 *
 * This is for effects that are built into the DAW, not external plugins.
 * They integrate directly with the track processing chain without plugin wrapper overhead.
 */
class EffectProcessor
{
public:
    //==============================================================================
    EffectProcessor();
    virtual ~EffectProcessor();

    //==============================================================================
    /**
     * @brief Prepare for processing
     */
    virtual void prepare(double sampleRate, int maxSamplesPerBlock) = 0;

    /**
     * @brief Reset internal state
     */
    virtual void reset() = 0;

    /**
     * @brief Process audio through the effect
     * @param buffer Input/output buffer
     * @param sidechain Optional sidechain input
     */
    virtual void process(juce::AudioBuffer<float>& buffer,
                        const juce::AudioBuffer<float>* sidechain = nullptr) = 0;

    //==============================================================================
    /**
     * @brief Enable/disable bypass
     */
    void setBypass(bool bypass) { bypassed_ = bypass; }
    bool isBypassed() const { return bypassed_.load(); }

    /**
     * @brief Get effect name
     */
    virtual juce::String getName() const = 0;

    /**
     * @brief Get effect type for categorization
     */
    virtual EffectType getType() const = 0;

    //==============================================================================
    /**
     * @brief Save/Load effect state
     */
    virtual juce::ValueTree getState() const = 0;
    virtual void setState(const juce::ValueTree& state) = 0;

    //==============================================================================
    /**
     * @brief Get latency in samples introduced by this effect
     */
    virtual int getLatencySamples() const { return 0; }

    /**
     * @brief Get tail length in seconds
     */
    virtual float getTailLengthSeconds() const { return 0.0f; }

protected:
    std::atomic<bool> bypassed_{false};
    double sampleRate_ = 44100.0;
    int maxSamplesPerBlock_ = 512;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectProcessor)
};

//==============================================================================
/**
 * Parameter for native effects with smoothing
 */
struct EffectParameter
{
    juce::String ID;
    juce::String name;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.5f;
    std::atomic<float>* value = nullptr;
    bool isAutomatable = true;
    float smoothingTime = 0.0f;  // ms

    // Smoothed value state
    float smoothedValue = 0.0f;
    bool isSmoothing = false;

    void updateSmoothing(float sampleRate, int numSamples);
    void setValueDirectly(float newValue);
};

//==============================================================================
/**
 * Base class for effects with parameters
 */
class ParameterizedEffect : public EffectProcessor
{
public:
    //==============================================================================
    ParameterizedEffect();
    ~ParameterizedEffect() override;

    //==============================================================================
    /**
     * @brief Get all parameters
     */
    const std::vector<EffectParameter>& getParameters() const { return parameters_; }

    /**
     * @brief Get parameter by ID
     */
    EffectParameter* getParameter(const juce::String& ID);
    const EffectParameter* getParameter(const juce::String& ID) const;

    /**
     * @brief Update parameter with smoothing
     */
    void setParameter(const juce::String& ID, float value);

    /**
     * @brief Update smoothed parameters each block
     */
    void updateSmoothedParameters(int numSamples);

protected:
    //==============================================================================
    /**
     * @brief Register a parameter
     */
    void addParameter(EffectParameter&& param);

    std::vector<EffectParameter> parameters_;
};

} // namespace engine
} // namespace zenith
