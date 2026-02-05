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

    HarmonyGenerator.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Automatic harmony generation from lead vocal.
    
    Creates 2-4 part harmonies from a single vocal track.

    Features:
    - Intelligent voice leading
    - Scale-aware harmony selection
    - Close vs open voicing
    - Humanize timing and pitch
    - MIDI output for harmony voices

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <atomic>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    Harmony voice configuration.
*/
struct HarmonyVoice
{
    int interval = 3;           // Interval in semitones (3 = minor third)
    int scaleDegree = 3;        // Scale degree (1-7)
    bool above = true;          // Above or below lead
    float pan = 0.0f;           // -1 to 1 (left to right)
    float gain = 0.8f;          // Volume relative to lead
    float timing = 0.0f;        // Timing offset in ms (positive = delay)
    float humanize = 0.3f;      // Amount of pitch variation
    bool enabled = true;
};

//==============================================================================
/**
    Harmony generation mode.
*/
enum class HarmonyMode
{
    FixedInterval,      // Fixed semitone interval
    Diatonic,           // Scale degree following
    Chordal,            // Follow chord progression
    Intelligent         // AI voice leading
};

//==============================================================================
/**
    Automatic harmony generation.
    
    Creates background vocals from lead vocal in real-time.
*/
class HarmonyGenerator
{
public:
    //==============================================================================
    HarmonyGenerator();
    ~HarmonyGenerator();

    //==============================================================================
    /**
     * @brief Prepare for processing
     */
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    //==============================================================================
    /**
     * @brief Generate harmonies from lead vocal
     * @param leadBuffer Input lead vocal (mono)
     * @param harmonyBuffers Output harmony voices (stereo)
     * @param detectedPitch Lead vocal pitch in Hz (0 if unvoiced)
     */
    void process(const juce::AudioBuffer<float>& leadBuffer,
                 std::vector<juce::AudioBuffer<float>*>& harmonyBuffers,
                 float detectedPitch);

    //==============================================================================
    /**
     * @brief Set number of harmony voices (1-4)
     */
    void setNumVoices(int voices) { numVoices_ = juce::jlimit(1, 4, voices); }
    int getNumVoices() const { return numVoices_.load(); }
    
    /**
     * @brief Configure a harmony voice
     */
    void setVoiceConfig(int voiceIndex, const HarmonyVoice& config);
    HarmonyVoice getVoiceConfig(int voiceIndex) const;
    
    /**
     * @brief Enable/disable harmony generation
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_.load(); }
    
    /**
     * @brief Set harmony generation mode
     */
    void setMode(HarmonyMode mode) { mode_ = mode; }
    HarmonyMode getMode() const { return mode_; }
    
    /**
     * @brief Set current key/scale for diatonic harmony
     */
    void setKey(int rootNote, int scaleType);  // rootNote: 0=C, scaleType: 0=major
    
    /**
     * @brief Set chord progression for chordal mode
     * @param chords Array of chord roots (0-11) for each bar
     */
    void setChordProgression(const std::vector<int>& chords);

    //==============================================================================
    /**
     * @brief Get MIDI output for harmonies (for external instruments)
     */
    void getMidiOutput(juce::MidiBuffer& midiBuffer, double sampleRate);

    //==============================================================================
    // Quick presets
    enum class Preset
    {
        Octave,         // Simple octave up/down
        Thirds,         // Major/minor thirds
        Power,          // Power chord (root + fifth)
        Triad,          // 3-part triad
        Seventh,        // 4-part seventh chord
        OctaveDouble,   // Double with octaves
        Feminine,       // Higher harmonies
        Masculine       // Lower harmonies
    };
    
    void loadPreset(Preset preset);

private:
    //==============================================================================
    void calculateHarmonyPitches(float leadPitch, std::array<float, 4>& harmonyPitches);
    float getScalePitch(float leadPitch, int scaleDegree);
    int freqToMidiNote(float freq) const;
    float midiNoteToFreq(int note) const;
    void processVoice(int voiceIndex, const float* input, float* output, 
                      int numSamples, float targetPitch);
    
    //==============================================================================
    // Configuration
    std::atomic<bool> enabled_{false};
    std::atomic<int> numVoices_{2};
    std::array<HarmonyVoice, 4> voices_;
    HarmonyMode mode_ = HarmonyMode::Diatonic;
    
    // Scale/Key
    int keyRoot_ = 0;       // C
    int scaleType_ = 0;     // Major
    std::vector<bool> currentScale_;  // 12 bools for scale notes
    
    // Chord progression
    std::vector<int> chordProgression_;
    double currentBar_ = 0.0;
    
    // State
    double sampleRate_ = 44100.0;
    std::array<float, 4> currentVoicePitches_;
    std::array<float, 4> smoothedPitches_;
    std::array<juce::AudioBuffer<float>, 4> delayBuffers_;
    std::array<int, 4> delayWritePos_;
    
    // Pitch shifters for each voice
    class VoicePitchShifter;  // Forward declaration
    std::array<std::unique_ptr<VoicePitchShifter>, 4> pitchShifters_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonyGenerator)
};

} // namespace dsp
} // namespace zenith
