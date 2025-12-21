/*
  ==============================================================================

    SkiaButton.h
    Created: 2025-11-30
    Authors: Kenji Nakamura (lead), Leo Rossi, Isabella Moretti, Diego Martinez

    Beautiful, glowing, animated button component.
    
  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "ZenithUIComponents.h"
#include <core/SkTextBlob.h>
#include <core/SkImage.h>

namespace zenith {

class SkiaButton : public SkiaComponent {
public:
    enum class Style {
        Primary,
        Secondary,
        Danger,
        Warn,
        Success,
        Ghost
    };
    
    enum class Size {
        Small,
        Medium,
        Large
    };
    
    enum class IconPosition {
        Left,
        Right
    };
    
    explicit SkiaButton(const juce::String& text = "");
    ~SkiaButton() override;
    
    void setStyle(Style style);
    Style getStyle() const { return style_; }
    
    void setSize(Size size);
    Size getSize() const { return size_; }
    
    void setText(const juce::String& text);
    juce::String getText() const { return text_; }
    
    void setIcon(sk_sp<SkImage> icon);
    sk_sp<SkImage> getIcon() const { return icon_; }
    
    void setIconPosition(IconPosition pos);
    IconPosition getIconPosition() const { return iconPosition_; }
    
    void setToggleable(bool toggleable);
    bool isToggleable() const { return toggleable_; }
    
    void setToggleState(bool toggled);
    bool getToggleState() const { return toggled_; }
    
    void setAudioReactive(bool reactive);
    bool isAudioReactive() const { return audioReactive_; }
    
    void setAudioLevel(float level); // 0.0 to 1.0
    
    std::function<void()> onClick;
    std::function<void(bool)> onToggle;
    
    void drawSkia(SkCanvas* canvas) override;
    
protected:
    void onHoverEnter() override;
    void onHoverExit() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    
private:
    SkColor getBackgroundColor() const;
    SkColor getTextColor() const;
    SkColor getBorderColor() const;
    SkColor getGlowColor() const;
    
    void invalidateColors();
    void calculateColors();
    void updateTextBlob();
    void calculateLayout();
    
    void drawGlow(SkCanvas* canvas, const SkRRect& bounds);
    void drawBackground(SkCanvas* canvas, const SkRRect& bounds);
    void drawBorder(SkCanvas* canvas, const SkRRect& bounds);
    void drawIcon(SkCanvas* canvas);
    void drawText(SkCanvas* canvas);
    
    Style style_ = Style::Primary;
    Size size_ = Size::Medium;
    IconPosition iconPosition_ = IconPosition::Left;
    
    juce::String text_;
    sk_sp<SkImage> icon_;
    
    bool toggleable_ = false;
    bool toggled_ = false;
    bool pressed_ = false;
    
    bool audioReactive_ = false;
    float audioLevel_ = 0.0f;
    
    sk_sp<SkTextBlob> textBlob_;
    bool textDirty_ = true;
    
    SkColor cachedBgColor_ = 0;
    SkColor cachedTextColor_ = 0;
    SkColor cachedBorderColor_ = 0;
    SkColor cachedGlowColor_ = 0;
    bool colorsDirty_ = true;
    
    SkRect iconRect_ = SkRect::MakeEmpty();
    SkRect textRect_ = SkRect::MakeEmpty();
    bool layoutDirty_ = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaButton)
};

} // namespace zenith