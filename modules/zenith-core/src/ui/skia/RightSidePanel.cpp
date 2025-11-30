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
    
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(255, 255, 255, 255)); // White text
    canvas->drawSimpleText("WINGMAN AI", 10, SkTextEncoding::kUTF8, 20, 30, font, paint);
    
    // Placeholder content
    font.setSize(12.0f);
    font.setEmbolden(false);
    paint.setColor(SkColorSetARGB(180, 200, 200, 200)); // Light grey text
    canvas->drawSimpleText("Chat with your AI assistant...", 32, SkTextEncoding::kUTF8, 20, 60, font, paint);
}

void RightSidePanel::resized() {
    // Layout children
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
