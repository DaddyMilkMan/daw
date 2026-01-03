/**
 * @file ZenithLookAndFeel.h
 * @brief Custom LookAndFeel for standard JUCE widgets (not Skia components)
 * @author Fixed by Claude - December 2025
 * 
 * NOTE: This LookAndFeel is ONLY used for standard JUCE widgets (TextButton, Slider,
 * ComboBox, etc.) that are rendered internally by JUCE. It uses juce::Graphics because
 * JUCE's widget rendering system requires it.
 * 
 * For custom Zenith DAW UI components, use SkiaComponent and drawSkia() instead.
 * This ensures hardware-accelerated rendering via Skia.
 * 
 * ARCHITECTURE SPLIT:
 * - ZenithLookAndFeel -> For JUCE standard widgets (legacy support)
 * - SkiaComponent     -> For all custom Zenith DAW UI (primary rendering)
 */

#pragma once
#include "ZenithTheme.h"
#include "ColorBridge.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @class ZenithLookAndFeel
 * @brief Custom LookAndFeel for standard JUCE widgets only
 * 
 * This class implements rendering for JUCE's standard widget types (buttons, sliders, etc.)
 * using juce::Graphics. It exists solely to provide visual consistency when standard JUCE
 * widgets are used.
 * 
 * IMPORTANT: Do NOT use this for custom Zenith DAW components. Instead:
 * 1. Inherit from SkiaComponent
 * 2. Override drawSkia(SkCanvas* canvas)
 * 3. Use Skia rendering APIs directly
 * 
 * This LookAndFeel is only used by the AI style applicator for legacy JUCE widgets.
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
