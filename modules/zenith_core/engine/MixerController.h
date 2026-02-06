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

// MixerController.h


#include <juce_core/juce_core.h>

namespace zenith {

class Engine;

/**
 * @class MixerController
 * @brief Manages mixer state and operations
 *
 * This class provides a high-level API for mixer operations, including:
 * - Setting volume/pan/mute/solo for tracks
 * - Managing mix groups (VCA)
 * - Monitoring levels
 * - Resetting peaks
 */
class MixerController {
public:
  explicit MixerController(Engine& engine);
  ~MixerController() = default;

  //==============================================================================
  // Track Strip Controls
  //==============================================================================

  void setVolume(int trackIndex, float volume);
  float getVolume(int trackIndex) const;

  void setPan(int trackIndex, float pan);
  float getPan(int trackIndex) const;

  void setMute(int trackIndex, bool muted);
  bool isMuted(int trackIndex) const;

  void setSolo(int trackIndex, bool solo);
  bool isSolo(int trackIndex) const;

  void setRecArm(int trackIndex, bool armed);
  bool isRecArmed(int trackIndex) const;

  //==============================================================================
  // Global Mixer Actions
  //==============================================================================

  /**
   * @brief Clear solo state on all tracks
   */
  void clearAllSolos();

  /**
   * @brief Reset peak indicators on all tracks and master
   */
  void resetAllPeakMeters();



private:
  Engine& engine_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerController)
};

} // namespace zenith
