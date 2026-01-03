/**
 * @file ZenithLookAndFeel.h
 * @brief DEPRECATED: Modern LookAndFeel with proper micro-interactions and visual polish
 * @author Fixed by Claude - December 2025
 * 
 * NOTE: This LookAndFeel is maintained for backward compatibility with legacy JUCE
 * components. All new UI components should use Skia rendering via SkiaComponent.
 * JUCE rendering is no longer the primary rendering path - Skia is used for all
 * custom UI components.
 * 
 * MIGRATION PATH: Replace juce::TextButton -> ZenithButton/SkiaButton
 *                Replace juce::Slider -> ZenithSlider/SkiaSlider
 *                Replace juce::ComboBox -> ZenithDropdown/SkiaComboBox
 */

#pragma once
#include "ZenithTheme.h"
#include "ColorBridge.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @class ZenithLookAndFeel
 * @brief LEGACY: Custom LookAndFeel for backward compatibility with standard JUCE components
 * 
 * This class provides JUCE-based rendering for any remaining standard JUCE components
 * (juce::TextButton, juce::Slider, juce::ComboBox, etc.). It should NOT be used for
 * new components - use Skia-based components instead (SkiaComponent, ZenithButton, etc.).
 */
class ZenithLookAndFeel : public juce::LookAndFeel_V4 {
public:
  ZenithLookAndFeel();
  ~ZenithLookAndFeel() override = default;

  static ZenithLookAndFeel &getInstance();

  //==========================================================================
  // BACKWARDS COMPATIBILITY: Legacy color aliases (map to ZenithTheme)
  //==========================================================================
  struct Colors {
    static const juce::Colour &background;
    static const juce::Colour &backgroundPanel;
    static const juce::Colour &panel;
    static const juce::Colour &textPrimary;
    static const juce::Colour &textSecondary;
    static const juce::Colour &border;
    static const juce::Colour &accent;
  };

  //==========================================================================
  // BACKWARDS COMPATIBILITY: Legacy font accessors
  //==========================================================================
  static juce::Font getFontSmall() {
    return ZenithTheme::Typography::getSmallFont();
  }
  static juce::Font getFontMedium() {
    return ZenithTheme::Typography::getBodyFont();
  }
  static juce::Font getFontLarge() {
    return ZenithTheme::Typography::getLargeFont();
  }

  //==========================================================================
  // Button Rendering
  //==========================================================================
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

  void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

  //==========================================================================
  // Slider Rendering (Knobs & Faders)
  //==========================================================================
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, const float rotaryStartAngle,
                        const float rotaryEndAngle,
                        juce::Slider &slider) override;

  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override;

  //==========================================================================
  // ComboBox Rendering
  //==========================================================================
  void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &box) override;

  //==========================================================================
  // Popup Menu Rendering
  //==========================================================================
  void drawPopupMenuBackground(juce::Graphics &g, int width,
                               int height) override;

  void drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area,
                         bool isSeparator, bool isActive, bool isHighlighted,
                         bool isTicked, bool hasSubMenu,
                         const juce::String &text,
                         const juce::String &shortcutKeyText,
                         const juce::Drawable *icon,
                         const juce::Colour *textColour) override;

  //==========================================================================
  // ScrollBar Rendering
  //==========================================================================
  void drawScrollbar(juce::Graphics &g, juce::ScrollBar &scrollbar, int x,
                     int y, int width, int height, bool isScrollbarVertical,
                     int thumbStartPosition, int thumbSize, bool isMouseOver,
                     bool isMouseDown) override;

  //==========================================================================
  // Label Rendering
  //==========================================================================
  void drawLabel(juce::Graphics &g, juce::Label &label) override;

  //==========================================================================
  // TextEditor Rendering
  //==========================================================================
  void fillTextEditorBackground(juce::Graphics &g, int width, int height,
                                juce::TextEditor &textEditor) override;

  void drawTextEditorOutline(juce::Graphics &g, int width, int height,
                             juce::TextEditor &textEditor) override;

  //==========================================================================
  // ToggleButton Rendering
  //==========================================================================
  void drawToggleButton(juce::Graphics &g, juce::ToggleButton &button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

  //==========================================================================
  // TabBar Rendering
  //==========================================================================
  void drawTabButton(juce::TabBarButton &button, juce::Graphics &g,
                     bool isMouseOver, bool isMouseDown) override;

  //==========================================================================
  // Tooltip Rendering
  //==========================================================================
  void drawTooltip(juce::Graphics &g, const juce::String &text, int width,
                   int height) override;

  //==========================================================================
  // Utility Methods
  //==========================================================================

  // Draw modern card with shadow
  static void drawCard(juce::Graphics &g, juce::Rectangle<float> bounds,
                       float cornerRadius = ZenithTheme::Radius::md,
                       float elevation = ZenithTheme::Shadows::elevation_2);

  // Draw focus ring
  static void drawFocusRing(juce::Graphics &g, juce::Rectangle<float> bounds,
                            float cornerRadius = ZenithTheme::Radius::sm);

  // Draw separator line
  static void drawSeparator(juce::Graphics &g, juce::Rectangle<float> bounds,
                            bool vertical = false);

private:
  // Singleton instance
  static std::unique_ptr<ZenithLookAndFeel> instance_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithLookAndFeel)
};

} // namespace zenith
