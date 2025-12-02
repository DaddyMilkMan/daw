/**
 * @file ZenithTheme.cpp
 * @brief Implementation of centralized theme system
 * @author Marcus "The Craftsman" Rodriguez - Operation Polish
 */

#include "ZenithTheme.h"

namespace zenith {

//==============================================================================
// Color Palette Definitions
//==============================================================================

const juce::Colour ZenithTheme::Colors::background         = juce::Colour(0xff121212);
const juce::Colour ZenithTheme::Colors::backgroundPanel    = juce::Colour(0xff1e1e1e);
const juce::Colour ZenithTheme::Colors::backgroundElevated = juce::Colour(0xff2a2a2a);

const juce::Colour ZenithTheme::Colors::border             = juce::Colour(0xff333333);
const juce::Colour ZenithTheme::Colors::borderSubtle       = juce::Colour(0xff444444);

const juce::Colour ZenithTheme::Colors::textPrimary        = juce::Colour(0xffffffff);
const juce::Colour ZenithTheme::Colors::textSecondary      = juce::Colour(0xffb0b0b0);
const juce::Colour ZenithTheme::Colors::textMuted          = juce::Colour(0xff808080);

const juce::Colour ZenithTheme::Colors::accent             = juce::Colour(0xff00d4aa);
const juce::Colour ZenithTheme::Colors::accentHover        = juce::Colour(0xff00ffcc);
const juce::Colour ZenithTheme::Colors::accentPressed      = juce::Colour(0xff00a088);

const juce::Colour ZenithTheme::Colors::success            = juce::Colour(0xff4ade80);
const juce::Colour ZenithTheme::Colors::warning            = juce::Colour(0xfffbbf24);
const juce::Colour ZenithTheme::Colors::error              = juce::Colour(0xfff87171);
const juce::Colour ZenithTheme::Colors::info               = juce::Colour(0xff60a5fa);

const juce::Colour ZenithTheme::Colors::waveform           = juce::Colour(0xff4a9eff);
const juce::Colour ZenithTheme::Colors::midiNote           = juce::Colour(0xffff9944);
const juce::Colour ZenithTheme::Colors::automation         = juce::Colour(0xffa855f7);

//==============================================================================
// Typography Helpers
//==============================================================================

juce::Font ZenithTheme::Typography::getTinyFont() {
    return juce::FontOptions(tiny);
}

juce::Font ZenithTheme::Typography::getSmallFont() {
    return juce::FontOptions(small);
}

juce::Font ZenithTheme::Typography::getBodyFont() {
    return juce::FontOptions(body);
}

juce::Font ZenithTheme::Typography::getHeadingFont() {
    return juce::FontOptions(heading, juce::Font::bold);
}

juce::Font ZenithTheme::Typography::getLargeFont() {
    return juce::FontOptions(large, juce::Font::bold);
}

juce::Font ZenithTheme::Typography::getDisplayFont() {
    return juce::FontOptions(display, juce::Font::bold);
}

//==============================================================================
// Theme Mode Management
//==============================================================================

ZenithTheme::Mode ZenithTheme::currentMode_ = ZenithTheme::Mode::Standard;

void ZenithTheme::setMode(Mode mode) {
    currentMode_ = mode;
    
    // Apply mode-specific adjustments
    // (In future, this could modify color values dynamically)
}

ZenithTheme::Mode ZenithTheme::getMode() {
    return currentMode_;
}

} // namespace zenith
