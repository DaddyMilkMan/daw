/**
 * @file SkiaTextDisplay.h
 * @brief Text display component with native Skia rendering
 */

#pragma once

#include "SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>


#ifdef ZENITH_USE_SKIA
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkTextBlob.h"
#endif

namespace zenith {

/**
 * @class SkiaTextDisplay
 * @brief Multi-line text display using native Skia rendering
 *
 * Renders text directly with SkCanvas instead of JUCE Graphics.
 * Demonstrates pure Skia rendering with anti-aliasing and custom fonts.
 */
class SkiaTextDisplay : public SkiaComponent {
public:
  SkiaTextDisplay() { setOpaque(false); }

  void setText(const juce::String &newText) {
    text = newText;
    repaint();
  }

  void append(const juce::String &newText) {
    text += newText + "\n";
    repaint();
  }

  void clear() {
    text.clear();
    repaint();
  }

  void drawSkia(SkCanvas *canvas) override {
#ifdef ZENITH_USE_SKIA
    auto bounds = getLocalBounds();
    SkRect rect = SkRect::MakeXYWH(0, 0, (float)bounds.getWidth(),
                                   (float)bounds.getHeight());

    // Debug: Log that we're rendering
    static bool logged = false;
    if (!logged) {
      logged = true;
      DBG("SkiaTextDisplay::drawSkia() called!");
    }

    // Background - DARK for contrast
    SkPaint bgPaint;
    bgPaint.setStyle(SkPaint::kFill_Style); // EXPLICITLY set fill style
    bgPaint.setColor(0xFF0A0A0A);           // Dark gray/black
    bgPaint.setAntiAlias(true);
    canvas->drawRect(rect, bgPaint);

    // Draw colored bars using STROKE style (outline) since fills aren't showing
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(10.0f); // THICK outlines

    // RED bar
    paint.setColor(0xFFFF0000);
    canvas->drawRect(SkRect::MakeXYWH(rect.x() + 10, rect.y() + 10, 100, 20),
                     paint);

    // YELLOW bar
    paint.setColor(0xFFFFFF00);
    canvas->drawRect(SkRect::MakeXYWH(rect.x() + 10, rect.y() + 40, 100, 20),
                     paint);

    // CYAN bar
    paint.setColor(0xFF00FFFF);
    canvas->drawRect(SkRect::MakeXYWH(rect.x() + 10, rect.y() + 70, 100, 20),
                     paint);

    // MAGENTA bar
    paint.setColor(0xFFFF00FF);
    canvas->drawRect(SkRect::MakeXYWH(rect.x() + 10, rect.y() + 100, 100, 20),
                     paint);

    // Border - GREEN for debugging (draw last so it's on top)
    SkPaint borderPaint;
    borderPaint.setColor(0xFF00FF00); // GREEN border
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(3.0f);
    canvas->drawRect(rect, borderPaint);
#endif
  }

private:
  juce::String text;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTextDisplay)
};

} // namespace zenith
