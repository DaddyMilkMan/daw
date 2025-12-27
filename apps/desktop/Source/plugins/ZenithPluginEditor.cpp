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
  // Neon Noir Background
  g.fillAll(juce::Colour(0xFF0D0D11)); // Deep dark background

  // Header
  auto area = getLocalBounds();
  auto header = area.removeFromTop(40);
  g.setColour(juce::Colour(0xFF1A1A22));
  g.fillRect(header);

  // Title
  g.setColour(juce::Colours::white);
  g.setFont(18.0f);
  g.drawText(pluginProc.getName(), header.reduced(10, 0),
             juce::Justification::centredLeft, true);

  // Border
  g.setColour(juce::Colour(0xFF2D2D35));
  g.drawRect(getLocalBounds(), 1);
}

void ZenithPluginEditor::resized() {
  // Layout common components here
}

} // namespace zenith
