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
class SessionAccessibilityManager {
public:
    //==========================================================================
    // Constructor and lifecycle
    //==========================================================================

    SessionAccessibilityManager();
    ~SessionAccessibilityManager() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set accessibility preferences
     */
    void setPreferences(const AccessibilityPreferences& preferences);

    /**
     * @brief Get current preferences
     */
    const AccessibilityPreferences& getPreferences() const { return preferences_; }

    /**
     * @brief Update preferences from system settings
     */
    void updateFromSystemSettings();

    //==========================================================================
    // High Contrast Mode
    //==========================================================================

    /**
     * @brief Enable/disable high contrast mode
     */
    void setHighContrastEnabled(bool enabled);

    /**
     * @brief Get high contrast color scheme
     */
    juce::Colour getHighContrastColor(const juce::Colour& original) const;

    /**
     * @brief Get high contrast colors for specific elements
     */
    juce::Colour getHighContrastClipSlotColor(const ClipSlotData& data) const;
    juce::Colour getHighContrastTrackColor(const SessionTrackData& track) const;

    //==========================================================================
    // Reduced Motion
    //==========================================================================

    /**
     * @brief Enable/disable reduced motion
     */
    void setReducedMotionEnabled(bool enabled);

    /**
     * @brief Get adjusted animation duration
     */
    float getAdjustedDuration(float originalDuration) const;

    /**
     * @brief Check if animation should be skipped
     */
    bool shouldSkipAnimation() const;

    /**
     * @brief Get minimum animation speed
     */
    float getMinimumAnimationSpeed() const { return preferences_.maxAnimationSpeed; }

    //==========================================================================
    // Screen Reader Support
    ::===========

    /**
     * @brief Announce message to screen reader
     */
    void announce(const ScreenReaderAnnouncement& announcement);

    /**
     * @brief Queue announcement for batch processing
     */
    void queueAnnouncement(const ScreenReaderAnnouncement& announcement);

    /**
     * @brief Process queued announcements
     */
    void processAnnouncements();

    /**
     * @brief Get clip slot accessibility info
     */
    ClipSlotAccessibilityInfo getClipSlotAccessibility(int trackIndex, int sceneIndex) const;

    /**
     * @brief Get track accessibility info
     */
    juce::String getTrackAccessibilityInfo(int trackIndex) const;

    /**
     * @brief Get scene accessibility info
     */
    juce::String getSceneAccessibilityInfo(int sceneIndex) const;

    //==========================================================================
    // Keyboard Navigation
    //===========

    /**
     * @brief Handle keyboard input for accessibility
     */
    bool handleAccessibilityKeyPress(const juce::KeyPress& key);

    /**
     * @brief Navigate to next focusable element
     */
    bool navigateNext();

    /**
     * @brief Navigate to previous focusable element
     */
    bool navigatePrevious();

    /**
     * @brief Get current focus position
     */
    juce::Point<int> getCurrentFocusPosition() const { return focusPosition_; }

    /**
     * @brief Set focus position
     */
    void setFocusPosition(int trackIndex, int sceneIndex);

    //==========================================================================
    // Focus Indicators
    //==========================================================================

    /**
     * @brief Draw accessibility-focused focus indicator
     */
    void drawFocusIndicator(SkCanvas* canvas, const SkRect& bounds, float cornerRadius) const;

    /**
     * @brief Draw high contrast focus indicator
     */
    void drawHighContrastFocusIndicator(SkCanvas* canvas, const SkRect& bounds, float cornerRadius) const;

    /**
     * @brief Get focus indicator color
     */
    SkColor getFocusIndicatorColor() const;

    //==========================================================================
    // Event Handlers
    //==========================================================================

    /**
     * @brief Handle clip state change for accessibility
     */
    void handleClipStateChange(int trackIndex, int sceneIndex, const ClipSlotData& data);

    /**
     * @brief Handle selection change for accessibility
     */
    void handleSelectionChange(int trackIndex, int sceneIndex);

    /**
     * @brief Handle track state change for accessibility
     */
    void handleTrackStateChange(int trackIndex, const SessionTrackData& track);

    //==========================================================================
    // Utilities
    //==========================================================================

    /**
     * @brief Get WCAG compliance report
     */
    juce::String getComplianceReport() const;

    /**
     * @brief Check if current mode meets WCAG AA
     */
    bool meetsWCAGAA() const;

    /**
     * @brief Check if current mode meets WCAG AAA
     */
    bool meetsWCAGAAA() const;

    /**
     * @brief Export accessibility settings
     */
    juce::String exportAccessibilitySettings() const;

    /**
     * @brief Import accessibility settings
     */
    void importAccessibilitySettings(const juce::String& settings);

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    AccessibilityPreferences preferences_;
    juce::Point<int> focusPosition_;
    std::vector<ScreenReaderAnnouncement> announcementQueue_;
    juce::Component* parentComponent_;

    //==========================================================================
    // Private Helpers
    //==========================================================================

    void announceImmediately(const ScreenReaderAnnouncement& announcement);
    void scheduleAnnouncement(const ScreenReaderAnnouncement& announcement);
    void updateFocusIndicator();
    void notifyScreenReader(const juce::String& message, const juce::String& priority = "polite");

    juce::String generateClipDescription(const ClipSlotData& data) const;
    juce::String generateTrackDescription(const SessionTrackData& track) const;
    juce::String generateSceneDescription(const SceneData& scene) const;

    static juce::String getClipStateDescription(ClipSlotState state);
    static juce::String getTrackStateDescription(const SessionTrackData& track);
};

} // namespace zenith::ui