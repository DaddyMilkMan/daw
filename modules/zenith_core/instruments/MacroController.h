/*
    MacroController.h - Macro Control System for Zenith

    Phase 3A: Core Modulation System
    Task #100: Implement Macro Control System

    Features:
    - 4 macro controls (matching Serum/Vital/Pigments)
    - 8 assignments per macro (32 total assignments)
    - Min/max range control
    - Bipolar modulation support
    - MIDI learn capability
    - Real-time safe (no memory allocation)

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#ifndef ZENITH_MACRO_CONTROLLER_H
#define ZENITH_MACRO_CONTROLLER_H

#include "ModulationMatrix.h"
#include <juce_core/juce_core.h>
#include <cstring>

namespace zenith {

//==============================================================================
// Macro Assignment
//==============================================================================

struct MacroAssignment {
    ModulationTarget target = ModulationTarget::NumTargets;
    float minValue = 0.0f;      // Value when macro is 0.0
    float maxValue = 1.0f;      // Value when macro is 1.0
    ModulationCurve curve = ModulationCurve::Linear;
    bool enabled = false;

    void reset() {
        target = ModulationTarget::NumTargets;
        minValue = 0.0f;
        maxValue = 1.0f;
        curve = ModulationCurve::Linear;
        enabled = false;
    }

    bool isActive() const {
        return enabled && target != ModulationTarget::NumTargets;
    }

    // Calculate output value based on macro position (0.0 to 1.0)
    float evaluate(float macroValue) const {
        if (!isActive()) return 0.0f;

        float normalized = juce::jlimit(0.0f, 1.0f, macroValue);
        float range = maxValue - minValue;
        float value = minValue + range * normalized;

        return applyCurve(value, curve);
    }

private:
    static float applyCurve(float value, ModulationCurve curve) {
        switch (curve) {
            case ModulationCurve::Linear:
                return value;
            case ModulationCurve::Exponential:
                return value * value;
            case ModulationCurve::Logarithmic:
                return std::sqrt(value);
            case ModulationCurve::Sine:
                return std::sin(value * juce::MathConstants<float>::halfPi);
            default:
                return value;
        }
    }
};

//==============================================================================
// Macro Control
//==============================================================================

struct MacroControl {
    static constexpr int MAX_ASSIGNMENTS = 8;
    static constexpr int NAME_LENGTH = 16;

    char name[NAME_LENGTH];           // Macro name
    float value;                      // Current value (0.0 to 1.0)
    float defaultValue;               // Default value
    uint32_t color;                   // Display color (ABGR format)
    int midiCC;                       // MIDI CC number (-1 = none)
    bool midiLearned;                 // Has MIDI been learned?

    MacroAssignment assignments[MAX_ASSIGNMENTS];
    int activeCount;

    MacroControl() {
        reset();
    }

    void reset() {
        std::strncpy(name, "Macro", NAME_LENGTH);
        value = 0.5f;
        defaultValue = 0.5f;
        color = 0xFFFFFFFF;  // White (ABGR)
        midiCC = -1;
        midiLearned = false;
        activeCount = 0;
        std::memset(assignments, 0, sizeof(assignments));
    }

    void setName(const char* newName) {
        std::strncpy(name, newName, NAME_LENGTH - 1);
        name[NAME_LENGTH - 1] = '\0';
    }

    //==========================================================================
    // Assignment Management
    //==========================================================================

    void addAssignment(const MacroAssignment& assignment) {
        if (activeCount >= MAX_ASSIGNMENTS) return;
        assignments[activeCount++] = assignment;
    }

    void removeAssignment(int index) {
        if (index < 0 || index >= activeCount) return;
        for (int i = index; i < activeCount - 1; ++i) {
            assignments[i] = assignments[i + 1];
        }
        activeCount--;
        assignments[activeCount].reset();
    }

    void clearAssignments() {
        std::memset(assignments, 0, sizeof(assignments));
        activeCount = 0;
    }

    //==========================================================================
    // Macro Processing
    //==========================================================================

    // Get modulation amount for a specific target
    float getModulation(ModulationTarget target) const {
        float total = 0.0f;

        for (int i = 0; i < activeCount; ++i) {
            const MacroAssignment& assignment = assignments[i];
            if (!assignment.isActive()) continue;
            if (assignment.target != target) continue;

            total += assignment.evaluate(value);
        }

        return total;
    }

    // Optimized: Apply all macro modulations to parameters
    // Returns array of modulation amounts for all targets
    void applyModulations(float modulations[], int numTargets) const {
        for (int i = 0; i < activeCount; ++i) {
            const MacroAssignment& assignment = assignments[i];
            if (!assignment.isActive()) continue;

            int targetIndex = static_cast<int>(assignment.target);
            if (targetIndex >= 0 && targetIndex < numTargets) {
                modulations[targetIndex] += assignment.evaluate(value);
            }
        }
    }
};

//==============================================================================
// Macro Controller
//==============================================================================

class MacroController {
public:
    static constexpr int NUM_MACROS = 4;  // 4 macros (matching Serum/Vital/Pigments)

    MacroController() {
        // Initialize with default names
        macros[0].setName("Macro 1");
        macros[1].setName("Macro 2");
        macros[2].setName("Macro 3");
        macros[3].setName("Macro 4");
    }

    //==========================================================================
    // Macro Access
    //==========================================================================

    MacroControl& getMacro(int index) {
        jassert(index >= 0 && index < NUM_MACROS);
        return macros[index];
    }

    const MacroControl& getMacro(int index) const {
        jassert(index >= 0 && index < NUM_MACROS);
        return macros[index];
    }

    //==========================================================================
    // MIDI Learn
    //==========================================================================

    void assignMIDICC(int macroIndex, int midiCC) {
        if (macroIndex < 0 || macroIndex >= NUM_MACROS) return;
        macros[macroIndex].midiCC = midiCC;
        macros[macroIndex].midiLearned = true;
    }

    void clearMIDILearn(int macroIndex) {
        if (macroIndex < 0 || macroIndex >= NUM_MACROS) return;
        macros[macroIndex].midiCC = -1;
        macros[macroIndex].midiLearned = false;
    }

    // Process MIDI CC message
    // Returns: true if a macro was updated
    bool processMIDI(int midiCC, float value) {
        for (int i = 0; i < NUM_MACROS; ++i) {
            if (macros[i].midiCC == midiCC) {
                macros[i].value = juce::jlimit(0.0f, 1.0f, value);
                return true;
            }
        }
        return false;
    }

    //==========================================================================
    // Macro Processing (Used in Voice Render Loop)
    //==========================================================================

    // Get total macro modulation for a target
    // Optimized for real-time performance
    float getModulation(ModulationTarget target) const {
        float total = 0.0f;

        for (int i = 0; i < NUM_MACROS; ++i) {
            total += macros[i].getModulation(target);
        }

        return total;
    }

    // Apply all macro modulations to parameter array
    // This is the fastest method for voice rendering
    void applyAllModulations(float modulations[], int numTargets) const {
        for (int i = 0; i < NUM_MACROS; ++i) {
            macros[i].applyModulations(modulations, numTargets);
        }
    }

    //==========================================================================
    // Preset Save/Load
    //==========================================================================

    struct MacroState {
        char names[NUM_MACROS][MacroControl::NAME_LENGTH];
        float defaultValues[NUM_MACROS];
        uint32_t colors[NUM_MACROS];
        int midiCCs[NUM_MACROS];
        MacroAssignment assignments[NUM_MACROS][MacroControl::MAX_ASSIGNMENTS];
        int activeCounts[NUM_MACROS];
    };

    void saveState(MacroState& state) const {
        for (int i = 0; i < NUM_MACROS; ++i) {
            std::memcpy(state.names[i], macros[i].name, MacroControl::NAME_LENGTH);
            state.defaultValues[i] = macros[i].defaultValue;
            state.colors[i] = macros[i].color;
            state.midiCCs[i] = macros[i].midiCC;
            state.activeCounts[i] = macros[i].activeCount;
            std::memcpy(state.assignments[i], macros[i].assignments,
                      sizeof(MacroAssignment) * MacroControl::MAX_ASSIGNMENTS);
        }
    }

    void loadState(const MacroState& state) {
        for (int i = 0; i < NUM_MACROS; ++i) {
            std::memcpy(macros[i].name, state.names[i], MacroControl::NAME_LENGTH);
            macros[i].name[MacroControl::NAME_LENGTH - 1] = '\0';
            macros[i].defaultValue = state.defaultValues[i];
            macros[i].value = state.defaultValues[i];
            macros[i].color = state.colors[i];
            macros[i].midiCC = state.midiCCs[i];
            macros[i].midiLearned = (state.midiCCs[i] >= 0);
            macros[i].activeCount = state.activeCounts[i];
            std::memcpy(macros[i].assignments, state.assignments[i],
                      sizeof(MacroAssignment) * MacroControl::MAX_ASSIGNMENTS);
        }
    }

private:
    MacroControl macros[NUM_MACROS];
};

//==============================================================================
// Helper Functions
//==============================================================================

inline const char* getMacroDefaultName(int index) {
    switch (index) {
        case 0: return "Macro 1";
        case 1: return "Macro 2";
        case 2: return "Macro 3";
        case 3: return "Macro 4";
        default: return "Macro";
    }
}

} // namespace zenith

#endif // ZENITH_MACRO_CONTROLLER_H
