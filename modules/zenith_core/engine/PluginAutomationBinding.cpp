/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/


    PluginAutomationBinding.cpp
    Created: 2025-12-25
    Author:  Zenith DAW

    Plugin parameter automation binding implementation.

  ==============================================================================

*/

#include "PluginAutomationBinding.h"

namespace zenith {

//==============================================================================
PluginAutomationBinding::PluginAutomationBinding(juce::AudioPluginInstance* plugin,
                                                   int paramIndex,
                                                   float smoothingTimeMs)
    : plugin_(plugin),
      parameterIndex_(paramIndex),
      smoothingTimeMs_(smoothingTimeMs) {
    jassert(plugin_ != nullptr);
    jassert(parameterIndex_ >= 0);

    // Initialize with current parameter value
    if (parameterIndex_ < plugin_->getParameters().size()) {
        float currentValue = plugin_->getParameters()[parameterIndex_]->getValue();
        targetValue_.store(currentValue, std::memory_order_release);
        smoothedValue_.setCurrentAndTargetValue(currentValue);
    }
}

//==============================================================================
void PluginAutomationBinding::prepare(double sampleRate) {
    currentSampleRate_ = sampleRate;

    // Calculate smoothing in samples: smoothingTimeMs * sampleRate / 1000
    double smoothingSamples = (smoothingTimeMs_ / 1000.0) * sampleRate;
    smoothedValue_.reset(sampleRate, smoothingTimeMs_ / 1000.0);

    DBG("PluginAutomationBinding: Prepared with " +
        juce::String(smoothingSamples, 1) + " samples of smoothing");
}

//==============================================================================
void PluginAutomationBinding::setTargetValue(float normalizedValue) {
    // Clamp to valid range
    normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);

    float oldTarget = targetValue_.load(std::memory_order_acquire);

    // Only update if value has changed meaningfully
    if (std::abs(normalizedValue - oldTarget) > 0.0001f) {
        targetValue_.store(normalizedValue, std::memory_order_release);
        active_.store(true, std::memory_order_release);
    }
}

//==============================================================================
void PluginAutomationBinding::applyAutomation(int numSamples) {
    if (!enabled_.load(std::memory_order_acquire))
        return;

    if (plugin_ == nullptr)
        return;

    auto& params = plugin_->getParameters();
    if (parameterIndex_ < 0 || parameterIndex_ >= params.size())
        return;

    // Get target value atomically
    float target = targetValue_.load(std::memory_order_acquire);

    // Update smoothed value target
    smoothedValue_.setTargetValue(target);

    // Process smoothing
    if (smoothedValue_.isSmoothing()) {
        // Skip ahead by numSamples
        smoothedValue_.skip(numSamples);
    }

    // Get final smoothed value
    float smoothedVal = smoothedValue_.getCurrentValue();

    // Apply to plugin parameter
    // Using setValueNotifyingHost ensures plugin UI updates
    params[parameterIndex_]->setValue(smoothedVal);

    // Check if we've reached target
    if (!smoothedValue_.isSmoothing()) {
        active_.store(false, std::memory_order_release);
    }
}

//==============================================================================
float PluginAutomationBinding::getCurrentValue() const {
    return smoothedValue_.getCurrentValue();
}

} // namespace zenith
