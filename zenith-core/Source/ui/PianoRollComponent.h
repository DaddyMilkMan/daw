/*
  ==============================================================================

    PianoRollComponent.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 4: Piano Roll MIDI Editor

    GPU-accelerated piano roll component with Skia rendering

    Features:
    - GPU-accelerated Skia rendering via SkiaCanvasComponent
    - Zebra striping for visual legibility (alternating dark backgrounds)
    - Smooth hover animations and note selection feedback
    - MIDI note editing (create, move, delete)
    - Professional piano keyboard display with proper key colors

    Responsibilities:
    - Display piano keys (left panel) with proper black/white key rendering
    - Draw note grid (time × pitch) with zebra striping
    - Render MIDI notes with smooth animations
    - Handle note editing interactions (create, move, delete)

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include "../engine/Track.h"

#ifdef ZENITH_USE_SKIA
#include "skia/SkiaCanvasComponent.h"
#endif

class Engine; // Forward declaration

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
    Piano roll MIDI editor component with GPU-accelerated Skia rendering

    When ZENITH_USE_SKIA is enabled, inherits from SkiaCanvasComponent for
    GPU acceleration. Otherwise falls back to standard JUCE rendering.
*/
#ifdef ZENITH_USE_SKIA
class PianoRollComponent : public zenith::SkiaCanvasComponent,
                           public juce::KeyListener,
                           private juce::Timer
{
public:
    //==============================================================================
    PianoRollComponent(zenith::Track::Clip* clipToEdit, Engine& engine);
    ~PianoRollComponent() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

protected:
    //==============================================================================
    // Skia rendering override
    void paintSkia(SkCanvas& canvas, const juce::Rectangle<int>& bounds) override;

#else
class PianoRollComponent : public juce::Component,
                           public juce::KeyListener,
                           private juce::Timer
{
public:
    //==============================================================================
    PianoRollComponent(zenith::Track::Clip* clipToEdit, Engine& engine);
    ~PianoRollComponent() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

#endif  // ZENITH_USE_SKIA

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
    void setPixelsPerBeat(float ppb) [[maybe_unused]];
    float getPixelsPerBeat() const { return pixelsPerBeat; }

    void setNoteHeight(float height) [[maybe_unused]];
    float getNoteHeight() const { return noteHeight; }

private:
    //==============================================================================
    // Timer callback (for animations)
    void timerCallback() override;

    //==============================================================================
    // Rendering helpers (JUCE Graphics fallback)
    void drawPianoKeys(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawNotes(juce::Graphics& g, juce::Rectangle<int> bounds);

#ifdef ZENITH_USE_SKIA
    //==============================================================================
    // Skia-specific rendering helpers
    void drawPianoKeysSkia(SkCanvas& canvas, float x, float y, float width, float height);
    void drawGridSkia(SkCanvas& canvas, const juce::Rectangle<int>& bounds);
    void drawNotesSkia(SkCanvas& canvas, const juce::Rectangle<int>& bounds);
    void drawZebraStripingSkia(SkCanvas& canvas, const juce::Rectangle<int>& bounds);
#endif

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

    void createNote(int noteNumber, double startBeats, double lengthBeats, int velocity) [[maybe_unused]];
    void deleteNote(int noteNumber, double startTime) [[maybe_unused]];
    void moveNote(NoteVisual* note, int newNoteNumber, double newStartBeats) [[maybe_unused]];

    //==============================================================================
    // Grid snapping
    double snapToGrid(double beats) const;

    //==============================================================================
    // Member variables
    //==============================================================================

    zenith::Track::Clip* clip;  // Clip being edited (not owned)
    Engine& engine;             // Reference to engine

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
    PianoRollWindow(zenith::Track::Clip* clip, Engine& engine);
    ~PianoRollWindow() override;

    void closeButtonPressed() override;

    zenith::Track::Clip* getClip() const { return clip; }

private:
    zenith::Track::Clip* clip;
    Engine& engine;
    std::unique_ptr<PianoRollComponent> pianoRoll;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWindow)
};
