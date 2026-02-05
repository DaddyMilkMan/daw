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

    FilterResponseDisplay.h
    Created: 2026-02-01
    Author:  Zenith DAW

    Real-time filter frequency response visualizer for ZenithPolySynth.
    Shows Bode plot (magnitude response) with optional phase overlay.


  ==============================================================================
*/

#pragma once

#include "../../instruments/ZenithPolySynthDefs.h"
#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <complex>
#include <atomic>

#ifdef ZENITH_USE_SKIA
extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <core/SkFont.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#pragma clang diagnostic pop
}
#endif

namespace zenith {

//==============================================================================
/**
    Filter response data computed from filter parameters
*/
struct FilterResponseData {
    std::vector<float> frequencies;     // Frequency points (Hz)
    std::vector<float> magnitudes;      // Magnitude response (dB)
    std::vector<float> phases;          // Phase response (degrees)
    float cutoffFrequency = 1000.0f;    // Current cutoff
    float resonanceValue = 0.5f;        // Current resonance
    FilterType filterType = FilterType::Lowpass;
    FilterModelType filterModel = FilterModelType::MoogLadder;

    void resize(size_t numPoints) {
        frequencies.resize(numPoints);
        magnitudes.resize(numPoints);
        phases.resize(numPoints);
    }
};

//==============================================================================
/**
    Computes frequency response for various filter types
*/
class FilterResponseCalculator {
public:
    /**
        Compute the frequency response for a State Variable Filter
    */
    static void computeSVFResponse(float cutoff, float resonance,
                                   FilterType type, double sampleRate,
                                   std::vector<float>& frequencies,
                                   std::vector<float>& magnitudes,
                                   std::vector<float>& phases);

    /**
        Compute the frequency response for a Moog Ladder filter (4-pole)
    */
    static void computeMoogLadderResponse(float cutoff, float resonance,
                                          double sampleRate,
                                          std::vector<float>& frequencies,
                                          std::vector<float>& magnitudes,
                                          std::vector<float>& phases);

    /**
        Compute frequency response for MS-20 style diode ladder (3-pole)
    */
    static void computeMS20Response(float cutoff, float resonance,
                                    double sampleRate,
                                    std::vector<float>& frequencies,
                                    std::vector<float>& magnitudes,
                                    std::vector<float>& phases);

    /**
        Compute frequency response for Prophet-5 CEM 3320 style (4-pole)
    */
    static void computeProphetResponse(float cutoff, float resonance,
                                       double sampleRate,
                                       std::vector<float>& frequencies,
                                       std::vector<float>& magnitudes,
                                       std::vector<float>& phases);

    /**
        Compute frequency response for SEM State Variable Filter
    */
    static void computeSEMResponse(float cutoff, float resonance,
                                   FilterType type, double sampleRate,
                                   std::vector<float>& frequencies,
                                   std::vector<float>& magnitudes,
                                   std::vector<float>& phases);

    /**
        Compute frequency response for TB-303 diode ladder (3-pole)
    */
    static void computeTB303Response(float cutoff, float resonance,
                                     double sampleRate,
                                     std::vector<float>& frequencies,
                                     std::vector<float>& magnitudes,
                                     std::vector<float>& phases);

private:
    // Helper to convert linear magnitude to dB
    static float magnitudeToDb(float linear) {
        return 20.0f * std::log10(std::max(1e-10f, linear));
    }

    // Helper to compute complex frequency response of a 1-pole lowpass
    static std::complex<float> onePoleLowpass(float omega, float wc);

    // Helper to compute complex frequency response of a 1-pole highpass
    static std::complex<float> onePoleHighpass(float omega, float wc);

    // Helper to compute complex frequency response of a 2-pole SVF
    static void computeSVFTransferFunction(float omega, float wc, float k,
                                           std::complex<float>& Hlp,
                                           std::complex<float>& Hbp,
                                           std::complex<float>& Hhp);
};

//==============================================================================
/**
    Real-time filter frequency response visualization component

    Features:
    - Bode plot showing magnitude (dB) vs log frequency
    - Optional phase response overlay
    - Grid with frequency labels (20Hz - 20kHz)
    - Real-time updates as filter parameters change
    - Support for dual filter visualization
    - Spectrum overlay capability
*/
class FilterResponseDisplay : public SkiaComponent {
public:
    FilterResponseDisplay();
    ~FilterResponseDisplay() override;

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Set the filter parameters to visualize */
    void setFilterParameters(float cutoff, float resonance,
                             FilterType type, FilterModelType model);

    /** Set sample rate for accurate frequency response calculation */
    void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; updateResponse(); }

    /** Enable/disable dual filter mode (show two curves) */
    void setDualFilterMode(bool dual) { dualFilterMode_ = dual; updateResponse(); }

    /** Set second filter parameters (when dual mode enabled) */
    void setFilter2Parameters(float cutoff, float resonance,
                              FilterType type, FilterModelType model);

    /** Show/hide phase response */
    void setShowPhase(bool show) { showPhase_ = show; markDirty(); }

    /** Show/hide grid */
    void setShowGrid(bool show) { showGrid_ = show; markDirty(); }

    /** Set frequency range for display */
    void setFrequencyRange(float minHz, float maxHz);

    /** Set dB range for display */
    void setDbRange(float minDb, float maxDb);

    /** Overlay spectrum data (optional) */
    void setSpectrumData(const std::vector<float>& spectrum);

    //==========================================================================
    // Component overrides
    //==========================================================================

#ifdef ZENITH_USE_SKIA
    void drawSkia(SkCanvas* canvas) override;
#endif

    void timerCallback() override;

private:
    //==========================================================================
    // Internal methods
    //==========================================================================

    /** Update the frequency response based on current parameters */
    void updateResponse();

    /** Compute the response for current settings */
    void computeResponse();

#ifdef ZENITH_USE_SKIA
    /** Draw the background and grid */
    void drawBackground(SkCanvas* canvas);

    /** Draw the frequency grid and labels */
    void drawGrid(SkCanvas* canvas);

    /** Draw the filter response curve */
    void drawResponseCurve(SkCanvas* canvas, const FilterResponseData& data,
                          SkColor curveColor, const SkRect& bounds);

    /** Draw the spectrum overlay */
    void drawSpectrumOverlay(SkCanvas* canvas);

    /** Draw labels */
    void drawLabels(SkCanvas* canvas);

    /** Draw phase response */
    void drawPhaseCurve(SkCanvas* canvas, const FilterResponseData& data,
                       const SkRect& bounds);

    /** Convert frequency to X coordinate */
    float freqToX(float freq) const;

    /** Convert X coordinate to frequency */
    float xToFreq(float x) const;

    /** Convert dB to Y coordinate */
    float dbToY(float db) const;

    /** Convert Y coordinate to dB */
    float yToDb(float y) const;

    /** Format frequency for display */
    juce::String formatFrequency(float freq) const;
#endif

    //==========================================================================
    // Member variables
    //==========================================================================

    // Filter parameters
    std::atomic<float> cutoff_{1000.0f};
    std::atomic<float> resonance_{0.5f};
    std::atomic<float> cutoff2_{2000.0f};
    std::atomic<float> resonance2_{0.3f};
    FilterType filterType_ = FilterType::Lowpass;
    FilterModelType filterModel_ = FilterModelType::MoogLadder;
    FilterType filterType2_ = FilterType::Highpass;
    FilterModelType filterModel2_ = FilterModelType::MoogLadder;

    // Display settings
    double sampleRate_ = 44100.0;
    bool dualFilterMode_ = false;
    bool showPhase_ = false;
    bool showGrid_ = true;
    float minFreq_ = 20.0f;
    float maxFreq_ = 20000.0f;
    float minDb_ = -60.0f;
    float maxDb_ = 10.0f;

    // Computed response data
    FilterResponseData response1_;
    FilterResponseData response2_;
    std::vector<float> spectrumData_;

    // Rendering state
    static constexpr int numFreqPoints_ = 512;
    bool needsUpdate_ = true;

    // Animation state
    float animationPhase_ = 0.0f;  // For animated elements
    float glowPulse_ = 0.0f;       // Pulsing glow effect

    // Colors (premium palette with gradients)
    SkColor backgroundColor_ = SkColorSetARGB(255, 12, 12, 18);
    SkColor gridColor_ = SkColorSetARGB(80, 45, 45, 55);
    SkColor gridMajorColor_ = SkColorSetARGB(120, 65, 65, 75);
    SkColor textColor_ = SkColorSetARGB(220, 165, 165, 175);

    // Curve colors with vibrant, modern look
    SkColor curve1Color_ = SkColorSetARGB(255, 0, 220, 255);   // Bright Cyan
    SkColor curve1GlowColor_ = SkColorSetARGB(180, 0, 200, 255);
    SkColor curve2Color_ = SkColorSetARGB(255, 255, 0, 150);   // Magenta
    SkColor curve2GlowColor_ = SkColorSetARGB(180, 220, 0, 120);
    SkColor phaseColor_ = SkColorSetARGB(200, 255, 220, 0);    // Yellow
    SkColor spectrumColor_ = SkColorSetARGB(100, 120, 255, 100); // Green

    // Premium gradient colors
    SkColor curve1GradientStart_ = SkColorSetARGB(120, 0, 200, 255);
    SkColor curve1GradientEnd_ = SkColorSetARGB(20, 0, 100, 150);
    SkColor curve2GradientStart_ = SkColorSetARGB(120, 255, 0, 120);
    SkColor curve2GradientEnd_ = SkColorSetARGB(20, 100, 0, 80);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterResponseDisplay)
};

} // namespace zenith
