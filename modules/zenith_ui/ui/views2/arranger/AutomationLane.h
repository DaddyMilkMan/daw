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
struct AutomationLane {
    juce::String parameterName = "Volume";
    juce::Colour color = juce::Colours::cyan;
    bool isVisible = true;
    bool isExpanded = false;
    float height = 60.0f;

    // Automation points (beat, value 0-1)
    std::vector<std::pair<double, float>> points;
};

/**
 * @brief Time signature marker
 */

} // namespace
