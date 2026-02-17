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
struct ScreenReaderAnnouncement {
    juce::String message;
    juce::String priority; // "polite", "assertive", "important"
    juce::String liveRegion; // "polite", "assertive"
    bool isAtomic = true;
    bool containsImportantChange = false;
};

/**
 * @brief WCAG 2.1 compliance checker
 */

} // namespace
