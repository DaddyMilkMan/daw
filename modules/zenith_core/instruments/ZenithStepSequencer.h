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

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace zenith {

//==============================================================================
// BUILT-IN SEQUENCER - SERUM 2 STYLE
//==============================================================================
/**
 * Professional step sequencer matching Serum 2:
 * - 16 steps x 8 rows = 128 step pattern
 * - Per-step velocity, gate, probability
 * - Groove/swing per row
 * - Pattern length (1-16 steps)
 * - Direction (forward, backward, random, ping-pong)
 * - Shuffle (rotate, random, none)
 * - MIDI output (can target internal voices)
 */
class ZenithStepSequencer {
public:
    ZenithStepSequencer();
    ~ZenithStepSequencer() = default;

    //==========================================================================
    // Pattern Data
    //==========================================================================

    static constexpr int NUM_ROWS = 8;
    static constexpr int NUM_STEPS = 16;
    static constexpr int MAX_PATTERNS = 128;

    struct Step {
        uint8_t note = 0;          ///< MIDI note (0-127)
        uint8_t velocity = 100;     ///< Velocity (0-127)
        uint8_t gate = 255;         ///< Gate/Prob (0-255)
        bool tie = false;           ///< Tie to next note
        bool slide = false;          ///< Slide/portamento
    };

    struct Pattern {
        Step steps[NUM_ROWS * NUM_STEPS];
    uint8_t length = NUM_STEPS;
        uint8_t direction = 0;       ///< 0=up, 1=down, 2=up/down
        uint8_t shuffle = 0;          ///< 0=none, 1=rotate, 2=random
        uint8_t groove = 0;           ///< Swing amount (0-100%)
    };

    //==========================================================================
    // Configuration
    //==========================================================================

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void reset();

    /** Get current pattern */
    const Pattern& getPattern(int index) const { return patterns_[index]; }

    /** Get/set current pattern */
    int getCurrentPattern() const { return currentPattern_; }
    void setCurrentPattern(int index) { currentPattern_ = index; }

    /** Get/set current step */
    int getCurrentStep() const { return currentStep_; }
    void setCurrentStep(int step) { currentStep_ = step % NUM_STEPS; }

    /** Set step value */
    void setStepValue(int row, int step, const Step& value);
    void clearStep(int row, int step);
    void clearRow(int row);

    //==========================================================================
    // Transport Control
    //==========================================================================

    enum class PlayMode {
        Forward,       ///< Play straight through
        PingPong,      ///< Alternating directions
        Random,        ///< Random order
        Reverse        ///< Play backward
    };

    void setPlayMode(PlayMode mode) { playMode_ = mode; }
    PlayMode getPlayMode() const { return playMode_; }

    void setBPM(float bpm) { bpm_ = juce::jlimit(20.0f, 300.0f, bpm); }
    void setGateLength(float beats) { gateLength_ = juce::jlimit(0.0f, 4.0f, beats); }

    /** Start/Stop playback */
    void start();
    void stop();
    bool isPlaying() const { return playing_; }

    //==========================================================================
    // Output
    //==========================================================================

    /**
     * @brief Get current note to play (one per row)
     * @param row Current row (0-7)
     * @return MIDI note number
     */
    uint8_t getCurrentNote(int row) const;

    /**
     * @brief Process sequencer (advance one step)
     * @return True if pattern wrapped
     */
    bool process();

private:
    //==========================================================================
    // State
    //==========================================================================

    std::array<Pattern, MAX_PATTERNS> patterns_;
    int currentPattern_ = 0;
    int currentStep_ = 0;
    double sampleRate_ = 44100.0;

    bool playing_ = false;
    PlayMode playMode_ = PlayMode::Forward;

    float bpm_ = 120.0f;
    float gateLength_ = 0.25f;  // 1/4 note default

    double phase_ = 0.0;              // Accumulated phase
    int lastNote_[NUM_ROWS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
    int lastVelocity_[NUM_ROWS] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    int getCurrentRow() const;
    float getSecondsPerBeat() const;
    double getPhaseIncrement() const;

}; // namespace zenith
