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

} // namespace
