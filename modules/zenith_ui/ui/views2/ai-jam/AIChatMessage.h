/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../../framework/SkiaComponent.h"
#include "../../framework/GlassmorphicPanel.h"
#include "../../design-system/ZenithTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>

// Forward declarations
namespace zenith {
struct AIChatMessage {
    bool fromUser = true;
    juce::String text;
    juce::String timestamp;
    bool hasAction = false;
    juce::String actionLabel;
};

/**
 * @brief Quick action button data
 */

} // namespace
