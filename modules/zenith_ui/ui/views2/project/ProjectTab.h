/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "../../design-system/ZenithTheme.h"
#include "engine/Engine.h"
#include "engine/ProjectState.h"
#include "engine/RecentProjectManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace zenith {
namespace ui {

// Project information
    struct ProjectTab {
        ViewMode mode;
        juce::String label;
        SkRect bounds;
        bool isActive = false;
        bool isHovered = false;
    };

} // namespace
