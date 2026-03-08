/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux
    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
    SPDX-License-Identifier: Apache-2.0 
*/
#pragma once
#include "AdvancedModulation.h"
#include <juce_audio_basics/juce_audio_basics.h>
namespace zenith {
//==============================================================================
// VELOCITY CURVE MANAGER
//==============================================================================
/**
 * @class VelocityCurve
 * @brief Professional velocity response curves for expressive playing
 *
 * Features:
 * - 20+ preset curves (Linear, Exponential, Log, Soft, Hard, etc.)
 * - Custom user-defined curves
 * - Per-voice velocity tracking
 * - Velocity sensitivity control
 * - Random velocity variation for humanization
 */
class VelocityCurve {
public:
    // Named velocity curves
    enum class Preset {
        Linear,         // 1:1 mapping
        Exponential,    // Velocity sensitive
        Logarithmic,    // Low velocity emphasis
        Soft,           // Compressed dynamic range
        Hard,           // Expanded dynamic range
        Velocity1,      // Vel 1 = very soft
        Velocity2,      // Vel 2 = soft
        Velocity3,      // Vel 3 = medium
        Velocity4,      // Vel 4 = hard
        Large,          // Large dynamic range
        Constant,       // No velocity sensitivity
        NoteOn,         // Fixed attack velocity
        Gate,           // Gated at threshold
        Inverted,       // High vel = low output
        Bump,           // Boosted mid velocities
        Dip,            // Dipped mid velocities
        SShaped,        // S-curve
        ReverseS,       // Reverse S-curve
        Humanized,      // With random variation
        Fixed           // Fixed output
    };
    VelocityCurve();
    ~VelocityCurve() = default;
    // Process velocity through curve
    float process(float velocityInput) const;
    void processBlock(const float* velocities, float* outputs, int numSamples) const;
    // Preset management
    void setPreset(Preset preset);
    void setPresetByName(const juce::String& name);
    Preset getPreset() const { return preset_; }
    static juce::String getPresetName(Preset preset);
    // Custom curve
    void setCustomCurve(const ResponseCurve& curve) { customCurve_ = curve; useCustom_ = true; }
    const ResponseCurve& getCustomCurve() const { return customCurve_; }
    // Parameters
    void setSensitivity(float sensitivity) { sensitivity_ = juce::jlimit(0.0f, 2.0f, sensitivity); }
    void setOffset(float offset) { offset_ = juce::jlimit(-1.0f, 1.0f, offset); }
    void setFixedValue(float value) { fixedValue_ = juce::jlimit(0.0f, 1.0f, value); }
    // Humanization
    void setHumanization(float amount) { humanization_ = juce::jlimit(0.0f, 1.0f, amount); }
    void setRandomSeed(int seed) { random_.setSeed(seed); }
    // Velocity tracking
    void setTrackMode(bool track) { trackMode_ = track; }
    float getTrackedVelocity() const { return trackedVelocity_; }
    // Curve visualization
    std::array<float, 128> getCurveTable() const;
private:
    Preset preset_ = Preset::Linear;
    ResponseCurve customCurve_;
    bool useCustom_ = false;
    float sensitivity_ = 1.0f;
    float offset_ = 0.0f;
    float fixedValue_ = 0.5f;
    float humanization_ = 0.0f;
    bool trackMode_ = false;
    mutable float trackedVelocity_ = 0.0f;
    juce::Random random_;
    float processPreset(float velocity, Preset preset) const;
    float applyHumanization(float value) const;
};
//==============================================================================
// AFTERTOUCH CURVE MANAGER
//==============================================================================
/**
 * @class AftertouchCurve
 * @brief Aftertouch response curves for MPE and channel pressure
 *
 * Features:
 * - Multiple preset curves optimized for different use cases
 * - Configurable depth and scaling
 * - Per-note MPE aftertouch tracking
 * - Smooth aftertouch transitions
 * - Aftertouch gating for noise floor
 */
class AftertouchCurve {
public:
    enum class Preset {
        Linear,         // Direct mapping
        Exponential,    // Increasing sensitivity
        Logarithmic,    // Low pressure emphasis
        Soft,           // Compressed early response
        Hard,           // Aggressive response
        Delayed,        // Threshold before response
        Quick,          // Immediate response
        Fixed,          // Fixed modulation amount
        Inverted,       // Reverse response
        Bell,           // Bell curve for mid-range focus
        Dip,            // Dip curve
        SShaped,        // Smooth S-curve
        VShaped,        // V-shape (low and high emphasis)
        Narrow,         // Narrow pressure range
        Wide,           // Wide pressure range
        Gated           // On/off gate
    };
    enum class Source {
        Channel,        // Channel pressure
        Poly,           // Polyphonic aftertouch (MPE)
        Both            // Sum of both
    };
    AftertouchCurve();
    ~AftertouchCurve() = default;
    // Process aftertouch through curve
    float process(float aftertouchInput) const;
    void processPoly(int noteNumber, float aftertouchInput);
    // Preset management
    void setPreset(Preset preset);
    void setPresetByName(const juce::String& name);
    Preset getPreset() const { return preset_; }
    static juce::String getPresetName(Preset preset);
