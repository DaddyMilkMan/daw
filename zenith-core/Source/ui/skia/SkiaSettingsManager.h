/**
 * @file SkiaSettingsManager.h
 * @brief Persistent settings management for Skia UI theme and physics
 *
 * Handles:
 * - Theme mode persistence (Dark/Light)
 * - Spring physics customization per component
 * - GPU rendering settings (FPS, quality, antialiasing)
 * - User preferences save/load from config files
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
#include "SkiaTheme.h"

namespace zenith {

/**
 * @class SkiaSettingsManager
 * @brief Persistent settings for Skia UI rendering
 *
 * Singleton pattern for accessing and persisting user preferences.
 * Automatically loads settings on first access and saves on changes.
 */
class SkiaSettingsManager
{
public:
    /**
     * @brief Get the singleton instance
     */
    static SkiaSettingsManager& getInstance();

    //==========================================================================
    // Theme Settings
    //==========================================================================

    /**
     * @brief Get the saved theme mode
     */
    ThemeMode getThemeMode() const { return themeMode_; }

    /**
     * @brief Set and persist theme mode
     */
    void setThemeMode(ThemeMode mode);

    /**
     * @brief Get whether dark mode is preferred
     */
    bool isDarkModePreferred() const { return themeMode_ == ThemeMode::Dark; }

    //==========================================================================
    // Spring Physics Settings
    //==========================================================================

    /**
     * @brief Get button physics settings
     */
    SpringPhysicsSettings getButtonPhysics() const { return buttonPhysics_; }

    /**
     * @brief Set and persist button physics
     */
    void setButtonPhysics(const SpringPhysicsSettings& settings);

    /**
     * @brief Get slider physics settings
     */
    SpringPhysicsSettings getSliderPhysics() const { return sliderPhysics_; }

    /**
     * @brief Set and persist slider physics
     */
    void setSliderPhysics(const SpringPhysicsSettings& settings);

    /**
     * @brief Get knob physics settings
     */
    SpringPhysicsSettings getKnobPhysics() const { return knobPhysics_; }

    /**
     * @brief Set and persist knob physics
     */
    void setKnobPhysics(const SpringPhysicsSettings& settings);

    /**
     * @brief Get waveform physics settings
     */
    SpringPhysicsSettings getWaveformPhysics() const { return waveformPhysics_; }

    /**
     * @brief Set and persist waveform physics
     */
    void setWaveformPhysics(const SpringPhysicsSettings& settings);

    /**
     * @brief Get timeline physics settings
     */
    SpringPhysicsSettings getTimelinePhysics() const { return timelinePhysics_; }

    /**
     * @brief Set and persist timeline physics
     */
    void setTimelinePhysics(const SpringPhysicsSettings& settings);

    //==========================================================================
    // GPU Settings
    //==========================================================================

    /**
     * @brief Get GPU rendering settings
     */
    SkiaTheme::GPUSettings getGPUSettings() const { return gpuSettings_; }

    /**
     * @brief Set and persist GPU settings
     */
    void setGPUSettings(const SkiaTheme::GPUSettings& settings);

    /**
     * @brief Get target FPS
     */
    int getTargetFPS() const { return gpuSettings_.targetFPS; }

    /**
     * @brief Set target FPS and persist
     */
    void setTargetFPS(int fps);

    /**
     * @brief Get adaptive FPS enabled state
     */
    bool isAdaptiveFPSEnabled() const { return gpuSettings_.adaptiveFPS; }

    /**
     * @brief Enable/disable adaptive FPS and persist
     */
    void setAdaptiveFPS(bool enabled);

    //==========================================================================
    // Depth Settings
    //==========================================================================

    /**
     * @brief Get depth style
     */
    DepthStyle getDepthStyle() const { return depthStyle_; }

    /**
     * @brief Set and persist depth style
     */
    void setDepthStyle(const DepthStyle& style);

    //==========================================================================
    // File Operations
    //==========================================================================

    /**
     * @brief Load all settings from disk
     * @return True if settings were successfully loaded
     */
    bool loadSettings();

    /**
     * @brief Save all settings to disk
     * @return True if settings were successfully saved
     */
    bool saveSettings();

    /**
     * @brief Reset all settings to defaults
     */
    void resetToDefaults();

    /**
     * @brief Get the settings file path
     */
    juce::File getSettingsFile() const;

private:
    SkiaSettingsManager();
    ~SkiaSettingsManager() = default;

    // Delete copy/move
    SkiaSettingsManager(const SkiaSettingsManager&) = delete;
    SkiaSettingsManager& operator=(const SkiaSettingsManager&) = delete;

    //==========================================================================
    // Helper methods
    //==========================================================================

    void loadFromProperties(const juce::PropertiesFile::Options& options);
    void saveToProperties(const juce::PropertiesFile::Options& options);

    //==========================================================================
    // Member variables
    //==========================================================================

    ThemeMode themeMode_ = ThemeMode::Dark;

    SpringPhysicsSettings buttonPhysics_ = SpringPhysicsSettings::snappy();
    SpringPhysicsSettings sliderPhysics_ = SpringPhysicsSettings::smooth();
    SpringPhysicsSettings knobPhysics_ = SpringPhysicsSettings::smooth();
    SpringPhysicsSettings waveformPhysics_ = SpringPhysicsSettings::precise();
    SpringPhysicsSettings timelinePhysics_ = SpringPhysicsSettings::smooth();

    DepthStyle depthStyle_ = DepthStyle::moderate();
    SkiaTheme::GPUSettings gpuSettings_;

    std::unique_ptr<juce::PropertiesFile> propertiesFile_;
};

} // namespace zenith
