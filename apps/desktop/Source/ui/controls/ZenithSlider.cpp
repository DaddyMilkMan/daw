/*
  ==============================================================================

    ZenithSlider.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the premium Zenith slider/fader.

  ==============================================================================
*/

#include "ZenithSlider.h"
<<<<<<< HEAD:apps/desktop/Source/ui/controls/ZenithSlider.cpp
#include "ui/design-system/ZenithDesignSystem.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkBlurTypes.h>
#include <core/SkRRect.h>
#include <core/SkMaskFilter.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

ZenithSlider::ZenithSlider() : ZenithControl("") {
  accentColor_ = design::colors::MAGENTA;
=======
#include "../design-system/ZenithDesignSystem.h" // Use design tokens

namespace zenith {

ZenithSlider::ZenithSlider(Orientation orientation)
    : orientation_(orientation) {
  startTimerHz(60); // 60fps animation
>>>>>>> origin/refactor/header-consolidation:apps/desktop/Source/ui/widgets/ZenithSlider.cpp
}

ZenithSlider::~ZenithSlider() { stopTimer(); }

//==============================================================================
// Value control
//==============================================================================

void ZenithSlider::setValue(float newValue, bool sendNotification) {
  targetValue_ = juce::jlimit(0.0f, 1.0f, newValue);

  // If not animating (e.g. direct set), update immediately for responsiveness
  if (!isTimerRunning()) {
    value_ = targetValue_;
    repaint();
  }

  if (sendNotification && onValueChange) {
    onValueChange(getDisplayValue());
  }
}

void ZenithSlider::setRange(float min, float max, float defaultValue) {
  minValue_ = min;
  maxValue_ = max;
  defaultValue_ = defaultValue;

  // Update internal normalized value based on new range if needed,
  // but usually value_ stays 0-1.
  // If we wanted to preserve the *actual* value, we'd need to know what it was
  // before. For now, let's just clamp.
}

float ZenithSlider::getDisplayValue() const {
  return minValue_ + value_ * (maxValue_ - minValue_);
}

//==============================================================================
// Component interface
//==============================================================================

void ZenithSlider::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  if (orientation_ == Vertical) {
    drawVerticalSlider(g, bounds);
  } else {
    drawHorizontalSlider(g, bounds);
  }
}

void ZenithSlider::resized() {
  // Layout changes if needed
}

void ZenithSlider::timerCallback() {
  bool needsRepaint = false;

  // Animate value
  if (std::abs(value_ - targetValue_) > 0.001f) {
    value_ += (targetValue_ - value_) * 0.2f; // Smooth spring
    needsRepaint = true;
  } else {
    value_ = targetValue_;
  }

  // Animate hover
  float targetHover = isHovered_ ? 1.0f : 0.0f;
  if (std::abs(hoverAnimation_ - targetHover) > 0.001f) {
    hoverAnimation_ += (targetHover - hoverAnimation_) * 0.1f;
    needsRepaint = true;
  }

  // Animate drag
  float targetDrag = isDragging_ ? 1.0f : 0.0f;
  if (std::abs(dragAnimation_ - targetDrag) > 0.001f) {
    dragAnimation_ += (targetDrag - dragAnimation_) * 0.2f;
    needsRepaint = true;
  }

  if (needsRepaint) {
    repaint();
  }
}

//==============================================================================
// Drawing methods
//==============================================================================

void ZenithSlider::drawVerticalSlider(juce::Graphics &g,
                                      const juce::Rectangle<float> &bounds) {
  // Track
  float trackWidth = 4.0f;
  auto trackBounds =
      bounds.withWidth(trackWidth)
          .withCentre({bounds.getCentreX(), bounds.getCentreY()});
  // Shorten track slightly for padding
  trackBounds.reduce(0, 10);

  drawTrack(g, trackBounds);

  // Thumb
  float thumbHeight = 12.0f;
  float thumbWidth = 24.0f;
  float thumbY = trackBounds.getBottom() - (trackBounds.getHeight() * value_) -
                 (thumbHeight / 2.0f);

  juce::Rectangle<float> thumbBounds(bounds.getCentreX() - thumbWidth / 2.0f,
                                     thumbY, thumbWidth, thumbHeight);

  drawThumb(g, thumbBounds);

  // Label if exists
  if (label_.isNotEmpty()) {
    drawLabel(g, bounds.removeFromBottom(20));
  }
}

void ZenithSlider::drawHorizontalSlider(juce::Graphics &g,
                                        const juce::Rectangle<float> &bounds) {
  // Track
  float trackHeight = 4.0f;
  auto trackBounds =
      bounds.withHeight(trackHeight)
          .withCentre({bounds.getCentreX(), bounds.getCentreY()});
  trackBounds.reduce(10, 0);

  drawTrack(g, trackBounds);

  // Thumb
  float thumbWidth = 12.0f;
  float thumbHeight = 24.0f;
  float thumbX = trackBounds.getX() + (trackBounds.getWidth() * value_) -
                 (thumbWidth / 2.0f);

  juce::Rectangle<float> thumbBounds(thumbX,
                                     bounds.getCentreY() - thumbHeight / 2.0f,
                                     thumbWidth, thumbHeight);

  drawThumb(g, thumbBounds);

  if (label_.isNotEmpty()) {
    drawLabel(g, bounds.removeFromLeft(30)); // Simple label placement
  }
}

void ZenithSlider::drawTrack(juce::Graphics &g,
                             const juce::Rectangle<float> &trackBounds) {
  g.setColour(juce::Colour(0xFF25252D)); // BG_MEDIUM
  g.fillRoundedRectangle(trackBounds,
                         trackBounds.getWidth() < trackBounds.getHeight()
                             ? trackBounds.getWidth() / 2.0f
                             : trackBounds.getHeight() / 2.0f);

  // Active fill (optional - simple for now)
}

void ZenithSlider::drawThumb(juce::Graphics &g,
                             const juce::Rectangle<float> &thumbBounds) {
  // Shadow
  g.setColour(juce::Colours::black.withAlpha(0.3f));
  g.fillRoundedRectangle(thumbBounds.translated(0, 2), 2.0f);

  // Body
  g.setColour(juce::Colour(0xFFF2F2F7)); // TEXT_PRIMARY
  if (isHovered_)
    g.setColour(juce::Colour(0xFFFFFFFF));
  if (isDragging_)
    g.setColour(juce::Colour(0xFF00F0FF)); // CYAN

  g.fillRoundedRectangle(thumbBounds, 2.0f);
}

void ZenithSlider::drawLabel(juce::Graphics &g,
                             const juce::Rectangle<float> &bounds) {
  g.setColour(juce::Colour(0xFFA1A1AA)); // TEXT_SECONDARY
  g.setFont(12.0f);
  g.drawText(label_, bounds, juce::Justification::centred, false);
}

//==============================================================================
// Mouse interaction
//==============================================================================

void ZenithSlider::mouseDown(const juce::MouseEvent &event) {
  if (!isEnabled())
    return;

  isDragging_ = true;
  dragStartPos_ = event.getPosition();
  dragStartValue_ = value_;

  repaint();
}

void ZenithSlider::mouseDrag(const juce::MouseEvent &event) {
  if (!isDragging_ || !isEnabled())
    return;

  float delta = 0.0f;
  float sensitivity = 0.005f; // pixels to value

  if (orientation_ == Vertical) {
    delta =
        (dragStartPos_.y - event.position.y) * sensitivity; // Up is positive
  } else {
    delta = (event.position.x - dragStartPos_.x) * sensitivity;
  }

  // Apply modifier for fine control
  if (event.mods.isShiftDown()) {
    delta *= 0.1f;
  }

  float newValue = dragStartValue_ + delta;
  setValue(newValue, true);
}

void ZenithSlider::mouseUp(const juce::MouseEvent &) {
  isDragging_ = false;
  repaint();
}

void ZenithSlider::mouseEnter(const juce::MouseEvent &) {
  isHovered_ = true;
  repaint();
}

void ZenithSlider::mouseExit(const juce::MouseEvent &) {
  isHovered_ = false;
  repaint();
}

void ZenithSlider::mouseDoubleClick(const juce::MouseEvent &) {
  if (!isEnabled())
    return;

  // Reset to default (normalized)
  // We need to map default value (actual) to normalized (0-1)
  if (maxValue_ > minValue_) {
    float normDefault = (defaultValue_ - minValue_) / (maxValue_ - minValue_);
    setValue(normDefault, true);
  }
}

<<<<<<< HEAD:apps/desktop/Source/ui/controls/ZenithSlider.cpp
void ZenithSlider::drawTrack(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(design::colors::BG_DARKER); // Dark background

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
  paint.setColor(design::colors::BORDER_DEFAULT);
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

=======
>>>>>>> origin/refactor/header-consolidation:apps/desktop/Source/ui/widgets/ZenithSlider.cpp
} // namespace zenith
