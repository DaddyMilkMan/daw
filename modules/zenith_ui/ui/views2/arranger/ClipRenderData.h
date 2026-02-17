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
    struct ClipRenderData {
        juce::String id;
        int trackIndex = 0;
        double startBeats = 0.0;
        double lengthBeats = 0.0;
        juce::String name;
        juce::Colour color;
        bool isMidi = false;
        bool isSelected = false;
        double fadeInBeats = 0.0;
        double fadeOutBeats = 0.0;
        juce::String audioFilePath;  // For loading real waveforms
        int clipIndex = -1;          // Index in the track
    };

    /**
     * @brief Waveform cache entry
     */

} // namespace
