/**
 * @file PianoRollComponent.h
 * @brief Professional-grade MIDI Piano Roll Editor
 * 
 * FEATURES:
 * ✅ Core Editing: Create, move, delete, resize notes
 * ✅ Multi-Selection: Ctrl-click, marquee, batch operations
 * ✅ Velocity Editing: Lane + gradient visualization
 * ✅ Copy/Paste: Full clipboard support
 * ✅ Quantize: With strength & swing
 * ✅ Smart Duplicate: Pattern-aware duplication
 * ✅ Velocity Curves: Ramp, compress, humanize
 * ✅ Chord Detection: Real-time chord naming
 * ✅ Scale Highlighting: Visual scale guide
 * ✅ Note Muting: Per-note mute toggle
 * ✅ Batched Undo: Proper multi-operation undo/redo
 * ✅ Cursor Feedback: Context-aware cursors
 * ✅ Note Color by Velocity: Visual dynamics
 * 
 * Architecture:
 * - Single unified implementation
 * - All edits through ProjectState with batched undo support
 * - ValueTree reactive (auto-refresh on changes)
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "ProjectState.h"
#include <vector>
#include <memory>

//==============================================================================
/**
 * @struct MidiClipContext
 * @brief Identifies which MIDI clip is being edited
 */
struct MidiClipContext
{
    juce::String clipId;
    juce::String trackId;
    double clipStartBeats = 0.0;
    double clipLengthBeats = 4.0;
    juce::String clipName = "Untitled Clip";

    bool isValid() const { return clipId.isNotEmpty(); }
};

//==============================================================================
/**
 * @class PianoRollComponent
 * @brief Professional-grade MIDI piano roll editor component
 */
class PianoRollComponent : public juce::Component,
                           private juce::ValueTree::Listener
{
public:
    //==========================================================================
    explicit PianoRollComponent(ProjectState& state);
    ~PianoRollComponent() override;

    //==========================================================================
    // Clip Management
    //==========================================================================
    
    void setClipContext(const MidiClipContext& context);
    const MidiClipContext& getClipContext() const { return currentClip; }

    //==========================================================================
    // Component Interface
    //==========================================================================
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    bool keyPressed(const juce::KeyPress& key) override;
    
    juce::MouseCursor getMouseCursor() override;

    //==========================================================================
    // Public API - Advanced Features
    //==========================================================================
    
    /** Quantize selected notes with strength and swing */
    void quantizeSelected(double gridSize, float strength = 1.0f, float swing = 0.0f);
    
    /** Humanize velocities of selected notes */
    void humanizeVelocity(float amount = 0.3f);
    
    /** Apply velocity curve to selected notes */
    enum class VelocityCurve { RampUp, RampDown, Compress, Expand, Invert };
    void applyVelocityCurve(VelocityCurve curve, float amount = 1.0f);
    
    /** Duplicate selected notes with smart offset */
    void smartDuplicate();
    
    /** Create note roll/trill */
    void createRoll(float x, float y, double rollSpeed);
    
    /** Set scale highlighting */
    enum class ScaleType { 
        Chromatic, Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, 
        Aeolian, Locrian, HarmonicMinor, MelodicMinor 
    };
    void setScaleHighlight(int rootNote, ScaleType scale);
    void clearScaleHighlight();
    
    /** Toggle note mute */
    void toggleMuteSelected();

private:
    //==========================================================================
    // Internal Note Representation
    //==========================================================================
    
    struct NoteRect
    {
        juce::String id;
        int pitch;
        double startBeats;
        double lengthBeats;
        int velocity;
        bool muted;
        bool selected;
        
        juce::Rectangle<float> bounds;
        juce::Rectangle<float> velocityBounds;
        
        // Visual state
        bool isHovered = false;
        
        NoteRect() : pitch(60), startBeats(0.0), lengthBeats(1.0), 
                     velocity(100), muted(false), selected(false) {}
    };

    //==========================================================================
    // Drag Modes (with cursor support)
    //==========================================================================
    
    enum class DragMode
    {
        None,
        MoveNote,
        ResizeLeft,
        ResizeRight,
        VelocityEdit,
        MarqueeSelect
    };
    
    enum class CursorType
    {
        Normal,
        Hand,
        ResizeHorizontal,
        ResizeLeft,
        ResizeRight,
        Crosshair
    };

    //==========================================================================
    // Clipboard Support
    //==========================================================================
    
    struct ClipboardNote
    {
        int pitch;
        double startBeats;
        double lengthBeats;
        int velocity;
        bool muted;
    };
    
    std::vector<ClipboardNote> clipboard;
    double clipboardReferenceTime = 0.0;  // For relative paste

    //==========================================================================
    // Scale Highlighting
    //==========================================================================
    
    struct ScaleHighlight
    {
        bool enabled = false;
        int rootNote = 0;  // 0-11 (C-B)
        ScaleType scale = ScaleType::Major;
        std::vector<bool> highlightedPitches;  // 128 bools for each MIDI note
    };
    
    ScaleHighlight scaleHighlight;
    void updateScaleHighlight();
    bool isNoteInScale(int pitch) const;

    //==========================================================================
    // Chord Detection
    //==========================================================================
    
    juce::String detectChord(const std::vector<int>& pitches) const;
    juce::String getCurrentChordName() const;

    //==========================================================================
    // Data Management
    //==========================================================================
    
    void refreshNotesFromProjectState();
    void updateNoteRectangles();

    //==========================================================================
    // Coordinate Conversion
    //==========================================================================
    
    int pixelsToPitch(float y) const;
    float pitchToPixels(int pitch) const;
    double pixelsToBeats(float x) const;
    float beatsToPixels(double beats) const;
    double snapToGrid(double beats) const;
    
    int pixelsToVelocity(float y) const;
    float velocityToPixels(int velocity) const;

    //==========================================================================
    // Mouse Interaction Helpers
    //==========================================================================
    
    NoteRect* findNoteAtPosition(float x, float y);
    NoteRect* findNoteInVelocityLane(float x, float y);
    DragMode detectNoteHitRegion(const NoteRect& note, float x, float y) const;
    CursorType getCursorForPosition(float x, float y) const;

    //==========================================================================
    // Editing Operations (with batched undo)
    //==========================================================================
    
    void createNoteAtPosition(float x, float y);
    void deleteSelectedNotes();
    void copySelectedNotes();
    void pasteNotes();
    void cutSelectedNotes();
    
    //==========================================================================
    // Selection Management
    //==========================================================================
    
    void clearSelection();
    void selectNote(NoteRect* note, bool addToSelection);
    void selectNotesInRectangle(const juce::Rectangle<float>& rect);
    void selectAll();
    void invertSelection();
    int getSelectedNoteCount() const;

    //==========================================================================
    // Drag Operations
    //==========================================================================
    
    void startMovingSelection(const juce::MouseEvent& e);
    void updateSelectionMove(const juce::MouseEvent& e);
    void finishSelectionMove();
    
    void startResizingNote(NoteRect* note, DragMode mode, const juce::MouseEvent& e);
    void updateNoteResize(const juce::MouseEvent& e);
    void finishNoteResize();
    
    void startEditingVelocity(NoteRect* note, const juce::MouseEvent& e);
    void updateVelocityEdit(const juce::MouseEvent& e);
    void finishVelocityEdit();
    
    void startMarqueeSelect(const juce::MouseEvent& e);
    void updateMarqueeSelect(const juce::MouseEvent& e);
    void finishMarqueeSelect();

    //==========================================================================
    // Zoom & Scroll
    //==========================================================================
    
    void zoomHorizontal(float factor, float centerX);
    void zoomVertical(float factor, float centerY);
    void scrollHorizontal(float delta);
    void scrollVertical(float delta);

    //==========================================================================
    // Rendering Helpers
    //==========================================================================
    
    juce::Colour getColorForVelocity(int velocity) const;
    void drawPianoKeys(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawNotes(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawVelocityLane(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawChordName(juce::Graphics& g);

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================
    
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    //==========================================================================
    // Member Variables
    //==========================================================================
    
    ProjectState& projectState;
    MidiClipContext currentClip;
    
    std::vector<NoteRect> noteRects;
    
    // Grid & Snap
    double gridBeats = 0.25;  // 1/16 note
    bool snapEnabled = true;
    
    // View & Zoom
    double pixelsPerBeat = 80.0;
    double pixelsPerPitch = 16.0;
    double viewStartBeats = 0.0;
    int viewLowestPitch = 0;
    int scrollOffsetX = 0;
    int scrollOffsetY = 0;
    
    // Layout
    static constexpr int PIANO_WIDTH = 60;
    static constexpr int RULER_HEIGHT = 30;
    int velocityLaneHeight = 120;
    float resizeHandleWidth = 8.0f;
    
    // Interaction State
    DragMode currentDragMode = DragMode::None;
    NoteRect* activeNote = nullptr;
    NoteRect* hoveredNote = nullptr;
    
    juce::Point<float> dragStartPos;
    juce::Rectangle<float> marqueeRect;
    
    // Multi-drag state cache
    struct NoteDragState
    {
        juce::String id;
        int originalPitch;
        double originalStartBeats;
        double originalLengthBeats;
        int originalVelocity;
    };
    std::vector<NoteDragState> dragStates;
    
    // Cursor state
    CursorType currentCursorType = CursorType::Normal;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};

//==============================================================================
/**
 * @class PianoRollWindow
 * @brief Standalone window wrapper for PianoRollComponent
 */
class PianoRollWindow : public juce::DocumentWindow
{
public:
    PianoRollWindow(ProjectState& state, const juce::String& trackId, const juce::String& clipId)
        : DocumentWindow("Piano Roll",
                         juce::Desktop::getInstance().getDefaultLookAndFeel()
                             .findColour(juce::ResizableWindow::backgroundColourId),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        
        auto* content = new PianoRollComponent(state);
        setContentOwned(content, true);
        
        // Setup clip context
        MidiClipContext context;
        context.clipId = clipId;
        context.trackId = trackId;
        
        // Find clip info from state
        auto [track, clip] = state.findClip(clipId);
        if (clip.isValid())
        {
            context.clipName = clip.getProperty(ProjectState::PROP_NAME).toString();
            context.clipStartBeats = clip.getProperty(ProjectState::PROP_START);
            context.clipLengthBeats = clip.getProperty(ProjectState::PROP_LENGTH);
        }
        
        content->setClipContext(context);
        
        setResizable(true, true);
        centreWithSize(1000, 600);
        setVisible(true);
    }
    
    ~PianoRollWindow() override = default;

    void closeButtonPressed() override
    {
        delete this;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWindow)
};
