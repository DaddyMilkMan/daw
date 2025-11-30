/**
 * @file SkiaTheme.h
 * @brief Complete theme system for Skia-rendered UI components
 *
 * Provides colors, typography, and interaction styles for consistent UI rendering.
 */

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <include/core/SkColor.h>

namespace zenith {

/**
 * @brief Theme system for Skia-rendered components
 *
 * Singleton providing colors, typography, and interaction styles
 */
class SkiaTheme {
public:
    // Color scheme
    struct Colors {
        SkColor bg1 = SkColorSetARGB(255, 26, 26, 26);         // Darkest background
        SkColor bg2 = SkColorSetARGB(255, 36, 36, 36);         // Medium background
        SkColor bg3 = SkColorSetARGB(255, 48, 48, 48);         // Lighter background

        SkColor accentMain = SkColorSetARGB(255, 0, 212, 170); // Primary accent (cyan-teal)
        SkColor accentAlt = SkColorSetARGB(255, 138, 43, 226); // Secondary accent (purple)

        SkColor textStrong = SkColorSetARGB(255, 255, 255, 255); // Primary text
        SkColor textMuted = SkColorSetARGB(255, 180, 180, 180);  // Secondary text
        SkColor textDisabled = SkColorSetARGB(255, 100, 100, 100); // Disabled text

        SkColor borderSubtle = SkColorSetARGB(255, 60, 60, 60); // Subtle borders
        SkColor borderStrong = SkColorSetARGB(255, 90, 90, 90); // Strong borders

        // Waveform colors
        SkColor waveformMidi = SkColorSetARGB(255, 52, 199, 89);  // Green for MIDI
        SkColor waveformAudio = SkColorSetARGB(255, 74, 158, 255); // Blue for audio
    };

    // Typography settings
    struct Typography {
        struct FontSpec {
            float size;
            bool bold = false;
        };

        FontSpec h1 = {24.0f, true};
        FontSpec h2 = {20.0f, true};
        FontSpec h3 = {16.0f, true};
        FontSpec body = {14.0f, false};
        FontSpec small = {12.0f, false};
        FontSpec tiny = {10.0f, false};
    };

    // Interaction styles
    struct Interaction {
        float hoverOpacity = 0.8f;
        float pressedOpacity = 0.6f;
        float disabledOpacity = 0.3f;

        SkColor hoverTint = SkColorSetARGB(20, 255, 255, 255);
        SkColor pressTint = SkColorSetARGB(40, 0, 0, 0);
        SkColor hoverOverlay = SkColorSetARGB(30, 0, 212, 170); // Subtle hover overlay with accent color
    };

    // Selection styles
    struct SelectionStyle {
        SkColor tintColor = SkColorSetARGB(80, 0, 212, 170);
        SkColor borderColor = SkColorSetARGB(255, 0, 255, 200);
        float glowRadius = 4.0f;
    };

    // Singleton access
    static SkiaTheme& getInstance() {
        static SkiaTheme instance;
        return instance;
    }

    // Accessors
    const Colors& getColors() const { return colors_; }
    const Typography& getTypography() const { return typography_; }
    const Interaction& getInteraction() const { return interaction_; }
    const SelectionStyle& getSelectionStyle() const { return selection_; }

    // Legacy JUCE compatibility
    static juce::Colour getBackgroundColour() {
        return juce::Colour(0xff1a1a1a);
    }

private:
    SkiaTheme() = default;
    ~SkiaTheme() = default;

    // Delete copy/move
    SkiaTheme(const SkiaTheme&) = delete;
    SkiaTheme& operator=(const SkiaTheme&) = delete;
    SkiaTheme(SkiaTheme&&) = delete;
    SkiaTheme& operator=(SkiaTheme&&) = delete;

    Colors colors_;
    Typography typography_;
    Interaction interaction_;
    SelectionStyle selection_;
};

} // namespace zenith
