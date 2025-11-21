/**
 * @file SkiaClipRenderer.h
 * @brief GPU-accelerated clip and arranger rendering
 *
 * Features:
 * - GPU-accelerated audio/MIDI clip visualization
 * - Real-time waveform display with GPU rendering
 * - 3D depth effects (shadows, gradients, highlights)
 * - Smooth spring physics animations
 * - Professional timeline/arranger view
 * - Optimized for large projects (100s of clips)
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include "SkiaWaveformRenderer.h"
    #include "SkiaTextRenderer.h"
    #include "SkiaTheme.h"
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @struct ClipVisual
 * @brief Visual representation of an audio/MIDI clip
 */
struct ClipVisual
{
    juce::String clipId;                ///< Unique clip ID
    juce::String trackId;               ///< Parent track ID
    juce::String name;                  ///< Clip name

    double startBeats;                  ///< Start time in beats
    double lengthBeats;                 ///< Duration in beats
    bool isMidi;                        ///< MIDI vs audio clip

    SkRect bounds;                      ///< Screen rectangle

    bool isSelected = false;            ///< Selection state
    bool isHovered = false;             ///< Hover state
    bool isMuted = false;               ///< Muted flag
    bool isLooping = false;             ///< Loop enabled

    // Animation state (spring physics)
    float hoverProgress = 0.0f;         ///< Hover animation (0-1)
    float hoverVelocity = 0.0f;
    float selectProgress = 0.0f;        ///< Selection animation (0-1)
    float selectVelocity = 0.0f;
    float scaleProgress = 1.0f;         ///< Scale animation for emphasis
    float scaleVelocity = 0.0f;

    // Optional waveform data for audio clips
    WaveformData* waveformData = nullptr;
};

/**
 * @struct TrackLaneVisual
 * @brief Visual representation of a track lane in arranger
 */
struct TrackLaneVisual
{
    juce::String trackId;
    juce::String trackName;
    SkColor trackColor;
    SkRect bounds;
    int trackIndex;
    bool isMuted = false;
    bool isSoloed = false;
};

/**
 * @struct ClipRenderOptions
 * @brief Configuration for clip rendering
 */
struct ClipRenderOptions
{
    // Clip colors
    SkColor audioClipColor = SkColorSetARGB(255, 74, 144, 226);      // Blue
    SkColor midiClipColor = SkColorSetARGB(255, 126, 211, 33);       // Green
    SkColor selectedClipColor = SkColorSetARGB(255, 255, 255, 255);  // White
    SkColor mutedClipColor = SkColorSetARGB(255, 100, 100, 100);     // Gray

    // Effects
    bool useGradients = true;                   ///< Gradient fills
    bool use3DDepth = true;                     ///< Shadows and highlights
    bool showWaveforms = true;                  ///< Show audio waveforms
    bool enableGlow = true;                     ///< Glow on selected clips
    bool showClipNames = true;                  ///< Draw clip names

    // Track lanes
    SkColor trackLaneColor = SkColorSetARGB(255, 42, 42, 42);
    SkColor trackDividerColor = SkColorSetARGB(255, 26, 26, 26);
    SkColor trackNameBgColor = SkColorSetARGB(200, 32, 32, 32);

    // Grid
    SkColor gridLineColor = SkColorSetARGB(40, 255, 255, 255);
    SkColor barLineColor = SkColorSetARGB(80, 255, 255, 255);
    bool showGrid = true;

    // Timeline
    SkColor playheadColor = SkColorSetARGB(255, 255, 69, 58);        // Red
    SkColor loopRegionColor = SkColorSetARGB(60, 10, 132, 255);      // Blue

    // Animation
    bool enableAnimations = true;
    bool enableHoverScale = true;               ///< Scale up on hover

    // Factory presets
    static ClipRenderOptions producer()
    {
        ClipRenderOptions opts;
        opts.useGradients = true;
        opts.use3DDepth = true;
        opts.showWaveforms = true;
        opts.enableGlow = true;
        opts.showClipNames = true;
        opts.enableHoverScale = true;
        return opts;
    }

    static ClipRenderOptions minimal()
    {
        ClipRenderOptions opts;
        opts.useGradients = false;
        opts.use3DDepth = false;
        opts.showWaveforms = false;
        opts.enableGlow = false;
        opts.enableAnimations = false;
        return opts;
    }
};

/**
 * @class SkiaClipRenderer
 * @brief GPU-accelerated clip and arranger rendering
 *
 * High-performance rendering for DAW arranger view:
 * - GPU-accelerated clip rectangles with waveforms
 * - Smooth spring physics animations (60Hz+)
 * - Professional 3D depth effects
 * - Real-time waveform display
 * - Track lanes and timeline ruler
 *
 * Optimized for large sessions (100+ tracks, 1000+ clips).
 *
 * Usage:
 * @code
 * SkiaClipRenderer renderer;
 *
 * std::vector<ClipVisual> clips;
 * std::vector<TrackLaneVisual> tracks;
 * // ... populate ...
 *
 * renderer.renderArrangerView(canvas, clips, tracks, bounds,
 *                             pixelsPerBeat, trackHeight);
 * @endcode
 */
class SkiaClipRenderer
{
public:
    SkiaClipRenderer();
    ~SkiaClipRenderer();

    /**
     * @brief Render complete arranger view
     * @param canvas Skia canvas
     * @param clips Array of clips to render
     * @param tracks Array of track lanes
     * @param bounds Total render bounds
     * @param pixelsPerBeat Horizontal zoom level
     * @param trackHeight Height of each track lane
     * @param viewStartBeats Horizontal scroll position
     * @param firstVisibleTrack Vertical scroll position (track index)
     * @param options Rendering options
     */
    void renderArrangerView(SkCanvas* canvas,
                           const std::vector<ClipVisual>& clips,
                           const std::vector<TrackLaneVisual>& tracks,
                           const SkRect& bounds,
                           float pixelsPerBeat,
                           int trackHeight,
                           double viewStartBeats = 0.0,
                           int firstVisibleTrack = 0,
                           const ClipRenderOptions& options = ClipRenderOptions::producer());

    /**
     * @brief Render track lanes background
     */
    void renderTrackLanes(SkCanvas* canvas,
                         const std::vector<TrackLaneVisual>& tracks,
                         const SkRect& bounds,
                         int trackHeight,
                         int firstVisibleTrack,
                         const ClipRenderOptions& options);

    /**
     * @brief Render timeline grid
     */
    void renderTimelineGrid(SkCanvas* canvas,
                           const SkRect& bounds,
                           float pixelsPerBeat,
                           double viewStartBeats,
                           const ClipRenderOptions& options);

    /**
     * @brief Render clips
     */
    void renderClips(SkCanvas* canvas,
                    const std::vector<ClipVisual>& clips,
                    const SkRect& bounds,
                    const ClipRenderOptions& options);

    /**
     * @brief Render single clip
     */
    void renderClip(SkCanvas* canvas,
                   const ClipVisual& clip,
                   const SkRect& bounds,
                   const ClipRenderOptions& options);

    /**
     * @brief Render playhead indicator
     */
    void renderPlayhead(SkCanvas* canvas,
                       const SkRect& bounds,
                       double playheadBeats,
                       float pixelsPerBeat,
                       double viewStartBeats,
                       const ClipRenderOptions& options);

    /**
     * @brief Render loop region
     */
    void renderLoopRegion(SkCanvas* canvas,
                         const SkRect& bounds,
                         double loopStartBeats,
                         double loopEndBeats,
                         float pixelsPerBeat,
                         double viewStartBeats,
                         const ClipRenderOptions& options);

    /**
     * @brief Update spring physics animations
     */
    void updateAnimations(std::vector<ClipVisual>& clips, float deltaTime);

    /**
     * @brief Set waveform renderer
     */
    void setWaveformRenderer(SkiaWaveformRenderer* renderer) { waveformRenderer_ = renderer; }

    /**
     * @brief Set text renderer
     */
    void setTextRenderer(SkiaTextRenderer* renderer) { textRenderer_ = renderer; }

private:
    SkiaWaveformRenderer* waveformRenderer_ = nullptr;
    SkiaTextRenderer* textRenderer_ = nullptr;
    std::unique_ptr<SkiaWaveformRenderer> ownedWaveformRenderer_;
    std::unique_ptr<SkiaTextRenderer> ownedTextRenderer_;

    // Helper methods
    void renderClipBackground(SkCanvas* canvas,
                             const SkRect& clipRect,
                             const ClipVisual& clip,
                             const ClipRenderOptions& options);

    void renderClipWaveform(SkCanvas* canvas,
                           const SkRect& clipRect,
                           const ClipVisual& clip,
                           const ClipRenderOptions& options);

    void renderClipGlow(SkCanvas* canvas,
                       const SkRect& clipRect,
                       SkColor glowColor,
                       float intensity);

    void renderClipName(SkCanvas* canvas,
                       const SkRect& clipRect,
                       const ClipVisual& clip,
                       const ClipRenderOptions& options);

    SkColor getClipColor(const ClipVisual& clip,
                        const ClipRenderOptions& options) const;

    void updateSpringAnimation(float& progress, float& velocity,
                              float target, float deltaTime,
                              float stiffness, float damping);
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
