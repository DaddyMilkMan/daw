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

// MasterLimiter.h - Master limiter for final output stage

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {

/**
 * @class MasterLimiter
 * @brief Master limiter for final output stage
 *
 * This component provides peak protection and loudness maximization
 * for the master output bus.
 */
class MasterLimiter {
public:
    //==============================================================================
    // Construction
    //==============================================================================

    MasterLimiter();
    ~MasterLimiter();

    //==============================================================================
    // Processing
    //==============================================================================

    /**
     * @brief Process audio samples through the limiter
     * @param input Input audio buffer
     * @param output Output audio buffer
     * @param numSamples Number of samples to process
     */
    void process(const float* input, float* output, int numSamples);

    /**
     * @brief Process juce AudioBuffer
     * @param buffer Audio buffer to process
     */
    void process(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    // Parameters
    //==============================================================================

    /**
     * @brief Set the threshold level (dB)
     * @param threshold Threshold in dB
     */
    void setThreshold(float threshold);

    /**
     * @brief Set the ceiling level (dB)
     * @param ceiling Ceiling in dB
     */
    void setCeiling(float ceiling);

    /**
     * @brief Set the attack time (ms)
     * @param attack Attack time in milliseconds
     */
    void setAttack(float attack);

    /**
     * @brief Set the release time (ms)
     * @param release Release time in milliseconds
     */
    void setRelease(float release);

    /**
     * @brief Get the reduction amount (dB)
     * @return Current reduction in dB
     */
    float getReduction() const;

    //==============================================================================
    // Reset
    //==============================================================================

    void reset();
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    // Additional methods for AudioRenderer compatibility
    void initialize(double sampleRate) { prepareToPlay(sampleRate, 512); }
    void setEnabled(bool enabled) { /* Ignored for now */ }
    bool isEnabled() const { return true; }
    float getCeiling() const { return ceiling_; }
    float getGainReduction() const { return reduction_; }
    int getLatencySamples() const { return 0; }

private:
    //==============================================================================
    // Members
    //==============================================================================

    float threshold_{-6.0f};      // Threshold in dB
    float ceiling_{-0.1f};       // Ceiling in dB
    float attack_{5.0f};         // Attack time in ms
    float release_{100.0f};      // Release time in ms
    float reduction_{0.0f};      // Current reduction in dB

    // Internal processing state
    double sampleRate_{44100.0};
    int samplesPerBlock_{512};

    // RMS detector
    float rms_{0.0f};
    float envelope_{0.0f};

    // Gain reduction
    float gainReduction_{1.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterLimiter)
};

} // namespace zenith