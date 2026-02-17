/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

// SessionAccessibility.h

#include "../../framework/SkiaAccessibility.h"
#include "../../design-system/ZenithTheme.h"
#include <functional>
#include <map>
#include <memory>

namespace zenith::ui {

/**
 * @brief Accessibility mode configuration
 */
enum class AccessibilityMode {
    Default,      // Standard UI with normal contrast
    HighContrast, // WCAG AAA compliant high contrast
    ReducedMotion, // Minimal or no animations
    ScreenReader, // Optimized for screen readers
    AllModes     // Enable all accessibility features
};

/**
 * @brief Accessibility preference settings
 */
struct AccessibilityPreferences {
    AccessibilityMode mode = AccessibilityMode::Default;
    bool highContrastEnabled = false;
    bool reducedMotionEnabled = false;
    bool screenReaderEnabled = false;
    bool audioFeedbackEnabled = false;
    bool focusIndicatorsEnabled = true;
    bool keyboardNavigationEnabled = true;

    // High contrast settings
    float contrastLevel = 4.5f;  // WCAG AA minimum
    juce::Colour highContrastBackground = juce::Colour::black;
    juce::Colour highContrastForeground = juce::Colour::white;
    juce::Colour highContrastAccent = juce::Colour::yellow;

    // Reduced motion settings
    float maxAnimationSpeed = 2.0f;  // Maximum 2x speed
    bool enableAnimations = true;
    bool enableTransitions = true;

    // Screen reader settings
    bool enableLiveRegions = true;
    bool announceStateChanges = true;
    bool describeVisualElements = true;

    static AccessibilityPreferences createWCAGAA() {
        AccessibilityPreferences prefs;
        prefs.mode = AccessibilityMode::HighContrast;
        prefs.highContrastEnabled = true;
        prefs.contrastLevel = 4.5f;
        prefs.highContrastBackground = juce::Colour::black;
        prefs.highContrastForeground = juce::Colour::white;
        prefs.highContrastAccent = juce::Colour::yellow;
        prefs.reducedMotionEnabled = false;
        return prefs;
    }

    static AccessibilityPreferences createWCAGAAA() {
        AccessibilityPreferences prefs;
        prefs.mode = AccessibilityMode::HighContrast;
        prefs.highContrastEnabled = true;
        prefs.contrastLevel = 7.0f;
        prefs.highContrastBackground = juce::Colour::black;
        prefs.highContrastForeground = juce::Colour::white;
        prefs.highContrastAccent = juce::Colour::yellow;
        prefs.reducedMotionEnabled = true;
        prefs.maxAnimationSpeed = 1.0f;
        return prefs;
    }

    static AccessibilityPreferences createReducedMotion() {
        AccessibilityPreferences prefs;
        prefs.mode = AccessibilityMode::ReducedMotion;
        prefs.reducedMotionEnabled = true;
        prefs.maxAnimationSpeed = 1.0f;
        prefs.enableAnimations = true;  // Still enable, but at minimum speed
        prefs.enableTransitions = false;
        return prefs;
    }
};

/**
 * @brief Accessibility state for clip slots
 */

} // namespace
