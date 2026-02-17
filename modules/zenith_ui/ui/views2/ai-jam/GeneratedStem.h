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
struct GeneratedStem {
    StemType type = StemType::Other;
    juce::String name;
    bool isMuted = false;
    bool isSoloed = false;
    bool isPlaying = true;
    float volume = 1.0f;
    float meterLevel = 0.0f;
    std::vector<float> waveformPreview;  // 64 samples for mini display
    juce::File audioFile;  // Associated audio file (if any)
    juce::String clipId;   // Associated clip ID in project (if any)
};

/**
 * @brief Chat message in AI conversation
 */

} // namespace
