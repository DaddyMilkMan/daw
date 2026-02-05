/*
  ==============================================================================

    ZenithStyleApplicator.cpp
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Implementation of the style applicator utility.

  ==============================================================================
*/

#include "ZenithStyleApplicator.h"
#include "../ui/legacy/ZenithLookAndFeel.h"
#include "../ui/framework/SkiaComponent.h"
#include <typeinfo>


namespace zenith {
namespace ai {

//==============================================================================
// Global LookAndFeel Instance
//==============================================================================

static ZenithLookAndFeel &getGlobalZenithLAF() {
  static ZenithLookAndFeel instance;
  return instance;
}

juce::LookAndFeel &ZenithStyleApplicator::getZenithLookAndFeel() {
  return getGlobalZenithLAF();
}

//==============================================================================
// Main API
//==============================================================================

StyleApplicationResult
ZenithStyleApplicator::applyToComponent(juce::Component *component) {
  if (component == nullptr) {
    return {false, "", "Component is null"};
  }

  // Check if already styled
  if (isAlreadyStyled(component)) {
    return {true, "AlreadyStyled", "Component already has Zenith styling"};
  }

  // Try SkiaComponent first (most common in Zenith)
  if (auto *skiaComp = dynamic_cast<SkiaComponent *>(component)) {
    // Apply neon glow with design system color
    if (applySkiaGlow(skiaComp, design::colors::NEON_GREEN)) {
      return {true, "SkiaGlow", "Applied Skia glow effect"};
    }
  }

  // Try specific JUCE component types
  if (auto *button = dynamic_cast<juce::Button *>(component)) {
    return styleButton(button);
  }

  if (auto *slider = dynamic_cast<juce::Slider *>(component)) {
    return styleSlider(slider);
  }

  if (auto *editor = dynamic_cast<juce::TextEditor *>(component)) {
    return styleTextEditor(editor);
  }

  if (auto *comboBox = dynamic_cast<juce::ComboBox *>(component)) {
    return styleComboBox(comboBox);
  }

  // Generic fallback: apply LookAndFeel
  if (applyZenithLookAndFeel(component)) {
    return {true, "ZenithLookAndFeel", "Applied ZenithLookAndFeel"};
  }

  // Cannot style this component type
  return {false, "",
          "Unsupported component type: " +
              juce::String(typeid(*component).name())};
}

//==============================================================================
// Skia Styling
//==============================================================================

bool ZenithStyleApplicator::applySkiaGlow(SkiaComponent *component,
                                          uint32_t glowColor,
                                          float glowRadius) {
  if (component == nullptr) {
    return false;
  }

  component->setGlowEnabled(true);
  component->setGlowColor(static_cast<SkColor>(glowColor));
  component->setGlowRadius(glowRadius);
  component->markDirty();

  return true;
}

//==============================================================================
// LookAndFeel Application
//==============================================================================

bool ZenithStyleApplicator::applyZenithLookAndFeel(juce::Component *component) {
  if (component == nullptr) {
    return false;
  }

  component->setLookAndFeel(&getGlobalZenithLAF());
  component->repaint();

  return true;
}

//==============================================================================
// Component-Specific Styling
//==============================================================================

StyleApplicationResult
ZenithStyleApplicator::styleButton(juce::Button *button) {
  if (button == nullptr) {
    return {false, "", "Button is null"};
  }

  // Apply ZenithLookAndFeel
  button->setLookAndFeel(&getGlobalZenithLAF());

  // Set Zenith design system colors
  button->setColour(juce::TextButton::buttonColourId,
                    juce::Colour(design::colors::BG_DARK));
  button->setColour(juce::TextButton::buttonOnColourId,
                    juce::Colour(design::colors::NEON_GREEN));
  button->setColour(juce::TextButton::textColourOffId,
                    juce::Colour(design::colors::TEXT_PRIMARY));
  button->setColour(juce::TextButton::textColourOnId,
                    juce::Colour(design::colors::BG_DARKER));

  button->repaint();

  return {true, "ZenithButton", "Applied Zenith button styling"};
}

StyleApplicationResult
ZenithStyleApplicator::styleSlider(juce::Slider *slider) {
  if (slider == nullptr) {
    return {false, "", "Slider is null"};
  }

  slider->setLookAndFeel(&getGlobalZenithLAF());

  // Set Zenith design system colors
  slider->setColour(juce::Slider::backgroundColourId,
                    juce::Colour(design::colors::BG_DARKER));
  slider->setColour(juce::Slider::trackColourId,
                    juce::Colour(design::colors::NEON_GREEN));
  slider->setColour(juce::Slider::thumbColourId,
                    juce::Colour(design::colors::TEXT_PRIMARY));

  slider->repaint();

  return {true, "ZenithSlider", "Applied Zenith slider styling"};
}

StyleApplicationResult
ZenithStyleApplicator::styleTextEditor(juce::TextEditor *editor) {
  if (editor == nullptr) {
    return {false, "", "TextEditor is null"};
  }

  editor->setLookAndFeel(&getGlobalZenithLAF());

  // Set Zenith design system colors
  editor->setColour(juce::TextEditor::backgroundColourId,
                    juce::Colour(design::colors::BG_DARKER));
  editor->setColour(juce::TextEditor::textColourId,
                    juce::Colour(design::colors::TEXT_PRIMARY));
  editor->setColour(juce::TextEditor::highlightColourId,
                    juce::Colour(design::colors::NEON_GREEN).withAlpha(0.3f));
  editor->setColour(juce::TextEditor::outlineColourId,
                    juce::Colour(design::colors::BORDER_SUBTLE));
  editor->setColour(juce::TextEditor::focusedOutlineColourId,
                    juce::Colour(design::colors::NEON_GREEN));

  editor->repaint();

  return {true, "ZenithTextEditor", "Applied Zenith text editor styling"};
}

StyleApplicationResult
ZenithStyleApplicator::styleComboBox(juce::ComboBox *comboBox) {
  if (comboBox == nullptr) {
    return {false, "", "ComboBox is null"};
  }

  comboBox->setLookAndFeel(&getGlobalZenithLAF());

  comboBox->setColour(juce::ComboBox::backgroundColourId,
                      juce::Colour(design::colors::BG_DARK));
  comboBox->setColour(juce::ComboBox::textColourId,
                      juce::Colour(design::colors::TEXT_PRIMARY));
  comboBox->setColour(juce::ComboBox::outlineColourId,
                      juce::Colour(design::colors::BORDER_SUBTLE));
  comboBox->setColour(juce::ComboBox::arrowColourId,
                      juce::Colour(design::colors::NEON_GREEN));

  comboBox->repaint();

  return {true, "ZenithComboBox", "Applied Zenith combo box styling"};
}

//==============================================================================
// Utilities
//==============================================================================

bool ZenithStyleApplicator::isAlreadyStyled(juce::Component *component) {
  if (component == nullptr) {
    return false;
  }

  // Check if using ZenithLookAndFeel
  auto &currentLAF = component->getLookAndFeel();
  if (dynamic_cast<ZenithLookAndFeel *>(&currentLAF) != nullptr) {
    return true;
  }

  // Check if it's a SkiaComponent with glow enabled
  if (auto *skiaComp = dynamic_cast<SkiaComponent *>(component)) {
    if (skiaComp->isGlowEnabled()) {
      return true;
    }
  }

  return false;
}

} // namespace ai
} // namespace zenith
