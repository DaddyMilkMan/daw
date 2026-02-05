/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    PitchCurveEditor.h
    Created: 2026-01-29
    Author:  Zenith DAW

    ACTUALLY WORKING pitch curve editor with:
    - Real pitch curve drawing
    - Pitch drift correction

    - Undo/redo
    - Zero-crossing splits
    - Copy/paste

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include <stack>

namespace zenith {
namespace ui {

//==============================================================================
/**
    Represents a pitch point on the curve.
*/
struct PitchPoint
{
    double time = 0.0;           // Time in seconds
    float pitchSemitones = 0.0f; // Pitch relative to reference
    bool isEditable = true;      // Can user edit this point?
    bool isDriftPoint = false;   // Is this a pitch drift correction?
    
    // For curve interpolation
    float tension = 0.5f;        // 0=linear, 0.5=smooth, 1=step
};

//==============================================================================
/**
    A detected note with full editing capabilities.
*/
struct EditableNote
{
    double startTime = 0.0;
    double endTime = 0.0;
    float detectedPitchHz = 0.0f;
    float correctedPitchHz = 0.0f;
    int midiNote = 0;
    
    // Pitch drift within note
    std::vector<PitchPoint> driftCurve;
    
    // Per-note parameters
    float correctionAmount = 1.0f;
    float formantShift = 0.0f;
    float gain = 1.0f;
    bool isBypassed = false;
    bool isSelected = false;
    
    // Visual
    juce::Colour color = juce::Colours::cyan;
};

//==============================================================================
/**
    Undo action for pitch editing.
*/
class PitchEditAction
{
public:
    enum class Type
    {
        MoveNote,
        MovePoint,
        SplitNote,
        MergeNotes,
        DeleteNote,
        ChangePitch,
        ChangeDrift,
        SetParameter
    };
    
    Type type;
    int noteIndex = -1;
    int pointIndex = -1;
    double oldTime = 0.0, newTime = 0.0;
    float oldPitch = 0.0f, newPitch = 0.0f;
    float oldValue = 0.0f, newValue = 0.0f;
    EditableNote oldNote;  // For full note operations
    EditableNote newNote;
    
    void undo(std::vector<EditableNote>& notes);
    void redo(std::vector<EditableNote>& notes);
};

//==============================================================================
/**
    ACTUALLY WORKING pitch curve editor.
    
    This is the real deal - full pitch editing like Melodyne/Auto-Tune Graph.
*/
class PitchCurveEditor : public juce::Component,
                         public juce::ChangeBroadcaster,
                         private juce::Timer
{
public:
    //==============================================================================
    PitchCurveEditor();
    ~PitchCurveEditor() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, 
                        const juce::MouseWheelDetails& wheel) override;

    //==============================================================================
    // Audio Analysis
    void analyzeAudio(const juce::AudioBuffer<float>& audio, double sampleRate);
    void clear();
    
    //==============================================================================
    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    void clearHistory();
    
    //==============================================================================
    // Selection & Copy/Paste
    void selectAll();
    void deselectAll();
    void deleteSelected();
    void copySelection();
    void paste();
    bool hasSelection() const;
    
    //==============================================================================
    // Note Editing
    void splitNoteAt(int noteIndex, double time);
    void mergeNotes(int firstIndex, int secondIndex);
    void setNotePitch(int noteIndex, float pitchHz);
    void setNoteTiming(int noteIndex, double start, double end);
    
    //==============================================================================
    // Pitch Drift Editing
    void addDriftPoint(int noteIndex, double time, float pitchOffset);
    void removeDriftPoint(int noteIndex, int pointIndex);
    void clearDriftCurve(int noteIndex);
    
    //==============================================================================
    // View Control
    void zoomToFit();
    void zoomIn();
    void zoomOut();
    void scrollToTime(double time);
    void setViewRange(double startTime, double endTime);
    
    //==============================================================================
    // Tools
    enum class Tool
    {
        Pointer,        // Select and move
        Pencil,         // Draw pitch curve
        Line,           // Draw straight line
        Eraser,         // Delete points
        Split,          // Split notes
        Hand            // Pan view
    };
    
    void setTool(Tool tool) { currentTool_ = tool; }
    Tool getTool() const { return currentTool_; }
    
    //==============================================================================
    // Snapping
    void setSnapToGrid(bool snap) { snapToGrid_ = snap; }
    void setSnapToScale(bool snap) { snapToScale_ = snap; }
    void setGridResolution(int divisionsPerBeat);
    
    //==============================================================================
    // Playback
    void setPlayhead(double time);
    double getPlayhead() const { return playheadTime_; }
    
    //==============================================================================
    // Export/Apply
    void applyEdits(juce::AudioBuffer<float>& buffer, double sampleRate);
    juce::ValueTree exportToValueTree() const;
    void importFromValueTree(const juce::ValueTree& tree);
    
    //==============================================================================
    // Access
    const std::vector<EditableNote>& getNotes() const { return notes_; }
    int getNoteCount() const { return static_cast<int>(notes_.size()); }

private:
    //==============================================================================
    void timerCallback() override;
    
    //==============================================================================
    // Coordinate conversion
    float timeToX(double time) const;
    double xToTime(float x) const;
    float pitchToY(float pitchHz) const;
    float semitonesToY(float semitones) const;
    float yToPitchHz(float y) const;
    float yToSemitones(float y) const;
    
    //==============================================================================
    // Drawing
    void drawGrid(juce::Graphics& g);
    void drawWaveform(juce::Graphics& g);
    void drawPitchCurve(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawDriftCurves(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void drawToolPreview(juce::Graphics& g);
    
    //==============================================================================
    // Hit testing
    int getNoteAt(float x, float y) const;
    int getDriftPointAt(int noteIndex, float x, float y) const;
    bool isOverNoteEdge(int noteIndex, float x) const;
    
    //==============================================================================
    // Editing operations (with undo)
    void beginDrag(float x, float y);
    void continueDrag(float x, float y);
    void endDrag();
    void addUndoAction(const PitchEditAction& action);
    
    void doSplitNote(int noteIndex, double time);
    void doMergeNotes(int firstIndex, int secondIndex);
    void doMoveNote(int noteIndex, double newStart, double newEnd, float newPitch);
    void doChangeDriftPoint(int noteIndex, int pointIndex, float newPitch);
    void doDeleteNote(int noteIndex);
    
    //==============================================================================
    // Audio analysis
    void detectNotes();
    float snapPitchToScale(float pitchHz) const;
    double snapTimeToGrid(double time) const;
    int findZeroCrossing(const juce::AudioBuffer<float>& audio, int position, int searchRange) const;
    
    //==============================================================================
    // Data
    std::vector<EditableNote> notes_;
    std::vector<float> rawPitchCurve_;
    std::vector<float> waveformOverview_;
    juce::AudioBuffer<float> sourceAudio_;
    double sampleRate_ = 44100.0;
    
    // View
    double viewStartTime_ = 0.0;
    double viewEndTime_ = 10.0;
    float centerPitchHz_ = 440.0f;
    float semitoneRange_ = 36.0f;  // ±18 semitones
    
    // Tool state
    Tool currentTool_ = Tool::Pointer;
    bool snapToGrid_ = true;
    bool snapToScale_ = true;
    int gridDivisions_ = 16;
    
    // Drag state
    bool isDragging_ = false;
    int dragNoteIndex_ = -1;
    int dragPointIndex_ = -1;
    bool draggingLeftEdge_ = false;
    bool draggingRightEdge_ = false;
    juce::Point<float> dragStartPos_;
    double dragStartTime_ = 0.0;
    float dragStartPitch_ = 0.0f;
    EditableNote dragOriginalNote_;
    
    // Drawing state (for pencil tool)
    std::vector<juce::Point<float>> currentDrawPath_;
    
    // Playhead
    double playheadTime_ = 0.0;
    
    // Clipboard
    std::vector<EditableNote> clipboard_;
    
    // Undo/Redo
    std::stack<PitchEditAction> undoStack_;
    std::stack<PitchEditAction> redoStack_;
    static constexpr int kMaxUndoDepth = 50;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchCurveEditor)
};

} // namespace ui
} // namespace zenith
