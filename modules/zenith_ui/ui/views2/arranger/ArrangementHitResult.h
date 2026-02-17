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
struct ArrangementHitResult {
    enum class Type {
        None,
        TrackHeader,
        TrackLane,
        Clip,
        ClipEdgeLeft,
        ClipEdgeRight,
        ClipFadeIn,
        ClipFadeOut,
        Timeline,
        Playhead,
        LoopRegion
    };

    Type type = Type::None;
    int trackIndex = -1;
    int clipIndex = -1;
    double beatPosition = 0.0;
};

/**
 * @brief Drag operation state
 */

} // namespace
