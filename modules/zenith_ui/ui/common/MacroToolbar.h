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

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include "SkiaComponent.h"
#include "controls/ZenithButton.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>



namespace zenith {

struct MacroCommand {
  juce::String name;
  juce::String description;
  juce::String iconChar; // Emoji or unicode icon
  std::function<void(Engine &, ProjectState &)> action;
};

class MacroToolbar : public SkiaComponent {
public:
  MacroToolbar(Engine &engine, ProjectState &projectState);
  ~MacroToolbar() override;

  void resized() override;
  void paint(juce::Graphics &g) override; // Fallback or minimal
  void drawSkia(SkCanvas *canvas) override;

  // Mouse methods for auto-hide
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

  void timerCallback() override;

  // Callbacks to access Arranger context
  std::function<juce::StringArray()> getSelectedClipIds;
  std::function<juce::String()> getSelectedTrackId;
  std::function<void(float, const juce::String &)> onFreezeProgress;

  // Proximity check from parent
  void checkProximity(juce::Point<float> mousePosInParent);

private:
  Engine &engine_;
  ProjectState &projectState_;

  std::vector<MacroCommand> macros_;
  juce::OwnedArray<ZenithButton> buttons_;

  // Animation state for auto-hide
  float targetOpacity_ = 0.0f;
  float currentOpacity_ = 0.0f;
  bool isHovered_ = false;

  // Layout constants
  static constexpr float kPillHeight = 48.0f;
  static constexpr float kButtonSpacing = 8.0f;
  static constexpr float kPillPadding = 8.0f;

  void rebuildButtons();
  void executeMacro(int index);

  // Default macros implementation
  void healSplits();
  void instantFreeze();
  void colorByTrack();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroToolbar)
};

} // namespace zenith
