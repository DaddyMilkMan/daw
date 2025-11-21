/**
 * @file PianoRollEditor.h
 * @brief MIDI note editor with piano roll grid
 */

#pragma once

#include <JuceHeader.h>
#include "../ProjectState.h"
#include "TimelineRuler.h"

/**
 * @class PianoRollEditor
 * @brief Piano roll editor for MIDI notes
 *
 * Displays:
 * - Piano keyboard on the left
 * - Grid of beats vs pitch
 * - MIDI notes as rectangles
 *
 * Allows:
 * - Click to add note
 * - Drag to move note
 * - Double-click to delete note
 *
 * Operates on a single CLIP ValueTree node.
 */
class PianoRollEditor : public juce::Component,
                        private juce::ValueTree::Listener
{
public:
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     * @param trackId ID of track containing the clip
     * @param clipId ID of clip to edit
     */
    PianoRollEditor(ProjectState& projectState, const juce::String& trackId, const juce::String& clipId);
    ~PianoRollEditor() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    //==========================================================================
    // ValueTree::Listener interface
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override {}
    void valueTreeParentChanged(juce::ValueTree& tree) override {}

    //==========================================================================
    // Helper methods
    //==========================================================================

    /**
     * @brief Get note number (0-127) at Y position
     */
    int getNoteAtY(int y) const;

    /**
     * @brief Get beat position at X position
     */
    double getBeatAtX(int x) const;

    /**
     * @brief Get Y position for a note number
     */
    int getYForNote(int noteNumber) const;

    /**
     * @brief Get X position for a beat
     */
    int getXForBeat(double beat) const;

    /**
     * @brief Find note at position
     */
    juce::ValueTree findNoteAtPosition(int x, int y);

    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState;
    juce::String trackId;
    juce::String clipId;
    juce::ValueTree clipNode;

    TimelineRuler ruler;

    double viewStartBeat = 0.0;
    double viewLengthBeats = 8.0;
    double pixelsPerBeat = 40.0;

    int lowestNote = 36;   // C2
    int highestNote = 96;  // C7
    int noteHeight = 12;

    static constexpr int PIANO_WIDTH = 60;
    static constexpr int RULER_HEIGHT = 30;

    // Dragging state
    juce::ValueTree draggedNote;
    juce::Point<int> dragStartPos;
    double dragStartBeats = 0.0;
    int dragStartNoteNumber = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};

