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
#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace zenith {

//==============================================================================
// FILTER OUTPUT TYPE
//==============================================================================
/**
 * Output selection for all filter models
 * Allows accessing different filter outputs simultaneously
 */
enum class FilterOutput {
    Low,        ///< Low pass output
    High,        ///< High pass output
    Band,        ///< Band pass output
    Notch         ///< Notch output
    All          ///< Mix of all outputs (where applicable)
};

//==============================================================================
// SATURATION CURVE TYPE
//==============================================================================
/**
 * Professional saturation curves for filter drive
 * Matching Serum 2 drive quality
 */
enum class SaturationCurve {
    Soft,         ///< Smooth tanh saturation (warm)
    Hard,         ///< Hard clipping (aggressive)
    Wavefold,     ///< Wavefolding distortion (metallic)
    Sine,         ///< Sine shaping (smooth)
    Cubic,        ///< Cubic shaping (odd harmonic)
    Asymmetric     ///< Different curves for positive/negative
};

//==============================================================================
// KEYTRACKING CURVE TYPE
//==============================================================================
/**
 * Curve options for filter keytracking
 * Allows different response curves to pitch
 */
enum class KeytrackCurve {
    Linear,        ///< Linear tracking (1:1)
    Exponential,    ///< Exponential (faster at high pitch)
    ReverseExp,     ///< Reverse exponential (slower at high pitch)
    Custom         ///< User-definable curve
};

//==============================================================================
// PROFESSIONAL FILTER WITH ENHANCEMENTS
//==============================================================================
/**
 * Enhanced filter with:
 * - Saturation curves (tanh, hard, wavefold, cubic, asymmetric)
 * - Filter output selection (low, high, band, notch, all)
 * - Keytracking curve options (linear, exp, reverse exp)
 * - Oversampling (1x, 2x, 4x, 8x)
 */
class ZenithFilterEnhanced {
public:
    ZenithFilterEnhanced() = default;

    //==========================================================================
    // Filter Type
    //==========================================================================

    void setType(FilterType type) { type_ = type; }
    FilterType getType() const { return type_; }

    void setModel(FilterModelType model) { model_ = model; }
    FilterModelType getModel() const { return model_; }

    //==========================================================================
    // Filter Parameters
    //==========================================================================

    void setCutoff(float cutoffHz) {
        cutoff_ = juce::jlimit(20.0f, 20000.0f, cutoffHz);
    }
    float getCutoff() const { return cutoff_; }

    void setResonance(float res) {
        resonance_ = juce::jlimit(0.0f, 1.0f, res);
    }
    float getResonance() const { return resonance_; }

    /**
     * @brief Set drive amount with saturation curve
     * @param drive 0.0-1.0
     * @param curve Saturation curve type
     */
    void setDrive(float drive, SaturationCurve curve);
    float getDrive() const { return drive_; }

    /**
     * @brief Set keytracking with curve type
     * @param amount 0.0-1.0 (0=off, 1=full)
     * @param curve Keytracking curve type
     */
    void setKeytrack(float amount, KeytrackCurve curve);
    float getKeytrackAmount() const { return keytrackAmount_; }

    /**
     * @brief Set filter output type
     * @param output Which filter output to use
     */
    void setOutput(FilterOutput output) { output_ = output; }
    FilterOutput getOutput() const { return output_; }

    //==========================================================================
    // Oversampling
    //==========================================================================

    void setOversampling(int factor);
    int getOversampling() const { return oversamplingFactor_; }

    //==========================================================================
    // Sample Rate
    //==========================================================================

    void setSampleRate(double sr);
    void reset();

    //==========================================================================
    // Processing
    //==========================================================================

    float processSample(float input, float midiNote);
    void process(juce::AudioBuffer<float>& buffer, float midiNote);

private:
    //==========================================================================
    // State Variables
    //==========================================================================

    FilterType type_ = FilterType::LowPass;
    FilterModelType model_ = FilterModelType::SVF;
    float cutoff_ = 1000.0f;
    float resonance_ = 0.5f;
    float drive_ = 0.0f;
    float keytrackAmount_ = 0.0f;
    float keytrackCurveParam_ = 0.5f;

    SaturationCurve driveCurve_ = SaturationCurve::Soft;
    KeytrackCurve keytrackCurve_ = KeytrackCurve::Linear;
    FilterOutput output_ = FilterOutput::Low;

    int oversamplingFactor_ = 1;
    double sampleRate_ = 44100.0;

    //==========================================================================
    // Filter State (SVF)
    //==========================================================================

    struct SVFState {
        double low = 0.0;
        double band = 0.0;
        double high = 0.0;
    } svf_;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    float calculateCutoffWithKeytrack(float midiNote);
    float applySaturation(float sample, SaturationCurve curve);

    // Saturation processors
    inline float softClip(float x) { return std::tanh(x); }
    inline float hardClip(float x) { return juce::jlimit(-1.0f, 1.0f, x); }
    inline float wavefold(float x);
    inline float cubicClip(float x) { return x - x * x * x * 0.333f; }
    inline float sineShape(float x) { return std::sin(x * 1.57f); }
    inline float asymmetricClip(float x);

    // Processors per model
    float processSVF(float input, float midiNote);
    float processMoog(float input, float midiNote);
    float processMS20(float input, float midiNote);
    float processSEM(float input, float midiNote);
    float processTB303(float input, float midiNote);

    // Output mixers
    float getOutput(float low, float high, float band, float notch);
};

} // namespace zenith
