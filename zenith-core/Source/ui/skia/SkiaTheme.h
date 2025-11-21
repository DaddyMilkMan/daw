/**
 * @file SkiaTheme.h
 * @brief Comprehensive theme system for Skia UI with dark/light modes
 *
 * Provides a complete design system with:
 * - Dark and light color schemes
 * - Full 3D depth effects (shadows, highlights, gradients)
 * - Optimized spring physics parameters per component type
 * - GPU-optimized rendering constants
 * - Typography settings for flashy Skia text
 */

#pragma once

#include <JuceHeader.h>
#ifdef ZENITH_USE_SKIA
    #include <include/core/SkColor.h>
#endif

namespace zenith {

/**
 * @enum ThemeMode
 * @brief Theme color mode
 */
enum class ThemeMode
{
    Dark,
    Light
};

/**
 * @struct SpringPhysicsSettings
 * @brief Spring animation parameters optimized per component type
 */
struct SpringPhysicsSettings
{
    float stiffness;   ///< Spring stiffness coefficient (higher = snappier)
    float damping;     ///< Damping coefficient (higher = less overshoot)
    float fps;         ///< Animation frame rate

    // Common presets
    static SpringPhysicsSettings snappy() { return {450.0f, 30.0f, 60.0f}; }      // Fast, minimal overshoot
    static SpringPhysicsSettings smooth() { return {350.0f, 25.0f, 60.0f}; }      // Balanced feel
    static SpringPhysicsSettings bouncy() { return {300.0f, 18.0f, 60.0f}; }      // More playful
    static SpringPhysicsSettings precise() { return {500.0f, 35.0f, 120.0f}; }    // Ultra-responsive, high FPS
};

/**
 * @struct DepthStyle
 * @brief 3D depth rendering parameters
 */
struct DepthStyle
{
    float shadowBlur;         ///< Shadow blur radius
    float shadowOffsetY;      ///< Shadow vertical offset
    float shadowOpacity;      ///< Shadow opacity (0-1)
    float highlightOpacity;   ///< Top highlight opacity (0-1)
    float innerShadowSize;    ///< Inner shadow/bevel size
    bool useGradients;        ///< Enable gradient fills

    // Preset styles
    static DepthStyle subtle() { return {4.0f, 2.0f, 0.3f, 0.15f, 1.0f, true}; }
    static DepthStyle moderate() { return {8.0f, 3.0f, 0.4f, 0.25f, 2.0f, true}; }
    static DepthStyle dramatic() { return {12.0f, 4.0f, 0.5f, 0.35f, 3.0f, true}; }
};

/**
 * @struct ThemeColors
 * @brief Complete color palette for a theme mode
 */
struct ThemeColors
{
    // Background colors
    SkColor background;           ///< Main window background
    SkColor backgroundSecondary;  ///< Secondary panels
    SkColor backgroundTertiary;   ///< Raised/inset elements

    // Surface colors (for controls)
    SkColor surfaceDefault;       ///< Default control surface
    SkColor surfaceHover;         ///< Hovered state
    SkColor surfaceActive;        ///< Active/pressed state
    SkColor surfaceDisabled;      ///< Disabled state

    // Accent colors
    SkColor primary;              ///< Primary accent (blue)
    SkColor primaryHover;
    SkColor primaryActive;

    SkColor success;              ///< Success/green actions
    SkColor successHover;

    SkColor danger;               ///< Danger/red actions
    SkColor dangerHover;

    SkColor warning;              ///< Warning/orange
    SkColor warningHover;

    // Text colors
    SkColor textPrimary;          ///< Main text
    SkColor textSecondary;        ///< Secondary text
    SkColor textDisabled;         ///< Disabled text
    SkColor textOnAccent;         ///< Text on colored backgrounds

    // Border/divider colors
    SkColor border;               ///< Standard borders
    SkColor borderHover;          ///< Hovered borders
    SkColor divider;              ///< Section dividers

    // Depth/shadow colors
    SkColor shadowDark;           ///< Drop shadow color
    SkColor shadowLight;          ///< Light shadow/highlight
    SkColor innerShadow;          ///< Inset shadow
    SkColor highlight;            ///< Top highlight

    // DAW-specific colors
    SkColor waveformAudio;        ///< Audio waveform color
    SkColor waveformMidi;         ///< MIDI notes color
    SkColor gridLine;             ///< Grid lines
    SkColor playhead;             ///< Playback position indicator
    SkColor loopRegion;           ///< Loop region highlight
    SkColor meterGreen;           ///< Audio meter green zone
    SkColor meterYellow;          ///< Audio meter yellow zone
    SkColor meterRed;             ///< Audio meter red zone
};

/**
 * @class SkiaTheme
 * @brief Comprehensive theme system for Skia-rendered UI
 *
 * Provides centralized access to:
 * - Color palettes (dark/light)
 * - Spring physics parameters per component type
 * - 3D depth rendering settings
 * - Typography and text effects
 * - GPU rendering optimizations
 */
class SkiaTheme
{
public:
    //==========================================================================
    // Singleton access
    //==========================================================================

    static SkiaTheme& getInstance();

    //==========================================================================
    // Theme mode
    //==========================================================================

    void setThemeMode(ThemeMode mode);
    ThemeMode getThemeMode() const { return currentMode_; }

    const ThemeColors& getColors() const { return colors_; }

    //==========================================================================
    // Spring physics presets per component type
    //==========================================================================

    SpringPhysicsSettings getButtonPhysics() const { return buttonPhysics_; }
    SpringPhysicsSettings getSliderPhysics() const { return sliderPhysics_; }
    SpringPhysicsSettings getKnobPhysics() const { return knobPhysics_; }
    SpringPhysicsSettings getWaveformPhysics() const { return waveformPhysics_; }
    SpringPhysicsSettings getTimelinePhysics() const { return timelinePhysics_; }

    void setButtonPhysics(const SpringPhysicsSettings& settings) { buttonPhysics_ = settings; }
    void setSliderPhysics(const SpringPhysicsSettings& settings) { sliderPhysics_ = settings; }
    void setKnobPhysics(const SpringPhysicsSettings& settings) { knobPhysics_ = settings; }
    void setWaveformPhysics(const SpringPhysicsSettings& settings) { waveformPhysics_ = settings; }
    void setTimelinePhysics(const SpringPhysicsSettings& settings) { timelinePhysics_ = settings; }

    //==========================================================================
    // Depth style
    //==========================================================================

    const DepthStyle& getDepthStyle() const { return depthStyle_; }
    void setDepthStyle(const DepthStyle& style) { depthStyle_ = style; }

    //==========================================================================
    // Typography (for Skia text rendering)
    //==========================================================================

    struct Typography
    {
        float baseSize = 14.0f;           ///< Base font size
        float headingSize = 18.0f;        ///< Heading size
        float smallSize = 12.0f;          ///< Small text
        float largeSize = 16.0f;          ///< Large text

        bool enableTextGlow = true;       ///< Enable glow effects
        float textGlowRadius = 3.0f;      ///< Glow blur radius
        float textGlowOpacity = 0.6f;     ///< Glow opacity

        bool enableTextShadow = true;     ///< Enable drop shadow
        float textShadowOffsetY = 1.0f;   ///< Shadow offset
        float textShadowBlur = 2.0f;      ///< Shadow blur
        float textShadowOpacity = 0.5f;   ///< Shadow opacity
    };

    const Typography& getTypography() const { return typography_; }
    void setTypography(const Typography& typo) { typography_ = typo; }

    //==========================================================================
    // GPU rendering settings
    //==========================================================================

    struct GPUSettings
    {
        int targetFPS = 60;                      ///< Target frame rate
        bool adaptiveFPS = true;                 ///< Auto-adjust FPS based on load
        bool prioritizeQuality = true;           ///< Visual quality over performance
        int waveformDetailLevel = 3;             ///< Waveform detail (1-5, higher = more detail)
        bool enableAntialiasing = true;          ///< MSAA/SSAA
        int msaaSamples = 4;                     ///< MSAA sample count (2, 4, 8, 16)
        bool enableMipmaps = true;               ///< Mipmaps for scaled images
    };

    const GPUSettings& getGPUSettings() const { return gpuSettings_; }
    void setGPUSettings(const GPUSettings& settings) { gpuSettings_ = settings; }

    //==========================================================================
    // Helper methods
    //==========================================================================

    /**
     * @brief Create a gradient from two colors
     * @param topColor Top of gradient
     * @param bottomColor Bottom of gradient
     * @param bounds Rectangle to fill
     * @return SkPaint configured with gradient
     */
#ifdef ZENITH_USE_SKIA
    static SkPaint createGradientPaint(SkColor topColor, SkColor bottomColor, const SkRect& bounds);
#endif

    /**
     * @brief Create a glow effect paint
     * @param color Glow color
     * @param radius Blur radius
     * @param opacity Glow opacity
     * @return SkPaint configured for glow
     */
#ifdef ZENITH_USE_SKIA
    static SkPaint createGlowPaint(SkColor color, float radius, float opacity);
#endif

    /**
     * @brief Create a shadow effect
     * @param offsetY Vertical offset
     * @param blur Blur radius
     * @param opacity Shadow opacity
     * @return SkPaint configured for shadow
     */
#ifdef ZENITH_USE_SKIA
    static SkPaint createShadowPaint(float offsetY, float blur, float opacity);
#endif

private:
    SkiaTheme();
    ~SkiaTheme() = default;

    // Delete copy/move
    SkiaTheme(const SkiaTheme&) = delete;
    SkiaTheme& operator=(const SkiaTheme&) = delete;

    void updateColorsForMode();

    ThemeMode currentMode_ = ThemeMode::Dark;
    ThemeColors colors_;

    // Physics settings per component
    SpringPhysicsSettings buttonPhysics_ = SpringPhysicsSettings::snappy();
    SpringPhysicsSettings sliderPhysics_ = SpringPhysicsSettings::smooth();
    SpringPhysicsSettings knobPhysics_ = SpringPhysicsSettings::smooth();
    SpringPhysicsSettings waveformPhysics_ = SpringPhysicsSettings::precise();
    SpringPhysicsSettings timelinePhysics_ = SpringPhysicsSettings::smooth();

    DepthStyle depthStyle_ = DepthStyle::moderate();
    Typography typography_;
    GPUSettings gpuSettings_;
};

} // namespace zenith

