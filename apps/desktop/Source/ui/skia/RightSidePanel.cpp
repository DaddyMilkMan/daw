/*
  ==============================================================================

    RightSidePanel.cpp
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

  ==============================================================================
*/

#include "RightSidePanel.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <core/SkFont.h>
#include <core/SkColor.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

RightSidePanel::RightSidePanel()
{
    setSize(300, 600);
    startTimerHz(60); // Animation timer
}

RightSidePanel::~RightSidePanel() {
    stopTimer();
}

void RightSidePanel::timerCallback() {
    animationPhase_ += 0.05f;
    repaint();
}

void RightSidePanel::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Lazy update of cached resources on the Render Thread
    if (skBounds != cachedBounds_) {
        updateCachedPaints(skBounds);
        cachedBounds_ = skBounds;
    }
    
    // Glassmorphism Background
    canvas->drawRect(skBounds, bgPaint_);
    
    // Left border glow
    canvas->drawLine(0.0f, 0.0f, 0.0f, skBounds.height(), borderPaint_);
    
    // Header
    canvas->drawString("WINGMAN AI", 20.0f, 30.0f, headerFont_, textPaint_);
    
    // Placeholder content
    canvas->drawString("Chat with your AI assistant...", 20.0f, 60.0f, bodyFont_, subTextPaint_);

    // Master Meter (Visualist Request: Peak vs RMS)
    float meterX = skBounds.width() - 40.0f;
    float meterY = 80.0f;
    float meterW = 20.0f;
    float meterH = skBounds.height() - 100.0f;

    // Background
    canvas->drawRect(SkRect::MakeXYWH(meterX, meterY, meterW, meterH), meterBgPaint_);

    // Simulated Levels
    float peakLevel = (std::sin(animationPhase_) * 0.5f + 0.5f) * 0.8f + 0.1f; // 0.1 to 0.9
    float rmsLevel = peakLevel * 0.7f; // RMS is usually lower

    // Peak Bar (Fast, Green/Red)
    float peakH = meterH * peakLevel;
    meterPeakPaint_.setColor(peakLevel > 0.8f ? SkColorSetRGB(255, 50, 50) : SkColorSetRGB(0, 255, 100));
    canvas->drawRect(SkRect::MakeXYWH(meterX, meterY + meterH - peakH, meterW, peakH), meterPeakPaint_);

    // RMS Bar (Slow, Solid White line inside)
    float rmsH = meterH * rmsLevel;
    canvas->drawRect(SkRect::MakeXYWH(meterX + 5.0f, meterY + meterH - rmsH, meterW - 10.0f, rmsH), meterRmsPaint_);

    // Label
    canvas->drawString("RMS", meterX, meterY + meterH + 15.0f, labelFont_, subTextPaint_);
}

void RightSidePanel::updateCachedPaints(const SkRect& bounds) {
    // 1. Background Paint
    bgPaint_.setAntiAlias(true);
    bgPaint_.setColor(SkColorSetARGB(240, 20, 20, 20)); // Almost opaque dark grey
    bgPaint_.setStyle(SkPaint::kFill_Style);

    // 2. Border Paint
    borderPaint_.setAntiAlias(true);
    borderPaint_.setStyle(SkPaint::kStroke_Style);
    borderPaint_.setStrokeWidth(1.0f);
    borderPaint_.setColor(SkColorSetARGB(100, 0, 170, 255)); // Cyan accent

    // 3. Text Paints
    textPaint_.setAntiAlias(true);
    textPaint_.setStyle(SkPaint::kFill_Style);
    textPaint_.setColor(SkColorSetARGB(255, 255, 255, 255)); // White text

    subTextPaint_.setAntiAlias(true);
    subTextPaint_.setStyle(SkPaint::kFill_Style);
    subTextPaint_.setColor(SkColorSetARGB(180, 200, 200, 200)); // Light grey text

    // 4. Fonts
    headerFont_.setSize(16.0f);
    headerFont_.setEmbolden(true);
    headerFont_.setSubpixel(true);

    bodyFont_.setSize(12.0f);
    bodyFont_.setEmbolden(false);
    bodyFont_.setSubpixel(true);

    labelFont_.setSize(10.0f);
    labelFont_.setSubpixel(true);

    // 5. Meter Paints
    meterBgPaint_.setAntiAlias(true);
    meterBgPaint_.setColor(SkColorSetARGB(100, 10, 10, 10));
    meterBgPaint_.setStyle(SkPaint::kFill_Style);

    meterPeakPaint_.setAntiAlias(true);
    meterPeakPaint_.setStyle(SkPaint::kFill_Style);

    meterRmsPaint_.setAntiAlias(true);
    meterRmsPaint_.setColor(SkColorSetARGB(200, 255, 255, 255));
    meterRmsPaint_.setStyle(SkPaint::kFill_Style);
}

void RightSidePanel::resized() {
    // Layout children
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
