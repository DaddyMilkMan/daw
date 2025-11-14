/**
 * @file ArrangerComponent.h
 * @brief Timeline arranger view (Phase 8.1)
 *
 * Displays tracks and clips on a timeline.
 * Double-click MIDI clips to open PianoRollComponent.
 *
 * Features:
 * - Visual timeline with tracks/clips
 * - Double-click MIDI clip → opens PianoRollComponent
 * - Reads clip data from ProjectState ValueTree
 * - Auto-updates when clips change
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include "PianoRollComponent.h"

//==============================================================================
/**
 * @class ArrangerComponent
 * @brief Timeline arranger view with tracks and clips
 */
class ArrangerComponent : public juce::Component,
                          private juce::ValueTree::Listener
{
public:
    //==========================================================================
    explicit ArrangerComponent(ProjectState& state);
    ~ArrangerComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    //==========================================================================
    // ValueTree::Listener interface (for auto-refresh when clips change)
    //==========================================================================

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    //==========================================================================
    // Internal clip representation (UI-only, mirrors ProjectState)
    //==========================================================================

    struct ClipView
    {
        juce::String clipId;
        juce::String trackId;
        juce::String name;
        juce::String type;  // "audio" or "midi"
        double startBeats;
        double lengthBeats;
        int trackIndex;

        juce::Rectangle<float> bounds;

        ClipView() : startBeats(0.0), lengthBeats(4.0), trackIndex(0) {}
    };

    //==========================================================================
    // Data Management
    //==========================================================================

    void refreshClipsFromProjectState();
    void updateClipRectangles();

    //==========================================================================
    // Coordinate Conversion
    //==========================================================================

    double pixelsToBeats(float x) const;
    float beatsToPixels(double beats) const;

    int pixelsToTrack(float y) const;
    float trackToPixels(int trackIndex) const;

    //==========================================================================
    // Interaction
    //==========================================================================

    ClipView* findClipAtPosition(float x, float y);
    void openPianoRollForClip(const ClipView& clip);

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;

    // UI clip cache (rebuilt from ProjectState)
    std::vector<ClipView> clipViews;

    // Zoom & scroll
    double pixelsPerBeat = 40.0;
    int trackHeight = 60;
    int scrollOffsetX = 0;
    int scrollOffsetY = 0;

    // PianoRoll modal window
    std::unique_ptr<juce::DocumentWindow> pianoRollWindow;
    std::unique_ptr<PianoRollComponent> pianoRoll;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
