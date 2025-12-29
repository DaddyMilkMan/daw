/*
  ==============================================================================

    SkiaGridComponent.h
    Created: 2025-12-29
    Author:  Zenith DAW

    High-performance Session View grid using pure Skia rendering.
    - Tracks as columns, Scenes as rows (Ableton-style)
    - Interaction zones for Gain, Loop, Trigger
    - Lock-free audio synchronization
    - 60+ FPS with GPU-optimized rendering

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../../engine/Engine.h"
#include "../../engine/ProjectState.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkPath.h>
#include <include/core/SkImage.h>
#include <include/effects/SkGradientShader.h>
#include <include/core/SkMaskFilter.h>
#endif

namespace zenith {

//==============================================================================
// Interaction Zones
//==============================================================================
enum class InteractionZone {
    None,
    Gain,       // Top 15% - vertical drag for volume
    LoopStart,  // Left 10% - horizontal drag for loop start
    LoopEnd,    // Right 10% - horizontal drag for loop end
    Trigger     // Center - click to launch clip
};

//==============================================================================
// Clip Cell Data
//==============================================================================
struct SessionClipCell {
    juce::String clipId;
    int trackIndex = -1;
    int sceneIndex = -1;
    
    // State
    bool hasClip = false;
    bool isPlaying = false;
    bool isQueued = false;
    bool isSelected = false;
    bool isRecording = false;
    
    // Audio data
    bool isAudio = true;  // true = audio, false = MIDI
    float gainDb = 0.0f;
    float loopStart = 0.0f;
    float loopEnd = 1.0f;
    
    // Cached waveform data (normalized 0-1)
    std::vector<float> waveformPeaks;
    
    struct Note { int pitch; float start; float length; };
    std::vector<Note> midiNotes;
    
    // Visual state
    juce::Colour trackColor = juce::Colours::cyan;
    float hoverProgress = 0.0f;
    juce::Rectangle<float> bounds;
};

//==============================================================================
// SkiaGridComponent
//==============================================================================
class SkiaGridComponent : public SkiaComponent {
public:
    //==========================================================================
    // Construction
    //==========================================================================
    SkiaGridComponent(Engine& engine, ProjectState& projectState);
    ~SkiaGridComponent() override;

    //==========================================================================
    // SkiaComponent overrides
    //==========================================================================
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // Mouse Interaction
    //==========================================================================
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    //==========================================================================
    // Grid Configuration
    //==========================================================================
    void setGridSize(int numTracks, int numScenes);
    int getNumTracks() const { return numTracks_; }
    int getNumScenes() const { return numScenes_; }

    //==========================================================================
    // Clip Management
    //==========================================================================
    void setClipData(int trackIndex, int sceneIndex, const SessionClipCell& cell);
    SessionClipCell* getClipCell(int trackIndex, int sceneIndex);
    void triggerClip(int trackIndex, int sceneIndex);
    void triggerScene(int sceneIndex);
    void stopAllClips();

    //==========================================================================
    // Audio Thread Updates (Lock-Free)
    //==========================================================================
    void updatePlayingState(int trackIndex, int sceneIndex, bool playing);
    void updatePlayheadPosition(int trackIndex, float normalizedPosition);

private:
    //==========================================================================
    // Drawing Methods
    //==========================================================================
#ifdef ZENITH_USE_SKIA
    void drawBackground(SkCanvas* canvas);
    void drawGrid(SkCanvas* canvas);
    void drawTrackHeaders(SkCanvas* canvas);
    void drawSceneHeaders(SkCanvas* canvas);
    void drawCell(SkCanvas* canvas, int trackIndex, int sceneIndex);
    void drawCellContent(SkCanvas* canvas, const SessionClipCell& cell, const SkRect& cellRect);
    void drawWaveformSparkline(SkCanvas* canvas, const SessionClipCell& cell, const SkRect& rect);
    void drawMidiSparkline(SkCanvas* canvas, const SessionClipCell& cell, const SkRect& rect);
    void drawGlowEffect(SkCanvas* canvas, const SkRRect& rrect, SkColor color);
    void drawInteractionZoneHighlight(SkCanvas* canvas, const SkRect& cellRect, InteractionZone zone);
#endif

    //==========================================================================
    // Hit Testing
    //==========================================================================
    InteractionZone getZoneAt(float x, float y) const;
    juce::Point<int> getCellAt(float x, float y) const;
    juce::Rectangle<float> getCellBounds(int trackIndex, int sceneIndex) const;

    //==========================================================================
    // Helpers
    //==========================================================================
    void rebuildCellCache();
    void invalidateBackgroundCache();

    //==========================================================================
    // Dependencies
    //==========================================================================
    Engine& engine_;
    ProjectState& projectState_;

    //==========================================================================
    // Grid State
    //==========================================================================
    int numTracks_ = 8;
    int numScenes_ = 8;
    
    float cellWidth_ = 100.0f;
    float cellHeight_ = 80.0f;
    float trackHeaderHeight_ = 48.0f;
    float sceneHeaderWidth_ = 80.0f;
    
    std::vector<std::vector<SessionClipCell>> cells_;  // [track][scene]

    //==========================================================================
    // Interaction State
    //==========================================================================
    juce::Point<int> hoveredCell_{-1, -1};
    juce::Point<int> selectedCell_{-1, -1};
    InteractionZone hoveredZone_ = InteractionZone::None;
    InteractionZone activeZone_ = InteractionZone::None;
    
    float dragStartValue_ = 0.0f;
    juce::Point<float> dragStartPos_;

    //==========================================================================
    // Cached Rendering
    //==========================================================================
#ifdef ZENITH_USE_SKIA
    sk_sp<SkImage> backgroundCache_;
    bool backgroundDirty_ = true;
#endif

    //==========================================================================
    // Playhead State (from audio thread)
    //==========================================================================
    std::atomic<float> playheadPositions_[32]{};  // Per-track playhead (0-1)
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaGridComponent)
};

} // namespace zenith
