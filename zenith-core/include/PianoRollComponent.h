/**
 * @file PianoRollComponent.h
 * @brief Piano Roll MIDI editor (Phase 8.1)
 *
 * Provides MIDI note editing with full undo/redo support via ProjectState.
 * All edits go through ProjectState's undoable MIDI API - never modifies
 * Clip's internal data directly.
 *
 * Features:
 * - Create/move/delete notes via mouse
 * - Grid snapping (configurable)
 * - Undo/Redo (Cmd/Ctrl+Z)
 * - Auto-updates when notes change (ValueTree listener)
 * - Supports Wingman MIDI commands
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @struct MidiClipContext
 * @brief Identifies which MIDI clip is being edited
 */
struct MidiClipContext
{
    juce::String clipId;           // Unique clip ID from ProjectState
    juce::String trackId;          // Parent track ID
    double clipStartBeats;         // Clip start time in beats
    double clipLengthBeats;        // Clip duration in beats
    juce::String clipName;         // Display name

    MidiClipContext() : clipStartBeats(0.0), clipLengthBeats(4.0), clipName("Untitled Clip") {}

    bool isValid() const { return clipId.isNotEmpty(); }
};

//==============================================================================
/**
 * @class PianoRollComponent
 * @brief MIDI piano roll editor with undo support
 *
 * Architecture:
 * - Reads notes from ProjectState::getMidiNotesForClip()
 * - Writes via ProjectState::addMidiNote/removeMidiNote/moveMidiNote
 * - Listens to ValueTree changes for auto-refresh (undo/redo/Wingman)
 * - Never touches Clip's internal MidiMessageSequence
 */
class PianoRollComponent : public juce::Component,
                           private juce::ValueTree::Listener,
                           private juce::Timer
{
public:
    //==========================================================================
    explicit PianoRollComponent(ProjectState& state);
    ~PianoRollComponent() override;

    //==========================================================================
    /**
     * @brief Set which MIDI clip to edit
     * @param context Clip identity and metadata
     *
     * Call this when opening a clip for editing.
     * Loads notes from ProjectState and attaches ValueTree listener.
     */
    void setClipContext(const MidiClipContext& context);

    /**
     * @brief Get current clip context
     */
    const MidiClipContext& getClipContext() const { return currentClip; }

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;

private:
    //==========================================================================
    // ValueTree::Listener interface (for auto-refresh on changes)
    //==========================================================================

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    //==========================================================================
    // Timer interface (for periodic refresh if needed)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Internal note representation (UI-only, mirrors ProjectState)
    //==========================================================================

    struct NoteRect
    {
        juce::String id;           // Note ID from ProjectState
        int pitch;                 // MIDI note number
        double startBeats;         // Start time in beats
        double lengthBeats;        // Duration in beats
        int velocity;              // Note velocity
        bool muted;                // Muted flag
        bool selected;             // UI selection state

        juce::Rectangle<float> bounds;  // Screen coordinates

        NoteRect() : pitch(60), startBeats(0.0), lengthBeats(1.0), velocity(100), muted(false), selected(false) {}
    };

    //==========================================================================
    // Data Management
    //==========================================================================

    /**
     * @brief Reload notes from ProjectState
     *
     * Call after any MIDI operation or when ValueTree changes.
     * Rebuilds noteRects from ProjectState's canonical data.
     */
    void refreshNotesFromProjectState();

    /**
     * @brief Rebuild screen rectangles for notes
     */
    void updateNoteRectangles();

    //==========================================================================
    // Coordinate Conversion
    //==========================================================================

    int pixelsToPitch(float y) const;
    float pitchToPixels(int pitch) const;

    double pixelsToBeats(float x) const;
    float beatsToPixels(double beats) const;

    double snapToGrid(double beats) const;

    //==========================================================================
    // Mouse Interaction
    //==========================================================================

    NoteRect* findNoteAtPosition(float x, float y);
    void startDraggingNote(NoteRect* note, const juce::MouseEvent& e);
    void updateNoteDrag(const juce::MouseEvent& e);
    void finishNoteDrag();

    void createNoteAtPosition(float x, float y);
    void deleteSelectedNotes();

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    MidiClipContext currentClip;

    // UI note cache (rebuilt from ProjectState)
    std::vector<NoteRect> noteRects;

    // Grid settings
    double gridBeats = 0.25;       // 1/16 note
    bool snapEnabled = true;

    // Zoom & scroll
    double pixelsPerBeat = 80.0;
    int pixelsPerPitch = 12;
    int scrollOffsetX = 0;
    int scrollOffsetY = 0;

    // Interaction state
    NoteRect* draggingNote = nullptr;
    juce::Point<float> dragStartPos;
    int dragStartPitch = 60;
    double dragStartBeats = 0.0;

    bool needsRefresh = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};
