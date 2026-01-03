/*
  ==============================================================================

    ZenithPluginEditor.cpp
    Created: 2025-12-19
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithPluginEditor.h"

namespace zenith {

ZenithPluginEditor::ZenithPluginEditor(ZenithPlugin &p)
    : AudioProcessorEditor(&p), pluginProc(p) {
  // Default size
  setSize(400, 300);
}

ZenithPluginEditor::~ZenithPluginEditor() {}

//==============================================================================
void ZenithPluginEditor::paint(juce::Graphics &g) {
  // Rendering handled by Skia in parent window integration
  juce::ignoreUnused(g);
}

void ZenithPluginEditor::resized() {
  // Layout common components here
}

} // namespace zenith
