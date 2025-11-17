/**
 * @file ArrangerView.h
 * @brief Main arranger/timeline view for tracks and clips
 */

#pragma once

#include <JuceHeader.h>
#include "../ProjectState.h"
#include "TimelineRuler.h"
#include "ClipComponent.h"

/**
 * @class ArrangerView
 * @brief Main timeline view showing tracks and clips
 *
 * Displays:
 * - Timeline ruler at top
 * - Track lanes with clips
 * - Horizontal scroll and zoom
 *
 * Operates on ProjectState ValueTree.
 */
class ArrangerView : public juce::Component,
                     private juce::ValueTree::Listener
{
public:
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     */
    ArrangerView(ProjectState& projectState);
    ~ArrangerView() override;

    //==========================================================================
    // View control
    //==========================================================================

    /**
     * @brief Set zoom level (pixels per beat)
     */
    void setZoom(double pixelsPerBeat);

    /**
     * @brief Scroll to a specific beat position
     */
    void scrollToBeat(double beat);

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

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

    /**
     * @brief Rebuild all clip components from state
     */
    void rebuildClips();

    /**
     * @brief Update clip positions and sizes
     */
    void updateClipBounds();

    /**
     * @brief Get track index at Y position
     */
    int getTrackAtY(int y) const;

    /**
     * @brief Get beat position at X position
     */
    double getBeatAtX(int x) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState;

    TimelineRuler ruler;
    std::vector<std::unique_ptr<ClipComponent>> clipComponents;

    double viewStartBeat = 0.0;
    double viewLengthBeats = 32.0;
    double pixelsPerBeat = 20.0;

    static constexpr int TRACK_HEIGHT = 80;
    static constexpr int RULER_HEIGHT = 30;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerView)
};
