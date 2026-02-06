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

#pragma once

#include "Engine.h" // Corrected include path
#include "ProjectState.h"
#include "../../engine/Track.h"
#include "SkiaComponent.h"
#include "SkiaKnob.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "ZenithSkia.h"
#include <vector>


class Engine; // Forward declaration

namespace zenith {

class DeviceSlotComponent;

class DeviceChainComponent : public SkiaComponent,
                             public juce::ValueTree::Listener {
public:
  DeviceChainComponent(Engine &engine, ProjectState &state);
  ~DeviceChainComponent() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  // ValueTree::Listener overrides
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeParentChanged(juce::ValueTree &tree) override;

  void setTrack(Track *track);

private:
  Engine &engine_;
  ProjectState &projectState_;
  Track *currentTrack_ = nullptr;

  // UI Resources
  ::SkPaint bgPaint_;
  ::SkFont labelFont_;

  juce::Viewport viewport_;
  std::unique_ptr<juce::Component> contentContainer_;
  std::vector<std::unique_ptr<DeviceSlotComponent>> deviceSlots_;

  void updateTrackFromSelection();
  void rebuildSlots();
  Track *findTrackById(const juce::String &trackId);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceChainComponent)
};

} // namespace zenith
