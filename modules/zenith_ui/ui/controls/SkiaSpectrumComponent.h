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

#include "SkiaComponent.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>
#include <zenith_core/dsp/AudioFifo.h>

namespace zenith {

//==============================================================================
class SkiaSpectrumComponent : public SkiaComponent {
public:
    enum class DisplayMode {
        Bars,
        Curve,
        FilledCurve
    };
    
    enum class FFTSize {
        Size256 = 8,
        Size512 = 9,
        Size1024 = 10,
        Size2048 = 11
    };

    explicit SkiaSpectrumComponent(FFTSize fftSize = FFTSize::Size512);
    ~SkiaSpectrumComponent() override;

    // Component overrides
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    // Timer callback for FFT processing and repaint
    void timerCallback() override;

    // Configuration
    void setDisplayMode(DisplayMode mode) { displayMode_ = mode; }
    void setGradientColors(SkColor top, SkColor bottom);
    void setDecaySpeed(float speed); // 0.0 to 1.0
    void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }

    // Audio Data Input
    // Lock-free FIFO: Push samples here from audio thread
    AudioFifo& getAudioFifo() { return audioFifo_; }
    
    // Push a block of samples (convenience wrapper)
    void pushSamples(const juce::AudioBuffer<float>& buffer);

private:
    void processFFT();
    void updateVisualData();
    float getFrequencyForIndex(int index) const;
    int getIndexForFrequency(float freq) const;
    
    // FFT
    juce::dsp::FFT forwardFFT_;
    juce::dsp::WindowingFunction<float> window_;
    AudioFifo audioFifo_;
    std::vector<float> fftData_;     // Complex/Raw data
    std::vector<float> scopeData_;   // Time domain data
    bool nextFFTBlockReady_ = false;
    
    // Visualization
    std::vector<float> logFrequencyData_; // Mapped to logical display buckets
    std::vector<float> peakData_;         // Peak hold values
    std::vector<int> peakHoldCounters_;   // Counters for peak hold
    
    // Settings
    DisplayMode displayMode_ = DisplayMode::FilledCurve;
    SkColor colorTop_ = SkColorSetARGB(255, 0, 255, 255); // Cyan
    SkColor colorBottom_ = SkColorSetARGB(50, 0, 255, 255); // Transparent Cyan
    
    double sampleRate_ = 48000.0;
    float decaySpeed_ = 0.85f;
    int peakHoldTime_ = 60; // Frames
    
    // Layout cache
    std::vector<float> bucketFrequencies_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaSpectrumComponent)
};

} // namespace zenith
