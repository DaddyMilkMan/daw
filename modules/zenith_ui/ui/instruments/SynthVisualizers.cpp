/*
    SynthVisualizers.cpp - Real-time Synth Visualization Components

    Collection of visual components that show synth parameters in real-time:
    - Oscilloscope: Shows output waveform
    - Filter Analyzer: Shows filter frequency response
    - Envelope Visualizer: Shows ADSR envelope shapes animating
    - LFO Visualizer: Shows LFO waveforms with modulation

    All components render at 60fps using Skia.

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>

extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <effects/SkGradientShader.h>
#pragma clang diagnostic pop
}

namespace zenith {

//==============================================================================
// Oscilloscope Display
//==============================================================================

class SkiaOscilloscope : public SkiaComponent {
public:
    SkiaOscilloscope() {
        startTimerHz(60); // 60 FPS update
    }

    ~SkiaOscilloscope() override = default;

    void drawSkia(SkCanvas* canvas) override {
        if (!canvas) return;

        auto bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

        // Background
        SkPaint bgPaint;
        bgPaint.setColor(SkColorSetARGB(255, 10, 10, 15));
        bgPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bgPaint);

        // Grid lines
        drawGrid(canvas, bounds);

        // Waveform
        if (!waveformBuffer_.empty()) {
            drawWaveform(canvas, bounds);
        }
    }

    // New audio data to display
    void addAudioSamples(const float* samples, int numSamples) {
        waveformBuffer_.clear();

        // Downsample to display resolution
        int displaySamples = static_cast<int>(getWidth());
        if (displaySamples <= 0) return;

        float step = static_cast<float>(numSamples) / displaySamples;

        for (int i = 0; i < displaySamples; ++i) {
            int sampleIndex = static_cast<int>(i * step);
            if (sampleIndex < numSamples) {
                waveformBuffer_.push_back(samples[sampleIndex]);
            }
        }

        markDirty();
    }

    void setWaveformColor(SkColor color) { waveformColor_ = color; }
    void setGridColor(SkColor color) { gridColor_ = color; }
    void setShowGrid(bool show) { showGrid_ = show; }

protected:
    void timerCallback() override {
        // Decay old waveform data
        // In real implementation, this would pull from audio buffer
    }

private:
    std::vector<float> waveformBuffer_;
    SkColor waveformColor_ = SkColorSetARGB(255, 0, 255, 100);
    SkColor gridColor_ = SkColorSetARGB(100, 50, 50, 60);
    bool showGrid_ = true;

    void drawGrid(SkCanvas* canvas, const SkRect& bounds) {
        if (!showGrid_) return;

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(gridColor_);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(1.0f);

        // Horizontal lines (0.5, 0.25, 0.75, etc.)
        for (float yNorm : {0.25f, 0.5f, 0.75f}) {
            float y = bounds.top() + bounds.height() * yNorm;
            canvas->drawLine(bounds.left(), y, bounds.right(), y, paint);
        }

        // Vertical lines
        for (float xNorm : {0.25f, 0.5f, 0.75f}) {
            float x = bounds.left() + bounds.width() * xNorm;
            canvas->drawLine(x, bounds.top(), x, bounds.bottom(), paint);
        }

        // Center line
        paint.setStrokeWidth(1.5f);
        float centerY = bounds.centerY();
        canvas->drawLine(bounds.left(), centerY, bounds.right(), centerY, paint);
    }

    void drawWaveform(SkCanvas* canvas, const SkRect& bounds) {
        if (waveformBuffer_.size() < 2) return;

        SkPath path;
        bool firstPoint = true;

        float xScale = bounds.width() / waveformBuffer_.size();
        float yScale = bounds.height() * 0.45f; // Leave some padding
        float centerY = bounds.centerY();

        for (size_t i = 0; i < waveformBuffer_.size(); ++i) {
            float x = bounds.left() + i * xScale;
            float y = centerY - waveformBuffer_[i] * yScale;

            if (firstPoint) {
                path.moveTo(x, y);
                firstPoint = false;
            } else {
                path.lineTo(x, y);
            }
        }

        // Draw waveform glow
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setColor(waveformColor_);
        glowPaint.setAlpha(80);
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(4.0f);
        canvas->drawPath(path, glowPaint);

        // Draw waveform core
        SkPaint corePaint;
        corePaint.setAntiAlias(true);
        corePaint.setColor(waveformColor_);
        corePaint.setStyle(SkPaint::kStroke_Style);
        corePaint.setStrokeWidth(2.0f);
        canvas->drawPath(path, corePaint);
    }
};

//==============================================================================
// Filter Frequency Response Analyzer
//==============================================================================

class SkiaFilterAnalyzer : public SkiaComponent {
public:
    SkiaFilterAnalyzer() {
        startTimerHz(60);
    }

    ~SkiaFilterAnalyzer() override = default;

    void drawSkia(SkCanvas* canvas) override {
        if (!canvas) return;

        auto bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

        // Background
        SkPaint bgPaint;
        bgPaint.setColor(SkColorSetARGB(255, 10, 10, 15));
        bgPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bgPaint);

        // Frequency grid
        drawFrequencyGrid(canvas, bounds);

        // Filter response curve
        drawFilterResponse(canvas, bounds);
    }

    void setFilterType(int type) { filterType_ = type; markDirty(); }
    void setCutoff(float cutoff) { cutoff_ = cutoff; markDirty(); }
    void setResonance(float resonance) { resonance_ = resonance; markDirty(); }
    void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }

private:
    int filterType_ = 0; // 0=LPF, 1=HPF, 2=BPF, 3=Notch
    float cutoff_ = 1000.0f;
    float resonance_ = 0.5f;
    double sampleRate_ = 48000.0;

    void drawFrequencyGrid(SkCanvas* canvas, const SkRect& bounds) {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(100, 60, 60, 70));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(1.0f);

        // Frequency lines (20Hz, 100Hz, 1kHz, 10kHz, 20kHz)
        float frequencies[] = {20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f};

        for (float freq : frequencies) {
            float xNorm = frequencyToX(freq, bounds);
            canvas->drawLine(xNorm, bounds.top(), xNorm, bounds.bottom(), paint);
        }

        // dB lines
        for (float dB : {-12.0f, -6.0f, 0.0f, 6.0f, 12.0f}) {
            float yNorm = decibelToY(dB, bounds);
            canvas->drawLine(bounds.left(), yNorm, bounds.right(), yNorm, paint);
        }
    }

    void drawFilterResponse(SkCanvas* canvas, const SkRect& bounds) {
        SkPath path;
        bool firstPoint = true;

        // Draw frequency response from 20Hz to Nyquist
        int numPoints = static_cast<int>(bounds.width());

        for (int i = 0; i < numPoints; ++i) {
            float xNorm = static_cast<float>(i) / numPoints;
            float freq = xFromFrequency(xNorm, bounds);

            // Calculate filter gain at this frequency
            float gain = calculateFilterGain(freq);

            float x = bounds.left() + i;
            float y = decibelToY(gain, bounds);

            if (firstPoint) {
                path.moveTo(x, y);
                firstPoint = false;
            } else {
                path.lineTo(x, y);
            }
        }

        // Draw response curve with gradient
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(2.5f);

        // Gradient color based on filter type
        SkColor startColor = SkColorSetARGB(255, 0, 200, 255);
        SkColor endColor = SkColorSetARGB(255, 100, 255, 200);

        paint.setColor(startColor);
        canvas->drawPath(path, paint);

        // Draw cutoff marker
        float cutoffX = frequencyToX(cutoff_, bounds);
        SkPaint markerPaint;
        markerPaint.setAntiAlias(true);
        markerPaint.setColor(SkColorSetARGB(200, 255, 100, 100));
        markerPaint.setStyle(SkPaint::kStroke_Style);
        markerPaint.setStrokeWidth(2.0f);

        canvas->drawLine(cutoffX, bounds.top(), cutoffX, bounds.bottom(), markerPaint);
    }

    float calculateFilterGain(float frequency) const {
        // Simplified filter response calculation
        // In real implementation, use actual filter coefficients

        float omega = 2.0f * juce::MathConstants<float>::pi * frequency / static_cast<float>(sampleRate_);
        float omegaC = 2.0f * juce::MathConstants<float>::pi * cutoff_ / static_cast<float>(sampleRate_);

        float s = omega / omegaC;
        float gain = 0.0f;

        switch (filterType_) {
            case 0: // Low-pass
                gain = 1.0f / std::sqrt(1.0f + std::pow(s, 4.0f) + resonance_ * std::pow(s, 2.0f));
                break;
            case 1: // High-pass
                gain = std::pow(s, 2.0f) / std::sqrt(1.0f + std::pow(s, 4.0f) + resonance_ * std::pow(s, 2.0f));
                break;
            case 2: // Band-pass
                gain = s / std::sqrt(std::pow(1.0f - s * s, 2.0f) + resonance_ * s * s);
                break;
            case 3: // Notch
                gain = std::abs(1.0f - s * s) / std::sqrt(std::pow(1.0f - s * s, 2.0f) + resonance_ * s * s);
                break;
        }

        return 20.0f * std::log10(std::max(0.0001f, gain)); // Convert to dB
    }

    float frequencyToX(float frequency, const SkRect& bounds) const {
        float minFreq = 20.0f;
        float maxFreq = static_cast<float>(sampleRate_) * 0.5f;
        float xNorm = std::log10(frequency / minFreq) / std::log10(maxFreq / minFreq);
        return bounds.left() + bounds.width() * xNorm;
    }

    float xFromFrequency(float xNorm, const SkRect& bounds) const {
        float minFreq = 20.0f;
        float maxFreq = static_cast<float>(sampleRate_) * 0.5f;
        float logRatio = std::log10(maxFreq / minFreq);
        return minFreq * std::pow(10.0f, xNorm * logRatio);
    }

    float decibelToY(float dB, const SkRect& bounds) const {
        float minDB = -24.0f;
        float maxDB = 24.0f;
        float yNorm = 1.0f - (dB - minDB) / (maxDB - minDB);
        return bounds.top() + bounds.height() * yNorm;
    }
};

//==============================================================================
// ADSR Envelope Visualizer
//==============================================================================

class SkiaEnvelopeVisualizer : public SkiaComponent {
public:
    SkiaEnvelopeVisualizer() {
        startTimerHz(60);
    }

    ~SkiaEnvelopeVisualizer() override = default;

    void drawSkia(SkCanvas* canvas) override {
        if (!canvas) return;

        auto bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

        // Background
        SkPaint bgPaint;
        bgPaint.setColor(SkColorSetARGB(255, 10, 10, 15));
        bgPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bgPaint);

        // Grid
        drawGrid(canvas, bounds);

        // Envelope shape
        drawEnvelope(canvas, bounds);

        // Current position indicator
        drawCurrentPosition(canvas, bounds);
    }

    void setEnvelope(float attack, float decay, float sustain, float release) {
        attack_ = attack;
        decay_ = decay;
        sustain_ = sustain;
        sustainLevel_ = sustain;
        release_ = release;
        markDirty();
    }

    void setCurrentPhase(int phase) { currentPhase_ = phase; markDirty(); } // 0=Attack, 1=Decay, 2=Sustain, 3=Release
    void setCurrentPosition(float pos) { currentPosition_ = pos; markDirty(); } // 0-1 within phase

private:
    float attack_ = 0.1f;
    float decay_ = 0.2f;
    float sustain_ = 0.7f;
    float release_ = 0.3f;
    float sustainLevel_ = 0.7f;

    int currentPhase_ = 0;
    float currentPosition_ = 0.0f;

    void drawGrid(SkCanvas* canvas, const SkRect& bounds) {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(80, 50, 50, 60));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(1.0f);

        // Horizontal lines
        for (float yNorm : {0.25f, 0.5f, 0.75f}) {
            float y = bounds.top() + bounds.height() * (1.0f - yNorm);
            canvas->drawLine(bounds.left(), y, bounds.right(), y, paint);
        }

        // Vertical lines (phase boundaries)
        float total = attack_ + decay_ + release_ + 0.1f; // + 0.1f for sustain minimum
        float x1 = bounds.left() + bounds.width() * (attack_ / total);
        float x2 = x1 + bounds.width() * (decay_ / total);

        paint.setColor(SkColorSetARGB(100, 80, 80, 90));
        canvas->drawLine(x1, bounds.top(), x1, bounds.bottom(), paint);
        canvas->drawLine(x2, bounds.top(), x2, bounds.bottom(), paint);
    }

    void drawEnvelope(SkCanvas* canvas, const SkRect& bounds) {
        SkPath path;

        float padding = 20.0f;
        float drawWidth = bounds.width() - padding * 2.0f;
        float drawHeight = bounds.height() - padding * 2.0f;

        float total = attack_ + decay_ + release_ + 0.1f;

        // Start point
        float startX = bounds.left() + padding;
        float startY = bounds.bottom() - padding;
        path.moveTo(startX, startY);

        // Attack
        float attackX = startX + drawWidth * (attack_ / total);
        float attackY = bounds.top() + padding;
        path.lineTo(attackX, attackY);

        // Decay
        float decayX = attackX + drawWidth * (decay_ / total);
        float decayY = bounds.bottom() - padding - drawHeight * sustainLevel_;
        path.lineTo(decayX, decayY);

        // Sustain
        float sustainEndX = decayX + drawWidth * (0.1f / total);
        path.lineTo(sustainEndX, decayY);

        // Release
        float releaseX = sustainEndX + drawWidth * (release_ / total);
        float releaseY = bounds.bottom() - padding;
        path.lineTo(releaseX, releaseY);

        // Draw envelope path
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(255, 0, 255, 150));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(2.5f);
        canvas->drawPath(path, paint);

        // Fill under envelope
        SkPath fillPath = path;
        fillPath.lineTo(releaseX, bounds.bottom() - padding);
        fillPath.lineTo(startX, bounds.bottom() - padding);
        fillPath.close();

        SkPaint fillPaint;
        fillPaint.setAntiAlias(true);
        fillPaint.setColor(SkColorSetARGB(50, 0, 255, 150));
        fillPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawPath(fillPath, fillPaint);

        // Draw phase labels
        drawPhaseLabels(canvas, bounds);
    }

    void drawCurrentPosition(SkCanvas* canvas, const SkRect& bounds) {
        // Calculate current X position based on phase
        float padding = 20.0f;
        float drawWidth = bounds.width() - padding * 2.0f;

        float total = attack_ + decay_ + release_ + 0.1f;
        float x = bounds.left() + padding;

        switch (currentPhase_) {
            case 0: // Attack
                x += drawWidth * (attack_ / total) * currentPosition_;
                break;
            case 1: // Decay
                x += drawWidth * (attack_ / total);
                x += drawWidth * (decay_ / total) * currentPosition_;
                break;
            case 2: // Sustain
                x += drawWidth * ((attack_ + decay_) / total);
                x += drawWidth * (0.1f / total) * currentPosition_;
                break;
            case 3: // Release
                x += drawWidth * ((attack_ + decay_ + 0.1f) / total);
                x += drawWidth * (release_ / total) * currentPosition_;
                break;
        }

        // Draw position indicator
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(255, 255, 100, 100));
        paint.setStyle(SkPaint::kFill_Style);

        canvas->drawCircle(x, bounds.bottom() - padding, 5.0f, paint);
    }

    void drawPhaseLabels(SkCanvas* canvas, const SkRect& bounds) {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(200, 150, 150, 160));
        paint.setTextSize(10.0f);

        // Draw labels at top of each phase
        // In real implementation, use proper text rendering
    }
};

//==============================================================================
// LFO Visualizer
//==============================================================================

class SkiaLFOVisualizer : public SkiaComponent {
public:
    enum class Waveform { Sine, Triangle, Saw, Square, SampleAndHold };

    SkiaLFOVisualizer() {
        startTimerHz(60);
    }

    ~SkiaLFOVisualizer() override = default;

    void drawSkia(SkCanvas* canvas) override {
        if (!canvas) return;

        auto bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

        // Background
        SkPaint bgPaint;
        bgPaint.setColor(SkColorSetARGB(255, 10, 10, 15));
        bgPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bgPaint);

        // Center line
        SkPaint linePaint;
        linePaint.setAntiAlias(true);
        linePaint.setColor(SkColorSetARGB(100, 60, 60, 70));
        linePaint.setStyle(SkPaint::kStroke_Style);
        linePaint.setStrokeWidth(1.0f);
        canvas->drawLine(bounds.left(), bounds.centerY(), bounds.right(), bounds.centerY(), linePaint);

        // LFO waveform
        drawWaveform(canvas, bounds);

        // Current value indicator
        drawCurrentValue(canvas, bounds);
    }

    void setWaveform(Waveform waveform) { waveform_ = waveform; markDirty(); }
    void setRate(float rateHz) { rate_ = rateHz; markDirty(); }
    void setAmount(float amount) { amount_ = amount; markDirty(); }
    void setCurrentValue(float value) { currentValue_ = value; markDirty(); }

private:
    Waveform waveform_ = Waveform::Sine;
    float rate_ = 5.0f;
    float amount_ = 1.0f;
    float currentValue_ = 0.0f;
    float phase_ = 0.0f;

    void drawWaveform(SkCanvas* canvas, const SkRect& bounds) {
        SkPath path;
        bool firstPoint = true;

        int numPoints = static_cast<int>(bounds.width());

        for (int i = 0; i < numPoints; ++i) {
            float phase = static_cast<float>(i) / numPoints; // 0 to 1
            float value = calculateWaveform(phase) * amount_;

            float x = bounds.left() + i;
            float y = bounds.centerY() - value * bounds.height() * 0.4f;

            if (firstPoint) {
                path.moveTo(x, y);
                firstPoint = false;
            } else {
                path.lineTo(x, y);
            }
        }

        // Draw waveform
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(255, 100, 200, 255));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(2.0f);
        canvas->drawPath(path, paint);
    }

    void drawCurrentValue(SkCanvas* canvas, const SkRect& bounds) {
        float y = bounds.centerY() - currentValue_ * bounds.height() * 0.4f;

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(255, 255, 100, 100));
        paint.setStyle(SkPaint::kFill_Style);

        // Draw indicator circle
        canvas->drawCircle(bounds.left() + 10.0f, y, 6.0f, paint);

        // Draw indicator line
        SkPaint linePaint;
        linePaint.setAntiAlias(true);
        linePaint.setColor(SkColorSetARGB(150, 255, 100, 100));
        linePaint.setStyle(SkPaint::kStroke_Style);
        linePaint.setStrokeWidth(1.0f);
        canvas->drawLine(bounds.left() + 20.0f, y, bounds.right(), y, linePaint);
    }

    float calculateWaveform(float phase) const {
        switch (waveform_) {
            case Waveform::Sine:
                return std::sin(phase * juce::MathConstants<float>::twoPi);
            case Waveform::Triangle: {
                float p = phase * 4.0f;
                if (p < 1.0f) return p;
                if (p < 3.0f) return 2.0f - p;
                return p - 4.0f;
            }
            case Waveform::Saw:
                return 1.0f - 2.0f * phase;
            case Waveform::Square:
                return (phase < 0.5f) ? 1.0f : -1.0f;
            case Waveform::SampleAndHold:
                // Simplified - returns value based on phase
                return (phase < 0.5f) ? 0.7f : -0.5f;
        }
        return 0.0f;
    }
};

//==============================================================================
// Spectrum Analyzer
//==============================================================================

class SkiaSpectrumAnalyzer : public SkiaComponent {
public:
    SkiaSpectrumAnalyzer() {
        startTimerHz(30); // 30 FPS is sufficient for spectrum
    }

    ~SkiaSpectrumAnalyzer() override = default;

    void drawSkia(SkCanvas* canvas) override {
        if (!canvas) return;

        auto bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

        // Background
        SkPaint bgPaint;
        bgPaint.setColor(SkColorSetARGB(255, 10, 10, 15));
        bgPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(bounds, bgPaint);

        // Frequency grid
        drawFrequencyGrid(canvas, bounds);

        // Spectrum bars
        drawSpectrum(canvas, bounds);
    }

    void setSpectrumData(const std::vector<float>& data) {
        spectrumData_ = data;
        markDirty();
    }

    void setGradientColors(SkColor low, SkColor mid, SkColor high) {
        lowColor_ = low;
        midColor_ = mid;
        highColor_ = high;
    }

private:
    std::vector<float> spectrumData_;
    SkColor lowColor_ = SkColorSetARGB(255, 0, 255, 100);
    SkColor midColor_ = SkColorSetARGB(255, 255, 200, 0);
    SkColor highColor_ = SkColorSetARGB(255, 255, 50, 50);

    void drawFrequencyGrid(SkCanvas* canvas, const SkRect& bounds) {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(80, 50, 50, 60));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(1.0f);

        // Frequency markers
        float frequencies[] = {100.0f, 1000.0f, 10000.0f};

        for (float freq : frequencies) {
            float x = frequencyToX(freq, bounds);
            canvas->drawLine(x, bounds.top(), x, bounds.bottom(), paint);
        }
    }

    void drawSpectrum(SkCanvas* canvas, const SkRect& bounds) {
        if (spectrumData_.empty()) return;

        float barWidth = bounds.width() / spectrumData_.size();

        for (size_t i = 0; i < spectrumData_.size(); ++i) {
            float magnitude = spectrumData_[i]; // 0 to 1

            // Convert to dB for display
            float dB = 20.0f * std::log10(std::max(0.0001f, magnitude));
            float dBNorm = (dB + 60.0f) / 60.0f; // -60dB to 0dB range
            dBNorm = juce::jlimit(0.0f, 1.0f, dBNorm);

            float x = bounds.left() + i * barWidth;
            float height = bounds.height() * dBNorm;
            float y = bounds.bottom() - height;

            SkRect barRect = SkRect::MakeXYWH(x + 1.0f, y, barWidth - 2.0f, height);

            // Gradient color based on frequency
            float freqNorm = static_cast<float>(i) / spectrumData_.size();
            SkColor barColor = interpolateColor(freqNorm);

            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setColor(barColor);
            paint.setStyle(SkPaint::kFill_Style);

            canvas->drawRect(barRect, paint);
        }
    }

    float frequencyToX(float frequency, const SkRect& bounds) const {
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;
        float xNorm = std::log10(frequency / minFreq) / std::log10(maxFreq / minFreq);
        return bounds.left() + bounds.width() * xNorm;
    }

    SkColor interpolateColor(float t) const {
        // Interpolate between low, mid, high colors
        if (t < 0.5f) {
            float localT = t * 2.0f;
            return SkColorSetARGB(
                255,
                static_cast<int>(SkColorGetR(lowColor_) + (SkColorGetR(midColor_) - SkColorGetR(lowColor_)) * localT),
                static_cast<int>(SkColorGetG(lowColor_) + (SkColorGetG(midColor_) - SkColorGetG(lowColor_)) * localT),
                static_cast<int>(SkColorGetB(lowColor_) + (SkColorGetB(midColor_) - SkColorGetB(lowColor_)) * localT)
            );
        } else {
            float localT = (t - 0.5f) * 2.0f;
            return SkColorSetARGB(
                255,
                static_cast<int>(SkColorGetR(midColor_) + (SkColorGetR(highColor_) - SkColorGetR(midColor_)) * localT),
                static_cast<int>(SkColorGetG(midColor_) + (SkColorGetG(highColor_) - SkColorGetG(midColor_)) * localT),
                static_cast<int>(SkColorGetB(midColor_) + (SkColorGetB(highColor_) - SkColorGetB(midColor_)) * localT)
            );
        }
    }
};

} // namespace zenith
