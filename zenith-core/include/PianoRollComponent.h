/**
 * @file PianoRollComponent.h
 * @brief Piano Roll MIDI editor (Phase 8.2 - Enhanced Ergonomics)
 *
 * Provides MIDI note editing with full undo/redo support via ProjectState.
 * All edits go through ProjectState's undoable MIDI API - never modifies
 * Clip's internal data directly.
 *
 * Phase 8.1 Features:
 * - Create/move/delete notes via mouse
 * - Grid snapping (configurable)
 * - Undo/Redo (Cmd/Ctrl+Z)
 * - Auto-updates when notes change (ValueTree listener)
 * - Supports Wingman MIDI commands
 *
 * Phase 8.2 NEW Features:
 * - Velocity editing (velocity lane at bottom)
 * - Note length resize (edge drag)
 * - Multi-selection (Ctrl/Cmd-click, marquee drag)
 * - Zoom & scroll (mousewheel, keyboard shortcuts)
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
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

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

        juce::Rectangle<float> bounds;          // Screen coordinates (note grid area)
        juce::Rectangle<float> velocityBounds;  // Velocity lane bar (Phase 8.2)

        NoteRect() : pitch(60), startBeats(0.0), lengthBeats(1.0), velocity(100), muted(false), selected(false) {}
    };

    //==========================================================================
    // Phase 8.2: Drag Modes
    //==========================================================================

    enum class DragMode
    {
        None,
        MoveNote,           // Dragging note body (move pitch + time)
        ResizeLeft,         // Dragging left edge (change start + length)
        ResizeRight,        // Dragging right edge (change length only)
        VelocityEdit,       // Dragging velocity bar
        MarqueeSelect       // Rectangle selection drag
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

    // Phase 8.2: Velocity conversion
    int pixelsToVelocity(float y) const;
    float velocityToPixels(int velocity) const;

    //==========================================================================
    // Mouse Interaction (Phase 8.1)
    //==========================================================================

    NoteRect* findNoteAtPosition(float x, float y);
    void startDraggingNote(NoteRect* note, const juce::MouseEvent& e);
    void updateNoteDrag(const juce::MouseEvent& e);
    void finishNoteDrag();

    void createNoteAtPosition(float x, float y);
    void deleteSelectedNotes();

    //==========================================================================
    // Phase 8.2: Edge Detection & Resize
    //==========================================================================

    /**
     * @brief Detect what part of a note was clicked
     * @param note Note to test
     * @param x Mouse x position
     * @param y Mouse y position
     * @return DragMode (MoveNote, ResizeLeft, ResizeRight)
     */
    DragMode detectNoteHitRegion(const NoteRect& note, float x, float y) const;

    void startResizingNote(NoteRect* note, DragMode mode, const juce::MouseEvent& e);
    void updateNoteResize(const juce::MouseEvent& e);
    void finishNoteResize();

    //==========================================================================
    // Phase 8.2: Multi-Selection
    //==========================================================================

    void clearSelection();
    void selectNote(NoteRect* note, bool addToSelection);
    void selectNotesInRectangle(const juce::Rectangle<float>& rect);

    void startMarqueeSelect(const juce::MouseEvent& e);
    void updateMarqueeSelect(const juce::MouseEvent& e);
    void finishMarqueeSelect();

    void startMovingSelection(const juce::MouseEvent& e);
    void updateSelectionMove(const juce::MouseEvent& e);
    void finishSelectionMove();

    //==========================================================================
    // Phase 8.2: Velocity Editing
    //==========================================================================

    NoteRect* findNoteInVelocityLane(float x);
    void startEditingVelocity(NoteRect* note, const juce::MouseEvent& e);
    void updateVelocityEdit(const juce::MouseEvent& e);
    void finishVelocityEdit();

    //==========================================================================
    // Phase 8.2: Zoom & Scroll
    //==========================================================================

    void zoomHorizontal(float factor, float centerX);
    void zoomVertical(float factor, float centerY);
    void scrollHorizontal(float delta);
    void scrollVertical(float delta);

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

    // Zoom & scroll (Phase 8.2: user-controllable)
    double pixelsPerBeat = 80.0;
    double pixelsPerPitch = 12.0;
    double viewStartBeats = 0.0;
    int viewLowestPitch = 0;

    // Phase 8.2: Velocity lane
    int velocityLaneHeight = 80;

    // Phase 8.2: Edge resize detection
    float resizeHandleWidth = 6.0f;

    // Interaction state
    DragMode currentDragMode = DragMode::None;
    NoteRect* activeNote = nullptr;  // Note being dragged/resized/velocity-edited

    // Phase 8.2: Multi-select state
    juce::Point<float> dragStartPos;
    juce::Rectangle<float> marqueeRect;

    // Phase 8.2: Multi-note drag (cache original positions)
    struct NoteDragState
    {
        juce::String id;
        int originalPitch;
        double originalStartBeats;
        double originalLengthBeats;  // For resize
        int originalVelocity;         // For velocity edits
    };
    std::vector<NoteDragState> dragStates;

    // Legacy state (for compatibility with Phase 8.1 code during migration)
    NoteRect* draggingNote = nullptr;
    int dragStartPitch = 60;
    double dragStartBeats = 0.0;
    int scrollOffsetX = 0;
    int scrollOffsetY = 0;

    bool needsRefresh = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};
