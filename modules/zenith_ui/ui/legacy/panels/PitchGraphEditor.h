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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../../../dsp/PitchDetector.h"
#include <vector>
#include <memory>

namespace zenith {
namespace ui {

//==============================================================================
/**
    Represents a detected/edited note in the pitch graph.
*/
struct GraphNote
{
    double startTime = 0.0;           // Start time in seconds
    double endTime = 0.0;             // End time in seconds
    float pitchHz = 0.0f;             // Detected pitch
    float targetPitchHz = 0.0f;       // Target/corrected pitch
    float confidence = 0.0f;          // Detection confidence (0-1)
    int midiNote = 0;                 // MIDI note number
    
    // Per-note parameters
    float correctionAmount = 1.0f;    // 0-1
    float formantShift = 0.0f;        // Semitones
    float gain = 1.0f;                // Volume adjustment
    bool isSelected = false;
    bool isBypassed = false;          // Skip correction for this note
    
    juce::Rectangle<float> getBounds(double pixelsPerSecond, float pixelsPerSemitone, 
                                      float centerPitch) const;
};

//==============================================================================
/**
    Pitch curve point for freehand editing.
*/
struct PitchCurvePoint
{
    double time = 0.0;
    float pitchSemitones = 0.0f;
    float tension = 0.5f;  // For curve smoothing
};

//==============================================================================
/**
    Graph Mode pitch editor - visual pitch correction.
    
    This is the "killer feature" that puts Zenith on par with Auto-Tune Pro
    and Melodyne. Users can see the pitch curve and manually edit it.
*/
class PitchGraphEditor : public juce::Component,
                         public juce::ChangeBroadcaster,
                         private juce::Timer
{
public:
    //==============================================================================
    PitchGraphEditor();
    ~PitchGraphEditor() override;

    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    //==============================================================================
    // Audio Analysis
    void analyzeAudio(const juce::AudioBuffer<float>& audio, double sampleRate);
    void clearAnalysis();
    
    //==============================================================================
    // Note Management
    const std::vector<GraphNote>& getNotes() const { return notes_; }
    void addNote(const GraphNote& note);
    void deleteNote(int index);
    void updateNotePitch(int index, float newPitchHz);
    void updateNoteTiming(int index, double newStart, double newEnd);
    void setNoteSelected(int index, bool selected);
    void setAllNotesSelected(bool selected);
    
    //==============================================================================
    // Per-Note Parameters
    void setNoteCorrectionAmount(int index, float amount);
    void setNoteFormantShift(int index, float semitones);
    void setNoteGain(int index, float gain);
    void setNoteBypass(int index, bool bypass);
    
    //==============================================================================
    // View Control
    void setViewRange(double startTime, double endTime);
    void setPitchRange(float centerPitch, float semitoneRange);
    void zoomToFit();
    void zoomIn();
    void zoomOut();
    void scrollToTime(double time);
    
    //==============================================================================
    // Editing Modes
    enum class EditMode
    {
        Select,         // Select and move notes
        DrawPitch,      // Draw pitch curve
        Split,          // Split notes
        Erase           // Delete notes/points
    };
    
    void setEditMode(EditMode mode) { editMode_ = mode; }
    EditMode getEditMode() const { return editMode_; }
    
    //==============================================================================
    // Grid/Snap
    void setSnapToGrid(bool snap) { snapToGrid_ = snap; }
    void setSnapToScale(bool snap) { snapToScale_ = snap; }
    void setGridDivisions(int beatsPerBar, int divisions);
    
    //==============================================================================
    // Playback
    void setPlayheadPosition(double time);
    double getPlayheadPosition() const { return playheadTime_; }
    
    //==============================================================================
    // Export
    void applyCorrectionsToBuffer(juce::AudioBuffer<float>& buffer, double sampleRate);
    juce::ValueTree exportToValueTree() const;
    void importFromValueTree(const juce::ValueTree& tree);
    
    //==============================================================================
    // Display Options
    void setShowPitchCurve(bool show) { showPitchCurve_ = show; repaint(); }
    void setShowNoteBlocks(bool show) { showNoteBlocks_ = show; repaint(); }
    void setShowGrid(bool show) { showGrid_ = show; repaint(); }
    
private:
    //==============================================================================
    void timerCallback() override;
    
    //==============================================================================
    // Coordinate conversion
    float timeToX(double time) const;
    double xToTime(float x) const;
    float pitchToY(float pitchHz) const;
    float semitonesToY(float semitones) const;
    float yToPitch(float y) const;
    
    //==============================================================================
    // Drawing
    void drawGrid(juce::Graphics& g);
    void drawPitchCurve(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    
    //==============================================================================
    // Editing
    int getNoteAtPosition(juce::Point<float> pos) const;
    void startDraggingNote(int noteIndex, juce::Point<float> startPos);
    void dragNote(juce::Point<float> currentPos);
    void finishDraggingNote();
    void splitNoteAt(int noteIndex, double time);
    void mergeNotes(int firstIndex, int secondIndex);
    
    //==============================================================================
    // Analysis
    void detectNotesFromPitchCurve();
    float snapPitchToScale(float pitchHz) const;
    float snapTimeToGrid(double time) const;
    
    //==============================================================================
    // Data
    std::vector<GraphNote> notes_;
    std::vector<float> pitchCurve_;     // Raw pitch detection per frame
    std::vector<PitchCurvePoint> editedCurve_;  // User-edited pitch points
    double sampleRate_ = 44100.0;
    double audioDuration_ = 0.0;
    
    // View settings
    double viewStartTime_ = 0.0;
    double viewEndTime_ = 10.0;
    float centerPitchHz_ = 440.0f;  // A4
    float semitoneRange_ = 24.0f;    // ±12 semitones
    
    // Grid
    bool snapToGrid_ = true;
    bool snapToScale_ = true;
    int beatsPerBar_ = 4;
    int gridDivisions_ = 16;  // 16th notes
    
    // Display options
    bool showPitchCurve_ = true;
    bool showNoteBlocks_ = true;
    bool showGrid_ = true;
    
    // Editing state
    EditMode editMode_ = EditMode::Select;
    int draggingNoteIndex_ = -1;
    juce::Point<float> dragStartPos_;
    double dragStartTime_ = 0.0f;
    float dragStartPitch_ = 0.0f;
    bool isDragging_ = false;
    
    // Playback
    double playheadTime_ = 0.0;
    
    // Pitch detection for analysis
    std::unique_ptr<dsp::PitchDetector> pitchDetector_;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchGraphEditor)
};

} // namespace ui
} // namespace zenith
