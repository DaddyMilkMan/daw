/*
  ==============================================================================

    SkiaSpectrumComponent.h
    Created: 2025-12-08
    Author:  Zenith DAW

    Real-time Spectrum Analyzer using Skia for GPU-accelerated rendering.
    
    Features:
    - FFT-based frequency analysis
    - Gradient-filled frequency curve
    - Smooth peak decay animations
    - 60 FPS rendering target
    
    Thread Safety:
    - FFT processing on UI thread (NOT audio thread!)
    - Audio data passed via lock-free AudioFifo
    - All rendering on Skia/OpenGL thread

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "AudioFifo.h"
#include "ZenithDesignSystem.h"

#include <juce_dsp/juce_dsp.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkPath.h>
#include <include/core/SkPaint.h>
#include <include/effects/SkGradientShader.h>

#include <array>
#include <atomic>
#include <vector>

namespace zenith {

/**
 * @class SkiaSpectrumComponent
 * @brief GPU-accelerated spectrum analyzer with gradient fill
 * 
 * Usage:
 * 1. Call getAudioFifo() to get reference to the audio FIFO
 * 2. Audio thread pushes samples to the FIFO
 * 3. Component automatically processes FFT and renders at 60fps
 */
class SkiaSpectrumComponent : public SkiaComponent {
public:
    //==========================================================================
    // FFT Size options (must be power of 2)
    //==========================================================================
    enum class FFTSize {
        Size256  = 256,
        Size512  = 512,
        Size1024 = 1024,
        Size2048 = 2048
    };

    //==========================================================================
    // Display modes
    //==========================================================================
    enum class DisplayMode {
        Bars,       // Classic bar display
        Curve,      // Smooth curve (default)
        FilledCurve // Curve with gradient fill
    };

    //==========================================================================
    // Construction/Destruction
    //==========================================================================
    
    explicit SkiaSpectrumComponent(FFTSize fftSize = FFTSize::Size1024);
    ~SkiaSpectrumComponent() override;

    //==========================================================================
    // Audio Interface (Thread-Safe)
    //==========================================================================

    /**
     * @brief Get reference to audio FIFO for pushing samples
     * @return Reference to the internal AudioFifo
     * 
     * Usage: Audio thread calls fifo.pushSamples() or pushStereoAsMono()
     */
    AudioFifo& getAudioFifo() { return audioFifo_; }

    //==========================================================================
    // Appearance Settings
    //==========================================================================

    void setDisplayMode(DisplayMode mode) { displayMode_ = mode; markDirty(); }
    DisplayMode getDisplayMode() const { return displayMode_; }

    void setGradientColors(SkColor low, SkColor mid, SkColor high) {
        gradientColorLow_ = low;
        gradientColorMid_ = mid;
        gradientColorHigh_ = high;
        markDirty();
    }

    void setLineColor(SkColor color) { lineColor_ = color; markDirty(); }
    SkColor getLineColor() const { return lineColor_; }

    void setLineWidth(float width) { lineWidth_ = width; markDirty(); }
    float getLineWidth() const { return lineWidth_; }

    void setBackgroundColor(SkColor color) { backgroundColor_ = color; markDirty(); }

    /** Enable/disable peak hold indicators */
    void setPeakHoldEnabled(bool enabled) { peakHoldEnabled_ = enabled; }
    bool isPeakHoldEnabled() const { return peakHoldEnabled_; }

    /** Set peak hold time in milliseconds */
    void setPeakHoldTimeMs(int ms) { peakHoldTimeMs_ = ms; }

    /** Set decay speed (0.0 = instant, 1.0 = no decay) */
    void setDecaySpeed(float speed) { decaySpeed_ = juce::jlimit(0.0f, 0.99f, speed); }

    //==========================================================================
    // Frequency Range
    //==========================================================================

    /** Set frequency range to display (default: 20Hz - 20kHz) */
    void setFrequencyRange(float minHz, float maxHz) {
        minFrequency_ = minHz;
        maxFrequency_ = maxHz;
        markDirty();
    }

    /** Set sample rate (needed for correct frequency mapping) */
    void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }

protected:
    //==========================================================================
    // SkiaComponent Override
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void timerCallback() override;

private:
    //==========================================================================
    // FFT Processing
    //==========================================================================

    void processFFT();
    float frequencyToX(float frequency) const;
    float binToFrequency(int bin) const;
    void updatePeaks();

    //==========================================================================
    // Drawing Helpers
    //==========================================================================

    void drawBars(SkCanvas* canvas);
    void drawCurve(SkCanvas* canvas, bool filled);
    void drawPeaks(SkCanvas* canvas);
    void drawBackground(SkCanvas* canvas);
    void drawFrequencyLabels(SkCanvas* canvas);

    //==========================================================================
    // Member Variables
    //==========================================================================

    // FFT
    const int fftSize_;
    juce::dsp::FFT fft_;
    std::vector<float> fftInput_;       // Windowed input samples
    std::vector<float> fftOutput_;      // Complex FFT output
    std::vector<float> magnitudes_;     // Magnitude spectrum (dB)
    std::vector<float> smoothedMagnitudes_; // Smoothed for display
    juce::dsp::WindowingFunction<float> window_;

    // Audio buffer
    AudioFifo audioFifo_;
    std::vector<float> inputBuffer_;
    int inputWritePos_ = 0;

    // Peak hold
    std::vector<float> peakMagnitudes_;
    std::vector<int> peakHoldCounters_;
    bool peakHoldEnabled_ = true;
    int peakHoldTimeMs_ = 2000; // 2 seconds

    // Appearance
    DisplayMode displayMode_ = DisplayMode::FilledCurve;
    SkColor gradientColorLow_  = design::colors::BLUE;
    SkColor gradientColorMid_  = design::colors::CYAN;
    SkColor gradientColorHigh_ = design::colors::MAGENTA;
    SkColor lineColor_ = design::colors::CYAN;
    SkColor backgroundColor_ = design::colors::BG_DARKEST;
    float lineWidth_ = 2.0f;
    float decaySpeed_ = 0.85f;

    // Frequency range
    float minFrequency_ = 20.0f;
    float maxFrequency_ = 20000.0f;
    double sampleRate_ = 44100.0;

    // Animation
    static constexpr int kTargetFPS = 60;
    static constexpr float kMinDb = -60.0f;
    static constexpr float kMaxDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaSpectrumComponent)
};

} // namespace zenith
