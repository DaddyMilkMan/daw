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

    ProPitchShifter.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Professional pitch shifting using Rubber Band library.
    
    Features:

    - Studio-quality pitch shifting
    - Formant preservation
    - Time stretching capability
    - Low-latency mode
    - Phase-locked processing

    Uses: Rubber Band Library (GPL v2 or later)
    https://breakfastquay.com/rubberband/

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// Try to include Rubber Band, fall back to basic implementation if not available
#if __has_include(<rubberband/RubberBandStretcher.h>)
    #include <rubberband/RubberBandStretcher.h>
    #define ZENITH_HAS_RUBBERBAND 1
#else
    #define ZENITH_HAS_RUBBERBAND 0
    // Forward declaration for when library is not available
    namespace RubberBand {
        class RubberBandStretcher;
    }
#endif

#include <memory>
#include <atomic>

namespace zenith {
namespace dsp {

//==============================================================================
/**
    Professional pitch shifter using Rubber Band library.
    
    This provides studio-quality pitch shifting with formant preservation,
    suitable for professional vocal production.
    
    Key features:
    - Phase-locked pitch shifting
    - Independent formant control
    - Configurable quality vs latency tradeoff
    - Real-time safe processing
*/
class ProPitchShifter
{
public:
    //==============================================================================
    /**
     * Quality preset for pitch shifting
     */
    enum class Quality
    {
        Draft,      // Fastest, lowest quality (for preview)
        Balanced,   // Good balance (default)
        Quality,    // High quality, more CPU
        Maximum     // Maximum quality, highest CPU
    };

    //==============================================================================
    ProPitchShifter();
    ~ProPitchShifter();

    //==============================================================================
    /**
     * @brief Prepare the shifter for processing
     * @param sampleRate Sample rate in Hz
     * @param channels Number of channels (1 or 2)
     * @param maxBlockSize Maximum expected block size
     */
    void prepare(double sampleRate, int channels, int maxBlockSize);
    
    /**
     * @brief Reset internal state
     */
    void reset();

    //==============================================================================
    /**
     * @brief Set pitch shift ratio
     * @param semitones Semitones to shift (negative = down, positive = up)
     * 
     * Example: 12.0 = octave up, -12.0 = octave down, 7.0 = perfect fifth up
     */
    void setPitchSemitones(float semitones);
    
    /**
     * @brief Set pitch ratio directly
     * @param ratio 1.0 = no change, 2.0 = octave up, 0.5 = octave down
     */
    void setPitchRatio(float ratio);

    //==============================================================================
    /**
     * @brief Set formant shift ratio
     * @param ratio 1.0 = no change, preserve vocal character
     * 
     * Values < 1.0 make voice deeper/larger
     * Values > 1.0 make voice brighter/smaller
     */
    void setFormantRatio(float ratio);
    
    /**
     * @brief Enable/disable formant preservation
     */
    void setFormantPreservation(bool preserve);

    //==============================================================================
    /**
     * @brief Set processing quality
     */
    void setQuality(Quality quality);
    
    /**
     * @brief Set latency mode
     * @param lowLatency If true, sacrifices some quality for speed
     */
    void setLowLatencyMode(bool lowLatency);

    //==============================================================================
    /**
     * @brief Get reported latency in samples
     */
    int getLatencySamples() const;
    
    /**
     * @brief Get latency in milliseconds
     */
    float getLatencyMs() const;

    //==============================================================================
    /**
     * @brief Process audio
     * @param input Input audio buffer (interleaved if stereo)
 * @param output Output audio buffer (interleaved if stereo)
     * @param numSamples Number of samples to process
     */
    void process(const float* input, float* output, int numSamples);
    
    /**
     * @brief Process JUCE audio buffer
     */
    void process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output);

    //==============================================================================
    /**
     * @brief Check if the shifter is ready
     */
    bool isReady() const { return stretcher_ != nullptr; }

private:
    //==============================================================================
    void updatePitchRatio();
    void createStretcher();
    
    //==============================================================================
    std::unique_ptr<RubberBand::RubberBandStretcher> stretcher_;
    
    // Parameters
    double sampleRate_ = 44100.0;
    int channels_ = 1;
    int maxBlockSize_ = 512;
    
    std::atomic<float> pitchSemitones_{0.0f};
    std::atomic<float> pitchRatio_{1.0f};
    std::atomic<float> formantRatio_{1.0f};
    std::atomic<bool> formantPreservation_{true};
    
    Quality quality_ = Quality::Balanced;
    bool lowLatencyMode_ = false;
    
    bool needsReset_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProPitchShifter)
};

} // namespace dsp
} // namespace zenith
