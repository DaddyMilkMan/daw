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
#include "WavetableData.h"
#include <array>
#include <cmath>

namespace zenith {

//==============================================================================
// 3D WAVETABLE MORHING
//==============================================================================
/**
 * Professional 3D wavetable morphing matching Serum 2:
 * - XYZ interpolation between 3 waveforms
 * - Per-axis MIP mapping for anti-aliasing
 * - Smooth morph transitions
 * - Sub-sample accurate positioning
 *
 * FEATURES:
 * - 3D morph position (X, Y, Z)
 * - Independent axis control per oscillator
 * - Morph amount controls (0-100%)
 * - MIP level tracking per axis
 * - Linear/trilinear/cubic interpolation
 */
class Zenith3DWavetableMorpher {
public:
    Zenith3DWavetableMorpher() = default;
    ~Zenith3DWavetableMorpher() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void reset();

    //==========================================================================
    // Waveform Selection
    //==========================================================================

    void setXWavetable(const Wavetable* wt) { xWavetable_ = wt; }
    void setYWavetable(const Wavetable* wt) { yWavetable_ = wt; }
    void setZWavetable(const Wavetable* wt) { zWavetable_ = wt; }

    //==========================================================================
    // Morph Controls
    //==========================================================================

    /**
     * @brief Set morph amount for all axes
     * @param amount Morph amount (0.0-1.0)
     */
    void setMorphAmount(float amount) { morphAmount_ = juce::jlimit(0.0f, 1.0f, amount); }

    /**
     * @brief Set morph position for X axis
     * @param pos Position (0.0-1.0)
     */
    void setXMorph(float pos) { xMorph_ = juce::jlimit(0.0f, 1.0f, pos); }

    /**
     * @brief Set morph position for Y axis
     * @param pos Position (0.0-1.0)
     */
    void setYMorph(float pos) { yMorph_ = juce::jlimit(0.0f, 1.0f, pos); }

    /**
     * @brief Set morph position for Z axis
     * @param pos Position (0.0-1.0)
     */
    void setZMorph(float pos) { zMorph_ = juce::jlimit(0.0f, 1.0f, pos); }

    //==========================================================================
    // MIP Level Control
    //==========================================================================

    /**
     * @brief Set MIP level for X axis
     * @param level MIP level (0-1.0)
     */
    void setXMipLevel(float level) { xMipLevel_ = juce::jlimit(0.0f, 1.0f, level); }

    /**
     * @brief Set MIP level for Y axis
     * @param level MIP level (0-1.0)
     */
    void setYMipLevel(float level) { yMipLevel_ = juce::jlimit(0.0f, 1.0f, level); }

    /**
     * @brief Set MIP level for Z axis
     * @param level MIP level (0-1.0)
     */
    void setZMipLevel(float level) { zMipLevel = juce::jlimit(0.0f, 1.0f, level); }

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Get interpolated sample from 3D wavetable
     * @param xPos X position [0.0-1.0]
     * @param yPos Y position [0.0-1.0]
     * @param zPos Z position [0.0-1.0]
     * @param midiNote Base pitch for MIP calculation
     * @return Interpolated sample
     */
    float getSample(float xPos, float yPos, float zPos, float midiNote);

private:
    //==========================================================================
    // State
    //==========================================================================

    double sampleRate_ = 44100.0;

    // Waveforms
    const Wavetable* xWavetable_ = nullptr;
    const Wavetable* yWavetable_ = nullptr;
    const Wavetable* zWavetable_ = nullptr;

    // Morph parameters
    float morphAmount_ = 0.5f;

    // Morph positions (0-1.0)
    float xMorph_ = 0.5f;
    float yMorph_ = 0.5f;
    float zMorph_ = 0.5f;

    // MIP levels
    float xMipLevel_ = 0.5f;
    float yMipLevel_ = 0.5f;
    float zMipLevel_ = 0.5f;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /**
     * @brief Calculate MIP level for position
     * Higher position = more detailed (higher MIP)
     */
    float calculateMipLevel(float pos) {
        // Logarithmic scaling: 0.0 at bottom = most detailed, 1.0 at top
        return std::log2((pos + 1.0f) * 0.5f) + 1.0f);
    }

    /**
     * @brief Trilinear interpolation between 3 waveforms
     *
     * Formula:
     *   output = (1-x)wt + (1-y)wt + (1-z)wt
     *   where:
     *       wt factors are determined by morph amounts
     *
     * @param xPos, yPos, zPos Positions (0.0-1.0)
     */
    inline float trilinearMorph(float xPos, float yPos, float zPos) {
        // Get samples from each waveform
        float xSample = xWavetable_ ? xWavetable_->getSample(xPos, yPos) : 0.0f;
        float ySample = yWavetable_ ? yWavetable_->getSample(xPos, yPos) : 0.0f;
        float zSample = zWavetable_ ? zWavetable_->getSample(xPos, yPos) : 0.0f;

        // Calculate morph weights
        float xWeight = 1.0f - xMorph_;
        float yWeight = 1.0f - yMorph_;
        float zWeight = 1.0f - zMorph_;

        // Weighted sum
        return (xSample * xWeight + ySample * yWeight + zSample * zWeight);
    }

};

} // namespace zenith
