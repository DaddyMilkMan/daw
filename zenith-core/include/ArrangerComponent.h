/**
 * @file ArrangerComponent.h
 * @brief Minimal arranger view for displaying tracks and clips
 *
 * Phase 12: Recording UX & Track Types
 * - Displays tracks with basic headers
 * - Shows clips on timeline
 * - Listens to ProjectState changes
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @class ArrangerComponent
 * @brief Minimal arranger view for Phase 12
 *
 * Displays:
 * - Track headers (name, type badge, arm button)
 * - Clips on timeline
 * - Responds to ValueTree changes
 */
class ArrangerComponent : public juce::Component,
                          private juce::ValueTree::Listener
{
public:
    //==========================================================================
    ArrangerComponent(ProjectState& projectState);
    ~ArrangerComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    // ValueTree::Listener interface
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override {}
    void valueTreeParentChanged(juce::ValueTree& tree) override {}

    //==========================================================================
    // Helper methods
    //==========================================================================

    void rebuildTrackComponents();
    juce::Rectangle<int> getTrackBounds(int trackIndex) const;
    juce::Rectangle<int> getClipBounds(int trackIndex, int64_t startSamples, int64_t lengthSamples) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState_;

    // Track header components
    struct TrackHeader : public juce::Component
    {
        TrackHeader(ProjectState& ps, const juce::String& trackId);

        void paint(juce::Graphics& g) override;
        void resized() override;

        ProjectState& projectState;
        juce::String trackId;

        juce::Label nameLabel;
        juce::Label typeLabel;
        juce::TextButton armButton;
    };

    juce::OwnedArray<TrackHeader> trackHeaders_;

    // Layout constants
    static constexpr int trackHeight = 60;
    static constexpr int headerWidth = 200;
    static constexpr int pixelsPerSecond = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
