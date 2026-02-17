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
    class ProjectState;
}

namespace zenith::ui {

//==============================================================================
// Types
//==============================================================================

/**
 * @brief Stem type for generated content
 */
enum class StemType {
    Drums,
    Bass,
    Chords,
    Melody,
    Vocals,
    FX,
    Other
};

/**
 * @brief Button type within a stem card
 */
enum class StemButtonType {
    Play,  ///< Play/pause button
    Solo,  ///< Solo button
    Mute   ///< Mute button
};

/**
 * @brief Generated stem data
 */

} // namespace
