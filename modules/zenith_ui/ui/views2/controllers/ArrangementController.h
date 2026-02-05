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

    ArrangementController.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Bridge between SkiaArrangementView and the engine.
    
    Connects the view to:

    - ProjectState (tracks, clips, automation)
    - TransportController (playhead, loop, tempo)
    - Engine (playback state)

  ==============================================================================
*/

#pragma once

#include "../../../engine/ProjectState.h"
#include "../../../engine/TransportController.h"
#include "../../../engine/Engine.h"
#include "../arranger/SkiaArrangementView.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

namespace zenith::ui {

/**
 * @brief Track data extracted from ProjectState for view rendering
 */
struct TrackViewData {
    juce::String id;
    juce::String name;
    juce::String type;
    juce::Colour color;
    bool isMuted = false;
    bool isSoloed = false;
    bool isArmed = false;
    float volume = 1.0f;
    float pan = 0.0f;
    float meterLevel = 0.0f;
};

/**
 * @brief Clip data extracted from ProjectState for view rendering
 */
struct ClipViewData {
    juce::String id;
    juce::String trackId;
    juce::String name;
    juce::String type;  // "audio" or "midi"
    juce::Colour color;
    double startBeats = 0.0;
    double lengthBeats = 0.0;
    double offsetBeats = 0.0;
    double fadeInBeats = 0.0;
    double fadeOutBeats = 0.0;
    juce::String audioFilePath;
    bool isSelected = false;
};

/**
 * @class ArrangementController
 * @brief Connects SkiaArrangementView to engine state
 */
class ArrangementController : public juce::ValueTree::Listener,
                               public juce::Timer {
public:
    ArrangementController(SkiaArrangementView& view,
                          zenith::Engine& engine,
                          zenith::ProjectState& projectState);
    ~ArrangementController() override;

    //==========================================================================
    // Data Access
    //==========================================================================

    /**
     * @brief Get all tracks for rendering
     */
    const std::vector<TrackViewData>& getTracks() const { return tracks_; }

    /**
     * @brief Get clips for a specific track
     */
    std::vector<ClipViewData> getClipsForTrack(int trackIndex) const;

    /**
     * @brief Get all clips (all tracks)
     */
    const std::vector<ClipViewData>& getAllClips() const { return clips_; }

    //==========================================================================
    // Transport Sync
    //==========================================================================

    /**
     * @brief Sync view playhead with transport
     */
    void syncPlayhead();

    /**
     * @brief Sync loop region with transport
     */
    void syncLoopRegion();

    /**
     * @brief Sync tempo from project state
     */
    void syncTempo();

    /**
     * @brief Sync time signature from project state
     */
    void syncTimeSignature();
    
    /**
     * @brief Sync meter levels from engine
     */
    void syncMeters();

    //==========================================================================
    // User Actions (routed to engine/state)
    //==========================================================================

    void play();
    void stop();
    void togglePlayback();
    void setPlayheadBeats(double beats);
    void setLoopRegion(double startBeats, double endBeats);
    void setLoopEnabled(bool enabled);

    //==========================================================================
    // Clip Operations
    //==========================================================================

    void selectClip(const juce::String& clipId);
    void selectClips(const std::vector<juce::String>& clipIds);
    void clearSelection();

    void moveClip(const juce::String& clipId, const juce::String& newTrackId, double newStartBeats);
    void resizeClip(const juce::String& clipId, double newLengthBeats);
    void splitClipAtPlayhead(const juce::String& clipId);
    void deleteSelectedClips();
    void duplicateSelectedClips();

    //==========================================================================
    // Callbacks
    //==========================================================================

    /**
     * @brief Callback to open clip editor for a clip
     * Set this to receive notifications when a clip is double-clicked
     */
    std::function<void(const juce::String& clipId, const juce::String& trackId)> onOpenClipEditor;

    //==========================================================================
    // Track Operations
    //==========================================================================

    void selectTrack(int trackIndex);
    void setTrackMute(int trackIndex, bool muted);
    void setTrackSolo(int trackIndex, bool soloed);
    void setTrackArmed(int trackIndex, bool armed);

    //==========================================================================
    // Full Sync
    //==========================================================================

    /**
     * @brief Rebuild all view data from project state
     */
    void fullSync();

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // Timer (for playhead animation)
    //==========================================================================

    void timerCallback() override;

private:
    SkiaArrangementView& view_;
    zenith::Engine& engine_;
    zenith::ProjectState& projectState_;

    std::vector<TrackViewData> tracks_;
    std::vector<ClipViewData> clips_;
    std::vector<juce::String> selectedClipIds_;
    int selectedTrackIndex_ = -1;

    double tempo_ = 120.0;
    double sampleRate_ = 44100.0;

    //==========================================================================
    // Re-entrancy Guard
    //==========================================================================

    // Guard against re-entrant ValueTree callbacks that could cause infinite loops
    bool isProcessingValueTreeChange_ = false;

    //==========================================================================
    // Internal Sync Methods
    //==========================================================================

    void rebuildTrackList();
    void rebuildClipList();
    void updateViewFromState();

    TrackViewData extractTrackData(const juce::ValueTree& trackTree, int index) const;
    ClipViewData extractClipData(const juce::ValueTree& clipTree, const juce::String& trackId) const;

    double samplesToBeats(juce::int64 samples) const;
    juce::int64 beatsToSamples(double beats) const;
    
    //==========================================================================
    // View Callback Handlers
    //==========================================================================
    
    void onClipMoved(const juce::String& clipId, int newTrackIndex, double newStartBeat);
    void onClipDoubleClicked(const juce::String& clipId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementController)
};

} // namespace zenith::ui
