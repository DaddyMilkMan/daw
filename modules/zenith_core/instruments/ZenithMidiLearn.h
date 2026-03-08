/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <functional>
#include <unordered_map>

namespace zenith {

//==============================================================================
// MIDI MAPPING ENTRY
//==============================================================================
/**
 * Single MIDI CC mapping
 */
struct MidiMapping {
    int ccNumber = -1;                  ///< CC number (-1 = unmapped)
    int paramIndex = -1;                ///< Parameter index to control
    float minValue = 0.0f;               ///< Minimum value
    float maxValue = 1.0f;               ///< Maximum value
    bool invert = false;                   ///< Invert the mapping
    float smoothing = 0.0f;                ///< Smoothing time (seconds)

    //==========================================================================
    // Learn Mode
    //==========================================================================

    bool isLearning() const { return isLearning_; }
    void setLearning(bool learning) { isLearning_ = learning; }

private:
    bool isLearning_ = false;
};

//==============================================================================
// MIDI LEARN MANAGER
//==============================================================================
/**
 * Professional MIDI learn system matching Serum 2
 *
 * FEATURES:
 * - Learn CC assignments by moving controls
 * - Visual feedback during learning
 * - Pickup/safe takeover
 * - Multiple mappings per CC
 * - Preset save/load of mappings
 */
class ZenithMidiLearnManager {
public:
    //==========================================================================
    // Callback Types
    //==========================================================================

    using LearnCallback = std::function<void(int ccNumber, float value)>;
    using LearnStartCallback = std::function<void(int paramIndex)>;
    using LearnCompleteCallback = std::function<void(int paramIndex, int ccNumber)>;

    ZenithMidiLearnManager();
    ~ZenithMidiLearnManager() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set learn mode timeout (seconds)
     */
    void setLearnTimeout(float seconds) {
        learnTimeout_ = juce::jlimit(1.0f, 30.0f, seconds);
    }

    /**
     * @brief Enable/disable pickup mode
     */
    void setPickupMode(bool enabled) {
        pickupMode_ = enabled;
    }

    //==========================================================================
    // Learn Mode Control
    //==========================================================================

    /**
     * @brief Start learn mode for a parameter
     * @param paramIndex Parameter index to learn
     * @return True if learning started
     */
    bool startLearn(int paramIndex);

    /**
     * @brief Cancel current learn
     */
    void cancelLearn();

    /**
     * @brief Check if currently learning
     */
    bool isLearning() const { return learningParamIndex_ >= 0; }

    /**
     * @brief Get current learning parameter index
     */
    int getLearningParamIndex() const { return learningParamIndex_; }

    //==========================================================================
    // MIDI Processing
    //==========================================================================

    /**
     * @brief Process incoming MIDI CC
     * @param ccNumber CC number (0-127)
     * @param ccValue CC value (0-127)
     * @return True if CC was mapped to a parameter
     */
    bool processMidiCC(int ccNumber, int ccValue);

    //==========================================================================
    // Mapping Management
    //==========================================================================

    /**
     * @brief Add a mapping
     */
    void addMapping(const MidiMapping& mapping);

    /**
     * @brief Remove a mapping
     */
    void removeMapping(int ccNumber);

    /**
     * @brief Clear all mappings
     */
    void clearAllMappings();

    /**
     * @brief Get mapping for CC number
     */
    const MidiMapping* getMapping(int ccNumber) const;

    /**
     * @brief Get all mappings
     */
    const std::unordered_map<int, MidiMapping>& getAllMappings() const {
        return mappings_;
    }

    //==========================================================================
    // Value Retrieval
    //==========================================================================

    /**
     * @brief Get mapped parameter value
     * @param paramIndex Parameter index
     * @return Mapped value (or 0 if not mapped)
     */
    float getParamValue(int paramIndex) const;

    //==========================================================================
    // Callbacks
    //==========================================================================

    void setLearnCallback(LearnCallback cb) { learnCallback_ = cb; }
    void setLearnStartCallback(LearnStartCallback cb) { learnStartCallback_ = cb; }
    void setLearnCompleteCallback(LearnCompleteCallback cb) { learnCompleteCallback_ = cb; }

private:
    //==========================================================================
    // State
    //==========================================================================

    std::unordered_map<int, MidiMapping> mappings_;
    int learningParamIndex_ = -1;
    double learnStartTime_ = 0.0;
    float learnTimeout_ = 10.0f;
    bool pickupMode_ = true;

    //==========================================================================
    // Pickup State
    //==========================================================================

    struct PickupState {
        float lastValue = 0.0f;
        bool detected = false;
    };
    std::array<PickupState, 128> pickupStates_;

    //==========================================================================
    // Callbacks
    //==========================================================================

    LearnCallback learnCallback_;
    LearnStartCallback learnStartCallback_;
    LearnCompleteCallback learnCompleteCallback_;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    float normalizeCCValue(int ccValue) const;
    void timeoutLearn();
};

//==============================================================================
// PARAMETER DESCRIPTION FOR MIDI LEARN
//==============================================================================
struct ParameterInfo {
    int index;
    const char* name;
    float minValue;
    float maxValue;
    float defaultValue;
    bool isBipolar;

    ParameterInfo() = default;
    ParameterInfo(int idx, const char* n, float minV, float maxV, float defV, bool bipolar)
        : index(idx), name(n), minValue(minV), maxValue(maxV)
        , defaultValue(defV), isBipolar(bipolar) {}
};

} // namespace zenith
