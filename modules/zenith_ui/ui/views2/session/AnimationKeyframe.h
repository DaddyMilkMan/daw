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
struct AnimationKeyframe {
    AnimationState state;
    float timeMs = 0.0f;
    TransitionConfig config;

    bool operator<(const AnimationKeyframe& other) const {
        return timeMs < other.timeMs;
    }
};

/**
 * @clipStateTransition system with advanced animation capabilities
 */

} // namespace
