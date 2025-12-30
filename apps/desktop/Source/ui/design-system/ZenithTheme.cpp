/**
 * @file ZenithTheme.cpp
 * @brief Implementation of modern theme system
 * @author Fixed by Claude - December 2025
 */

#include "ZenithTheme.h"
#include "ColorBridge.h"
#include <cmath>

namespace zenith {

//==============================================================================
// Dynamic Theme Modification (God Mode)
//==============================================================================

void ZenithTheme::Colors::setColor(const juce::String& colorId, const juce::Colour& color) {
    if (colorId == "bg_00") bg_00 = color;
    else if (colorId == "bg_01") bg_01 = color;
    else if (colorId == "bg_02") bg_02 = color;
    else if (colorId == "bg_03") bg_03 = color;
    else if (colorId == "bg_04") bg_04 = color;
    else if (colorId == "accent_primary") accent_primary = color;
    else if (colorId == "accent_secondary") accent_secondary = color;
    else if (colorId == "waveform_audio") waveform_audio = color;
    else if (colorId == "waveform_midi") waveform_midi = color;
    else if (colorId == "playhead") playhead = color;
    else if (colorId == "text_primary") text_primary = color;
    else if (colorId == "text_secondary") text_secondary = color;
}

void ZenithTheme::setSpacing(const juce::String& key, int value) {
    if (key == "trackHeight") Spacing::trackHeight = value;
    else if (key == "trackHeaderWidth") Spacing::trackHeaderWidth = value;
    else if (key == "mixerWidth") Spacing::mixerWidth = value;
    else if (key == "sidebarWidth") Spacing::sidebarWidth = value;
    else if (key == "toolbarHeight") Spacing::toolbarHeight = value;
    
    // Trigger global UI refresh
    if (auto* mm = juce::MessageManager::getInstance()) {
        mm->callAsync([]() {
            for (int i = 0; i < juce::TopLevelWindow::getNumTopLevelWindows(); ++i)
                if (auto* win = juce::TopLevelWindow::getTopLevelWindow(i))
                    win->resized();
        });
    }
}

//==============================================================================
// Utility Functions
//==============================================================================

juce::Colour ZenithTheme::Colors::getTrackColor(int index, float saturation,
                                                float brightness) {
  float goldenRatio = 0.618033988749895f;
  float hue = std::fmod(index * goldenRatio, 1.0f);
  return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
}

juce::Colour ZenithTheme::Colors::withAlpha(const juce::Colour &color, float alpha) {
  return color.withAlpha(juce::jlimit(0.0f, 1.0f, alpha));
}

juce::Colour ZenithTheme::Colors::lighten(const juce::Colour &color, float amount) {
  return color.brighter(amount);
}

juce::Colour ZenithTheme::Colors::darken(const juce::Colour &color, float amount) {
  return color.darker(amount);
}

//==============================================================================
// Typography System
//==============================================================================

juce::Font ZenithTheme::Typography::getFont(float size, Weight weight) {
  auto fontName = juce::Font::getDefaultSansSerifFontName();
  switch (weight) {
  case Weight::Regular: return juce::Font(juce::FontOptions(fontName, size, juce::Font::plain));
  case Weight::Medium: return juce::Font(juce::FontOptions(fontName, size, juce::Font::plain)).withExtraKerningFactor(0.05f);
  case Weight::Bold: return juce::Font(juce::FontOptions(fontName, size, juce::Font::bold));
  default: return juce::Font(juce::FontOptions(fontName, size, juce::Font::plain));
  }
}

juce::Font ZenithTheme::Typography::getTinyFont(Weight weight) { return getFont(tiny, weight); }
juce::Font ZenithTheme::Typography::getSmallFont(Weight weight) { return getFont(sm, weight); }
juce::Font ZenithTheme::Typography::getBodyFont(Weight weight) { return getFont(body, weight); }
juce::Font ZenithTheme::Typography::getHeadingFont(Weight weight) { return getFont(heading, weight); }
juce::Font ZenithTheme::Typography::getLargeFont(Weight weight) { return getFont(large, weight); }
juce::Font ZenithTheme::Typography::getDisplayFont(Weight weight) { return getFont(display, weight); }

//==============================================================================
// Shadow System
//==============================================================================

void ZenithTheme::Shadows::drawShadow(juce::Graphics &g, juce::Rectangle<float> bounds, float elevation, float radius) {
  int layers = static_cast<int>(elevation * 20.0f) + 1;
  for (int i = 0; i < layers; ++i) {
    float layerAlpha = elevation * 0.5f / (i + 1);
    float layerOffset = (i + 1) * 2.0f;
    float layerBlur = (i + 1) * 3.0f;
    auto shadowBounds = bounds.translated(0, layerOffset).expanded(layerBlur);
    g.setColour(juce::Colours::black.withAlpha(layerAlpha));
    if (radius > 0.0f) g.fillRoundedRectangle(shadowBounds, radius);
    else g.fillRect(shadowBounds);
  }
}

void ZenithTheme::Shadows::drawInnerShadow(juce::Graphics &g, juce::Rectangle<float> bounds, float radius) {
  g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.3f), bounds.getTopLeft(),
      juce::Colours::transparentBlack, bounds.getBottomRight(), false));
  if (radius > 0.0f) g.fillRoundedRectangle(bounds.reduced(1.0f), radius);
  else g.fillRect(bounds.reduced(1.0f));
}

//==============================================================================
// Theme Mode Management
//==============================================================================

ZenithTheme::Mode ZenithTheme::currentMode_ = ZenithTheme::Mode::Standard;
void ZenithTheme::setMode(Mode mode) { currentMode_ = mode; }
ZenithTheme::Mode ZenithTheme::getMode() { return currentMode_; }

} // namespace zenith