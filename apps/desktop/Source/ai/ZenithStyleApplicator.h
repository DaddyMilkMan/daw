/*
  ==============================================================================

    ZenithStyleApplicator.h
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Utility class for applying Zenith design system styling to components.
    Used by UXDirectorAgent to fix unstyled components.

  ==============================================================================
*/

#pragma once

#include "../ui/design-system/ZenithDesignSystem.h"
#include "../ui/design-system/ZenithLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Forward declarations
namespace zenith {
class SkiaComponent;
}

namespace zenith {
namespace ai {

//==============================================================================
/**
    Result of a style application attempt
*/
struct StyleApplicationResult {
  bool success = false;
  juce::String appliedStyle; // e.g. "Skia Glow", "ZenithLookAndFeel"
  juce::String reason;       // Failure reason if !success
};

//==============================================================================
/**
    Utility class for applying Zenith design system styling to components.

    This handles:
    - SkiaComponent: Enable glow effects, set appropriate colors
    - JUCE Buttons: Apply ZenithLookAndFeel
    - JUCE Sliders: Apply ZenithLookAndFeel
    - JUCE TextEditors: Apply ZenithLookAndFeel

    Usage:
      auto result = ZenithStyleApplicator::applyToComponent(someComponent);
      if (result.success) {
        // Component now styled
      }
*/
class ZenithStyleApplicator {
public:
  //============================================================================
  // Main API
  //============================================================================

  /**
   * Apply Zenith styling to a component.
   * Automatically detects component type and applies appropriate style.
   *
   * @param component The component to style
   * @return Result indicating success/failure and what was applied
   */
  static StyleApplicationResult applyToComponent(juce::Component *component);

  /**
   * Apply Skia glow effect to a SkiaComponent.
   *
   * @param component The SkiaComponent to style
   * @param glowColor Color for the glow effect
   * @param glowRadius Radius of the glow
   * @return True if successful
   */
  static bool applySkiaGlow(SkiaComponent *component, uint32_t glowColor,
                            float glowRadius = 8.0f);

  /**
   * Apply ZenithLookAndFeel to a standard JUCE component.
   *
   * @param component The component to style
   * @return True if LookAndFeel was applied
   */
  static bool applyZenithLookAndFeel(juce::Component *component);

  //============================================================================
  // Component-Specific Styling
  //============================================================================

  /**
   * Style a button with Zenith design.
   */
  static StyleApplicationResult styleButton(juce::Button *button);

  /**
   * Style a slider with Zenith design.
   */
  static StyleApplicationResult styleSlider(juce::Slider *slider);

  /**
   * Style a text editor with Zenith design.
   */
  static StyleApplicationResult styleTextEditor(juce::TextEditor *editor);

  /**
   * Style a combobox with Zenith design.
   */
  static StyleApplicationResult styleComboBox(juce::ComboBox *comboBox);

  //============================================================================
  // Utilities
  //============================================================================

  /**
   * Check if a component is already styled with Zenith design.
   */
  static bool isAlreadyStyled(juce::Component *component);

  /**
   * Get the global ZenithLookAndFeel instance.
   */
  static juce::LookAndFeel &getZenithLookAndFeel();

private:
  ZenithStyleApplicator() = delete; // Static-only class
};

} // namespace ai
} // namespace zenith
