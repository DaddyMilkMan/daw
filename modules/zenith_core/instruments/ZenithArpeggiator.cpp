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

#include "ZenithArpeggiator.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithArpeggiator::ZenithArpeggiator() {
    notes_.fill({});
    reset();
}

void ZenithArpeggiator::reset() {
    numHeld_ = 0;
    orderCounter_ = 0;
    phase_ = 0.0;
    gatePhase_ = 0.0f;
    currentStep_ = 0;
    currentNote_ = -1;
    currentVelocity_ = 0.0f;
    nextNoteTime_ = 0.0;
}

void ZenithArpeggiator::setSampleRate(double sr) {
    sampleRate_ = sr;
}

//==============================================================================
// NOTE MANAGEMENT
//==============================================================================

void ZenithArpeggiator::addNote(int midiNote, float velocity) {
    // Find empty slot or lowest velocity note
    int slot = -1;
    float lowestVel = 2.0f;
    for (int i = 0; i < MAX_NOTES; ++i) {
        if (notes_[i].active) {
            if (notes_[i].velocity < lowestVel) {
                lowestVel = notes_[i].velocity;
                slot = i;
            }
        } else {
            slot = i;
        }
    }

    if (slot >= 0) {
        notes_[slot] = {true, velocity, midiNote};
        notes_[slot].order = orderCounter_++;
        numHeld_++;
    }
}

void ZenithArpeggiator::removeNote(int midiNote) {
    for (int i = 0; i < MAX_NOTES; ++i) {
        if (notes_[i].active && notes_[i].midiNote == midiNote) {
            notes_[i].active = false;
            numHeld_--;
        }
    }
}

void ZenithArpeggiator::clearNotes() {
    notes_.fill({});
    numHeld_ = 0;
    currentNote_ = -1;
}

//==============================================================================
// PROCESSING
//==============================================================================

bool ZenithArpeggiator::getNextNote(int& outNote, float& outVelocity) {
    if (numHeld_ == 0) {
        outNote = -1;
        outVelocity = 0.0f;
        return false;
    }

    // Calculate step duration from rate
    double secondsPerBeat = 60.0 / rate_;
    double samplesPerBeat = secondsPerBeat * sampleRate_;

    // Check timing
    double time = juce::Time::getHighResolutionTicks() * 1.0e-9;
    bool shouldTrigger = (time >= nextNoteTime_);

    if (shouldTrigger || currentNote_ < 0) {
        advanceStep();
        nextNoteTime_ = time + samplesPerBeat;
    }

    if (currentNote_ >= 0) {
        outNote = currentNote_;
        outVelocity = currentVelocity_;
        return true;
    }

    return false;
}

void ZenithArpeggiator::process(int numSamples) {
    // Process gate
    if (gate_ > 0.0f && numHeld_ > 0) {
        double gateFreq = gate_ * 4.0;  // Hz
        double gateInc = gateFreq / sampleRate_;
        gatePhase_ = std::fmod(gatePhase_ + gateInc * numSamples, 1.0);
    }
}

//==============================================================================
// STEP ADVANCE
//==============================================================================

void ZenithArpeggiator::advanceStep() {
    currentStep_ = (currentStep_ + 1) % numSteps();

    // Get sorted indices based on pattern
    auto sortedIndices = getSortedIndices();

    switch (pattern_) {
        case Pattern::Up:
            currentNote_ = notes_[sortedIndices[currentStep_ % numHeld_]].midiNote;
            break;

        case Pattern::Down:
            currentNote_ = notes_[sortedIndices[(numHeld_ - 1) - (currentStep_ % numHeld_)]].midiNote;
            break;

        case Pattern::UpDown:
            if ((currentStep_ / numHeld_) % 2 == 0) {
                currentNote_ = notes_[sortedIndices[currentStep_ % numHeld_]].midiNote;
            } else {
                int idx = (numHeld_ - 1) - (currentStep_ % numHeld_);
                currentNote_ = notes_[sortedIndices[idx]].midiNote;
            }
            break;

        case Pattern::Random: {
            currentNote_ = notes_[getRandomNoteIndex()].midiNote;
            break;
        }

        case Pattern::Chord:
            if (currentStep_ == 0) {
                // All notes on first step
            }
            break;

        case Pattern::AsPlayed: {
            currentNote_ = notes_[sortedIndices[currentStep_ % numHeld_]].midiNote;
            break;
        }

        case Pattern::Order: {
            // Use original order
            int idx = 0;
            int count = 0;
            for (int i = 0; i < MAX_NOTES; ++i) {
                if (notes_[i].active) {
                    if (count == currentStep_) {
                        currentNote_ = notes_[i].midiNote;
                        break;
                    }
                    count++;
                }
            }
            break;
        }
    }

    // Apply octave offset
    currentNote_ += octaveRange_ * 12;

    // Apply velocity curve
    if (hold_) {
        currentVelocity_ *= duration_;
    } else {
        currentVelocity_ = std::min(1.0f, currentVelocity_ * (1.0f - duration_));
    }

    // Reset phase on new pattern cycle
    if (currentStep_ == 0) {
        phase_ = 0.0;
    }
}

//==============================================================================
// SORTING
//==============================================================================

int* ZenithArpeggiator::getSortedIndices() {
    static int indices[MAX_NOTES];
    int count = 0;

    for (int i = 0; i < MAX_NOTES; ++i) {
        if (notes_[i].active) {
            indices[count++] = i;
        }
    }

    // Sort based on pattern
    switch (sort_) {
        case -1: // Original
            break;
        case 0:  // Ascending note
            std::sort(indices, indices + count, [this](int a, int b) {
                return notes_[a].midiNote < notes_[b].midiNote;
            });
            break;
        case 1:  // Descending note
            std::sort(indices, indices + count, [this](int a, int b) {
                return notes_[a].midiNote > notes_[b].midiNote;
            });
            break;
    }

    return indices;
}

int ZenithArpeggiator::getRandomNoteIndex() {
    // Count active notes
    int count = 0;
    for (int i = 0; i < MAX_NOTES; ++i) {
        if (notes_[i].active) count++;
    }

    if (count == 0) return 0;
    return juce::Random::getSystemRandom().nextInt(count);
}

} // namespace zenith
