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

    ClipStateManager.h
    Created: 2025-12-11
    Author:  Zenith DAW

    Focused module for clip management within ProjectState.
    
    Extracted from ProjectState.cpp for better modularity.


    Thread Safety:
    - All methods are MESSAGE THREAD ONLY
    - Uses ValueTree for persistent state

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <utility>

namespace zenith {

// Forward declarations
class ProjectState;

//==============================================================================
/**
    Manages clip state within ProjectState.
    
    Handles:
    - Clip creation and deletion
    - Clip property queries
    - Clip movement and resizing
    - Clip splitting and merging
*/
class ClipStateManager {
public:
    //==========================================================================
    explicit ClipStateManager(ProjectState& projectState);
    ~ClipStateManager() = default;

    //==========================================================================
    // Clip Creation/Deletion
    //==========================================================================

    /**
     * @brief Add a new clip to a track
     * @param trackId Track ID to add clip to
     * @param clipType Clip type ("audio" or "midi")
     * @param startBeats Start position in beats
     * @param lengthBeats Length in beats
     * @param laneIndex Clip lane index (for comping)
     * @return Clip ID
     * @note MESSAGE THREAD ONLY
     */
    juce::String addClip(const juce::String& trackId,
                         const juce::String& clipType,
                         double startBeats, double lengthBeats,
                         int laneIndex = 0);

    /**
     * @brief Create an empty clip (MIDI or Audio placeholder)
     * @param trackId Track ID
     * @param startBeats Start position in beats
     * @param lengthBeats Length in beats
     * @param isMidi True for MIDI clip, false for audio
     * @param name Clip name
     * @param actionName Undo action name
     * @return Clip ID
     */
    juce::String createEmptyClip(const juce::String& trackId,
                                  double startBeats, double lengthBeats,
                                  bool isMidi, const juce::String& name,
                                  const juce::String& actionName = "Create Clip");

    /**
     * @brief Remove a clip
     * @param trackId Track ID containing clip
     * @param clipId Clip ID to remove
     * @param actionName Undo action name
     * @return true if removed
     */
    bool removeClip(const juce::String& trackId, const juce::String& clipId,
                    const juce::String& actionName = "Delete Clip");

    /**
     * @brief Delete a clip by ID (searches all tracks)
     * @param clipId Clip ID to delete
     * @param actionName Undo action name
     */
    void deleteClip(const juce::String& clipId,
                    const juce::String& actionName = "Delete Clip");

    //==========================================================================
    // Clip Queries
    //==========================================================================

    /**
     * @brief Get clip ValueTree
     * @param trackId Track ID
     * @param clipId Clip ID
     * @return Clip ValueTree (invalid if not found)
     */
    juce::ValueTree getClip(const juce::String& trackId, 
                            const juce::String& clipId) const;

    /**
     * @brief Find clip by ID (searches all tracks)
     * @param clipId Clip ID
     * @return Pair of (track ValueTree, clip ValueTree)
     */
    std::pair<juce::ValueTree, juce::ValueTree> findClip(const juce::String& clipId) const;

    /**
     * @brief Get clip audio file path
     * @param trackId Track ID
     * @param clipId Clip ID
     * @return Audio file path, or empty if not set
     */
    juce::String getClipAudioFile(const juce::String& trackId,
                                   const juce::String& clipId) const;

    //==========================================================================
    // Clip Modification
    //==========================================================================

    /**
     * @brief Move clip to new position
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param newStartBeats New start position in beats
     * @param actionName Undo action name
     * @return true if moved
     */
    bool moveClip(const juce::String& trackId, const juce::String& clipId,
                  double newStartBeats, 
                  const juce::String& actionName = "Move Clip");

    /**
     * @brief Move clip to a different track
     * @param clipId Clip ID
     * @param newTrackId New track ID
     * @param newStartBeats New start position in beats
     * @param actionName Undo action name
     */
    void moveClipToTrack(const juce::String& clipId,
                         const juce::String& newTrackId,
                         double newStartBeats,
                         const juce::String& actionName = "Move Clip");

    /**
     * @brief Resize clip length
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param newLengthBeats New length in beats
     * @param actionName Undo action name
     * @return true if resized
     */
    bool resizeClip(const juce::String& trackId, const juce::String& clipId,
                    double newLengthBeats,
                    const juce::String& actionName = "Resize Clip");

    /**
     * @brief Set clip range (start + length)
     * @param clipId Clip ID
     * @param newStartBeats New start position
     * @param newLengthBeats New length
     * @param actionName Undo action name
     */
    void setClipRange(const juce::String& clipId,
                      double newStartBeats, double newLengthBeats,
                      const juce::String& actionName = "Set Clip Range");

    /**
     * @brief Set clip audio file
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param audioFile Audio file
     * @param actionName Undo action name
     * @return true if set
     */
    bool setClipAudioFile(const juce::String& trackId, const juce::String& clipId,
                          const juce::File& audioFile,
                          const juce::String& actionName = "Set Clip Audio");

    //==========================================================================
    // Clip Editing
    //==========================================================================

    /**
     * @brief Split clip at position
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param splitBeats Split position in beats
     * @param actionName Undo action name
     * @return Pair of (left clip ID, right clip ID)
     */
    std::pair<juce::String, juce::String> splitClip(
        const juce::String& trackId, const juce::String& clipId,
        double splitBeats, const juce::String& actionName = "Split Clip");

    /**
     * @brief Duplicate clip
     * @param trackId Track ID
     * @param clipId Clip ID
     * @param offsetBeats Position offset for duplicate
     * @param actionName Undo action name
     * @return New clip ID
     */
    juce::String duplicateClip(const juce::String& trackId, const juce::String& clipId,
                               double offsetBeats = 0.0,
                               const juce::String& actionName = "Duplicate Clip");

private:
    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /** Get clips container for a track */
    juce::ValueTree getClipsContainer(const juce::String& trackId) const;

    /** Generate unique clip ID */
    juce::String generateClipId() const;

    //==========================================================================
    // References
    //==========================================================================

    ProjectState& projectState_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipStateManager)
};

} // namespace zenith
