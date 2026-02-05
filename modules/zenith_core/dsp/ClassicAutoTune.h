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

    ClassicAutoTune.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Recreation of the Auto-Tune 5 "Classic" algorithm.
    
    This is the iconic sound used on:

    - T-Pain
    - Cher "Believe"
    - Daft Punk
    - Countless hip-hop tracks
    
    The "Classic" mode is characterized by:
    - Hard pitch quantization
    - Fast retune speed (robotic effect)
    - Slight artifacts that define the sound
    - Less transparent than modern algorithms

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    Classic Auto-Tune 5 style pitch correction.
    
    This algorithm recreates the iconic "robot voice" effect that
    defined the 2000s pop sound. It's less natural than modern algorithms
    but has a specific character that artists specifically request.
    
    Key characteristics:
    - Hard quantization to scale
    - Very fast correction (almost instant)
    - Step-like pitch transitions
    - Slight phasing/artifacts (part of the sound)
    - Formant preservation (optional)
*/
class ClassicAutoTune
{
public:
    //==============================================================================
    ClassicAutoTune();
    ~ClassicAutoTune();

    //==============================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    //==============================================================================
    /**
     * @brief Process audio with classic Auto-Tune sound
     */
    void process(const juce::AudioBuffer<float>& input,
                 juce::AudioBuffer<float>& output,
                 const std::vector<float>& detectedPitches);

    //==============================================================================
    /**
     * @brief Set retune speed (0-100)
     * 0 = instant (robot)
     * 20-30 = fast (T-Pain style)
     * 50+ = slower
     */
    void setRetuneSpeed(float speed) { retuneSpeed_ = juce::jlimit(0.0f, 100.0f, speed); }
    float getRetuneSpeed() const { return retuneSpeed_.load(); }
    
    /**
     * @brief Set scale/note quantization
     * 0 = chromatic (all notes)
     * Higher = snap to fewer notes
     */
    void setScale(const std::array<bool, 12>& scaleNotes, int rootNote);
    
    /**
     * @brief Set correction amount (0-1)
     */
    void setCorrectionAmount(float amount) { correctionAmount_ = juce::jlimit(0.0f, 1.0f, amount); }
    float getCorrectionAmount() const { return correctionAmount_.load(); }
    
    /**
     * @brief Enable formant preservation
     */
    void setPreserveFormants(bool preserve) { preserveFormants_ = preserve; }
    bool getPreserveFormants() const { return preserveFormants_.load(); }
    
    /**
     * @brief Set humanize (0-1)
     * Adds slight variation for less robotic sound
     */
    void setHumanize(float amount) { humanize_ = juce::jlimit(0.0f, 1.0f, amount); }
    float getHumanize() const { return humanize_.load(); }

    //==============================================================================
    // Presets
    enum class Preset
    {
        Robot,          // Instant retune, full correction
        TPain,          // Fast retune, formant preserve
        Cher,           // Medium speed, some humanize
        ModernClassic,  // Slower, more natural
        Subtle          // Barely noticeable
    };
    
    void loadPreset(Preset preset);

private:
    //==============================================================================
    float correctPitchClassic(float inputPitch, float& smoothedPitch);
    float quantizeToScale(float pitchHz);
    void applyFormantShift(float* samples, int numSamples, float shiftSemitones);
    float midiNoteToFreq(int note) const;
    int freqToMidiNote(float freq) const;
    
    //==============================================================================
    // Parameters
    std::atomic<float> retuneSpeed_{20.0f};        // 0-100
    std::atomic<float> correctionAmount_{1.0f};    // 0-1
    std::atomic<float> humanize_{0.0f};            // 0-1
    std::atomic<bool> preserveFormants_{true};
    
    // Scale
    std::array<bool, 12> scaleNotes_;
    int rootNote_ = 0;
    bool useScale_ = false;
    
    // State
    double sampleRate_ = 44100.0;
    float currentSemitoneShift_ = 0.0f;
    float targetSemitoneShift_ = 0.0f;
    
    // Pitch smoothing filter
    float smoothedPitch_ = 0.0f;
    float smoothingCoeff_ = 0.5f;
    
    // Pitch correction delay buffer (creates the "step" sound)
    std::vector<float> delayBuffer_;
    int delayWritePos_ = 0;
    int delayReadPos_ = 0;
    int delayLength_ = 512;
    
    // Formant filter state
    struct FormantFilter
    {
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
        
        void setPeaking(float freq, float q, float gain, float sampleRate);
        float process(float input);
    };
    
    std::array<FormantFilter, 4> formantFilters_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassicAutoTune)
};

} // namespace dsp
} // namespace zenith
