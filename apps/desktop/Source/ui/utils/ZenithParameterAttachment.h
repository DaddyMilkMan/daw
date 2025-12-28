/*
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
