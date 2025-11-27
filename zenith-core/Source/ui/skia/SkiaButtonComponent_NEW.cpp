/**
 * @file SkiaButtonComponent_NEW.cpp
 * @brief Implementation of modern Skia button
 */

#include "SkiaButtonComponent_NEW.h"

#ifdef ZENITH_USE_SKIA
#include "include/core/SkBlurTypes.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/effects/SkGradientShader.h"

#endif

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

namespace {
constexpr float SPRING_STIFFNESS = 350.0f;
constexpr float SPRING_DAMPING = 25.0f;
constexpr float ANIMATION_FPS = 60.0f;
constexpr float ANIMATION_DT = 1.0f / ANIMATION_FPS;
constexpr float PRESS_SCALE = 0.95f;
constexpr float HOVER_GLOW_ALPHA = 0.3f;
constexpr float CORNER_RADIUS = 8.0f;
} // namespace

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaButtonComponent_NEW::SkiaButtonComponent_NEW(const juce::String &buttonText,
                                                 Style style)
    : buttonText_(buttonText), style_(style) {
  // Start animation timer
  startTimer((int)(1000.0f / ANIMATION_FPS));
}

SkiaButtonComponent_NEW::~SkiaButtonComponent_NEW() { stopTimer(); }

//==============================================================================
// Skia Rendering
//==============================================================================

void SkiaButtonComponent_NEW::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  auto bounds = getLocalBounds();
  float width = (float)bounds.getWidth();
  float height = (float)bounds.getHeight();

  // Calculate press scale
  float scale = 1.0f - (pressProgress_ * (1.0f - PRESS_SCALE));
  float scaledWidth = width * scale;
  float scaledHeight = height * scale;
  float offsetX = (width - scaledWidth) / 2.0f;
  float offsetY = (height - scaledHeight) / 2.0f;

  // Button rectangle
  SkRect buttonRect =
      SkRect::MakeXYWH(offsetX, offsetY, scaledWidth, scaledHeight);
  SkRRect roundRect =
      SkRRect::MakeRectXY(buttonRect, CORNER_RADIUS, CORNER_RADIUS);

  //==========================================================================
  // Shadow
  //==========================================================================

  if (pressProgress_ < 0.5f) {
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(
        SkColorSetARGB((int)(60 * (1.0f - pressProgress_ * 2.0f)), 0, 0, 0));
    shadowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 6.0f));

    SkRRect shadowRect = roundRect;
    shadowRect.offset(0, 3.0f * (1.0f - pressProgress_));

    canvas->drawRRect(shadowRect, shadowPaint);
  }

  //==========================================================================
  // Button body with gradient
  //==========================================================================

  SkPaint bodyPaint;
  bodyPaint.setAntiAlias(true);

  auto baseColour = getStyleColour();

  SkColor topColor = SkColorSetARGB(
      baseColour.getAlpha(), (int)(baseColour.getRed() * 1.2f),
      (int)(baseColour.getGreen() * 1.2f), (int)(baseColour.getBlue() * 1.2f));

  SkColor bottomColor = SkColorSetARGB(
      baseColour.getAlpha(), (int)(baseColour.getRed() * 0.8f),
      (int)(baseColour.getGreen() * 0.8f), (int)(baseColour.getBlue() * 0.8f));

  SkColor colors[] = {topColor, bottomColor};
  SkPoint points[] = {SkPoint::Make(buttonRect.centerX(), buttonRect.top()),
                      SkPoint::Make(buttonRect.centerX(), buttonRect.bottom())};

  sk_sp<SkShader> gradient = SkGradientShader::MakeLinear(
      points, colors, nullptr, 2, SkTileMode::kClamp);

  bodyPaint.setShader(gradient);
  canvas->drawRRect(roundRect, bodyPaint);

  //==========================================================================
  // Inner highlight
  //==========================================================================

  SkPaint highlightPaint;
  highlightPaint.setAntiAlias(true);
  highlightPaint.setColor(SkColorSetARGB(30, 255, 255, 255));

  SkRect highlightRect = buttonRect;
  highlightRect.fBottom = highlightRect.fTop + (highlightRect.height() * 0.3f);

  SkRRect highlightRRect =
      SkRRect::MakeRectXY(highlightRect, CORNER_RADIUS, CORNER_RADIUS / 2.0f);

  canvas->drawRRect(highlightRRect, highlightPaint);

  //==========================================================================
  // Hover glow
  //==========================================================================

  if (hoverProgress_ > 0.01f) {
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(3.0f);

    auto glowColour = baseColour.brighter(0.4f);
    glowPaint.setColor(SkColorSetARGB(
        (int)(HOVER_GLOW_ALPHA * 255 * hoverProgress_), glowColour.getRed(),
        glowColour.getGreen(), glowColour.getBlue()));

    SkRRect glowRect = roundRect;
    glowRect.outset(2.0f, 2.0f);

    canvas->drawRRect(glowRect, glowPaint);
  }

  //==========================================================================
  // Text
  //==========================================================================

  if (buttonText_.isNotEmpty()) {
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorWHITE);

    SkFont font(nullptr, 14.0f);

    SkRect textBounds;
    font.measureText(buttonText_.toRawUTF8(), buttonText_.length(),
                     SkTextEncoding::kUTF8, &textBounds);

    float textX = buttonRect.centerX() - textBounds.width() / 2.0f;
    float textY = buttonRect.centerY() + textBounds.height() / 2.0f;

    canvas->drawString(buttonText_.toRawUTF8(), textX, textY, font, textPaint);
  }

  //==========================================================================
  // Press overlay
  //==========================================================================

  if (pressProgress_ > 0.01f) {
    SkPaint pressOverlay;
    pressOverlay.setAntiAlias(true);
    pressOverlay.setColor(SkColorSetARGB((int)(50 * pressProgress_), 0, 0, 0));

    canvas->drawRRect(roundRect, pressOverlay);
  }
#endif
}

//==============================================================================
// Mouse Events
//==============================================================================

void SkiaButtonComponent_NEW::mouseEnter(const juce::MouseEvent &) {
  isHovered_ = true;
  repaint();
}

void SkiaButtonComponent_NEW::mouseExit(const juce::MouseEvent &) {
  isHovered_ = false;
  isPressed_ = false;
  repaint();
}

void SkiaButtonComponent_NEW::mouseDown(const juce::MouseEvent &) {
  isPressed_ = true;
  repaint();
}

void SkiaButtonComponent_NEW::mouseUp(const juce::MouseEvent &event) {
  bool wasPressed = isPressed_;
  isPressed_ = false;

  if (wasPressed && event.mouseWasClicked() && onClick) {
    onClick();
  }

  repaint();
}

//==============================================================================
// Configuration
//==============================================================================

void SkiaButtonComponent_NEW::setButtonText(const juce::String &text) {
  if (buttonText_ != text) {
    buttonText_ = text;
    repaint();
  }
}

void SkiaButtonComponent_NEW::setStyle(Style style) {
  if (style_ != style) {
    style_ = style;
    repaint();
  }
}

juce::Colour SkiaButtonComponent_NEW::getStyleColour() const {
  switch (style_) {
  case Style::Primary:
    return juce::Colour(0xff0A84FF);
  case Style::Secondary:
    return juce::Colour(0xff3A3A3C);
  case Style::Success:
    return juce::Colour(0xff34C759);
  case Style::Danger:
    return juce::Colour(0xffFF3B30);
  case Style::Warning:
    return juce::Colour(0xffFF9500);
  default:
    return juce::Colour(0xff0A84FF);
  }
}

//==============================================================================
// Animation
//==============================================================================

void SkiaButtonComponent_NEW::timerCallback() { updateAnimations(); }

void SkiaButtonComponent_NEW::updateAnimations() {
  bool needsRepaint = false;

  // Hover animation
  float hoverTarget = isHovered_ ? 1.0f : 0.0f;
  if (std::abs(hoverProgress_ - hoverTarget) > 0.001f) {
    float force = -SPRING_STIFFNESS * (hoverProgress_ - hoverTarget) -
                  SPRING_DAMPING * hoverVelocity_;

    hoverVelocity_ += force * ANIMATION_DT;
    hoverProgress_ += hoverVelocity_ * ANIMATION_DT;
    hoverProgress_ = std::max(0.0f, std::min(1.0f, hoverProgress_));

    needsRepaint = true;
  }

  // Press animation
  float pressTarget = isPressed_ ? 1.0f : 0.0f;
  if (std::abs(pressProgress_ - pressTarget) > 0.001f) {
    float force = -SPRING_STIFFNESS * 2.0f * (pressProgress_ - pressTarget) -
                  SPRING_DAMPING * 1.5f * pressVelocity_;

    pressVelocity_ += force * ANIMATION_DT;
    pressProgress_ += pressVelocity_ * ANIMATION_DT;
    pressProgress_ = std::max(0.0f, std::min(1.0f, pressProgress_));

    needsRepaint = true;
  }

  if (needsRepaint) {
    repaint();
  }
}

} // namespace zenith
