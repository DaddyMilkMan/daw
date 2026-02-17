/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../../framework/SkiaComponent.h"
#include "../../framework/GlassmorphicPanel.h"
#include "../../design-system/ZenithTheme.h"
#include <core/SkPicture.h>
#include <core/SkPictureRecorder.h>
#include <vector>
#include <memory>

namespace zenith::ui {

// Forward declarations
struct ArrangementDrag {
    enum class Mode {
        None,
        Scroll,
        Select,
        MoveClip,
        ResizeClipLeft,
        ResizeClipRight,
        FadeIn,
        FadeOut,
        MovePlayhead,
        AdjustLoop
    };

    Mode mode = Mode::None;
    juce::Point<float> startPoint;
    juce::Point<float> currentPoint;
    double startBeat = 0.0;
    ArrangementHitResult initialHit;
    bool isActive = false;
    juce::String draggedClipId;  // For MoveClip/Resize modes
    int draggedClipOriginalTrack = -1;
    double draggedClipOriginalStart = 0.0;
    double draggedClipOriginalLength = 0.0;  // For resize modes
    double draggedClipOriginalEnd = 0.0;     // For resize modes
};

/**
 * @brief Automation lane data
 */

} // namespace
