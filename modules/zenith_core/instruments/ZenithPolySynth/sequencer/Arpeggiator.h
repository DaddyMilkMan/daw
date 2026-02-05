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

  ==============================================================================

    Arpeggiator.h
    Created: 2025-01-28
    Updated: 2025-02-02 - S-tier implementation
    Author: Zenith DAW

    Professional arpeggiator with sample-accurate timing and BPM sync.
    Patterned after JUCE's official ArpeggiatorPluginDemo example.


  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>

namespace zenith {

//==============================================================================
// Arpeggiator Modes
//==============================================================================

enum class ArpMode {
    Up = 0,
    Down,
    UpDown,
    Random,
    Chord,
    Order,
    AsPlayed
};

enum class ArpSyncRate {
    Free = 0,
    _1_64,
    _1_32,
    _1_16,
    _1_8,
    _1_4,
    _1_2,
    _1_1,
    _2_1,
    _4_1,
    _8_1,
    _16_1
};

//==============================================================================
// Arpeggiator Class
//==============================================================================

class Arpeggiator {
public:
    Arpeggiator();

    // Configuration
    void setMode(ArpMode mode);
    void setRate(float rateHz);
    void setSyncRate(ArpSyncRate rate);
    void setBPM(double bpm);
    void setGate(float gate);
    void setOctaveRange(int octaves);
    void setSwing(float swing);
    void setPattern(const juce::Array<int>& pattern);
    void setVelocityPattern(const juce::Array<float>& velPattern);
    void setHoldMode(bool hold);

    // MIDI input
    void noteOn(int note, float velocity);
    void noteOff(int note);

    // State management
    void reset();

    // Processing - generates arpeggiated MIDI output
    void process(juce::MidiBuffer& buffer, double sampleRate, int numSamples);

    // Query
    juce::SortedSet<int> getActiveNotes() const { return activeNotes_; }
    int getCurrentNoteIndex() const { return currentNoteIndex_; }

private:
    // Parameters
    ArpMode mode_;
    float rateHz_;
    ArpSyncRate syncRate_;
    double bpm_;
    float gate_;
    int octaveRange_;
    float swing_;
    juce::Array<int> pattern_;
    juce::Array<float> velocityPattern_;
    bool holdMode_;

    // State - uses JUCE pattern from ArpeggiatorPluginDemo
    juce::SortedSet<int> activeNotes_;  // Automatically sorted
    std::map<int, float> noteVelities_; // Per-note velocity storage
    int currentNoteIndex_;              // Current position in note array
    int lastNoteValue_;                 // Last note sent to output
    int timeAccumulator_;               // Sample counter (persists across blocks)
    double samplesPerStep_;             // Current step duration in samples
    int currentOctaveOffset_;
    int direction_;                     // For UpDown mode

    // Internal helpers
    void updateTiming(double sampleRate);
    double getSyncRateDivisor(ArpSyncRate rate);
    void advanceToNextNote();
    float getVelocityForCurrentStep();
};

} // namespace zenith
