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

// TrackAutomationSynchronizer.h

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <map>
#include <memory>
#include <vector>

// Forward declarations to break circular include
namespace zenith {
class ProjectState;
class Engine;
class Track;

//==============================================================================
/**
 * @class TrackAutomationSynchronizer
 // Brief: Samples automation curves and updates track atomics (RT-safe)
 *
 * This class bridges the gap between:
 * - ProjectState (ValueTree, message thread only)
 * - Track atomics (read by audio thread, written by message thread)
 *
 * This class runs a timer on the message thread that:
 * 1. Gets the current playback position from Engine (in samples)
 * 2. Converts to beats using tempo
 * 3. Samples automation envelopes at that beat position
 * 4. Writes sampled values to Track atomics
 *
 * Thread safety:
 * - All methods run on MESSAGE THREAD only
 * - Updates std::atomic values in Track objects
 * - Audio thread reads those atomics lock-free
 */
class TrackAutomationSynchronizer : public juce::Timer,
                                    private juce::ValueTree::Listener {
public:
  //==========================================================================
  /**
   // Brief: Constructor
   * @param projectState Reference to project state (must outlive this object)
   * @param engine Reference to audio engine (must outlive this object)
   */
  TrackAutomationSynchronizer(ProjectState &projectState, Engine &engine);

  /**
   // Brief: Destructor
   */
  ~TrackAutomationSynchronizer() override;

  //==========================================================================
  /**
   // Brief: Start automation synchronization
   * @param updateRateHz Update rate in Hz (default 60)
   */
  void start(int updateRateHz = 60);

  /**
   // Brief: Stop automation synchronization
   */
  void stop();

  /**
   // Brief: Check if synchronizer is running
   */
  bool isRunning() const { return isTimerRunning(); }

private:
  //==========================================================================
  // Timer callback (MESSAGE THREAD)
  //==========================================================================

  /**
   // Brief: Timer callback - samples automation and updates tracks
   * @note Called on MESSAGE THREAD at regular intervals
   */
  void timerCallback() override;

  //==========================================================================
  // ValueTree::Listener (MESSAGE THREAD)
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;
  void valueTreeParentChanged(juce::ValueTree &tree) override;

  //==========================================================================
  // Helper Methods
  //==========================================================================

  /**
   // Brief: Sample automation envelope at a specific time
   * @param envelope Envelope ValueTree
   * @param timeBeats Time in beats
   * @return Interpolated value
   */
  double sampleEnvelope(const juce::ValueTree &envelope,
                        double timeBeats) const;

  /**
   // Brief: Update automation for a specific track
   * @param trackId Track ID from ProjectState
   * @param track Track object to update
   * @param playbackBeats Current playback position in beats
   */
  void updateTrackAutomation(const juce::String &trackId, Track *track,
                             double playbackBeats);

  /**
   // Brief: Rebuild automation listeners
   * @note Call when tracks change or automation structure changes
   */
  void rebuildListeners();

  //==========================================================================
  // Member Variables
  //==========================================================================

  ProjectState &projectState;
  Engine &engine;

  // Track ID mapping (ProjectState ID -> Engine track index)
  std::map<juce::String, int> trackIdToIndex;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackAutomationSynchronizer)
};

} // namespace zenith
