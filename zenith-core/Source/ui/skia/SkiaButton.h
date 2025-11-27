/*
  ==============================================================================
    SkiaButton.h
    A modern, flat DAW button rendered with Skia.
  ==============================================================================
*/
#pragma once
#include "SkiaComponent.h"
#ifdef ZENITH_USE_SKIA
#include "include/core/SkFont.h"
#endif

namespace zenith {

class SkiaButton : public SkiaComponent {
public:
  SkiaButton(const juce::String &buttonText) : text(buttonText) {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
  }

  void mouseDown(const juce::MouseEvent &e) override {
    isDown = true;
    repaint();
    if (onClick)
      onClick();
  }

  void mouseUp(const juce::MouseEvent &e) override {
    isDown = false;
    repaint();
  }

  void mouseEnter(const juce::MouseEvent &e) override {
    isHover = true;
    repaint();
  }
  void mouseExit(const juce::MouseEvent &e) override {
    isHover = false;
    repaint();
  }

  std::function<void()> onClick;

  void drawSkia(SkCanvas *canvas) override {
#ifdef ZENITH_USE_SKIA
    auto bounds = getLocalBounds();
    SkRect rect = SkRect::MakeXYWH(0, 0, bounds.getWidth(), bounds.getHeight());

    // 1. Background Paint
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    // Modern DAW Colors (Flat Design)
    if (isDown)
      bgPaint.setColor(SkColorSetRGB(60, 60, 60)); // Darker when clicked
    else if (isHover)
      bgPaint.setColor(SkColorSetRGB(80, 80, 80)); // Lighter hover
    else
      bgPaint.setColor(SkColorSetRGB(45, 45, 45)); // Default dark grey

    // Draw Rounded Rectangle (Radius 4.0f)
    canvas->drawRRect(SkRRect::MakeRectXY(rect, 4.0f, 4.0f), bgPaint);

    // 2. Text Paint (Simple centered text)
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    // Basic font setup (replace with SkFont for proper text handling)
    juce::Font font(14.0f);
    float textWidth = font.getStringWidthFloat(text);
    float x = (bounds.getWidth() - textWidth) / 2.0f;
    float y = (bounds.getHeight() / 2.0f) +
              (font.getHeight() / 3.0f); // approximate vertical center

    // NOTE: For pure Skia text, you would use SkTextBlob here.
    // For hybrid, we can cheat and use JUCE for text ONLY if needed,
    // but better to use Skia's DrawTextBlob for crisp rendering.
    // Since we are inside drawSkia, we can't easily use JUCE font rendering
    // unless we render to image. But the user's code just calculates positions.
    // Wait, the user's code doesn't actually DRAW the text!
    // "NOTE: For pure Skia text, you would use SkTextBlob here."

    // I should probably implement text drawing using Skia if possible, or at
    // least a placeholder. The user's code stopped there.

    // I will add simple text drawing using SkFont/SkTextBlob if available.
    // Or just leave it as is if that's what the user provided?
    // The user said "Here is the complete... code".
    // But the code ends with a comment.

    // I'll try to add basic text drawing.

    SkFont skFont;
    skFont.setSize(14.0f);
    skFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);

    // Simple text drawing
    canvas->drawString(text.toRawUTF8(), x, y, skFont, textPaint);
#endif
  }

private:
  juce::String text;
  bool isDown = false;
  bool isHover = false;
};

} // namespace zenith
