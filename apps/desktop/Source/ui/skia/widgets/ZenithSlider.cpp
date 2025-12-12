/*
  ==============================================================================

    ZenithSlider.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the premium Zenith slider/fader.

  ==============================================================================
*/

#include "ZenithSlider.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkBlurTypes.h>
#include <core/SkRRect.h>
#endif

namespace zenith {

ZenithSlider::ZenithSlider() : ZenithControl("") {
  accentColor_ = SkColorSetRGB(255, 0, 255);
}

ZenithSlider::ZenithSlider(const juce::String &name, SkColor color)
    : ZenithControl(name) {
  accentColor_ = color;
}

void ZenithSlider::mouseDrag(const juce::MouseEvent &e) {
  if (!isDragging_ || !isEnabled())
    return;

  auto bounds = getLocalBounds().toFloat();
  float trackLength, dragPos, dragStart;

  if (orientation_ == Orientation::Vertical) {
    float trackStart = bounds.getHeight() * marginStart_;
    trackLength = bounds.getHeight() * (1.0f - marginStart_ - marginEnd_);
    dragPos = e.position.y - trackStart;
    dragStart = dragStartPos_.y - trackStart;
  } else {
    float trackStart = bounds.getWidth() * marginStart_;
    trackLength = bounds.getWidth() * (1.0f - marginStart_ - marginEnd_);
    dragPos = e.position.x - trackStart;
    dragStart = dragStartPos_.x - trackStart;
  }

  // Calculate sensitivity
  float sensitivity = isFineMode_ ? fineControlMultiplier_ : 1.0f;
  float dragDelta = (dragPos - dragStart) * sensitivity;

  // For vertical, invert (up = increase)
  if (orientation_ == Orientation::Vertical) {
    dragDelta = -dragDelta;
  }

  float startNorm =
      (dragStartValue_ - range_.start) / (range_.end - range_.start);
  float newNorm = juce::jlimit(0.0f, 1.0f, startNorm + dragDelta / trackLength);
  float newValue = range_.start + newNorm * (range_.end - range_.start);

  setValue(newValue, true);

  if (onValueChange) {
    onValueChange();
  }
}

void ZenithSlider::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr)
    return;

  float handlePos = getHandlePosition();

  // 1. Track
  drawTrack(canvas);

  // 2. Fill bar (optional)
  if (showFillBar_) {
    drawFillBar(canvas, handlePos);
  }

  // 3. Handle
  drawHandle(canvas, handlePos);

  // 4. Value tooltip
  if ((isHovered_ && showValueOnHover_) ||
      (isDragging_ && showValueWhileDragging_)) {
    drawValueTooltip(canvas, handlePos);
  }
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

float ZenithSlider::getHandlePosition() const {
  auto bounds = getLocalBounds().toFloat();
  float normValue = getNormalizedValue();

  if (orientation_ == Orientation::Vertical) {
    float trackStart = bounds.getHeight() * marginStart_;
    float trackLength = bounds.getHeight() * (1.0f - marginStart_ - marginEnd_);
    // For vertical, 0 = bottom, 1 = top
    return trackStart + trackLength * (1.0f - normValue);
  } else {
    float trackStart = bounds.getWidth() * marginStart_;
    float trackLength = bounds.getWidth() * (1.0f - marginStart_ - marginEnd_);
    return trackStart + trackLength * normValue;
  }
}

void ZenithSlider::drawTrack(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(SkColorSetARGB(255, 15, 15, 20)); // Dark background

  SkRect trackRect;
  float cornerRadius = 2.0f;

  if (orientation_ == Orientation::Vertical) {
    float cx = w / 2.0f;
    float trackStart = h * marginStart_;
    float trackEnd = h * (1.0f - marginEnd_);
    trackRect = SkRect::MakeXYWH(cx - trackWidth_ / 2, trackStart, trackWidth_,
                                 trackEnd - trackStart);
  } else {
    float cy = h / 2.0f;
    float trackStart = w * marginStart_;
    float trackEnd = w * (1.0f - marginEnd_);
    trackRect = SkRect::MakeXYWH(trackStart, cy - trackWidth_ / 2,
                                 trackEnd - trackStart, trackWidth_);
  }

  SkRRect trackRRect =
      SkRRect::MakeRectXY(trackRect, cornerRadius, cornerRadius);
  canvas->drawRRect(trackRRect, paint);

  // Subtle highlight edge
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(SkColorSetARGB(50, 255, 255, 255));
  canvas->drawRRect(trackRRect, paint);
}

void ZenithSlider::drawFillBar(SkCanvas *canvas, float handlePos) {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(SkColorSetA(accentColor_, 180));

  SkRect fillRect;
  float cornerRadius = 2.0f;

  if (orientation_ == Orientation::Vertical) {
    float cx = w / 2.0f;
    float trackEnd = h * (1.0f - marginEnd_);

    if (bipolar_) {
      float center =
          h * (marginStart_ + (1.0f - marginStart_ - marginEnd_) * 0.5f);
      if (handlePos < center) {
        fillRect = SkRect::MakeXYWH(cx - trackWidth_ / 2, handlePos,
                                    trackWidth_, center - handlePos);
      } else {
        fillRect = SkRect::MakeXYWH(cx - trackWidth_ / 2, center, trackWidth_,
                                    handlePos - center);
      }
    } else {
      fillRect = SkRect::MakeXYWH(cx - trackWidth_ / 2, handlePos, trackWidth_,
                                  trackEnd - handlePos);
    }
  } else {
    float cy = h / 2.0f;
    float trackStart = w * marginStart_;

    if (bipolar_) {
      float center =
          w * (marginStart_ + (1.0f - marginStart_ - marginEnd_) * 0.5f);
      if (handlePos < center) {
        fillRect = SkRect::MakeXYWH(handlePos, cy - trackWidth_ / 2,
                                    center - handlePos, trackWidth_);
      } else {
        fillRect = SkRect::MakeXYWH(center, cy - trackWidth_ / 2,
                                    handlePos - center, trackWidth_);
      }
    } else {
      fillRect = SkRect::MakeXYWH(trackStart, cy - trackWidth_ / 2,
                                  handlePos - trackStart, trackWidth_);
    }
  }

  if (fillRect.width() > 0 && fillRect.height() > 0) {
    SkRRect fillRRect =
        SkRRect::MakeRectXY(fillRect, cornerRadius, cornerRadius);
    canvas->drawRRect(fillRRect, paint);
  }
}

void ZenithSlider::drawHandle(SkCanvas *canvas, float handlePos) {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  SkRect handleRect;

  if (orientation_ == Orientation::Vertical) {
    float cx = w / 2.0f;
    handleRect =
        SkRect::MakeXYWH(cx - handleWidth_ / 2, handlePos - handleHeight_ / 2,
                         handleWidth_, handleHeight_);
  } else {
    float cy = h / 2.0f;
    handleRect =
        SkRect::MakeXYWH(handlePos - handleWidth_ / 2, cy - handleHeight_ / 2,
                         handleWidth_, handleHeight_);
  }

  SkRRect handleRRect = SkRRect::MakeRectXY(handleRect, 3.0f, 3.0f);

  // Shadow
  SkPaint shadowPaint;
  shadowPaint.setAntiAlias(true);
  shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
  canvas->drawRRect(handleRRect.makeOffset(0, 2), shadowPaint);

  // Handle gradient (metallic)
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);

  SkPoint pts[2] = {{handleRect.left(), handleRect.top()},
                    {handleRect.left(), handleRect.bottom()}};
  SkColor colors[2] = {SkColorSetRGB(60, 60, 70), SkColorSetRGB(30, 30, 35)};
  paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                               SkTileMode::kClamp));
  canvas->drawRRect(handleRRect, paint);
  paint.setShader(nullptr);

  // Hover glow
  if (isHovered_) {
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setColor(accentColor_);
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    canvas->drawRRect(handleRRect, paint);
    paint.setMaskFilter(nullptr);
  }

  // Grip line
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(SkColorSetARGB(100, 255, 255, 255));

  if (orientation_ == Orientation::Vertical) {
    float cy = handleRect.centerY();
    canvas->drawLine(handleRect.centerX() - 6, cy, handleRect.centerX() + 6, cy,
                     paint);
  } else {
    float cx = handleRect.centerX();
    canvas->drawLine(cx, handleRect.centerY() - 4, cx, handleRect.centerY() + 4,
                     paint);
  }
}

void ZenithSlider::drawValueTooltip(SkCanvas *canvas, float handlePos) {
  auto bounds = getLocalBounds().toFloat();
  juce::String valueText = getValueAsText();
  std::string str = valueText.toStdString();

  SkFont font;
  font.setSize(10.0f);
  font.setSubpixel(true);

  float textWidth =
      font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);

  float tooltipX, tooltipY;
  float padding = 4.0f;

  if (orientation_ == Orientation::Vertical) {
    tooltipX = bounds.getWidth() / 2 + handleWidth_ / 2 + 8.0f;
    tooltipY = handlePos + 4.0f;
  } else {
    tooltipX = handlePos - textWidth / 2;
    tooltipY = bounds.getHeight() / 2 - handleHeight_ / 2 - 12.0f;
  }

  // Background
  SkRect bgRect = SkRect::MakeXYWH(tooltipX - padding, tooltipY - 10.0f,
                                   textWidth + padding * 2, 14.0f);
  SkRRect bgRRect = SkRRect::MakeRectXY(bgRect, 3.0f, 3.0f);

  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(SkColorSetARGB(200, 20, 20, 25));
  canvas->drawRRect(bgRRect, bgPaint);

  // Border
  bgPaint.setStyle(SkPaint::kStroke_Style);
  bgPaint.setStrokeWidth(1.0f);
  bgPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
  canvas->drawRRect(bgRRect, bgPaint);

  // Text
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(SK_ColorWHITE);
  canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                         tooltipX, tooltipY, font, textPaint);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
