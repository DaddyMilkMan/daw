/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// ViewTheme.h


#include "../design-system/ZenithTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <string>
#include <vector>

namespace zenith::ui {

/**
 * @brief Configuration system for SkiaSessionView themes
 *
 * This system provides a comprehensive configuration for all visual aspects
 * of the session view, allowing for easy customization and consistent theming
 * across the application.
 */
class ViewTheme {
public:
    //==============================================================================
    // Layout Configuration
    //==============================================================================

    struct Layout {
        // Core dimensions
        float sceneLauncherWidth = 100.0f;
        float trackWidth = 140.0f;
        float clipSlotHeight = 80.0f;
        float trackHeaderHeight = 44.0f;
        float mixerHeight = 100.0f;
        float stopRowHeight = 36.0f;

        // Spacing
        float clipPadding = 6.0f;
        float scenePadding = 6.0f;
        float headerPadding = 8.0f;
        float mixerPadding = 8.0f;

        // Border radius
        float clipSlotRadius = 12.0f;
        float buttonRadius = 12.0f;
        float sceneRadius = 10.0f;
        float panelRadius = 8.0f;

        // Component sizes
        float stopButtonSize = 16.0f;
        float soloMuteButtonSize = 24.0f;
        float faderWidth = 8.0f;
        float meterWidth = 4.0f;

        // Animation
        float animationSpeed = 1.0f;
        float glowIntensity = 1.0f;

        // Sizing presets
        enum class Preset {
            Compact,    // Smaller elements for dense layouts
            Standard,   // Default sizing
            Expanded    // Larger elements for better visibility
        };

        static Layout createPreset(Preset preset);
        void applyPreset(Preset preset);
    };

    //==============================================================================
    // Color Configuration
    //==============================================================================

    struct Colors {
        // Background colors
        juce::Color backgroundGradientStart;
        juce::Color backgroundGradientEnd;
        juce::Color backgroundNoise;

        // Panel colors
        juce::Color sceneLauncherBg;
        juce::Color trackHeaderBg;
        juce::Color clipGridBg;
        juce::Color mixerBg;
        juce::Color stopRowBg;

        // Border colors
        juce::Color sceneLauncherBorder;
        juce::Color trackDividerBorder;
        juce::Color sectionDividerBorder;
        juce::Color focusBorder;

        // Text colors
        juce::Color sceneText;
        juce::Color trackHeaderText;
        juce::Color clipText;
        juce::Color meterText;

        // Clip state colors
        juce::Color emptySlotColor;
        juce::Color stoppedClipColor;
        juce::Color playingClipColor;
        juce::Color queuedClipColor;
        juce::Color recordingClipColor;
        juce::Color stoppingClipColor;

        // Button colors
        juce::Color stopButtonColor;
        juce::Color soloButtonColor;
        juce::Color muteButtonColor;
        juce::Color accentButtonColor;

        // Meter colors
        juce::Color meterSafeColor;
        juce::Color meterWarningColor;
        juce::Color meterHotColor;

        // Animation colors
        juce::Color playingIndicatorColor;
        juce::Color queuedIndicatorColor;
        juce::Color glowColor;

        // Selection colors
        juce::Color selectionColor;
        juce::Color hoverColor;
        juce::Color selectionGlowColor;

        // Opacity levels
        float hoverOverlayOpacity = 0.08f;
        float selectionGlowOpacity = 0.3f;
        float glowEffectOpacity = 0.4f;

        // Theme presets
        enum class Preset {
            NeonNoir,      // Original neon aesthetic
            DarkModern,    // Clean dark theme
            OLED,          // Pure black theme
            HighContrast   // Accessibility theme
        };

        static Colors createPreset(Preset preset);
        void applyPreset(Preset preset);

        // Utility methods
        juce::Color withAlpha(const juce::Color& color, float alpha) const;
        juce::Color lighten(const juce::Color& color, float amount) const;
        juce::Color darken(const juce::Color& color, float amount) const;
    };

    //==============================================================================
    // Animation Configuration
    //==============================================================================

    struct Animation {
        // Timing
        float pulseSpeed = 1.0f;
        float glowSpeed = 1.0f;
        float selectionSpeed = 1.0f;

        // Intensity
        float glowIntensity = 0.6f;
        float pulseIntensity = 0.3f;
        float selectionIntensity = 0.25f;

        // Easing
        enum class Easing {
            Linear,
            Sine,
            Cosine,
            EaseIn,
            EaseOut,
            EaseInOut
        };

        Easing glowEasing = Easing::Sine;
        Easing pulseEasing = Easing::Cosine;
        Easing selectionEasing = Easing::EaseInOut;

        // Presets
        enum class Preset {
            Subtle,    // Minimal animations
            Normal,    // Standard animations
            Vibrant    // Energetic animations
        };

        static Animation createPreset(Preset preset);
        void applyPreset(Preset preset);
    };

    //==============================================================================
    // Error Handling Configuration
    //==============================================================================

    struct ErrorHandling {
        // Visual error indicators
        juce::Color errorColor;
        juce::Color warningColor;
        juce::Color successColor;

        // Error display settings
        float errorGlowRadius = 8.0f;
        float errorPulseSpeed = 2.0f;
        float errorDuration = 3000.0f; // milliseconds

        // Recovery settings
        bool enableAutoRecovery = true;
        float recoveryDelay = 1000.0f; // milliseconds
        int maxRetries = 3;

        // Fallback settings
        juce::Color fallbackBgColor;
        juce::Color fallbackTextColor;
        bool useSimpleFallback = false;

        // Logging
        bool logErrors = true;
        bool logWarnings = true;
        bool logPerformance = false;

        // Presets
        enum class Preset {
            Strict,     // Aggressive error handling
            Balanced,   // Moderate error handling
            Lenient     // Minimal error handling
        };

        static ErrorHandling createPreset(Preset preset);
        void applyPreset(Preset preset);
    };

    //==============================================================================
    // Complete Theme Configuration
    //==============================================================================

    struct Theme {
        Layout layout;
        Colors colors;
        Animation animation;
        ErrorHandling errorHandling;

        // Theme metadata
        std::string name;
        std::string description;
        std::string author;
        std::string version;

        // Built-in themes
        enum class BuiltIn {
            NeonNoir,
            DarkModern,
            OLED,
            HighContrast,
            Studio
        };

        static Theme createBuiltIn(BuiltIn theme);
        static std::vector<Theme> getAllBuiltInThemes();

        // Validation
        bool isValid() const;
        void validate();

        // Serialization
        bool saveToFile(const juce::String& filePath) const;
        static Theme loadFromFile(const juce::String& filePath);

        // Preset application
        void applyPreset(const std::string& presetName);
    };

    //==============================================================================
    // Theme Manager
    //==============================================================================

    class ThemeManager {
    public:
        static ThemeManager& getInstance();

        // Theme management
        void setTheme(const Theme& theme);
        const Theme& getCurrentTheme() const;
        std::string getCurrentThemeName() const;

        // Built-in themes
        void applyBuiltInTheme(Theme::BuiltIn theme);
        std::vector<Theme> getAvailableThemes() const;

        // Custom themes
        bool saveCustomTheme(const Theme& theme, const std::string& name);
        bool loadCustomTheme(const std::string& name);
        bool deleteCustomTheme(const std::string& name);
        std::vector<std::string> getCustomThemeNames() const;

        // Settings management
        void saveSettings(const juce::String& filePath) const;
        void loadSettings(const juce::String& filePath);

        // Global settings
        void setGlobalAnimationSpeed(float speed);
        void setGlobalGlowIntensity(float intensity);
        void setErrorLoggingEnabled(bool enabled);

        // Error handling
        void reportError(const std::string& component, const std::string& error);
        void reportWarning(const std::string& component, const std::string& warning);
        void logPerformance(const std::string& operation, float duration);

        // Observers
        void addThemeChangeListener(std::function<void(const Theme&)> listener);
        void removeThemeChangeListener(std::function<void(const Theme&)> listener);

    private:
        ThemeManager();
        ~ThemeManager();

        Theme currentTheme_;
        std::map<std::string, Theme> customThemes_;
        std::vector<std::function<void(const Theme&)>> listeners_;

        // Error handling state
        struct ErrorState {
            std::string lastError;
            std::string lastWarning;
            float lastErrorTime = 0.0f;
            int errorCount = 0;
            int warningCount = 0;
        } errorState_;

        // Performance tracking
        struct PerformanceTracker {
            std::map<std::string, float> operationTimes;
            float totalTime = 0.0f;
            int operationCount = 0;
        } performanceTracker_;

        // Utility methods
        void notifyThemeChanged();
        void validateCurrentTheme();
        void applyFallbackTheme();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeManager)
    };

    //==============================================================================
    // Global accessors
    //==============================================================================

    // Get the current theme
    static const Theme& getCurrentTheme();

    // Get theme manager
    static ThemeManager& getThemeManager();

    // Quick access to theme components
    static const Layout& getLayout();
    static const Colors& getColors();
    static const Animation& getAnimation();
    static const ErrorHandling& getErrorHandling();

    // Convenience methods
    static float getSceneLauncherWidth();
    static float getTrackWidth();
    static float getClipSlotHeight();
    static juce::Color getClipStateColor(ClipSlotState state);
    static juce::Color getGlowColor();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewTheme)
};

} // namespace zenith::ui