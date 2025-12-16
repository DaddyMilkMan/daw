/*
  ==============================================================================

    InteractionHelper.h
    Created: 2025-12-15
    Author:  Zenith DAW

    Utility class for consistent hover/pressed state tracking and animations.
    Provides smooth visual feedback for interactive UI elements.

  ==============================================================================
*/

#pragma once

#include "../design-system/ZenithDesignSystem.h"
#include <cmath>
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>

namespace zenith {

/**
 * @brief Tracks interaction state (hover, pressed) with smooth animations
 */
struct InteractionState {
  bool isHovered = false;
  bool isPressed = false;
  bool isFocused = false;

  float hoverAmount = 0.0f;
  float pressAmount = 0.0f;
  float focusAmount = 0.0f;
  float animationSpeed = 8.0f;

  void update(float deltaTime) {
    float speed = animationSpeed * deltaTime;
    hoverAmount = lerp(hoverAmount, isHovered ? 1.0f : 0.0f, speed);
    pressAmount = lerp(pressAmount, isPressed ? 1.0f : 0.0f, speed * 1.5f);
    focusAmount = lerp(focusAmount, isFocused ? 1.0f : 0.0f, speed);
  }

  SkColor blendWithState(SkColor base, SkColor hover, SkColor pressed) const {
    SkColor result = base;
    if (hoverAmount > 0.001f)
      result = design::interpolateColor(result, hover, hoverAmount);
    if (pressAmount > 0.001f)
      result = design::interpolateColor(result, pressed, pressAmount);
    return result;
  }

  float getHoverBrightness() const { return 1.0f + hoverAmount * 0.2f; }
  float getGlowIntensity() const {
    return hoverAmount * 0.5f + pressAmount * 0.5f;
  }

  bool isAnimating() const {
    return std::abs(hoverAmount - (isHovered ? 1.0f : 0.0f)) > 0.01f ||
           std::abs(pressAmount - (isPressed ? 1.0f : 0.0f)) > 0.01f ||
           std::abs(focusAmount - (isFocused ? 1.0f : 0.0f)) > 0.01f;
  }

private:
  static float lerp(float a, float b, float t) {
    t = std::min(1.0f, std::max(0.0f, t));
    return a + (b - a) * t;
  }
};

/**
 * @brief Helper to apply consistent hover overlay to a bounds rectangle
 */
class InteractionHelper {
public:
  static void drawHoverOverlay(SkCanvas *canvas, const SkRect &bounds,
                               float hoverAmount, float cornerRadius = 4.0f) {
    if (hoverAmount < 0.01f)
      return;

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(
        design::withAlpha(design::colors::GLASS_HOVER, hoverAmount * 0.8f));

    if (cornerRadius > 0)
      canvas->drawRoundRect(bounds, cornerRadius, cornerRadius, paint);
    else
      canvas->drawRect(bounds, paint);
  }

  static void drawPressedOverlay(SkCanvas *canvas, const SkRect &bounds,
                                 float pressAmount, float cornerRadius = 4.0f) {
    if (pressAmount < 0.01f)
      return;

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SkColorSetARGB(static_cast<int>(pressAmount * 30), 0, 0, 0));

    if (cornerRadius > 0)
      canvas->drawRoundRect(bounds, cornerRadius, cornerRadius, paint);
    else
      canvas->drawRect(bounds, paint);
  }

  static void drawFocusRing(SkCanvas *canvas, const SkRect &bounds,
                            float focusAmount, float cornerRadius = 4.0f) {
    if (focusAmount < 0.01f)
      return;

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setColor(
        design::withAlpha(design::colors::BORDER_FOCUS, focusAmount * 0.8f));

    SkRect focusBounds = bounds;
    focusBounds.outset(2.0f, 2.0f);

    if (cornerRadius > 0)
      canvas->drawRoundRect(focusBounds, cornerRadius + 2, cornerRadius + 2,
                            paint);
    else
      canvas->drawRect(focusBounds, paint);
  }
};

} // namespace zenith
