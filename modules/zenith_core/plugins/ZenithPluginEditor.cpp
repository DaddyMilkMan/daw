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
