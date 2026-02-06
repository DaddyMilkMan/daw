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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <atomic>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    Physical vocal tract model for timbre shaping.
    
    Models the throat as a series of cylindrical sections with varying
    diameter, simulating the acoustic properties of the human vocal tract.
    
    Parameters:
    - Throat Length: Longer = darker, more resonant
    - Throat Width: Wider = breathier, less focused
    - Breathiness: Add aspiration noise
    - Character: Blend between different vocal tract shapes
*/
class ThroatModel
{
public:
    //==============================================================================
    ThroatModel();
    ~ThroatModel();

    //==============================================================================
    /**
     * @brief Prepare for processing
     */
    void prepare(double sampleRate, int maxBlockSize);
    
    /**
     * @brief Reset internal state
     */
    void reset();

    //==============================================================================
    /**
     * @brief Process audio through vocal tract model
     */
    void process(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    /**
     * @brief Set throat length (0-1)
     * 0.0 = short (soprano/child)
     * 0.5 = medium (typical adult)
     * 1.0 = long (bass/large vocal tract)
     */
    void setLength(float length) { length_ = juce::jlimit(0.0f, 1.0f, length); }
    float getLength() const { return length_.load(); }
    
    /**
     * @brief Set throat width/openness (0-1)
     * 0.0 = narrow/tight (focused, bright)
     * 0.5 = typical
     * 1.0 = wide/open (breathy, dark)
     */
    void setWidth(float width) { width_ = juce::jlimit(0.0f, 1.0f, width); }
    float getWidth() const { return width_.load(); }
    
    /**
     * @brief Set breathiness amount (0-1)
     * Adds aspiration noise for airy quality
     */
    void setBreathiness(float breathiness) { 
        breathiness_ = juce::jlimit(0.0f, 1.0f, breathiness); 
    }
    float getBreathiness() const { return breathiness_.load(); }
    
    /**
     * @brief Set vocal character/vowel shape (0-1)
     * Blends between different vocal tract configurations
     * 0.0 = "ah" (open)
     * 0.5 = "ee" (closed, bright)
     * 1.0 = "oo" (rounded, dark)
     */
    void setCharacter(float character) { 
        character_ = juce::jlimit(0.0f, 1.0f, character); 
    }
    float getCharacter() const { return character_.load(); }
    
    /**
     * @brief Set formant shift (-12 to +12 semitones)
     * Shifts all formants up or down
     */
    void setFormantShift(float semitones) { 
        formantShift_ = juce::jlimit(-12.0f, 12.0f, semitones); 
    }
    float getFormantShift() const { return formantShift_.load(); }
    
    /**
     * @brief Enable/disable the throat model
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_.load(); }

    //==============================================================================
    /**
     * @brief Load a preset vocal tract shape
     */
    enum class Preset
    {
        Default,
        Soprano,      // Short, narrow tract
        Alto,         // Medium-short
        Tenor,        // Medium
        Baritone,     // Longer
        Bass,         // Long, wide
        Child,        // Short, small
        Monster,      // Very long, very wide
        Robot,        // Uniform tube
        Telephone     // Bandlimited
    };
    
    void loadPreset(Preset preset);

private:
    //==============================================================================
    void updateVocalTractShape();
    void processTubeModel(float* samples, int numSamples);
    void addBreathiness(float* samples, int numSamples);
    void applyFormantShift(float* samples, int numSamples);
    
    //==============================================================================
    // Vocal tract sections (modeled as cylindrical tubes)
    static constexpr int kNumSections = 8;
    
    struct TubeSection
    {
        float area = 1.0f;       // Cross-sectional area
        float reflection = 0.0f;  // Reflection coefficient
        std::vector<float> forwardDelay;
        std::vector<float> backwardDelay;
        int writePos = 0;
    };
    
    std::array<TubeSection, kNumSections> sections_;
    
    //==============================================================================
    // Parameters
    std::atomic<bool> enabled_{false};
    std::atomic<float> length_{0.5f};       // 0-1
    std::atomic<float> width_{0.5f};        // 0-1
    std::atomic<float> breathiness_{0.0f};  // 0-1
    std::atomic<float> character_{0.5f};    // 0-1
    std::atomic<float> formantShift_{0.0f}; // -12 to +12 semitones
    
    //==============================================================================
    // State
    double sampleRate_ = 44100.0;
    
    // Formant filters
    struct FormantFilter
    {
        float frequency = 1000.0f;
        float bandwidth = 100.0f;
        float gain = 1.0f;
        
        // Filter state (biquad)
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
        
        void calculateCoefficients();
        float process(float input);
    };
    
    std::array<FormantFilter, 4> formants_;  // F1, F2, F3, F4
    
    // Noise generator for breathiness
    juce::Random noiseGenerator_;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThroatModel)
};

} // namespace dsp
} // namespace zenith
