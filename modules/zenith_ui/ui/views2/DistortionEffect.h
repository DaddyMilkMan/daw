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
    struct DistortionEffect {
        float amount = 0.0f;        // 0-1, amount of distortion
        float frequency = 5000.0f;      // Frequency threshold for distortion
        juce::Colour color = juce::Colours::red;   // Distortion color
    };

    // Current effect values
    std::vector<GlowEffect> glowEffects;
    PulseEffect pulseEffect{};
    ColorModulation colorMod{};
    DistortionEffect distortionEffect{};
};

//==============================================================================
// Audio Reactive System Class
//==============================================================================

/**
 * @brief Real-time audio analysis and reactive visual system
 */

} // namespace
