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
#include <atomic>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    Real-time pitch detector optimized for vocal processing.
    
    Uses YIN (YINdeX) algorithm with optimizations for:
    - Low latency (configurable buffer size)
    - Fast processing (early termination)
    - Accuracy in vocal range (80Hz - 1000Hz)
    
    Features:
    - Confidence threshold (0.0 - 1.0)
    - Adjustable buffer size
    - Real-time safe (no allocations during processing)
*/
class PitchDetector
{
public:
    //==============================================================================
    PitchDetector();
    ~PitchDetector();

    //==============================================================================
    /**
     * @brief Prepare the detector for processing
     * @param sampleRate Sample rate in Hz
     * @param bufferSize Size of analysis buffer (larger = more accurate, more latency)
     */
    void prepare(double sampleRate, int bufferSize = 2048);
    
    /**
     * @brief Reset internal state
     */
    void reset();

    //==============================================================================
    /**
     * @brief Process a sample and detect pitch
     * @param sample Input audio sample
     * @return Detected frequency in Hz, or 0.0 if no pitch detected
     */
    float processSample(float sample);
    
    /**
     * @brief Process a block of samples (more efficient)
     * @param buffer Input audio buffer
     * @return Detected frequency (uses center of buffer), or 0.0 if no pitch
     */
    float processBlock(const juce::AudioBuffer<float>& buffer);

    //==============================================================================
    /**
     * @brief Get the last detected pitch
     */
    float getLastPitch() const { return lastPitch_.load(); }
    
    /**
     * @brief Get confidence of last detection (0.0 - 1.0)
     */
    float getConfidence() const { return confidence_.load(); }
    
    /**
     * @brief Check if currently detecting a valid pitch
     */
    bool isVoiced() const { return voiced_.load(); }

    //==============================================================================
    /**
     * @brief Set minimum detectable frequency (default 80Hz)
     */
    void setMinFrequency(float freqHz) { minFreqHz_ = freqHz; }
    
    /**
     * @brief Set maximum detectable frequency (default 1000Hz)
     */
    void setMaxFrequency(float freqHz) { maxFreqHz_ = freqHz; }
    
    /**
     * @brief Set confidence threshold (0.0 - 1.0, default 0.7)
     */
    void setConfidenceThreshold(float threshold) { 
        confidenceThreshold_ = juce::jlimit(0.0f, 1.0f, threshold); 
    }

private:
    //==============================================================================
    // YIN algorithm implementation
    void updateCircularBuffer(float sample);
    float detectPitchYIN();
    float parabolicInterpolation(int tau, float y1, float y2, float y3);
    
    //==============================================================================
    // Parameters
    double sampleRate_ = 44100.0;
    int bufferSize_ = 2048;
    float minFreqHz_ = 80.0f;      // ~E2, lowest guitar/vocal note
    float maxFreqHz_ = 1000.0f;    // ~C6, highest soprano
    float confidenceThreshold_ = 0.7f;
    
    // State
    std::vector<float> circularBuffer_;
    int writePos_ = 0;
    std::atomic<float> lastPitch_{0.0f};
    std::atomic<float> confidence_{0.0f};
    std::atomic<bool> voiced_{false};
    
    // YIN working buffers (pre-allocated)
    std::vector<float> differenceBuffer_;
    std::vector<float> cumulativeBuffer_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDetector)
};

} // namespace dsp
} // namespace zenith
