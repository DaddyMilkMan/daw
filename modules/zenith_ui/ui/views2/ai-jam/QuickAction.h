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
struct QuickAction {
    juce::String emoji;
    juce::String label;
    juce::String prompt;  // What to send to AI
};

//==============================================================================
// Main View
//==============================================================================

/**
 * @class SkiaAIJamView
 * @brief AI-powered jamming overlay
 */

} // namespace
