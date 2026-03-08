/*
    ModulationMatrix.h - Flexible Modulation Routing for Zenith

    Phase 3A: Core Modulation System
    Task #99: Implement Modulation Matrix

    Features:
    - 16 modulation slots (vs 8 in Serum)
    - 13 modulation sources
    - 40+ modulation targets
    - Bipolar modulation (-1.0 to 1.0)
    - 4 curve types (linear, exp, log, sine)
    - Real-time safe (no memory allocation)

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#ifndef ZENITH_MODULATION_MATRIX_H
#define ZENITH_MODULATION_MATRIX_H

#include <juce_core/juce_core.h>
#include <cstring>

namespace zenith {

//==============================================================================
// Modulation Sources
//==============================================================================

enum class ModulationSource : int {
    None = -1,
    LFO1 = 0,
    LFO2,
    LFO3,
    Env1,
    Env2,
    Env3,
    Velocity,
    Aftertouch,
    ModWheel,
    Macro1,
    Macro2,
    Macro3,
    Macro4,
    PitchBend,
    KeyTrack,
    NumSources
};

//==============================================================================
// Modulation Targets
//==============================================================================

enum class ModulationTarget : int {
    // Oscillator 1
    Osc1Pitch = 0,
    Osc1Mix,
    Osc1PulseWidth,
    Osc1WavePosition,

    // Oscillator 2
    Osc2Pitch,
    Osc2Mix,
    Osc2PulseWidth,
    Osc2WavePosition,

    // Oscillator 3
    Osc3Pitch,
    Osc3Mix,
    Osc3PulseWidth,
    Osc3WavePosition,

    // Filter
    FilterCutoff,
    FilterResonance,
    FilterDrive,
    FilterEnvAmount,
    Filter2Cutoff,
    Filter2Resonance,

    // Amplitude
    MasterLevel,
    Pan,
    OscBalance,

    // LFO 1
    LFO1Rate,
    LFO1Amount,

    // LFO 2
    LFO2Rate,
    LFO2Amount,

    // LFO 3
    LFO3Rate,
    LFO3Amount,

    // Effects
    DistortionDrive,
    DistortionTone,
    ChorusRate,
    ChorusDepth,
    DelayTime,
    DelayFeedback,
    ReverbMix,
    ReverbDecay,

    // Modulation
    FMAmount,
    RingModAmount,
    GlideTime,

    // Global
    MasterTune,
    DriftAmount,

    NumTargets
};

//==============================================================================
// Modulation Curves
//==============================================================================

enum class ModulationCurve : int {
    Linear = 0,
    Exponential,
    Logarithmic,
    Sine,
    NumCurves
};

//==============================================================================
// Modulation Slot
//==============================================================================

struct ModulationSlot {
    ModulationSource source = ModulationSource::None;
    ModulationTarget target = ModulationTarget::NumTargets;
    float amount = 0.0f;           // -1.0 to 1.0 (bipolar)
    ModulationCurve curve = ModulationCurve::Linear;
    bool enabled = false;

    // Real-time safe (no dynamic allocation)
    void reset() {
        source = ModulationSource::None;
        target = ModulationTarget::NumTargets;
        amount = 0.0f;
        curve = ModulationCurve::Linear;
        enabled = false;
    }

    bool isActive() const {
        return enabled && source != ModulationSource::None &&
               target != ModulationTarget::NumTargets;
    }
};

//==============================================================================
// Modulation Matrix
//==============================================================================

class ModulationMatrix {
public:
    static constexpr int MAX_SLOTS = 16;  // 16 slots (vs 8 in Serum)

    ModulationMatrix() {
        std::memset(slots, 0, sizeof(slots));
        activeCount = 0;
    }

    //==========================================================================
    // Slot Management
    //==========================================================================

    void addSlot(const ModulationSlot& slot) {
        if (activeCount >= MAX_SLOTS) return;
        slots[activeCount++] = slot;
    }

    void removeSlot(int index) {
        if (index < 0 || index >= activeCount) return;
        for (int i = index; i < activeCount - 1; ++i) {
            slots[i] = slots[i + 1];
        }
        activeCount--;
        slots[activeCount].reset();
    }

    void clear() {
        std::memset(slots, 0, sizeof(slots));
        activeCount = 0;
    }

    //==========================================================================
    // Slot Access
    //==========================================================================

    int getActiveCount() const { return activeCount; }
    bool isEmpty() const { return activeCount == 0; }

    const ModulationSlot& getSlot(int index) const {
        return slots[index];
    }

    ModulationSlot& getSlot(int index) {
        return slots[index];
    }

    //==========================================================================
    // Real-time Modulation Processing
    //==========================================================================

    // Get total modulation value for a target
    // Returns: Sum of all active modulations to this target
    float getModulation(ModulationTarget target, const float sourceValues[]) const {
        float total = 0.0f;

        for (int i = 0; i < activeCount; ++i) {
            const ModulationSlot& slot = slots[i];
            if (!slot.isActive()) continue;
            if (slot.target != target) continue;

            int sourceIndex = static_cast<int>(slot.source);
            if (sourceIndex < 0 || sourceIndex >= static_cast<int>(ModulationSource::NumSources))
                continue;

            float sourceValue = sourceValues[sourceIndex];
            total += applyCurve(sourceValue * slot.amount, slot.curve);
        }

        return total;
    }

    // Optimized version for single target (used in voice render loop)
    float getModulationForTarget(ModulationTarget target,
                                 float lfo1, float lfo2, float lfo3,
                                 float env1, float env2, float env3,
                                 float vel, float aft, float mw,
                                 float mac1, float mac2, float mac3, float mac4,
                                 float pb, float kt) const {
        float total = 0.0f;

        for (int i = 0; i < activeCount; ++i) {
            const ModulationSlot& slot = slots[i];
            if (!slot.isActive() || slot.target != target) continue;

            float sourceValue = 0.0f;
            switch (slot.source) {
                case ModulationSource::LFO1: sourceValue = lfo1; break;
                case ModulationSource::LFO2: sourceValue = lfo2; break;
                case ModulationSource::LFO3: sourceValue = lfo3; break;
                case ModulationSource::Env1: sourceValue = env1; break;
                case ModulationSource::Env2: sourceValue = env2; break;
                case ModulationSource::Env3: sourceValue = env3; break;
                case ModulationSource::Velocity: sourceValue = vel; break;
                case ModulationSource::Aftertouch: sourceValue = aft; break;
                case ModulationSource::ModWheel: sourceValue = mw; break;
                case ModulationSource::Macro1: sourceValue = mac1; break;
                case ModulationSource::Macro2: sourceValue = mac2; break;
                case ModulationSource::Macro3: sourceValue = mac3; break;
                case ModulationSource::Macro4: sourceValue = mac4; break;
                case ModulationSource::PitchBend: sourceValue = pb; break;
                case ModulationSource::KeyTrack: sourceValue = kt; break;
                default: break;
            }

            total += applyCurve(sourceValue * slot.amount, slot.curve);
        }

        return total;
    }

private:
    ModulationSlot slots[MAX_SLOTS];
    int activeCount;

    //==========================================================================
    // Curve Shaping
    //==========================================================================

    static float applyCurve(float value, ModulationCurve curve) {
        switch (curve) {
            case ModulationCurve::Linear:
                return value;

            case ModulationCurve::Exponential:
                // Approximate exp curve (faster than pow())
                if (value >= 0.0f)
                    return value * value;
                return -(-value * -value);

            case ModulationCurve::Logarithmic:
                // Approximate log curve
                return std::copysign(std::sqrt(std::abs(value)), value);

            case ModulationCurve::Sine:
                // Sine curve
                return std::sin(value * juce::MathConstants<float>::halfPi);

            default:
                return value;
        }
    }
};

//==============================================================================
// Helper Functions
//==============================================================================

inline const char* getSourceName(ModulationSource source) {
    switch (source) {
        case ModulationSource::LFO1: return "LFO 1";
        case ModulationSource::LFO2: return "LFO 2";
        case ModulationSource::LFO3: return "LFO 3";
        case ModulationSource::Env1: return "Env 1";
        case ModulationSource::Env2: return "Env 2";
        case ModulationSource::Env3: return "Env 3";
        case ModulationSource::Velocity: return "Velocity";
        case ModulationSource::Aftertouch: return "Aftertouch";
        case ModulationSource::ModWheel: return "Mod Wheel";
        case ModulationSource::Macro1: return "Macro 1";
        case ModulationSource::Macro2: return "Macro 2";
        case ModulationSource::Macro3: return "Macro 3";
        case ModulationSource::Macro4: return "Macro 4";
        case ModulationSource::PitchBend: return "Pitch Bend";
        case ModulationSource::KeyTrack: return "Key Track";
        default: return "None";
    }
}

inline const char* getTargetName(ModulationTarget target) {
    switch (target) {
        case ModulationTarget::Osc1Pitch: return "Osc 1 Pitch";
        case ModulationTarget::Osc1Mix: return "Osc 1 Mix";
        case ModulationTarget::Osc1PulseWidth: return "Osc 1 Pulse Width";
        case ModulationTarget::Osc2Pitch: return "Osc 2 Pitch";
        case ModulationTarget::Osc2Mix: return "Osc 2 Mix";
        case ModulationTarget::Osc2PulseWidth: return "Osc 2 Pulse Width";
        case ModulationTarget::Osc3Pitch: return "Osc 3 Pitch";
        case ModulationTarget::Osc3Mix: return "Osc 3 Mix";
        case ModulationTarget::FilterCutoff: return "Filter Cutoff";
        case ModulationTarget::FilterResonance: return "Filter Resonance";
        case ModulationTarget::FilterDrive: return "Filter Drive";
        case ModulationTarget::FilterEnvAmount: return "Filter Env Amount";
        case ModulationTarget::MasterLevel: return "Master Level";
        case ModulationTarget::Pan: return "Pan";
        case ModulationTarget::LFO1Rate: return "LFO 1 Rate";
        case ModulationTarget::LFO1Amount: return "LFO 1 Amount";
        case ModulationTarget::LFO2Rate: return "LFO 2 Rate";
        case ModulationTarget::LFO2Amount: return "LFO 2 Amount";
        case ModulationTarget::FMAmount: return "FM Amount";
        case ModulationTarget::RingModAmount: return "Ring Mod Amount";
        default: return "Unknown";
    }
}

inline const char* getCurveName(ModulationCurve curve) {
    switch (curve) {
        case ModulationCurve::Linear: return "Linear";
        case ModulationCurve::Exponential: return "Exponential";
        case ModulationCurve::Logarithmic: return "Logarithmic";
        case ModulationCurve::Sine: return "Sine";
        default: return "Unknown";
    }
}

} // namespace zenith

#endif // ZENITH_MODULATION_MATRIX_H
