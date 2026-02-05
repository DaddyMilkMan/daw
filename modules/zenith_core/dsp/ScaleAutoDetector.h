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

    ScaleAutoDetector.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Automatic musical scale detection from audio.
    
    Analyzes pitch content to determine:

    - Key/Root note
    - Scale type (major, minor, etc.)
    - Confidence score

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "PitchCorrector.h"  // For Scale definitions
#include <vector>
#include <map>
#include <atomic>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    Scale detection result.
*/
struct ScaleDetectionResult
{
    Note rootNote = Note::C;
    MusicalScale scaleType = MusicalScale::Major;
    float confidence = 0.0f;     // 0-1
    float majorMinorConfidence = 0.0f;  // How sure about major vs minor
    
    juce::String getDescription() const;
    bool isConfident(float threshold = 0.6f) const { return confidence >= threshold; }
};

//==============================================================================
/**
    Automatic scale/key detection.
    
    Analyzes pitch histogram to determine most likely key and scale.
    Useful for:
    - Auto-setting pitch correction scale
    - Key detection for DJ mixing
    - Music analysis
*/
class ScaleAutoDetector
{
public:
    //==============================================================================
    ScaleAutoDetector();
    ~ScaleAutoDetector();

    //==============================================================================
    /**
     * @brief Reset analysis
     */
    void reset();
    
    /**
     * @brief Add a pitch sample for analysis
     * @param pitchHz Pitch in Hz (0 for unvoiced/silence)
     * @param confidence Detection confidence (0-1)
     */
    void addPitchSample(float pitchHz, float confidence = 1.0f);
    
    /**
     * @brief Analyze a buffer of audio
     */
    void analyzeAudio(const juce::AudioBuffer<float>& audio, double sampleRate);
    
    /**
     * @brief Finish analysis and get result
     */
    ScaleDetectionResult getResult() const;
    
    /**
     * @brief Get the detected scale (convenience)
     */
    Scale getDetectedScale() const;

    //==============================================================================
    /**
     * @brief Get current analysis progress
     */
    int getSampleCount() const { return sampleCount_.load(); }
    
    /**
     * @brief Check if enough samples for reliable detection
     */
    bool hasEnoughSamples(int minSamples = 500) const { 
        return sampleCount_.load() >= minSamples; 
    }
    
    /**
     * @brief Set minimum pitch confidence to include in analysis
     */
    void setMinConfidence(float minConf) { minConfidence_ = juce::jlimit(0.0f, 1.0f, minConf); }

    //==============================================================================
    /**
     * @brief Get pitch histogram for visualization
     */
    std::array<float, 12> getPitchHistogram() const;
    
    /**
     * @brief Get best matching scales (top 3)
     */
    std::vector<std::pair<ScaleDetectionResult, float>> getTopMatches(int numMatches = 3) const;

private:
    //==============================================================================
    void updateHistogram(float pitchHz, float confidence);
    float calculateScaleMatch(const std::array<float, 12>& histogram, 
                               Note root, MusicalScale scale) const;
    int freqToNoteClass(float freqHz) const;
    
    //==============================================================================
    // Pitch histogram (chroma) - accumulated note classes
    std::array<float, 12> pitchHistogram_;
    
    // Total confidence-weighted samples
    std::atomic<int> sampleCount_{0};
    float totalWeight_ = 0.0f;
    
    // Settings
    float minConfidence_ = 0.5f;
    
    // Scale profiles for matching
    static const std::array<float, 12> majorProfile_;
    static const std::array<float, 12> minorProfile_;
    static const std::array<float, 12> minorHarmonicProfile_;
    static const std::array<float, 12> dorianProfile_;
    static const std::array<float, 12> mixolydianProfile_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleAutoDetector)
};

} // namespace dsp
} // namespace zenith
