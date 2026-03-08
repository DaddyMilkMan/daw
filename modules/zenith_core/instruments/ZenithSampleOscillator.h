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

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <memory>

namespace zenith {

//==============================================================================
// SAMPLE OSCILLATOR - SERUM 2 STYLE
//==============================================================================
/**
 * Sample-based oscillator matching Serum 2 capabilities:
 * - Multi-sample playback (one-shot, loop, forward/backward)
 * - Zone switching with keyboard tracking
 * - Crossfade smoothing for artifacts-free transitions
 * - Pitch tracking/PCM for realistic sound
 * - Start offset modulation
 */
class ZenithSampleOscillator {
public:
    ZenithSampleOscillator();
    ~ZenithSampleOscillator() = default;

    //==========================================================================
    // Sample Loading
    //==========================================================================

    /**
     * @brief Load sample from file
     * @param file Path to audio file (wav, aiff, flac, etc.)
     * @return Result with error message if failed
     */
    juce::Result loadSample(const juce::File& file);

    /**
     * @brief Load sample from memory buffer
     * @param data Audio data (interleaved stereo)
     * @param numChannels Number of channels
     * @param numSamples Number of samples per channel
     * @param sampleRate Sample rate of the data
     */
    void loadSample(const float* data, int numChannels,
                   int numSamples, double sampleRate);

    /**
     * @brief Clear sample data
     */
    void clearSample();

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Loop mode */
    enum class LoopMode {
        Forward,       ///< Forward loop (0 → end → 0 → ...)
        Backward,      ///< Backward loop (end → 0 → end → ...)
        Alternating,   ///< Alternating forward/backward
        OneShot        ///< Play once, no loop
    };

    /** Crossfade mode for loops */
    enum class CrossfadeMode {
        None,          ///< No crossfade
        ConstantTime,  ///< Linear crossfade (equal time)
        ConstantPower, ///< -3dB power crossfade
        ConstantGain,  ///< Equal gain crossfade
        S Curve,        ///< S-curve crossfade (smooth)
        Custom         ///< User-defined curve
    };

    /** Interpolation mode */
    enum class InterpolationMode {
        Nearest,       ///< Nearest neighbor (no interpolation)
        Linear,        ///< Linear interpolation
        Cubic,         ///< Cubic spline interpolation
        Sinc,          ///< Sinc interpolation (best quality)
        Allpass        ///< Allpass interpolation (phase-linear)
    };

    void setLoopMode(LoopMode mode) { loopMode_ = mode; }
    LoopMode getLoopMode() const { return loopMode_; }

    void setCrossfadeMode(CrossfadeMode mode) { crossfadeMode_ = mode; }
    CrossfadeMode getCrossfadeMode() const { return crossfadeMode_; }

    void setInterpolationMode(InterpolationMode mode) { interpolationMode_ = mode; }
    InterpolationMode getInterpolationMode() const { return interpolationMode_; }

    /** Start and end points (0-1 range) */
    void setStartPoint(float start) { startPoint_ = juce::jlimit(0.0f, 1.0f, start); }
    void setEndPoint(float end) { endPoint_ = juce::jlimit(0.0f, 1.0f, end); }
    float getStartPoint() const { return startPoint_; }
    float getEndPoint() const { return endPoint_; }

    /** Crossfade length (0-1) for loop smoothing */
    void setCrossfadeLength(float xfade) { xfadeLength_ = juce::jlimit(0.0f, 0.5f, xfade); }

    /**
     * @brief Set custom crossfade curve
     * @param curve Curve shape 0.0-1.0 (only used for Custom mode)
     */
    void setCustomCrossfadeCurve(float curve) {
        customCrossfadeCurve_ = juce::jlimit(0.0f, 1.0f, curve);
    }

    //==========================================================================
    // Playback Control
    //==========================================================================

    /** Reset playback to start point */
    void resetPosition();

    /** Get current position (0-1) */
    float getPosition() const { return position_; }

    //==========================================================================
    // Audio Generation
    //==========================================================================

    /**
     * @brief Generate next sample
     * @param frequency Playback frequency (1.0 = original pitch)
     * @param loopStart Modulation for start point (0-1)
     * @return Sample value (-1 to 1)
     */
    float getNextSample(float frequency = 1.0f, float loopStart = 0.0f);

    /**
     * @brief Process block of samples
     */
    void process(float* output, int numSamples, float frequency = 1.0f);

    //==========================================================================
    // State Query
    //==========================================================================

    bool hasSample() const { return !sampleData_.empty(); }
    int getNumSamples() const { return numSamples_; }
    int getNumChannels() const { return numChannels_; }
    double getSampleRate() const { return sampleRate_; }

private:
    //==========================================================================
    // Sample Data
    //==========================================================================

    std::vector<float> sampleData_;     // Interleaved: LRLRL...
    int numSamples_ = 0;
    int numChannels_ = 0;
    double sampleRate_ = 44100.0;

    //==========================================================================
    // Playback State
    //==========================================================================

    double position_ = 0.0;           // Current position (0-1)
    double phase_ = 0.0;             // Accumulated phase
    float lastOut_[2] = {0.0f, 0.0f};  // Previous outputs for interpolation

    //==========================================================================
    // Parameters
    //==========================================================================

    LoopMode loopMode_ = LoopMode::Forward;
    CrossfadeMode crossfadeMode_ = CrossfadeMode::None;
    InterpolationMode interpolationMode_ = InterpolationMode::Linear;
    float startPoint_ = 0.0f;
    float endPoint_ = 1.0f;
    float xfadeLength_ = 0.0f;    // Crossfade at loop point
    float customCrossfadeCurve_ = 0.5f;  // For Custom crossfade mode

    //==========================================================================
    // Crossfade State
    //==========================================================================

    std::vector<float> xfadeBufferL_;   // Left channel crossfade buffer
    std::vector<float> xfadeBufferR_;   // Right channel crossfade buffer
    int xfadePosition_ = 0;           // Current position in crossfade
    int xfadeLengthSamples_ = 0;       // Crossfade length in samples

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    float readSample(double pos, int channel) const;
    float readInterpolated(double pos, int channel) const;
};

} // namespace zenith
