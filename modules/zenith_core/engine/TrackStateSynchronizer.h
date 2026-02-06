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

// TrackStateSynchronizer.h

#include "Engine.h"
#include "ProjectState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {

//==============================================================================
/**
 * @class TrackStateSynchronizer
 // Brief: Binds ProjectState track properties to Engine track objects
 *
 * Usage:
 * ```cpp
 * auto synchronizer = std::make_unique<TrackStateSynchronizer>(projectState,
 * engine); synchronizer->initialize();  // Start listening
 * ```
 *
 * The synchronizer will:
 * 1. Listen to TRACKS node for track add/remove
 * 2. Listen to each TRACK node for property changes
 * (volume/pan/mute/solo/armed)
 * 3. Update Engine track objects when properties change
 * 4. Maintain index mapping between ProjectState tracks and Engine tracks
 */
class TrackStateSynchronizer : private juce::ValueTree::Listener {
public:
  //==========================================================================
  /**
   // Brief: Constructor
   * @param projectState Reference to ProjectState (must outlive this object)
   * @param engine Reference to Engine (must outlive this object)
   */
  TrackStateSynchronizer(ProjectState &projectState, Engine &engine);

  /**
   // Brief: Destructor
   */
  ~TrackStateSynchronizer() override;

  //==========================================================================
  /**
   // Brief: Initialize synchronizer and start listening to ProjectState
   // Note: Call this after both ProjectState and Engine are fully initialized
   */
  void initialize();

  /**
   // Brief: Shutdown synchronizer and stop listening
   */
  void shutdown();

  /**
   // Brief: Force full sync from ProjectState to Engine
   // Note: Useful after loading a project or when tracks are added externally
   */
  void syncAll();

private:
  //==========================================================================
  // ValueTree::Listener interface
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
   // Brief: Sync a single track property to engine
   * @param track Track ValueTree node
   * @param property Property identifier
   */
  void syncTrackProperty(const juce::ValueTree &track,
                         const juce::Identifier &property);

  /**
   // Brief: Get engine track index for a ProjectState track
   * @param track Track ValueTree node
   * @return Engine track index, or -1 if not found
   */
  int getEngineTrackIndex(const juce::ValueTree &track) const;

  /**
   // Brief: Add track listeners to a track node
   */
  void addTrackListener(const juce::ValueTree &track);

  /**
   // Brief: Remove track listeners from a track node
   */
  void removeTrackListener(const juce::ValueTree &track);

  //==========================================================================
  // Member Variables
  //==========================================================================

  ProjectState &projectState;
  Engine &engine;

  bool initialized{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackStateSynchronizer)
};

} // namespace zenith
