/*
  ==============================================================================

    MixerController.h
    Created: 2025-12-26
    Author:  Zenith DAW

    Controller for mixer operations.
    Acts as a facade/manager for track mixing state, grouping, and metering.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
<<<<<<< HEAD
namespace zenith {
class MixerController {
public:
  MixerController() = default;
  ~MixerController() = default;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerController)
};
=======

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

>>>>>>> origin/master
} // namespace zenith
