/*
  ==============================================================================

    ArrangerComponent.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 4: Timeline/Arranger View

    Main timeline/arranger component

    Responsibilities:
    - Display tracks and clips in a scrollable timeline
    - Draw time ruler (bars/beats grid)
    - Render playhead
    - Handle clip selection and interaction
    - Open piano roll on MIDI clip double-click

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "../Source/engine/Track.h"

// Forward declarations
class ProjectState;

// Forward declarations
class Engine;

//==============================================================================
/**
    Visual representation of a clip for hit-testing (no Component overhead)
*/
struct ClipVisual
{
    zenith::Track* track = nullptr;
    zenith::Track::Clip* clip = nullptr;
    juce::Rectangle<float> bounds;
    bool isMidi = false;
    int trackIndex = -1;
};

//==============================================================================
/**
    Arranger/Timeline component showing tracks and clips
*/
class ArrangerComponent : public juce::Component,
                          private juce::Timer
{
public:
    //==============================================================================
    ArrangerComponent(ProjectState& projectState);
    ~ArrangerComponent() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    //==============================================================================
    // View control
    void setPixelsPerBeat(float ppb) [[maybe_unused]];
    float getPixelsPerBeat() const { return pixelsPerBeat; }

    void setTrackHeight(float height) [[maybe_unused]];
    float getTrackHeight() const { return trackHeight; }

    //==============================================================================
    // Selection
    ClipVisual* getSelectedClip() { return selectedClip; }
    const ClipVisual* getSelectedClip() const { return selectedClip; }

    void clearSelection();

private:
    //==============================================================================
    // Timer callback (for playhead animation)
    void timerCallback() override;

    //==============================================================================
    // Rendering helpers
    void drawTimeRuler(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawPlayhead(juce::Graphics& g, juce::Rectangle<int> bounds);

    //==============================================================================
    // Time mapping
    float beatsToPixels(double beats) const;
    double pixelsToBeats(float pixels) const;
    int64_t beatsToSamples(double beats) const;
    double samplesToBeats(int64_t samples) const;

    //==============================================================================
    // Clip cache management
    void updateClipCache();
    ClipVisual* hitTestClip(juce::Point<float> position);

    //==============================================================================
    // Member variables
    //==============================================================================

    Engine& engine;

    // View parameters
    float pixelsPerBeat = 40.0f;  // Zoom level (horizontal)
    float trackHeight = 80.0f;     // Height of each track lane
    float rulerHeight = 30.0f;     // Height of time ruler at top

    // Cached tempo (fetched from Engine or default)
    double currentTempo = 120.0;
    double currentSampleRate = 44100.0;

    // Clip visuals cache (rebuilt when tracks change)
    std::vector<ClipVisual> clipCache;
    ClipVisual* selectedClip = nullptr;

    // Mouse drag state
    bool isDragging = false;
    juce::Point<float> dragStartPosition;
    int64_t clipDragStartSamples = 0;

    // Hover state for micro-interactions
    ClipVisual* hoveredClip = nullptr;
    float hoverAlpha = 0.0f;
    
    // Selection animation
    float selectionAlpha = 0.0f;
    bool selectionAnimating = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

