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

  ==============================================================================

    ZenithPluginEditor.h
    Created: 2025-12-19
    Author:  Zenith DAW

    Standard UI container for Zenith Plugins.
    Provides a header with Title, Bypass, and generic styling.


  ==============================================================================
*/

#pragma once

#include "ZenithPlugin.h"
#include <juce_audio_processors/juce_audio_processors.h>


namespace zenith {

class ZenithPluginEditor : public juce::AudioProcessorEditor {
public:
  explicit ZenithPluginEditor(ZenithPlugin &p);
  ~ZenithPluginEditor() override;

  //==============================================================================
  void paint(juce::Graphics &g) override;
  void resized() override;

protected:
  ZenithPlugin &pluginProc;

  // Future: Common header components (Title, Bypass Button)
  // juce::Label titleLabel;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPluginEditor)
};

} // namespace zenith
