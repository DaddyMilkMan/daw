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
    struct GlowEffect {
        float intensity = 0.0f;      // 0-1, current intensity
        float targetIntensity = 0.0f; // 0-1, target intensity based on audio
        float size = 10.0f;          // Glow size in pixels
        juce::Colour color = juce::Colours::white;   // Glow color
        float frequency = 0.0f;      // Frequency band this glow responds to
    };

    // Pulse effects

} // namespace
