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

    TrackStateManager.h
    Created: 2025-12-11
    Author:  Zenith DAW

    Focused module for track management within ProjectState.

    Extracted from ProjectState.cpp for better modularity.


    Thread Safety:
    - All methods are MESSAGE THREAD ONLY
    - Uses ValueTree for persistent state

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>

namespace zenith {

// Forward declarations
class ProjectState;

//==============================================================================
/**
    Manages track state within ProjectState.

    Handles:
    - Track creation and deletion
    - Track property queries
    - Track ordering
    - Track type management
*/
class TrackStateManager {
public:
  //==========================================================================
  explicit TrackStateManager(ProjectState &projectState);
  ~TrackStateManager() = default;

  //==========================================================================
  // Track Creation/Deletion
  //==========================================================================

  /**
   * @brief Add a new track
   * @param name Track display name
   * @param type Track type ("audio", "midi", "instrument", "bus", "master")
   * @return Track ID
   * @note MESSAGE THREAD ONLY
   */
  juce::String addTrack(const juce::String &name, const juce::String &type);

  /**
   * @brief Remove a track by ID
   * @param trackId Track ID to remove
   * @note MESSAGE THREAD ONLY
   */
  void removeTrack(const juce::String &trackId);

  /**
   * @brief Remove a track by index
   * @param index Track index
   * @note MESSAGE THREAD ONLY
   */
  void removeTrackByIndex(int index);

  //==========================================================================
  // Track Queries
  //==========================================================================

  /**
   * @brief Get number of tracks
   * @return Track count
   */
  int getNumTracks() const;

  /**
   * @brief Get track ValueTree by ID
   * @param trackId Track ID
   * @return Track ValueTree (invalid if not found)
   */
  juce::ValueTree getTrack(const juce::String &trackId) const;

  /**
   * @brief Get track ValueTree by index
   * @param index Track index
   * @return Track ValueTree (invalid if out of range)
   */
  juce::ValueTree getTrackByIndex(int index) const;

  /**
   * @brief Get track index by ID
   * @param trackId Track ID
   * @return Index, or -1 if not found
   */
  int getTrackIndex(const juce::String &trackId) const;

  //==========================================================================
  // Track Properties
  //==========================================================================

  // Getters
  juce::String getTrackName(const juce::String &trackId) const;
  juce::String getTrackType(const juce::String &trackId) const;
  float getTrackVolume(const juce::String &trackId) const;
  float getTrackPan(const juce::String &trackId) const;
  bool isTrackMuted(const juce::String &trackId) const;
  bool isTrackSolo(const juce::String &trackId) const;
  bool isTrackArmed(const juce::String &trackId) const;
  bool isTrackInputMonitoring(const juce::String &trackId) const;
  juce::Colour getTrackColor(const juce::String &trackId) const;

  // Setters (with undo action name)
  void setTrackName(const juce::String &trackId, const juce::String &name,
                    const juce::String &actionName = "Rename Track");
  void setTrackVolume(const juce::String &trackId, float volume,
                      const juce::String &actionName = "Set Track Volume");
  void setTrackPan(const juce::String &trackId, float pan,
                   const juce::String &actionName = "Set Track Pan");
  void setTrackMute(const juce::String &trackId, bool muted,
                    const juce::String &actionName = "Toggle Track Mute");
  void setTrackSolo(const juce::String &trackId, bool solo,
                    const juce::String &actionName = "Toggle Track Solo");
  void setTrackArmed(const juce::String &trackId, bool armed,
                     const juce::String &actionName = "Toggle Track Armed");
  void
  setTrackInputMonitor(const juce::String &trackId, bool monitoring,
                       const juce::String &actionName = "Toggle Input Monitor");
  void setTrackColor(const juce::String &trackId, const juce::Colour &color,
                     bool manuallySet = true,
                     const juce::String &actionName = "Set Track Color");

  //==========================================================================
  // Track Ordering
  //==========================================================================

  /**
   * @brief Move track to new position
   * @param trackId Track ID to move
   * @param newIndex New position (0-based)
   * @param actionName Undo action name
   * @note MESSAGE THREAD ONLY
   */
  void moveTrack(const juce::String &trackId, int newIndex,
                 const juce::String &actionName = "Move Track");

  /**
   * @brief Duplicate a track (including clips)
   * @param trackId Track ID to duplicate
   * @param actionName Undo action name
   * @return New track ID
   * @note MESSAGE THREAD ONLY
   */
  juce::String
  duplicateTrack(const juce::String &trackId,
                 const juce::String &actionName = "Duplicate Track");

  /**
   * @brief Insert a new track above an existing one
   * @param targetTrackId ID of the track to insert above
   * @param type Track type
   * @param actionName Undo action name
   * @return New track ID
   */
  juce::String insertTrackAbove(const juce::String &targetTrackId,
                                const juce::String &type,
                                const juce::String &actionName = "Insert Track Above");

  /**
   * @brief Insert a new track below an existing one
   * @param targetTrackId ID of the track to insert below
   * @param type Track type
   * @param actionName Undo action name
   * @return New track ID
   */
  juce::String insertTrackBelow(const juce::String &targetTrackId,
                                const juce::String &type,
                                const juce::String &actionName = "Insert Track Below");

private:
  //==========================================================================
  // Internal Helpers
  //==========================================================================

  /** Find track ValueTree by ID */
  juce::ValueTree findTrack(const juce::String &trackId) const;

  /** Get tracks container */
  juce::ValueTree getTracksContainer() const;

  /** Generate unique track ID */
  juce::String generateTrackId() const;

  /** Assign automatic color based on track index */
  juce::Colour getAutoColor(int trackIndex) const;

  /** Internal helper to insert a track at a specific index */
  juce::String insertTrackAt(int index, const juce::String &name,
                             const juce::String &type,
                             const juce::String &actionName);

  //==========================================================================
  // References
  //==========================================================================

  ProjectState &projectState_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackStateManager)
};

} // namespace zenith
