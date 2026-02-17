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
struct AnimationState {
    float opacity = 0.0f;
    float scale = 1.0f;
    float rotation = 0.0f;
    float brightness = 1.0f;
    float saturation = 1.0f;
    SkColor glowColor = 0;
    float glowIntensity = 0.0f;
    float pulsePhase = 0.0f;
    juce::Point<float> offset = {0.0f, 0.0f};

    bool operator==(const AnimationState& other) const {
        return opacity == other.opacity && scale == other.scale &&
               rotation == other.rotation && brightness == other.brightness &&
               saturation == other.saturation && glowColor == other.glowColor &&
               glowIntensity == other.glowIntensity && pulsePhase == other.pulsePhase &&
               offset == other.offset;
    }
};

/**
 * @brief Animation configuration for state transitions
 */

} // namespace
