/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <functional>
#include <memory>

namespace zenith {

//==============================================================================
// Macro Control Configuration
//==============================================================================

/**
    The shape of the modulation curve applied by a macro control
*/
enum class MacroCurve {
    Linear,           // Straight line mapping
    Logarithmic,      // Logarithmic (good for frequency)
    Exponential,      // Exponential (good for filter resonance)
    Parabolic,        // Quadratic curve
    InverseParabolic, // Inverse quadratic
    Sine,             // Sine wave easing
    EaseInQuad,       // Slow start, fast end
    EaseOutQuad,      // Fast start, slow end
    EaseInOutQuad,    // Slow start and end
    EaseInCubic,      // Slower start
    EaseOutCubic,     // Slower end
    EaseInOutCubic,   // Smooth S-curve
    Stepped,          // Quantized steps
    Random            // Random variation within range
};

/**
    How the macro value is applied
*/
enum class MacroMode {
    Absolute,         // Direct value mapping
    Bipolar,          // -1 to +1 range (centered)
    Unipolar,         // 0 to 1 range
    Offset,           // Added to current value
    Scale,            // Multiplied with current value
    Quantized,        // Snapped to discrete steps
    Toggle            // Binary on/off
};

/**
    Macro control configuration for a single parameter
*/
struct MacroMapping {
    juce::String parameterId;      // ID of parameter to control
    juce::String parameterName;    // Display name

    float minimum = 0.0f;          // Minimum value when macro is at 0
    float maximum = 1.0f;          // Maximum value when macro is at 1
    float center = 0.5f;           // Center value for bipolar

    MacroCurve curve = MacroCurve::Linear;
    MacroMode mode = MacroMode::Unipolar;

    bool isInverted = false;       // Invert the mapping
    float amount = 1.0f;           // Depth of modulation (0-1)

    // For quantized mode
    int numSteps = 8;              // Number of discrete steps

    // For offset mode
    float offsetAmount = 0.0f;     // Offset amount

    // For toggle mode
    float toggleThreshold = 0.5f;  // Threshold for on/off

    // Modifiers
    float skew = 0.5f;             // Skew the response curve
    bool retrigger = false;        // Retrigger envelopes when macro changes

    JUCE_LEAK_DETECTOR(MacroMapping)
};

//==============================================================================
// Macro Control Slot
//==============================================================================

/**
    A single macro control with up to 16 parameter assignments
*/
class MacroControl {
public:
    static constexpr int maxAssignments = 16;

    MacroControl(int index);
    ~MacroControl() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Get the macro index (0-7) */
    int getIndex() const { return index_; }

    /** Set the macro name (for display) */
    void setName(const juce::String& name) { name_ = name; }
    juce::String getName() const { return name_; }

    /** Set the macro color (for UI) */
    void setColor(juce::uint32 color) { color_ = color; }
    juce::uint32 getColor() const { return color_; }

    //==========================================================================
    // Value Control
    //==========================================================================

    /** Set the normalized macro value (0.0 - 1.0) */
    void setValue(float newValue, bool notifyListeners = true);

    /** Get current normalized value */
    float getValue() const { return value_.load(std::memory_order_relaxed); }

    /** Get value as atomic (for audio thread) */
    std::atomic<float>& getAtomicValue() { return value_; }

    /** Set default value */
    void setDefaultValue(float def) {
        defaultValue_ = juce::jlimit(0.0f, 1.0f, def);
    }
    float getDefaultValue() const { return defaultValue_; }

    /** Reset to default value */
    void resetToDefault() { setValue(defaultValue_); }

    //==========================================================================
    // Parameter Assignments
    //==========================================================================

    /** Add a parameter mapping */
    bool addMapping(const MacroMapping& mapping);

    /** Remove a parameter mapping by ID */
    bool removeMapping(const juce::String& parameterId);

    /** Remove all mappings */
    void clearMappings();

    /** Get number of mappings */
    int getNumMappings() const { return numMappings_; }

    /** Get mapping at index */
    const MacroMapping* getMapping(int index) const;

    /** Get mapping by parameter ID */
    const MacroMapping* getMappingForParameter(const juce::String& parameterId) const;

    /** Update existing mapping */
    bool updateMapping(const juce::String& parameterId, const MacroMapping& updated);

    //==========================================================================
    // Value Processing
    //==========================================================================

    /** Apply macro to a parameter value */
    float applyToParameter(const juce::String& parameterId, float currentValue) const;

    /** Get the modulated value for a mapped parameter */
    float getModulatedValue(const juce::String& parameterId) const;

    //==========================================================================
    // Presets
    //==========================================================================

    /** Save current mappings as preset */
    juce::ValueTree saveToValueTree() const;

    /** Load mappings from preset */
    void loadFromValueTree(const juce::ValueTree& tree);

    //==========================================================================
    // Callbacks
    //==========================================================================

    using ValueChangedCallback = std::function<void(int macroIndex, float newValue)>;
    void setValueChangedCallback(ValueChangedCallback callback) {
        valueChangedCallback_ = std::move(callback);
    }

private:
    int index_;
    juce::String name_;
    juce::uint32 color_;
    std::atomic<float> value_;
    float defaultValue_;

    std::array<MacroMapping, maxAssignments> mappings_;
    std::atomic<int> numMappings_{0};

    ValueChangedCallback valueChangedCallback_;

    // Curve processing helpers
    float applyCurve(float input, MacroCurve curve, float skew) const;
    float remap(float value, const MacroMapping& mapping) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroControl)
};

//==============================================================================
// Macro Control Manager
//==============================================================================

/**
    Manages all macro controls (typically 8 macros for a synth)
*/
class MacroControlManager {
public:
    static constexpr int numMacros = 8;

    MacroControlManager();
    ~MacroControlManager() = default;

    //==========================================================================
    // Access
    //==========================================================================

    /** Get macro control by index */
    MacroControl* getMacro(int index) {
        return juce::isPositiveAndBelow(index, numMacros) ? &macros_[static_cast<size_t>(index)] : nullptr;
    }

    const MacroControl* getMacro(int index) const {
        return juce::isPositiveAndBelow(index, numMacros) ? &macros_[static_cast<size_t>(index)] : nullptr;
    }

    //==========================================================================
    // MIDI Learn
    //==========================================================================

    /** Map MIDI CC to macro */
    void mapMidiCC(int macroIndex, int ccNumber, int channel = 0);

    /** Map MIDI note to macro */
    void mapMidiNote(int macroIndex, int noteNumber, int channel = 0);

    /** Clear MIDI mapping for macro */
    void clearMidiMapping(int macroIndex);

    /** Get macro index for MIDI CC (-1 if none) */
    int getMacroForMidiCC(int ccNumber, int channel = 0) const;

    /** Get macro index for MIDI note (-1 if none) */
    int getMacroForMidiNote(int noteNumber, int channel = 0) const;

    //==========================================================================
    // Automation
    //==========================================================================

    /** Process MIDI input and update macros */
    void processMidi(const juce::MidiBuffer& midi);

    /** Set macro from MIDI value (0-127) */
    void setMacroFromMidi(int macroIndex, uint8 midiValue);

    //==========================================================================
    // Smoothing
    //==========================================================================

    /** Enable smoothing for macro changes */
    void setSmoothingTime(float seconds) { smoothingTime_ = seconds; }
    float getSmoothingTime() const { return smoothingTime_; }

    /** Process smoothing (call each block) */
    void processSmoothing(double sampleRate, int numSamples);

    //==========================================================================
    // Preset Management
    //==========================================================================

    /** Save all macros to value tree */
    juce::ValueTree saveToValueTree() const;

    /** Load all macros from value tree */
    void loadFromValueTree(const juce::ValueTree& tree);

    //==========================================================================
    // Factory Presets
    //==========================================================================

    /** Apply factory preset to all macros */
    void applyPreset(const juce::String& presetName);

    static juce::StringArray getAvailablePresets();

    //==========================================================================
    // Quick Assign
    //==========================================================================

    /** Quick assign common parameters to a macro */
    void assignCommonParameters(int macroIndex, CommonParamType type);

    enum class CommonParamType {
        Filter,           // Cutoff + Resonance
        Envelope,         // Attack + Decay + Sustain + Release
        Oscillator,       // Detune + Mix + Shape
        Effects,          // Send levels to effects
        LFO,              // Rate + Amount
        Velocity,         // Velocity curve + Aftertouch
        vibrato,          // Vibrato depth + rate
        All               // All commonly modulated params
    };

private:
    std::array<MacroControl, numMacros> macros_;

    // MIDI mapping
    struct MidiMapping {
        int ccNumber = -1;
        int noteNumber = -1;
        int channel = 0;
        bool isValid = false;
    };
    std::array<MidiMapping, numMacros> midiMappings_;

    // Smoothing state
    float smoothingTime_ = 0.01f; // 10ms default
    std::array<float, numMacros> targetValues_{};
    std::array<float, numMacros> currentValues_{};
    std::array<float, numMacros> smoothingCoefficients_{};

    // Colors for default macros
    static juce::uint32 getDefaultColor(int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroControlManager)
};

//==============================================================================
// Macro Modulation Source (for use in modulation matrix)
//==============================================================================

/**
    Provides macro values as modulation sources for the modulation matrix
*/
class MacroModulationSource {
public:
    MacroModulationSource(MacroControlManager& manager) : manager_(manager) {}

    /** Get current modulation value for a macro */
    float getModulationValue(int macroIndex) const {
        if (const auto* macro = manager_.getMacro(macroIndex)) {
            return macro->getValue();
        }
        return 0.0f;
    }

    /** Get bipolar value (-1 to +1) */
    float getBipolarValue(int macroIndex) const {
        return getModulationValue(macroIndex) * 2.0f - 1.0f;
    }

private:
    MacroControlManager& manager_;
};

//==============================================================================
// Inline Functions
//==============================================================================

inline float MacroControl::applyCurve(float input, MacroCurve curve, float skew) const {
    // Clamp input
    input = juce::jlimit(0.0f, 1.0f, input);

    switch (curve) {
        case MacroCurve::Linear:
            return input;

        case MacroCurve::Logarithmic:
            return std::pow(input, 0.3f + skew * 0.4f);

        case MacroCurve::Exponential:
            return std::pow(input, 1.5f + skew * 2.0f);

        case MacroCurve::Parabolic:
            return input * input;

        case MacroCurve::InverseParabolic:
            return 1.0f - std::pow(1.0f - input, 2);

        case MacroCurve::Sine:
            return std::sin(input * juce::MathConstants<float>::halfPi);

        case MacroCurve::EaseInQuad:
            return input * input;

        case MacroCurve::EaseOutQuad:
            return input * (2.0f - input);

        case MacroCurve::EaseInOutQuad:
            return input < 0.5f
                ? 2.0f * input * input
                : 1.0f - 2.0f * (1.0f - input) * (1.0f - input);

        case MacroCurve::EaseInCubic:
            return input * input * input;

        case MacroCurve::EaseOutCubic:
            return 1.0f - std::pow(1.0f - input, 3);

        case MacroCurve::EaseInOutCubic:
            return input < 0.5f
                ? 4.0f * input * input * input
                : 1.0f - 4.0f * std::pow(1.0f - input, 3);

        case MacroCurve::Stepped: {
            int steps = juce::roundToInt(skew * 16) + 2;
            return std::floor(input * steps) / steps;
        }

        case MacroCurve::Random:
            // Random is handled externally via state
            return input;

        default:
            return input;
    }
}

} // namespace zenith
