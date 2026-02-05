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

    MidiPitchController.h
    Created: 2026-01-29
    Author:  Zenith DAW

    MIDI-controlled pitch targeting for Auto-Tune.
    
    Allows playing target notes on a MIDI keyboard while singing.

    This is the classic "Auto-Tune with MIDI" workflow used by pros.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <array>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    MIDI-controlled pitch target selector.
    
    Features:
    - MIDI note sets target pitch
    - Pitch bend for microtonal control
    - Velocity for correction amount
    - Sustain pedal for legato
    - Glide/portamento between notes
    - Chord mode (target multiple notes)
*/
class MidiPitchController
{
public:
    //==============================================================================
    MidiPitchController();
    ~MidiPitchController();

    //==============================================================================
    /**
     * @brief Process incoming MIDI and update target
     * @param midiBuffer MIDI messages for this block
     * @param sampleRate Current sample rate
     */
    void processMidi(const juce::MidiBuffer& midiBuffer, double sampleRate);
    
    /**
     * @brief Get the current target pitch in Hz
     * @return 0.0 if no MIDI note active, otherwise target frequency
     */
    float getTargetPitchHz() const { return targetPitchHz_.load(); }
    
    /**
     * @brief Get the current target as MIDI note number
     */
    int getTargetMidiNote() const { return currentNote_.load(); }
    
    /**
     * @brief Check if MIDI is controlling pitch (note is active)
     */
    bool isActive() const { return isActive_.load(); }
    
    /**
     * @brief Get the current correction amount from velocity
     */
    float getCorrectionAmount() const { return correctionAmount_.load(); }

    //==============================================================================
    /**
     * @brief Set glide/portamento time in seconds
     */
    void setGlideTime(float seconds) { glideTime_ = juce::jlimit(0.0f, 5.0f, seconds); }
    float getGlideTime() const { return glideTime_.load(); }
    
    /**
     * @brief Enable/disable glide
     */
    void setGlideEnabled(bool enabled) { glideEnabled_ = enabled; }
    bool isGlideEnabled() const { return glideEnabled_.load(); }
    
    /**
     * @brief Set pitch bend range in semitones
     */
    void setPitchBendRange(int semitones) { pitchBendRange_ = juce::jlimit(1, 24, semitones); }
    int getPitchBendRange() const { return pitchBendRange_.load(); }
    
    /**
     * @brief Enable chord mode (target multiple notes)
     */
    void setChordMode(bool enabled) { chordMode_ = enabled; }
    bool isChordMode() const { return chordMode_.load(); }
    
    /**
     * @brief Set velocity sensitivity (0-1)
     * At 0: velocity doesn't affect correction
     * At 1: velocity fully controls correction amount
     */
    void setVelocitySensitivity(float sensitivity) { 
        velocitySensitivity_ = juce::jlimit(0.0f, 1.0f, sensitivity); 
    }
    float getVelocitySensitivity() const { return velocitySensitivity_.load(); }

    //==============================================================================
    /**
     * @brief Reset all MIDI state (panic)
     */
    void reset();

private:
    //==============================================================================
    void noteOn(int note, int velocity);
    void noteOff(int note);
    void pitchBend(int value);
    void sustainPedal(bool down);
    void updateTargetPitch();
    float midiNoteToHz(int note) const;

    //==============================================================================
    // Note tracking
    static constexpr int kMaxNotes = 128;
    std::array<bool, kMaxNotes> activeNotes_;
    std::array<int, kMaxNotes> noteVelocities_;
    int lowestActiveNote_ = -1;
    int highestActiveNote_ = -1;
    int lastNotePlayed_ = -1;
    
    // State
    std::atomic<int> currentNote_{-1};
    std::atomic<float> targetPitchHz_{0.0f};
    std::atomic<float> smoothedPitchHz_{0.0f};
    std::atomic<bool> isActive_{false};
    std::atomic<float> correctionAmount_{1.0f};
    
    // Pitch bend
    std::atomic<float> pitchBendAmount_{0.0f};  // -1 to +1
    std::atomic<int> pitchBendRange_{2};  // semitones
    
    // Settings
    std::atomic<float> glideTime_{0.05f};  // seconds
    std::atomic<bool> glideEnabled_{true};
    std::atomic<bool> chordMode_{false};
    std::atomic<float> velocitySensitivity_{0.5f};
    
    // Sustain pedal
    std::atomic<bool> sustainPedalDown_{false};
    std::array<bool, kMaxNotes> notesHeldBySustain_;
    
    // Sample rate for glide calculation
    double sampleRate_ = 44100.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiPitchController)
};

} // namespace dsp
} // namespace zenith
