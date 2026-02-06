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

// ClipComponent.h


#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "SkiaComponent.h"

/**
 * @class ClipComponent
 * @brief Flat clip display on the arranger timeline
 *
 * Clean design with:
 * - Track-colored backgrounds (muted, using theme clip colors)
 * - Typography.body for clip names
 * - Simple 1-2px selection border
 * - Rounded corners (4px, 8px grid)
 */
class ClipComponent : public zenith::SkiaComponent {
public:
  /**
   * @brief Constructor
   * @param clipNode ValueTree node for this clip
   */
  ClipComponent(juce::ValueTree clipNode);
  ~ClipComponent() override;

  //==========================================================================
  // Clip data
  //==========================================================================

  /**
   * @brief Get the clip's ValueTree node
   */
  juce::ValueTree getClipNode() const { return clip; }

  /**
   * @brief Get clip ID
   */
  juce::String getClipId() const;

  /**
   * @brief Get start position in beats
   */
  double getStartBeats() const;

  /**
   * @brief Get length in beats
   */
  double getLengthBeats() const;

  /**
   * @brief Update bounds from clip data and pixels-per-beat ratio
   */

  //==========================================================================
  // Smart Rendering Helpers
  //==========================================================================

  struct NeighborhoodState {
    bool leftConnected = false;
    bool rightConnected = false;
  };

  NeighborhoodState getNeighborhoodState() const;
  double getLoopLength() const;

  void updateBounds(double pixelsPerBeat, int yPosition, int height)
      [[maybe_unused]];

  //==========================================================================
  // Component interface
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override;

  void mouseDown(const juce::MouseEvent &event) override;
  void mouseDrag(const juce::MouseEvent &event) override;
  void mouseEnter(const juce::MouseEvent &event) override;
  void mouseExit(const juce::MouseEvent &event) override;

  //==========================================================================
  // Timer interface (for smooth animations)
  //==========================================================================

  void timerCallback() override;

private:
  juce::ValueTree clip;
  juce::Point<int> dragStartPos;
  double dragStartBeats = 0.0;

  // Animation state
  bool isHovered = false;
  bool isSelected = false;
  float hoverAnimation = 0.0f; // 0.0 to 1.0 for smooth hover animation
  float selectionPulse = 0.0f; // 0.0 to 1.0 for selection glow pulse

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
