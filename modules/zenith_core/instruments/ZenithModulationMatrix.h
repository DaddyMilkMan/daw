/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>

namespace zenith {

//==============================================================================
// MODULATION SLOT (EXPANDED)
//==============================================================================
/**
 * Single modulation routing with scaling options
 */
struct ModulationSlotExpanded {
    ModulationSource source = ModulationSource::None;
    ModulationDestination destination = ModulationDestination::None;
    float amount = 0.0f;           // -1 to 1 (or scaled)
    float min = -1.0f;              // Minimum output value
    float max = 1.0f;               // Maximum output value
    bool bipolar = true;             // Is source bipolar?
    bool active = false;             // Slot is active (amount != 0)
    int curve = 0;                  // 0=linear, 1=concave, 2=convex

    ModulationSlotExpanded() = default;

    ModulationSlotExpanded(ModulationSource s, ModulationDestination d, float a)
        : source(s), destination(d), amount(a), active(a != 0.0f) {}
};

//==============================================================================
// MACRO LINKING
//==============================================================================

/**
 * Macro linking configuration
 */
struct MacroLink {
    int macroIndex = -1;         // Which macro (-1 = none)
    ModulationSource source = ModulationSource::None;
    float amount = 0.0f;          // Amount of influence
    bool invert = false;           // Invert signal
    float min = 0.0f;             // Minimum output
    float max = 1.0f;             // Maximum output

    MacroLink() = default;
    MacroLink(int idx, ModulationSource src, float amt = 1.0f)
        : macroIndex(idx), source(src), amount(amt) {}
};

//==============================================================================
// MODULATION STATE
//==============================================================================
/**
 * Current values of all modulation sources (computed each block)
 */
struct ModulationStateExpanded {
    // LFOs
    float lfo1 = 0.0f;
    float lfo2 = 0.0f;
    std::array<float, 4> stepLFO = {0.0f, 0.0f, 0.0f, 0.0f};

    // Envelopes
    float ampEnv = 0.0f;
    float modEnv = 0.0f;

    // MPE
    float velocity = 0.0f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    float pitchBend = 0.0f;
    float timbre = 0.0f;
    float noteNumber = 60.0f;

    // Combiners
    float macro1 = 0.0f;
    float macro2 = 0.0f;
    float macro3 = 0.0f;
    float macro4 = 0.0f;

    /** Get value for a source */
    float getSourceValue(ModulationSource source) const;
};

//==============================================================================
// MODULATION MATRIX
//==============================================================================
/**
 * Professional modulation matrix supporting 32 slots
 * Matches Serum/Vital capabilities
 */
class ZenithModulationMatrix {
public:
    static constexpr int MAX_SLOTS = 32;
    static constexpr int MAX_MACROS = 4;

    ZenithModulationMatrix() = default;

    //==========================================================================
    // Slot Management
    //==========================================================================

    /** Set a modulation slot */
    void setSlot(int index, ModulationSource source,
                 ModulationDestination dest, float amount,
                 float min = -1.0f, float max = 1.0f,
                 bool bipolar = true, int curve = 0);

    /** Clear a slot */
    void clearSlot(int index);

    /** Get slot */
    const ModulationSlotExpanded& getSlot(int index) const {
        return slots_[juce::jlimit(0, MAX_SLOTS - 1, index)];
    }

    //==========================================================================
    // Macro Controls
    //==========================================================================

    /** Set macro value (0-1) */
    void setMacroValue(int macroIndex, float value);

    /** Get macro value */
    float getMacroValue(int macroIndex) const {
        return state_.macro1;  // Simplified
    }

    //==========================================================================
    // Macro Linking
    //==========================================================================

    /**
     * @brief Link a macro to a modulation source
     * @param macroIndex Macro to link (0-3)
     * @param source Source to link from
     * @param amount Influence amount (0-1)
     */
    void linkMacroToSource(int macroIndex, ModulationSource source, float amount = 1.0f);

    /**
     * @brief Set macro link configuration
     */
    void setMacroLink(int macroIndex, const MacroLink& link);

    /**
     * @brief Get macro link configuration
     */
    const MacroLink& getMacroLink(int macroIndex) const;

    /**
     * @brief Clear macro link
     */
    void clearMacroLink(int macroIndex);

    //==========================================================================
    // Copy/Paste
    //==========================================================================

    /**
     * @brief Copy modulation slot to clipboard
     * @param index Slot index to copy
     */
    void copySlot(int index);

    /**
     * @brief Paste modulation slot from clipboard
     * @param index Slot index to paste to
     * @return True if clipboard had valid data
     */
    bool pasteSlot(int index);

    /**
     * @brief Check if clipboard has valid slot data
     */
    bool hasClipboardData() const { return clipboardValid_; }

    //==========================================================================
    // Processing
    //==========================================================================

    /** Update all modulation sources (call each block) */
    void updateSources(float midiNote, float velocity,
                    const juce::MPENote& mpeNote);

    /** Get total modulation for a destination */
    float getModulationFor(ModulationDestination dest) const;

    /** Get the modulation state */
    const ModulationStateExpanded& getState() const { return state_; }

    //==========================================================================
    // Presets
    //==========================================================================

    /** Clear all slots */
    void clearAll();

    /** Load from JSON */
    void fromJSON(const juce::var& json);

    /** Save to JSON */
    juce::var toJSON() const;

private:
    std::array<ModulationSlotExpanded, MAX_SLOTS> slots_;
    ModulationStateExpanded state_;

    //==========================================================================
    // Macro Linking State
    //==========================================================================

    std::array<MacroLink, MAX_MACROS> macroLinks_;

    //==========================================================================
    // Copy/Paste State
    //==========================================================================

    ModulationSlotExpanded clipboardSlot_;
    bool clipboardValid_ = false;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /** Apply curve to value */
    static float applyCurve(float value, int curve);

    /** Scale value from source range to destination range */
    static float scaleValue(float value, float amount, float min, float max);
};

//==============================================================================
// ENVELOPE FOLLOWER
//==============================================================================
/**
 * Audio-rate envelope follower for sidechain modulation
 */
class EnvelopeFollower {
public:
    EnvelopeFollower() = default;

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setAttack(float seconds);
    void setRelease(float seconds);

    /** Process sample and return envelope value */
    float processSample(float input);

    void reset() { envelope_ = 0.0f; }

private:
    double sampleRate_ = 44100.0;
    float envelope_ = 0.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
};

//==============================================================================
// STEP LFO
//==============================================================================
/**
 * 16-step sequencer LFO
 */
class StepLFO {
public:
    static constexpr int NUM_STEPS = 16;

    StepLFO();

    void setStep(int index, float value);  // 0-1
    float getStep(int index) const;

    void setRate(float hz);
    void setSmooth(float amount);  // 0-1

    /** Get current value */
    float getCurrentValue() const { return currentValue_; }

    /** Advance phase (call each sample) */
    void process(int numSamples);

    /** Reset to start */
    void reset() { phase_ = 0.0; }

private:
    std::array<float, NUM_STEPS> steps_;
    double phase_ = 0.0;
    float rate_ = 1.0f;
    float smooth_ = 0.5f;
    float currentValue_ = 0.0f;
    double sampleRate_ = 44100.0;

    /** Get interpolated step value */
    float getInterpolatedStep(double phase) const;
};

} // namespace zenith
