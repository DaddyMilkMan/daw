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
struct ProjectTemplate {
    juce::String id;
    juce::String name;
    juce::String category;
    juce::String description;
    juce::String thumbnailPath;
    juce::File templatePath;
    int estimatedDuration = 180;  // seconds
    bool isPopular = false;
};

/**
 * @class SkiaProjectManager
 * @brief Skia-based project management interface
 *
 * Features:
 * - Project metadata editing
 * - Track management (add, remove, reorder)
 * - Effect chain management
 * - Auto-save configuration
 * - Recent projects
 * - Template system
 * - Glassmorphic design
 */

} // namespace
