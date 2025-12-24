/**
 * @file ZenithTheme.cpp
 * @brief Implementation of modern theme system
 * @author Fixed by Claude - December 2025
 */

#include "ZenithTheme.h"
#include "../../engine/ZenithLogger.h"
#include "FontManager.h"
#include <cmath>

namespace zenith {

//==============================================================================
// Modern Color Palette Definitions
//==============================================================================

// Background layers
const juce::Colour ZenithTheme::Colors::bg_00 = juce::Colour(0xff0a0a0a);
const juce::Colour ZenithTheme::Colors::bg_01 = juce::Colour(0xff121212);
const juce::Colour ZenithTheme::Colors::bg_02 = juce::Colour(0xff1a1a1a);
const juce::Colour ZenithTheme::Colors::bg_03 = juce::Colour(0xff242424);
const juce::Colour ZenithTheme::Colors::bg_04 = juce::Colour(0xff2e2e2e);

// Borders with proper opacity
const juce::Colour ZenithTheme::Colors::border_subtle =
    juce::Colour(0xffffffff).withAlpha(0.06f);
const juce::Colour ZenithTheme::Colors::border_default =
    juce::Colour(0xffffffff).withAlpha(0.12f);
const juce::Colour ZenithTheme::Colors::border_strong =
    juce::Colour(0xffffffff).withAlpha(0.20f);
const juce::Colour ZenithTheme::Colors::border_focus = juce::Colour(0xff3b82f6);

// Text hierarchy
const juce::Colour ZenithTheme::Colors::text_primary =
    juce::Colour(0xffffffff).withAlpha(0.95f);
const juce::Colour ZenithTheme::Colors::text_secondary =
    juce::Colour(0xffffffff).withAlpha(0.60f);
const juce::Colour ZenithTheme::Colors::text_tertiary =
    juce::Colour(0xffffffff).withAlpha(0.35f);
const juce::Colour ZenithTheme::Colors::text_inverse =
    juce::Colour(0xff000000).withAlpha(0.90f);

// Professional blue accent (not garish cyan)
const juce::Colour ZenithTheme::Colors::accent_primary =
    juce::Colour(0xff3b82f6);
const juce::Colour ZenithTheme::Colors::accent_hover = juce::Colour(0xff60a5fa);
const juce::Colour ZenithTheme::Colors::accent_pressed =
    juce::Colour(0xff2563eb);
const juce::Colour ZenithTheme::Colors::accent_subtle =
    juce::Colour(0xff3b82f6).withAlpha(0.10f);

// Semantic colors
const juce::Colour ZenithTheme::Colors::success = juce::Colour(0xff10b981);
const juce::Colour ZenithTheme::Colors::warning = juce::Colour(0xfff59e0b);
const juce::Colour ZenithTheme::Colors::error = juce::Colour(0xffef4444);
const juce::Colour ZenithTheme::Colors::info = juce::Colour(0xff06b6d4);

// Audio-specific colors
const juce::Colour ZenithTheme::Colors::waveform_audio =
    juce::Colour(0xff3b82f6);
const juce::Colour ZenithTheme::Colors::waveform_midi =
    juce::Colour(0xff8b5cf6);
const juce::Colour ZenithTheme::Colors::automation = juce::Colour(0xffec4899);
const juce::Colour ZenithTheme::Colors::playhead = juce::Colour(0xfff97316);

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
    // Determine internal weight and family mapping
    design::FontWeight fw = design::FontWeight::Regular;
    design::FontFamily ff = design::FontFamily::UI;

    switch (weight) {
        case Weight::Regular: fw = design::FontWeight::Regular; break;
        case Weight::Medium:  fw = design::FontWeight::Medium; break;
        case Weight::Bold:    fw = design::FontWeight::Bold; break;
        default: break;
    }

    // Use FontManager to get the JUCE Typeface (bypassing system lookup)
    auto typeface = design::FontManager::getInstance().getJuceTypeface(ff, fw);

    if (typeface != nullptr) {
        return juce::Font(typeface).withHeight(size);
    }

    // Fallback (should not happen if FontManager initialized correctly)
    auto fontName = juce::Font::getDefaultSansSerifFontName();
    return juce::Font(fontName, size, juce::Font::plain);
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
