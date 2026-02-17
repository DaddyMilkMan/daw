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
struct AudioReactiveConfig {
    // Sensitivity settings
    float bassSensitivity = 1.0f;        // Multiplier for bass response
    float midSensitivity = 1.0f;         // Multiplier for mid response
    float trebleSensitivity = 1.0f;      // Multiplier for treble response
    float overallSensitivity = 1.0f;     // Multiplier for overall level
    float transientSensitivity = 1.0f;    // Multiplier for transient response

    // Smoothing settings
    float frequencySmoothing = 0.8f;    // Low-pass filter for frequencies
    float levelSmoothing = 0.9f;         // Low-pass filter for levels
    float transientSmoothing = 0.7f;     // Low-pass filter for transients

    // Visual mapping
    bool enableBassReactivity = true;     // Enable bass-driven effects
    bool enableMidReactivity = true;      // Enable mid-driven effects
    bool enableTrebleReactivity = true;   // Enable treble-driven effects
    bool enableBeatReactivity = true;     // Enable beat-synced animations
    bool enableTransientReactivity = true; // Enable transient-triggered effects

    // Visual scaling
    float glowIntensityScale = 1.0f;     // Scale glow intensity based on audio
    float animationSpeedScale = 1.0f;    // Scale animation speed based on tempo
    float colorShiftScale = 1.0f;        // Scale color shifts based on spectral content
};

//==============================================================================
// Audio Reactive Visual Effects
//==============================================================================

/**
 * @brief Audio-reactive visual effects for UI elements
 */

} // namespace
