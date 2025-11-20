/*
  ==============================================================================

    PianoRollComponent.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 4: Piano Roll MIDI Editor

    Piano roll component for editing MIDI notes

    Responsibilities:
    - Display piano keys (left panel)
    - Draw note grid (time × pitch)
    - Render MIDI notes as rectangles
    - Handle note editing (create, move, delete)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include "../Source/engine/Track.h"

//==============================================================================
/**
    Visual representation of a MIDI note for hit-testing
*/
struct NoteVisual
{
    int noteNumber = 0;      // MIDI note number (0-127)
    double startTime = 0.0;  // Start time in seconds
    double duration = 0.0;   // Duration in seconds
    int velocity = 100;      // Velocity (0-127)
    juce::Rectangle<float> bounds;  // Screen position
};

//==============================================================================
/**
    Piano roll MIDI editor component
*/
class PianoRollComponent : public juce::Component,
                           public juce::KeyListener,
                           private juce::Timer
{
public:
    //==============================================================================
    PianoRollComponent(zenith::Track::Clip* clipToEdit);
    ~PianoRollComponent() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    //==============================================================================
    // Key listener (for delete key)
    bool keyPressed(const juce::KeyPress& key, Component* originatingComponent) override;

    //==============================================================================
    // View control
    void setPixelsPerBeat(float ppb);
    float getPixelsPerBeat() const { return pixelsPerBeat; }

    void setNoteHeight(float height);
    float getNoteHeight() const { return noteHeight; }

private:
    //==============================================================================
    // Timer callback (for animations)
    void timerCallback() override;

    //==============================================================================
    // Rendering helpers
    void drawPianoKeys(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawNotes(juce::Graphics& g, juce::Rectangle<int> bounds);

    //==============================================================================
    // Time/pitch mapping
    float beatsToPixels(double beats) const;
    double pixelsToBeats(float pixels) const;

    int pixelsToNoteNumber(float y) const;
    float noteNumberToPixels(int noteNumber) const;

    //==============================================================================
    // Note management
    void updateNoteCache();
    NoteVisual* hitTestNote(juce::Point<float> position);

    void createNote(int noteNumber, double startBeats, double lengthBeats, int velocity);
    void deleteNote(int noteNumber, double startTime);
    void moveNote(NoteVisual* note, int newNoteNumber, double newStartBeats);

    //==============================================================================
    // Grid snapping
    double snapToGrid(double beats) const;

    //==============================================================================
    // Member variables
    //==============================================================================

    zenith::Track::Clip* clip;  // Clip being edited (not owned)

    // View parameters
    float pixelsPerBeat = 40.0f;       // Horizontal zoom
    float noteHeight = 12.0f;          // Height of each semitone row
    float pianoKeysWidth = 60.0f;      // Width of piano keys panel

    // MIDI note range (88-key piano)
    int lowestNote = 21;   // A0
    int highestNote = 108; // C8

    // Tempo and sample rate (from clip/engine)
    double currentTempo = 120.0;
    double currentSampleRate = 44100.0;

    // Note visuals cache
    std::vector<NoteVisual> noteCache;
    NoteVisual* selectedNote = nullptr;

    // Mouse drag state
    bool isDragging = false;
    bool isCreatingNote = false;
    juce::Point<float> dragStartPosition;
    int noteDragStartNumber = 0;
    double noteDragStartBeats = 0.0;

    // Grid snap resolution (in beats)
    double gridResolution = 0.25;  // 1/16 note in 4/4 time

    // Hover state for micro-interactions
    NoteVisual* hoveredNote = nullptr;
    int hoveredPianoKey = -1;  // For piano key hover
    float hoverAlpha = 0.0f;
    
    // Selection animation
    float selectionAlpha = 0.0f;
    bool selectionAnimating = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};

//==============================================================================
/**
    Window that hosts a PianoRollComponent (similar to PluginEditorWindow)
*/
class PianoRollWindow : public juce::DocumentWindow
{
public:
    PianoRollWindow(zenith::Track::Clip* clip);
    ~PianoRollWindow() override;

    void closeButtonPressed() override;

    zenith::Track::Clip* getClip() const { return clip; }

private:
    zenith::Track::Clip* clip;
    std::unique_ptr<PianoRollComponent> pianoRoll;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWindow)
};
