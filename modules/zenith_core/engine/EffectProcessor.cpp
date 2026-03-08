/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "EffectProcessor.h"
#include <cmath>

namespace zenith {
namespace engine {

//==============================================================================
// EffectProcessor
//==============================================================================
EffectProcessor::EffectProcessor()
{
}

EffectProcessor::~EffectProcessor()
{
}

//==============================================================================
// EffectParameter
//==============================================================================
void EffectParameter::updateSmoothing(float sampleRate, int numSamples)
{
    if (!isAutomatable || smoothingTime <= 0.0f)
    {
        smoothedValue = *value;
        isSmoothing = false;
        return;
    }

    // Calculate smoothing coefficient
    float T = (smoothingTime / 1000.0f) * sampleRate;
    float alpha = (T > 0.0f) ? (1.0f - std::exp(-static_cast<float>(numSamples) / T)) : 1.0f;

    // Smooth towards target
    float diff = *value - smoothedValue;
    if (std::abs(diff) < 0.0001f)
    {
        smoothedValue = *value;
        isSmoothing = false;
    }
    else
    {
        smoothedValue += alpha * diff;
        isSmoothing = true;
    }
}

void EffectParameter::setValueDirectly(float newValue)
{
    if (value)
        *value = juce::jlimit(minValue, maxValue, newValue);
    smoothedValue = newValue;
    isSmoothing = false;
}

//==============================================================================
// ParameterizedEffect
//==============================================================================
ParameterizedEffect::ParameterizedEffect()
{
}

ParameterizedEffect::~ParameterizedEffect()
{
}

EffectParameter* ParameterizedEffect::getParameter(const juce::String& ID)
{
    for (auto& param : parameters_)
    {
        if (param.ID == ID)
            return &param;
    }
    return nullptr;
}

const EffectParameter* ParameterizedEffect::getParameter(const juce::String& ID) const
{
    for (auto& param : parameters_)
    {
        if (param.ID == ID)
            return &param;
    }
    return nullptr;
}

void ParameterizedEffect::setParameter(const juce::String& ID, float newValue)
{
    if (auto* param = getParameter(ID))
    {
        param->setValueDirectly(juce::jlimit(param->minValue, param->maxValue, newValue));
    }
}

void ParameterizedEffect::updateSmoothedParameters(int numSamples)
{
    for (auto& param : parameters_)
    {
        param.updateSmoothing(static_cast<float>(sampleRate_), numSamples);
    }
}

void ParameterizedEffect::addParameter(EffectParameter&& param)
{
    param.smoothedValue = *param.value;
    parameters_.push_back(std::move(param));
}

} // namespace engine
} // namespace zenith
