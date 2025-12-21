/**
 * @file ZenithLookAndFeel.h
 * @brief Modern Vibrant Professional Design System for Zenith DAW
 *
 * Design Philosophy:
 * - Vibrant yet professional color palette inspired by Logic Pro & Ableton Live
 * - Material Design dark theme elevation system (#121212 base, not pure black)
 * - WCAG AAA accessibility (7:1 contrast for text, 4.5:1 for UI components)
 * - 8px spacing grid for consistency and scalability
 * - Smooth animations with proper easing (150-300ms, ease-out)
 * - Color-coded tracks for organization (frequency-based mapping)
 * - 60-30-10 color rule (60% neutral, 30% secondary, 10% accent)
 *
 * Color Theory Applied:
 * - Jewel tones for rich professional look (S: 73-83%, B: 56-76%)
 * - Reduced saturation in dark mode (-20-40%) to prevent eye strain
 * - Warm color bias to reduce blue light fatigue
 * - Text at 87% white opacity (#DEDEDE) not pure white
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @class ZenithLookAndFeel
 * @brief Modern vibrant professional design system for Zenith DAW
 */
class ZenithLookAndFeel : public juce::LookAndFeel_V4 {
public:
  //==========================================================================
  // Constructor
  //==========================================================================

    //==========================================================================
    // Design Tokens: Colors
    //==========================================================================
    
    struct Colors
    {
        // Backgrounds - Logic Pro gray palette
        static constexpr juce::uint32 backgroundDark = 0xff1F1F1F;      // Main window background (Logic gray)
        static constexpr juce::uint32 backgroundMid = 0xff2B2B2B;       // Panels (browser, mixer)
        static constexpr juce::uint32 backgroundLight = 0xff3E3E3E;     // Hovered items, raised surfaces
        static constexpr juce::uint32 backgroundPanel = 0xff262626;     // Panel backgrounds
        
        // Accents - Logic Pro color scheme
        static constexpr juce::uint32 accentPrimary = 0xff006FFF;       // Logic Blue - primary interactive
        static constexpr juce::uint32 accentPrimaryDark = 0xff0055CC;   // Darker Logic Blue for pressed state
        static constexpr juce::uint32 accentSecondary = 0xff3A7DFF;     // Logic Blue glow - focus ring
        static constexpr juce::uint32 accentWarning = 0xffFF9900;       // Orange - warnings
        static constexpr juce::uint32 accentDanger = 0xffFF0000;        // Red - destructive/record
        
        // Text - Logic Pro softer white
        static constexpr juce::uint32 textPrimary = 0xffDFDFDF;         // Main text (softer than pure white)
        static constexpr juce::uint32 textSecondary = 0xff9A9A9A;       // Subtle text, labels
        static constexpr juce::uint32 textDisabled = 0xff666666;        // Disabled state
        
        // Borders - Logic Pro engraved style
        static constexpr juce::uint32 border = 0xff111111;              // Default borders (dark engraved)
        static constexpr juce::uint32 borderLight = 0xff444444;         // Lighter borders
        
        // Transport/Status - Logic Pro exact colors
        static constexpr juce::uint32 playGreen = 0xff00FF00;           // Play button (bright green)
        static constexpr juce::uint32 recordRed = 0xffFF0000;           // Record button (pure red)
        static constexpr juce::uint32 stopGrey = 0xff888888;            // Stop button
        
        // Meters - Logic Pro gradient
        static constexpr juce::uint32 meterGreen = 0xff00FF00;          // Level meter (low)
        static constexpr juce::uint32 meterYellow = 0xffFFFF00;         // Level meter (mid)
        static constexpr juce::uint32 meterRed = 0xffFF0000;            // Level meter (clip)

        // === Compatibility Aliases (Material Design -> Logic Pro) ===
        static constexpr juce::uint32 backgroundBase = backgroundDark;
        static constexpr juce::uint32 backgroundRaised = backgroundMid;
        static constexpr juce::uint32 backgroundElevated = backgroundLight;
        static constexpr juce::uint32 backgroundModal = backgroundPanel;
        
        static constexpr juce::uint32 accentPrimaryHover = accentSecondary;
        static constexpr juce::uint32 accentPrimaryPressed = accentPrimaryDark;
        static constexpr juce::uint32 accentPrimaryGlow = 0x66006FFF;
        
        static constexpr juce::uint32 accentSecondaryHover = accentWarning;
        
        static constexpr juce::uint32 success = playGreen;
        static constexpr juce::uint32 warning = accentWarning;
        static constexpr juce::uint32 danger = accentDanger;
        static constexpr juce::uint32 info = accentSecondary;
        
        static constexpr juce::uint32 textOnAccent = 0xffFFFFFF;
        
        static constexpr juce::uint32 borderSubtle = border;
        static constexpr juce::uint32 borderMedium = borderLight;
        static constexpr juce::uint32 borderStrong = 0xff666666;
        
        static constexpr juce::uint32 divider = border;
        
        static constexpr juce::uint32 meterAmber = meterYellow;
        
        // Track colors (Legacy)
        static constexpr juce::uint32 trackRedDark = 0xffcc3311;
        static constexpr juce::uint32 trackOrange = 0xffde8f05;
        static constexpr juce::uint32 trackAmber = 0xffffb302;
        static constexpr juce::uint32 trackYellow = 0xffffdd00;
        static constexpr juce::uint32 trackLime = 0xff88cc00;
        static constexpr juce::uint32 trackGreen = 0xff44aa99;
        static constexpr juce::uint32 trackCyan = 0xff00d9ff;
        static constexpr juce::uint32 trackBlue = 0xff0173b2;
        static constexpr juce::uint32 trackIndigo = 0xff6366f1;
        static constexpr juce::uint32 trackPurple = 0xff9c27b0;
        static constexpr juce::uint32 trackMagenta = 0xffe91e63;
        static constexpr juce::uint32 trackPink = 0xffff69b4;
    };

    //==========================================================================
    // Design Tokens: Metrics
    //==========================================================================
    
    struct Metrics
    {
        // Spacing (8px grid)
        static constexpr int spacingXS = 4;
        static constexpr int spacingS = 8;
        static constexpr int spacingM = 16;
        static constexpr int spacingL = 24;
        static constexpr int spacingXL = 32;
        static constexpr int spacingXXL = 48;
        
        // Border radius - Logic Pro style (more rounded)
        static constexpr float radiusS = 2.0f;
        static constexpr float radiusM = 4.0f;      // Scrollbars
        static constexpr float radiusL = 6.0f;      // Buttons, regions (Logic standard)
        static constexpr float radiusXL = 8.0f;
        
        // Component heights - Logic Pro dimensions
        static constexpr int buttonHeightS = 24;
        static constexpr int buttonHeightM = 32;
        static constexpr int buttonHeightL = 40;
        static constexpr int transportBarHeight = 60;    // Logic standard (40-60px)
        static constexpr int statusBarHeight = 24;
        static constexpr int mixerHeight = 200;
        
        // Widths
        static constexpr int browserPanelWidth = 280;
        static constexpr int inspectorPanelWidth = 320;
        static constexpr int masterStripWidth = 80;
        static constexpr int mixerStripWidth = 80;
        
        // Legacy aliases
        static constexpr int xs = spacingXS;
        static constexpr int s = spacingS;
        static constexpr int m = spacingM;
        static constexpr int l = spacingL;
        static constexpr int xl = spacingXL;
        static constexpr int xxl = spacingXXL;
        
        static constexpr int paddingPanel = 16;
        static constexpr int paddingCard = 12;
        static constexpr int paddingButton = 12;
    };
    
    using Spacing = Metrics;
    
    //==========================================================================
    // Design Tokens: Elevation (Compatibility)
    //==========================================================================
    
    struct Elevation {
        static constexpr juce::uint32 dp0 = Colors::backgroundDark;
        static constexpr juce::uint32 dp1 = Colors::backgroundPanel;
        static constexpr juce::uint32 dp2 = Colors::backgroundMid;
        static constexpr juce::uint32 dp4 = Colors::backgroundLight;
        static constexpr juce::uint32 dp8 = 0xff444444;
        static constexpr juce::uint32 dp12 = 0xff555555;
        static constexpr juce::uint32 dp24 = 0xff666666;
    };

    //==========================================================================
    // Design Tokens: Border Radius
    //==========================================================================

    struct Radius {
        static constexpr float none = 0.0f;
        static constexpr float xs = 2.0f;      // Minimal rounding
        static constexpr float s = 4.0f;       // Small controls
        static constexpr float m = 6.0f;       // Standard buttons
        static constexpr float l = 8.0f;       // Cards, panels
        static constexpr float xl = 12.0f;     // Large panels
        static constexpr float round = 999.0f; // Fully rounded

        // Legacy aliases
        static constexpr float radiusS = s;
        static constexpr float radiusL = l;
    };

    ZenithLookAndFeel();
    ~ZenithLookAndFeel() override = default;


  //==========================================================================
  // Typography (San Francisco / System UI font)
  // Sizes follow 12-14-16-18-20-24-32 scale
  //==========================================================================

  struct Typography {
    // Display (rarely used, headers only)
    static juce::Font getDisplay() {
      return juce::FontOptions(32.0f, juce::Font::bold);
    }

    // Headings
    static juce::Font getH1() {
      return juce::FontOptions(24.0f, juce::Font::bold);
    }
    static juce::Font getH2() {
      return juce::FontOptions(20.0f, juce::Font::bold);
    }
    static juce::Font getH3() {
      return juce::FontOptions(18.0f, juce::Font::bold);
    }
    static juce::Font getH4() {
      return juce::FontOptions(16.0f, juce::Font::bold);
    }

    // Body text
    static juce::Font getBody() { return juce::FontOptions(14.0f); } // Primary
    static juce::Font getBodyBold() {
      return juce::FontOptions(14.0f, juce::Font::bold);
    }
    static juce::Font getSmall() {
      return juce::FontOptions(12.0f);
    } // Secondary
    static juce::Font getSmallBold() {
      return juce::FontOptions(12.0f, juce::Font::bold);
    }

    // Special purpose
    static juce::Font getTiny() {
      return juce::FontOptions(10.0f);
    } // Minimum (rarely use)
    static juce::Font getMonospace() {
      return juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f,
                        juce::Font::plain);
    }

    // Line height multiplier (1.5x font size recommended)
    static constexpr float lineHeightMultiplier = 1.5f;
  };

  // DESIGN SYSTEM: Static font accessors for legacy code compatibility
  static juce::Font getFontSmall() { return Typography::getSmall(); }
  static juce::Font getFontBody() { return Typography::getBody(); }
  static juce::Font getFontHeading() { return Typography::getH4(); }
  static juce::Font getFontLarge() { return Typography::getH3(); }
  static juce::Font getFontTiny() { return Typography::getTiny(); }

  //==========================================================================
  // Component Drawing Overrides
  //==========================================================================

  // Buttons
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

  void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

  void drawToggleButton(juce::Graphics &g, juce::ToggleButton &button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

  // Sliders
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override;

  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        juce::Slider::SliderStyle style,
                        juce::Slider &slider) override;

  // ComboBox
  void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &box) override;

  // Labels
  void drawLabel(juce::Graphics &g, juce::Label &label) override;

  // Scrollbars
  void drawScrollbar(juce::Graphics &g, juce::ScrollBar &scrollbar, int x,
                     int y, int width, int height, bool isScrollbarVertical,
                     int thumbStartPosition, int thumbSize, bool isMouseOver,
                     bool isMouseDown) override;

  // Tab buttons
  void drawTabButton(juce::TabBarButton &button, juce::Graphics &g,
                     bool isMouseOver, bool isMouseDown) override;

  // Popup menus
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
  // Helper Methods
  //==========================================================================

  /**
   * @brief Draw rounded rectangle with optional gradient and shadow
   */
  void drawRoundedRect(juce::Graphics &g, const juce::Rectangle<float> &bounds,
                       float cornerSize, const juce::Colour &fillColour,
                       const juce::Colour &strokeColour = juce::Colour(),
                       float strokeWidth = 0.0f);

  /**
   * @brief Draw glow effect (better than shadow for dark mode)
   */
  void drawGlow(juce::Graphics &g, const juce::Rectangle<float> &bounds,
                float cornerSize, const juce::Colour &glowColour,
                float glowSize);

  /**
   * @brief Get button color with proper state management
   */
  juce::Colour getButtonColour(juce::Button &button, bool isHighlighted,
                               bool isDown);

  /**
   * @brief Get track color by index (for color-coded organization)
   */
  static juce::Colour getTrackColor(int index);

  /**
   * @brief Calculate contrast ratio between two colors (WCAG compliance check)
   */
  static float calculateContrastRatio(const juce::Colour &fg,
                                      const juce::Colour &bg);

  /**
   * @brief Ensure text meets WCAG AA (4.5:1) or AAA (7:1) contrast
   */
  static juce::Colour ensureReadableText(const juce::Colour &background,
                                         bool requireAAA = false);

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithLookAndFeel)
};

} // namespace zenith
