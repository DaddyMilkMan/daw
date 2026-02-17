/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
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

} // namespace
