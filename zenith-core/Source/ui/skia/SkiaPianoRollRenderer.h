/**
 * @file SkiaPianoRollRenderer.h
 * @brief GPU-accelerated piano roll renderer with professional effects
 *
 * Features:
 * - GPU-accelerated MIDI note rendering
 * - 3D depth effects (shadows, highlights, gradients)
 * - Smooth spring physics animations for note selection/hover
 * - Flashy text rendering for note names
 * - Professional grid with adaptive detail
 * - Velocity-based visual feedback
 * - High-performance rendering for large MIDI sequences
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include "SkiaTextRenderer.h"
    #include "SkiaTheme.h"
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @struct MidiNoteVisual
 * @brief Visual representation of a MIDI note for rendering
 */
struct MidiNoteVisual
{
    int pitch;                      ///< MIDI note number (0-127)
    double startBeats;              ///< Start time in beats
    double lengthBeats;             ///< Duration in beats
    int velocity;                   ///< Note velocity (0-127)

    SkRect bounds;                  ///< Screen rectangle

    bool isSelected = false;        ///< Selection state
    bool isHovered = false;         ///< Hover state
    bool isMuted = false;           ///< Muted flag

    // Animation state (spring physics)
    float hoverProgress = 0.0f;     ///< Hover animation (0-1)
    float hoverVelocity = 0.0f;     ///< Spring velocity
    float selectProgress = 0.0f;    ///< Selection animation (0-1)
    float selectVelocity = 0.0f;    ///< Spring velocity
};

/**
 * @struct PianoRollRenderOptions
 * @brief Configuration for piano roll rendering
 */
struct PianoRollRenderOptions
{
    // Note appearance
    SkColor noteColorDefault = SK_ColorGREEN;
    SkColor noteColorSelected = SK_ColorYELLOW;
    SkColor noteColorHover = SK_ColorCYAN;
    SkColor noteColorMuted = SK_ColorGRAY;

    bool useGradientNotes = true;           ///< Gradient fill for notes
    bool use3DDepth = true;                 ///< 3D shadows and highlights
    bool showVelocityColors = true;         ///< Color intensity by velocity
    bool enableGlow = true;                 ///< Glow effect on selected notes

    // Grid
    SkColor gridLineColor = SkColorSetARGB(40, 255, 255, 255);
    SkColor barLineColor = SkColorSetARGB(80, 255, 255, 255);
    SkColor octaveLineColor = SkColorSetARGB(60, 255, 255, 255);

    bool showGrid = true;
    bool showOctaveLines = true;

    // Piano keys
    SkColor whiteKeyColor = SkColorSetARGB(255, 74, 74, 74);
    SkColor blackKeyColor = SkColorSetARGB(255, 26, 26, 26);
    SkColor keyBorderColor = SK_ColorBLACK;

    bool useFlashyText = true;              ///< Use Skia text renderer for labels

    // Animation
    bool enableAnimations = true;

    // Factory presets
    static PianoRollRenderOptions producer()
    {
        PianoRollRenderOptions opts;
        opts.useGradientNotes = true;
        opts.use3DDepth = true;
        opts.showVelocityColors = true;
        opts.enableGlow = true;
        opts.useFlashyText = true;
        return opts;
    }

    static PianoRollRenderOptions minimal()
    {
        PianoRollRenderOptions opts;
        opts.useGradientNotes = false;
        opts.use3DDepth = false;
        opts.showVelocityColors = false;
        opts.enableGlow = false;
        opts.useFlashyText = false;
        return opts;
    }
};

/**
 * @class SkiaPianoRollRenderer
 * @brief High-performance GPU-accelerated piano roll renderer
 *
 * Renders professional MIDI piano roll with:
 * - GPU-accelerated note rectangles with gradients and 3D depth
 * - Smooth spring physics animations (60Hz+)
 * - Flashy text rendering for note labels
 * - Adaptive grid detail based on zoom level
 * - Velocity-sensitive visual feedback
 *
 * Optimized for:
 * - Large MIDI sequences (1000s of notes)
 * - Real-time performance (60-120 FPS)
 * - Professional production use
 *
 * Usage:
 * @code
 * SkiaPianoRollRenderer renderer;
 *
 * // Update note visuals
 * std::vector<MidiNoteVisual> notes;
 * // ... populate notes ...
 *
 * // Render to Skia canvas
 * renderer.render(canvas, notes, bounds, 80.0f, 12.0f);
 * @endcode
 */
class SkiaPianoRollRenderer
{
public:
    SkiaPianoRollRenderer();
    ~SkiaPianoRollRenderer();

    /**
     * @brief Render complete piano roll view
     * @param canvas Skia canvas
     * @param notes Array of note visuals
     * @param bounds Total render bounds
     * @param pixelsPerBeat Horizontal zoom level
     * @param pixelsPerPitch Vertical zoom level (note height)
     * @param pianoKeysWidth Width of piano keys panel
     * @param lowestPitch Lowest visible MIDI note
     * @param highestPitch Highest visible MIDI note
     * @param options Rendering options
     */
    void render(SkCanvas* canvas,
                const std::vector<MidiNoteVisual>& notes,
                const SkRect& bounds,
                float pixelsPerBeat,
                float pixelsPerPitch,
                float pianoKeysWidth = 60.0f,
                int lowestPitch = 0,
                int highestPitch = 127,
                const PianoRollRenderOptions& options = PianoRollRenderOptions::producer());

    /**
     * @brief Render only the grid
     */
    void renderGrid(SkCanvas* canvas,
                   const SkRect& bounds,
                   float pixelsPerBeat,
                   float pixelsPerPitch,
                   int lowestPitch,
                   int highestPitch,
                   const PianoRollRenderOptions& options);

    /**
     * @brief Render piano keys panel
     */
    void renderPianoKeys(SkCanvas* canvas,
                        const SkRect& bounds,
                        float pixelsPerPitch,
                        int lowestPitch,
                        int highestPitch,
                        const PianoRollRenderOptions& options);

    /**
     * @brief Render MIDI notes
     */
    void renderNotes(SkCanvas* canvas,
                    const std::vector<MidiNoteVisual>& notes,
                    const SkRect& bounds,
                    float pixelsPerPitch,
                    int lowestPitch,
                    const PianoRollRenderOptions& options);

    /**
     * @brief Update spring physics animations
     * @param notes Note visuals to animate (modified in-place)
     * @param deltaTime Time since last update (seconds)
     */
    void updateAnimations(std::vector<MidiNoteVisual>& notes, float deltaTime);

    /**
     * @brief Set text renderer
     */
    void setTextRenderer(SkiaTextRenderer* textRenderer) { textRenderer_ = textRenderer; }

private:
    SkiaTextRenderer* textRenderer_ = nullptr;
    std::unique_ptr<SkiaTextRenderer> ownedTextRenderer_;

    // Helper methods
    void renderNote(SkCanvas* canvas,
                   const MidiNoteVisual& note,
                   const SkRect& bounds,
                   float pixelsPerPitch,
                   int lowestPitch,
                   const PianoRollRenderOptions& options);

    void renderNoteWithEffects(SkCanvas* canvas,
                              const SkRect& noteRect,
                              const MidiNoteVisual& note,
                              const PianoRollRenderOptions& options);

    SkColor getNoteColor(const MidiNoteVisual& note,
                        const PianoRollRenderOptions& options) const;

    void renderNoteShadow(SkCanvas* canvas,
                         const SkRect& noteRect,
                         const PianoRollRenderOptions& options);

    void renderNoteGlow(SkCanvas* canvas,
                       const SkRect& noteRect,
                       SkColor glowColor,
                       float intensity);

    bool isBlackKey(int pitch) const;
    juce::String getPitchName(int pitch) const;

    // Spring physics
    void updateSpringAnimation(float& progress, float& velocity,
                              float target, float deltaTime,
                              float stiffness, float damping);
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
