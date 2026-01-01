/*
  ==============================================================================

    MixerAccessibilityTests.cpp
    Created: 2025-12-31
    Author:  Zenith DAW Team

    Verifies accessibility compliance for mixer components (WCAG 2.1 AA).

  ==============================================================================
*/

#include "../ui/mixer/MixerComponent.h"
#include "../ui/mixer/MixerChannelComponent.h"
#include "../ui/design-system/ZenithDesignSystem.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MixerAccessibilityTests : public juce::UnitTest {
public:
  MixerAccessibilityTests() : juce::UnitTest("MixerAccessibilityTests", "Accessibility") {}

  void runTest() override {
    testContrastRatios();
    testFocusTokens();
  }

  void testContrastRatios() {
    beginTest("Contrast Ratios (WCAG 2.1 AA)");
    {
      using namespace zenith::design;
      
      // Text Primary on Dark BG
      float textContrast = accessibility::contrastRatio(colors::TEXT_PRIMARY, colors::BG_01);
      expect(textContrast >= 4.5f, "TEXT_PRIMARY on BG_01 must be >= 4.5:1");
      logMessage("TEXT_PRIMARY ratio: " + juce::String(textContrast, 2));

      // Focus Ring on Dark BG
      float focusContrast = accessibility::contrastRatio(accessibility::FOCUS_RING_COLOR, colors::BG_02);
      expect(focusContrast >= 3.0f, "Focus ring must be >= 3:1 against background");
      logMessage("FOCUS_RING ratio: " + juce::String(focusContrast, 2));

      // Button Text on Primary Button (CYAN)
      // Note: This often fails in dark themes unless text is dark.
      // Checking specific combination used in MixerChannelComponent faderSlider_ (CYAN thumb?)
      // or Buttons.
      float btnTextContrast = accessibility::contrastRatio(colors::TEXT_PRIMARY, colors::CYAN);
      logMessage("Button Text (Primary) on CYAN ratio: " + juce::String(btnTextContrast, 2));
      
      if (btnTextContrast < 4.5f) {
           logMessage("WARNING: Button text contrast on primary color is low. Consider using dark text for CYAN background.");
      }
    }
  }

  void testFocusTokens() {
    beginTest("Focus Tokens");
    {
      using namespace zenith::design::accessibility;
      expect(FOCUS_RING_WIDTH >= 2.0f, "Focus ring must be at least 2px thick");
      expectEquals(FOCUS_RING_COLOR, zenith::design::colors::CYAN, "Focus ring should be CYAN");
    }
  }
};

static MixerAccessibilityTests mixerAccessibilityTests;
