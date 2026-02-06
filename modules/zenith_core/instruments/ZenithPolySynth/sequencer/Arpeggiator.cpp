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

#include "Arpeggiator.h"
#include <cmath>

namespace zenith {

//==============================================================================
// Arpeggiator Implementation
//==============================================================================

Arpeggiator::Arpeggiator()
    : mode_(ArpMode::Up)
    , rateHz_(4.0f)
    , syncRate_(ArpSyncRate::_1_4)
    , bpm_(120.0)
    , gate_(0.8f)
    , octaveRange_(1)
    , swing_(0.0f)
    , holdMode_(false)
    , currentNoteIndex_(0)
    , lastNoteValue_(-1)
    , timeAccumulator_(0)
    , samplesPerStep_(0)
    , currentOctaveOffset_(0)
    , direction_(1)
{
    // Initialize with default pattern
    pattern_.add(0);
    velocityPattern_.add(1.0f);
}

void Arpeggiator::reset() {
    // Clear all state
    activeNotes_.clear();
    currentNoteIndex_ = 0;
    lastNoteValue_ = -1;
    timeAccumulator_ = 0;
    currentOctaveOffset_ = 0;
    direction_ = (mode_ == ArpMode::Down) ? -1 : 1;
}

void Arpeggiator::setMode(ArpMode mode) {
    if (mode_ != mode) {
        mode_ = mode;
        // Reset direction when changing modes
        if (mode == ArpMode::Up || mode == ArpMode::Down) {
            direction_ = (mode == ArpMode::Up) ? 1 : -1;
        } else if (mode == ArpMode::UpDown) {
            direction_ = 1;
        }
        currentNoteIndex_ = 0;
    }
}

void Arpeggiator::setRate(float rateHz) {
    rateHz_ = juce::jlimit(0.1f, 50.0f, rateHz);
}

void Arpeggiator::setSyncRate(ArpSyncRate rate) {
    syncRate_ = rate;
}

void Arpeggiator::setBPM(double bpm) {
    bpm_ = juce::jlimit(20.0, 300.0, bpm);
}

void Arpeggiator::setGate(float gate) {
    gate_ = juce::jlimit(0.01f, 1.0f, gate);
}

void Arpeggiator::setOctaveRange(int octaves) {
    octaveRange_ = juce::jlimit(1, 4, octaves);
}

void Arpeggiator::setSwing(float swing) {
    swing_ = juce::jlimit(0.0f, 1.0f, swing);
}

void Arpeggiator::setPattern(const juce::Array<int>& pattern) {
    if (pattern.isEmpty()) {
        pattern_.clear();
        pattern_.add(0);
    } else {
        pattern_ = pattern;
    }
}

void Arpeggiator::setVelocityPattern(const juce::Array<float>& velPattern) {
    if (velPattern.isEmpty()) {
        velocityPattern_.clear();
        velocityPattern_.add(1.0f);
    } else {
        velocityPattern_ = velPattern;
    }
}

void Arpeggiator::setHoldMode(bool hold) {
    if (!hold && holdMode_) {
        // When releasing hold mode, clear all held notes
        activeNotes_.clear();
        currentNoteIndex_ = 0;
    }
    holdMode_ = hold;
}

void Arpeggiator::noteOn(int note, float velocity) {
    // Ignore duplicate note-ons
    if (activeNotes_.contains(note)) {
        return;
    }

    // Add note - SortedSet maintains automatic ordering
    activeNotes_.add(note);

    // Store velocity for this note
    noteVelities_[note] = juce::jlimit(0.0f, 1.0f, velocity / 127.0f);
}

void Arpeggiator::noteOff(int note) {
    if (holdMode_) {
        // In hold mode, ignore note-off
        return;
    }

    activeNotes_.removeValue(note);
    noteVelities_.erase(note);

    // Reset state when no notes active
    if (activeNotes_.isEmpty()) {
        lastNoteValue_ = -1;
        currentNoteIndex_ = 0;
        currentOctaveOffset_ = 0;
        direction_ = (mode_ == ArpMode::Down) ? -1 : 1;
    }
}

void Arpeggiator::process(juce::MidiBuffer& buffer, double sampleRate, int numSamples) {
    // Update timing calculation
    updateTiming(sampleRate);

    // If no notes active, send note off for any sounding note
    if (activeNotes_.isEmpty()) {
        if (lastNoteValue_ >= 0) {
            buffer.addEvent(juce::MidiMessage::noteOff(1, static_cast<juce::uint8>(lastNoteValue_)), 0);
            lastNoteValue_ = -1;
        }
        timeAccumulator_ = 0;
        return;
    }

    // Calculate current step duration (with swing applied)
    int currentStepDuration = static_cast<int>(samplesPerStep_);
    if (swing_ > 0.0f && (currentNoteIndex_ % 2) == 1) {
        // Odd steps are longer with swing
        currentStepDuration = static_cast<int>(samplesPerStep_ * (1.0 + swing_));
    } else if (swing_ > 0.0f) {
        // Even steps are shorter with swing
        currentStepDuration = static_cast<int>(samplesPerStep_ * (1.0 - swing_));
    }

    // Calculate gate duration
    int gateDuration = static_cast<int>(currentStepDuration * gate_);

    // Check if we cross a step boundary in this block
    // This is the KEY pattern from JUCE's arpeggiator example
    if ((timeAccumulator_ + numSamples) >= currentStepDuration) {
        // Calculate exact sample offset within this block
        int offset = juce::jmax(0, juce::jmin(
            static_cast<int>(currentStepDuration - timeAccumulator_),
            numSamples - 1
        ));

        // Send note OFF for previous note first
        if (lastNoteValue_ >= 0) {
            buffer.addEvent(juce::MidiMessage::noteOff(1, static_cast<juce::uint8>(lastNoteValue_)), offset);
        }

        // Advance to next note
        advanceToNextNote();

        // Send note ON for new note
        if (lastNoteValue_ >= 0) {
            float velocity = getVelocityForCurrentStep();

            // Apply octave offset
            int finalNote = lastNoteValue_ + (currentOctaveOffset_ * 12);
            finalNote = juce::jlimit(0, 127, finalNote);

            juce::uint8 velByte = static_cast<juce::uint8>(juce::jlimit(1, 127, static_cast<int>(velocity * 127.0f)));
            buffer.addEvent(juce::MidiMessage::noteOn(1, static_cast<juce::uint8>(finalNote), velByte), offset);

            // Schedule note OFF based on gate
            int noteOffOffset = offset + gateDuration;
            if (noteOffOffset < numSamples) {
                buffer.addEvent(juce::MidiMessage::noteOff(1, static_cast<juce::uint8>(finalNote)), noteOffOffset);
            }
        }
    }

    // Accumulate time, wrap with modulo
    // This maintains timing continuity across processBlock calls
    timeAccumulator_ = (timeAccumulator_ + numSamples) % currentStepDuration;
}

void Arpeggiator::updateTiming(double sampleRate) {
    if (syncRate_ == ArpSyncRate::Free) {
        // Free running in Hz
        if (rateHz_ > 0.0f && sampleRate > 0.0f) {
            samplesPerStep_ = sampleRate / rateHz_;
        } else {
            samplesPerStep_ = sampleRate / 4.0; // Default 4 Hz
        }
    } else {
        // BPM synced - calculate samples per step
        double divisor = getSyncRateDivisor(syncRate_);
        double beatTime = 60.0 / bpm_;
        samplesPerStep_ = beatTime * sampleRate * divisor;
    }

    // Ensure minimum step duration
    samplesPerStep_ = juce::jmax(1.0, samplesPerStep_);
}

double Arpeggiator::getSyncRateDivisor(ArpSyncRate rate) {
    switch (rate) {
        case ArpSyncRate::_1_64: return 1.0 / 64.0;
        case ArpSyncRate::_1_32: return 1.0 / 32.0;
        case ArpSyncRate::_1_16: return 1.0 / 16.0;
        case ArpSyncRate::_1_8:  return 1.0 / 8.0;
        case ArpSyncRate::_1_4:  return 1.0 / 4.0;
        case ArpSyncRate::_1_2:  return 1.0 / 2.0;
        case ArpSyncRate::_1_1:  return 1.0;
        case ArpSyncRate::_2_1:  return 2.0;
        case ArpSyncRate::_4_1:  return 4.0;
        case ArpSyncRate::_8_1:  return 8.0;
        case ArpSyncRate::_16_1: return 16.0;
        case ArpSyncRate::Free:
        default: return 0.25;
    }
}

void Arpeggiator::advanceToNextNote() {
    if (activeNotes_.isEmpty()) {
        lastNoteValue_ = -1;
        return;
    }

    int numNotes = activeNotes_.size();

    switch (mode_) {
        case ArpMode::Up:
            currentNoteIndex_ = (currentNoteIndex_ + 1) % numNotes;
            break;

        case ArpMode::Down:
            currentNoteIndex_ = (currentNoteIndex_ + 1) % numNotes;
            break;

        case ArpMode::UpDown: {
            currentNoteIndex_ += direction_;

            // Reverse at boundaries
            if (currentNoteIndex_ >= numNotes - 1) {
                direction_ = -1;
                currentNoteIndex_ = numNotes - 1;
            } else if (currentNoteIndex_ <= 0) {
                direction_ = 1;
                currentNoteIndex_ = 0;
            }
            break;
        }

        case ArpMode::Random:
            currentNoteIndex_ = juce::Random::getSystemRandom().nextInt(numNotes);
            break;

        case ArpMode::Chord:
            // All notes at once - just use first note
            currentNoteIndex_ = 0;
            break;

        case ArpMode::Order:
        case ArpMode::AsPlayed:
        default:
            currentNoteIndex_ = (currentNoteIndex_ + 1) % numNotes;
            break;
    }

    // Get the note value based on mode
    if (mode_ == ArpMode::Down) {
        // For Down mode, access in reverse order
        lastNoteValue_ = activeNotes_[numNotes - 1 - currentNoteIndex_];
    } else {
        lastNoteValue_ = activeNotes_[currentNoteIndex_];
    }
}

float Arpeggiator::getVelocityForCurrentStep() {
    if (velocityPattern_.isEmpty()) {
        // Try to get the stored velocity for this note
        auto it = noteVelities_.find(lastNoteValue_);
        if (it != noteVelities_.end()) {
            return it->second;
        }
        return 0.8f; // Default velocity
    }

    int patternIndex = currentNoteIndex_ % velocityPattern_.size();
    return juce::jlimit(0.0f, 1.0f, velocityPattern_[patternIndex]);
}

} // namespace zenith
