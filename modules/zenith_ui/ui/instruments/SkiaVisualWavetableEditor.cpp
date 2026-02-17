/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "SkiaVisualWavetableEditor.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace zenith {

//==============================================================================
// WavetableData Implementation
//==============================================================================

void WavetableData::resize(int numFrames, int frameSize) {
    frames_.resize(numFrames);
    frameSize_ = frameSize;

    for (auto& frame : frames_) {
        frame.resize(frameSize);
    }
}

bool WavetableData::importWAV(const juce::File& file) {
    std::unique_ptr<juce::InputStream> stream(file.createInputStream());
    return stream ? importWAVStream(*stream) : false;
}

bool WavetableData::importWAVStream(juce::InputStream& stream) {
    // Parse WAV file and extract frames
    // This is a simplified implementation
    // In production, handle all WAV formats properly

    if (stream.readByte() != 'R' || stream.readByte() != 'I' ||
        stream.readByte() != 'F' || stream.readByte() != 'F') {
        return false;
    }

    // Skip to data chunk
    stream.skipNextBytes(8);

    int format = stream.readIntBigEndian();
    while (format != 0x64617461) {  // "data"
        int chunkSize = stream.readIntLittleEndian();
        stream.skipNextBytes(chunkSize);
        format = stream.readIntBigEndian();
    }

    int dataSize = stream.readIntLittleEndian();
    int numSamples = dataSize / 2;  // 16-bit

    // Determine frame layout based on size
    int frameSize = WavetableData::defaultFrameSize;
    int numFrames = (numSamples + frameSize - 1) / frameSize;

    resize(numFrames, frameSize);

    // Read samples
    std::vector<int16> rawSamples(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        rawSamples[i] = stream.readShortLittleEndian();
    }

    // Distribute into frames
    for (int f = 0; f < numFrames; ++f) {
        for (int s = 0; s < frameSize; ++s) {
            int srcIndex = f * frameSize + s;
            if (srcIndex < numSamples) {
                frames_[f].samples[s] = static_cast<float>(rawSamples[srcIndex]) / 32768.0f;
            }
        }
        frames_[f].normalize();
    }

    return true;
}

bool WavetableData::exportWAV(const juce::File& file) const {
    std::unique_ptr<juce::OutputStream> stream(file.createOutputStream());
    return stream ? exportWAVStream(*stream) : false;
}

bool WavetableData::exportWAVStream(juce::OutputStream& stream) const {
    // Write WAV file header
    stream.writeIntBigEndian('R');
    stream.writeIntBigEndian('I');
    stream.writeIntBigEndian('F');
    stream.writeIntBigEndian('F');

    int dataSize = frameSize_ * frames_.size() * 2;
    int fileSize = 36 + dataSize;

    stream.writeIntLittleEndian(fileSize);
    stream.writeIntBigEndian('W');
    stream.writeIntBigEndian('A');
    stream.writeIntBigEndian('V');
    stream.writeIntBigEndian('E');

    // fmt chunk
    stream.writeIntBigEndian('f');
    stream.writeIntBigEndian('m');
    stream.writeIntBigEndian('t');
    stream.writeIntBigEndian(' ');

    int fmtSize = 16;
    stream.writeIntLittleEndian(fmtSize);
    stream.writeShortLittleEndian(1);  // PCM
    stream.writeShortLittleEndian(1);  // Mono
    stream.writeIntLittleEndian(44100);  // Sample rate (adjustable)
    stream.writeIntLittleEndian(44100 * 2);  // Byte rate
    stream.writeShortLittleEndian(2);  // Block align
    stream.writeShortLittleEndian(16);  // Bits per sample

    // data chunk
    stream.writeIntBigEndian('d');
    stream.writeIntBigEndian('a');
    stream.writeIntBigEndian('t');
    stream.writeIntBigEndian('a');
    stream.writeIntLittleEndian(dataSize);

    // Write samples
    for (const auto& frame : frames_) {
        for (float sample : frame.samples) {
            int16 clamped = static_cast<int16>(juce::jlimit(-32768, 32767,
                static_cast<int>(sample * 32767)));
            stream.writeShortLittleEndian(clamped);
        }
    }

    return true;
}

void WavetableData::normalizeAllFrames() {
    for (auto& frame : frames_) {
        frame.normalize();
    }
}

void WavetableData::interpolateFrames(int numInterpolatedFrames) {
    if (frames_.size() < 2) return;

    std::vector<WavetableFrame> newFrames;
    newFrames.reserve(frames_.size() + numInterpolatedFrames * (frames_.size() - 1));

    for (size_t i = 0; i < frames_.size() - 1; ++i) {
        newFrames.push_back(frames_[i]);

        // Add interpolated frames
        for (int j = 1; j <= numInterpolatedFrames; ++j) {
            float t = static_cast<float>(j) / (numInterpolatedFrames + 1);
            WavetableFrame interpolated(frameSize_);

            for (int s = 0; s < frameSize_; ++s) {
                interpolated.samples[s] = (1.0f - t) * frames_[i].samples[s] +
                                          t * frames_[i + 1].samples[s];
            }
            newFrames.push_back(interpolated);
        }
    }
    newFrames.push_back(frames_.back());

    frames_ = newFrames;
}

void WavetableData::smoothAllFrames(float amount) {
    for (auto& frame : frames_) {
        // Simple moving average
        std::vector<float> smoothed = frame.samples;

        for (int i = 1; i < frameSize_ - 1; ++i) {
            smoothed[i] = (1.0f - amount) * frame.samples[i] +
                         amount * 0.5f * (frame.samples[i - 1] + frame.samples[i + 1]);
        }

        frame.samples = smoothed;
    }
}

std::vector<float> WavetableData::analyzeSpectrum(int frameIndex) const {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames_.size())) {
        return {};
    }

    const auto& frame = frames_[frameIndex];
    std::vector<float> spectrum;

    // Simple DFT for harmonic analysis
    int numHarmonics = 16;
    for (int h = 0; h < numHarmonics; ++h) {
        float real = 0.0f;
        float imag = 0.0f;

        for (size_t i = 0; i < frame.samples.size(); ++i) {
            float phase = 2.0f * juce::MathConstants<float>::pi * h * i / frame.samples.size();
            real += frame.samples[i] * std::cos(phase);
            imag -= frame.samples[i] * std::sin(phase);
        }

        float magnitude = std::sqrt(real * real + imag * imag) / frame.samples.size();
        spectrum.push_back(magnitude);
    }

    return spectrum;
}

float WavetableData::estimateFundamental(int frameIndex) const {
    // Use zero-crossing rate for fundamental estimation
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames_.size())) {
        return 0.0f;
    }

    const auto& frame = frames_[frameIndex];
    int crossings = 0;

    for (size_t i = 1; i < frame.samples.size(); ++i) {
        if ((frame.samples[i - 1] >= 0 && frame.samples[i] < 0) ||
            (frame.samples[i - 1] < 0 && frame.samples[i] >= 0)) {
            crossings++;
        }
    }

    if (crossings == 0) return 0.0f;

    float wavelength = static_cast<float>(frame.samples.size()) / (crossings * 0.5f);
    return wavelength;
}

//==============================================================================
// SkiaVisualWavetableEditor Implementation
//==============================================================================

SkiaVisualWavetableEditor::SkiaVisualWavetableEditor() {
    // Create default wavetable
    wavetableData_ = std::make_shared<WavetableData>();
    wavetableData_->resize(8, 2048);

    // Initialize with sine waves of varying phase
    for (int f = 0; f < 8; ++f) {
        auto& frame = wavetableData_->getFrame(f);
        for (int i = 0; i < frame.size(); ++i) {
            float phase = static_cast<float>(i) / frame.size();
            frame.samples[i] = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
        }
    }

    sampleSelection_.resize(2048, false);
    startTimerHz(30);  // 30 FPS for animation
}

void SkiaVisualWavetableEditor::setWavetableData(std::shared_ptr<WavetableData> data) {
    wavetableData_ = data;
    if (wavetableData_ && !wavetableData_->isEmpty()) {
        sampleSelection_.resize(wavetableData_->getFrameSize(), false);
    }
    markDirty();
}

void SkiaVisualWavetableEditor::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Draw background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(255, 15, 15, 20));
    canvas->drawRect(bounds, bgPaint);

    // Draw based on view mode
    switch (viewMode_) {
        case ViewMode::Waveform:
            drawWaveformView(canvas, bounds);
            break;
        case ViewMode::Spectrogram:
            drawSpectrogramView(canvas, bounds);
            break;
        case ViewMode::3D:
            draw3DView(canvas, bounds);
            break;
        case ViewMode::Matrix:
            drawMatrixView(canvas, bounds);
            break;
        case ViewMode::Harmonic:
            drawHarmonicView(canvas, bounds);
            break;
    }

    // Draw overlays
    if (showGrid_) drawGrid(canvas, bounds);
    if (showRulers_) drawRulers(canvas, bounds);
    if (showCrosshair_) drawCrosshair(canvas, bounds);
    drawFrameMarkers(canvas, bounds);
    drawSelection(canvas, bounds);
    drawPlaybackIndicator(canvas, bounds);
}

void SkiaVisualWavetableEditor::drawWaveformView(SkCanvas* canvas, const SkRect& bounds) {
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    const auto& frame = wavetableData_->getFrame(currentFrame_);

    SkPaint waveformPaint;
    waveformPaint.setAntiAlias(true);
    waveformPaint.setStyle(SkPaint::kStroke_Style);
    waveformPaint.setStrokeWidth(1.5f);
    waveformPaint.setColor(waveformColor_);

    if (isGlowEnabled()) {
        waveformPaint.setMaskFilter(SkMaskFilter::MakeBlur(
            kNormal_SkBlurStyle, 3.0f));
    }

    SkPath waveformPath;
    float centerY = bounds.centerY();
    float amplitude = bounds.height() * 0.4f;
    float startX = bounds.fLeft + 20;
    float scaleX = (bounds.width() - 40) / frame.size();

    waveformPath.moveTo(startX, centerY);

    for (int i = 0; i < frame.size(); ++i) {
        float x = startX + i * scaleX * zoomLevel_ + scrollOffsetX_;
        float y = centerY - frame.samples[i] * amplitude;

        if (x >= bounds.fLeft && x <= bounds.fRight) {
            waveformPath.lineTo(x, y);
        }
    }

    canvas->drawPath(waveformPath, waveformPaint);

    // Draw zero line
    SkPaint zeroPaint;
    zeroPaint.setStyle(SkPaint::kStroke_Style);
    zeroPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
    canvas->drawLine(bounds.fLeft, centerY, bounds.fRight, centerY, zeroPaint);
}

void SkiaVisualWavetableEditor::drawSpectrogramView(SkCanvas* canvas, const SkRect& bounds) {
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    int numFrames = wavetableData_->getNumFrames();
    int frameSize = wavetableData_->getFrameSize();
    int displayBins = 64;  // Number of frequency bins to display

    float cellWidth = bounds.width() / numFrames;
    float cellHeight = bounds.height() / displayBins;

    for (int f = 0; f < numFrames; ++f) {
        auto spectrum = wavetableData_->analyzeSpectrum(f);

        for (int b = 0; b < displayBins && b < static_cast<int>(spectrum.size()); ++b) {
            float magnitude = spectrum[b];
            float brightness = juce::jlimit(0.0f, 1.0f, magnitude * 2.0f);

            SkColor color = SkColorSetARGB(
                static_cast<U8CPU>(brightness * 255),
                static_cast<U8CPU>(brightness * 200),
                static_cast<U8CPU>(brightness * 255),
                static_cast<U8CPU>(brightness * 100)
            );

            SkPaint cellPaint;
            cellPaint.setColor(color);

            SkRect cellRect = SkRect::MakeXYWH(
                bounds.fLeft + f * cellWidth,
                bounds.fBottom - (b + 1) * cellHeight,
                cellWidth + 1,
                cellHeight + 1
            );

            canvas->drawRect(cellRect, cellPaint);
        }
    }
}

void SkiaVisualWavetableEditor::draw3DView(SkCanvas* canvas, const SkRect& bounds) {
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    // Create a 3D perspective effect
    int numFrames = wavetableData_->getNumFrames();

    // Perspective parameters
    float vanishingPointX = bounds.centerX();
    float vanishingPointY = bounds.fTop - bounds.height() * 0.3f;

    for (int f = 0; f < numFrames; ++f) {
        const auto& frame = wavetableData_->getFrame(f);

        // Calculate perspective scale for this frame
        float depth = static_cast<float>(f) / (numFrames - 1);
        float perspectiveScale = 0.6f + depth * 0.4f;

        float frameY = bounds.fBottom - bounds.height() * 0.1f - depth * bounds.height() * 0.7f;
        float frameHeight = bounds.height() * 0.4f * perspectiveScale;

        SkPaint framePaint;
        framePaint.setAntiAlias(true);
        framePaint.setStyle(SkPaint::kStroke_Style);
        framePaint.setStrokeWidth(1.0f);

        SkColor frameColor = getFrameColor(f);
        framePaint.setColor(SkColorSetARGB(
            f == currentFrame_ ? 255 : 100,
            SkColorGetR(frameColor),
            SkColorGetG(frameColor),
            SkColorGetB(frameColor)
        ));

        SkPath framePath;
        float startX = bounds.fLeft + bounds.width() * 0.1f;
        float width = bounds.width() * 0.8f;

        framePath.moveTo(startX, frameY);

        for (int i = 0; i < 256; ++i) {  // Reduced resolution
            int sampleIndex = i * frame.size() / 256;
            float x = startX + (i / 256.0f) * width;
            float y = frameY - frame.samples[sampleIndex] * frameHeight;

            framePath.lineTo(x, y);
        }

        canvas->drawPath(framePath, framePaint);
    }
}

void SkiaVisualWavetableEditor::drawMatrixView(SkCanvas* canvas, const SkRect& bounds) {
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    int numFrames = wavetableData_->getNumFrames();
    int frameSize = wavetableData_->getFrameSize();

    // Display samples as a matrix of cells
    int displaySamples = 64;
    int displayFrames = juce::jmin(numFrames, 32);

    float cellWidth = bounds.width() / displaySamples;
    float cellHeight = bounds.height() / displayFrames;

    for (int f = 0; f < displayFrames; ++f) {
        const auto& frame = wavetableData_->getFrame(f);

        for (int s = 0; s < displaySamples; ++s) {
            int sampleIndex = s * frameSize / displaySamples;
            float value = frame.samples[sampleIndex];

            // Color based on value
            float brightness = std::abs(value);
            SkColor color = value >= 0
                ? SkColorSetARGB(
                    static_cast<U8CPU>(brightness * 255),
                    static_cast<U8CPU>(brightness * 100),
                    static_cast<U8CPU>(brightness * 255),
                    static_cast<U8CPU>(brightness * 200)
                  )
                : SkColorSetARGB(
                    static_cast<U8CPU>(brightness * 255),
                    static_cast<U8CPU>(brightness * 255),
                    static_cast<U8CPU>(brightness * 100),
                    static_cast<U8CPU>(brightness * 200)
                  );

            SkPaint cellPaint;
            cellPaint.setColor(color);

            SkRect cellRect = SkRect::MakeXYWH(
                bounds.fLeft + s * cellWidth,
                bounds.fTop + f * cellHeight,
                cellWidth + 1,
                cellHeight + 1
            );

            canvas->drawRect(cellRect, cellPaint);
        }
    }
}

void SkiaVisualWavetableEditor::drawHarmonicView(SkCanvas* canvas, const SkRect& bounds) {
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    auto spectrum = wavetableData_->analyzeSpectrum(currentFrame_);

    SkPaint barPaint;
    barPaint.setAntiAlias(true);

    float barWidth = bounds.width() / spectrum.size();
    float maxBarHeight = bounds.height() * 0.8f;

    for (size_t i = 0; i < spectrum.size(); ++i) {
        float magnitude = spectrum[i];
        float barHeight = magnitude * maxBarHeight;

        SkColor barColor = getHarmonicColor(static_cast<int>(i));
        barPaint.setColor(SkColorSetARGB(
            static_cast<U8CPU>(150 + magnitude * 105),
            SkColorGetR(barColor),
            SkColorGetG(barColor),
            SkColorGetB(barColor)
        ));

        SkRect barRect = SkRect::MakeXYWH(
            bounds.fLeft + i * barWidth + 1,
            bounds.fBottom - barHeight,
            barWidth - 2,
            barHeight
        );

        canvas->drawRect(barRect, barPaint);
    }
}

void SkiaVisualWavetableEditor::drawGrid(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint gridPaint;
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setColor(SkColorSetARGB(20, 255, 255, 255));

    // Vertical lines
    for (int i = 0; i <= gridDivisions_; ++i) {
        float x = bounds.fLeft + (bounds.width() * i / gridDivisions_);
        canvas->drawLine(x, bounds.fTop, x, bounds.fBottom, gridPaint);
    }

    // Horizontal lines
    for (int i = 0; i <= 4; ++i) {
        float y = bounds.fTop + (bounds.height() * i / 4);
        canvas->drawLine(bounds.fLeft, y, bounds.fRight, y, gridPaint);
    }

    // Center line
    SkPaint centerPaint;
    centerPaint.setStyle(SkPaint::kStroke_Style);
    centerPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
    canvas->drawLine(bounds.fLeft, bounds.centerY(), bounds.fRight, bounds.centerY(), centerPaint);
}

void SkiaVisualWavetableEditor::drawRulers(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint rulerPaint;
    rulerPaint.setColor(SkColorSetARGB(150, 255, 255, 255));

    // Time/position markers
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    int frameSize = wavetableData_->getFrameSize();
    for (int i = 0; i <= 4; ++i) {
        float x = bounds.fLeft + (bounds.width() * i / 4);
        int samplePos = i * frameSize / 4;

        // Draw marker and label
        canvas->drawLine(x, bounds.fBottom - 5, x, bounds.fBottom, rulerPaint);
    }
}

void SkiaVisualWavetableEditor::drawCrosshair(SkCanvas* canvas, const SkRect& bounds) {
    if (hoveredSample_ < 0) return;

    SkPaint crosshairPaint;
    crosshairPaint.setStyle(SkPaint::kStroke_Style);
    crosshairPaint.setColor(SkColorSetARGB(100, 255, 255, 255));

    float x = bounds.fLeft + 20 + hoveredSample_ * (bounds.width() - 40) / 2048.0f * zoomLevel_ + scrollOffsetX_;
    float y = bounds.fTop;

    canvas->drawLine(x, y, x, bounds.fBottom, crosshairPaint);
}

void SkiaVisualWavetableEditor::drawFrameMarkers(SkCanvas* canvas, const SkRect& bounds) {
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    int numFrames = wavetableData_->getNumFrames();
    float markerWidth = bounds.width() / numFrames;

    SkPaint markerPaint;
    markerPaint.setStyle(SkPaint::kStroke_Style);
    markerPaint.setStrokeWidth(2.0f);

    for (int f = 0; f < numFrames; ++f) {
        if (f == currentFrame_) {
            markerPaint.setColor(SkColorSetRGB(0, 255, 200));
        } else {
            markerPaint.setColor(SkColorSetARGB(50, 100, 100));
        }

        float x = bounds.fLeft + f * markerWidth;
        canvas->drawLine(x, bounds.fBottom, x, bounds.fBottom - 10, markerPaint);
    }
}

void SkiaVisualWavetableEditor::drawSelection(SkCanvas* canvas, const SkRect& bounds) {
    // Draw selection rectangle
}

void SkiaVisualWavetableEditor::drawPlaybackIndicator(SkCanvas* canvas, const SkRect& bounds) {
    if (!isPlaying_) return;

    SkPaint playbackPaint;
    playbackPaint.setStyle(SkPaint::kStroke_Style);
    playbackPaint.setStrokeWidth(2.0f);
    playbackPaint.setColor(SkColorSetRGB(255, 100, 100));

    float x = bounds.fLeft + playbackPosition_ * bounds.width();
    canvas->drawLine(x, bounds.fTop, x, bounds.fBottom, playbackPaint);
}

void SkiaVisualWavetableEditor::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        if (editMode_ && editTool_ != EditTool::Select) {
            saveState();
            applyDrawTool(e.position.x, e.position.y,
                         SkRect::MakeXYWH(0, 0, getWidth(), getHeight()));
            isDragging_ = true;
            lastDrawPos_ = e.position;
        } else {
            // Select frame
            int frame = screenToFrame(e.position.x, bounds.toFloat());
            if (frame >= 0 && frame < wavetableData_->getNumFrames()) {
                currentFrame_ = frame;
                if (frameChangedCallback_) {
                    frameChangedCallback_(frame);
                }
            }
        }
    }
}

void SkiaVisualWavetableEditor::mouseDrag(const juce::MouseEvent& e) {
    if (isDragging_ && editMode_) {
        applyDrawTool(e.position.x, e.position.y,
                     SkRect::MakeXYWH(0, 0, getWidth(), getHeight()));
        lastDrawPos_ = e.position;
        markDirty();
    } else {
        // Update current frame based on X position
        int frame = screenToFrame(e.position.x, bounds.toFloat());
        if (frame >= 0 && frame < wavetableData_->getNumFrames()) {
            currentFrame_ = frame;
            if (frameChangedCallback_) {
                frameChangedCallback_(frame);
            }
        }
    }
}

void SkiaVisualWavetableEditor::mouseUp(const juce::MouseEvent& e) {
    if (isDragging_ && editMode_) {
        isDragging_ = false;
        if (dataModifiedCallback_) {
            dataModifiedCallback_();
        }
    }
}

void SkiaVisualWavetableEditor::mouseMove(const juce::MouseEvent& e) {
    hoveredSample_ = screenToSample(e.position.x, bounds.toFloat());

    if (editMode_ || isPlaying_) {
        markDirty();
    }
}

void SkiaVisualWavetableEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    // Zoom with scroll wheel
    float delta = wheel.deltaY;

    if (e.mods.isShiftDown()) {
        // Scroll horizontally
        scrollOffsetX_ -= delta * 50.0f;
    } else {
        // Zoom
        zoomLevel_ = juce::jlimit(0.1f, 10.0f, zoomLevel_ - delta * 0.1f);
    }

    markDirty();
}

bool SkiaVisualWavetableEditor::keyPressed(const juce::KeyPress& key) {
    if (key.getTextCharacter() == 'z' && key.getModifiers().isCommandDown()) {
        undo();
        return true;
    } else if (key.getTextCharacter() == 'z' && key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown()) {
        redo();
        return true;
    } else if (key.getTextCharacter() == 'a' && key.getModifiers().isCommandDown()) {
        selectAll();
        return true;
    } else if (key.getKeyCode() == juce::KeyPress::deleteKey) {
        // Clear selected samples
        return true;
    }

    return false;
}

void SkiaVisualWavetableEditor::undo() {
    if (canUndo()) {
        restoreState();
        historyPosition_--;
        markDirty();
    }
}

void SkiaVisualWavetableEditor::redo() {
    if (canRedo()) {
        historyPosition_++;
        restoreState();
        markDirty();
    }
}

bool SkiaVisualWavetableEditor::canUndo() const {
    return historyPosition_ > 0;
}

bool SkiaVisualWavetableEditor::canRedo() const {
    return historyPosition_ < undoHistory_.size();
}

void SkiaVisualWavetableEditor::selectAll() {
    std::fill(sampleSelection_.begin(), sampleSelection_.end(), true);
    markDirty();
}

void SkiaVisualWavetableEditor::selectNone() {
    std::fill(sampleSelection_.begin(), sampleSelection_.end(), false);
    markDirty();
}

void SkiaVisualWavetableEditor::invertSelection() {
    for (auto& selected : sampleSelection_) {
        selected = !selected;
    }
    markDirty();
}

void SkiaVisualWavetableEditor::applyDrawTool(float x, float y, const SkRect& bounds) {
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    auto& frame = wavetableData_->getFrame(currentFrame_);
    int sampleIndex = screenToSample(x, bounds);

    if (sampleIndex < 0 || sampleIndex >= frame.size()) return;

    float value = screenToValue(y, bounds);

    switch (editTool_) {
        case EditTool::Draw:
            frame.samples[sampleIndex] = value;
            break;

        case EditTool::Line:
            if (!isDragging_) {
                dragStartPos_ = juce::Point<float>(x, y);
            } else {
                // Interpolate between start and current
                int startIndex = screenToSample(dragStartPos_.x, bounds);
                int endIndex = sampleIndex;
                float startValue = screenToValue(dragStartPos_.y, bounds);

                for (int i = juce::jmin(startIndex, endIndex);
                     i <= juce::jmax(startIndex, endIndex); ++i) {
                    if (i >= 0 && i < frame.size()) {
                        float t = (startIndex == endIndex) ? 0.0f :
                                  static_cast<float>(i - startIndex) / (endIndex - startIndex);
                        frame.samples[i] = startValue + t * (value - startValue);
                    }
                }
            }
            break;

        case EditTool::Smooth: {
            int radius = 10;
            for (int i = -radius; i <= radius; ++i) {
                int idx = sampleIndex + i;
                if (idx >= 0 && idx < frame.size()) {
                    float weight = 1.0f - std::abs(i) / static_cast<float>(radius);
                    frame.samples[idx] = frame.samples[idx] * (1.0f - weight * 0.5f) + value * weight * 0.5f;
                }
            }
            break;
        }

        case EditTool::Sharpen: {
            int radius = 5;
            for (int i = -radius; i <= radius; ++i) {
                int idx = sampleIndex + i;
                if (idx >= 0 && idx < frame.size()) {
                    float weight = 1.0f - std::abs(i) / static_cast<float>(radius);
                    frame.samples[idx] = juce::jlimit(-1.0f, 1.0f,
                        frame.samples[idx] * (1.0f + weight * 0.3f));
                }
            }
            break;
        }

        case EditTool::Noise: {
            int radius = 20;
            juce::Random random;
            for (int i = -radius; i <= radius; ++i) {
                int idx = sampleIndex + i;
                if (idx >= 0 && idx < frame.size()) {
                    float noiseAmount = 1.0f - std::abs(i) / static_cast<float>(radius);
                    frame.samples[idx] += random.nextFloat() * 0.2f * noiseAmount;
                    frame.samples[idx] = juce::jlimit(-1.0f, 1.0f, frame.samples[idx]);
                }
            }
            break;
        }

        default:
            break;
    }

    frame.edited = true;
}

SkPoint SkiaVisualWavetableEditor::sampleToScreen(int sampleIndex, float value, const SkRect& bounds) {
    float x = bounds.fLeft + 20 + sampleIndex * (bounds.width() - 40) / 2048.0f * zoomLevel_ + scrollOffsetX_;
    float y = bounds.centerY() - value * bounds.height() * 0.4f;
    return SkPoint::Make(x, y);
}

int SkiaVisualWavetableEditor::screenToSample(float x, const SkRect& bounds) {
    float effectiveWidth = bounds.width() - 40;
    float relativeX = x - bounds.fLeft - 20 - scrollOffsetX_;
    int sample = static_cast<int>((relativeX / effectiveWidth / zoomLevel_) * 2048.0f);
    return juce::jlimit(0, 2047, sample);
}

float SkiaVisualWavetableEditor::screenToValue(float y, const SkRect& bounds) {
    float relativeY = bounds.centerY() - y;
    float value = relativeY / (bounds.height() * 0.4f);
    return juce::jlimit(-1.0f, 1.0f, value);
}

int SkiaVisualWavetableEditor::screenToFrame(float x, const SkRect& bounds) {
    if (!wavetableData_) return -1;
    int numFrames = wavetableData_->getNumFrames();
    float frameWidth = bounds.width() / numFrames;
    int frame = static_cast<int>((x - bounds.fLeft) / frameWidth);
    return juce::jlimit(0, numFrames - 1, frame);
}

void SkiaVisualWavetableEditor::saveState() {
    if (!wavetableData_ || wavetableData_->isEmpty()) return;

    HistoryEntry entry;
    entry.frameIndex = currentFrame_;
    entry.previousSamples = wavetableData_->getFrame(currentFrame_).samples;

    // Truncate redo history
    if (historyPosition_ < undoHistory_.size()) {
        undoHistory_.resize(historyPosition_);
    }

    undoHistory_.push_back(entry);
    historyPosition_ = undoHistory_.size();

    // Limit history size
    if (undoHistory_.size() > 50) {
        undoHistory_.erase(undoHistory_.begin());
        historyPosition_--;
    }
}

void SkiaVisualWavetableEditor::restoreState() {
    if (historyPosition_ == 0 || historyPosition_ > undoHistory_.size()) return;

    const auto& entry = undoHistory_[historyPosition_ - 1];
    auto& frame = wavetableData_->getFrame(entry.frameIndex);
    frame.samples = entry.previousSamples;
}

SkColor SkiaVisualWavetableEditor::getFrameColor(int frameIndex) const {
    // Color gradient across frames
    float t = frameIndex / static_cast<float>(std::max(1, wavetableData_->getNumFrames() - 1));
    return SkColorSetRGB(
        static_cast<U8CPU>(t * 100),
        static_cast<U8CPU>(200 - t * 50),
        static_cast<U8CPU>(255)
    );
}

SkColor SkiaVisualWavetableEditor::getHarmonicColor(int harmonicIndex) const {
    // Color based on harmonic index
    static const SkColor colors[] = {
        SkColorSetRGB(255, 100, 100),
        SkColorSetRGB(100, 255, 100),
        SkColorSetRGB(100, 100, 255),
        SkColorSetRGB(255, 255, 100),
        SkColorSetRGB(255, 100, 255),
        SkColorSetRGB(100, 255, 255),
    };
    return colors[harmonicIndex % 6];
}

void SkiaVisualWavetableEditor::timerCallback() {
    if (isPlaying_) {
        playbackPosition_ += 0.01f;
        if (playbackPosition_ >= 1.0f) {
            playbackPosition_ = 0.0f;
        }
        markDirty();
    }
}

//==============================================================================
// WavetableGenerator Implementation
//==============================================================================

std::vector<float> WavetableGenerator::generateSine(int size) {
    std::vector<float> samples(size);
    for (int i = 0; i < size; ++i) {
        float phase = static_cast<float>(i) / size;
        samples[i] = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    }
    return samples;
}

std::vector<float> WavetableGenerator::generateSaw(int size, int harmonics) {
    std::vector<float> samples(size, 0.0f);

    for (int h = 1; h <= harmonics; ++h) {
        float amplitude = 1.0f / h;
        for (int i = 0; i < size; ++i) {
            float phase = static_cast<float>(i) / size;
            samples[i] += amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * h * phase);
        }
    }

    return samples;
}

std::vector<float> WavetableGenerator::generateSquare(int size, int harmonics) {
    std::vector<float> samples(size, 0.0f);

    for (int h = 1; h <= harmonics; h += 2) {
        float amplitude = 1.0f / h;
        for (int i = 0; i < size; ++i) {
            float phase = static_cast<float>(i) / size;
            samples[i] += amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * h * phase);
        }
    }

    return samples;
}

std::vector<float> WavetableGenerator::generateTriangle(int size) {
    std::vector<float> samples(size);

    for (int i = 0; i < size; ++i) {
        float phase = static_cast<float>(i) / size;
        if (phase < 0.25f) {
            samples[i] = 4.0f * phase;
        } else if (phase < 0.75f) {
            samples[i] = 2.0f - 4.0f * phase;
        } else {
            samples[i] = -4.0f + 4.0f * phase;
        }
    }

    return samples;
}

std::vector<float> WavetableGenerator::generatePulse(int size, float pulseWidth) {
    std::vector<float> samples(size);
    float pw = juce::jlimit(0.01f, 0.99f, pulseWidth);

    for (int i = 0; i < size; ++i) {
        float phase = static_cast<float>(i) / size;
        samples[i] = (phase < pw) ? 1.0f : -1.0f;
    }

    return samples;
}

std::vector<float> WavetableGenerator::morphFrames(const std::vector<float>& frameA,
                                                   const std::vector<float>& frameB,
                                                   float amount) {
    std::vector<float> result(frameA.size());

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = (1.0f - amount) * frameA[i] + amount * frameB[i];
    }

    return result;
}

void WavetableGenerator::applyWindow(std::vector<float>& samples, const char* windowType) {
    int size = static_cast<int>(samples.size());

    if (strcmp(windowType, "hann") == 0) {
        for (int i = 0; i < size; ++i) {
            float phase = static_cast<float>(i) / (size - 1);
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * phase));
            samples[i] *= window;
        }
    } else if (strcmp(windowType, "hamming") == 0) {
        for (int i = 0; i < size; ++i) {
            float phase = static_cast<float>(i) / (size - 1);
            float window = 0.54f - 0.46f * std::cos(2.0f * juce::MathConstants<float>::pi * phase);
            samples[i] *= window;
        }
    } else if (strcmp(windowType, "blackman") == 0) {
        for (int i = 0; i < size; ++i) {
            float phase = static_cast<float>(i) / (size - 1);
            float window = 0.42f
                        - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * phase)
                        + 0.08f * std::cos(4.0f * juce::MathConstants<float>::pi * phase);
            samples[i] *= window;
        }
    }
}

std::vector<float> WavetableGenerator::getHarmonics(const std::vector<float>& samples, int numHarmonics) {
    std::vector<float> harmonics(numHarmonics, 0.0f);
    int size = static_cast<int>(samples.size());

    for (int h = 0; h < numHarmonics; ++h) {
        float real = 0.0f;
        float imag = 0.0f;

        for (int i = 0; i < size; ++i) {
            float phase = 2.0f * juce::MathConstants<float>::pi * (h + 1) * i / size;
            real += samples[i] * std::cos(phase);
            imag -= samples[i] * std::sin(phase);
        }

        harmonics[h] = std::sqrt(real * real + imag * imag) / size;
    }

    return harmonics;
}

float WavetableGenerator::estimateFundamental(const std::vector<float>& samples) {
    int crossings = 0;

    for (size_t i = 1; i < samples.size(); ++i) {
        if ((samples[i - 1] >= 0 && samples[i] < 0) ||
            (samples[i - 1] < 0 && samples[i] >= 0)) {
            crossings++;
        }
    }

    if (crossings == 0) return 0.0f;

    return static_cast<float>(samples.size()) / (crossings * 0.5f);
}

//==============================================================================
// WavetablePresets Implementation
//==============================================================================

std::shared_ptr<WavetableData> WavetablePresets::createBasicWaves() {
    auto data = std::make_shared<WavetableData>();
    data->resize(5, 2048);

    data->getFrame(0).samples = WavetableGenerator::generateSine(2048);
    data->getFrame(1).samples = WavetableGenerator::generateTriangle(2048);
    data->getFrame(2).samples = WavetableGenerator::generateSaw(2048, 16);
    data->getFrame(3).samples = WavetableGenerator::generateSquare(2048, 16);
    data->getFrame(4).samples = WavetableGenerator::generatePulse(2048, 0.25f);

    return data;
}

std::shared_ptr<WavetableData> WavetablePresets::createMorphingSaw() {
    auto data = std::make_shared<WavetableData>();
    data->resize(16, 2048);

    for (int f = 0; f < 16; ++f) {
        float t = static_cast<float>(f) / 15.0f;
        auto saw = WavetableGenerator::generateSaw(2048, 32);
        auto sine = WavetableGenerator::generateSine(2048);

        for (int i = 0; i < 2048; ++i) {
            saw[i] = (1.0f - t) * sine[i] + t * saw[i];
        }

        data->getFrame(f).samples = saw;
    }

    return data;
}

std::shared_ptr<WavetableData> WavetablePresets::createMorphingSquare() {
    auto data = std::make_shared<WavetableData>();
    data->resize(16, 2048);

    for (int f = 0; f < 16; ++f) {
        float t = static_cast<float>(f) / 15.0f;
        float pw = 0.5f + t * 0.4f;
        data->getFrame(f).samples = WavetableGenerator::generatePulse(2048, pw);
    }

    return data;
}

juce::StringArray WavetablePresets::getPresetNames() {
    return {
        "Basic Waves",
        "Morphing Saw",
        "Morphing Square",
        "Formant Filter 1",
        "Formant Filter 2",
        "Vocal Eee",
        "Vocal Ahh",
        "Vocal Ooo"
    };
}

std::shared_ptr<WavetableData> WavetablePresets::loadPreset(const juce::String& name) {
    if (name == "Basic Waves") return createBasicWaves();
    if (name == "Morphing Saw") return createMorphingSaw();
    if (name == "Morphing Square") return createMorphingSquare();

    // Return basic waves as default
    return createBasicWaves();
}

} // namespace zenith
