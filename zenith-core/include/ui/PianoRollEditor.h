/**
 * @file PianoRollEditor.h
 * @brief Piano Roll editor for MIDI note editing
 *
 * U4.3: Piano Roll Editor
 *
 * Features:
 * - Visual piano roll grid with beats (horizontal) and pitch (vertical)
 * - Add notes by clicking in empty space
 * - Move notes by dragging
 * - Delete notes by double-clicking
 * - Basic quantization support
 * - ValueTree-based synchronization for undo/redo
 */

#pragma once

#include <JuceHeader.h>
#include "../ProjectState.h"

//==============================================================================
/**
 * @class PianoRollEditor
 * @brief Piano roll editor for MIDI clips
 *
 * Displays and edits MIDI notes within a clip using a traditional piano roll interface.
 * All edits go through ProjectState APIs to ensure proper undo/redo support.
 *
 * Thread Safety:
 * - All operations run on MESSAGE THREAD
 * - Uses ValueTree listeners for automatic UI updates
 */
class PianoRollEditor : public juce::Component,
                        private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param state Reference to project state
     * @param trackId Track ID containing the clip
     * @param clipId Clip ID to edit
     */
    PianoRollEditor(ProjectState& state,
                    const juce::String& trackId,
                    const juce::String& clipId);

    /**
     * @brief Destructor
     */
    ~PianoRollEditor() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    //==========================================================================
    // Zoom and scroll
    //==========================================================================

    /**
     * @brief Set horizontal zoom (pixels per beat)
     */
    void setPixelsPerBeat(double ppb);

    /**
     * @brief Set vertical zoom (pixels per note)
     */
    void setPixelsPerNote(int ppn);

private:
    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreeChildAdded(juce::ValueTree& parent,
                             juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent,
                               juce::ValueTree& child,
                               int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent,
                                    int oldIndex,
                                    int newIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& tree,
                                  const juce::Identifier& property) override;

    //==========================================================================
    // Internal structures
    //==========================================================================

    struct NoteVisual
    {
        juce::String noteId;
        juce::Rectangle<float> bounds;
        int pitch;
        double startBeats;
        double lengthBeats;
        int velocity;
    };

    //==========================================================================
    // Helper methods
    //==========================================================================

    /**
     * @brief Rebuild note visuals from ValueTree
     */
    void rebuildNotes();

    /**
     * @brief Convert note data to screen bounds
     */
    juce::Rectangle<float> noteToBounds(double startBeats,
                                        double lengthBeats,
                                        int pitch) const;

    /**
     * @brief Convert screen position to beat time
     */
    double positionToBeats(float x) const;

    /**
     * @brief Convert screen position to pitch
     */
    int positionToPitch(float y) const;

    /**
     * @brief Quantize beat time to grid
     */
    double quantize(double beats) const;

    /**
     * @brief Find note at screen position
     */
    int findNoteAt(juce::Point<float> position);

    /**
     * @brief Draw piano keys on left side
     */
    void drawPianoKeys(juce::Graphics& g, juce::Rectangle<int> area);

    /**
     * @brief Draw grid lines
     */
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area);

    /**
     * @brief Draw all notes
     */
    void drawNotes(juce::Graphics& g, juce::Rectangle<int> area);

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    juce::String trackId;
    juce::String clipId;
    juce::ValueTree notesNode;

    // Display settings
    double pixelsPerBeat = 40.0;
    int pixelsPerNote = 12;
    int lowestNote = 36;   // C2
    int highestNote = 84;  // C6

    // Grid settings
    double gridDivision = 0.25; // 1/16th note quantization

    // Piano keys width
    int pianoKeysWidth = 60;

    // Note visuals cache
    std::vector<NoteVisual> noteVisuals;

    // Interaction state
    enum class InteractionMode
    {
        None,
        DraggingNote,
        CreatingNote
    };

    InteractionMode interactionMode = InteractionMode::None;
    int draggedNoteIndex = -1;
    juce::Point<float> dragStartPosition;
    double dragStartBeats = 0.0;
    int dragStartPitch = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};
