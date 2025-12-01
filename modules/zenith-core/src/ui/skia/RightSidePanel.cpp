/*
  ==============================================================================

    RightSidePanel.cpp
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

  ==============================================================================
*/

#include "RightSidePanel.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRect.h>
#include <include/core/SkFont.h>
#include <include/core/SkColor.h>
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
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), paint);
    
    // Left border glow (Accent color: Cyan)
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(SkColorSetARGB(100, 0, 170, 255)); // Cyan accent
    canvas->drawLine(0, 0, 0, getHeight(), paint);
    
    // Header
    SkFont font;
    font.setSize(16.0f);
    font.setEmbolden(true);
    font.setEdging(SkFont::Edging::kAntiAlias);
    
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(255, 255, 255, 255)); // White text
    canvas->drawString("WINGMAN AI", 20, 30, font, paint);
    
    // Chat History Area
    SkRect historyRect = SkRect::MakeXYWH(10, 50, (float)getWidth() - 20, (float)getHeight() - 100);
    paint.setColor(SkColorSetARGB(50, 0, 0, 0));
    canvas->drawRoundRect(historyRect, 5, 5, paint);
    
    // Fake chat bubbles
    SkPaint bubblePaint;
    bubblePaint.setAntiAlias(true);
    bubblePaint.setColor(SkColorSetARGB(255, 40, 40, 50));
    
    SkRect bubble1 = SkRect::MakeXYWH(20, 60, (float)getWidth() - 60, 40);
    canvas->drawRoundRect(bubble1, 10, 10, bubblePaint);
    
    font.setSize(12.0f);
    font.setEmbolden(false);
    paint.setColor(SK_ColorWHITE);
    canvas->drawString("How can I help you with your track?", 30, 85, font, paint);
    
    // Input Box
    SkRect inputRect = SkRect::MakeXYWH(10, (float)getHeight() - 40, (float)getWidth() - 20, 30);
    paint.setColor(SkColorSetARGB(255, 30, 30, 30));
    canvas->drawRoundRect(inputRect, 15, 15, paint);
    
    paint.setColor(SkColorSetARGB(100, 255, 255, 255));
    canvas->drawString("Type a command...", 20, (float)getHeight() - 20, font, paint);
}

void RightSidePanel::resized() {
    // Layout children
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
