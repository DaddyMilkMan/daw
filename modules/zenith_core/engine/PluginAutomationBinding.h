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

#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
/**
 * @struct PluginParameterTarget
 * @brief Unique identifier for a plugin parameter within a track
 *
 * Format: "plugin_X_Y" where X = plugin index, Y = parameter index
 */
struct PluginParameterTarget {
    juce::String trackId;
    int pluginIndex = -1;
    int parameterIndex = -1;
    juce::String parameterName; // For display/serialization

    //==========================================================================
    /**
     * @brief Generate automation parameter ID string
     * @return Format: "plugin_X_Y"
     */
    juce::String toParamId() const {
        return "plugin_" + juce::String(pluginIndex) + "_" +
               juce::String(parameterIndex);
    }

    /**
     * @brief Parse parameter ID string into plugin/param indices
     * @param paramId Parameter ID in format "plugin_X_Y"
     * @param outPluginIndex Output: plugin index
     * @param outParamIndex Output: parameter index
     * @return true if successfully parsed as plugin parameter
     */
    static bool fromParamId(const juce::String& paramId, int& outPluginIndex,
                            int& outParamIndex) {
        if (!paramId.startsWith("plugin_"))
            return false;

        auto remainder = paramId.substring(7); // After "plugin_"
        auto underscorePos = remainder.indexOf("_");
        if (underscorePos < 0)
            return false;

        outPluginIndex = remainder.substring(0, underscorePos).getIntValue();
        outParamIndex = remainder.substring(underscorePos + 1).getIntValue();

        return outPluginIndex >= 0 && outParamIndex >= 0;
    }

    /**
     * @brief Check if this is a valid target
     */
    bool isValid() const {
        return pluginIndex >= 0 && parameterIndex >= 0;
    }

    bool operator==(const PluginParameterTarget& other) const {
        return trackId == other.trackId && pluginIndex == other.pluginIndex &&
               parameterIndex == other.parameterIndex;
    }
};

//==============================================================================
/**
 * @class PluginAutomationBinding
 * @brief Handles RT-safe parameter updates with smoothing
 *
 * This class bridges automation values to plugin parameters:
 * - Message thread writes target values (from TrackAutomationSynchronizer)
 * - Audio thread reads smoothed values (during processBlock)
 *
 * Uses juce::SmoothedValue for glitch-free automation transitions.
 */
class PluginAutomationBinding {
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param plugin Pointer to the plugin instance (must remain valid)
     * @param paramIndex Index of the parameter to automate
     * @param smoothingTimeMs Smoothing time in milliseconds (default 5ms)
     */
    PluginAutomationBinding(juce::AudioPluginInstance* plugin, int paramIndex,
                            float smoothingTimeMs = 5.0f);

    ~PluginAutomationBinding() = default;

    //==========================================================================
    /**
     * @brief Prepare for playback
     * @param sampleRate Current sample rate
     */
    void prepare(double sampleRate);

    /**
     * @brief Set target automation value (MESSAGE THREAD)
     * @param normalizedValue Value in range [0.0, 1.0]
     * @note Thread-safe, called from TrackAutomationSynchronizer timer
     */
    void setTargetValue(float normalizedValue);

    /**
     * @brief Apply smoothed automation value to plugin (AUDIO THREAD)
     * @param numSamples Number of samples in current block
     * @note Must be called at start of each processBlock
     */
    void applyAutomation(int numSamples);

    /**
     * @brief Get current smoothed value (RT-SAFE)
     * @return Current normalized value
     */
    float getCurrentValue() const;

    /**
     * @brief Get target value (RT-SAFE)
     * @return Target normalized value
     */
    float getTargetValue() const { return targetValue_.load(std::memory_order_acquire); }

    /**
     * @brief Check if automation is currently active
     * @return true if actively automating (target != current)
     */
    bool isActive() const { return active_.load(std::memory_order_acquire); }

    /**
     * @brief Enable/disable this automation binding
     */
    void setEnabled(bool enabled) { enabled_.store(enabled, std::memory_order_release); }
    bool isEnabled() const { return enabled_.load(std::memory_order_acquire); }

    //==========================================================================
    // Accessors
    //==========================================================================

    juce::AudioPluginInstance* getPlugin() const { return plugin_; }
    int getParameterIndex() const { return parameterIndex_; }

private:
    juce::AudioPluginInstance* plugin_;
    int parameterIndex_;
    float smoothingTimeMs_;

    // Thread-safe state
    std::atomic<float> targetValue_{0.0f};
    std::atomic<bool> active_{false};
    std::atomic<bool> enabled_{true};

    // Audio thread only
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedValue_;
    double currentSampleRate_ = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAutomationBinding)
};

} // namespace zenith
