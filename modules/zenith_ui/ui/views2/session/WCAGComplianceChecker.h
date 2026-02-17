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
class WCAGComplianceChecker {
public:
    /**
     * @brief Check color contrast against WCAG standards
     */
    static bool checkContrast(const juce::Colour& foreground,
                             const juce::Colour& background,
                             float minimumRatio = 4.5f,
                             bool isLargeText = false) {
        SkColor skForeground = SkColorSetARGB(
            foreground.getAlpha(),
            foreground.getRed(),
            foreground.getGreen(),
            foreground.getBlue()
        );

        SkColor skBackground = SkColorSetARGB(
            background.getAlpha(),
            background.getRed(),
            background.getGreen(),
            background.getBlue()
        );

        float contrastRatio = SkiaAccessibility::calculateContrastRatio(skForeground, skBackground);
        float required = isLargeText ? 3.0f : minimumRatio;

        return contrastRatio >= required;
    }

    /**
     * @brief Validate high contrast color scheme
     */
    static bool validateHighContrastScheme(const AccessibilityPreferences& prefs) {
        // Check text contrast
        bool textOk = checkContrast(
            prefs.highContrastForeground,
            prefs.highContrastBackground,
            prefs.contrastLevel
        );

        // Check accent contrast
        bool accentOk = checkContrast(
            prefs.highContrastAccent,
            prefs.highContrastBackground,
            3.0f  // UI components require 3:1 contrast
        );

        return textOk && accentOk;
    }

    /**
     * @brief Get WCAG compliance report
     */
    static juce::String getComplianceReport(const AccessibilityPreferences& prefs) {
        juce::StringArray report;

        if (prefs.highContrastEnabled) {
            bool valid = validateHighContrastScheme(prefs);
            report.add(valid ? "✓ High contrast scheme: WCAG compliant" :
                             "✗ High contrast scheme: WCAG non-compliant");
        }

        if (prefs.reducedMotionEnabled) {
            report.add("✓ Reduced motion: Enabled");
        }

        if (prefs.screenReaderEnabled) {
            report.add("✓ Screen reader support: Enabled");
        }

        if (prefs.focusIndicatorsEnabled) {
            report.add("✓ Focus indicators: Enabled");
        }

        report.add("Accessibility mode: " + juce::String(getModeDescription(prefs.mode)));

        return report.joinIntoString("\n");
    }

private:
    static juce::String getModeDescription(AccessibilityMode mode) {
        switch (mode) {
            case AccessibilityMode::Default: return "Default";
            case AccessibilityMode::HighContrast: return "High Contrast";
            case AccessibilityMode::ReducedMotion: return "Reduced Motion";
            case AccessibilityMode::ScreenReader: return "Screen Reader";
            case AccessibilityMode::AllModes: return "All Modes";
            default: return "Unknown";
        }
    }
};

/**
 * @brief Accessibility manager for SkiaSessionView
 */

} // namespace
