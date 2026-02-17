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
struct TimeSignatureMarker {
    double beatPosition = 0.0;
    int numerator = 4;
    int denominator = 4;
};

//==============================================================================
// Main View
//==============================================================================

/**
 * @class SkiaArrangementView
 * @brief Main timeline arrangement view
 */

} // namespace
