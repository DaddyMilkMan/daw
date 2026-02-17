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
struct ArrangementSelection {
    std::vector<int> selectedTrackIndices;
    std::vector<std::pair<int, int>> selectedClips;  // (trackIdx, clipIdx)
    double selectionStartBeat = 0.0;
    double selectionEndBeat = 0.0;
    bool hasTimeSelection = false;
};

/**
 * @brief Hit test result for mouse interaction
 */

} // namespace
