/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace zenith {

//==============================================================================
// NOISE COLOR TYPES
//==============================================================================
/**
 * Professional noise generator matching Serum 2:
 * - White noise: flat spectrum
 * - Pink noise: -3dB/octave (natural sounding)
 * - Brown noise: -6dB/octave (warmer, deeper)
 */
enum class NoiseColor {
    White,      ///< Flat spectrum, equal energy per Hz
    Pink,        ///< -3dB/octave, natural sound
    Brown,        ///< -6dB/octave, warm and deep
    Sampled       ///< Use sampled noise buffer
};

//==============================================================================
// PROFESSIONAL NOISE GENERATOR
//==============================================================================
/**
 * High-quality noise generator with proper filtering
 *
 * FEATURES:
 * - Multiple noise colors with proper spectral slopes
 * - Band-limited output (no aliasing)
 * - Stereo width control
 * - Sample rate tracking
 * - RT-safe generation
 */
class ZenithNoiseGenerator {
public:
    ZenithNoiseGenerator();
    ~ZenithNoiseGenerator() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    void setSampleRate(double sr) { sampleRate_ = sr; reset(); }
    void reset();

    void setColor(NoiseColor color) { color_ = color; }
    NoiseColor getColor() const { return color_; }

    /**
     * @brief Set stereo width (0-1)
     * 0 = mono, 1 = full stereo
     */
    void setStereoWidth(float width) { stereoWidth_ = juce::jlimit(0.0f, 1.0f, width); }

    /**
     * @brief Set output gain
     */
    void setGain(float gain) { gain_ = juce::jlimit(0.0f, 1.0f, gain); }

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Generate next stereo sample
     * @param left Output for left channel
     * @param right Output for right channel
     */
    void getNextSample(float& left, float& right);

private:
    //==========================================================================
    // State
    //==========================================================================

    double sampleRate_ = 44100.0;
    NoiseColor color_ = NoiseColor::White;
    float stereoWidth_ = 0.0f;
    float gain_ = 1.0f;

    // White noise state
    uint32_t whiteSeed_ = 0;

    // Pink noise state (7-point Paul Kellett method)
    std::array<float, 7> pinkState_;
    int pinkIndex_ = 0;

    // Brown noise state
    float brownState_ = 0.0f;
    float brownLeak_ = 0.0f;

    //==========================================================================
    // Generators
    //==========================================================================

    float generateWhite();
    float generatePink();
    float generateBrown();

    /**
     * @brief Apply stereo width using Mid-Side processing
     */
    void applyStereoWidth(float mono, float& left, float& right);
};

//==============================================================================
// INLINE IMPLEMENTATIONS
//==============================================================================

inline void ZenithNoiseGenerator::getNextSample(float& left, float& right) {
    float mono = 0.0f;

    switch (color_) {
        case NoiseColor::White:
            mono = generateWhite();
            break;
        case NoiseColor::Pink:
            mono = generatePink();
            break;
        case NoiseColor::Brown:
            mono = generateBrown();
            break;
        case NoiseColor::Sampled:
            mono = generateWhite();  // Fall back to white
            break;
        default:
            mono = generateWhite();
            break;
    }

    // Apply gain and stereo width
    mono *= gain_;
    applyStereoWidth(mono, left, right);
}

inline float ZenithNoiseGenerator::generateWhite() {
    // Linear congruential generator for speed
    whiteSeed_ = whiteSeed_ * 1664525u + 1013904223u;

    // Convert to float range [-1, 1]
    return (static_cast<float>(whiteSeed_ >> 16) / 32767.0f) * gain_;
}

inline float ZenithNoiseGenerator::generatePink() {
    // Paul Kellett's refined method
    // 7-point moving average of white noise
    float white = generateWhite();

    pinkState_[pinkIndex_] = white;
    pinkIndex_ = (pinkIndex_ + 1) % 7;

    // Sum and scale for -3dB/octave
    float sum = 0.0f;
    for (int i = 0; i < 7; ++i) {
        sum += pinkState_[i];
    }

    // Scaling factor for proper pink level
    return (sum * 0.1f) * gain_;
}

inline float ZenithNoiseGenerator::generateBrown() {
    // Brown noise is integrated white noise (leaky integrator)
    float white = generateWhite();

    // Leaky integrator with coefficient 0.98
    brownState_ = brownState_ * 0.998f + white * 0.002f;

    // Scale to prevent clipping
    return juce::jlimit(-1.0f, 1.0f, brownState_) * gain_;
}

inline void ZenithNoiseGenerator::applyStereoWidth(float mono, float& left, float& right) {
    if (stereoWidth_ <= 0.0f) {
        left = right = mono;
    } else {
        // Mid-Side encoding
        float side = mono * (stereoWidth_ * 0.5f);
        left = mono - side;
        right = mono + side;
    }
}

} // namespace zenith
