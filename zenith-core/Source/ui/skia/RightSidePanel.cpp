/*
  ==============================================================================

    RightSidePanel.cpp
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

  ==============================================================================
*/

#include "RightSidePanel.h"

#ifdef ZENITH_USE_SKIA
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkRect.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkColor.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

RightSidePanel::RightSidePanel()
{
    setSize(300, 600);
}

RightSidePanel::~RightSidePanel() = default;

void RightSidePanel::drawSkia(SkCanvas* canvas) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    // Glassmorphism Background (Darker, matching theme)
    paint.setColor(SkColorSetARGB(240, 20, 20, 20)); // Almost opaque dark grey
    canvas->drawRect(::SkRect::MakeWH((float)getWidth(), (float)getHeight()), paint);
    
    // Left border glow (Accent color: Cyan)
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(SkColorSetARGB(100, 0, 170, 255)); // Cyan accent
    canvas->drawLine(0.0f, 0.0f, 0.0f, (float)getHeight(), paint);
    
    // Header
    SkFont font;
    font.setSize(16.0f);
    font.setEmbolden(true);
    
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(255, 255, 255, 255)); // White text
    canvas->drawString("WINGMAN AI", 20.0f, 30.0f, font, paint);
    
    // Placeholder content
    font.setSize(12.0f);
    font.setEmbolden(false);
    paint.setColor(SkColorSetARGB(180, 200, 200, 200)); // Light grey text
    canvas->drawString("Chat with your AI assistant...", 20.0f, 60.0f, font, paint);

    // Master Meter (Visualist Request: Peak vs RMS)
    float meterX = (float)getWidth() - 40.0f;
    float meterY = 80.0f;
    float meterW = 20.0f;
    float meterH = (float)getHeight() - 100.0f;

    // Background
    paint.setColor(SkColorSetARGB(100, 10, 10, 10));
    canvas->drawRect(::SkRect::MakeXYWH(meterX, meterY, meterW, meterH), paint);

    // Simulated Levels (since we don't have real audio data here yet)
    // In a real app, these would come from the Engine
    static float phase = 0.0f;
    phase += 0.05f;
    float peakLevel = (std::sin(phase) * 0.5f + 0.5f) * 0.8f + 0.1f; // 0.1 to 0.9
    float rmsLevel = peakLevel * 0.7f; // RMS is usually lower

    // Peak Bar (Fast, Green/Red)
    float peakH = meterH * peakLevel;
    paint.setColor(peakLevel > 0.8f ? SkColorSetRGB(255, 50, 50) : SkColorSetRGB(0, 255, 100));
    canvas->drawRect(::SkRect::MakeXYWH(meterX, meterY + meterH - peakH, meterW, peakH), paint);

    // RMS Bar (Slow, Solid White line inside)
    float rmsH = meterH * rmsLevel;
    paint.setColor(SkColorSetARGB(200, 255, 255, 255));
    canvas->drawRect(::SkRect::MakeXYWH(meterX + 5.0f, meterY + meterH - rmsH, meterW - 10.0f, rmsH), paint);

    // Label
    font.setSize(10.0f);
    paint.setColor(SkColorSetARGB(150, 255, 255, 255));
    canvas->drawString("RMS", meterX, meterY + meterH + 15.0f, font, paint);
}

void RightSidePanel::resized() {
    // Layout children
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
