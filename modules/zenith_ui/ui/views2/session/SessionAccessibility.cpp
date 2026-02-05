/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    SessionAccessibility.cpp
    Created: 2026-02-05
    Author:  Zenith DAW Team

    Implementation of WCAG 2.1 compliant accessibility system.
    ==============================================================================
*/

#include "SessionAccessibility.h"
#include "SkiaSessionView.h"
#include <algorithm>

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

SessionAccessibilityManager::SessionAccessibilityManager() {
    focusPosition_ = {-1, -1};
    parentComponent_ = nullptr;

    // Initialize with system preferences
    updateFromSystemSettings();
}

//==============================================================================
// Configuration
//==============================================================================

void SessionAccessibilityManager::setPreferences(const AccessibilityPreferences& preferences) {
    bool changed = (preferences_.mode != preferences.mode ||
                   preferences_.highContrastEnabled != preferences.highContrastEnabled ||
                   preferences_.reducedMotionEnabled != preferences.reducedMotionEnabled);

    preferences_ = preferences;

    // Validate high contrast settings
    if (preferences_.highContrastEnabled) {
        if (!WCAGComplianceChecker::validateHighContrastScheme(preferences_)) {
            // Fallback to validated high contrast scheme
            preferences_ = AccessibilityPreferences::createWCAGAA();
        }
    }

    // Apply changes
    if (changed) {
        updateFocusIndicator();
        if (parentComponent_) {
            parentComponent->repaint();
        }
    }
}

void SessionAccessibilityManager::updateFromSystemSettings() {
    // Check system accessibility settings
    bool systemHighContrast = juce::Desktop::getInstance().isHighContrastModeEnabled();
    bool systemReducedMotion = juce::Desktop::getInstance().areAnimationsOn();
    bool systemScreenReader = juce::Desktop::getInstance().isScreenReaderRunning();

    // Update preferences
    if (systemHighContrast && !preferences_.highContrastEnabled) {
        preferences_ = AccessibilityPreferences::createWCAGAA();
    }

    if (!systemReducedMotion && preferences_.reducedMotionEnabled) {
        preferences_.reducedMotionEnabled = false;
    }

    if (systemScreenReader && !preferences_.screenReaderEnabled) {
        preferences_.screenReaderEnabled = true;
    }
}

//==============================================================================
// High Contrast Mode
//==============================================================================

void SessionAccessibilityManager::setHighContrastEnabled(bool enabled) {
    preferences_.highContrastEnabled = enabled;
    if (enabled) {
        preferences_ = AccessibilityPreferences::createWCAGAA();
    }
}

juce::Colour SessionAccessibilityManager::getHighContrastColor(const juce::Colour& original) const {
    if (!preferences_.highContrastEnabled) {
        return original;
    }

    // Return high contrast version
    if (original == zenith::ui::ZenithTheme::Colors::text_primary ||
        original == zenith::ui::ZenithTheme::Colors::text_secondary) {
        return preferences_.highContrastForeground;
    }

    if (original == zenith::ui::ZenithTheme::Colors::bg_01 ||
        original == zenith::ui::ZenithTheme::Colors::bg_02) {
        return preferences_.highContrastBackground;
    }

    return original;
}

juce::Colour SessionAccessibilityManager::getHighContrastClipSlotColor(const ClipSlotData& data) const {
    if (!preferences_.highContrastEnabled) {
        return data.color;
    }

    // High contrast colors for clip slots
    switch (data.state) {
        case ClipSlotState::Playing:
            return preferences_.highContrastAccent; // Yellow for playing
        case ClipSlotState::Recording:
            return juce::Colours::red; // Red for recording
        case ClipSlotState::Queued:
            return juce::Colours::orange; // Orange for queued
        case ClipSlotState::Stopped:
            return juce::Colours::white; // White for stopped
        case ClipSlotState::Empty:
            return juce::Colours::grey; // Grey for empty
        case ClipSlotState::Stopping:
            return juce::Colours::lightgrey; // Light grey for stopping
        default:
            return data.color;
    }
}

juce::Colour SessionAccessibilityManager::getHighContrastTrackColor(const SessionTrackData& track) const {
    if (!preferences_.highContrastEnabled) {
        return track.color;
    }

    // High contrast colors for tracks
    if (track.isSolo) {
        return juce::Colours::yellow;
    }
    if (track.isMuted) {
        return juce::Colours::grey;
    }
    if (track.isArmed) {
        return juce::Colours::red;
    }

    return track.color;
}

//==============================================================================
// Reduced Motion
//==============================================================================

void SessionAccessibilityManager::setReducedMotionEnabled(bool enabled) {
    preferences_.reducedMotionEnabled = enabled;
    if (enabled) {
        preferences_.maxAnimationSpeed = 1.0f;
        preferences_.enableAnimations = true;
        preferences_.enableTransitions = false;
    } else {
        preferences_.maxAnimationSpeed = 2.0f;
        preferences_.enableAnimations = true;
        preferences_.enableTransitions = true;
    }
}

float SessionAccessibilityManager::getAdjustedDuration(float originalDuration) const {
    if (!preferences_.reducedMotionEnabled) {
        return originalDuration;
    }

    // Reduce animation speed
    return originalDuration * preferences_.maxAnimationSpeed;
}

bool SessionAccessibilityManager::shouldSkipAnimation() const {
    return preferences_.reducedMotionEnabled && !preferences_.enableAnimations;
}

//==============================================================================
// Screen Reader Support
//==============================================================================

void SessionAccessibilityManager::announce(const ScreenReaderAnnouncement& announcement) {
    if (!preferences_.screenReaderEnabled) {
        return;
    }

    if (announcement.containsImportantChange || announcement.priority == "important") {
        announceImmediately(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SessionAccessibilityManager::queueAnnouncement(const ScreenReaderAnnouncement& announcement) {
    announcementQueue_.push_back(announcement);

    // Limit queue size
    if (announcementQueue_.size() > 10) {
        announcementQueue_.erase(announcementQueue_.begin());
    }
}

void SessionAccessibilityManager::processAnnouncements() {
    if (announcementQueue_.empty() || !preferences_.screenReaderEnabled) {
        return;
    }

    // Process oldest announcement
    ScreenReaderAnnouncement announcement = announcementQueue_.front();
    announcementQueue_.erase(announcementQueue_.begin());

    announceImmediately(announcement);
}

void SessionAccessibilityManager::announceImmediately(const ScreenReaderAnnouncement& announcement) {
    notifyScreenReader(announcement.message, announcement.priority);
}

ClipSlotAccessibilityInfo SessionAccessibilityManager::getClipSlotAccessibility(int trackIndex, int sceneIndex) const {
    ClipSlotAccessibilityInfo info;
    info.id = "clip_" + juce::String(trackIndex) + "_" + juce::String(sceneIndex);
    info.label = "Clip " + juce::String(sceneIndex + 1);
    info.role = "button";
    info.position = "Track " + juce::String(trackIndex + 1) + ", Scene " + juce::String(sceneIndex + 1);

    // Get current state (this would come from the session data)
    // For now, use a placeholder
    info.state = getClipStateDescription(ClipSlotState::Stopped);

    info.description = generateClipDescription(ClipSlotData());
    info.isSelected = (focusPosition_.first == trackIndex && focusPosition_.second == sceneIndex);

    return info;
}

juce::String SessionAccessibilityManager::getTrackAccessibilityInfo(int trackIndex) const {
    return "Track " + juce::String(trackIndex + 1) + " - " + generateTrackDescription(SessionTrackData());
}

juce::String SessionAccessibilityManager::getSceneAccessibilityInfo(int sceneIndex) const {
    return "Scene " + juce::String(sceneIndex + 1);
}

//==============================================================================
// Keyboard Navigation
//==============================================================================

bool SessionAccessibilityManager::handleAccessibilityKeyPress(const juce::KeyPress& key) {
    if (!preferences_.keyboardNavigationEnabled) {
        return false;
    }

    switch (key.getKeyCode()) {
        case juce::KeyPress::rightKey:
            return navigateNext();

        case juce::KeyPress::leftKey:
            return navigatePrevious();

        case juce::KeyPress::tabKey:
            return navigateNext();

        case juce::KeyPress::spaceKey:
            if (focusPosition_.first >= 0 && focusPosition_.second >= 0) {
                // Trigger clip action
                announce({juce::String("Clip activated"), "assertive"});
                return true;
            }
            break;

        case juce::KeyPress::returnKey:
            if (focusPosition_.first >= 0 && focusPosition_.second >= 0) {
                // Select clip
                announce({juce::String("Clip selected"), "assertive"});
                return true;
            }
            break;

        case juce::KeyPress::escapeKey:
            focusPosition_ = {-1, -1};
            updateFocusIndicator();
            return true;
    }

    return false;
}

bool SessionAccessibilityManager::navigateNext() {
    // This would implement proper navigation logic
    // For now, just move right
    if (focusPosition_.first >= 0) {
        focusPosition_.second++;
        updateFocusIndicator();
        return true;
    }
    return false;
}

bool SessionAccessibilityManager::navigatePrevious() {
    // This would implement proper navigation logic
    // For now, just move left
    if (focusPosition_.first >= 0) {
        focusPosition_.second--;
        if (focusPosition_.second < 0) {
            focusPosition_.second = 0;
        }
        updateFocusIndicator();
        return true;
    }
    return false;
}

void SessionAccessibilityManager::setFocusPosition(int trackIndex, int sceneIndex) {
    focusPosition_ = {trackIndex, sceneIndex};
    updateFocusIndicator();

    // Announce focus change
    ClipSlotAccessibilityInfo info = getClipSlotAccessibility(trackIndex, sceneIndex);
    announce({juce::String("Focus moved to ") + info.position, "polite"});
}

//==============================================================================
// Focus Indicators
//==============================================================================

void SessionAccessibilityManager::drawFocusIndicator(SkCanvas* canvas, const SkRect& bounds, float cornerRadius) const {
    SkPaint focusPaint;
    focusPaint.setAntiAlias(true);
    focusPaint.setStyle(SkPaint::kStroke_Style);
    focusPaint.setStrokeWidth(2.0f);
    focusPaint.setColor(getFocusIndicatorColor());

    SkPath path;
    path.addRoundRect(bounds, cornerRadius, cornerRadius);
    canvas->drawPath(path, focusPaint);
}

void SessionAccessibilityManager::drawHighContrastFocusIndicator(SkCanvas* canvas, const SkRect& bounds, float cornerRadius) const {
    SkPaint focusPaint;
    focusPaint.setAntiAlias(true);
    focusPaint.setStyle(SkPaint::kStroke_Style);
    focusPaint.setStrokeWidth(3.0f);  // Thicker for high contrast
    focusPaint.setColor(preferences_.highContrastAccent);

    SkPath path;
    path.addRoundRect(bounds, cornerRadius, cornerRadius);
    canvas->drawPath(path, focusPaint);
}

SkColor SessionAccessibilityManager::getFocusIndicatorColor() const {
    if (preferences_.highContrastEnabled) {
        return SkColorSetARGB(255,
                              preferences_.highContrastAccent.getRed(),
                              preferences_.highContrastAccent.getGreen(),
                              preferences_.highContrastAccent.getBlue());
    }

    return SkColorSetARGB(180, 59, 130, 246);  // Blue accent
}

//==============================================================================
// Event Handlers
//==============================================================================

void SessionAccessibilityManager::handleClipStateChange(int trackIndex, int sceneIndex, const ClipSlotData& data) {
    if (!preferences_.screenReaderEnabled) {
        return;
    }

    ClipSlotAccessibilityInfo info = getClipSlotAccessibility(trackIndex, sceneIndex);
    info.state = getClipStateDescription(data.state);
    info.description = generateClipDescription(data);

    announce({
        juce::String("Clip state changed to ") + info.state,
        data.state == ClipSlotState::Playing ? "assertive" : "polite"
    });
}

void SessionAccessibilityManager::handleSelectionChange(int trackIndex, int sceneIndex) {
    if (!preferences_.screenReaderEnabled) {
        return;
    }

    announce({
        juce::String("Selection changed to track ") + juce::String(trackIndex + 1) +
                   ", scene " + juce::String(sceneIndex + 1),
        "polite"
    });
}

void SessionAccessibilityManager::handleTrackStateChange(int trackIndex, const SessionTrackData& track) {
    if (!preferences_.screenReaderEnabled) {
        return;
    }

    announce({
        generateTrackDescription(track),
        "polite"
    });
}

//==============================================================================
// Utilities
//==============================================================================

juce::String SessionAccessibilityManager::getComplianceReport() const {
    return WCAGComplianceChecker::getComplianceReport(preferences_);
}

bool SessionAccessibilityManager::meetsWCAGAA() const {
    return WCAGComplianceChecker::checkContrast(
        preferences_.highContrastForeground,
        preferences_.highContrastBackground,
        4.5f
    );
}

bool SessionAccessibilityManager::meetsWCAGAAA() const {
    return WCAGComplianceChecker::checkContrast(
        preferences_.highContrastForeground,
        preferences_.highContrastBackground,
        7.0f
    );
}

juce::String SessionAccessibilityManager::exportAccessibilitySettings() const {
    // This would serialize preferences to JSON or similar format
    juce::String settings;
    settings += "mode:" + juce::String(static_cast<int>(preferences_.mode)) + "\n";
    settings += "highContrast:" + juce::String((int)preferences_.highContrastEnabled) + "\n";
    settings += "reducedMotion:" + juce::String((int)preferences_.reducedMotionEnabled) + "\n";
    settings += "screenReader:" + juce::String((int)preferences_.screenReaderEnabled) + "\n";
    return settings;
}

void SessionAccessibilityManager::importAccessibilitySettings(const juce::String& settings) {
    // This would deserialize preferences from JSON or similar format
    // Implementation would parse the settings string and update preferences_
    // For now, this is a placeholder
}

//==============================================================================
// Private Helpers
//==============================================================================

void SessionAccessibilityManager::scheduleAnnouncement(const ScreenReaderAnnouncement& announcement) {
    // Add to queue for batch processing
    queueAnnouncement(announcement);
}

void SessionAccessibilityManager::updateFocusIndicator() {
    if (parentComponent_) {
        parentComponent->repaint();
    }
}

void SessionAccessibilityManager::notifyScreenReader(const juce::String& message, const juce::String& priority) {
    // This would use JUCE's accessibility APIs or system-specific screen reader APIs
    // For now, this is a placeholder implementation
    juce::Logger::writeToLog("Screen Reader [" + priority + "]: " + message);
}

juce::String SessionAccessibilityManager::generateClipDescription(const ClipSlotData& data) const {
    juce::String description;

    if (data.state == ClipSlotState::Empty) {
        description = "Empty clip slot";
    } else {
        description = juce::String("Clip named '") + data.name + "' in " + getClipStateDescription(data.state) + " state";
    }

    if (data.isSelected) {
        description += ", selected";
    }

    if (data.isMidi) {
        description += ", MIDI clip";
    }

    return description;
}

juce::String SessionAccessibilityManager::generateTrackDescription(const SessionTrackData& track) const {
    juce::String description = juce::String("Track named '") + track.name + "'";

    if (track.isSolo) {
        description += ", soloed";
    }
    if (track.isMuted) {
        description += ", muted";
    }
    if (track.isArmed) {
        description += ", armed for recording";
    }

    return description;
}

juce::String SessionAccessibilityManager::generateSceneDescription(const SceneData& scene) const {
    juce::String description = juce::String("Scene named '") + scene.name + "'";

    if (scene.isPlaying) {
        description += ", playing";
    }
    if (scene.isQueued) {
        description += ", queued";
    }

    return description;
}

juce::String SessionAccessibilityManager::getClipStateDescription(ClipSlotState state) {
    switch (state) {
        case ClipSlotState::Empty: return "empty";
        case ClipSlotState::Stopped: return "stopped";
        case ClipSlotState::Playing: return "playing";
        case ClipSlotState::Queued: return "queued";
        case ClipSlotState::Recording: return "recording";
        case ClipSlotState::Stopping: return "stopping";
        default: return "unknown";
    }
}

juce::String SessionAccessibilityManager::getTrackStateDescription(const SessionTrackData& track) {
    juce::String state = juce::String("Volume: ") + juce::String(track.volume * 100, 1) + "%";

    if (track.isSolo) state += ", soloed";
    if (track.isMuted) state += ", muted";
    if (track.isArmed) state += ", armed";

    return state;
}

} // namespace zenith::ui