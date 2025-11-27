/**
 * @file ArrangerView.h
 * @brief Main arranger view with track headers and timeline
 *
 * Layout:
 * ┌────────────┬─────────────────────────┐
 * │ Headers    │ Timeline                │
 * │ (~160px)   │ (rest of space)         │
 * │            │                         │
 * │  Track 1   │ ████████                │
 * │  Track 2   │   ████                  │
 * │  Track 3   │      ████████           │
 * │            │                         │
 * └────────────┴─────────────────────────┘
 *
 * For this phase, we focus on the track headers column.
 * Timeline rendering is deferred to a future phase.
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
#include "../ProjectState.h"
#include "TrackHeaderComponent.h"
#include <vector>
#include <memory>

//==============================================================================
/**
 * @class ArrangerView
 * @brief Main arranger view with track headers
 *
 * Manages:
 * - Track header components (left column)
 * - Timeline view (right area - placeholder for now)
 * - Synchronization with ProjectState tracks
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
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    // ValueTree::Listener (MESSAGE THREAD)
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override {}
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override;
    void valueTreeParentChanged(juce::ValueTree& tree) override {}

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Rebuild track headers from ProjectState
     */
    void rebuildTrackHeaders();

    /**
     * @brief Layout track headers in vertical column
     */
    void layoutTrackHeaders();

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState_;

    // Track headers
    std::vector<std::unique_ptr<TrackHeaderComponent>> trackHeaders_;

    // Layout constants
    static constexpr int HEADER_WIDTH = 160;
    static constexpr int TRACK_HEIGHT = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerView)
};

