/**
 * @file ArrangerRenderer.h
 * @brief Skia rendering for ArrangerComponent
 * 
 * This module handles all Skia-based drawing for the arranger view including:
 * - Grid and timeline rendering
 * - Track backgrounds and headers
 * - Clip rendering with glassmorphic effects
 * - Playhead, loop regions, and selection indicators
 * - Waveform and MIDI blob visualization
 */
#pragma once

#ifdef ZENITH_USE_SKIA

#include "ZenithSkia.h"

namespace zenith {

// Forward declarations
class ArrangerComponent;
class ArrangerClipManager;
class ArrangerGridUtils;
class ProjectState;
class Engine;
struct ClipView;

//==============================================================================
/**
 * @class ArrangerRenderer
 * @brief Handles all Skia rendering for the arranger view
 * 
 * Responsibilities:
 * - Drawing the timeline grid with bar/beat lines
 * - Rendering track backgrounds and dividers
 * - Drawing clips with premium glassmorphic effects
 * - Rendering playhead and loop regions
 * - Drawing waveform and MIDI thumbnails within clips
 */
class ArrangerRenderer {
public:
    /**
     * @brief Construct renderer for an arranger component
     * @param owner Reference to the owning ArrangerComponent
     * @param engine Reference to the audio engine
     * @param projectState Reference to the project state
     * @param clipManager Reference to the clip manager
     * @param gridUtils Reference to the grid utilities
     */
    ArrangerRenderer(ArrangerComponent& owner, Engine& engine, ProjectState& projectState,
                     ArrangerClipManager& clipManager, ArrangerGridUtils& gridUtils);

    /**
     * @brief Main Skia drawing entry point
     * @param canvas Skia canvas to draw on
     */
    void drawSkia(SkCanvas* canvas);

private:
    ArrangerComponent& owner_;         ///< Owning component
    Engine& engine_;                   ///< Audio engine reference
    ProjectState& projectState_;       ///< Project state reference
    ArrangerClipManager& clipManager_; ///< Clip manager reference
    ArrangerGridUtils& gridUtils_;     ///< Grid utilities reference

    //==========================================================================
    // Drawing Helpers
    //==========================================================================
    
    /**
     * @brief Draw the global background
     * @param canvas Skia canvas
     * @param width Component width
     * @param height Component height
     */
    void drawBackground(SkCanvas* canvas, float width, float height);
    
    /**
     * @brief Draw grid lines and bar highlights
     * @param canvas Skia canvas
     * @param width Component width
     * @param height Component height
     */
    void drawGrid(SkCanvas* canvas, float width, float height);
    
    /**
     * @brief Draw section highlighting for hovered/dragged sections
     * @param canvas Skia canvas
     * @param height Component height
     */
    void drawSectionHighlight(SkCanvas* canvas, float height);
    
    /**
     * @brief Draw track backgrounds (disabled - handled by TrackComponents)
     * @param canvas Skia canvas
     * @param width Component width
     * @param height Component height
     */
    void drawTracks(SkCanvas* canvas, float width, float height);
    
    /**
     * @brief Draw all clips with glassmorphic effects
     * @param canvas Skia canvas
     * @param width Component width
     * @param height Component height
     */
    void drawClips(SkCanvas* canvas, float width, float height);
    
    /**
     * @brief Draw a single clip with full premium styling
     * @param canvas Skia canvas
     * @param clipView Clip view data
     */
    void drawSingleClip(SkCanvas* canvas, const ClipView& clipView);
    
    /**
     * @brief Draw waveform visualization for an audio clip
     * @param canvas Skia canvas
     * @param clip Clip view data
     * @param contentRect Content area rectangle
     */
    void drawClipWaveform(SkCanvas* canvas, const ClipView& clip, const SkRect& contentRect);
    
    /**
     * @brief Draw MIDI note blobs for a MIDI clip
     * @param canvas Skia canvas
     * @param clip Clip view data
     * @param contentRect Content area rectangle
     */
    void drawClipMidiBlobs(SkCanvas* canvas, const ClipView& clip, const SkRect& contentRect);
    
    /**
     * @brief Draw marquee selection rectangle
     * @param canvas Skia canvas
     */
    void drawMarquee(SkCanvas* canvas);
    
    /**
     * @brief Draw section track component
     * @param canvas Skia canvas
     */
    void drawSectionTrack(SkCanvas* canvas);
    
    /**
     * @brief Draw playhead with neon glow effect
     * @param canvas Skia canvas
     * @param width Component width
     * @param height Component height
     */
    void drawPlayhead(SkCanvas* canvas, float width, float height);
    
    /**
     * @brief Draw loop region with bracket markers and highlight
     * @param canvas Skia canvas
     * @param width Component width
     * @param height Component height
     */
    void drawLoopRegion(SkCanvas* canvas, float width, float height);
    
    /**
     * @brief Draw insertion guide for Ripple/Insert edit modes
     * @param canvas Skia canvas
     * @param height Component height
     */
    void drawInsertionGuide(SkCanvas* canvas, float height);
};

} // namespace zenith

#endif // ZENITH_USE_SKIA
