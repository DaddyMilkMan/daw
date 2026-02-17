/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "SkiaSynthUI.h"
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// SkiaRotaryKnob Implementation
//==============================================================================

SkiaRotaryKnob::SkiaRotaryKnob() {
    updateAngle();
    startTimerHz(60);  // For smooth animation
}

void SkiaRotaryKnob::setValue(float value) {
    float clamped = juce::jlimit(minValue_, maxValue_, value);
    if (std::abs(value_ - clamped) > 0.0001f) {
        value_ = clamped;
        updateAngle();
        markDirty();

        if (parameter_) {
            parameter_->currentValue = value_;
        }

        if (valueChangedCallback_) {
            valueChangedCallback_(value_);
        }
    }
}

void SkiaRotaryKnob::setRange(float min, float max) {
    minValue_ = min;
    maxValue_ = max;
    setValue(juce::jlimit(min, max, value_));
}

void SkiaRotaryKnob::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Draw background track
    drawTrack(canvas, bounds);

    // Draw filled portion based on value
    float normalizedValue = (value_ - minValue_) / (maxValue_ - minValue_);
    drawFill(canvas, bounds, normalizedValue);

    // Draw indicator at current angle
    drawIndicator(canvas, bounds, currentAngle_);

    // Draw value text
    if (showValue_) {
        drawValueText(canvas, bounds);
    }

    // Draw label
    drawLabel(canvas, bounds);

    // Draw glow if enabled
    if (isGlowEnabled() && (isHovered() || isDragging_)) {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setColor(getGlowColor());
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(
            kNormal_SkBlurStyle, getGlowRadius() * 0.5f));
        canvas->drawCircle(bounds.centerX(), bounds.centerY(),
                          bounds.width() * 0.35f, glowPaint);
    }
}

std::vector<SkiaComponent::AIElementInfo> SkiaRotaryKnob::getInspectableElements() {
    AIElementInfo info;
    info.bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());
    info.type = "rotary_knob";
    info.id = parameter_ ? parameter_->id : "unknown";
    info.label = text_;
    info.parameterId = parameter_ ? parameter_->id : "";
    info.currentValue = value_;
    return {info};
}

void SkiaRotaryKnob::updateAngle() {
    float normalizedValue = (value_ - minValue_) / (maxValue_ - minValue_);
    targetAngle_ = arcStartAngle_ + normalizedValue * (arcEndAngle_ - arcStartAngle_);

    // Smooth animation
    float diff = targetAngle_ - currentAngle_;
    currentAngle_ += diff * 0.3f;
}

void SkiaRotaryKnob::drawTrack(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint trackPaint;
    trackPaint.setAntiAlias(true);
    trackPaint.setStyle(SkPaint::kStroke_Style);
    trackPaint.setStrokeWidth(trackWidth_);
    trackPaint.setColor(SkColorSetARGB(50, 255, 255, 255));

    SkRect trackRect = bounds;
    float inset = trackWidth_ * 2.0f + 2.0f;
    trackRect.inset(inset, inset);

    SkPath trackPath;
    trackPath.addArc(trackRect, arcStartAngle_ * 180.0f / juce::MathConstants<float>::pi,
                    (arcEndAngle_ - arcStartAngle_) * 180.0f / juce::MathConstants<float>::pi);
    canvas->drawPath(trackPath, trackPaint);
}

void SkiaRotaryKnob::drawFill(SkCanvas* canvas, const SkRect& bounds, float fillAmount) {
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setStyle(SkPaint::kStroke_Style);
    fillPaint.setStrokeWidth(trackWidth_);
    fillPaint.setColor(getColorForValue(fillAmount));

    // Create gradient
    SkColor colors[2] = {
        SkColorSetRGB(0, 200, 255),
        SkColorSetRGB(255, 100, 200)
    };
    SkPoint points[2] = {
        {bounds.fLeft, bounds.fBottom},
        {bounds.fRight, bounds.fTop}
    };
    fillPaint.setShader(SkGradientShader::MakeLinear(
        points, colors, nullptr, 2, SkTileMode::kClamp));

    SkRect fillRect = bounds;
    float inset = trackWidth_ * 2.0f + 2.0f;
    fillRect.inset(inset, inset);

    float startAngle = arcStartAngle_ * 180.0f / juce::MathConstants<float>::pi;
    float sweepAngle = (arcEndAngle_ - arcStartAngle_) * fillAmount * 180.0f / juce::MathConstants<float>::pi;

    SkPath fillPath;
    fillPath.addArc(fillRect, startAngle, sweepAngle);
    canvas->drawPath(fillPath, fillPaint);
}

void SkiaRotaryKnob::drawIndicator(SkCanvas* canvas, const SkRect& bounds, float angle) {
    float radius = bounds.width() * 0.35f;
    float cx = bounds.centerX();
    float cy = bounds.centerY();

    float indicatorX = cx + std::cos(angle - juce::MathConstants<float>::halfPi) * radius;
    float indicatorY = cy + std::sin(angle - juce::MathConstants<float>::halfPi) * radius;

    SkPaint indicatorPaint;
    indicatorPaint.setAntiAlias(true);
    indicatorPaint.setColor(SkColorSetRGB(255, 255, 255));
    indicatorPaint.setMaskFilter(SkMaskFilter::MakeBlur(
        kNormal_SkBlurStyle, 2.0f));

    canvas->drawCircle(indicatorX, indicatorY, indicatorSize_ * 0.5f, indicatorPaint);
}

void SkiaRotaryKnob::drawValueText(SkCanvas* canvas, const SkRect& bounds) {
    juce::String valueText;
    if (parameter_) {
        valueText = juce::String(value_, parameter_->decimalPlaces);
        if (parameter_->unit.isNotEmpty()) {
            valueText << " " << parameter_->unit;
        }
    } else {
        valueText = juce::String(value_, 2);
    }

    // Draw centered text below knob
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(200, 255, 255, 255));

    // Note: In production, you'd use proper font loading
    // For now, we'll create a simple representation
    float textY = bounds.fBottom - bounds.height() * 0.15f;

    SkRect textBounds;
    // textPaint.measureText(valueText.getCharPointer(), valueText.length(), &textBounds);

    // Draw text (simplified - use JUCE font in production)
    // canvas->drawText(valueText.toUTF8(), valueText.length(),
    //                 bounds.centerX() - textBounds.width() * 0.5f, textY, textPaint);
}

void SkiaRotaryKnob::drawLabel(SkCanvas* canvas, const SkRect& bounds) {
    if (text_.isEmpty()) return;

    SkPaint labelPaint;
    labelPaint.setAntiAlias(true);
    labelPaint.setColor(SkColorSetARGB(150, 200, 200, 200));

    // Draw label at bottom
    // canvas->drawText(text_.toUTF8(), text_.length(),
    //                 bounds.centerX(), bounds.fBottom - 5, labelPaint);
}

SkColor SkiaRotaryKnob::getColorForValue(float value) const {
    // Color gradient from blue (low) to purple (mid) to red (high)
    if (value < 0.5f) {
        float t = value * 2.0f;
        return SkColorSetRGB(
            static_cast<U8CPU>(0),
            static_cast<U8CPU>(150 + t * 105),
            static_cast<U8CPU>(255 - t * 55)
        );
    } else {
        float t = (value - 0.5f) * 2.0f;
        return SkColorSetRGB(
            static_cast<U8CPU>(t * 255),
            static_cast<U8CPU>(255 - t * 155),
            static_cast<U8CPU>(200 - t * 200)
        );
    }
}

void SkiaRotaryKnob::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        isDragging_ = true;
        lastDragY_ = e.position.y;
        dragStartValue_ = value_;

        if (dragStartedCallback_) {
            dragStartedCallback_();
        }
    }
}

void SkiaRotaryKnob::mouseDrag(const juce::MouseEvent& e) {
    if (isDragging_) {
        float deltaY = lastDragY_ - e.position.y;
        float range = maxValue_ - minValue_;
        float sensitivity = e.mods.isShiftDown() ? 0.1f : 0.5f;

        float newValue = dragStartValue_ + deltaY * range * sensitivity * 0.01f;
        setValue(newValue);

        lastDragY_ = e.position.y;
    }
}

void SkiaRotaryKnob::mouseUp(const juce::MouseEvent& e) {
    if (isDragging_) {
        isDragging_ = false;

        if (dragEndedCallback_) {
            dragEndedCallback_();
        }
    }
}

void SkiaRotaryKnob::mouseDoubleClick(const juce::MouseEvent& e) {
    setValue(defaultValue_);
}

void SkiaRotaryKnob::onMouseDown(const juce::MouseEvent& e) {
    // Hook for subclasses
}

void SkiaRotaryKnob::onMouseDrag(const juce::MouseEvent& e) {
    // Hook for subclasses
}

void SkiaRotaryKnob::onMouseUp(const juce::MouseEvent& e) {
    // Hook for subclasses
}

void SkiaRotaryKnob::timerCallback() {
    // Update animation
    if (std::abs(currentAngle_ - targetAngle_) > 0.001f) {
        updateAngle();
        markDirty();
    }
}

//==============================================================================
// SkiaLinearFader Implementation
//==============================================================================

SkiaLinearFader::SkiaLinearFader() {
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void SkiaLinearFader::setValue(float value) {
    float clamped = juce::jlimit(minValue_, maxValue_, value);
    if (std::abs(value_ - clamped) > 0.0001f) {
        value_ = clamped;
        markDirty();

        if (parameter_) {
            parameter_->currentValue = value_;
        }

        if (valueChangedCallback_) {
            valueChangedCallback_(value_);
        }
    }
}

void SkiaLinearFader::setRange(float min, float max) {
    minValue_ = min;
    maxValue_ = max;
    setValue(juce::jlimit(min, max, value_));
}

void SkiaLinearFader::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    drawTrack(canvas, bounds);
    drawFill(canvas, bounds);

    float thumbPos = getThumbPosition();
    drawThumb(canvas, bounds, thumbPos);

    if (showValue_) {
        drawValueText(canvas, bounds);
    }

    drawTickMarks(canvas, bounds);
}

void SkiaLinearFader::drawTrack(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint trackPaint;
    trackPaint.setAntiAlias(true);
    trackPaint.setStyle(SkPaint::kStroke_Style);
    trackPaint.setStrokeWidth(trackThickness_);
    trackPaint.setColor(SkColorSetARGB(50, 255, 255, 255));

    if (orientation_ == Orientation::Vertical) {
        float x = bounds.centerX();
        canvas->drawLine(x, bounds.fTop + 5, x, bounds.fBottom - 25, trackPaint);
    } else {
        float y = bounds.centerY();
        canvas->drawLine(bounds.fLeft + 5, y, bounds.fRight - 5, y, trackPaint);
    }
}

void SkiaLinearFader::drawFill(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setStyle(SkPaint::kStroke_Style);
    fillPaint.setStrokeWidth(trackThickness_);
    fillPaint.setColor(SkColorSetRGB(0, 200, 255));

    float normalizedValue = (value_ - minValue_) / (maxValue_ - minValue_);

    if (orientation_ == Orientation::Vertical) {
        float x = bounds.centerX();
        float topY = reverse_ ? bounds.fTop + 5 : bounds.fBottom - 25;
        float bottomY = reverse_ ? bounds.fBottom - 25 : bounds.fTop + 5;
        float fillHeight = (bounds.height() - 30) * normalizedValue;

        if (reverse_) {
            canvas->drawLine(x, topY, x, topY + fillHeight, fillPaint);
        } else {
            canvas->drawLine(x, bottomY - fillHeight, x, bottomY, fillPaint);
        }
    } else {
        float y = bounds.centerY();
        float leftX = reverse_ ? bounds.fLeft + 5 : bounds.fRight - 5;
        float fillWidth = (bounds.width() - 10) * normalizedValue;

        if (reverse_) {
            canvas->drawLine(leftX, y, leftX + fillWidth, y, fillPaint);
        } else {
            canvas->drawLine(leftX - fillWidth, y, leftX, y, fillPaint);
        }
    }
}

void SkiaLinearFader::drawThumb(SkCanvas* canvas, const SkRect& bounds, float thumbPos) {
    SkPaint thumbPaint;
    thumbPaint.setAntiAlias(true);
    thumbPaint.setColor(SkColorSetRGB(255, 255, 255));

    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
    shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));

    float thumbSize = thumbSize_;

    if (orientation_ == Orientation::Vertical) {
        float x = bounds.centerX();
        canvas->drawCircle(x, thumbPos, thumbSize * 0.5f + 2, shadowPaint);
        canvas->drawCircle(x, thumbPos, thumbSize * 0.5f, thumbPaint);
    } else {
        float y = bounds.centerY();
        canvas->drawCircle(thumbPos, y, thumbSize * 0.5f + 2, shadowPaint);
        canvas->drawCircle(thumbPos, y, thumbSize * 0.5f, thumbPaint);
    }
}

void SkiaLinearFader::drawValueText(SkCanvas* canvas, const SkRect& bounds) {
    juce::String valueText;
    if (parameter_) {
        valueText = juce::String(value_, parameter_->decimalPlaces);
    } else {
        valueText = juce::String(value_, 2);
    }

    // Draw text (simplified)
    // In production, use proper font rendering
}

void SkiaLinearFader::drawTickMarks(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint tickPaint;
    tickPaint.setAntiAlias(true);
    tickPaint.setColor(SkColorSetARGB(100, 255, 255, 255));

    int numTicks = 5;
    float tickSize = 4.0f;

    if (orientation_ == Orientation::Vertical) {
        float x = bounds.centerX() + trackThickness_ + 4;
        float topY = bounds.fTop + 5;
        float bottomY = bounds.fBottom - 25;
        float height = bottomY - topY;

        for (int i = 0; i <= numTicks; ++i) {
            float y = topY + (height * i / numTicks);
            canvas->drawLine(x, y, x + tickSize, y, tickPaint);
        }
    } else {
        float y = bounds.centerY() + trackThickness_ + 4;
        float leftX = bounds.fLeft + 5;
        float rightX = bounds.fRight - 5;
        float width = rightX - leftX;

        for (int i = 0; i <= numTicks; ++i) {
            float x = leftX + (width * i / numTicks);
            canvas->drawLine(x, y, x, y + tickSize, tickPaint);
        }
    }
}

float SkiaLinearFader::getThumbPosition() const {
    float normalizedValue = (value_ - minValue_) / (maxValue_ - minValue_);

    if (orientation_ == Orientation::Vertical) {
        float topY = 5.0f;
        float bottomY = getHeight() - 25.0f;
        float range = bottomY - topY;

        if (reverse_) {
            return topY + range * normalizedValue;
        } else {
            return bottomY - range * normalizedValue;
        }
    } else {
        float leftX = 5.0f;
        float rightX = getWidth() - 5.0f;
        float range = rightX - leftX;

        if (reverse_) {
            return leftX + range * normalizedValue;
        } else {
            return rightX - range * normalizedValue;
        }
    }
}

void SkiaLinearFader::mouseDown(const juce::MouseEvent& e) {
    isDragging_ = true;
    dragStartPos_ = orientation_ == Orientation::Vertical ? e.position.y : e.position.x;
    dragStartValue_ = value_;
}

void SkiaLinearFader::mouseDrag(const juce::MouseEvent& e) {
    if (isDragging_) {
        float currentPos = orientation_ == Orientation::Vertical ? e.position.y : e.position.x;
        float delta = dragStartPos_ - currentPos;

        float trackLength = orientation_ == Orientation::Vertical
            ? getHeight() - 30.0f
            : getWidth() - 10.0f;

        float range = maxValue_ - minValue_;
        float sensitivity = e.mods.isShiftDown() ? 0.25f : 1.0f;

        float newValue = dragStartValue_ + (delta / trackLength) * range * sensitivity;
        setValue(newValue);
    }
}

void SkiaLinearFader::mouseUp(const juce::MouseEvent& e) {
    isDragging_ = false;
}

void SkiaLinearFader::mouseDoubleClick(const juce::MouseEvent& e) {
    setValue(defaultValue_);
}

//==============================================================================
// SkiaWaveformDisplay Implementation
//==============================================================================

SkiaWaveformDisplay::SkiaWaveformDisplay() {
    startTimerHz(30);  // Animation at 30fps
}

void SkiaWaveformDisplay::setWavetableData(const float* samples, int numSamples) {
    wavetableSamples_.assign(samples, samples + numSamples);
    waveformType_ = WaveformType::Wavetable;
    markDirty();
}

void SkiaWaveformDisplay::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Draw background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(30, 20, 20, 30));
    canvas->drawRect(bounds, bgPaint);

    // Draw grid
    SkPaint gridPaint;
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setColor(SkColorSetARGB(30, 255, 255, 255));

    // Horizontal center line
    canvas->drawLine(bounds.fLeft, bounds.centerY(), bounds.fRight, bounds.centerY(), gridPaint);

    // Draw waveform based on type
    switch (waveformType_) {
        case WaveformType::Sine:
            drawSineWave(canvas, bounds);
            break;
        case WaveformType::Saw:
            drawSawWave(canvas, bounds);
            break;
        case WaveformType::Square:
            drawSquareWave(canvas, bounds);
            break;
        case WaveformType::Triangle:
            drawTriangleWave(canvas, bounds);
            break;
        case WaveformType::Wavetable:
            drawWavetable(canvas, bounds);
            break;
        default:
            drawSineWave(canvas, bounds);
            break;
    }

    // Draw glow effect
    if (glowIntensity_ > 0) {
        // Apply glow pass
    }
}

void SkiaWaveformDisplay::drawSineWave(SkCanvas* canvas, const SkRect& bounds) {
    drawWave(canvas, bounds, [this](float t) {
        float phase = t * juce::MathConstants<float>::twoPi + animationPhase_;
        return std::sin(phase);
    });
}

void SkiaWaveformDisplay::drawSawWave(SkCanvas* canvas, const SkRect& bounds) {
    drawWave(canvas, bounds, [this](float t) {
        float phase = t + animationPhase_ / juce::MathConstants<float>::twoPi;
        if (phase > 1.0f) phase -= 1.0f;
        return 2.0f * phase - 1.0f;
    });
}

void SkiaWaveformDisplay::drawSquareWave(SkCanvas* canvas, const SkRect& bounds) {
    float pw = juce::jlimit(0.01f, 0.99f, pulseWidth_);

    drawWave(canvas, bounds, [this, pw](float t) {
        float phase = t + animationPhase_ / juce::MathConstants<float>::twoPi;
        if (phase > 1.0f) phase -= 1.0f;
        return (phase < pw) ? 1.0f : -1.0f;
    });
}

void SkiaWaveformDisplay::drawTriangleWave(SkCanvas* canvas, const SkRect& bounds) {
    drawWave(canvas, bounds, [this](float t) {
        float phase = t + animationPhase_ / juce::MathConstants<float>::twoPi;
        if (phase > 1.0f) phase -= 1.0f;
        return 2.0f * std::abs(2.0f * (phase - 0.5f)) - 1.0f;
    });
}

void SkiaWaveformDisplay::drawWavetable(SkCanvas* canvas, const SkRect& bounds) {
    if (wavetableSamples_.empty()) {
        drawSineWave(canvas, bounds);
        return;
    }

    SkPaint wavePaint;
    wavePaint.setAntiAlias(true);
    wavePaint.setStyle(SkPaint::kStroke_Style);
    wavePaint.setStrokeWidth(lineWidth_);
    wavePaint.setColor(gradientStart_);

    SkPath wavePath;
    float centerY = bounds.centerY();
    float scaleX = bounds.width() / wavetableSamples_.size();
    float scaleY = bounds.height() * 0.45f;

    wavePath.moveTo(0, centerY);

    for (size_t i = 0; i < wavetableSamples_.size(); ++i) {
        float x = i * scaleX;
        float y = centerY - wavetableSamples_[i] * scaleY;
        wavePath.lineTo(x, y);
    }

    canvas->drawPath(wavePath, wavePaint);
}

void SkiaWaveformDisplay::drawWave(SkCanvas* canvas, const SkRect& bounds,
                                  std::function<float(float)> waveFunc) {
    SkPaint wavePaint;
    wavePaint.setAntiAlias(true);
    wavePaint.setStyle(SkPaint::kStroke_Style);
    wavePaint.setStrokeWidth(lineWidth_);
    wavePaint.setColor(gradientStart_);

    if (glowIntensity_ > 0) {
        wavePaint.setMaskFilter(SkMaskFilter::MakeBlur(
            kNormal_SkBlurStyle, glowIntensity_ * 5.0f));
    }

    SkPath wavePath;
    float centerY = bounds.centerY();
    float amplitude = bounds.height() * 0.4f;

    const int resolution = 200;
    for (int i = 0; i <= resolution; ++i) {
        float t = static_cast<float>(i) / resolution;
        float x = bounds.fLeft + t * bounds.width();
        float sample = waveFunc(t);
        float y = centerY - sample * amplitude;

        if (i == 0) {
            wavePath.moveTo(x, y);
        } else {
            wavePath.lineTo(x, y);
        }
    }

    canvas->drawPath(wavePath, wavePaint);
}

void SkiaWaveformDisplay::timerCallback() {
    if (animate_) {
        animationPhase_ += 0.05f * animationSpeed_;
        if (animationPhase_ >= juce::MathConstants<float>::twoPi) {
            animationPhase_ -= juce::MathConstants<float>::twoPi;
        }
        markDirty();
    }
}

//==============================================================================
// SkiaEnvelopeEditor Implementation
//==============================================================================

SkiaEnvelopeEditor::SkiaEnvelopeEditor() {
    initializePoints();
}

void SkiaEnvelopeEditor::initializePoints() {
    points_.clear();
    // ADSR envelope points
    points_.push_back({0.0f, 0.0f, false, 0.0f, true});        // Start (fixed)
    points_.push_back({0.0f, 1.0f, true, 0.5f, false});        // Attack peak
    points_.push_back({0.0f, 0.5f, true, 0.5f, false});       // Decay/sustain
    points_.push_back({0.0f, 0.5f, false, 0.0f, true});        // Sustain end
    points_.push_back({0.0f, 0.0f, false, 0.0f, true});        // Release end (fixed)

    updatePointsFromParameters();
}

void SkiaEnvelopeEditor::updatePointsFromParameters() {
    if (points_.size() < 5) return;

    float maxTime = maxTimeSeconds_;

    points_[1].x = attackTime_ / maxTime;
    points_[1].y = 1.0f;

    points_[2].x = (attackTime_ + decayTime_) / maxTime;
    points_[2].y = sustainLevel_;

    points_[3].x = (attackTime_ + decayTime_) / maxTime;
    points_[3].y = sustainLevel_;

    points_[4].x = (attackTime_ + decayTime_ + releaseTime_) / maxTime;
    points_[4].y = 0.0f;
}

void SkiaEnvelopeEditor::updateParametersFromPoints() {
    float maxTime = maxTimeSeconds_;

    attackTime_ = juce::jlimit(0.001f, maxTime, points_[1].x * maxTime);
    decayTime_ = juce::jlimit(0.001f, maxTime, (points_[2].x - points_[1].x) * maxTime);
    sustainLevel_ = juce::jlimit(0.0f, 1.0f, points_[2].y);
    releaseTime_ = juce::jlimit(0.001f, maxTime, (points_[4].x - points_[3].x) * maxTime);

    if (envelopeChangedCallback_) {
        envelopeChangedCallback_();
    }
}

void SkiaEnvelopeEditor::setAttackTime(float seconds) {
    attackTime_ = juce::jlimit(0.001f, maxTimeSeconds_, seconds);
    updatePointsFromParameters();
    markDirty();
}

void SkiaEnvelopeEditor::setDecayTime(float seconds) {
    decayTime_ = juce::jlimit(0.001f, maxTimeSeconds_, seconds);
    updatePointsFromParameters();
    markDirty();
}

void SkiaEnvelopeEditor::setSustainLevel(float level) {
    sustainLevel_ = juce::jlimit(0.0f, 1.0f, level);
    updatePointsFromParameters();
    markDirty();
}

void SkiaEnvelopeEditor::setReleaseTime(float seconds) {
    releaseTime_ = juce::jlimit(0.001f, maxTimeSeconds_, seconds);
    updatePointsFromParameters();
    markDirty();
}

void SkiaEnvelopeEditor::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Draw background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(20, 20, 20, 25));
    canvas->drawRect(bounds, bgPaint);

    drawGrid(canvas, bounds);
    drawEnvelope(canvas, bounds);
    drawNodes(canvas, bounds);
    drawNodeLabels(canvas, bounds);
}

void SkiaEnvelopeEditor::drawGrid(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint gridPaint;
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setColor(SkColorSetARGB(30, 255, 255, 255));

    // Draw horizontal lines
    for (int i = 0; i <= 4; ++i) {
        float y = bounds.fTop + (bounds.height() * i / 4.0f);
        canvas->drawLine(bounds.fLeft, y, bounds.fRight, y, gridPaint);
    }

    // Draw vertical lines
    for (int i = 0; i <= 4; ++i) {
        float x = bounds.fLeft + (bounds.width() * i / 4.0f);
        canvas->drawLine(x, bounds.fTop, x, bounds.fBottom, gridPaint);
    }
}

void SkiaEnvelopeEditor::drawEnvelope(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint linePaint;
    linePaint.setAntiAlias(true);
    linePaint.setStyle(SkPaint::kStroke_Style);
    linePaint.setStrokeWidth(lineWidth_);
    linePaint.setColor(SkColorSetRGB(0, 200, 255));

    if (glowIntensity_ > 0) {
        linePaint.setMaskFilter(SkMaskFilter::MakeBlur(
            kNormal_SkBlurStyle, glowIntensity_ * 3.0f));
    }

    SkPath envelopePath;

    for (size_t i = 0; i < points_.size(); ++i) {
        SkPoint pos = valueToPoint(points_[i].x, points_[i].y, bounds);

        if (i == 0) {
            envelopePath.moveTo(pos);
        } else {
            if (points_[i].curved && i > 0) {
                // Draw curve
                SkPoint prevPos = valueToPoint(points_[i - 1].x, points_[i - 1].y, bounds);
                SkPoint control = SkPoint::Make(
                    (prevPos.x() + pos.x()) * 0.5f,
                    prevPos.y() + (pos.y() - prevPos.y()) * 0.5f * points_[i].curveAmount
                );
                envelopePath.quadTo(control, pos);
            } else {
                envelopePath.lineTo(pos);
            }
        }
    }

    canvas->drawPath(envelopePath, linePaint);
}

void SkiaEnvelopeEditor::drawNodes(SkCanvas* canvas, const SkRect& bounds) {
    for (size_t i = 0; i < points_.size(); ++i) {
        if (points_[i].fixed) continue;

        SkPoint pos = valueToPoint(points_[i].x, points_[i].y, bounds);

        SkPaint nodePaint;
        nodePaint.setAntiAlias(true);

        if (static_cast<int>(i) == selectedNode_) {
            nodePaint.setColor(SkColorSetRGB(255, 255, 100));
            nodePaint.setMaskFilter(SkMaskFilter::MakeBlur(
                kNormal_SkBlurStyle, 4.0f));
        } else {
            nodePaint.setColor(SkColorSetRGB(200, 200, 255));
        }

        canvas->drawCircle(pos, nodeSize_, nodePaint);

        // Draw inner circle
        SkPaint innerPaint;
        innerPaint.setAntiAlias(true);
        innerPaint.setColor(SkColorSetRGB(50, 50, 80));
        canvas->drawCircle(pos, nodeSize_ * 0.5f, innerPaint);
    }
}

void SkiaEnvelopeEditor::drawNodeLabels(SkCanvas* canvas, const SkRect& bounds) {
    if (!showValues_) return;

    const char* labels[] = {"A", "D", "S", "R"};
    float values[] = {attackTime_, decayTime_, sustainLevel_, releaseTime_};

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(150, 255, 255, 255));

    for (int i = 1; i <= 4; ++i) {
        SkPoint pos = valueToPoint(points_[i].x, points_[i].y, bounds);

        juce::String label = labels[i - 1] + juce::String(":") + juce::String(values[i - 1], 2);
        // Draw text
    }
}

SkPoint SkiaEnvelopeEditor::valueToPoint(float time, float level, const SkRect& bounds) {
    float padding = 30.0f;
    float drawWidth = bounds.width() - padding * 2;
    float drawHeight = bounds.height() - padding * 2;

    float x = bounds.fLeft + padding + time * drawWidth;
    float y = bounds.fBottom - padding - level * drawHeight;

    return SkPoint::Make(x, y);
}

void SkiaEnvelopeEditor::pointToValue(const SkPoint& point, const SkRect& bounds,
                                     float& time, float& level) {
    float padding = 30.0f;
    float drawWidth = bounds.width() - padding * 2;
    float drawHeight = bounds.height() - padding * 2;

    time = (point.x() - bounds.fLeft - padding) / drawWidth;
    level = (bounds.fBottom - padding - point.y()) / drawHeight;

    time = juce::jlimit(0.0f, 1.0f, time);
    level = juce::jlimit(0.0f, 1.0f, level);
}

int SkiaEnvelopeEditor::findNodeAt(float x, float y) {
    for (size_t i = 0; i < points_.size(); ++i) {
        SkPoint pos = valueToPoint(points_[i].x, points_[i].y,
                                   SkRect::MakeXYWH(0, 0, getWidth(), getHeight()));
        float distance = std::sqrt(std::pow(pos.x() - x, 2) + std::pow(pos.y() - y, 2));
        if (distance < nodeSize_ * 1.5f) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void SkiaEnvelopeEditor::mouseDown(const juce::MouseEvent& e) {
    int node = findNodeAt(e.position.x, e.position.y);
    if (node >= 0 && !points_[node].fixed) {
        selectedNode_ = node;
        isDragging_ = true;
        dragOffsetX_ = 0;
        dragOffsetY_ = 0;
        markDirty();
    }
}

void SkiaEnvelopeEditor::mouseDrag(const juce::MouseEvent& e) {
    if (isDragging_ && selectedNode_ >= 0) {
        float time, level;
        pointToValue(SkPoint::Make(e.position.x, e.position.y),
                     SkRect::MakeXYWH(0, 0, getWidth(), getHeight()),
                     time, level);

        // Apply constraints based on node
        if (selectedNode_ == 1) { // Attack peak
            points_[selectedNode_].x = juce::jlimit(0.0f, points_[2].x - 0.01f, time);
            points_[selectedNode_].y = 1.0f;  // Always at max
        } else if (selectedNode_ == 2) { // Decay/sustain
            points_[selectedNode_].x = juce::jlimit(points_[1].x + 0.01f, 0.95f, time);
            points_[selectedNode_].y = juce::jlimit(0.0f, 1.0f, level);
            points_[3].y = level;  // Keep sustain level consistent
        } else if (selectedNode_ == 3) { // Sustain end
            points_[selectedNode_].x = juce::jlimit(points_[2].x, 0.98f, time);
            points_[selectedNode_].y = points_[2].y;
        }

        updateParametersFromPoints();
        markDirty();
    }
}

void SkiaEnvelopeEditor::mouseUp(const juce::MouseEvent& e) {
    isDragging_ = false;
    selectedNode_ = -1;
}

//==============================================================================
// SkiaSynthPanel Implementation
//==============================================================================

SkiaSynthPanel::SkiaSynthPanel() {
    // Initialize layout sections
    oscillatorWaveform_ = std::make_unique<SkiaWaveformDisplay>();
    ampEnvelope_ = std::make_unique<SkiaEnvelopeEditor>();
}

void SkiaSynthPanel::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    drawBackground(canvas, bounds);
    drawSectionBackgrounds(canvas, bounds);
    drawSectionLabels(canvas, bounds);
}

void SkiaSynthPanel::drawBackground(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(255, 15, 15, 20));
    canvas->drawRect(bounds, bgPaint);

    // Draw subtle gradient
    SkColor colors[2] = {
        SkColorSetARGB(255, 20, 20, 30),
        SkColorSetARGB(255, 10, 10, 15)
    };
    SkPoint points[2] = {
        {bounds.fLeft, bounds.fTop},
        {bounds.fLeft, bounds.fBottom}
    };
    bgPaint.setShader(SkGradientShader::MakeLinear(
        points, colors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(bounds, bgPaint);
}

void SkiaSynthPanel::drawSectionBackgrounds(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint sectionPaint;
    sectionPaint.setAntiAlias(true);
    sectionPaint.setStyle(SkPaint::kStroke_Style);
    sectionPaint.setStrokeWidth(1.0f);
    sectionPaint.setColor(SkColorSetARGB(50, 255, 255, 255));

    // Draw section dividers
    float sectionWidth = bounds.width() / 5.0f;

    for (int i = 1; i < 5; ++i) {
        float x = bounds.fLeft + sectionWidth * i;
        canvas->drawLine(x, bounds.fTop + 10, x, bounds.fBottom - 10, sectionPaint);
    }
}

void SkiaSynthPanel::drawSectionLabels(SkCanvas* canvas, const SkRect& bounds) {
    const char* labels[] = {"OSC", "FILTER", "ENV", "FX", "MACRO"};
    float sectionWidth = bounds.width() / 5.0f;

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(100, 255, 255, 255));

    for (int i = 0; i < 5; ++i) {
        float x = bounds.fLeft + sectionWidth * i + sectionWidth * 0.5f;
        // Draw label at x, bounds.fTop + 15
    }
}

void SkiaSynthPanel::resized() {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());
    float sectionWidth = bounds.width() / 5.0f;

    // Oscillator section
    oscillatorSection_ = SkRect::MakeXYWH(
        bounds.fLeft + 5, bounds.fTop + 30,
        sectionWidth - 10, bounds.height() - 40
    );

    // Filter section
    filterSection_ = SkRect::MakeXYWH(
        bounds.fLeft + sectionWidth + 5, bounds.fTop + 30,
        sectionWidth - 10, bounds.height() - 40
    );

    // Envelope section
    envelopeSection_ = SkRect::MakeXYWH(
        bounds.fLeft + sectionWidth * 2 + 5, bounds.fTop + 30,
        sectionWidth - 10, bounds.height() - 40
    );

    // Effects section
    effectsSection_ = SkRect::MakeXYWH(
        bounds.fLeft + sectionWidth * 3 + 5, bounds.fTop + 30,
        sectionWidth - 10, bounds.height() - 40
    );

    // Macro section
    macroSection_ = SkRect::MakeXYWH(
        bounds.fLeft + sectionWidth * 4 + 5, bounds.fTop + 30,
        sectionWidth - 10, bounds.height() - 40
    );

    // Position child components
    if (oscillatorWaveform_) {
        oscillatorWaveform_->setBounds(
            static_cast<int>(oscillatorSection_.fLeft),
            static_cast<int>(oscillatorSection_.fTop),
            static_cast<int>(oscillatorSection_.width()),
            static_cast<int>(oscillatorSection_.width())  // Square
        );
    }

    if (ampEnvelope_) {
        ampEnvelope_->setBounds(
            static_cast<int>(envelopeSection_.fLeft),
            static_cast<int>(envelopeSection_.fTop),
            static_cast<int>(envelopeSection_.width()),
            static_cast<int>(envelopeSection_.height())
        );
    }
}

void SkiaSynthPanel::addOscillatorControl(std::unique_ptr<SkiaRotaryKnob> knob) {
    oscillatorKnobs_.push_back(std::move(knob));
    addAndMakeVisible(oscillatorKnobs_.back().get());
}

void SkiaSynthPanel::setOscillatorWaveformDisplay(std::unique_ptr<SkiaWaveformDisplay> display) {
    oscillatorWaveform_ = std::move(display);
    addAndMakeVisible(oscillatorWaveform_.get());
}

void SkiaSynthPanel::addFilterControl(std::unique_ptr<SkiaRotaryKnob> knob) {
    filterKnobs_.push_back(std::move(knob));
    addAndMakeVisible(filterKnobs_.back().get());
}

void SkiaSynthPanel::setFilterCutoffKnob(std::unique_ptr<SkiaRotaryKnob> knob) {
    filterKnobs_.push_back(std::move(knob));
    addAndMakeVisible(filterKnobs_.back().get());
}

void SkiaSynthPanel::setEnvelopeEditor(std::unique_ptr<SkiaEnvelopeEditor> editor) {
    ampEnvelope_ = std::move(editor);
    addAndMakeVisible(ampEnvelope_.get());
}

void SkiaSynthPanel::addEffectControl(std::unique_ptr<SkiaLinearFader> fader) {
    effectFaders_.push_back(std::move(fader));
    addAndMakeVisible(effectFaders_.back().get());
}

void SkiaSynthPanel::addMacroControl(std::unique_ptr<SkiaMacroControl> macro) {
    macroControls_.push_back(std::move(macro));
    addAndMakeVisible(macroControls_.back().get());
}

//==============================================================================
// SkiaMacroControl Implementation
//==============================================================================

SkiaMacroControl::SkiaMacroControl() {
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
}

void SkiaMacroControl::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Draw background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(SkColorSetARGB(30, 20, 20, 30));
    canvas->drawRRect(SkRRect::MakeRectXY(bounds, 8, 8), bgPaint);

    // Draw colored border based on value
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(2.0f);

    float intensity = 0.3f + value_ * 0.7f;
    borderPaint.setColor(SkColorSetARGB(
        static_cast<U8CPU>(255 * intensity),
        SkColorGetR(color_),
        SkColorGetG(color_),
        SkColorGetB(color_)
    ));
    canvas->drawRRect(SkRRect::MakeRectXY(bounds, 8, 8), borderPaint);

    drawKnob(canvas, bounds);
    drawName(canvas, bounds);
    drawMidiIndicator(canvas, bounds);
    drawAssignmentCount(canvas, bounds);
}

void SkiaMacroControl::drawKnob(SkCanvas* canvas, const SkRect& bounds) {
    float size = juce::jmin(bounds.width(), bounds.height()) * 0.6f;
    float cx = bounds.centerX();
    float cy = bounds.fTop + size * 0.8f;

    // Draw track
    SkPaint trackPaint;
    trackPaint.setAntiAlias(true);
    trackPaint.setStyle(SkPaint::kStroke_Style);
    trackPaint.setStrokeWidth(3.0f);
    trackPaint.setColor(SkColorSetARGB(50, 255, 255, 255));

    SkRect trackRect = SkRect::MakeXYWH(cx - size * 0.4f, cy - size * 0.4f,
                                        size * 0.8f, size * 0.8f);
    canvas->drawArc(trackRect, 135, 270, false, trackPaint);

    // Draw fill
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setStyle(SkPaint::kStroke_Style);
    fillPaint.setStrokeWidth(3.0f);
    fillPaint.setColor(color_);

    float sweepAngle = value_ * 270;
    canvas->drawArc(trackRect, 135, sweepAngle, false, fillPaint);

    // Draw indicator
    float angle = (135 + sweepAngle) * juce::MathConstants<float>::pi / 180.0f;
    float ix = cx + std::cos(angle) * size * 0.35f;
    float iy = cy + std::sin(angle) * size * 0.35f;

    SkPaint indicatorPaint;
    indicatorPaint.setAntiAlias(true);
    indicatorPaint.setColor(SkColorSetRGB(255, 255, 255));
    canvas->drawCircle(ix, iy, 4.0f, indicatorPaint);
}

void SkiaMacroControl::drawName(SkCanvas* canvas, const SkRect& bounds) {
    // Draw macro name
    // (Use proper font rendering in production)
}

void SkiaMacroControl::drawMidiIndicator(SkCanvas* canvas, const SkRect& bounds) {
    if (midiCC_ >= 0 || midiLearning_) {
        SkPaint midiPaint;
        midiPaint.setAntiAlias(true);

        if (midiLearning_) {
            // Blink effect
            static int blinkCounter = 0;
            blinkCounter++;
            if ((blinkCounter / 10) % 2 == 0) {
                midiPaint.setColor(SkColorSetRGB(255, 255, 0));
            } else {
                midiPaint.setColor(SkColorSetRGB(100, 100, 0));
            }
        } else {
            midiPaint.setColor(SkColorSetRGB(0, 200, 100));
        }

        // Draw MIDI indicator dot
        canvas->drawCircle(bounds.fRight - 15, bounds.fTop + 15, 5.0f, midiPaint);
    }
}

void SkiaMacroControl::drawAssignmentCount(SkCanvas* canvas, const SkRect& bounds) {
    if (numAssignments_ > 0) {
        SkPaint countPaint;
        countPaint.setAntiAlias(true);
        countPaint.setColor(SkColorSetARGB(200, 255, 255, 255));

        // Draw assignment count badge
        float badgeX = bounds.fRight - 15;
        float badgeY = bounds.fBottom - 25;

        SkPaint badgeBgPaint;
        badgeBgPaint.setAntiAlias(true);
        badgeBgPaint.setColor(color_);

        canvas->drawCircle(badgeX, badgeY, 10.0f, badgeBgPaint);

        // Draw number (simplified)
    }
}

void SkiaMacroControl::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        isDragging_ = true;
        dragStartValue_ = value_;
    } else if (e.mods.isRightButtonDown()) {
        // Toggle MIDI learn
        midiLearning_ = !midiLearning_;
        markDirty();
    }
}

void SkiaMacroControl::mouseDrag(const juce::MouseEvent& e) {
    if (isDragging_) {
        float deltaY = e.getDistanceFromDragStartY();
        float sensitivity = e.mods.isShiftDown() ? 0.1f : 0.5f;
        float newValue = dragStartValue_ - deltaY * sensitivity * 0.01f;
        setValue(juce::jlimit(0.0f, 1.0f, newValue));
    }
}

void SkiaMacroControl::mouseUp(const juce::MouseEvent& e) {
    isDragging_ = false;
}

void SkiaMacroControl::mouseDoubleClick(const juce::MouseEvent& e) {
    setValue(0.5f);
}

} // namespace zenith
