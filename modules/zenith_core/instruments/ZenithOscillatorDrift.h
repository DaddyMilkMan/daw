/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace zenith {

/**
    Global analog drift simulation manager

    Simulates the warmup characteristics of analog synthesizers where
    oscillators drift in pitch and detune when cold, then stabilize
    as the instrument warms up.

    This creates subtle organic variation that makes digital synths
    sound more alive and analog-like.
*/
class AnalogDriftManager {
public:
    AnalogDriftManager() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Set overall drift amount (0.0 = disabled, 1.0 = full analog character) */
    void setDriftAmount(float amount) {
        driftAmount_.store(juce::jlimit(0.0f, 1.0f, amount), std::memory_order_relaxed);
    }

    float getDriftAmount() const {
        return driftAmount_.load(std::memory_order_relaxed);
    }

    /** Set warmup time in seconds (how long to reach stable temperature) */
    void setWarmupTime(float seconds) {
        warmupTime_.store(juce::jmax(0.1f, seconds), std::memory_order_relaxed);
    }

    float getWarmupTime() const {
        return warmupTime_.load(std::memory_order_relaxed);
    }

    /** Enable/disable per-voice temperature variation */
    void setPerVoiceDrift(bool enabled) {
        perVoiceDrift_.store(enabled, std::memory_order_relaxed);
    }

    bool getPerVoiceDrift() const {
        return perVoiceDrift_.load(std::memory_order_relaxed);
    }

    //==========================================================================
    // Temperature Control
    //==========================================================================

    /** Get current global temperature (0.0 = cold, 1.0 = warm/stable) */
    float getGlobalTemperature() const {
        return globalTemperature_.load(std::memory_order_relaxed);
    }

    /** Manually set temperature (useful for presets or immediate state changes) */
    void setGlobalTemperature(float temp) {
        globalTemperature_.store(juce::jlimit(0.0f, 1.0f, temp), std::memory_order_relaxed);
    }

    /** Reset warmup (simulate turning synth on - cold start) */
    void resetWarmup() {
        globalTemperature_.store(0.0f, std::memory_order_relaxed);
        accumulatedTime_.store(0.0, std::memory_order_relaxed);
    }

    /** Set to fully warmed up state */
    void setFullyWarmed() {
        globalTemperature_.store(1.0f, std::memory_order_relaxed);
        accumulatedTime_.store(warmupTime_.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /** Update drift simulation (call once per audio block) */
    void update(double sampleTime);

    /** Get random offset for per-voice variation */
    float getVoiceOffset(uint32 voiceId) const;

    /** Get random offset for oscillator variation */
    float getOscillatorOffset(int oscIndex) const;

    //==========================================================================
    // Presets
    //==========================================================================

    /** Classic vintage drift (maximum character) */
    void setVintageMode() {
        setDriftAmount(1.0f);
        setWarmupTime(15.0f);
        setPerVoiceDrift(true);
    }

    /** Modern stable mode (minimal drift) */
    void setModernMode() {
        setDriftAmount(0.15f);
        setWarmupTime(3.0f);
        setPerVoiceDrift(false);
    }

    /** Disabled (perfectly stable digital) */
    void setDisabled() {
        setDriftAmount(0.0f);
        setFullyWarmed();
    }

    /** Mild drift for subtle warmth */
    void setMildMode() {
        setDriftAmount(0.35f);
        setWarmupTime(8.0f);
        setPerVoiceDrift(true);
    }

private:
    // Configuration (atomic for thread-safe access from audio thread)
    std::atomic<float> driftAmount_{0.5f};
    std::atomic<float> warmupTime_{10.0f};
    std::atomic<bool> perVoiceDrift_{true};

    // State
    std::atomic<float> globalTemperature_{0.0f};
    std::atomic<double> accumulatedTime_{0.0};

    // Pseudo-random offsets (deterministic for consistency)
    static constexpr int numOscillatorOffsets = 16;
    float oscillatorOffsets_[numOscillatorOffsets] = {
        -0.08f,  0.12f, -0.05f,  0.07f,
        -0.11f,  0.03f, -0.09f,  0.14f,
         0.06f, -0.04f,  0.10f, -0.07f,
        -0.02f,  0.08f, -0.13f,  0.01f
    };

    juce::Random voiceRandom_;
};

//==============================================================================
/**
    Per-voice temperature tracker for individual drift variation
*/
class VoiceTemperatureTracker {
public:
    VoiceTemperatureTracker() {
        // Each voice has unique thermal characteristics
        thermalOffset_ = (random_.nextFloat() - 0.5f) * 0.3f; // +/- 15% variation
        warmupRate_ = 0.8f + random_.nextFloat() * 0.4f;     // 0.8x to 1.2x warmup speed
    }

    /** Update voice temperature based on global temperature */
    void update(float globalTemp, float warmupTime, double sampleTime);

    /** Get current voice temperature */
    float getTemperature() const { return voiceTemperature_; }

    /** Reset to cold */
    void reset() { voiceTemperature_ = 0.0f; }

private:
    float voiceTemperature_ = 0.0f;
    float thermalOffset_ = 0.0f;
    float warmupRate_ = 1.0f;
    juce::Random random_;
};

//==============================================================================
/**
    Drift configuration for individual oscillator fine-tuning
*/
struct OscillatorDriftPreset {
    const char* name;
    float amount;        // Overall drift amount
    float warmupTime;    // Seconds to warm up
    float freqDrift;     // Frequency drift in cents
    float detuneDrift;   // Detune drift in cents
    float phaseDrift;    // Phase jitter
    bool perVoice;       // Per-voice variation

    // Common presets
    static OscillatorDriftPreset vintage() {
        return {"Vintage", 1.0f, 15.0f, 8.0f, 4.0f, 0.002f, true};
    }

    static OscillatorDriftPreset modern() {
        return {"Modern", 0.15f, 3.0f, 2.0f, 1.0f, 0.0005f, false};
    }

    static OscillatorDriftPreset mild() {
        return {"Mild", 0.35f, 8.0f, 4.0f, 2.0f, 0.001f, true};
    }

    static OscillatorDriftPreset heavy() {
        return {"Heavy", 1.0f, 20.0f, 15.0f, 8.0f, 0.003f, true};
    }

    static OscillatorDriftPreset off() {
        return {"Off", 0.0f, 0.1f, 0.0f, 0.0f, 0.0f, false};
    }
};

} // namespace zenith
