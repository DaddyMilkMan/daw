/**
 * @file SkiaWaveformRenderer.h
 * @brief GPU-accelerated waveform rendering for audio visualization
 *
 * Features:
 * - High-performance GPU rendering of audio waveforms
 * - Multiple visualization modes (filled, outline, peaks, RMS)
 * - Adaptive detail level based on zoom
 * - Real-time waveform generation from audio buffers
 * - Smooth spring physics for waveform animations
 * - Gradient fills and visual effects
 * - Professional production-quality visualization
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
#include <vector>
#include <memory>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPath.h>
#endif

namespace zenith {

/**
 * @enum WaveformStyle
 * @brief Visualization style for waveforms
 */
enum class WaveformStyle
{
    Filled,         ///< Filled waveform (classic DAW style)
    Outline,        ///< Outline only
    Peaks,          ///< Peak markers (detailed view)
    RMS,            ///< RMS envelope
    Hybrid          ///< Peaks + RMS combined (professional)
};

/**
 * @struct WaveformData
 * @brief Cached waveform rendering data
 *
 * Pre-computed waveform data at various detail levels
 * for efficient GPU rendering at different zoom levels.
 */
struct WaveformData
{
    struct MinMaxPair
    {
        float min;
        float max;
        float rms;
    };

    std::vector<MinMaxPair> samples;  ///< Min/max/RMS per segment
    int sampleRate = 44100;           ///< Original audio sample rate
    double durationSeconds = 0.0;     ///< Total duration
    int numChannels = 1;              ///< Number of audio channels

    /**
     * @brief Generate waveform data from audio buffer
     * @param buffer Audio buffer to analyze
     * @param detailLevel Detail level (1-5, higher = more detail)
     */
    void generateFromAudioBuffer(const juce::AudioBuffer<float>& buffer, int detailLevel = 3);

    /**
     * @brief Generate waveform data from audio source
     * @param source Audio format reader
     * @param detailLevel Detail level
     */
    void generateFromAudioSource(juce::AudioFormatReader* source, int detailLevel = 3);

    /**
     * @brief Clear cached data
     */
    void clear();

    /**
     * @brief Check if data is valid
     */
    bool isValid() const { return !samples.empty(); }
};

#ifdef ZENITH_USE_SKIA

/**
 * @struct WaveformRenderOptions
 * @brief Rendering configuration for waveforms
 */
struct WaveformRenderOptions
{
    WaveformStyle style = WaveformStyle::Hybrid;

    SkColor fillColor = SK_ColorWHITE;          ///< Fill color
    SkColor outlineColor = SK_ColorWHITE;       ///< Outline color
    SkColor peakColor = SK_ColorWHITE;          ///< Peak marker color
    SkColor rmsColor = SK_ColorWHITE;           ///< RMS envelope color
    SkColor gradientTop = SK_ColorWHITE;        ///< Gradient top
    SkColor gradientBottom = SK_ColorWHITE;     ///< Gradient bottom

    float opacity = 1.0f;                       ///< Overall opacity
    float outlineWidth = 1.5f;                  ///< Outline stroke width
    float peakWidth = 1.0f;                     ///< Peak marker width

    bool useGradient = true;                    ///< Enable gradient fill
    bool showCenterLine = true;                 ///< Show zero-crossing line
    bool enableGlow = false;                    ///< Add glow effect
    float glowRadius = 3.0f;                    ///< Glow blur radius
    float glowOpacity = 0.4f;                   ///< Glow opacity

    bool antiAlias = true;                      ///< Anti-aliasing

    // Animation (using spring physics)
    bool animateEntry = true;                   ///< Animate waveform on load
    float animationProgress = 1.0f;             ///< Animation progress (0-1)

    // Factory presets
    static WaveformRenderOptions producer()
    {
        WaveformRenderOptions opts;
        opts.style = WaveformStyle::Hybrid;
        opts.useGradient = true;
        opts.showCenterLine = true;
        opts.enableGlow = false;
        return opts;
    }

    static WaveformRenderOptions minimal()
    {
        WaveformRenderOptions opts;
        opts.style = WaveformStyle::Outline;
        opts.useGradient = false;
        opts.showCenterLine = false;
        opts.enableGlow = false;
        return opts;
    }

    static WaveformRenderOptions flashy()
    {
        WaveformRenderOptions opts;
        opts.style = WaveformStyle::Filled;
        opts.useGradient = true;
        opts.enableGlow = true;
        opts.glowRadius = 5.0f;
        opts.glowOpacity = 0.6f;
        return opts;
    }
};

/**
 * @class SkiaWaveformRenderer
 * @brief GPU-accelerated waveform visualization
 *
 * Renders high-quality audio waveforms using Skia's GPU backend.
 * Optimized for real-time performance with adaptive detail levels.
 *
 * Features:
 * - Multi-resolution waveform data (LOD)
 * - Automatic detail level based on zoom
 * - GPU-accelerated path rendering
 * - Spring physics animations
 * - Professional visual effects
 *
 * Usage:
 * @code
 * SkiaWaveformRenderer renderer;
 *
 * // Generate waveform data from audio
 * WaveformData waveform;
 * waveform.generateFromAudioBuffer(audioBuffer, 4);
 *
 * // Render to Skia canvas
 * auto opts = WaveformRenderOptions::producer();
 * opts.fillColor = SK_ColorBLUE;
 * renderer.render(canvas, waveform, bounds, opts);
 * @endcode
 */
class SkiaWaveformRenderer
{
public:
    SkiaWaveformRenderer();
    ~SkiaWaveformRenderer();

    /**
     * @brief Render waveform to canvas
     * @param canvas Skia canvas
     * @param waveform Waveform data
     * @param bounds Rectangle to render into
     * @param options Rendering options
     */
    void render(SkCanvas* canvas,
                const WaveformData& waveform,
                const SkRect& bounds,
                const WaveformRenderOptions& options = WaveformRenderOptions::producer());

    /**
     * @brief Render waveform for a time range
     * @param canvas Skia canvas
     * @param waveform Waveform data
     * @param bounds Render bounds
     * @param startTime Start time in seconds
     * @param endTime End time in seconds
     * @param options Rendering options
     */
    void renderTimeRange(SkCanvas* canvas,
                        const WaveformData& waveform,
                        const SkRect& bounds,
                        double startTime,
                        double endTime,
                        const WaveformRenderOptions& options = WaveformRenderOptions::producer());

    /**
     * @brief Render MIDI notes as waveform-style visualization
     * @param canvas Skia canvas
     * @param notes MIDI note data (pitch, start, length, velocity)
     * @param bounds Render bounds
     * @param color Note color
     */
    void renderMidiNotes(SkCanvas* canvas,
                        const std::vector<std::tuple<int, double, double, int>>& notes,
                        const SkRect& bounds,
                        SkColor color);

    /**
     * @brief Set animation progress
     * @param progress Progress value (0-1)
     *
     * Used for spring-physics based entry animations.
     */
    void setAnimationProgress(float progress);

    /**
     * @brief Get current animation progress
     */
    float getAnimationProgress() const { return animationProgress_; }

private:
    // Animation state
    float animationProgress_ = 1.0f;
    float animationVelocity_ = 0.0f;
    juce::Time lastAnimationTime_;

    // Internal rendering methods
    void renderFilled(SkCanvas* canvas,
                     const WaveformData& waveform,
                     const SkRect& bounds,
                     size_t startSample,
                     size_t endSample,
                     const WaveformRenderOptions& options);

    void renderOutline(SkCanvas* canvas,
                      const WaveformData& waveform,
                      const SkRect& bounds,
                      size_t startSample,
                      size_t endSample,
                      const WaveformRenderOptions& options);

    void renderPeaks(SkCanvas* canvas,
                    const WaveformData& waveform,
                    const SkRect& bounds,
                    size_t startSample,
                    size_t endSample,
                    const WaveformRenderOptions& options);

    void renderRMS(SkCanvas* canvas,
                  const WaveformData& waveform,
                  const SkRect& bounds,
                  size_t startSample,
                  size_t endSample,
                  const WaveformRenderOptions& options);

    void renderHybrid(SkCanvas* canvas,
                     const WaveformData& waveform,
                     const SkRect& bounds,
                     size_t startSample,
                     size_t endSample,
                     const WaveformRenderOptions& options);

    // Helper methods
    SkPath createWaveformPath(const WaveformData& waveform,
                             const SkRect& bounds,
                             size_t startSample,
                             size_t endSample,
                             bool includeBottom = true);

    void applyGlowEffect(SkCanvas* canvas,
                        const SkPath& path,
                        const WaveformRenderOptions& options);

    size_t getSampleIndexForTime(const WaveformData& waveform, double timeSeconds) const;
    double getTimeForSampleIndex(const WaveformData& waveform, size_t index) const;
};

#endif // ZENITH_USE_SKIA

} // namespace zenith

