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

    ZenithParameterAttachment.h
    Created: 2025-12-23
    Author:  Zenith DAW

    A bridge between ZenithControl and JUCE's AudioProcessorValueTreeState
  parameters. Allows ZenithControl (which doesn't inherit from juce::Slider) to
  be easily attached to parameters with full undo/redo support.


  ==============================================================================
*/

#pragma once

#include "../controls/ZenithControl.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

/**
 * Manages the connection between a ZenithControl and an
 * AudioProcessorParameter. Handles thread-safe value synchronization and
 * gesture tracking (undo/redo).
 */
class ZenithParameterAttachment {
public:
  ZenithParameterAttachment(juce::RangedAudioParameter &param,
                            ZenithControl &control,
                            juce::UndoManager *undoManager = nullptr)
      : control_(control),
        attachment_(
            param, [this](float newValue) { updateControl(newValue); },
            undoManager) {
    // Initial value sync
    control_.setParameter(&param);
    attachment_.sendInitialUpdate();

    // Bind control callbacks back to the attachment
    control_.onValueChanged = [this](float newValue) {
      attachment_.setValueAsCompleteGesture(newValue);
    };

    control_.onDragStart = [this]() { attachment_.beginGesture(); };

    control_.onDragEnd = [this]() { attachment_.endGesture(); };
  }

  ~ZenithParameterAttachment() {
    // Clear callbacks to avoid dangling references if control outlives
    // attachment
    control_.onValueChanged = nullptr;
    control_.onDragStart = nullptr;
    control_.onDragEnd = nullptr;
  }

private:
  void updateControl(float newValue) {
    // Update the control without triggering its callback (to avoid feedback
    // loops)
    const juce::MessageManagerLock mmLock;
    if (mmLock.lockWasGained()) {
      control_.setValue(newValue, false);
    }
  }

  ZenithControl &control_;
  juce::ParameterAttachment attachment_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithParameterAttachment)
};

} // namespace zenith
