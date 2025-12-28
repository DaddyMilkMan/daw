/*
  ==============================================================================

    ZenithToggle.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the premium toggle switch.

  ==============================================================================
*/

#include "ZenithToggle.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkPath.h>
#endif

namespace zenith {

ZenithToggle::ZenithToggle() : label_("") { setWantsKeyboardFocus(true); }

ZenithToggle::ZenithToggle(const juce::String &label) : label_(label) {
  setWantsKeyboardFocus(true);
}

void ZenithToggle::setToggleState(bool state, bool sendNotification) {
  if (toggleState_ != state) {
    toggleState_ = state;
    toggleState_ = state;
    // animationProgress_ = state ? 1.0f : 0.0f;
    animateTo("toggle", state ? 1.0f : 0.0f, 150.0); // 150ms smooth transition
    repaint();

    if (sendNotification && onToggle) {
      onToggle(toggleState_);
    }
  }
}

void ZenithToggle::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  pressed_ = true;
  repaint();
}

void ZenithToggle::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);

  if (pressed_ && contains(e.position.toInt())) {
    setToggleState(!toggleState_, true);

    if (onClick) {
      onClick();
    }
  }

  pressed_ = false;
  repaint();
}

void ZenithToggle::mouseEnter(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hovered_ = true;
  repaint();
}

void ZenithToggle::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hovered_ = false;
  repaint();
}

void ZenithToggle::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr)
    return;

  switch (style_) {
  case Style::Switch:
    drawSwitch(canvas);
    break;
  case Style::Checkbox:
    drawCheckbox(canvas);
    break;
  case Style::Radio:
    drawRadio(canvas);
    break;
  }

  if (label_.isNotEmpty()) {
    drawLabel(canvas);
  }
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

void ZenithToggle::drawSwitch(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  float trackWidth = 40.0f;
  float trackHeight = 20.0f;
  float knobRadius = 8.0f;

  float trackX = labelOnRight_ ? 4.0f : bounds.getWidth() - trackWidth - 4.0f;
  float trackY = (bounds.getHeight() - trackHeight) / 2.0f;

  SkRect trackRect = SkRect::MakeXYWH(trackX, trackY, trackWidth, trackHeight);
  SkRRect trackRRect =
      SkRRect::MakeRectXY(trackRect, trackHeight / 2, trackHeight / 2);

  SkPaint paint;
  paint.setAntiAlias(true);

  // Track background
  // Track background
  float progress = getAnimatedValue("toggle");
  SkColor trackColor = design::interpolateColor(inactiveColor_, activeColor_, progress);

  if (!toggleState_ && hovered_) {
    trackColor = SkColorSetRGB(70, 70, 80);
  }

  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(trackColor);
  canvas->drawRRect(trackRRect, paint);

  // Track glow when active
  if (toggleState_) {
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
    paint.setColor(SkColorSetA(activeColor_, 100));
    canvas->drawRRect(trackRRect, paint);
    paint.setMaskFilter(nullptr);
  }

  // Knob position
  // Knob position
  float progValue = getAnimatedValue("toggle");
  float startX = knobRadius + 4.0f;
  float endX = trackWidth - knobRadius - 4.0f;
  float knobX = trackX + startX + (endX - startX) * progValue;
  float knobY = trackY + trackHeight / 2.0f;

  // Knob shadow
  SkPaint shadowPaint;
  shadowPaint.setAntiAlias(true);
  shadowPaint.setColor(SkColorSetARGB(80, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
  canvas->drawCircle(knobX, knobY + 1, knobRadius, shadowPaint);

  // Knob
  paint.setColor(SK_ColorWHITE);
  canvas->drawCircle(knobX, knobY, knobRadius, paint);

  // Knob inner highlight
  paint.setColor(SkColorSetARGB(30, 0, 0, 0));
  canvas->drawCircle(knobX, knobY + 1, knobRadius - 2, paint);
}

void ZenithToggle::drawCheckbox(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  float boxSize = 18.0f;
  float boxX = labelOnRight_ ? 4.0f : bounds.getWidth() - boxSize - 4.0f;
  float boxY = (bounds.getHeight() - boxSize) / 2.0f;

  SkRect boxRect = SkRect::MakeXYWH(boxX, boxY, boxSize, boxSize);
  SkRRect boxRRect = SkRRect::MakeRectXY(boxRect, 3.0f, 3.0f);

  SkPaint paint;
  paint.setAntiAlias(true);

  // Box background
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(toggleState_ ? activeColor_ : inactiveColor_);
  canvas->drawRRect(boxRRect, paint);

  // Box border
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(hovered_ ? SkColorSetARGB(150, 255, 255, 255)
                          : SkColorSetARGB(80, 255, 255, 255));
  canvas->drawRRect(boxRRect, paint);

  // Checkmark
  if (toggleState_) {
    SkPath checkPath;
    float cx = boxX + boxSize / 2;
    float cy = boxY + boxSize / 2;

    checkPath.moveTo(cx - 5, cy);
    checkPath.lineTo(cx - 1, cy + 4);
    checkPath.lineTo(cx + 5, cy - 4);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setStrokeCap(SkPaint::kRound_Cap);
    paint.setStrokeJoin(SkPaint::kRound_Join);
    paint.setColor(SK_ColorWHITE);
    canvas->drawPath(checkPath, paint);
  }
}

void ZenithToggle::drawRadio(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  float radioSize = 18.0f;
  float radioX = labelOnRight_ ? 4.0f + radioSize / 2
                               : bounds.getWidth() - radioSize / 2 - 4.0f;
  float radioY = bounds.getHeight() / 2.0f;

  SkPaint paint;
  paint.setAntiAlias(true);

  // Outer circle
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(2.0f);
  paint.setColor(toggleState_ ? activeColor_
                              : (hovered_ ? SkColorSetRGB(100, 100, 110)
                                          : inactiveColor_));
  canvas->drawCircle(radioX, radioY, radioSize / 2, paint);

  // Inner dot when selected
  if (toggleState_) {
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(activeColor_);
    canvas->drawCircle(radioX, radioY, radioSize / 4, paint);

    // Glow
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    paint.setColor(SkColorSetA(activeColor_, 100));
    canvas->drawCircle(radioX, radioY, radioSize / 4, paint);
  }
}

void ZenithToggle::drawLabel(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  SkFont font;
  font.setSize(12.0f);
  font.setSubpixel(true);

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(SkColorSetARGB(200, 220, 220, 230));

  std::string str = label_.toStdString();
  float textWidth =
      font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);

  float textX, textY;
  textY = bounds.getHeight() / 2 + 4.0f;

  if (labelOnRight_) {
    textX = 52.0f; // After the switch
  } else {
    textX = bounds.getWidth() - textWidth - 52.0f; // Before the switch
  }

  canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                         textX, textY, font, paint);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
