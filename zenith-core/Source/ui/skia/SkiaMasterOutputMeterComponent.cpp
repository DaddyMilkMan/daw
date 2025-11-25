/**
 * @file SkiaMasterOutputMeterComponent.cpp
 * @brief Skia-rendered master output meter with professional ballistics
 *
 * Implements VU-style ballistics (~300ms attack/release) for natural feel
 * with peak hold indicators and color-coded zones (green/yellow/red).
 */

#include "SkiaMasterOutputMeterComponent.h"

#ifdef ZENITH_USE_SKIA

#include <cmath>
#include <include/core/SkRect.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/core/SkTextBlob.h>

namespace zenith {

SkiaMasterOutputMeterComponent::SkiaMasterOutputMeterComponent()
{
    setSize(60, 300);
    startTimer(16); // ~60 FPS for smooth meter updates
}

SkiaMasterOutputMeterComponent::~SkiaMasterOutputMeterComponent()
{
    stopTimer();
}

void SkiaMasterOutputMeterComponent::setLevel(float leftLevel, float rightLevel)
{
    leftLevel_ = juce::jlimit(0.0f, 1.0f, leftLevel);
    rightLevel_ = juce::jlimit(0.0f, 1.0f, rightLevel);
}

void SkiaMasterOutputMeterComponent::setPeakLevel(float leftPeak, float rightPeak)
{
    leftPeak_ = juce::jlimit(0.0f, 1.0f, leftPeak);
    rightPeak_ = juce::jlimit(0.0f, 1.0f, rightPeak);
}

void SkiaMasterOutputMeterComponent::paintSkia(SkCanvas& canvas, const juce::Rectangle<int>& bounds)
{
    auto& theme = SkiaTheme::getInstance();
    const auto& colors = theme.getColors();

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(colors.bg2);
    bgPaint.setAntiAlias(true);
    canvas.drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bgPaint);

    // Meter dimensions
    const float meterWidth = 16.0f;
    const float meterHeight = bounds.getHeight() - 32.0f;
    const float leftMeterX = 10.0f;
    const float rightMeterX = bounds.getWidth() - meterWidth - 10.0f;
    const float meterY = 16.0f;

    // Render both meters
    renderMeter(canvas, leftMeterX, meterY, meterWidth, meterHeight,
                leftSmoothed_, leftPeakHold_, "L");
    renderMeter(canvas, rightMeterX, meterY, meterWidth, meterHeight,
                rightSmoothed_, rightPeakHold_, "R");
}

void SkiaMasterOutputMeterComponent::renderMeter(SkCanvas& canvas, float x, float y,
                                                  float width, float height,
                                                  float level, float peak,
                                                  const juce::String& label)
{
    auto& theme = SkiaTheme::getInstance();
    const auto& colors = theme.getColors();
    const auto& typo = theme.getTypography();

    // Meter background
    SkPaint bgPaint;
    bgPaint.setColor(colors.bg0);
    bgPaint.setAntiAlias(true);
    canvas.drawRect(SkRect::MakeXYWH(x, y, width, height), bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setColor(colors.borderSubtle);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas.drawRect(SkRect::MakeXYWH(x, y, width, height), borderPaint);

    // Level bar (bottom-up, color-coded)
    if (level > 0.0f)
    {
        // Thresholds: 0-60% green, 60-80% yellow, 80-100% red
        const float greenThreshold = 0.60f;
        const float yellowThreshold = 0.80f;

        float currentY = y + height;
        float levelHeight = height * level;

        // Draw red segment (80-100%)
        if (level > yellowThreshold)
        {
            float redStart = yellowThreshold;
            float redHeight = (level - yellowThreshold) * height;
            SkPaint redPaint;
            redPaint.setColor(colors.meterRed);
            redPaint.setAntiAlias(true);
            canvas.drawRect(SkRect::MakeXYWH(x, y + height * (1.0f - level),
                                              width, redHeight), redPaint);
        }

        // Draw yellow segment (60-80%)
        if (level > greenThreshold)
        {
            float yellowStart = greenThreshold;
            float yellowEnd = std::min(level, yellowThreshold);
            float yellowHeight = (yellowEnd - yellowStart) * height;
            SkPaint yellowPaint;
            yellowPaint.setColor(colors.meterYellow);
            yellowPaint.setAntiAlias(true);
            canvas.drawRect(SkRect::MakeXYWH(x, y + height * (1.0f - yellowEnd),
                                              width, yellowHeight), yellowPaint);
        }

        // Draw green segment (0-60%)
        {
            float greenEnd = std::min(level, greenThreshold);
            float greenHeight = greenEnd * height;
            SkPaint greenPaint;
            greenPaint.setColor(colors.meterGreen);
            greenPaint.setAntiAlias(true);
            canvas.drawRect(SkRect::MakeXYWH(x, y + height * (1.0f - greenEnd),
                                              width, greenHeight), greenPaint);
        }
    }

    // Peak hold indicator (white line)
    if (peak > 0.0f)
    {
        float peakY = y + height * (1.0f - peak);
        SkPaint peakPaint;
        peakPaint.setColor(colors.textStrong);
        peakPaint.setStrokeWidth(2.0f);
        peakPaint.setAntiAlias(true);
        canvas.drawLine(x - 2, peakY, x + width + 2, peakY, peakPaint);
    }

    // Label at bottom
    SkPaint textPaint;
    textPaint.setColor(colors.textMuted);
    textPaint.setAntiAlias(true);

    SkFont font;
    font.setSize(typo.small.size);

    auto labelStr = label.toStdString();
    auto blob = SkTextBlob::MakeFromString(labelStr.c_str(), font);

    // Center text under meter
    SkRect textBounds;
    font.measureText(labelStr.c_str(), labelStr.length(), SkTextEncoding::kUTF8, &textBounds);
    float textX = x + (width - textBounds.width()) / 2.0f;
    float textY = y + height + 12.0f;

    canvas.drawTextBlob(blob, textX, textY, textPaint);
}

void SkiaMasterOutputMeterComponent::timerCallback()
{
    // VU-style ballistics: ~300ms time constant
    // At 60fps (16.67ms), smoothing factor = 1 - exp(-dt/tau)
    // tau = 300ms, dt = 16.67ms -> factor ~0.054
    // For more responsive feel, using slightly faster attack
    const float attackFactor = 0.15f;   // ~100ms rise time (faster)
    const float releaseFactor = 0.08f;  // ~200ms fall time (slower)

    // Smooth level meters with asymmetric attack/release
    if (leftLevel_ > leftSmoothed_)
        leftSmoothed_ += (leftLevel_ - leftSmoothed_) * attackFactor;
    else
        leftSmoothed_ += (leftLevel_ - leftSmoothed_) * releaseFactor;

    if (rightLevel_ > rightSmoothed_)
        rightSmoothed_ += (rightLevel_ - rightSmoothed_) * attackFactor;
    else
        rightSmoothed_ += (rightLevel_ - rightSmoothed_) * releaseFactor;

    // Peak hold logic: hold peak for ~1.5 seconds (90 frames at 60fps)
    const int peakHoldFrames = 90;
    const float peakFalloff = 0.05f; // Gradual falloff after hold

    // Left peak
    if (leftLevel_ > leftPeakHold_)
    {
        leftPeakHold_ = leftLevel_;
        leftPeakHoldTimer_ = peakHoldFrames;
    }
    else if (leftPeakHoldTimer_ > 0)
    {
        leftPeakHoldTimer_--;
    }
    else
    {
        leftPeakHold_ -= peakFalloff;
        leftPeakHold_ = std::max(0.0f, leftPeakHold_);
    }

    // Right peak
    if (rightLevel_ > rightPeakHold_)
    {
        rightPeakHold_ = rightLevel_;
        rightPeakHoldTimer_ = peakHoldFrames;
    }
    else if (rightPeakHoldTimer_ > 0)
    {
        rightPeakHoldTimer_--;
    }
    else
    {
        rightPeakHold_ -= peakFalloff;
        rightPeakHold_ = std::max(0.0f, rightPeakHold_);
    }

    repaint();
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
