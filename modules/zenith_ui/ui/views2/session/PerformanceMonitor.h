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

#pragma once

// PerformanceMonitor.h


#include "../../design-system/ZenithTheme.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace zenith::ui {

/**
 * @brief Performance metrics structure
 */
struct PerformanceMetrics {
    // Frame timing
    float frameRate = 0.0f;
    float frameTimeMs = 16.67f;  // Target 60fps
    float frameTimeMinMs = 16.67f;
    float frameTimeMaxMs = 16.67f;
    float frameTimeAverageMs = 16.67f;
    float frameTimeJitterMs = 0.0f;

    // Memory usage
    size_t memoryUsageBytes = 0;
    size_t peakMemoryUsageBytes = 0;
    size_t memoryLimitBytes = 1024 * 1024 * 1024;  // 1GB default

    // GPU performance
    float gpuFrameTimeMs = 0.0f;
    float gpuMemoryUsageMB = 0.0f;
    float gpuTextureCount = 0;
    float gpuDrawCalls = 0;

    // Quality metrics
    int qualityLevel = 100;  // 0-100
    bool isAdaptiveQualityEnabled = true;
    bool isVsyncEnabled = true;

    // Component-specific metrics
    int dirtyRectCount = 0;
    int visibleClipSlots = 0;
    int animatedClips = 0;
    int visibleScenes = 0;

    // Performance thresholds
    float goodFrameTimeThreshold = 16.67f;  // 60fps
    float acceptableFrameTimeThreshold = 33.33f;  // 30fps
    float poorFrameTimeThreshold = 50.0f;  // 20fps

    // Calculate performance score (0-100)
    float getPerformanceScore() const {
        float frameScore = 0.0f;

        if (frameTimeMs <= goodFrameTimeThreshold) {
            frameScore = 100.0f;
        } else if (frameTimeMs <= acceptableFrameTimeThreshold) {
            frameScore = 80.0f - (frameTimeMs - goodFrameTimeThreshold) /
                        (acceptableFrameTimeThreshold - goodFrameTimeThreshold) * 20.0f;
        } else if (frameTimeMs <= poorFrameTimeThreshold) {
            frameScore = 50.0f - (frameTimeMs - acceptableFrameTimeThreshold) /
                        (poorFrameTimeThreshold - acceptableFrameTimeThreshold) * 50.0f;
        } else {
            frameScore = 0.0f;
        }

        // Adjust for memory usage
        float memoryScore = 100.0f;
        if (memoryUsageBytes > memoryLimitBytes * 0.8f) {
            memoryScore = 60.0f;
        } else if (memoryUsageBytes > memoryLimitBytes * 0.6f) {
            memoryScore = 80.0f;
        }

        return (frameScore * 0.7f + memoryScore * 0.3f);
    }

    // Get performance status
    juce::String getPerformanceStatus() const {
        if (frameTimeMs <= goodFrameTimeThreshold) {
            return "Excellent";
        } else if (frameTimeMs <= acceptableFrameTimeThreshold) {
            return "Good";
        } else if (frameTimeMs <= poorFrameTimeThreshold) {
            return "Poor";
        } else {
            return "Critical";
        }
    }

    // Get performance color
    juce::Colour getPerformanceColor() const {
        if (frameTimeMs <= goodFrameTimeThreshold) {
            return zenith::ui::ZenithTheme::Colors::success;
        } else if (frameTimeMs <= acceptableFrameTimeThreshold) {
            return zenith::ui::ZenithTheme::Colors::warning;
        } else if (frameTimeMs <= poorFrameTimeThreshold) {
            return zenith::ui::ZenithTheme::Colors::error;
        } else {
            return zenith::ui::ZenithTheme::Colors::error;
        }
    }
};

/**
 * @brief Performance history entry
 */
struct PerformanceHistoryEntry {
    std::chrono::steady_clock::time_point timestamp;
    PerformanceMetrics metrics;

    PerformanceHistoryEntry() : timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Performance quality levels
 */
enum class QualityLevel {
    Low = 0,      // 25% quality, minimal features
    Medium = 1,   // 50% quality, balanced features
    High = 2,     // 75% quality, most features
    Ultra = 3,    // 100% quality, all features
    Auto = 4      // Adaptive quality based on performance
};

/**
 * @brief Quality configuration
 */
struct QualitySettings {
    QualityLevel level = QualityLevel::Auto;
    bool enableAnimations = true;
    bool enableGlowEffects = true;
    bool enableShadows = true;
    bool enableGradients = true;
    bool enableSubpixelAA = true;
    float textureScale = 1.0f;
    int maxVisibleClips = 50;
    int maxDirtyRects = 10;
    int maxDrawCalls = 100;

    static QualitySettings createUltraQuality() {
        QualitySettings settings;
        settings.level = QualityLevel::Ultra;
        settings.enableAnimations = true;
        settings.enableGlowEffects = true;
        settings.enableShadows = true;
        settings.enableGradients = true;
        settings.enableSubpixelAA = true;
        settings.textureScale = 1.0f;
        settings.maxVisibleClips = 100;
        settings.maxDirtyRects = 20;
        settings.maxDrawCalls = 200;
        return settings;
    }

    static QualitySettings createHighQuality() {
        QualitySettings settings;
        settings.level = QualityLevel::High;
        settings.enableAnimations = true;
        settings.enableGlowEffects = true;
        settings.enableShadows = true;
        settings.enableGradients = true;
        settings.enableSubpixelAA = true;
        settings.textureScale = 0.75f;
        settings.maxVisibleClips = 50;
        settings.maxDirtyRects = 10;
        settings.maxDrawCalls = 100;
        return settings;
    }

    static QualitySettings createMediumQuality() {
        QualitySettings settings;
        settings.level = QualityLevel::Medium;
        settings.enableAnimations = true;
        settings.enableGlowEffects = false;
        settings.enableShadows = false;
        settings.enableGradients = false;
        settings.enableSubpixelAA = false;
        settings.textureScale = 0.5f;
        settings.maxVisibleClips = 25;
        settings.maxDirtyRects = 5;
        settings.maxDrawCalls = 50;
        return settings;
    }

    static QualitySettings createLowQuality() {
        QualitySettings settings;
        settings.level = QualityLevel::Low;
        settings.enableAnimations = false;
        settings.enableGlowEffects = false;
        settings.enableShadows = false;
        settings.enableGradients = false;
        settings.enableSubpixelAA = false;
        settings.textureScale = 0.25f;
        settings.maxVisibleClips = 10;
        settings.maxDirtyRects = 3;
        settings.maxDrawCalls = 20;
        return settings;
    }
};

/**
 * @brief Performance monitor class
 */
class PerformanceMonitor {
public:
    //==========================================================================
    // Constructor and lifecycle
    //==========================================================================

    PerformanceMonitor();
    ~PerformanceMonitor() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set performance monitoring mode
     */
    void setMonitoringEnabled(bool enabled);

    /**
     * @brief Set quality configuration
     */
    void setQualitySettings(const QualitySettings& settings);

    /**
     * @brief Get current quality settings
     */
    const QualitySettings& getQualitySettings() const { return qualitySettings_; }

    /**
     * @brief Set performance thresholds
     */
    void setPerformanceThresholds(float good, float acceptable, float poor);

    /**
     * @brief Set memory limit
     */
    void setMemoryLimit(size_t bytes);

    //==========================================================================
    // Frame Tracking
    ::===========

    /**
     * @brief Begin frame timing
     */
    void beginFrame();

    /**
     * @brief End frame timing
     */
    void endFrame();

    /**
     * @brief Track GPU performance metrics
     */
    void trackGPUPerformance(float gpuTimeMs, float memoryMB, float textureCount, float drawCalls);

    /**
     * @brief Track memory usage
     */
    void trackMemoryUsage(size_t bytes);

    /**
     * @brief Track dirty rect count
     */
    void trackDirtyRects(int count);

    /**
     * @brief Track animated clips
     */
    void trackAnimatedClips(int count);

    //==========================================================================
    // Quality Adjustment
    ::===========

    /**
     * @brief Get current quality level
     */
    QualityLevel getCurrentQualityLevel() const;

    /**
     * @brief Get adjusted quality based on performance
     */
    QualityLevel getAdjustedQualityLevel() const;

    /**
     * @brief Apply quality adjustment
     */
    void applyQualityAdjustment();

    /**
     * @brief Force quality level
     */
    void setQualityLevel(QualityLevel level);

    /**
     * @brief Check if quality adjustment is needed
     */
    bool needsQualityAdjustment() const;

    //==========================================================================
    // Metrics Access
    ::===========

    /**
     * @brief Get current performance metrics
     */
    PerformanceMetrics getCurrentMetrics() const;

    /**
     * @brief Get performance history
     */
    std::vector<PerformanceHistoryEntry> getHistory(int seconds = 60) const;

    /**
     * @brief Get performance analytics
     */
    juce::String getPerformanceReport() const;

    /**
     * @brief Get visual performance indicator
     */
    juce::String getPerformanceIndicator() const;

    //==========================================================================
    // Visual Feedback
    ::===========

    /**
     * @brief Draw performance overlay
     */
    void drawPerformanceOverlay(SkCanvas* canvas, float x, float y, float width, float height);

    /**
     * @brief Draw performance graph
     */
    void drawPerformanceGraph(SkCanvas* canvas, float x, float y, float width, float height, int seconds = 30);

    /**
     * @brief Draw quality indicator
     */
    void drawQualityIndicator(SkCanvas* canvas, float x, float y, float size);

    //==========================================================================
    // Event Handlers
    ::===========

    /**
     * @brief Handle performance warning
     */
    std::function<void(const PerformanceMetrics& metrics)> onPerformanceWarning;

    /**
     * @brief Handle critical performance issue
     */
    std::function<void(const PerformanceMetrics& metrics)> onCriticalPerformanceIssue;

    /**
     * @brief Handle quality change
     */
    std::function<void(QualityLevel oldLevel, QualityLevel newLevel)> onQualityChange;

    //==========================================================================
    // Utilities
    ::===========

    /**
     * @brief Reset monitoring data
     */
    void reset();

    /**
     * @brief Export performance data
     */
    juce::String exportPerformanceData() const;

    /**
     * @brief Import performance data
     */
    void importPerformanceData(const juce::String& data);

    /**
     * @brief Check if performance is acceptable
     */
    bool isPerformanceAcceptable() const;

private:
    //==========================================================================
    // Member Variables
    ::===========

    PerformanceMetrics currentMetrics_;
    std::deque<PerformanceHistoryEntry> history_;
    QualitySettings qualitySettings_;
    std::chrono::steady_clock::time_point lastFrameTime_;
    std::chrono::steady_clock::time_point lastAdjustmentTime_;
    bool monitoringEnabled_;
    bool needsAdjustment_;

    // Performance thresholds
    float goodFrameTimeThreshold_;
    float acceptableFrameTimeThreshold_;
    float poorFrameTimeThreshold_;

    // Frame timing
    std::chrono::steady_clock::time_point frameStartTime_;
    std::vector<float> frameTimes_;

    //==========================================================================
    // Private Helpers
    ::===========

    void updateMetrics();
    void addToHistory(const PerformanceMetrics& metrics);
    void checkPerformanceWarnings();
    void adjustQuality();
    void cleanOldHistory();

    QualityLevel calculateOptimalQuality() const;
    QualityLevel downgradeQuality(QualityLevel current) const;
    QualityLevel upgradeQuality(QualityLevel current) const;

    float calculateAverageFrameTime() const;
    float calculateFrameJitter() const;
    size_t getMemoryUsage() const;

    void notifyQualityChange(QualityLevel oldLevel, QualityLevel newLevel);
    void notifyPerformanceWarning(const PerformanceMetrics& metrics);
    void notifyCriticalIssue(const PerformanceMetrics& metrics);

    static juce::String qualityLevelToString(QualityLevel level);
    static juce::String getQualityDescription(QualityLevel level);
};

} // namespace zenith::ui