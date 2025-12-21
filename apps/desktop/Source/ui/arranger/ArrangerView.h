/**
 * @file ArrangerView.h
 * @brief Timeline arranger view showing tracks and clips
 *
 * Integration stub: Shows how ArrangerView integrates with ProjectState
 * and provides UI for clip editing and automation display.
 *
 * Data Flow:
 * 1. Listens to ProjectState ValueTree for track/clip changes
 * 2. Displays clips from ProjectState CLIPS nodes
 * 3. User double-clicks MIDI clip → calls openPianoRoll callback
 * 4. User toggles "Show Automation" → displays AutomationLaneComponent
 * 5. User edits clips → updates ProjectState (triggers ClipSynchronizer)
 */

#pragma once

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

#include "AutomationLaneComponent.h"

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



