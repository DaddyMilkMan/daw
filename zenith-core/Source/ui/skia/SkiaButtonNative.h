/**
 * @file SkiaButtonNative.h
 * @brief Beautiful GPU-accelerated button with PURE native Skia rendering
 */

#pragma once

#include "SkiaComponent.h"
#include "SkiaTheme.h"
#include <functional>
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
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>

#endif

namespace zenith {

/**
 * @class SkiaButtonNative
 * @brief GPU-accelerated button using PURE Skia rendering (no intermediate
 * renderer)
 *
 * Features:
 * - Direct SkCanvas rendering via parent's renderer
 * - Spring physics animations
 * - Multiple visual styles
 * - Gradients, shadows, and anti-aliasing
 */
class SkiaButtonNative : public SkiaComponent, private juce::Timer {
public:
  enum class Style {
    Primary,   ///< Blue accent
    Secondary, ///< Gray (neutral)
    Success,   ///< Green
    Danger,    ///< Red
    Warning    ///< Orange
  };

  SkiaButtonNative(const juce::String &buttonText = {},
                   Style style = Style::Primary)
      : buttonText_(buttonText), style_(style), isHovered_(false),
        isPressed_(false), hoverProgress_(0.0f), hoverVelocity_(0.0f),
        pressProgress_(0.0f), pressVelocity_(0.0f) {
    // setOpaque(false);  // Handled by SkiaComponent
    startTimer(16); // 60 FPS animation
  }

  ~SkiaButtonNative() override { stopTimer(); }

  //==========================================================================
  // Configuration
  //==========================================================================

  void setButtonText(const juce::String &text) {
    buttonText_ = text;
    repaint();
  }

  juce::String getButtonText() const { return buttonText_; }

  void setStyle(Style style) {
    style_ = style;
    repaint();
  }

  Style getStyle() const { return style_; }

  std::function<void()> onClick;

  //==========================================================================
  // Mouse events
  //==========================================================================

  void mouseEnter(const juce::MouseEvent &) override { isHovered_ = true; }

  void mouseExit(const juce::MouseEvent &) override {
    isHovered_ = false;
    isPressed_ = false;
  }

  void mouseDown(const juce::MouseEvent &) override { isPressed_ = true; }

  void mouseUp(const juce::MouseEvent &event) override {
    bool wasPressed = isPressed_;
    isPressed_ = false;

    if (wasPressed && event.mouseWasClicked() && onClick) {
      onClick();
    }
  }

  //==========================================================================
  // SkiaComponent implementation - NATIVE SKIA RENDERING
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override {
    auto bounds = SkRect::MakeWH(getWidth(), getHeight());
#ifdef ZENITH_USE_SKIA
    // Get theme colors
    const auto &theme = SkiaTheme::getInstance();
    const auto &colors = theme.getColors();

    // Determine button color based on style
    SkColor baseColor;
    switch (style_) {
    case Style::Primary:
      baseColor = 0xFF0A84FF;
      break; // Blue
    case Style::Secondary:
      baseColor = 0xFF636366;
      break; // Gray
    case Style::Success:
      baseColor = 0xFF34C759;
      break; // Green
    case Style::Danger:
      baseColor = 0xFFFF3B30;
      break; // Red
    case Style::Warning:
      baseColor = 0xFFFF9500;
      break; // Orange
    default:
      baseColor = 0xFF0A84FF;
      break;
    }

    // Apply press animation (scale down slightly)
    float scale = 1.0f - (pressProgress_ * 0.05f); // 5% scale down when pressed
    float dx = bounds.width() * (1.0f - scale) / 2.0f;
    float dy = bounds.height() * (1.0f - scale) / 2.0f;
    SkRect scaledBounds = bounds.makeInset(dx, dy);

    // Corner radius (modern rounded buttons)
    float cornerRadius = 6.0f;

    // Shadow (when not pressed)
    if (pressProgress_ < 0.5f) {
      SkPaint shadowPaint;
      shadowPaint.setAntiAlias(true);
      shadowPaint.setColor(SkColorSetARGB(30, 0, 0, 0));
      SkRRect shadowRRect = SkRRect::MakeRectXY(scaledBounds.makeOffset(0, 2),
                                                cornerRadius, cornerRadius);
      canvas->drawRRect(shadowRRect, shadowPaint);
    }

    // Button background with gradient
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    // Create subtle gradient (lighter at top)
    uint8_t r = std::min<uint8_t>(255, SkColorGetR(baseColor) + 20);
    uint8_t g = std::min<uint8_t>(255, SkColorGetG(baseColor) + 20);
    uint8_t b = std::min<uint8_t>(255, SkColorGetB(baseColor) + 20);

    SkColor gradColors[2] = {SkColorSetARGB(SkColorGetA(baseColor), r, g, b),
                             baseColor};
    SkPoint gradPoints[2] = {{scaledBounds.x(), scaledBounds.y()},
                             {scaledBounds.x(), scaledBounds.bottom()}};
    bgPaint.setShader(SkGradientShader::MakeLinear(
        gradPoints, gradColors, nullptr, 2, SkTileMode::kClamp));

    SkRRect buttonRRect =
        SkRRect::MakeRectXY(scaledBounds, cornerRadius, cornerRadius);
    canvas->drawRRect(buttonRRect, bgPaint);

    // Hover glow overlay
    if (hoverProgress_ > 0.01f) {
      SkPaint glowPaint;
      glowPaint.setAntiAlias(true);
      glowPaint.setColor(
          SkColorSetARGB((uint8_t)(hoverProgress_ * 40), // Max 40 alpha
                         255, 255, 255));
      canvas->drawRRect(buttonRRect, glowPaint);
    }

    // Button text
    if (!buttonText_.isEmpty()) {
      SkFont font(nullptr, 14.0f);
      font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
      font.setSubpixel(true);

      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      textPaint.setColor(0xFFFFFFFF); // White text

      // Measure text for centering
      SkRect textBounds;
      const char *textStr = buttonText_.toRawUTF8();
      font.measureText(textStr, buttonText_.getNumBytesAsUTF8(),
                       SkTextEncoding::kUTF8, &textBounds);

      // Center text
      float textX = scaledBounds.centerX() - textBounds.width() / 2.0f;
      float textY = scaledBounds.centerY() + textBounds.height() / 3.0f;

      canvas->drawSimpleText(textStr, buttonText_.getNumBytesAsUTF8(),
                             SkTextEncoding::kUTF8, textX, textY, font,
                             textPaint);
    }
#endif
  }

private:
  //==========================================================================
  // Animation (spring physics)
  //==========================================================================

  void timerCallback() override {
    const float dt = 0.016f; // 16ms = 60 FPS
    const float stiffness = 300.0f;
    const float damping = 20.0f;

    bool needsRepaint = false;

    // Hover animation
    float hoverTarget = isHovered_ ? 1.0f : 0.0f;
    if (std::abs(hoverProgress_ - hoverTarget) > 0.001f) {
      float force = -stiffness * (hoverProgress_ - hoverTarget) -
                    damping * hoverVelocity_;
      hoverVelocity_ += force * dt;
      hoverProgress_ += hoverVelocity_ * dt;
      hoverProgress_ = juce::jlimit(0.0f, 1.0f, hoverProgress_);
      needsRepaint = true;
    }

    // Press animation
    float pressTarget = isPressed_ ? 1.0f : 0.0f;
    if (std::abs(pressProgress_ - pressTarget) > 0.001f) {
      float force = -stiffness * (pressProgress_ - pressTarget) -
                    damping * pressVelocity_;
      pressVelocity_ += force * dt;
      pressProgress_ += pressVelocity_ * dt;
      pressProgress_ = juce::jlimit(0.0f, 1.0f, pressProgress_);
      needsRepaint = true;
    }

    if (needsRepaint)
      repaint();
  }

  //==========================================================================
  // Member variables
  //==========================================================================

  juce::String buttonText_;
  Style style_;
  bool isHovered_;
  bool isPressed_;
  float hoverProgress_;
  float hoverVelocity_;
  float pressProgress_;
  float pressVelocity_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaButtonNative)
};

} // namespace zenith
