/*
  ==============================================================================

    GlobalSettingsPanel.cpp
    Created: 2026-01-18
    Author:  Zenith DAW Team

    Placeholder for GlobalSettingsPanel - stub implementation

  ==============================================================================
*/

#include "GlobalSettingsPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"

namespace zenith {

GlobalSettingsPanel::GlobalSettingsPanel() = default;

GlobalSettingsPanel::~GlobalSettingsPanel() = default;

void GlobalSettingsPanel::paint(juce::Graphics& g) {
    g.fillAll(design::toJuceColour(design::colors::BG_DARK));
}

void GlobalSettingsPanel::resized() {
    // Stub implementation
}

} // namespace zenith
