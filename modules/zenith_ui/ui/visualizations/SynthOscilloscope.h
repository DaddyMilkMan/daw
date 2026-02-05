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

    SynthOscilloscope.h
    Created: 2026-02-01
    Author:  Zenith DAW

    Real-time oscilloscope for ZenithPolySynth output.
    Shows waveform with trigger, time scale, and X-Y mode.


  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include <memory>

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
#pragma clang diagnostic pop
}
#endif

namespace zenith {

//==============================================================================
/**
    Lock-free FIFO for oscilloscope data
*/
class OscilloscopeBuffer {
public:
    OscilloscopeBuffer() : buffer_(4096), writePos_(0), readPos_(0) {}

    /** Push samples into the buffer (call from audio thread) */
    void push(const float* samples, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            buffer_[writePos_] = samples[i];
            writePos_ = (writePos_ + 1) % buffer_.size();
        }
    }

    /** Push stereo samples */
    void pushStereo(const float* left, const float* right, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            buffer_[writePos_] = left[i];
            writePos_ = (writePos_ + 1) % buffer_.size();

            buffer_[writePos_] = right[i];
            writePos_ = (writePos_ + 1) % buffer_.size();
        }
    }

    /** Read samples from buffer (call from UI thread) */
    std::vector<float> read(int numSamples) {
        std::vector<float> result;
        result.reserve(numSamples);

        int readPos = readPos_.load();
        int writePos = writePos_.load();

        for (int i = 0; i < numSamples; ++i) {
            if (readPos == writePos) break;
            result.push_back(buffer_[readPos]);
            readPos = (readPos + 1) % buffer_.size();
        }

        readPos_.store(readPos);
        return result;
    }

    /** Get read position without consuming */
    int getWritePosition() const { return writePos_.load(); }

    void clear() {
        readPos_.store(0);
        writePos_.store(0);
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

private:
    std::vector<float> buffer_;
    std::atomic<int> writePos_;
    std::atomic<int> readPos_;
};

//==============================================================================
/**
    Trigger modes for oscilloscope
*/
enum class TriggerMode {
    Free,      // No triggering
    Auto,      // Auto-trigger on signal crossing
    Normal,    // Trigger only on crossing
    Single     // Single shot trigger
};

//==============================================================================
/**
    Real-time oscilloscope component for synth output

    Features:
    - Large waveform display
    - Trigger modes (free, auto, normal)
    - Time base control (zoom)
    - X-Y mode for stereo correlation
    - Waveform overlay
    - Grid with voltage markers
*/
class SynthOscilloscope : public SkiaComponent {
public:
    SynthOscilloscope();
    ~SynthOscilloscope() override;

    //==========================================================================
    // Audio Input
    //==========================================================================

    /** Add audio data for visualization (call from audio thread) */
    void addAudioData(const juce::AudioBuffer<float>& buffer);

    /** Add audio data (mono) */
    void addAudioData(const float* samples, int numSamples);

    /** Clear the display buffer */
    void clear();

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Set time scale (zoom) - samples per display */
    void setTimeScale(float scale) { timeScale_ = juce::jlimit(100.0f, 10000.0f, scale); }

    /** Set trigger level */
    void setTriggerLevel(float level) { triggerLevel_ = juce::jlimit(-1.0f, 1.0f, level); }

    /** Set trigger mode */
    void setTriggerMode(TriggerMode mode) { triggerMode_ = mode; }

    /** Enable/disable X-Y mode (stereo correlation) */
    void setXYMode(bool enabled) { xyMode_ = enabled; }

    /** Enable/disable grid */
    void setShowGrid(bool show) { showGrid_ = show; markDirty(); }

    /** Set waveform color */
    void setWaveformColor(SkColor color) { waveformColor_ = color; markDirty(); }

    /** Set secondary waveform color (for stereo) */
    void setSecondaryWaveformColor(SkColor color) { secondaryColor_ = color; markDirty(); }

    /** Enable waveform persistence (trace effect) */
    void setPersistence(float persistence) {
        persistence_ = juce::jlimit(0.0f, 1.0f, persistence);
    }

    //==========================================================================
    // Component overrides
    //==========================================================================

#ifdef ZENITH_USE_SKIA
    void drawSkia(SkCanvas* canvas) override;
#endif

    void timerCallback() override;

private:
#ifdef ZENITH_USE_SKIA
    //==========================================================================
    // Drawing methods
    //==========================================================================

    /** Draw the background and grid */
    void drawBackground(SkCanvas* canvas);

    /** Draw the grid with voltage markers */
    void drawGrid(SkCanvas* canvas);

    /** Draw the waveform */
    void drawWaveform(SkCanvas* canvas);

    /** Draw X-Y mode display */
    void drawXYDisplay(SkCanvas* canvas);

    /** Draw trigger indicator */
    void drawTriggerIndicator(SkCanvas* canvas);

    /** Draw time scale info */
    void drawInfo(SkCanvas* canvas);

    /** Draw CRT-style scanlines overlay */
    void drawScanlines(SkCanvas* canvas);

    /** Draw frequency detection info */
    void drawFrequencyInfo(SkCanvas* canvas);

    /** Find trigger position in buffer */
    int findTriggerPosition(const std::vector<float>& buffer);

    /** Detect fundamental frequency from buffer */
    float detectFrequency(const std::vector<float>& buffer);

    /** Convert sample value to Y coordinate */
    float sampleToY(float sample) const;

    /** Convert Y coordinate to sample value */
    float yToSample(float y) const;

    /** Convert sample index to X coordinate */
    float indexToX(int index, int totalSamples) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    // Audio buffer
    OscilloscopeBuffer buffer_;
    std::vector<float> displayBuffer_;

    // Display settings
    float timeScale_ = 1000.0f;        // Samples to display
    float triggerLevel_ = 0.0f;         // Trigger level
    TriggerMode triggerMode_ = TriggerMode::Auto;
    bool xyMode_ = false;               // X-Y stereo mode
    bool showGrid_ = true;
    float persistence_ = 0.0f;          // Trail persistence

    // Premium CRT effects
    bool crtEffect_ = true;             // Enable CRT-style rendering
    float scanlineIntensity_ = 0.15f;   // Scanline darkness
    float phosphorDecay_ = 0.85f;       // Phosphor persistence
    float vignetteStrength_ = 0.3f;     // Vignette effect

    // Frequency detection
    float detectedFrequency_ = 0.0f;     // Detected fundamental frequency
    float confidence_ = 0.0f;            // Detection confidence
    int zeroCrossings_ = 0;             // For frequency estimation

    // Animation
    float scanPhase_ = 0.0f;            // Animated scanline position
    float glowPulse_ = 0.0f;            // Pulsing glow effect

    // Colors (premium palette)
    SkColor backgroundColor_ = SkColorSetARGB(255, 8, 8, 12);
    SkColor gridColor_ = SkColorSetARGB(60, 35, 35, 45);
    SkColor gridMajorColor_ = SkColorSetARGB(100, 55, 55, 65);
    SkColor textColor_ = SkColorSetARGB(220, 150, 150, 160);
    SkColor waveformColor_ = SkColorSetARGB(255, 0, 255, 220);     // Bright cyan-green
    SkColor waveformGlowColor_ = SkColorSetARGB(150, 0, 200, 180); // Dimmer for glow
    SkColor secondaryColor_ = SkColorSetARGB(200, 255, 0, 140);   // Magenta
    SkColor triggerColor_ = SkColorSetARGB(255, 255, 220, 0);     // Yellow
    SkColor scanlineColor_ = SkColorSetARGB(25, 0, 0, 0);        // Dark scanlines

    // Trigger state
    int lastTriggerPos_ = 0;
    bool triggered_ = false;

    // Previous waveform for persistence (with timestamps for decay)
    struct TracePoint {
        float x, y;
        float age;  // For phosphor decay
    };
    std::vector<TracePoint> previousTrace_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthOscilloscope)
#endif
};

} // namespace zenith
