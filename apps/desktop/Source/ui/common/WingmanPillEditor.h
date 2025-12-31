/*
  ==============================================================================

    WingmanPillEditor.h
    Created: 2025-12-30 (Refined for Glassmorphism)

    Custom pill-shaped text editor for Wingman AI.
    Features:
    - Pill shape with fully rounded ends
    - Hairline border (0.5px)
    - Subtle glassmorphism background
    - Thin cyan focus glow

  ==============================================================================
*/

#pragma once

#include "../design-system/ZenithDesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

class WingmanPillEditor : public juce::TextEditor {
public:
  // Callback for return key
  std::function<void()> onReturnKey;

  WingmanPillEditor() {
    setMultiLine(false);
    setReturnKeyStartsNewLine(false);
    setReadOnly(false);
    setScrollbarsShown(false);
    setCaretVisible(true);
    setPopupMenuEnabled(true);

    // Use design system colors
    setColour(juce::TextEditor::ColourIds::backgroundColourId,
              juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::outlineColourId,
              juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::focusedOutlineColourId,
              juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::textColourId,
              juce::Colour(design::colors::TEXT_PRIMARY));
    setColour(juce::TextEditor::ColourIds::highlightColourId,
              juce::Colour(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.3f)));
    setColour(juce::CaretComponent::caretColourId,
              juce::Colour(design::colors::ACCENT_PRIMARY));

    setFont(design::typography::getJuceFont(design::typography::FONT_MD));
    setTextToShowWhenEmpty("Ask Wingman...",
                           juce::Colour(design::colors::TEXT_TERTIARY));
  }

  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds().toFloat();
    float radius = bounds.getHeight() * 0.5f;  // Full pill shape

    //==========================================================================
    // 1. Background (Glassmorphism)
    //==========================================================================
    g.setColour(juce::Colour(design::withAlpha(design::colors::BG_02, 0.5f)));
    g.fillRoundedRectangle(bounds, radius);

    //==========================================================================
    // 2. Hairline Border (0.5px) with Focus State
    //==========================================================================
    bool hasFocus = hasKeyboardFocus(true);

    if (hasFocus) {
      // Thin cyan glow on focus
      g.setColour(
          juce::Colour(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.3f)));
      g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);
    } else {
      // Hairline border when not focused
      g.setColour(
          juce::Colour(design::withAlpha(design::colors::BORDER_SUBTLE, 0.4f)));
      g.drawRoundedRectangle(bounds.reduced(0.25f), radius, 0.5f);
    }

    // Call base for text rendering
    juce::TextEditor::paint(g);
  }

  void returnPressed() override {
    if (onReturnKey)
      onReturnKey();
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPillEditor)
};

} // namespace zenith
