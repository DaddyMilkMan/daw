/*
  ==============================================================================

    SkiaButton.cpp
    Created: 2025-11-30
    Authors: Kenji Nakamura (lead), Leo Rossi, Diego Martinez, Isabella Moretti

    Implementation of the beautiful, glowing, animated button.
  ==============================================================================
*/

#include "SkiaButton.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <core/SkFont.h>
#include <core/SkColor.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <core/SkBlurTypes.h> // Explicitly include


using namespace zenith;

// ============================================================================

SkiaButton::SkiaButton(const juce::String& text) : text_(text) {
    // Default size
    juce::Component::setSize(static_cast<int>(design::dimensions::BUTTON_HEIGHT * 3), 
                             static_cast<int>(design::dimensions::BUTTON_HEIGHT));
    setSize(Size::Medium);
    
    // Accessibility
    setDescription(text.isEmpty() ? "Button" : text);
    setWantsKeyboardFocus(true);
}

SkiaButton::~SkiaButton() {
}

// ============================================================================
// APPEARANCE SETTERS
// ============================================================================

void SkiaButton::setStyle(Style style) {
    if (style_ != style) {
        style_ = style;
        
        // Update glow based on style
        setGlowEnabled(style == Style::Primary || style == Style::Danger);
        
        invalidateColors();
        
        if (isVisible()) {
            animateColorChange();
        }
        
        markDirty();
    }
}

void SkiaButton::setSize(Size size) {
    if (size_ != size) {
        size_ = size;
        layoutDirty_ = true;
        
        // Update component height
        float height = 0;
        switch (size) {
            case Size::Small:  height = design::dimensions::BUTTON_HEIGHT_SM; break;
            case Size::Medium: height = design::dimensions::BUTTON_HEIGHT; break;
            case Size::Large:  height = design::dimensions::BUTTON_HEIGHT_LG; break;
        }
        juce::Component::setSize(getWidth(), static_cast<int>(height));
        
        markDirty();
    }
}

void SkiaButton::setText(const juce::String& text) {
    if (text_ != text) {
        text_ = text;
        textDirty_ = true;
        layoutDirty_ = true;
        markDirty();
    }
}

void SkiaButton::setIcon(sk_sp<SkImage> icon) {
    icon_ = icon;
    layoutDirty_ = true;
    markDirty();
}

void SkiaButton::setIconPosition(IconPosition pos) {
    if (iconPosition_ != pos) {
        iconPosition_ = pos;
        layoutDirty_ = true;
        markDirty();
    }
}

// ============================================================================
// BEHAVIOR SETTERS
// ============================================================================

void SkiaButton::setToggleable(bool toggleable) {
    toggleable_ = toggleable;
}

void SkiaButton::setToggleState(bool toggled) {
    if (toggled_ != toggled) {
        toggled_ = toggled;
        invalidateColors();
        markDirty();
        
        if (onToggle) {
            onToggle(toggled_);
        }
    }
}

void SkiaButton::setAudioReactive(bool reactive) {
    audioReactive_ = reactive;
}

void SkiaButton::setAudioLevel(float level) {
    float clampedLevel = juce::jlimit(0.0f, 1.0f, level);
    
    if (audioLevel_ != clampedLevel) {
        audioLevel_ = clampedLevel;
        
        if (audioReactive_) {
            markDirty();
        }
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void SkiaButton::drawSkia(SkCanvas* canvas) {
    if (colorsDirty_) {
        calculateColors();
    }
    
    if (textDirty_) {
        updateTextBlob();
    }
    
    if (layoutDirty_) {
        calculateLayout();
    }
    
    auto bounds = getLocalBounds().toFloat();
    float cornerRadius = design::dimensions::RADIUS_MD;
    
    SkRRect rrect = SkRRect::MakeRectXY(
        SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()),
        cornerRadius, cornerRadius
    );
    
    // 1. Draw Shadow (New)
    SkPaint shadowPaint;
    shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
    shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    canvas->drawRRect(rrect.makeOffset(0, 2), shadowPaint);

    // 2. Draw glow
    if (isGlowEnabled() && (isHovered() || pressed_ || toggled_)) {
        drawGlow(canvas, rrect);
    }
    
    // 3. Draw background
    drawBackground(canvas, rrect);
    
    // 4. Draw border (Subtle stroke for all buttons)
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(SkColorSetARGB(40, 255, 255, 255)); // 15% white border
    canvas->drawRRect(rrect, borderPaint);
    
    // 5. Draw icon
    if (icon_) {
        drawIcon(canvas);
    }
    
    // 6. Draw text
    if (textBlob_) {
        drawText(canvas);
    }
}

// ============================================================================
// INTERACTION OVERRIDES
// ============================================================================

void SkiaButton::onHoverEnter() {
    invalidateColors();
    animateTo("scale", 1.02f, design::animation::DURATION_FAST);
    animateTo("glow", 1.0f, design::animation::DURATION_FAST);
}

void SkiaButton::onHoverExit() {
    invalidateColors();
    animateTo("scale", 1.0f, design::animation::DURATION_FAST);
    animateTo("glow", 0.0f, design::animation::DURATION_FAST);
}

void SkiaButton::mouseDown(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    
    pressed_ = true;
    invalidateColors();
    
    animateTo("scale", 0.98f, design::animation::DURATION_INSTANT);
    
    // Handle click
    if (getLocalBounds().contains(e.getPosition())) {
        if (toggleable_) {
            setToggleState(!toggled_);
        }
        
        if (onClick) {
            onClick();
        }
    }
}

void SkiaButton::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    pressed_ = false;
    invalidateColors();
    animateTo("scale", 1.0f, design::animation::DURATION_FAST);
}

void SkiaButton::focusGained(juce::Component::FocusChangeType cause) {
    juce::ignoreUnused(cause);
    setGlowEnabled(true);
    animateTo("glow", 0.8f, design::animation::DURATION_NORMAL);
}

void SkiaButton::focusLost(juce::Component::FocusChangeType cause) {
    juce::ignoreUnused(cause);
    setGlowEnabled(style_ == Style::Primary || style_ == Style::Danger);
    animateTo("glow", 0.0f, design::animation::DURATION_NORMAL);
}

// ============================================================================
// COLOR CALCULATION
// ============================================================================

void SkiaButton::calculateColors() {
    cachedBgColor_ = getBackgroundColor();
    cachedTextColor_ = getTextColor();
    cachedBorderColor_ = getBorderColor();
    cachedGlowColor_ = getGlowColor();
    
    colorsDirty_ = false;
}

SkColor SkiaButton::getBackgroundColor() const {
    if (!isEnabled()) {
        return design::withAlpha(design::colors::BG_DARK, 0.5f);
    }
    
    // Base color by style
    SkColor baseColor;
    switch (style_) {
        case Style::Primary:
            baseColor = design::colors::CYAN;
            break;
        case Style::Secondary:
            baseColor = design::colors::BG_MEDIUM;
            break;
        case Style::Danger:
            baseColor = design::colors::RED;
            break;
        case Style::Warn:
            baseColor = design::colors::AMBER;
            break;
        case Style::Success:
            baseColor = design::colors::NEON_GREEN;
            break;
        case Style::Ghost:
            baseColor = design::colors::GLASS_10;
            break;
    }
    
    if (pressed_ || toggled_) {
        return design::darken(baseColor, 0.2f);
    }
    
    if (isHovered()) {
        return design::lighten(baseColor, 0.2f);
    }
    
    return baseColor;
}

SkColor SkiaButton::getTextColor() const {
    if (!isEnabled()) {
        return design::colors::TEXT_DISABLED;
    }
    
    return design::colors::TEXT_PRIMARY;
}

SkColor SkiaButton::getBorderColor() const {
    if (style_ != Style::Ghost) {
        return SK_ColorTRANSPARENT;
    }
    
    if (isHovered() || pressed_) {
        return design::colors::BORDER_STRONG;
    }
    
    return design::colors::BORDER_DEFAULT;
}

SkColor SkiaButton::getGlowColor() const {
    switch (style_) {
        case Style::Primary:   return design::colors::CYAN;
        case Style::Secondary: return design::colors::TEXT_PRIMARY;
        case Style::Danger:    return design::colors::RED;
        case Style::Warn:      return design::colors::AMBER;
        case Style::Success:   return design::colors::NEON_GREEN;
        case Style::Ghost:     return design::colors::CYAN;
    }
    
    return design::colors::CYAN;
}

void SkiaButton::invalidateColors() {
    colorsDirty_ = true;
}

// ============================================================================
// TEXT RENDERING
// ============================================================================

void SkiaButton::updateTextBlob() {
    if (text_.isEmpty()) {
        textBlob_ = nullptr;
        textDirty_ = false;
        return;
    }
    
    // Get font size based on button size
    float fontSize;
    switch (size_) {
        case Size::Small:  fontSize = design::typography::FONT_SM; break;
        case Size::Medium: fontSize = design::typography::FONT_MD; break;
        case Size::Large:  fontSize = design::typography::FONT_LG; break;
    }
    
    SkFont font;
    font.setSize(fontSize);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    
    textBlob_ = SkTextBlob::MakeFromText(
        text_.toStdString().c_str(),
        text_.length(),
        font,
        SkTextEncoding::kUTF8
    );
    
    textDirty_ = false;
}

// ============================================================================
// LAYOUT CALCULATION
// ============================================================================

void SkiaButton::calculateLayout() {
    auto bounds = getLocalBounds().toFloat();
    float padding = design::spacing::SM;
    
    float centerY = bounds.getCentreY();
    float iconSize = bounds.getHeight() * 0.5f;
    
    float totalWidth = 0;
    
    if (icon_) {
        totalWidth += iconSize + design::spacing::XS;
    }
    
    if (textBlob_) {
        SkRect textBounds = textBlob_->bounds();
        totalWidth += textBounds.width();
    }
    
    float startX = bounds.getCentreX() - (totalWidth / 2.0f);
    
    if (icon_ && iconPosition_ == IconPosition::Left) {
        iconRect_ = SkRect::MakeXYWH(startX, centerY - iconSize/2, iconSize, iconSize);
        startX += iconSize + design::spacing::XS;
    }
    
    if (textBlob_) {
        SkRect textBounds = textBlob_->bounds();
        textRect_ = SkRect::MakeXYWH(startX, centerY + textBounds.height()/2, 
                                     textBounds.width(), textBounds.height());
        startX += textBounds.width() + design::spacing::XS;
    }
    
    if (icon_ && iconPosition_ == IconPosition::Right) {
        iconRect_ = SkRect::MakeXYWH(startX, centerY - iconSize/2, iconSize, iconSize);
    }
    
    layoutDirty_ = false;
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================

void SkiaButton::drawGlow(SkCanvas* canvas, const SkRRect& bounds) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setColor(cachedGlowColor_);
    
    float glowIntensity = getAnimatedValue("glow");
    
    if (audioReactive_ && audioLevel_ > 0.1f) {
        glowIntensity += audioLevel_ * 0.5f;
    }
    
    float radius = design::effects::GLOW_MEDIUM * glowIntensity;
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, radius));
    
    canvas->drawRRect(bounds, paint);
}

void SkiaButton::drawBackground(SkCanvas* canvas, const SkRRect& bounds) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    
    // Vertical gradient
    SkPoint pts[] = { {0, bounds.rect().top()}, {0, bounds.rect().bottom()} };
    SkColor base = cachedBgColor_;
    SkColor lighter = design::lighten(base, 0.1f);
    SkColor darker = design::darken(base, 0.1f);
    
    SkColor colors[] = { lighter, base, darker };
    float pos[] = { 0.0f, 0.5f, 1.0f };
    
    paint.setShader(SkGradientShader::MakeLinear(pts, colors, pos, 3, SkTileMode::kClamp));
    
    canvas->drawRRect(bounds, paint);
}

void SkiaButton::drawBorder(SkCanvas* canvas, const SkRRect& bounds) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(cachedBorderColor_);
    
    canvas->drawRRect(bounds, paint);
}

void SkiaButton::drawIcon(SkCanvas* canvas) {
    if (!icon_) return;
    
    SkPaint paint;
    paint.setAntiAlias(true);
    
    canvas->drawImageRect(icon_.get(), iconRect_, SkSamplingOptions(), &paint);
}

void SkiaButton::drawText(SkCanvas* canvas) {
    if (!textBlob_) return;
    
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(cachedTextColor_);
    
    canvas->drawTextBlob(textBlob_.get(), textRect_.x(), textRect_.y(), paint);
}