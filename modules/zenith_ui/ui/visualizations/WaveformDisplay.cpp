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

#include <cmath>

namespace zenith {
namespace ui {

//==============================================================================
// WaveformDisplay Implementation
//==============================================================================
WaveformDisplay::WaveformDisplay() {
    // Start timer for real-time updates
    startTimerHz(20);  // 20 FPS

    // Enable mouse interaction
    setMouseCursor(juce::MouseCursor::IBeam);
    setWantsKeyboardFocus(true);

    updateViewRange();
}

WaveformDisplay::~WaveformDisplay() {
    stopTimer();
}

void WaveformDisplay::setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    currentSampleRate_ = sampleRate;
    totalSamplesProcessed_ = 0;
    audioDuration_ = 0.0;

    waveformBuffer_.clear();
    processAudioData(buffer);

    audioDuration_ = currentSampleRate_ > 0.0 ? (static_cast<double>(totalSamplesProcessed_) / currentSampleRate_)
                                              : 0.0;

    needsRedraw_ = true;
    updateViewRange();
    markDirty();
}

void WaveformDisplay::addAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    if (currentSampleRate_ != sampleRate) {
        currentSampleRate_ = sampleRate;
        waveformBuffer_.clear();
        totalSamplesProcessed_ = 0;
    }

    processAudioData(buffer);

    // Optimize buffer size
    if (waveformBuffer_.size() > MAX_BUFFER_SIZE) {
        optimizeBuffer();
    }

    audioDuration_ = currentSampleRate_ > 0.0 ? (static_cast<double>(totalSamplesProcessed_) / currentSampleRate_)
                                              : audioDuration_;

    needsRedraw_ = true;
    updateViewRange();
    markDirty();
}

void WaveformDisplay::clearAudioData() {
    std::lock_guard<std::mutex> lock(dataMutex_);

    waveformBuffer_.clear();
    audioDuration_ = 0.0;
    currentSampleRate_ = 44100.0;
    totalSamplesProcessed_ = 0;

    needsRedraw_ = true;
    updateViewRange();
    markDirty();
}

void WaveformDisplay::setMode(WaveformMode mode) {
    if (currentMode_ != mode) {
        currentMode_ = mode;
        needsRedraw_ = true;
        markDirty();
    }
}

void WaveformDisplay::setColors(const WaveformColors& newColors) {
    colors_ = newColors;
    needsRedraw_ = true;
    markDirty();
}

void WaveformDisplay::setShowGrid(bool show) {
    showGrid_ = show;
    markDirty();
}

void WaveformDisplay::setShowLabels(bool show) {
    showLabels_ = show;
    markDirty();
}

void WaveformDisplay::setShowRMS(bool show) {
    showRMS_ = show;
    markDirty();
}

void WaveformDisplay::setShowPeaks(bool show) {
    showPeaks_ = show;
    markDirty();
}

void WaveformDisplay::setZoomLevel(float zoom) {
    zoomLevel_ = juce::jlimit(0.1f, 100.0f, zoom);
    updateViewRange();
    markDirty();
}

void WaveformDisplay::setScrollPosition(float position) {
    scrollPosition_ = juce::jlimit(0.0f, 1.0f, position);
    updateViewRange();
    markDirty();
}

void WaveformDisplay::setViewRange(double startTime, double endTime) {
    if (endTime < startTime) {
        std::swap(startTime, endTime);
    }

    viewStartTime_ = std::max(0.0, startTime);
    viewEndTime_ = std::max(viewStartTime_ + 0.001, endTime);

    if (audioDuration_ > 0.0) {
        viewStartTime_ = std::min(viewStartTime_, audioDuration_);
        viewEndTime_ = std::min(viewEndTime_, audioDuration_);
    }

    scrollPosition_ = audioDuration_ > 0.0 ? static_cast<float>(viewStartTime_ / audioDuration_) : 0.0f;
    markDirty();
}

void WaveformDisplay::fitToWindow() {
    viewStartTime_ = 0.0;
    viewEndTime_ = audioDuration_;
    zoomLevel_ = 1.0f;
    scrollPosition_ = 0.0f;
    markDirty();
}

void WaveformDisplay::setSelection(double startTime, double endTime) {
    selectionStart_ = std::min(startTime, endTime);
    selectionEnd_ = std::max(startTime, endTime);
    hasSelectionFlag_ = true;
    markDirty();
}

void WaveformDisplay::clearSelection() {
    hasSelectionFlag_ = false;
    selectionStart_ = 0.0;
    selectionEnd_ = 0.0;
    markDirty();
}

std::pair<double, double> WaveformDisplay::getSelection() const {
    return {selectionStart_, selectionEnd_};
}

bool WaveformDisplay::hasSelection() const {
    return hasSelectionFlag_;
}

void WaveformDisplay::setPlaybackPosition(double time) {
    playbackPosition_ = time;
    markDirty();
}

void WaveformDisplay::clearPlaybackPosition() {
    playbackPosition_ = -1.0;
    markDirty();
}

double WaveformDisplay::getPlaybackPosition() const {
    return playbackPosition_;
}

void WaveformDisplay::showGenreDetection(bool show) {
    showGenreDetectionFlag_ = show;
    markDirty();
}

void WaveformDisplay::showQualityAnalysis(bool show) {
    showQualityAnalysisFlag_ = show;
    markDirty();
}

void WaveformDisplay::showSpectrumAnalysis(bool show) {
    showSpectrumAnalysisFlag_ = show;
    markDirty();
}

void WaveformDisplay::updateAnalysisData(const juce::String& analysis) {
    currentAnalysis_ = analysis;
    markDirty();
}

void WaveformDisplay::drawSkia(SkCanvas* canvas) {
    if (!canvas) {
        return;
    }

    drawBackground(canvas);

    if (showGrid_) {
        drawGrid(canvas);
    }

    drawWaveform(canvas);

    if (showRMS_) {
        drawRMS(canvas);
    }

    if (showPeaks_) {
        drawPeaks(canvas);
    }

    if (hasSelectionFlag_) {
        drawSelection(canvas);
    }

    if (playbackPosition_ >= 0.0) {
        drawPlaybackPosition(canvas);
    }

    if (showLabels_) {
        drawLabels(canvas);
    }

    if (showGenreDetectionFlag_ || showQualityAnalysisFlag_ || showSpectrumAnalysisFlag_) {
        drawAnalysisOverlay(canvas);
    }
}

void WaveformDisplay::resized() {
    needsRedraw_ = true;
    updateViewRange();
    markDirty();
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& event) {
    if (event.mods.isLeftButtonDown()) {
        double time = xToTime(event.position.x);
        startSelection(time);
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& event) {
    if (isSelecting_ && event.mods.isLeftButtonDown()) {
        double time = xToTime(event.position.x);
        updateSelection(time);
    }
}

void WaveformDisplay::mouseUp(const juce::MouseEvent&) {
    if (isSelecting_) {
        endSelection();
    }
}

void WaveformDisplay::mouseMove(const juce::MouseEvent& event) {
    double time = xToTime(event.position.x);
    float amplitude = yToAmplitude(event.position.y);

    // Update tooltip
    juce::String tooltip = formatTime(time) + " | " + formatAmplitude(amplitude);
    setTooltip(tooltip);
}

void WaveformDisplay::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    if (event.mods.isCommandDown()) {
        // Zoom with Ctrl+wheel
        float zoomDelta = wheel.deltaY > 0 ? 1.1f : 0.9f;
        setZoomLevel(zoomLevel_ * zoomDelta);
    } else {
        // Pan with wheel
        float scrollDelta = wheel.deltaY * 0.1f;
        setScrollPosition(scrollPosition_ + scrollDelta);
    }
}

void WaveformDisplay::timerCallback() {
    if (needsRedraw_) {
        needsRedraw_ = false;
        markDirty();
    } else {
        repaint();
    }
}

//==============================================================================
// Coordinate conversions
//==============================================================================

double WaveformDisplay::timeToX(double time) const {
    if (viewEndTime_ <= viewStartTime_) return 0.0;

    double viewDuration = viewEndTime_ - viewStartTime_;
    double relativeTime = time - viewStartTime_;
    double normalized = relativeTime / viewDuration;

    return normalized * getWidth();
}

double WaveformDisplay::xToTime(double x) const {
    if (getWidth() <= 0) return viewStartTime_;
    double normalized = x / getWidth();
    double relativeTime = normalized * (viewEndTime_ - viewStartTime_);
    return viewStartTime_ + relativeTime;
}

float WaveformDisplay::amplitudeToY(float amplitude) const {
    float normalized = (amplitude + 1.0f) * 0.5f;  // Convert -1..1 to 0..1
    return (1.0f - normalized) * getHeight();  // Flip Y axis
}

float WaveformDisplay::yToAmplitude(float y) const {
    float normalized = 1.0f - (y / getHeight());  // Flip Y axis
    return normalized * 2.0f - 1.0f;  // Convert 0..1 to -1..1
}

//==============================================================================
// Drawing helpers
//==============================================================================

void WaveformDisplay::drawBackground(SkCanvas* canvas) {
    canvas->clear(colors_.background);
}

void WaveformDisplay::drawGrid(SkCanvas* canvas) {
    SkPaint gridPaint;
    gridPaint.setAntiAlias(true);
    gridPaint.setStrokeWidth(1.0f);
    gridPaint.setColor(design::withAlpha(colors_.grid, gridOpacity_));

    int width = getWidth();
    int height = getHeight();

    // Calculate grid spacing
    double viewDuration = viewEndTime_ - viewStartTime_;
    if (viewDuration <= 0.0) {
        return;
    }

    // Major grid lines (per second by default)
    for (int i = 0; i <= static_cast<int>(viewDuration); i += std::max(1, majorGridInterval_)) {
        double time = viewStartTime_ + i;
        float x = static_cast<float>(timeToX(time));

        canvas->drawLine(x, 0.0f, x, static_cast<float>(height), gridPaint);

        // Time label
        if (showLabels_) {
            SkPaint textPaint;
            textPaint.setAntiAlias(true);
            textPaint.setColor(colors_.text);
            SkFont font = design::typography::getSkFont(10.0f, design::FontWeight::Regular);
            auto label = formatTime(time).toStdString();
            canvas->drawString(label.c_str(), x + 2.0f, static_cast<float>(height) - 6.0f, font, textPaint);
        }
    }

    // Minor grid lines
    int minorDivisions = std::max(1, minorGridInterval_);
    double minorInterval = (majorGridInterval_ > 0) ? (static_cast<double>(majorGridInterval_) / minorDivisions) : 0.0;
    if (minorInterval > 0.0) {
        SkPaint minorPaint = gridPaint;
        minorPaint.setColor(design::withAlpha(colors_.grid, gridOpacity_ * 0.5f));

        for (double t = viewStartTime_; t <= viewEndTime_; t += minorInterval) {
            if (std::fmod(t - viewStartTime_, majorGridInterval_) < 1e-6) {
                continue;
            }
            float x = static_cast<float>(timeToX(t));
            canvas->drawLine(x, 0.0f, x, static_cast<float>(height), minorPaint);
        }
    }

    // Horizontal reference lines
    canvas->drawLine(0.0f, static_cast<float>(height) * 0.5f, static_cast<float>(width), static_cast<float>(height) * 0.5f, gridPaint);
    canvas->drawLine(0.0f, static_cast<float>(height) * 0.25f, static_cast<float>(width), static_cast<float>(height) * 0.25f, gridPaint);
    canvas->drawLine(0.0f, static_cast<float>(height) * 0.75f, static_cast<float>(width), static_cast<float>(height) * 0.75f, gridPaint);
}

void WaveformDisplay::drawWaveform(SkCanvas* canvas) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    if (waveformBuffer_.empty()) {
        return;
    }

    switch (currentMode_) {
        case WaveformMode::Normal:
            drawNormalWaveform(canvas);
            break;
        case WaveformMode::Stereo:
            drawStereoWaveform(canvas);
            break;
        case WaveformMode::Spectrogram:
            drawSpectrogram(canvas);
            break;
        case WaveformMode::Phase:
            drawPhaseDisplay(canvas);
            break;
        default:
            drawNormalWaveform(canvas);
            break;
    }
}

void WaveformDisplay::drawNormalWaveform(SkCanvas* canvas) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(colors_.waveform);

    SkPath leftPath;
    SkPath rightPath;
    bool firstPoint = true;

    for (const auto& data : waveformBuffer_) {
        double time = data.timeSeconds;
        if (time >= viewStartTime_ && time <= viewEndTime_) {
            float x = static_cast<float>(timeToX(time));
            float leftY = amplitudeToY(data.leftChannel);
            float rightY = amplitudeToY(data.rightChannel);

            if (firstPoint) {
                leftPath.moveTo(x, leftY);
                rightPath.moveTo(x, rightY);
                firstPoint = false;
            } else {
                leftPath.lineTo(x, leftY);
                rightPath.lineTo(x, rightY);
            }
        }
    }

    canvas->drawPath(leftPath, paint);

    if (!waveformBuffer_.empty() && std::abs(waveformBuffer_.front().rightChannel) > 0.0f) {
        SkPaint rightPaint = paint;
        rightPaint.setColor(design::withAlpha(colors_.waveform, 0.5f));
        canvas->drawPath(rightPath, rightPaint);
    }
}

void WaveformDisplay::drawStereoWaveform(SkCanvas* canvas) {
    int halfHeight = getHeight() / 2;

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(colors_.waveform);

    SkPath leftPath;
    SkPath rightPath;
    bool firstPoint = true;

    for (const auto& data : waveformBuffer_) {
        double time = data.timeSeconds;
        if (time >= viewStartTime_ && time <= viewEndTime_) {
            float x = static_cast<float>(timeToX(time));
            float y = amplitudeToY(data.leftChannel) * 0.5f;

            if (firstPoint) {
                leftPath.moveTo(x, y);
                firstPoint = false;
            } else {
                leftPath.lineTo(x, y);
            }
        }
    }

    canvas->drawPath(leftPath, paint);

    firstPoint = true;
    for (const auto& data : waveformBuffer_) {
        double time = data.timeSeconds;
        if (time >= viewStartTime_ && time <= viewEndTime_) {
            float x = static_cast<float>(timeToX(time));
            float y = amplitudeToY(data.rightChannel) * 0.5f + static_cast<float>(halfHeight);

            if (firstPoint) {
                rightPath.moveTo(x, y);
                firstPoint = false;
            } else {
                rightPath.lineTo(x, y);
            }
        }
    }

    canvas->drawPath(rightPath, paint);

    // Divider line
    SkPaint dividerPaint = paint;
    dividerPaint.setColor(colors_.grid);
    canvas->drawLine(0.0f, static_cast<float>(halfHeight), static_cast<float>(getWidth()), static_cast<float>(halfHeight), dividerPaint);
}

void WaveformDisplay::drawSpectrogram(SkCanvas* canvas) {
    if (waveformBuffer_.empty()) {
        return;
    }

    // Simple energy-based heatmap (time vs energy).
    // Uses RMS values to provide a meaningful visual signal without fake FFT data.
    int width = getWidth();
    int height = getHeight();

    // Count points in view
    int pointsInView = 0;
    for (const auto& data : waveformBuffer_) {
        if (data.timeSeconds >= viewStartTime_ && data.timeSeconds <= viewEndTime_) {
            ++pointsInView;
        }
    }

    if (pointsInView <= 0) {
        return;
    }

    float barWidth = std::max(1.0f, static_cast<float>(width) / static_cast<float>(pointsInView));
    SkPaint paint;
    paint.setAntiAlias(false);

    for (const auto& data : waveformBuffer_) {
        double time = data.timeSeconds;
        if (time < viewStartTime_ || time > viewEndTime_) {
            continue;
        }

        float x = static_cast<float>(timeToX(time));
        float intensity = juce::jlimit(0.0f, 1.0f, data.rms * 2.0f);

        // Map intensity to a cyan/magenta ramp
        uint8_t r = static_cast<uint8_t>(40 + intensity * 180.0f);
        uint8_t g = static_cast<uint8_t>(80 + intensity * 150.0f);
        uint8_t b = static_cast<uint8_t>(160 + intensity * 80.0f);
        paint.setColor(SkColorSetARGB(200, r, g, b));

        SkRect bar = SkRect::MakeXYWH(x, 0.0f, barWidth, static_cast<float>(height));
        canvas->drawRect(bar, paint);
    }
}

void WaveformDisplay::drawPhaseDisplay(SkCanvas* canvas) {
    if (waveformBuffer_.empty()) {
        return;
    }

    float centerX = static_cast<float>(getWidth()) * 0.5f;
    float centerY = static_cast<float>(getHeight()) * 0.5f;
    float halfW = centerX;
    float halfH = centerY;

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(colors_.waveform);

    SkPath phasePath;
    bool firstPoint = true;

    for (const auto& data : waveformBuffer_) {
        double time = data.timeSeconds;
        if (time < viewStartTime_ || time > viewEndTime_) {
            continue;
        }

        float x = centerX + data.leftChannel * halfW;
        float y = centerY - data.rightChannel * halfH;

        if (firstPoint) {
            phasePath.moveTo(x, y);
            firstPoint = false;
        } else {
            phasePath.lineTo(x, y);
        }
    }

    // Draw crosshair
    SkPaint gridPaint = paint;
    gridPaint.setColor(design::withAlpha(colors_.grid, 0.6f));
    canvas->drawLine(centerX, 0.0f, centerX, static_cast<float>(getHeight()), gridPaint);
    canvas->drawLine(0.0f, centerY, static_cast<float>(getWidth()), centerY, gridPaint);

    canvas->drawPath(phasePath, paint);
}

void WaveformDisplay::drawRMS(SkCanvas* canvas) {
    if (!showRMS_) return;

    std::lock_guard<std::mutex> lock(dataMutex_);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setColor(colors_.rms);

    SkPath rmsPath;
    bool firstPoint = true;

    for (const auto& data : waveformBuffer_) {
        double time = data.timeSeconds;
        if (time >= viewStartTime_ && time <= viewEndTime_) {
            float x = static_cast<float>(timeToX(time));
            float y = amplitudeToY(data.rms);

            if (firstPoint) {
                rmsPath.moveTo(x, y);
                firstPoint = false;
            } else {
                rmsPath.lineTo(x, y);
            }
        }
    }

    canvas->drawPath(rmsPath, paint);
}

void WaveformDisplay::drawPeaks(SkCanvas* canvas) {
    if (!showPeaks_) return;

    std::lock_guard<std::mutex> lock(dataMutex_);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);

    int height = getHeight();

    for (const auto& data : waveformBuffer_) {
        if (!data.isClipping) {
            continue;
        }

        double time = data.timeSeconds;
        if (time < viewStartTime_ || time > viewEndTime_) {
            continue;
        }

        float x = static_cast<float>(timeToX(time));
        paint.setColor(colors_.clipping);
        canvas->drawLine(x, 0.0f, x, static_cast<float>(height), paint);
        canvas->drawCircle(x, 6.0f, 3.0f, paint);
    }
}

void WaveformDisplay::drawSelection(SkCanvas* canvas) {
    if (!hasSelectionFlag_) return;

    float startX = static_cast<float>(timeToX(selectionStart_));
    float endX = static_cast<float>(timeToX(selectionEnd_));

    if (startX > endX) {
        std::swap(startX, endX);
    }

    SkRect rect = SkRect::MakeLTRB(startX, 0.0f, endX, static_cast<float>(getHeight()));

    SkPaint fillPaint;
    fillPaint.setAntiAlias(false);
    fillPaint.setStyle(SkPaint::kFill_Style);
    fillPaint.setColor(colors_.selection);
    canvas->drawRect(rect, fillPaint);

    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(2.0f);
    borderPaint.setColor(colors_.waveform);
    canvas->drawRect(rect, borderPaint);
}

void WaveformDisplay::drawPlaybackPosition(SkCanvas* canvas) {
    if (playbackPosition_ < 0.0) return;

    float x = static_cast<float>(timeToX(playbackPosition_));

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorWHITE);
    paint.setStrokeWidth(1.0f);

    canvas->drawLine(x, 0.0f, x, static_cast<float>(getHeight()), paint);

    SkPath triangle;
    triangle.moveTo(x - 5.0f, 0.0f);
    triangle.lineTo(x + 5.0f, 0.0f);
    triangle.lineTo(x, 10.0f);
    triangle.close();

    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawPath(triangle, paint);
}

void WaveformDisplay::drawLabels(SkCanvas* canvas) {
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors_.text);
    SkFont font = design::typography::getSkFont(12.0f, design::FontWeight::Regular);

    canvas->drawString("+6 dB", 5.0f, 15.0f, font, textPaint);
    canvas->drawString("0 dB", 5.0f, static_cast<float>(getHeight()) * 0.5f + 5.0f, font, textPaint);
    canvas->drawString("-6 dB", 5.0f, static_cast<float>(getHeight()) - 5.0f, font, textPaint);

    // Time labels
    auto startLabel = formatTime(viewStartTime_).toStdString();
    auto endLabel = formatTime(viewEndTime_).toStdString();
    canvas->drawString(startLabel.c_str(), 5.0f, static_cast<float>(getHeight()) - 20.0f, font, textPaint);
    canvas->drawString(endLabel.c_str(), static_cast<float>(getWidth()) - 65.0f, static_cast<float>(getHeight()) - 20.0f, font, textPaint);
}

void WaveformDisplay::drawAnalysisOverlay(SkCanvas* canvas) {
    if (currentAnalysis_.isEmpty()) return;

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorWHITE);
    SkFont font = design::typography::getSkFont(14.0f, design::FontWeight::Medium);

    auto text = currentAnalysis_.toStdString();
    canvas->drawString(text.c_str(), 10.0f, 24.0f, font, textPaint);
}

//==============================================================================
// Data processing
//==============================================================================

void WaveformDisplay::processAudioData(const juce::AudioBuffer<float>& buffer) {
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if (numSamples <= 0 || currentSampleRate_ <= 0.0) {
        return;
    }

    // Process samples in chunks
    int chunkSize = 1024;  // Process 1024 samples at a time

    for (int start = 0; start < numSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, numSamples);
        int chunkLength = end - start;

        WaveformData data;
        data.leftChannel = 0.0f;
        data.rightChannel = 0.0f;
        data.rms = 0.0f;
        data.peak = 0.0f;
        data.isClipping = false;

        // Timestamp based on audio time (not wall time)
        double chunkMidSample = static_cast<double>(totalSamplesProcessed_ + start) + (static_cast<double>(chunkLength) * 0.5);
        data.timeSeconds = chunkMidSample / currentSampleRate_;
        data.timestamp = juce::Time::getCurrentTime();

        // Calculate RMS and peak
        for (int ch = 0; ch < numChannels; ++ch) {
            for (int i = start; i < end; ++i) {
                float sample = buffer.getSample(ch, i);

                if (ch == 0) {
                    data.leftChannel += sample;
                } else if (ch == 1) {
                    data.rightChannel += sample;
                }

                data.rms += sample * sample;
                data.peak = std::max(data.peak, std::abs(sample));

                if (std::abs(sample) > 0.99f) {
                    data.isClipping = true;
                }
            }
        }

        // Average the values
        data.leftChannel /= static_cast<float>(chunkLength);
        if (numChannels > 1) {
            data.rightChannel /= static_cast<float>(chunkLength);
        }

        data.rms = std::sqrt(data.rms / static_cast<float>(chunkLength * numChannels));

        waveformBuffer_.push_back(data);
    }

    totalSamplesProcessed_ += numSamples;
}

void WaveformDisplay::updateViewRange() {
    if (audioDuration_ > 0.0) {
        double viewDuration = audioDuration_ / zoomLevel_;
        viewStartTime_ = scrollPosition_ * audioDuration_;
        viewEndTime_ = viewStartTime_ + viewDuration;

        // Clamp to audio duration
        if (viewEndTime_ > audioDuration_) {
            viewEndTime_ = audioDuration_;
            viewStartTime_ = std::max(0.0, viewEndTime_ - viewDuration);
        }

        if (viewStartTime_ < 0.0) {
            viewStartTime_ = 0.0;
            viewEndTime_ = std::min(audioDuration_, viewDuration);
        }
    } else {
        viewStartTime_ = 0.0;
        viewEndTime_ = 0.0;
    }
}

void WaveformDisplay::optimizeBuffer() {
    if (waveformBuffer_.size() > MAX_BUFFER_SIZE) {
        size_t removeCount = waveformBuffer_.size() - MAX_BUFFER_SIZE;
        waveformBuffer_.erase(waveformBuffer_.begin(), waveformBuffer_.begin() + removeCount);
    }
}

void WaveformDisplay::startSelection(double time) {
    isSelecting_ = true;
    selectionAnchor_ = time;
    selectionStart_ = time;
    selectionEnd_ = time;
    hasSelectionFlag_ = true;
    markDirty();
}

void WaveformDisplay::updateSelection(double time) {
    if (isSelecting_) {
        selectionStart_ = std::min(selectionAnchor_, time);
        selectionEnd_ = std::max(selectionAnchor_, time);
        markDirty();
    }
}

void WaveformDisplay::endSelection() {
    isSelecting_ = false;
    hasSelectionFlag_ = (std::abs(selectionEnd_ - selectionStart_) > 0.01);  // Min 10ms selection
    markDirty();
}

juce::String WaveformDisplay::formatTime(double time) const {
    int minutes = static_cast<int>(time) / 60;
    int seconds = static_cast<int>(time) % 60;
    int milliseconds = static_cast<int>((time - static_cast<int>(time)) * 1000);

    if (minutes > 0) {
        return juce::String::formatted("%02d:%02d.%03d", minutes, seconds, milliseconds);
    }
    return juce::String::formatted("%d.%03d", seconds, milliseconds);
}

juce::String WaveformDisplay::formatAmplitude(float amplitude) const {
    float db = juce::Decibels::gainToDecibels(std::abs(amplitude));
    return juce::String(db, 1) + " dB";
}

//==============================================================================
// WaveformControlPanel Implementation
//==============================================================================
WaveformControlPanel::WaveformControlPanel(WaveformDisplay& display)
    : waveformDisplay_(display) {

    createDisplayControls();
    createZoomControls();
    createNavigationControls();
    createAnalysisControls();
    createExportControls();
}

WaveformControlPanel::~WaveformControlPanel() = default;

void WaveformControlPanel::drawSkia(SkCanvas* canvas) {
    if (!canvas) {
        return;
    }

    SkRect bounds = SkRect::MakeWH(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(design::colors::BG_02);
    canvas->drawRect(bounds, paint);
}

void WaveformControlPanel::resized() {
    auto bounds = getLocalBounds();

    // Layout controls in rows
    int rowHeight = 30;
    int margin = 5;

    // Row 1: Display controls
    int y = margin;
    modeComboBox_->setBounds(margin, y, 100, rowHeight);
    gridToggle_->setBounds(110, y, 60, rowHeight);
    labelsToggle_->setBounds(175, y, 60, rowHeight);
    rmsToggle_->setBounds(240, y, 50, rowHeight);
    peaksToggle_->setBounds(295, y, 50, rowHeight);

    // Row 2: Zoom controls
    y += rowHeight + margin;
    zoomSlider_->setBounds(margin, y, 200, rowHeight);
    zoomInButton_->setBounds(210, y, 40, rowHeight);
    zoomOutButton_->setBounds(255, y, 40, rowHeight);
    fitToWindowButton_->setBounds(300, y, 80, rowHeight);

    // Row 3: Navigation controls
    y += rowHeight + margin;
    goStartButton_->setBounds(margin, y, 60, rowHeight);
    goEndButton_->setBounds(70, y, 60, rowHeight);
    playPauseButton_->setBounds(140, y, 60, rowHeight);
    loopButton_->setBounds(210, y, 60, rowHeight);

    // Row 4: Analysis controls
    y += rowHeight + margin;
    genreToggle_->setBounds(margin, y, 80, rowHeight);
    qualityToggle_->setBounds(90, y, 80, rowHeight);
    spectrumToggle_->setBounds(180, y, 80, rowHeight);

    // Row 5: Export controls
    y += rowHeight + margin;
    exportButton_->setBounds(margin, y, 60, rowHeight);
    screenshotButton_->setBounds(70, y, 80, rowHeight);
}

void WaveformControlPanel::buttonClicked(juce::Button* button) {
    if (button == gridToggle_.get()) {
        waveformDisplay_.setShowGrid(gridToggle_->getToggleState());
        return;
    }
    if (button == labelsToggle_.get()) {
        waveformDisplay_.setShowLabels(labelsToggle_->getToggleState());
        return;
    }
    if (button == rmsToggle_.get()) {
        waveformDisplay_.setShowRMS(rmsToggle_->getToggleState());
        return;
    }
    if (button == peaksToggle_.get()) {
        waveformDisplay_.setShowPeaks(peaksToggle_->getToggleState());
        return;
    }
    if (button == genreToggle_.get()) {
        waveformDisplay_.showGenreDetection(genreToggle_->getToggleState());
        return;
    }
    if (button == qualityToggle_.get()) {
        waveformDisplay_.showQualityAnalysis(qualityToggle_->getToggleState());
        return;
    }
    if (button == spectrumToggle_.get()) {
        waveformDisplay_.showSpectrumAnalysis(spectrumToggle_->getToggleState());
        return;
    }

    if (button == zoomInButton_.get()) {
        waveformDisplay_.setZoomLevel(waveformDisplay_.getZoomLevel() * 1.5f);
    } else if (button == zoomOutButton_.get()) {
        waveformDisplay_.setZoomLevel(waveformDisplay_.getZoomLevel() / 1.5f);
    } else if (button == fitToWindowButton_.get()) {
        waveformDisplay_.fitToWindow();
    } else if (button == goStartButton_.get()) {
        waveformDisplay_.setScrollPosition(0.0f);
    } else if (button == goEndButton_.get()) {
        waveformDisplay_.setScrollPosition(1.0f);
    } else if (button == playPauseButton_.get()) {
        // Toggle playback
    } else if (button == loopButton_.get()) {
        // Toggle loop
    } else if (button == exportButton_.get()) {
        // Export waveform
    } else if (button == screenshotButton_.get()) {
        // Take screenshot
    }
}

void WaveformControlPanel::comboBoxChanged(juce::ComboBox* comboBox) {
    if (comboBox == modeComboBox_.get()) {
        int selectedId = comboBox->getSelectedId();
        switch (selectedId) {
            case 1: waveformDisplay_.setMode(WaveformMode::Normal); break;
            case 2: waveformDisplay_.setMode(WaveformMode::Stereo); break;
            case 3: waveformDisplay_.setMode(WaveformMode::Spectrogram); break;
            case 4: waveformDisplay_.setMode(WaveformMode::Phase); break;
            default: waveformDisplay_.setMode(WaveformMode::Normal); break;
        }
    }
}

void WaveformControlPanel::sliderValueChanged(juce::Slider* slider) {
    if (slider == zoomSlider_.get()) {
        waveformDisplay_.setZoomLevel(static_cast<float>(slider->getValue()));
    }
}

void WaveformControlPanel::createDisplayControls() {
    modeComboBox_ = std::make_unique<juce::ComboBox>();
    modeComboBox_->addItem("Normal", 1);
    modeComboBox_->addItem("Stereo", 2);
    modeComboBox_->addItem("Spectrogram", 3);
    modeComboBox_->addItem("Phase", 4);
    modeComboBox_->setSelectedId(1);
    modeComboBox_->addListener(this);
    addAndMakeVisible(*modeComboBox_);

    gridToggle_ = std::make_unique<juce::ToggleButton>("Grid");
    gridToggle_->setToggleState(true, juce::dontSendNotification);
    gridToggle_->addListener(this);
    addAndMakeVisible(*gridToggle_);

    labelsToggle_ = std::make_unique<juce::ToggleButton>("Labels");
    labelsToggle_->setToggleState(true, juce::dontSendNotification);
    labelsToggle_->addListener(this);
    addAndMakeVisible(*labelsToggle_);

    rmsToggle_ = std::make_unique<juce::ToggleButton>("RMS");
    rmsToggle_->setToggleState(true, juce::dontSendNotification);
    rmsToggle_->addListener(this);
    addAndMakeVisible(*rmsToggle_);

    peaksToggle_ = std::make_unique<juce::ToggleButton>("Peaks");
    peaksToggle_->setToggleState(true, juce::dontSendNotification);
    peaksToggle_->addListener(this);
    addAndMakeVisible(*peaksToggle_);
}

void WaveformControlPanel::createZoomControls() {
    zoomSlider_ = std::make_unique<juce::Slider>("Zoom");
    zoomSlider_->setRange(0.1f, 100.0f, 0.1f);
    zoomSlider_->setValue(1.0f);
    zoomSlider_->setSliderStyle(juce::Slider::LinearHorizontal);
    zoomSlider_->addListener(this);
    addAndMakeVisible(*zoomSlider_);

    zoomInButton_ = std::make_unique<juce::TextButton>("+");
    zoomInButton_->addListener(this);
    addAndMakeVisible(*zoomInButton_);

    zoomOutButton_ = std::make_unique<juce::TextButton>("-");
    zoomOutButton_->addListener(this);
    addAndMakeVisible(*zoomOutButton_);

    fitToWindowButton_ = std::make_unique<juce::TextButton>("Fit");
    fitToWindowButton_->addListener(this);
    addAndMakeVisible(*fitToWindowButton_);
}

void WaveformControlPanel::createNavigationControls() {
    goStartButton_ = std::make_unique<juce::TextButton>("|<<");
    goStartButton_->addListener(this);
    addAndMakeVisible(*goStartButton_);

    goEndButton_ = std::make_unique<juce::TextButton>(">>|");
    goEndButton_->addListener(this);
    addAndMakeVisible(*goEndButton_);

    playPauseButton_ = std::make_unique<juce::TextButton>("Play");
    playPauseButton_->addListener(this);
    addAndMakeVisible(*playPauseButton_);

    loopButton_ = std::make_unique<juce::TextButton>("Loop");
    loopButton_->addListener(this);
    addAndMakeVisible(*loopButton_);
}

void WaveformControlPanel::createAnalysisControls() {
    genreToggle_ = std::make_unique<juce::ToggleButton>("Genre");
    genreToggle_->addListener(this);
    addAndMakeVisible(*genreToggle_);

    qualityToggle_ = std::make_unique<juce::ToggleButton>("Quality");
    qualityToggle_->addListener(this);
    addAndMakeVisible(*qualityToggle_);

    spectrumToggle_ = std::make_unique<juce::ToggleButton>("Spectrum");
    spectrumToggle_->addListener(this);
    addAndMakeVisible(*spectrumToggle_);
}

void WaveformControlPanel::createExportControls() {
    exportButton_ = std::make_unique<juce::TextButton>("Export");
    exportButton_->addListener(this);
    addAndMakeVisible(*exportButton_);

    screenshotButton_ = std::make_unique<juce::TextButton>("Screenshot");
    screenshotButton_->addListener(this);
    addAndMakeVisible(*screenshotButton_);
}

//==============================================================================
// WaveformContainer Implementation
//==============================================================================
WaveformContainer::WaveformContainer() {
    waveformDisplay_ = std::make_unique<WaveformDisplay>();
    controlPanel_ = std::make_unique<WaveformControlPanel>(*waveformDisplay_);

    addAndMakeVisible(*waveformDisplay_);
    addAndMakeVisible(*controlPanel_);
}

WaveformContainer::~WaveformContainer() = default;

void WaveformContainer::drawSkia(SkCanvas* canvas) {
    if (!canvas) {
        return;
    }

    SkRect bounds = SkRect::MakeWH(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(design::colors::BG_01);
    canvas->drawRect(bounds, paint);
}

void WaveformContainer::resized() {
    auto bounds = getLocalBounds();

    // Control panel at top
    int controlPanelHeight = 150;
    controlPanel_->setBounds(bounds.removeFromTop(controlPanelHeight));

    // Waveform display takes remaining space
    waveformDisplay_->setBounds(bounds);
}

void WaveformContainer::setColors(const WaveformColors& colors) {
    waveformDisplay_->setColors(colors);
}

void WaveformContainer::setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    waveformDisplay_->setAudioData(buffer, sampleRate);
}

} // namespace ui
} // namespace zenith
