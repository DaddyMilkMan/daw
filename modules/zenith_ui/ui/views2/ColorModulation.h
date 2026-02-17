/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <functional>

namespace zenith::ui {

//==============================================================================
// Audio Analysis Data Structure
//==============================================================================

/**
 * @brief Real-time audio analysis data for reactive visuals
 */
    struct ColorModulation {
        float hueShift = 0.0f;      // Hue shift (0-360 degrees)
        float saturation = 1.0f;     // Saturation modulation (0-1)
        float brightness = 1.0f;    // Brightness modulation (0-1)
        bool respondToBass = false;   // True if bass affects hue
        bool respondToMids = false;   // True if mids affects saturation
        bool respondToTreble = false; // True if trebles affects brightness
    };

    // Distortion effects

} // namespace
