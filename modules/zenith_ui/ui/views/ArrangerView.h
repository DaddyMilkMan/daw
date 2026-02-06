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

// ArrangerView.h


#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include "ProjectState.h"
#include <functional>
#include <memory>

// Forward declaration
class AutomationLaneComponent;

//==============================================================================
/**
 * @class ArrangerView
 * @brief Timeline view for arranging clips and automation
 *
 * The main timeline editor showing:
 * - Horizontal tracks (one per ProjectState track)
 * - Clips positioned on timeline (beat-based)
 * - Automation lanes (toggle per track)
 * - Playhead position
 * - Grid lines (based on tempo)
 */
class ArrangerView : public juce::Component,
                     private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     */
    explicit ArrangerView(ProjectState& projectState);

    /**
     * @brief Destructor
     */
    ~ArrangerView() override;

    //==========================================================================
    // Callbacks
    //==========================================================================

    /**
     * @brief Set callback for opening piano roll editor
     * @param callback Called with (trackId, clipId) when user double-clicks MIDI clip
     */
    void setOpenPianoRollCallback(std::function<void(juce::String, juce::String)> callback)
    {
        openPianoRollCallback = std::move(callback);
    }

    //==========================================================================
    // Track Automation Display
    //==========================================================================

    /**
     * @brief Toggle automation lane visibility for a track
     * @param trackId Track ID
     * @param show true to show, false to hide
     */
    void setTrackAutomationVisible(const juce::String& trackId, bool show) [[maybe_unused]];

    /**
     * @brief Check if track automation is visible
     */
    bool isTrackAutomationVisible(const juce::String& trackId) const;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    //==========================================================================
    // ValueTree::Listener (for ProjectState changes)
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override {}
    void valueTreeParentChanged(juce::ValueTree& tree) override {}

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Rebuild track display from ProjectState
     */
    void rebuildTracks();

    /**
     * @brief Find clip at mouse position
     * @return {trackId, clipId} or empty strings if none found
     */
    std::pair<juce::String, juce::String> findClipAtPosition(juce::Point<int> position);

    //==========================================================================
    // UI Helpers
    //==========================================================================

    /**
     * @brief Convert beats to pixels
     */
    float beatsToPixels(double beats) const;

    /**
     * @brief Convert pixels to beats
     */
    double pixelsToBeats(float pixels) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;

    // Callbacks
    std::function<void(juce::String, juce::String)> openPianoRollCallback;

    // View settings
    float pixelsPerBeat = 40.0f;
    int trackHeight = 60;
    int automationLaneHeight = 80;

    // Track automation visibility
    std::map<juce::String, bool> trackAutomationVisible;

    // Automation lane components (created when visible)
    std::map<juce::String, std::unique_ptr<AutomationLaneComponent>> automationLanes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerView)
};

//==============================================================================
/**
 * @class AutomationLaneComponent
 * @brief Displays and edits automation curves for a track parameter
 *
 * Shows automation points from ProjectState and allows editing:
 * - Click to add point
 * - Drag to move point
 * - Delete key to remove point
 * - Draws interpolated curve between points
 */
class AutomationLaneComponent : public juce::Component
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", or "mute")
     */
    AutomationLaneComponent(ProjectState& projectState,
                            const juce::String& trackId,
                            const juce::String& paramId);

    /**
     * @brief Destructor
     */
    ~AutomationLaneComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Convert value (0-1 or -1 to 1) to Y pixel coordinate
     */
    float valueToY(double value) const;

    /**
     * @brief Convert Y pixel coordinate to value
     */
    double yToValue(float y) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    juce::String trackId;
    juce::String paramId;

    // Selected automation point (for dragging)
    juce::String selectedPointId;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLaneComponent)
};

