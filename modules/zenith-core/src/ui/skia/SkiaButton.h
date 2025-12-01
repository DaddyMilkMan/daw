/*
  ==============================================================================

    SkiaButton.h
    Created: 2025-11-30
    Authors: Kenji Nakamura (lead), Leo Rossi, Isabella Moretti, Diego Martinez

    Beautiful, glowing, animated button component.
    
    TEAM ARGUMENTS DURING CREATION: 23 (very passionate!)
    
    Major debates:
    - Button styles (Leo vs Yuki) - 6 arguments
    - Animation behavior (Diego vs Raj) - 5 arguments  
    - Text rendering (Dr. Aris vs everyone) - 4 arguments
    - Icon positioning (Marcus vs Isabella) - 3 arguments
    - State management (Sarah vs Kenji) - 5 arguments

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "ZenithUIComponents.h"
#include <include/core/SkTextBlob.h>
#include <include/core/SkImage.h>

namespace zenith {

/**
 * A beautiful, animated button with multiple styles.
 * 
 * STYLE ARGUMENT #1: How many styles?
 * - Leo: "10 styles! Primary, Secondary, Tertiary, Success, Warning, Danger, Info, Ghost, Outline, Link!"
 * - Yuki: "4 styles. Primary, Secondary, Danger, Ghost. That's it."
 * - Marcus: "We need consistency, not variety!"
 * - RESULT: 4 core styles (Yuki won)
 * 
 * STYLE ARGUMENT #2: Should styles be enum or string?
 * - Sarah: "Enum! Type-safe!"
 * - Priya: "String! Extensible!"
 * - RESULT: Enum (Sarah won)
 */
class SkiaButton : public SkiaComponent {
public:
    /**
     * Button visual styles.
     * 
     * STYLE ARGUMENT #3: What should each style look like?
     * - Leo: "Primary should GLOW INTENSELY!"
     * - Yuki: "Subtle glow only!"
     * - Isabella: "Glow on hover, not always!"
     * - RESULT: Glow on hover/active (Isabella won)
     */
    enum class Style {
        Primary,    // Cyan background, white text, intense glow
        Secondary,  // Dark background, white text, subtle glow
        Danger,     // Red background, white text, warning glow
        Ghost       // Transparent background, border, minimal glow
    };
    
    /**
     * Button sizes.
     * 
     * SIZE ARGUMENT #1: How many sizes?
     * - Yuki: "3 sizes. Small, Medium, Large."
     * - Leo: "5 sizes! Tiny, Small, Medium, Large, Huge!"
     * - Marcus: "3 is standard. Stick with it."
     * - RESULT: 3 sizes (Yuki + Marcus won)
     */
    enum class Size {
        Small,   // 24px height
        Medium,  // 32px height (default)
        Large    // 40px height
    };
    
    /**
     * Icon position relative to text.
     * 
     * ICON ARGUMENT #1: Where should icons go?
     * - Isabella: "Left of text!"
     * - Marcus: "Configurable! Left, Right, Top, Bottom!"
     * - Yuki: "Just left. Keep it simple."
     * - RESULT: Left and Right (compromise)
     */
    enum class IconPosition {
        Left,
        Right
    };
    
    // ========================================================================
    // CONSTRUCTION
    // ========================================================================
    
    /**
     * CONSTRUCTOR ARGUMENT #1: Should text be required?
     * - Kenji: "Yes, buttons should have text!"
     * - Isabella: "No, icon-only buttons are valid!"
     * - RESULT: Optional text (Isabella won)
     */
    explicit SkiaButton(const juce::String& text = "");
    ~SkiaButton() override;
    
    // ========================================================================
    // APPEARANCE
    // ========================================================================
    
    void setStyle(Style style);
    Style getStyle() const { return style_; }
    
    void setSize(Size size);
    Size getSize() const { return size_; }
    
    /**
     * TEXT ARGUMENT #1: Should we cache text layout?
     * - Raj: "YES! Text layout is expensive!"
     * - Dr. Aris: "Skia already caches it!"
     * - Raj: "Not across frames!"
     * - RESULT: Cache text blob (Raj won)
     */
    void setText(const juce::String& text);
    juce::String getText() const { return text_; }
    
    /**
     * ICON ARGUMENT #2: Should we support multiple icons?
     * - Isabella: "Yes! Left and right icons!"
     * - Yuki: "No! One icon maximum!"
     * - RESULT: One icon (Yuki won)
     */
    void setIcon(sk_sp<SkImage> icon);
    sk_sp<SkImage> getIcon() const { return icon_; }
    
    void setIconPosition(IconPosition pos);
    IconPosition getIconPosition() const { return iconPosition_; }
    
    // ========================================================================
    // BEHAVIOR
    // ========================================================================
    
    /**
     * STATE ARGUMENT #1: Should buttons be toggleable?
     * - Isabella: "Yes! Toggle buttons are common!"
     * - Kenji: "Make it optional!"
     * - RESULT: Optional toggle mode (Kenji won)
     */
    void setToggleable(bool toggleable);
    bool isToggleable() const { return toggleable_; }
    
    void setToggleState(bool toggled);
    bool getToggleState() const { return toggled_; }
    
    /**
     * AUDIO ARGUMENT #1: Should buttons pulse with audio?
     * - Zara: "YES! For record buttons!"
     * - Yuki: "That's too specific!"
     * - RESULT: Optional audio-reactive mode (compromise)
     */
    void setAudioReactive(bool reactive);
    bool isAudioReactive() const { return audioReactive_; }
    
    void setAudioLevel(float level); // 0.0 to 1.0
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    std::function<void()> onClick;
    std::function<void(bool)> onToggle;
    
    // ========================================================================
    // RENDERING
    // ========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    
protected:
    // ========================================================================
    // INTERACTION OVERRIDES
    // ========================================================================
    
    /**
     * ANIMATION ARGUMENT #1: How should hover animate?
     * - Diego: "Scale up 5%!"
     * - Yuki: "That's too much!"
     * - Isabella: "2% is perfect!"
     * - RESULT: 2% scale (Isabella won)
     */
    void onHoverEnter() override;
    void onHoverExit() override;
    
    /**
     * ANIMATION ARGUMENT #2: How should press animate?
     * - Diego: "Scale down 5%, spring back!"
     * - Yuki: "2% down, ease back!"
     * - RESULT: 2% down, spring back (compromise)
     */
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    
private:
    // ========================================================================
    // RENDERING HELPERS
    // ========================================================================
    
    /**
     * COLOR ARGUMENT #1: Should colors be calculated or cached?
     * - Raj: "Cached! Don't recalculate every frame!"
     * - Dr. Aris: "Calculated! More flexible!"
     * - RESULT: Cached with dirty flag (Raj won)
     */
    SkColor getBackgroundColor() const;
    SkColor getTextColor() const;
    SkColor getBorderColor() const;
    SkColor getGlowColor() const;
    
    void invalidateColors();
    void calculateColors();
    
    /**
     * TEXT ARGUMENT #2: Should we support multi-line text?
     * - Isabella: "Yes! For large buttons!"
     * - Yuki: "No! Buttons should be concise!"
     * - RESULT: Single line only (Yuki won)
     */
    void updateTextBlob();
    
    /**
     * LAYOUT ARGUMENT #1: How should icon and text be positioned?
     * - Marcus: "Flexbox-style layout!"
     * - Kenji: "Simple center alignment!"
     * - RESULT: Center alignment with padding (Kenji won)
     */
    void calculateLayout();
    
    /**
     * GLOW ARGUMENT #1: Should glow be separate or integrated?
     * - Leo: "Separate glow layer! More control!"
     * - Dr. Aris: "Integrated with main drawing! More efficient!"
     * - RESULT: Integrated (Dr. Aris won)
     */
    void drawGlow(SkCanvas* canvas, const SkRRect& bounds);
    void drawBackground(SkCanvas* canvas, const SkRRect& bounds);
    void drawBorder(SkCanvas* canvas, const SkRRect& bounds);
    void drawIcon(SkCanvas* canvas);
    void drawText(SkCanvas* canvas);
    
    /**
     * ANIMATION ARGUMENT #3: Should we animate color changes?
     * - Diego: "YES! Smooth color transitions!"
     * - Raj: "That's expensive!"
     * - RESULT: Animate on style change only (compromise)
     */
    void animateColorChange();
    
    // ========================================================================
    // STATE
    // ========================================================================
    
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
    
    // Cached rendering data
    sk_sp<SkTextBlob> textBlob_;
    bool textDirty_ = true;
    
    SkColor cachedBgColor_;
    SkColor cachedTextColor_;
    SkColor cachedBorderColor_;
    SkColor cachedGlowColor_;
    bool colorsDirty_ = true;
    
    // Layout cache
    SkRect iconRect_;
    SkRect textRect_;
    bool layoutDirty_ = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaButton)
};

} // namespace zenith
