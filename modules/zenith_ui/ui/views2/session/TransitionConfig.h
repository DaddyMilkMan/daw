/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

// ClipTransitionState.h

#include "../../framework/Animation.h"
#include "../../design-system/ZenithTheme.h"
#include <functional>
#include <map>
#include <memory>

namespace zenith::ui {

/**
 * @brief Represents a single state in the transition system
 */
struct TransitionConfig {
    zenith::animation::Easing easing = zenith::animation::Easing::EaseInOutCubic;
    float durationMs = 200.0f;  // Default: 200ms
    bool enableGlow = true;
    bool enablePulse = false;
    bool enableScale = true;
    bool enableRotation = false;
    float overshoot = 0.0f;  // For spring animations

    // Customizable properties
    SkColor glowTarget = SkColorSetARGB(255, 59, 130, 246);  // Blue glow
    float maxGlowIntensity = 0.8f;
    float pulseFrequency = 2.0f;  // Hz
    float maxPulseScale = 1.1f;
    float maxRotation = 15.0f;  // degrees

    static TransitionConfig createQuickTransition() {
        TransitionConfig config;
        config.durationMs = 100.0f;
        config.easing = zenith::animation::Easing::EaseOutExpo;
        config.enableGlow = false;
        config.enablePulse = false;
        return config;
    }

    static TransitionConfig createSmoothTransition() {
        TransitionConfig config;
        config.durationMs = 300.0f;
        config.easing = zenith::animation::Easing::EaseInOutCubic;
        config.enableGlow = true;
        config.enablePulse = true;
        return config;
    }

    static TransitionConfig createElasticTransition() {
        TransitionConfig config;
        config.durationMs = 400.0f;
        config.easing = zenith::animation::Easing::EaseOutElastic;
        config.enableScale = true;
        config.enableRotation = true;
        config.overshoot = 1.2f;
        return config;
    }
};

/**
 * @brief Keyframe for animation interpolation
 */

} // namespace
