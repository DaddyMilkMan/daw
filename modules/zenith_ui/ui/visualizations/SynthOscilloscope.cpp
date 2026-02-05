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

    SynthOscilloscope.cpp
    Created: 2026-02-01
    Author:  Zenith DAW

    Implementation of real-time oscilloscope for synth output.

  ==============================================================================

*/

#include "SynthOscilloscope.h"
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// SynthOscilloscope Implementation
//==============================================================================

SynthOscilloscope::SynthOscilloscope() {
    displayBuffer_.reserve(4096);
    previousTrace_.reserve(2048);

    // 60 FPS for smooth CRT animations
    startTimerHz(60);
}

SynthOscilloscope::~SynthOscilloscope() {
    stopTimer();
}

void SynthOscilloscope::addAudioData(const juce::AudioBuffer<float>& buffer) {
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    if (numChannels == 1) {
        buffer_.push(buffer.getReadPointer(0), numSamples);
    } else if (numChannels >= 2) {
        buffer_.pushStereo(buffer.getReadPointer(0),
                          buffer.getReadPointer(1), numSamples);
    }
}

void SynthOscilloscope::addAudioData(const float* samples, int numSamples) {
    buffer_.push(samples, numSamples);
}

void SynthOscilloscope::clear() {
    buffer_.clear();
    displayBuffer_.clear();
    previousTrace_.clear();
    markDirty();
}

void SynthOscilloscope::timerCallback() {
    // Update animation phases
    scanPhase_ += 0.016f; // ~60fps
    if (scanPhase_ > 1.0f) scanPhase_ -= 1.0f;

    glowPulse_ = 0.5f + 0.5f * std::sin(scanPhase_ * juce::MathConstants<float>::twoPi);

    // Read new data from buffer
    displayBuffer_ = buffer_.read(static_cast<int>(timeScale_));

    // Detect frequency from incoming audio
    if (!displayBuffer_.empty()) {
        detectedFrequency_ = detectFrequency(displayBuffer_);
    }

    // Age previous trace points for phosphor decay
    for (auto& point : previousTrace_) {
        point.age += 0.016f;
    }
    // Remove old points
    previousTrace_.erase(
        std::remove_if(previousTrace_.begin(), previousTrace_.end(),
                     [](const TracePoint& p) { return p.age > phosphorDecay_; }),
        previousTrace_.end());

    // Always mark dirty for smooth 60fps CRT effects
    markDirty();
}

#ifdef ZENITH_USE_SKIA

void SynthOscilloscope::drawSkia(SkCanvas* canvas) {
    if (xyMode_) {
        drawXYDisplay(canvas);
    } else {
        drawBackground(canvas);
        if (showGrid_) {
            drawGrid(canvas);
        }
        drawWaveform(canvas);
        drawTriggerIndicator(canvas);
        drawInfo(canvas);
        drawFrequencyInfo(canvas);
    }
}

void SynthOscilloscope::drawBackground(SkCanvas* canvas) {
    SkPaint bgPaint;
    bgPaint.setColor(backgroundColor_);
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
}

void SynthOscilloscope::drawGrid(SkCanvas* canvas) {
    if (getWidth() <= 0 || getHeight() <= 0) return;

    SkPaint gridPaint, majorGridPaint;
    gridPaint.setColor(gridColor_);
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setStrokeWidth(1.0f);
    gridPaint.setAntiAlias(true);

    majorGridPaint.setColor(gridMajorColor_);
    majorGridPaint.setStyle(SkPaint::kStroke_Style);
    majorGridPaint.setStrokeWidth(1.5f);
    majorGridPaint.setAntiAlias(true);

    const float width = getWidth();
    const float height = getHeight();

    // Horizontal lines (voltage markers)
    for (int i = -5; i <= 5; ++i) {
        if (i == 0) continue; // Skip center line, draw later
        float y = sampleToY(static_cast<float>(i) * 0.2f);
        bool isMajor = (i % 2 == 0);
        canvas->drawLine(0, y, width, y, isMajor ? majorGridPaint : gridPaint);
    }

    // Vertical lines (time markers)
    int numDivisions = 10;
    for (int i = 0; i <= numDivisions; ++i) {
        float x = width * i / numDivisions;
        canvas->drawLine(x, 0, x, height, gridPaint);
    }

    // Center line (0V)
    float centerY = sampleToY(0.0f);
    SkPaint centerPaint;
    centerPaint.setColor(SkColorSetARGB(180, 100, 100, 110));
    centerPaint.setStyle(SkPaint::kStroke_Style);
    centerPaint.setStrokeWidth(2.0f);
    canvas->drawLine(0, centerY, width, centerY, centerPaint);

    // Voltage labels
    SkFont font;
    font.setSize(10.0f);
    SkPaint textPaint;
    textPaint.setColor(textColor_);
    textPaint.setAntiAlias(true);

    for (int i = -4; i <= 4; i += 2) {
        if (i == 0) continue;
        float y = sampleToY(static_cast<float>(i) * 0.2f);
        juce::String label = juce::String(i * 20) + "%";
        canvas->drawText(label.toUTF8(), label.length(), 5, y - 3, font, textPaint);
    }
}

void SynthOscilloscope::drawWaveform(SkCanvas* canvas) {
    if (displayBuffer_.empty() || getWidth() <= 0 || getHeight() <= 0) return;

    // Find trigger position if not in free run mode
    int startPos = 0;
    if (triggerMode_ != TriggerMode::Free) {
        startPos = findTriggerPosition(displayBuffer_);
    }

    // Calculate how many samples we can display
    int numSamples = std::min(static_cast<int>(displayBuffer_.size()) - startPos,
                              static_cast<int>(timeScale_));
    if (numSamples <= 0) return;

    SkPath path;

    // Build the waveform path and store trace points
    bool started = false;

    for (int i = 0; i < numSamples; ++i) {
        int bufferIdx = startPos + i;
        if (bufferIdx >= static_cast<int>(displayBuffer_.size())) break;

        float sample = displayBuffer_[bufferIdx];
        float x = indexToX(i, numSamples);
        float y = sampleToY(sample);

        if (!started) {
            path.moveTo(x, y);
            started = true;
        } else {
            path.lineTo(x, y);
        }

        // Store trace points for phosphor decay effect
        if (i % 3 == 0) { // Sample every 3rd point for efficiency
            previousTrace_.push_back({x, y, 0.0f});
        }
    }

    // Limit trace buffer
    if (previousTrace_.size() > 2048) {
        previousTrace_.erase(previousTrace_.begin(),
                           previousTrace_.begin() + (previousTrace_.size() - 2048));
    }

    // ========== CRT PHOSPHOR DECAY TRAIL ==========
    // Draw multiple layers with decreasing brightness
    for (int layer = 0; layer < 3; ++layer) {
        SkPath tracePath;
        bool traceStarted = false;
        for (const auto& point : previousTrace_) {
            float alpha = 1.0f - (point.age / phosphorDecay_);
            alpha = juce::jmax(0.0f, alpha);
            // Each layer has different threshold
            float layerThreshold = 0.3f + layer * 0.25f;
            if (alpha < layerThreshold) continue;

            if (!traceStarted) {
                tracePath.moveTo(point.x, point.y);
                traceStarted = true;
            } else {
                tracePath.lineTo(point.x, point.y);
            }
        }

        if (traceStarted) {
            SkPaint tracePaint;
            tracePaint.setColor(waveformGlowColor_);
            tracePaint.setStyle(SkPaint::kStroke_Style);
            tracePaint.setStrokeWidth(1.5f + layer * 0.5f);
            tracePaint.setAlpha(static_cast<U8CPU>((40 - layer * 10) * (1.0f - layer * 0.25f)));
            tracePaint.setAntiAlias(true);
            canvas->drawPath(tracePath, tracePaint);
        }
    }

    // ========== OUTER GLOW (CRT bloom effect) ==========
    SkPaint glowOuter;
    glowOuter.setColor(waveformColor_);
    glowOuter.setStyle(SkPaint::kStroke_Style);
    glowOuter.setStrokeWidth(8.0f);
    glowOuter.setAlpha(50);
    glowOuter.setAntiAlias(true);
    auto blurOuter = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 6.0f);
    glowOuter.setMaskFilter(blurOuter);
    canvas->drawPath(path, glowOuter);

    // ========== INNER GLOW (bright core) ==========
    SkPaint glowInner;
    glowInner.setColor(waveformColor_);
    glowInner.setStyle(SkPaint::kStroke_Style);
    glowInner.setStrokeWidth(3.0f);
    glowInner.setAlpha(100);
    glowInner.setAntiAlias(true);
    auto blurInner = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 3.0f);
    glowInner.setMaskFilter(blurInner);
    canvas->drawPath(path, glowInner);

    // ========== MAIN WAVEFORM (bright CRT trace) ==========
    SkPaint mainPaint;
    mainPaint.setColor(SkColorSetRGB(255, 255, 255)); // CRT phosphor white-hot center
    mainPaint.setStyle(SkPaint::kStroke_Style);
    mainPaint.setStrokeWidth(2.0f);
    mainPaint.setAntiAlias(true);
    canvas->drawPath(path, mainPaint);

    // ========== SCANLINES OVERLAY ==========
    if (crtEffect_) {
        drawScanlines(canvas);
    }
}

void SynthOscilloscope::drawXYDisplay(SkCanvas* canvas) {
    // Draw X-Y mode (Lissajous figure for stereo correlation)
    SkPaint bgPaint;
    bgPaint.setColor(backgroundColor_);
    bgPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);

    // Draw grid
    SkPaint gridPaint;
    gridPaint.setColor(gridColor_);
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setStrokeWidth(1.0f);

    float cx = getWidth() * 0.5f;
    float cy = getHeight() * 0.5f;
    canvas->drawLine(0, cy, getWidth(), cy, gridPaint);
    canvas->drawLine(cx, 0, cx, getHeight(), gridPaint);

    // Draw circle for reference
    float radius = std::min(getWidth(), getHeight()) * 0.35f;
    canvas->drawCircle(cx, cy, radius, gridPaint);

    if (displayBuffer_.size() < 2) return;

    SkPath path;
    bool started = false;

    // Interleaved stereo: even = left (X), odd = right (Y)
    for (size_t i = 0; i + 1 < displayBuffer_.size(); i += 2) {
        float left = displayBuffer_[i];
        float right = displayBuffer_[i + 1];

        float x = cx + left * (getWidth() * 0.4f);
        float y = cy - right * (getHeight() * 0.4f);

        if (!started) {
            path.moveTo(x, y);
            started = true;
        } else {
            path.lineTo(x, y);
        }
    }

    // Draw X-Y trace with glow
    SkPaint glowPaint;
    glowPaint.setColor(waveformColor_);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(4.0f);
    glowPaint.setAlpha(60);
    canvas->drawPath(path, glowPaint);

    SkPaint mainPaint;
    mainPaint.setColor(waveformColor_);
    mainPaint.setStyle(SkPaint::kStroke_Style);
    mainPaint.setStrokeWidth(2.0f);
    mainPaint.setAntiAlias(true);
    canvas->drawPath(path, mainPaint);

    // Current position dot
    if (!displayBuffer_.empty() && displayBuffer_.size() >= 2) {
        size_t lastIdx = displayBuffer_.size() - 2;
        float x = cx + displayBuffer_[lastIdx] * (getWidth() * 0.4f);
        float y = cy - displayBuffer_[lastIdx + 1] * (getHeight() * 0.4f);

        SkPaint dotPaint;
        dotPaint.setColor(SkColorSetRGB(255, 255, 255));
        dotPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(x, y, 4.0f, dotPaint);
    }

    // Label
    SkFont font;
    font.setSize(12.0f);
    SkPaint textPaint;
    textPaint.setColor(textColor_);
    canvas->drawText("X-Y MODE", 7, getWidth() - 70, getHeight() - 10, font, textPaint);
}

void SynthOscilloscope::drawTriggerIndicator(SkCanvas* canvas) {
    if (triggerMode_ == TriggerMode::Free) return;

    float y = sampleToY(triggerLevel_);
    float width = getWidth();

    SkPaint triggerPaint;
    triggerPaint.setColor(triggerColor_);
    triggerPaint.setStyle(SkPaint::kStroke_Style);
    triggerPaint.setStrokeWidth(2.0f);
    triggerPaint.setAntiAlias(true);

    // Draw trigger level line
    canvas->drawLine(0, y, width, y, triggerPaint);

    // Draw triangle indicator at left
    SkPath triangle;
    triangle.moveTo(0, y);
    triangle.lineTo(10, y - 5);
    triangle.lineTo(10, y + 5);
    triangle.close();

    SkPaint fillPaint;
    fillPaint.setColor(triggerColor_);
    fillPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawPath(triangle, fillPaint);

    // Trigger level label
    SkFont font;
    font.setSize(10.0f);
    SkPaint textPaint;
    textPaint.setColor(triggerColor_);
    juce::String label = juce::String(triggerLevel_ * 100, 1) + "%";
    canvas->drawText(label.toUTF8(), label.length(), 15, y - 3, font, textPaint);
}

void SynthOscilloscope::drawInfo(SkCanvas* canvas) {
    SkFont font;
    font.setSize(11.0f);
    SkPaint textPaint;
    textPaint.setColor(textColor_);
    textPaint.setAntiAlias(true);

    float x = getWidth() - 120;
    float y = 15;

    // Time scale
    float ms = (timeScale_ / 44100.0f) * 1000.0f;
    juce::String timeText = juce::String(ms, 1) + "ms";
    canvas->drawText(timeText.toUTF8(), timeText.length(), x, y, font, textPaint);

    // Trigger mode
    y += 14;
    const char* modes[] = {"FREE", "AUTO", "NORM", "SINGLE"};
    juce::String modeText = modes[static_cast<int>(triggerMode_)];
    canvas->drawText(modeText.toUTF8(), modeText.length(), x, y, font, textPaint);

    // Sample count
    y += 14;
    juce::String sampleText = juce::String(displayBuffer_.size()) + " smpl";
    canvas->drawText(sampleText.toUTF8(), sampleText.length(), x, y, font, textPaint);
}

int SynthOscilloscope::findTriggerPosition(const std::vector<float>& buffer) {
    if (buffer.size() < 10) return 0;

    float threshold = triggerLevel_;
    int hysteresis = 5; // Minimum samples between triggers

    // Start from last trigger position + hysteresis
    int searchStart = (lastTriggerPos_ + hysteresis) % static_cast<int>(buffer.size());

    for (int i = searchStart; i < static_cast<int>(buffer.size()) - 1; ++i) {
        // Look for upward crossing of threshold
        if (buffer[i] <= threshold && buffer[i + 1] > threshold) {
            lastTriggerPos_ = i;
            triggered_ = true;
            return i;
        }
    }

    // If auto trigger and no trigger found, start from beginning
    if (triggerMode_ == TriggerMode::Auto && !triggered_) {
        return 0;
    }

    return lastTriggerPos_;
}

float SynthOscilloscope::sampleToY(float sample) const {
    float height = getHeight();
    float margin = 10.0f;
    float displayHeight = height - 2.0f * margin;

    // Clamp sample to [-1, 1]
    sample = juce::jlimit(-1.0f, 1.0f, sample);

    // Map: -1 -> bottom, 0 -> center, 1 -> top
    return margin + displayHeight * (0.5f - sample * 0.5f);
}

float SynthOscilloscope::yToSample(float y) const {
    float height = getHeight();
    float margin = 10.0f;
    float displayHeight = height - 2.0f * margin;

    float normalized = (y - margin) / displayHeight;
    return 0.5f - normalized * 2.0f;
}

float SynthOscilloscope::indexToX(int index, int totalSamples) const {
    if (totalSamples <= 0) return 0.0f;
    float width = getWidth();
    float margin = 10.0f;
    float displayWidth = width - 2.0f * margin;
    return margin + (index * displayWidth / totalSamples);
}

//==============================================================================
// PREMIUM CRT EFFECT FUNCTIONS
//==============================================================================

void SynthOscilloscope::drawScanlines(SkCanvas* canvas) {
    float height = getHeight();
    float width = getWidth();

    // Horizontal scanlines with subtle animation
    SkPaint scanlinePaint;
    scanlinePaint.setColor(scanlineColor_);
    scanlinePaint.setStyle(SkPaint::kFill_Style);

    // Draw scanlines (every 2 pixels for CRT effect)
    for (float y = 0; y < height; y += 2.0f) {
        // Slight variation in brightness for realism
        float brightnessVariation = 0.8f + 0.2f * std::sin(y * 0.1f + scanPhase_ * juce::MathConstants<float>::twoPi);
        scanlinePaint.setAlpha(static_cast<U8CPU>(scanlineIntensity_ * 255 * brightnessVariation));
        canvas->drawRect(SkRect::MakeXYWH(0, y, width, 1.0f), scanlinePaint);
    }

    // Vignette effect (darker at corners)
    float vignetteRadius = std::max(width, height) * 0.7f;
    float centerX = width * 0.5f;
    float centerY = height * 0.5f;

    SkPoint center = {centerX, centerY};
    SkColor vignetteColors[2] = {
        SkColorSetARGB(0, 0, 0, 0),
        SkColorSetARGB(static_cast<U8CPU>(vignetteStrength_ * 180), 0, 0, 0)
    };
    float vignettePos[2] = {0.0f, 1.0f};

    auto vignetteShader = SkGradientShader::MakeRadial(
        center, vignetteRadius * 0.3f, center, vignetteRadius,
        vignetteColors, vignettePos, 2, SkTileMode::kClamp);

    SkPaint vignettePaint;
    vignettePaint.setShader(vignetteShader);
    vignettePaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(SkRect::MakeWH(0, 0, width, height), vignettePaint);
}

void SynthOscilloscope::drawFrequencyInfo(SkCanvas* canvas) {
    if (detectedFrequency_ < 1.0f) return;

    SkFont font;
    font.setSize(14.0f);
    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(255, 0, 255, 220));
    textPaint.setAntiAlias(true);

    // Display detected frequency
    juce::String freqText;
    if (detectedFrequency_ < 1000.0f) {
        freqText = juce::String(detectedFrequency_, 1) + " Hz";
    } else {
        freqText = juce::String(detectedFrequency_ / 1000.0, 2) + " kHz";
    }

    // Draw with glow
    float x = getWidth() - 120;
    float y = 20;

    SkPaint glowPaint;
    glowPaint.setColor(SkColorSetARGB(150, 0, 255, 220));
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(3.0f);
    auto blur = SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 4.0f);
    glowPaint.setMaskFilter(blur);
    canvas->drawSimpleText(freqText.toUTF8(), freqText.length(), SkTextEncoding::kUTF8,
                         x, y, font, glowPaint);

    canvas->drawSimpleText(freqText.toUTF8(), freqText.length(), SkTextEncoding::kUTF8,
                         x, y, font, textPaint);

    // Note/octave display
    float noteNum = 12.0f * std::log2(detectedFrequency_ / 440.0f) + 69.0f;
    const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int noteIndex = static_cast<int>(std::round(noteNum)) % 12;
    int octave = static_cast<int>(std::round(noteNum)) / 12 - 1;

    juce::String noteText = notes[noteIndex] + juce::String(octave);
    y += 18;

    SkPaint notePaint;
    notePaint.setColor(SkColorSetARGB(200, 150, 200, 180));
    notePaint.setAntiAlias(true);
    canvas->drawSimpleText(noteText.toUTF8(), noteText.length(), SkTextEncoding::kUTF8,
                         x, y, font, notePaint);
}

float SynthOscilloscope::detectFrequency(const std::vector<float>& buffer) {
    if (buffer.size() < 64) return 0.0f;

    // Zero-crossing detection for fundamental frequency
    int crossings = 0;
    bool wasPositive = buffer[0] > 0;

    for (size_t i = 1; i < buffer.size(); ++i) {
        bool isPositive = buffer[i] > 0;
        if (wasPositive != isPositive) {
            crossings++;
            wasPositive = isPositive;
        }
    }

    zeroCrossings_ = crossings;

    if (crossings < 2) return 0.0f;

    // Calculate frequency from zero crossings
    float samplesPerCycle = static_cast<float>(buffer.size()) / (crossings / 2);
    float frequency = 44100.0f / samplesPerCycle;

    // Basic validation - filter out unreasonable values
    if (frequency < 20.0f || frequency > 20000.0f) {
        confidence_ = 0.0f;
        return 0.0f;
    }

    confidence_ = std::min(1.0f, std::abs(buffer[0]) * 2.0f);
    return frequency;
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
