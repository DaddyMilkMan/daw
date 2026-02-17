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
struct ClipSlotAccessibilityInfo {
    juce::String id;
    juce::String label;
    juce::String role; // "button", "gridcell", "status"
    juce::String state; // "playing", "stopped", "empty", "recording"
    juce::String position; // "row 1, column 2"
    juce::String description;
    bool isSelected = false;
    bool isFocused = false;
    bool isRequired = false;
};

/**
 * @brief Screen reader announcement
 */

} // namespace
