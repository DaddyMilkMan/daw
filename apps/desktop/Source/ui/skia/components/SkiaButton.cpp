/*
  ==============================================================================

    SkiaButton.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based button implementation

  ==============================================================================
*/

#include "SkiaButton.h"
#include "../ZenithDesignSystem.h"

namespace zenith {

SkiaButton::SkiaButton(const juce::String &buttonText) {
  setName(buttonText);
  buttonText_ = buttonText;
  updateColorsForStyle();
}

SkiaButton::~SkiaButton() { stopAllAnimations(); }

void SkiaButton::setButtonText(const juce::String &text) {
  if (buttonText_ != text) {
    buttonText_ = text;
    markDirty();
  }
}

void SkiaButton::setIcon(const juce::String &iconText) {
  if (iconText_ != iconText) {
    iconText_ = iconText;
    markDirty();
  }
}

void SkiaButton::setButtonStyle(Style style) {
  if (style_ != style) {
    style_ = style;
    updateColorsForStyle();
    markDirty();
  }
}

void SkiaButton::setButtonSize(Size size) {
  if (size_ != size) {
    size_ = size;
    markDirty();
  }
}

void SkiaButton::setToggleable(bool toggleable) { toggleable_ = toggleable; }

void SkiaButton::setToggleState(bool state, bool sendNotification) {
  if (toggleState_ != state) {
    toggleState_ = state;
    markDirty();

    if (sendNotification && onToggle) {
      onToggle(state);
    }
  }
}

void SkiaButton::setEnabled(bool enabled) {
  SkiaComponent::setEnabled(enabled);
  markDirty();
}

void SkiaButton::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  auto buttonRect = getButtonRect();

  // Determine current colors based on state
  SkColor currentBgColor = bgColorNormal_;

  if (!isEnabled()) {
    currentBgColor = design::withAlpha(bgColorNormal_, 0.5f);
  } else if (isPressed_) {
    currentBgColor = bgColorPressed_;
  } else if (isHovered()) {
    currentBgColor = bgColorHover_;
  }

  // Apply toggle state color
  if (toggleable_ && toggleState_) {
    currentBgColor = design::lighten(currentBgColor, 0.2f);
  }

  // Draw button background with rounded corners
  SkRRect rrect;
  rrect.setRectXY(buttonRect, getCornerRadius(), getCornerRadius());

  SkPaint bgPaint;
  bgPaint.setColor(currentBgColor);
  bgPaint.setAntiAlias(true);
  canvas->drawRRect(rrect, bgPaint);

  // Draw border
  SkPaint borderPaint;
  borderPaint.setColor(borderColor_);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  canvas->drawRRect(rrect, borderPaint);

  // Draw text and/or icon
  if (!buttonText_.isEmpty() || !iconText_.isEmpty()) {
    SkPaint textPaint;
    textPaint.setColor(textColor_);
    textPaint.setAntiAlias(true);

    SkFont font;
    font.setSize(getButtonHeight() * 0.4f);

    // Calculate text position
    float textY = buttonRect.centerY() + font.getSize() * 0.3f;

    if (!iconText_.isEmpty() && !buttonText_.isEmpty()) {
      // Draw both icon and text
      float iconWidth = font.measureText(
          iconText_.toRawUTF8(), iconText_.length(), SkTextEncoding::kUTF8);
      float spacing = 8.0f;

      float totalWidth =
          iconWidth + spacing +
          font.measureText(buttonText_.toRawUTF8(), buttonText_.length(),
                           SkTextEncoding::kUTF8);
      float startX = buttonRect.centerX() - totalWidth * 0.5f;

      // Draw icon
      canvas->drawString(iconText_.toRawUTF8(), startX, textY, font, textPaint);

      // Draw text
      canvas->drawString(buttonText_.toRawUTF8(), startX + iconWidth + spacing,
                         textY, font, textPaint);
    } else if (!iconText_.isEmpty()) {
      // Draw only icon
      float textWidth = font.measureText(
          iconText_.toRawUTF8(), iconText_.length(), SkTextEncoding::kUTF8);
      float x = buttonRect.centerX() - textWidth * 0.5f;
      canvas->drawString(iconText_.toRawUTF8(), x, textY, font, textPaint);
    } else {
      // Draw only text
      float textWidth = font.measureText(
          buttonText_.toRawUTF8(), buttonText_.length(), SkTextEncoding::kUTF8);
      float x = buttonRect.centerX() - textWidth * 0.5f;
      canvas->drawString(buttonText_.toRawUTF8(), x, textY, font, textPaint);
    }
  }

  // Draw glow effect if hovered and enabled
  if (isHovered() && isEnabled() && !isPressed_) {
    SkPaint glowPaint;
    glowPaint.setColor(design::colors::CYAN);
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur((SkBlurStyle)0, 4.0f));
    glowPaint.setAlpha(64); // 25% opacity

    // Draw glow slightly larger than button
    SkRRect glowRRect;
    SkRect glowRect = buttonRect.makeOutset(2.0f, 2.0f);
    glowRRect.setRectXY(glowRect, getCornerRadius() + 1.0f,
                        getCornerRadius() + 1.0f);

    canvas->drawRRect(glowRRect, glowPaint);
  }
}

void SkiaButton::resized() {
  // Layout logic can be added here if needed
}

void SkiaButton::mouseEnter(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  setHovered(true);
  animateHover(true);
}

void SkiaButton::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  setHovered(false);
  animateHover(false);
}

void SkiaButton::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (!isEnabled())
    return;

  isPressed_ = true;
  animatePress(true);
}

void SkiaButton::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (!isEnabled())
    return;

  bool wasPressed = isPressed_;
  isPressed_ = false;
  animatePress(false);

  // Check if mouse is still over button
  if (wasPressed && getLocalBounds().contains(e.position.toInt())) {
    if (toggleable_) {
      setToggleState(!toggleState_, true);
    }

    if (onClick) {
      onClick();
    }
  }
}

void SkiaButton::updateColorsForStyle() {
  switch (style_) {
  case Style::Primary:
    bgColorNormal_ = design::colors::BG_DARK;
    bgColorHover_ = design::lighten(design::colors::BG_DARK, 0.1f);
    bgColorPressed_ = design::darken(design::colors::BG_DARK, 0.1f);
    textColor_ = design::colors::TEXT_PRIMARY;
    borderColor_ = design::colors::CYAN;
    break;

  case Style::Secondary:
    bgColorNormal_ = design::colors::BG_DARKER;
    bgColorHover_ = design::lighten(design::colors::BG_DARKER, 0.1f);
    bgColorPressed_ = design::darken(design::colors::BG_DARKER, 0.1f);
    textColor_ = design::colors::TEXT_PRIMARY;
    borderColor_ = design::colors::BORDER_DEFAULT;
    break;

  case Style::Danger:
    bgColorNormal_ = design::colors::BG_DARKER;
    bgColorHover_ = design::lighten(design::colors::BG_DARKER, 0.1f);
    bgColorPressed_ = design::darken(design::colors::BG_DARKER, 0.1f);
    textColor_ = design::colors::TEXT_PRIMARY;
    borderColor_ = design::colors::RED;
    break;

  case Style::Warning:
    bgColorNormal_ = design::colors::BG_DARKER;
    bgColorHover_ = design::lighten(design::colors::BG_DARKER, 0.1f);
    bgColorPressed_ = design::darken(design::colors::BG_DARKER, 0.1f);
    textColor_ = design::colors::TEXT_PRIMARY;
    borderColor_ = design::colors::AMBER;
    break;

  case Style::Success:
    bgColorNormal_ = design::colors::BG_DARKER;
    bgColorHover_ = design::lighten(design::colors::BG_DARKER, 0.1f);
    bgColorPressed_ = design::darken(design::colors::BG_DARKER, 0.1f);
    textColor_ = design::colors::TEXT_PRIMARY;
    borderColor_ = design::colors::NEON_GREEN;
    break;

  case Style::Ghost:
    bgColorNormal_ = SkColorSetARGB(0, 0, 0, 0); // Transparent
    bgColorHover_ = design::withAlpha(design::colors::BG_LIGHT, 0.2f);
    bgColorPressed_ = design::withAlpha(design::colors::BG_LIGHT, 0.3f);
    textColor_ = design::colors::TEXT_PRIMARY;
    borderColor_ = design::colors::BORDER_DEFAULT;
    break;
  }
}

SkRect SkiaButton::getButtonRect() const {
  auto bounds = getLocalBounds().toFloat();
  float margin = 2.0f; // Small margin for border
  return SkRect::MakeXYWH(margin, margin, bounds.getWidth() - margin * 2,
                          bounds.getHeight() - margin * 2);
}

float SkiaButton::getButtonHeight() const {
  switch (size_) {
  case Size::Small:
    return 24.0f;
  case Size::Medium:
    return 32.0f;
  case Size::Large:
    return 40.0f;
  }
  return 32.0f; // Default
}

float SkiaButton::getCornerRadius() const {
  switch (size_) {
  case Size::Small:
    return 2.0f;
  case Size::Medium:
    return 4.0f;
  case Size::Large:
    return 6.0f;
  }
  return 4.0f; // Default
}

void SkiaButton::animateHover(bool hover) {
  float target = hover ? 1.0f : 0.0f;
  animateTo("hover", target, design::animation::DURATION_FAST);
}

void SkiaButton::animatePress(bool press) {
  float target = press ? 1.0f : 0.0f;
  animateTo("press", target, design::animation::DURATION_FAST);
}

} // namespace zenith