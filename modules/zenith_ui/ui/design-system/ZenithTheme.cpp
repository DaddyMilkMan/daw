/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

 * @file ZenithTheme.cpp
 * @brief Implementation of modern theme system
 * @author Fixed by Claude - December 2025
 */


namespace zenith {

//==============================================================================
// Modern Color Palette Definitions (Sourced from ZenithDesignSystem)
//==============================================================================

// Background layers
const juce::Colour ZenithTheme::Colors::bg_00 = design::toJuceColour(design::colors::BG_00);
const juce::Colour ZenithTheme::Colors::bg_01 = design::toJuceColour(design::colors::BG_01);
const juce::Colour ZenithTheme::Colors::bg_02 = design::toJuceColour(design::colors::BG_02);
const juce::Colour ZenithTheme::Colors::bg_03 = design::toJuceColour(design::colors::BG_03);
const juce::Colour ZenithTheme::Colors::bg_04 = design::toJuceColour(design::colors::BG_04);

// Borders with proper opacity
const juce::Colour ZenithTheme::Colors::border_subtle = design::toJuceColour(design::colors::BORDER_SUBTLE);
const juce::Colour ZenithTheme::Colors::border_default = design::toJuceColour(design::colors::BORDER_DEFAULT);
const juce::Colour ZenithTheme::Colors::border_strong = design::toJuceColour(design::colors::BORDER_STRONG);
const juce::Colour ZenithTheme::Colors::border_focus = design::toJuceColour(design::colors::BORDER_FOCUS);

// Text hierarchy
const juce::Colour ZenithTheme::Colors::text_primary = design::toJuceColour(design::colors::TEXT_PRIMARY);
const juce::Colour ZenithTheme::Colors::text_secondary = design::toJuceColour(design::colors::TEXT_SECONDARY);
const juce::Colour ZenithTheme::Colors::text_tertiary = design::toJuceColour(design::colors::TEXT_TERTIARY);
const juce::Colour ZenithTheme::Colors::text_inverse = design::toJuceColour(design::colors::TEXT_INVERSE);

// Primary accent (unified with Neon Noir design system)
const juce::Colour ZenithTheme::Colors::accent_primary = design::toJuceColour(design::colors::ACCENT_PRIMARY);
const juce::Colour ZenithTheme::Colors::accent_secondary = design::toJuceColour(design::colors::ACCENT_SECONDARY);
const juce::Colour ZenithTheme::Colors::accent_hover = design::toJuceColour(design::colors::ACCENT_PRIMARY).brighter(0.2f);
const juce::Colour ZenithTheme::Colors::accent_pressed = design::toJuceColour(design::colors::ACCENT_PRIMARY).darker(0.1f);
const juce::Colour ZenithTheme::Colors::accent_subtle = design::toJuceColour(design::colors::ACCENT_PRIMARY).withAlpha(0.10f);
const juce::Colour ZenithTheme::Colors::hover_overlay = design::toJuceColour(design::colors::GLASS_HOVER);

// Semantic colors
const juce::Colour ZenithTheme::Colors::success = design::toJuceColour(design::colors::SUCCESS);
const juce::Colour ZenithTheme::Colors::warning = design::toJuceColour(design::colors::WARNING);
const juce::Colour ZenithTheme::Colors::error = design::toJuceColour(design::colors::DANGER);
const juce::Colour ZenithTheme::Colors::info = design::toJuceColour(design::colors::INFO);

// Audio-specific colors
const juce::Colour ZenithTheme::Colors::waveform_audio = design::toJuceColour(design::colors::WAVEFORM_AUDIO);
const juce::Colour ZenithTheme::Colors::waveform_midi = design::toJuceColour(design::colors::WAVEFORM_MIDI);
const juce::Colour ZenithTheme::Colors::automation = design::toJuceColour(design::colors::AUTOMATION);
const juce::Colour ZenithTheme::Colors::playhead = design::toJuceColour(design::colors::PLAYHEAD);

// Track color generation using golden ratio for even distribution
juce::Colour ZenithTheme::Colors::getTrackColor(int index, float saturation,
                                                float brightness) {
  // Golden ratio (1.618...) provides optimal color distribution
  float goldenRatio = 0.618033988749895f;
  float hue = std::fmod(index * goldenRatio, 1.0f);

  return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
}

juce::Colour ZenithTheme::Colors::withAlpha(const juce::Colour &color,
                                            float alpha) {
  return color.withAlpha(juce::jlimit(0.0f, 1.0f, alpha));
}

juce::Colour ZenithTheme::Colors::lighten(const juce::Colour &color,
                                          float amount) {
  return color.brighter(amount);
}

juce::Colour ZenithTheme::Colors::darken(const juce::Colour &color,
                                         float amount) {
  return color.darker(amount);
}

//==============================================================================
// Typography System
//==============================================================================

juce::Font ZenithTheme::Typography::getFont(float size, Weight weight) {
  juce::FontOptions options;
  options = options.withHeight(size);
  options = options.withName("Inter");
  
  switch (weight) {
  case Weight::Regular:
     break;
  case Weight::Medium:
    options = options.withStyle("Medium");
    break;
  case Weight::Bold:
    options = options.withStyle("Bold");
    break;
  }
  
  return juce::Font(options);
}

juce::Font ZenithTheme::Typography::getTinyFont(Weight weight) {
  return getFont(tiny, weight);
}

juce::Font ZenithTheme::Typography::getSmallFont(Weight weight) {
  return getFont(sm, weight);
}

juce::Font ZenithTheme::Typography::getBodyFont(Weight weight) {
  return getFont(body, weight);
}

juce::Font ZenithTheme::Typography::getHeadingFont(Weight weight) {
  return getFont(heading, weight);
}

juce::Font ZenithTheme::Typography::getLargeFont(Weight weight) {
  return getFont(large, weight);
}

juce::Font ZenithTheme::Typography::getDisplayFont(Weight weight) {
  return getFont(display, weight);
}

//==============================================================================
// Shadow System Implementation
//==============================================================================

void ZenithTheme::Shadows::drawShadow(juce::Graphics &g,
                                      juce::Rectangle<float> bounds,
                                      float elevation, float radius) {
  // Multi-layer shadows for realistic depth
  int layers = static_cast<int>(elevation * 20.0f) + 1;

  for (int i = 0; i < layers; ++i) {
    float layerAlpha = elevation * 0.5f / (i + 1);
    float layerOffset = (i + 1) * 2.0f;
    float layerBlur = (i + 1) * 3.0f;

    auto shadowBounds = bounds.translated(0, layerOffset).expanded(layerBlur);

    g.setColour(juce::Colours::black.withAlpha(layerAlpha));

    if (radius > 0.0f) {
      g.fillRoundedRectangle(shadowBounds, radius);
    } else {
      g.fillRect(shadowBounds);
    }
  }
}

void ZenithTheme::Shadows::drawInnerShadow(juce::Graphics &g,
                                           juce::Rectangle<float> bounds,
                                           float radius) {
  // Inner shadow for recessed appearance
  g.setGradientFill(juce::ColourGradient(
      juce::Colours::black.withAlpha(0.3f), bounds.getTopLeft(),
      juce::Colours::transparentBlack, bounds.getBottomRight(), false));

  if (radius > 0.0f) {
    g.fillRoundedRectangle(bounds.reduced(1.0f), radius);
  } else {
    g.fillRect(bounds.reduced(1.0f));
  }
}

//==============================================================================
// Theme Mode Management
//==============================================================================

ZenithTheme::Mode ZenithTheme::currentMode_ = ZenithTheme::Mode::Standard;

void ZenithTheme::setMode(Mode mode) {
  currentMode_ = mode;

  // Future: Dynamically adjust colors based on mode
  // For OLED mode, pure blacks
  // For HighContrast mode, increased contrast ratios
}

ZenithTheme::Mode ZenithTheme::getMode() { return currentMode_; }

} // namespace zenith
