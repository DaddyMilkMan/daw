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
#include <array>
#include <algorithm>

namespace zenith {

//==============================================================================
// ARPEGGIATOR - SERUM 2 STYLE
//==============================================================================

/**
 * Professional arpeggiator with:
 * - 8 patterns × 128 steps (matching Serum 2)
 * - Multiple pattern modes (Up, Down, Random, Chord)
 * - Octave range control (0-4 octaves)
 * - Gate and probability control
 * - Swing (0-50% groove)
 * - Sort direction (Original, Sorted, Inverted)
 * - Hold mode for sustain chords
 * - Latch mode for continuous playback
 * - Step probability (0-100%)
 * - Step velocity offset
 * - Step gate control
 * - Pattern evolution
 * - Real-time processing
 * - 128-note polyphony
 */
class ZenithArpeggiator {
public:
    ZenithArpeggiator();
    ~ZenithArpeggiator() = default;

    //==========================================================================
    // Pattern Types
    //==========================================================================

    enum class Pattern {
        Up,            ///< Ascending pattern
        Down,          ///< Descending pattern
        UpDown,        ///< Alternating up/down
        Random,         ///< Random note order
        Chord,         ///< Play all notes simultaneously
        AsPlayed,       ///< Preserve played order
        Order           ///< Custom order sequence
    };

    //==========================================================================
    // Configuration
    //==========================================================================

    void setPattern(int index, const Pattern& pattern) {
        patterns_[index] = pattern;
    }

    void setOctaveRange(int octaves) { octaveRange_ = juce::jlimit(0, 4, octaves); }
    void setGate(float gate) { gate_ = juce::jlimit(0.0f, 1.0f, gate); }
    void setSwing(float swing) { swing_ = juce::jlimit(0.0f, 0.5f, swing); }

    void setSortDirection(int sort) { sort_ = juce::jlimit(-1, 1, sort); }
    void setMode(PatternMode mode) { mode_ = mode; }
    void setClockMode(bool external) { externalClock_ = external; }
    void setHoldMode(bool hold) { holdMode_ = hold; }

    //==========================================================================
    // Latch Mode
    //==========================================================================

    /**
     * @brief Set latch mode - continues playing after note release
     * @param latch True to enable latch
     */
    void setLatchMode(bool latch) { latchMode_ = latch; }

    /**
     * @brief Check if latched
     */
    bool isLatched() const { return latched_; }

    //==========================================================================
    // Step Probability
    //==========================================================================

    /**
     * @brief Set probability for a specific step
     * @param step Step index (0-127)
     * @param probability Probability 0.0-1.0 (0-100%)
     */
    void setStepProbability(int step, float probability) {
        stepProbability_[juce::jlimit(0, MAX_STEPS - 1, step)] =
            juce::jlimit(0.0f, 1.0f, probability);
    }

    /**
     * @brief Get probability for a step
     */
    float getStepProbability(int step) const {
        return stepProbability_[juce::jlimit(0, MAX_STEPS - 1, step)];
    }

    //==========================================================================
    // Step Velocity Control
    //==========================================================================

    /**
     * @brief Set velocity offset for a specific step
     * @param step Step index (0-127)
     * @param offset Velocity offset in percentage (-1.0 to 1.0)
     */
    void setStepVelocity(int step, float offset) {
        stepVelocity_[juce::jlimit(0, MAX_STEPS - 1, step)] =
            juce::jlimit(-1.0f, 1.0f, offset);
    }

    /**
     * @brief Get velocity offset for a step
     */
    float getStepVelocity(int step) const {
        return stepVelocity_[juce::jlimit(0, MAX_STEPS - 1, step)];
    }

    //==========================================================================
    // Step Gate Control
    //==========================================================================

    /**
     * @brief Set gate length for a specific step
     * @param step Step index (0-127)
     * @param gateLength Gate length 0.0-1.0 (short to long)
     */
    void setStepGate(int step, float gateLength) {
        stepGate_[juce::jlimit(0, MAX_STEPS - 1, step)] =
            juce::jlimit(0.0f, 1.0f, gateLength);
    }

    /**
     * @brief Get gate length for a step
     */
    float getStepGate(int step) const {
        return stepGate_[juce::jlimit(0, MAX_STEPS - 1, step)];
    }

    //==========================================================================
    // Pattern Evolution
    //==========================================================================

    /**
     * @brief Enable pattern evolution
     * @param enable True to enable evolution
     */
    void setEvolutionEnabled(bool enable) { evolutionEnabled_ = enable; }

    /**
     * @brief Set evolution rate (how fast pattern mutates)
     * @param rate Evolution rate 0.0-1.0
     */
    void setEvolutionRate(float rate) {
        evolutionRate_ = juce::jlimit(0.0f, 1.0f, rate);
    }

    /**
     * @brief Set evolution intensity (how much steps can change)
     * @param intensity Intensity 0.0-1.0
     */
    void setEvolutionIntensity(float intensity) {
        evolutionIntensity_ = juce::jlimit(0.0f, 1.0f, intensity);
    }

    /**
     * @brief Trigger pattern evolution (call to evolve pattern)
     */
    void evolvePattern();

    //==========================================================================
    // Playback Control
    //==========================================================================

    void start();
    void stop();
    bool isPlaying() const { return playing_; }

    //==========================================================================
    // Note Management
    //==========================================================================

    /**
     * @brief Add note to arpeggio
     */
    void addNote(int note, float velocity = 100.0f);

    /**
     * @brief Remove note from arpeggio
     */
    void removeNote(int note);

    /**
     * @brief Clear all notes
     */
    void clearNotes();

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Process and get next output note
     * @param numSamples Number of samples to process
     * @return True if pattern wrapped
     */
    bool process(int numSamples);

    //==========================================================================
    // State Query
    //==========================================================================

    const Pattern& getPattern(int index) const { return patterns_[index]; }
    Pattern getCurrentPattern() const { return patterns_[currentPatternIndex_]; }
    int getCurrentPatternIndex() const { return currentPatternIndex_; }

    //==========================================================================
    // Internal State
    //==========================================================================

    bool playing_ = false;
    int currentPatternIndex_ = 0;
    int currentStep_ = 0;
    double phase_ = 0.0;
    int currentOutputNote_ = -1;      // Currently outputted note (-1 = none)
    float currentVelocity_ = 0.0f;    // Current velocity
    float gatePhase_ = 0.0f;           // Gate phase (0-1 for gating)

    double sampleRate_ = 44100.0;
    double bpm_ = 120.0f;                // Default BPM

    // Timing
    double samplesPerBeat = 0.0;
    double samplesPerStep = 0.0;
    double phaseInc = 0.0;

    //==========================================================================
    // Note State (128-notes)
    //==========================================================================

    static constexpr int MAX_NOTES = 128;
    std::array<bool, MAX_NOTES> noteActive_;
    std::array<float, MAX_NOTES> noteVelocities_;  // Velocity per note

    //==========================================================================
    // Pattern State
    //==========================================================================

    std::array<Pattern, MAX_PATTERNS> patterns_;

    //==========================================================================
    // Latch Mode State
    //==========================================================================

    bool latchMode_ = false;       // Latch mode enabled
    bool latched_ = false;         // Currently latched
    std::array<int, 128> latchedNotes_;  // Notes to continue playing
    int numLatchedNotes_ = 0;

    //==========================================================================
    // Step Probability State (128 steps)
    //==========================================================================

    static constexpr int MAX_STEPS = 128;
    std::array<float, MAX_STEPS> stepProbability_;  // 0.0-1.0 per step
    juce::Random probabilityRandom_;

    //==========================================================================
    // Step Velocity State
    //==========================================================================

    std::array<float, MAX_STEPS> stepVelocity_;  // Velocity offset per step

    //==========================================================================
    // Step Gate State
    //==========================================================================

    std::array<float, MAX_STEPS> stepGate_;  // Gate length per step
    std::array<float, MAX_STEPS> stepGatePhase_;  // Gate phase per step

    //==========================================================================
    // Pattern Evolution State
    //==========================================================================

    bool evolutionEnabled_ = false;
    float evolutionRate_ = 0.0f;
    float evolutionIntensity_ = 0.0f;
    float evolutionPhase_ = 0.0f;
    juce::Random evolutionRandom_;
    std::array<int, MAX_STEPS> evolvedPattern_;  // Original pattern backup
};

};

} // namespace zenith
