/*
  ==============================================================================

    SkiaButton.cpp
    Created: 2025-11-30
    Authors: Kenji Nakamura (lead), Leo Rossi, Diego Martinez, Isabella Moretti

    Implementation of the beautiful, glowing, animated button.
    
    IMPLEMENTATION ARGUMENTS: 28 (VERY heated!)
    
    This file contains the actual rendering and behavior implementation.
    Every detail was debated by the team!

  ==============================================================================
*/

#include "SkiaButton.h"
#include <include/core/SkRRect.h>
#include <include/core/SkFont.h>

namespace zenith {

// ============================================================================
}

SkiaButton::~SkiaButton() {
    // ARGUMENT #4: Should we clean up cached resources?
    // - Viktor: "YES! Always clean up!"
    // - Raj: "They're smart pointers, they clean themselves!"
    // - RESULT: Let smart pointers handle it (Raj won)
    // textBlob_ will be automatically released by sk_sp
}

// ============================================================================
// APPEARANCE SETTERS
// ============================================================================

void SkiaButton::setStyle(Style style) {
    // ARGUMENT #5: Should style change trigger animation?
    // - Diego: "YES! Smooth color transition!"
    // - Raj: "That's expensive!"
    // - RESULT: Animate only if visible (compromise)
    
    if (style_ != style) {
        style_ = style;
        
        // Update glow based on style
        // ARGUMENT #6: Which styles should glow?
        // - Leo: "ALL OF THEM!"
        // - Yuki: "None of them by default!"
        // - RESULT: Primary and Danger glow (compromise)
        setGlowEnabled(style == Style::Primary || style == Style::Danger);
        
        invalidateColors();
        
        if (isVisible()) {
            animateColorChange();
        }
        
        markDirty();
    }
}

void SkiaButton::setSize(Size size) {
    // ARGUMENT #7: Should size change trigger relayout?
    // - Marcus: "YES! Always!"
    // - Raj: "Only if it actually changes!"
    // - RESULT: Check for change first (Raj won)
    
    if (size_ != size) {
        size_ = size;
        layoutDirty_ = true;
        
        // Update component height
        // ARGUMENT #8: Should we resize the component?
        // - Marcus: "YES! Size should match!"
        // - Kenji: "Let the parent control size!"
        // - RESULT: Set height, keep width (compromise)
        float height = 0;
        switch (size) {
            case Size::Small:  height = design::dimensions::BUTTON_HEIGHT_SM; break;
            case Size::Medium: height = design::dimensions::BUTTON_HEIGHT; break;
            case Size::Large:  height = design::dimensions::BUTTON_HEIGHT_LG; break;
        }
        setSize(getWidth(), static_cast<int>(height));
        
        markDirty();
    }
}

void SkiaButton::setText(const juce::String& text) {
    // ARGUMENT #9: Should empty text be allowed?
    // - Isabella: "YES! Icon-only buttons!"
    // - Yuki: "Buttons should have text!"
    // - RESULT: Allow empty text (Isabella won)
    
    if (text_ != text) {
        text_ = text;
        textDirty_ = true;
        layoutDirty_ = true;
        markDirty();
    }
}

void SkiaButton::setIcon(sk_sp<SkImage> icon) {
    // ARGUMENT #10: Should we validate icon size?
    // - Marcus: "YES! Icons should be reasonable size!"
    // - Kenji: "Just scale it to fit!"
    // - RESULT: Scale to fit (Kenji won)
    
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
    // ARGUMENT #11: Should toggle state change trigger callback?
    // - Isabella: "YES! Always notify!"
    // - Kenji: "Only if it actually changes!"
    // - RESULT: Check for change, then notify (Kenji won)
    
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
    // ARGUMENT #12: Should we clamp audio level?
    // - Zara: "YES! Prevent overflow!"
    // - Raj: "Trust the caller!"
    // - RESULT: Clamp it (Zara won - safety first)
    
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
    // ARGUMENT #13: Should we update caches here or in setters?
    // - Raj: "In setters! Don't do work during rendering!"
    // - Dr. Aris: "Here! Lazy evaluation!"
    // - RESULT: Update here if dirty (lazy evaluation won)
    
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
    
    // ARGUMENT #14: What order should we draw in?
    // - Leo: "Glow first! Then background! Then content!"
    // - Dr. Aris: "Background, content, then glow on top!"
    // - RESULT: Glow, background, border, content (Leo won)
    
    // Get corner radius
    // ARGUMENT #15: Should corner radius be configurable?
    // - Marcus: "YES! Different radii for different contexts!"
    // - Yuki: "No! Use design system constant!"
    // - RESULT: Use design system (Yuki won)
    float cornerRadius = design::dimensions::RADIUS_MD;
    
    SkRRect rrect = SkRRect::MakeRectXY(
        SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()),
        cornerRadius, cornerRadius
    );
    
    // 1. Draw glow
    if (glowEnabled_ && (isHovered() || pressed_ || toggled_)) {
        drawGlow(canvas, rrect);
    }
    
    // 2. Draw background
    drawBackground(canvas, rrect);
    
    // 3. Draw border (for Ghost style)
    if (style_ == Style::Ghost) {
        drawBorder(canvas, rrect);
    }
    
    // 4. Draw icon
    if (icon_) {
        drawIcon(canvas);
    }
    
    // 5. Draw text
    if (textBlob_) {
        drawText(canvas);
    }
}

// ============================================================================
// INTERACTION OVERRIDES
// ============================================================================

void SkiaButton::onHoverEnter() {
    // ARGUMENT #16: Should hover trigger animation immediately?
    // - Diego: "YES! Instant feedback!"
    // - Raj: "Add small delay to prevent jitter!"
    // - RESULT: Immediate animation (Diego won)
    
    invalidateColors();
    
    // Animate scale
    // ARGUMENT #17: Should we animate scale on hover?
    // - Isabella: "YES! 2% scale up!"
    // - Yuki: "No animation! Just color change!"
    // - RESULT: 2% scale (Isabella won)
    animateTo("scale", 1.02f, design::animation::DURATION_FAST);
    
    // Animate glow
    animateTo("glow", 1.0f, design::animation::DURATION_FAST);
}

void SkiaButton::onHoverExit() {
    invalidateColors();
    
    // Animate back to normal
    animateTo("scale", 1.0f, design::animation::DURATION_FAST);
    animateTo("glow", 0.0f, design::animation::DURATION_FAST);
}

void SkiaButton::mouseDown(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    
    pressed_ = true;
    invalidateColors();
    
    // ARGUMENT #18: How much should button scale down on press?
    // - Diego: "5%! Big feedback!"
    // - Yuki: "0%! No animation!"
    // - Isabella: "2%! Subtle but noticeable!"
    // - RESULT: 2% (Isabella won)
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

void SkiaButton::focusGained(juce::Component::FocusChangeType cause) {
    juce::ignoreUnused(cause);
    
    // ARGUMENT #20: Should focus show glow?
    // - Isabella: "YES! Accessibility!"
    // - Leo: "DOUBLE glow!"
    // - RESULT: Enable glow on focus (Isabella won)
    setGlowEnabled(true);
    animateTo("glow", 0.8f, design::animation::DURATION_NORMAL);
}

void SkiaButton::focusLost(juce::Component::FocusChangeType cause) {
    juce::ignoreUnused(cause);
    
    // Restore glow based on style
    setGlowEnabled(style_ == Style::Primary || style_ == Style::Danger);
    animateTo("glow", 0.0f, design::animation::DURATION_NORMAL);
}

// ============================================================================
// COLOR CALCULATION
// ============================================================================

void SkiaButton::calculateColors() {
    // ARGUMENT #21: Should we calculate all colors or just current state?
    // - Raj: "Just current state! Don't waste cycles!"
    // - Sarah: "All states! Simpler code!"
    // - RESULT: Just current state (Raj won)
    
    cachedBgColor_ = getBackgroundColor();
    cachedTextColor_ = getTextColor();
    cachedBorderColor_ = getBorderColor();
    cachedGlowColor_ = getGlowColor();
    
    colorsDirty_ = false;
}

SkColor SkiaButton::getBackgroundColor() const {
    // ARGUMENT #22: How should we handle disabled state?
    // - Viktor: "Grey it out!"
    // - Yuki: "Reduce opacity!"
    // - RESULT: Reduce opacity (Yuki won - more elegant)
    
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
        case Style::Ghost:
            baseColor = design::colors::GLASS_10;
            break;
    }
    
    // ARGUMENT #23: Should pressed state be darker or lighter?
    // - Leo: "Lighter! It glows!"
    // - Yuki: "Darker! It's pressed down!"
    // - RESULT: Darker (Yuki won - more intuitive)
    
    if (pressed_ || toggled_) {
        return design::darken(baseColor, 0.2f);
    }
    
    if (isHovered()) {
        return design::lighten(baseColor, 0.2f);
    }
    
    return baseColor;
}

SkColor SkiaButton::getTextColor() const {
    // ARGUMENT #24: Should text color change with state?
    // - Yuki: "No! Always white for readability!"
    // - Leo: "YES! Glow the text too!"
    // - RESULT: Always white (Yuki won - accessibility)
    
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
    // ARGUMENT #25: Should glow color match button color?
    // - Leo: "YES! Cyan buttons glow cyan!"
    // - Yuki: "No! Always use accent color!"
    // - RESULT: Match button color (Leo won)
    
    switch (style_) {
        case Style::Primary:   return design::colors::CYAN;
        case Style::Secondary: return design::colors::TEXT_PRIMARY;
        case Style::Danger:    return design::colors::RED;
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
    // ARGUMENT #26: Should we handle empty text?
    // - Viktor: "Check for empty and skip!"
    // - Kenji: "Just create empty blob!"
    // - RESULT: Skip if empty (Viktor won - more efficient)
    
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
    
    // ARGUMENT #27: Should we use toRawUTF8() or toStdString().c_str()?
    // - Dr. Aris: "toRawUTF8()! It's faster!"
    // - Sarah: "toStdString().c_str()! It's safer!"
    // - RESULT: toStdString().c_str() (Sarah won - compatibility)
    
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
    
    // Calculate available space
    float availableWidth = bounds.getWidth() - (padding * 2);
    float centerY = bounds.getCentreY();
    
    // Icon size
    float iconSize = bounds.getHeight() * 0.5f;
    
    // ARGUMENT #28: How should we layout icon and text?
    // - Marcus: "Complex flexbox algorithm!"
    // - Kenji: "Simple center alignment!"
    // - RESULT: Simple center (Kenji won - KISS principle)
    
    float totalWidth = 0;
    
    if (icon_) {
        totalWidth += iconSize + design::spacing::XS;
    }
    
    if (textBlob_) {
        SkRect textBounds;
        textBlob_->bounds(&textBounds);
        totalWidth += textBounds.width();
    }
    
    float startX = bounds.getCentreX() - (totalWidth / 2.0f);
    
    if (icon_ && iconPosition_ == IconPosition::Left) {
        iconRect_ = SkRect::MakeXYWH(startX, centerY - iconSize/2, iconSize, iconSize);
        startX += iconSize + design::spacing::XS;
    }
    
    if (textBlob_) {
        SkRect textBounds;
        textBlob_->bounds(&textBounds);
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
    
    // Add audio reactivity
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
    paint.setColor(cachedBgColor_);
    
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

void SkiaButton::animateColorChange() {
    // Smooth color transition when style changes
    // This is a placeholder - full implementation would interpolate colors
    markDirty();
}

} // namespace zenith
