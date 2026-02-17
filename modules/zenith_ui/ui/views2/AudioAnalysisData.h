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
struct AudioAnalysisData {
    // Frequency analysis (32 bands)
    std::array<float, 32> frequencies{};  // Initialized to 0
    float bassLevel = 0.0f;        // 0-100, average of first 4 bands
    float midLevel = 0.0f;         // 0-100, average of bands 8-16
    float trebleLevel = 0.0f;      // 0-100, average of bands 24-32
    float overallLevel = 0.0f;     // 0-1, overall RMS level
    float peakLevel = 0.0f;        // 0-1, peak level over last 100ms

    // Dynamic analysis
    float spectralCentroid = 0.0f;  // "brightness" of sound (0-20000 Hz)
    float spectralSpread = 0.0f;    // Width of frequency distribution
    float zeroCrossingRate = 0.0f; // 0-1, indicates noise vs tonal content

    // Tempo and rhythm detection
    float tempo = 120.0f;          // BPM detected (default 120)
    float beatIntensity = 0.0f;    // 0-1, strength of current beat
    bool isOnBeat = false;         // True if on detected beat
    float phase = 0.0f;            // 0-1, phase within beat cycle

    // Transient detection
    bool hasTransient = false;     // True if transient detected
    float transientStrength = 0.0f; // 0-1, strength of transient
    juce::uint64 lastTransientTime = 0; // Time of last transient
};

//==============================================================================
// Audio Reactivity Configuration
//==============================================================================

/**
 * @brief Configuration for audio-reactive visual effects
 */

} // namespace
