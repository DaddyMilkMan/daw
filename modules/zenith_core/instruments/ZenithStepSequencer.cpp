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

#include "ZenithStepSequencer.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithStepSequencer::ZenithStepSequencer() {
    patterns_.fill({});
    for (int p = 0; p < MAX_PATTERNS; ++p) {
        for (int r = 0; r < NUM_ROWS; ++r) {
            for (int s = 0; s < NUM_STEPS; ++s) {
                patterns_[p].steps[r][s] = {};
            }
        }
    }

    reset();
}

//==============================================================================
// PATTERN DATA
//==============================================================================

void ZenithStepSequencer::reset() {
    currentPattern_ = 0;
    currentStep_ = 0;
    playing_ = false;
    phase_ = 0.0;
    currentNote_ = -1;
    currentVelocity_ = 0.0f;
    gatePhase_ = 0.0f;

    for (int row = 0; row < NUM_ROWS; ++row) {
        lastNote_[row] = -1;
        lastVelocity_[row] = 0.0f;
    }
}

void ZenithStepSequencer::setCurrentPattern(int index) {
    currentPattern_ = juce::jlimit(0, MAX_PATTERNS - 1, index);
}

//==============================================================================
// STEP CONTROL
//==============================================================================

void ZenithStepSequencer::setStepValue(int row, int step, const Step& value) {
    if (row >= 0 && row < NUM_ROWS && step >= 0 && step < NUM_STEPS) {
        patterns_[currentPattern_].steps[row][step] = value;
    }
}

void ZenithStepSequencer::clearStep(int row, int step) {
    if (row >= 0 && row < NUM_ROWS && step >= 0 && step < NUM_STEPS) {
        patterns_[currentPattern_].steps[row][step] = {};
    }
}

void ZenithStepSequencer::clearRow(int row) {
    for (int s = 0; s < NUM_STEPS; ++s) {
        clearStep(row, s);
    }
}

//==============================================================================
// PLAYBACK CONTROL
//==============================================================================

void ZenithStepSequencer::start() {
    playing_ = true;
}

void ZenithStepSequencer::stop() {
    playing_ = false;

    for (int row = 0; row < NUM_ROWS; ++row) {
        lastNote_[row] = -1;
        lastVelocity_[row] = 0.0f;
    }
}

bool ZenithStepSequencer::isPlaying() const {
    return playing_;
}

void ZenithStepSequencer::setSampleRate(double sr) {
    sampleRate_ = sr;
}

//==============================================================================
// PROCESSING
//==============================================================================

bool ZenithStepSequencer::process() {
    if (!playing_) return false;

    // Calculate timing
    double secondsPerBeat = 60.0 / bpm_;
    double samplesPerBeat = secondsPerBeat * sampleRate_;
    double phaseIncrement = 1.0 / samplesPerBeat;

    // Advance phase
    phase_ = std::fmod(phase_ + phaseIncrement, 1.0);

    // Calculate current step
    int step = static_cast<int>(phase_ * NUM_STEPS);
    if (step >= NUM_STEPS) return false;

    // Get pattern
    const Pattern& pattern = patterns_[currentPattern_];

    // Process each row
    bool wrapped = false;
    for (int row = 0; row < NUM_ROWS; ++row) {
        const Step& stepData = pattern.steps[row][step];

        // Check gate
        double gatePhase = gatePhase_;
        bool gateOpen = (gatePhase >= stepData.gate);

        if (gateOpen && stepData.note < MAX_NOTES) {
            // Trigger note
            uint8_t note = stepData.note;
            float velocity = stepData.velocity / 255.0f;

            // Apply probability
            if (stepData.probability > 0 && stepData.probability < 255) {
                if ((random_.next() & 0xFF) >= stepData.probability) {
                    // Skip note
                    continue;
                }
            }

            // Play note
            currentNote_ = note;
            currentVelocity_ = velocity;
            lastNote_[row] = note;
            lastVelocity_[row] = velocity;

            wrapped = true;
        }
    }

    return wrapped;
}

uint8_t ZenithStepSequencer::getCurrentNote(int row) const {
    if (row >= 0 && row < NUM_ROWS) {
        return static_cast<uint8_t>(lastNote_[row]);
    }
    return 0;
}

int ZenithStepSequencer::getCurrentRow() const {
    return currentStep_ / NUM_STEPS;
}

int ZenithStepSequencer::getCurrentStep() const {
    return currentStep_ % NUM_STEPS;
}

float ZenithStepSequencer::getCurrentVelocity(int row) const {
    if (row >= 0 && row < NUM_ROWS) {
        return lastVelocity_[row] / 255.0f;
    }
    return 0.0f;
}

float ZenithStepSequencer::getSecondsPerBeat() const {
    return 60.0f / bpm_;
}

double ZenithStepSequencer::getPhaseIncrement() const {
    double secondsPerBeat = 60.0 / bpm_;
    double samplesPerBeat = secondsPerBeat * sampleRate_;
    return 1.0 / samplesPerBeat;
}

//==============================================================================
// PATTERN EDITING
//==============================================================================

} // namespace zenith
