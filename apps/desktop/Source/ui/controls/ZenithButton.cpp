/*
  ==============================================================================

    ZenithButton.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the premium Zenith button.

  ==============================================================================
*/

#include "ZenithButton.h"

#include "../design-system/ZenithDesignSystem.h" // Add design system include
#include "../design-system/ZenithIcons.h" // Add icons include
#include <core/SkBlurTypes.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

ZenithButton::ZenithButton() : text_(""), iconText_("") {
  setWantsKeyboardFocus(true);
}

ZenithButton::ZenithButton(const juce::String &text,
                           std::function<void()> clickHandler)
    : text_(text), iconText_(""), onClick(clickHandler) {
  setWantsKeyboardFocus(true);
}

ZenithButton::~ZenithButton() = default;

void ZenithButton::setButtonText(const juce::String &text) {
  if (text_ != text) {
    text_ = text;
    textDirty_ = true;
    layoutDirty_ = true;
    repaint();
  }
}

void ZenithButton::setButtonStyle(Style style) {
  if (style_ != style) {
    style_ = style;
    repaint();
  }
}

void ZenithButton::setButtonSize(Size size) {
  if (size_ != size) {
    size_ = size;
    layoutDirty_ = true;
    textDirty_ = true;
    repaint();
  }
}

void ZenithButton::setIcon(sk_sp<SkImage> icon) {
  icon_ = icon;
  layoutDirty_ = true;
  repaint();
}

void ZenithButton::setIconPath(const SkPath& path) {
  iconPath_ = path;
  layoutDirty_ = true;
  repaint();
}

void ZenithButton::setIconText(const juce::String &iconText) {
  iconText_ = iconText;
  layoutDirty_ = true;
  repaint();
}

void ZenithButton::setIconPosition(IconPosition pos) {
  if (iconPosition_ != pos) {
    iconPosition_ = pos;
    layoutDirty_ = true;
    repaint();
  }
}

void ZenithButton::setToggleable(bool toggleable) { toggleable_ = toggleable; }

void ZenithButton::setToggleState(bool state, bool sendNotification) {
  if (toggleState_ != state) {
    toggleState_ = state;
    repaint();

    if (sendNotification && onToggle) {
      onToggle(toggleState_);
    }
  }
}

void ZenithButton::setAudioReactive(bool reactive) {
  audioReactive_ = reactive;
}

void ZenithButton::setAudioLevel(float level) {
  audioLevel_ = juce::jlimit(0.0f, 1.0f, level);
  if (audioReactive_) {
    repaint();
  }
}

void ZenithButton::setEnabled(bool enabled) {
  if (isEnabled() != enabled) {
    juce::Component::setEnabled(enabled);
    repaint();
  }
}

void ZenithButton::mouseEnter(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hovered_ = true;
  animateTo("hover", 1.0f, 150);
}

void ZenithButton::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hovered_ = false;
  animateTo("hover", 0.0f, 200);
}

void ZenithButton::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (!isEnabled())
    return;

  pressed_ = true;
  repaint();
}

void ZenithButton::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);

  if (pressed_) {
    pressed_ = false;

    if (isEnabled() && contains(e.position.toInt())) {
      if (toggleable_) {
        setToggleState(!toggleState_, true);
      }

      if (onClick) {
        onClick();
      }
    }

    repaint();
  }
}

void ZenithButton::focusGained(juce::Component::FocusChangeType cause) {
  juce::ignoreUnused(cause);
  focused_ = true;
  repaint();
}

void ZenithButton::focusLost(juce::Component::FocusChangeType cause) {
  juce::ignoreUnused(cause);
  focused_ = false;
  repaint();
}

void ZenithButton::resized() { layoutDirty_ = true; }

float ZenithButton::getButtonHeight() const {
  switch (size_) {
  case Size::Small:
    return 24.0f;
  case Size::Large:
    return 40.0f;
  default:
    return 32.0f; // Medium
  }
}

float ZenithButton::getCornerRadius() const {
  switch (size_) {
  case Size::Small:
    return design::dimensions::RADIUS_SM;
  case Size::Large:
    return design::dimensions::RADIUS_LG;
  default:
    return design::dimensions::RADIUS_SM; // Medium
  }
}

float ZenithButton::getFontSize() const {
  switch (size_) {
  case Size::Small:
    return 11.0f;
  case Size::Large:
    return 15.0f;
  default:
    return 13.0f; // Medium
  }
}

void ZenithButton::drawSkia(SkCanvas *canvas) {
  if (canvas == nullptr)
    return;

  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  SkRRect rrect =
      SkRRect::MakeRectXY(rect, getCornerRadius(), getCornerRadius());

  // Draw layers
  if (hovered_ || pressed_ || (toggleable_ && toggleState_)) {
    drawGlow(canvas, rrect);
  }

  drawBackground(canvas, rrect);
  drawBorder(canvas, rrect);

  // Focus ring
  if (focused_) {
    SkPaint focusPaint;
    focusPaint.setAntiAlias(true);
    focusPaint.setStyle(SkPaint::kStroke_Style);
    focusPaint.setStrokeWidth(2.0f);
    focusPaint.setColor(SkColorSetARGB(150, 0, 200, 255));

    SkRRect focusRect = rrect;
    focusRect.outset(2.0f, 2.0f);
    canvas->drawRRect(focusRect, focusPaint);
  }

  // Layout content
  if (layoutDirty_) {
    calculateLayout();
  }

  // Draw icon
  if (iconPosition_ != IconPosition::Only || iconText_.isNotEmpty()
      || icon_ != nullptr
  ) {
    drawIcon(canvas, iconRect_);
  }

  // Draw text (if not icon-only)
  if (iconPosition_ != IconPosition::Only && text_.isNotEmpty()) {
    drawText(canvas, textRect_);
  }
}

SkColor ZenithButton::getBackgroundColor() const {
  SkColor base;
  float hoverAnim = getAnimatedValue("hover");
  
  uint8_t normalAlpha = 150;
  uint8_t hoverAlpha = 180;
  uint8_t currentAlpha = (uint8_t)(normalAlpha + (hoverAlpha - normalAlpha) * hoverAnim);
  
  if (pressed_) currentAlpha = 200;

  switch (style_) {
  case Style::Primary:
    base = design::colors::CYAN;
    break;
  case Style::Danger:
    base = design::colors::RED;
    break;
  case Style::Warning:
    base = design::colors::AMBER;
    break;
  case Style::Success:
    base = design::colors::GREEN;
    break;
  case Style::Ghost:
    base = design::lighten(design::colors::BG_DARK, 0.1f);
    currentAlpha = (uint8_t)(100 * hoverAnim);
    if (pressed_) currentAlpha = 150;
    break;
  case Style::Secondary:
  default:
    base = design::colors::BG_DARK;
    break;
  }

  if (toggleable_ && toggleState_) {
    base = getGlowColor();
    currentAlpha = 200;
  }

  if (!isEnabled()) {
    currentAlpha = 80;
  }

  return SkColorSetA(base, currentAlpha);
}

SkColor ZenithButton::getTextColor() const {
  if (!isEnabled()) {
    return design::withAlpha(design::colors::TEXT_PRIMARY, 0.4f);
  }

  return design::colors::TEXT_PRIMARY;
}

SkColor ZenithButton::getBorderColor() const {
  if (style_ == Style::Ghost) {
    return hovered_ ? design::withAlpha(design::colors::BORDER_DEFAULT, 0.6f)
                    : design::withAlpha(design::colors::BORDER_DEFAULT, 0.3f);
  }
  return design::colors::BORDER_DEFAULT;
}

SkColor ZenithButton::getGlowColor() const {
  if (isGlowEnabled()) {
    return SkiaComponent::getGlowColor();
  }
  
  switch (style_) {
  case Style::Primary:
    return design::colors::CYAN;
  case Style::Danger:
    return design::colors::RED;
  case Style::Warning:
    return design::colors::AMBER;
  case Style::Success:
    return design::colors::GREEN;
  case Style::Ghost:
    return design::colors::VIOLET;
  case Style::Secondary:
  default:
    return design::colors::VIOLET;
  }
}

void ZenithButton::drawGlow(SkCanvas *canvas, const SkRRect &bounds) {
  SkPaint glowPaint;
  glowPaint.setAntiAlias(true);
  glowPaint.setStyle(SkPaint::kStroke_Style);
  glowPaint.setStrokeWidth(2.0f);

  SkColor glowColor = getGlowColor();
  float glowIntensity = pressed_ ? 8.0f : 6.0f;

  if (audioReactive_) {
    glowIntensity *= (0.5f + audioLevel_ * 0.5f);
  }

  glowPaint.setColor(design::withAlpha(glowColor, 0.6f));
  glowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, glowIntensity));
  canvas->drawRRect(bounds, glowPaint);
}

void ZenithButton::drawBackground(SkCanvas *canvas, const SkRRect &bounds) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);

  // Gradient background
  auto rect = bounds.getBounds();
  SkPoint pts[2] = {{0, rect.top()}, {0, rect.bottom()}};

  SkColor bgColor = getBackgroundColor();
  SkColor colors[2] = {SkColorSetA(bgColor, hovered_ ? 200 : 150),
                       SkColorSetA(bgColor, hovered_ ? 150 : 100)};

  paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                               SkTileMode::kClamp));
  canvas->drawRRect(bounds, paint);
}

void ZenithButton::drawBorder(SkCanvas *canvas, const SkRRect &bounds) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);

  // Gradient border (top-left light, bottom-right dark)
  auto rect = bounds.getBounds();
  SkPoint pts[2] = {{0, 0}, {rect.width(), rect.height()}};
  SkColor colors[2] = {SkColorSetARGB(100, 255, 255, 255),
                       SkColorSetARGB(50, 0, 0, 0)};

  paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                               SkTileMode::kClamp));
  canvas->drawRRect(bounds, paint);
}

void ZenithButton::drawIcon(SkCanvas *canvas, const SkRect &rect) {
  if (iconText_.isNotEmpty()) {
    // Use design system font
    SkFont font =
        design::getSkFont(getFontSize() + 2.0f, design::FontWeight::Bold);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(getTextColor());

    std::string str = iconText_.toStdString();
    float textWidth =
        font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);

    canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                           rect.centerX() - textWidth / 2,
                           rect.centerY() + getFontSize() * 0.35f, font, paint);
  } else if (!iconPath_.isEmpty()) {
     // Use ZenithIcons helper if available, or manual scaling
     // Since we included ZenithIcons.h, we can use drawIconCentered? 
     // ZenithIcons.h functions are in zenith::icons namespace.
     // Let's use it for consistency.
     
     icons::IconStyle style;
     style.color = getTextColor();
     // Active/Pressed state handling
     if (toggleable_ && toggleState_) {
         style.filled = true;
         // Brighten color for active state if appropriate
         style.color = SK_ColorWHITE; 
     }
     
     // Highlight on hover
     if (hovered_ && !toggleState_) {
          style.color = SK_ColorWHITE;
     }

     style.strokeWidth = icons::STROKE_REGULAR;
     
     // Calculate size
     float size = std::min(rect.width(), rect.height());
     
     icons::drawIconCentered(canvas, iconPath_, rect, size, style);
      
  } else if (icon_ != nullptr) {
    SkPaint paint;
    paint.setAntiAlias(true);
    if (!isEnabled()) {
      paint.setAlpha(100);
    }

    // Scale icon to fit rect
    float scale = std::min(rect.width() / icon_->width(),
                           rect.height() / icon_->height());
    float drawWidth = icon_->width() * scale;
    float drawHeight = icon_->height() * scale;

    SkRect destRect = SkRect::MakeXYWH(rect.centerX() - drawWidth / 2,
                                       rect.centerY() - drawHeight / 2,
                                       drawWidth, drawHeight);

    canvas->drawImageRect(icon_, destRect, SkSamplingOptions(), &paint);
  }
}

void ZenithButton::drawText(SkCanvas *canvas, const SkRect &rect) {
  if (text_.isEmpty())
    return;

  // Use design system font
  SkFont font = design::getSkFont(getFontSize(), design::FontWeight::Medium);

  std::string str = text_.toStdString();
  float textWidth =
      font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);

  // Text shadow
  SkPaint shadowPaint;
  shadowPaint.setAntiAlias(true);
  shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));

  float textX = rect.centerX() - textWidth / 2;
  float textY = rect.centerY() + getFontSize() * 0.35f;

  canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                         textX + 1, textY + 1, font, shadowPaint);

  // Text foreground
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(getTextColor());

  canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                         textX, textY, font, textPaint);
}

void ZenithButton::calculateLayout() {
  auto bounds = getLocalBounds().toFloat();
  float padding = 12.0f;
  float iconSize = getFontSize() + 4.0f;
  float spacing = 6.0f;

  bool hasIcon = iconText_.isNotEmpty() || icon_ != nullptr || !iconPath_.isEmpty();
  bool hasText = text_.isNotEmpty() && iconPosition_ != IconPosition::Only;

  if (hasIcon && hasText) {
    // Both icon and text
    if (iconPosition_ == IconPosition::Left) {
      iconRect_ = SkRect::MakeXYWH(padding, (bounds.getHeight() - iconSize) / 2,
                                   iconSize, iconSize);
      textRect_ =
          SkRect::MakeXYWH(padding + iconSize + spacing, 0,
                           bounds.getWidth() - padding * 2 - iconSize - spacing,
                           bounds.getHeight());
    } else {
      textRect_ = SkRect::MakeXYWH(
          padding, 0, bounds.getWidth() - padding * 2 - iconSize - spacing,
          bounds.getHeight());
      iconRect_ = SkRect::MakeXYWH(bounds.getWidth() - padding - iconSize,
                                   (bounds.getHeight() - iconSize) / 2,
                                   iconSize, iconSize);
    }
  } else if (hasIcon) {
    // Icon only - center it
    iconRect_ = SkRect::MakeXYWH((bounds.getWidth() - iconSize) / 2,
                                 (bounds.getHeight() - iconSize) / 2, iconSize,
                                 iconSize);
    textRect_ = SkRect::MakeEmpty();
  } else {
    // Text only - use full width
    iconRect_ = SkRect::MakeEmpty();
    textRect_ = SkRect::MakeXYWH(padding, 0, bounds.getWidth() - padding * 2,
                                 bounds.getHeight());
  }

  layoutDirty_ = false;
}


} // namespace zenith
