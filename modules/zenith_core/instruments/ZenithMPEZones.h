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
#include <array>

namespace zenith {

//==============================================================================
// MPE ZONE CONFIGURATION
//==============================================================================
/**
 * Configuration for a single MPE zone
 */
struct MPEZoneConfig {
    //==========================================================================
    // Zone Range
    //==========================================================================

    int startNote = 0;              ///< Start of zone (0-127)
    int endNote = 127;               ///< End of zone (0-127)
    int startChannel = 0;            ///< Start master channel (0-15)
    int numChannels = 16;            ///< Number of member channels

    //==========================================================================
    // Pitch Bend
    //==========================================================================

    int pitchBendRange = 48;        ///< Bend range in semitones
    bool pitchBendSensitivity = true; ///< Use MPE sensitivity

    //==========================================================================
    // Pressure (Channel Aftertouch)
    //==========================================================================

    bool pressureEnabled = true;
    float pressureCurve = 1.0f;      ///< Curve shape (0.5-2.0)
    float pressureAmount = 1.0f;     ///< Depth amount (0-1)

    //==========================================================================
    // Timbre (Poly Aftertouch)
    //==========================================================================

    bool timbreEnabled = true;
    float timbreCurve = 1.0f;        ///< Curve shape
    float timbreAmount = 1.0f;       ///< Depth amount (0-1)
    juce::String timbreDestination;      ///< Which parameter to control

    //==========================================================================
    // Note Priority
    //==========================================================================

    enum class Priority {
        Lowest = 0,
        Highest,
        Newest,
        Oldest,
        Custom
    };

    Priority stealPriority = Priority::Lowest;

    MPEZoneConfig() = default;
};

//==============================================================================
// MPE ZONE MANAGER
//==============================================================================
/**
 * MPE zone management for professional MPE support
 *
 * FEATURES:
 * - Lower and upper zone configuration
 * - Per-zone pitch bend range
 * - Per-zone pressure/timbre response
 * - Note stealing priority per zone
 * - Zone overlap detection
 */
class ZenithMPEZoneManager {
public:
    ZenithMPEZoneManager();
    ~ZenithMPEZoneManager() = default;

    //==========================================================================
    // Zone Configuration
    //==========================================================================

    /**
     * @brief Set lower zone configuration
     */
    void setLowerZone(const MPEZoneConfig& config) {
        lowerZone_ = config;
        updateZones();
    }

    /**
     * @brief Set upper zone configuration
     */
    void setUpperZone(const MPEZoneConfig& config) {
        upperZone_ = config;
        updateZones();
    }

    /**
     * @brief Get lower zone
     */
    const MPEZoneConfig& getLowerZone() const { return lowerZone_; }

    /**
     * @brief Get upper zone
     */
    const MPEZoneConfig& getUpperZone() const { return upperZone_; }

    //==========================================================================
    // Zone Detection
    //==========================================================================

    /**
     * @brief Check if note is in lower zone
     */
    bool isInLowerZone(int note, int channel) const;

    /**
     * @brief Check if note is in upper zone
     */
    bool isInUpperZone(int note, int channel) const;

    /**
     * @brief Get zone index for note/channel
     * @return 0 = lower, 1 = upper, -1 = none
     */
    int getZoneForNote(int note, int channel) const;

    //==========================================================================
    // Priority Management
    //==========================================================================

    /**
     * @brief Check if voice should be stolen
     */
    bool shouldStealVoice(int zoneIndex, int newNote, int existingNote) const;

    //==========================================================================
    // MPE Settings
    //==========================================================================

    /**
     * @brief Enable/disable MPE mode
     */
    void setMPEEnabled(bool enabled) { mpeEnabled_ = enabled; }

    /**
     * @brief Is MPE enabled
     */
    bool isMPEEnabled() const { return mpeEnabled_; }

    //==========================================================================
    // Legacy MIDI
    //==========================================================================

    /**
     * @brief Enable legacy MIDI mode (single channel)
     */
    void setLegacyMIDI(bool enabled) { legacyMidi_ = enabled; }

    /**
     * @brief Is legacy MIDI mode
     */
    bool isLegacyMIDI() const { return legacyMidi_; }

private:
    MPEZoneConfig lowerZone_;
    MPEZoneConfig upperZone_;

    bool mpeEnabled_ = false;
    bool legacyMidi_ = false;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void updateZones();
    bool comparePriority(const MPEZoneConfig& config, int noteA, int noteB) const;
};

} // namespace zenith
