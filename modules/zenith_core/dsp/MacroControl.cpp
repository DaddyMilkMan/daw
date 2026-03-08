/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "MacroControl.h"
#include <cmath>

namespace zenith {

//==============================================================================
// MacroControl Implementation
//==============================================================================

MacroControl::MacroControl(int index)
    : index_(index)
    , name_("Macro " + juce::String(index + 1))
    , color_(getDefaultColor(index))
    , value_(0.5f)
    , defaultValue_(0.5f)
{
    clearMappings();
}

void MacroControl::setValue(float newValue, bool notifyListeners) {
    float clamped = juce::jlimit(0.0f, 1.0f, newValue);
    float oldValue = value_.load(std::memory_order_relaxed);
    value_.store(clamped, std::memory_order_relaxed);

    if (notifyListeners && std::abs(clamped - oldValue) > 0.0001f) {
        if (valueChangedCallback_) {
            valueChangedCallback_(index_, clamped);
        }
    }
}

bool MacroControl::addMapping(const MacroMapping& mapping) {
    int currentCount = numMappings_.load(std::memory_order_relaxed);
    if (currentCount >= maxAssignments) {
        return false;
    }

    // Check if parameter already mapped
    for (int i = 0; i < currentCount; ++i) {
        if (mappings_[static_cast<size_t>(i)].parameterId == mapping.parameterId) {
            // Update existing mapping
            mappings_[static_cast<size_t>(i)] = mapping;
            return true;
        }
    }

    // Add new mapping
    mappings_[static_cast<size_t>(currentCount)] = mapping;
    numMappings_.store(currentCount + 1, std::memory_order_relaxed);
    return true;
}

bool MacroControl::removeMapping(const juce::String& parameterId) {
    int currentCount = numMappings_.load(std::memory_order_relaxed);
    int foundIndex = -1;

    for (int i = 0; i < currentCount; ++i) {
        if (mappings_[static_cast<size_t>(i)].parameterId == parameterId) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex < 0) {
        return false;
    }

    // Shift remaining mappings
    for (int i = foundIndex; i < currentCount - 1; ++i) {
        mappings_[static_cast<size_t>(i)] = mappings_[static_cast<size_t>(i + 1)];
    }

    numMappings_.store(currentCount - 1, std::memory_order_relaxed);
    return true;
}

void MacroControl::clearMappings() {
    numMappings_.store(0, std::memory_order_relaxed);
    for (auto& mapping : mappings_) {
        mapping = MacroMapping{};
    }
}

const MacroMapping* MacroControl::getMapping(int index) const {
    if (index >= 0 && index < numMappings_.load(std::memory_order_relaxed)) {
        return &mappings_[static_cast<size_t>(index)];
    }
    return nullptr;
}

const MacroMapping* MacroControl::getMappingForParameter(const juce::String& parameterId) const {
    int count = numMappings_.load(std::memory_order_relaxed);
    for (int i = 0; i < count; ++i) {
        if (mappings_[static_cast<size_t>(i)].parameterId == parameterId) {
            return &mappings_[static_cast<size_t>(i)];
        }
    }
    return nullptr;
}

bool MacroControl::updateMapping(const juce::String& parameterId, const MacroMapping& updated) {
    int count = numMappings_.load(std::memory_order_relaxed);
    for (int i = 0; i < count; ++i) {
        if (mappings_[static_cast<size_t>(i)].parameterId == parameterId) {
            mappings_[static_cast<size_t>(i)] = updated;
            return true;
        }
    }
    return false;
}

float MacroControl::applyToParameter(const juce::String& parameterId, float currentValue) const {
    if (const auto* mapping = getMappingForParameter(parameterId)) {
        return remap(getValue(), *mapping);
    }
    return currentValue;
}

float MacroControl::getModulatedValue(const juce::String& parameterId) const {
    float macroValue = getValue();

    if (const auto* mapping = getMappingForParameter(parameterId)) {
        return remap(macroValue, *mapping);
    }

    return macroValue;
}

float MacroControl::remap(float value, const MacroMapping& mapping) const {
    // Apply curve
    float curvedValue = applyCurve(value, mapping.curve, mapping.skew);

    // Invert if needed
    if (mapping.isInverted) {
        curvedValue = 1.0f - curvedValue;
    }

    // Apply mode
    float result = 0.0f;

    switch (mapping.mode) {
        case MacroMode::Absolute:
            result = mapping.minimum + curvedValue * (mapping.maximum - mapping.minimum);
            break;

        case MacroMode::Bipolar: {
            float range = (mapping.maximum - mapping.center);
            result = mapping.center + (curvedValue - 0.5f) * 2.0f * range;
            break;
        }

        case MacroMode::Unipolar:
            result = mapping.minimum + curvedValue * (mapping.maximum - mapping.minimum);
            break;

        case MacroMode::Offset:
            result = curvedValue * mapping.amount;
            break;

        case MacroMode::Scale:
            result = curvedValue * mapping.amount;
            break;

        case MacroMode::Quantized: {
            float range = mapping.maximum - mapping.minimum;
            float stepped = std::floor(curvedValue * mapping.numSteps) / mapping.numSteps;
            result = mapping.minimum + stepped * range;
            break;
        }

        case MacroMode::Toggle:
            result = (curvedValue >= mapping.toggleThreshold) ? mapping.maximum : mapping.minimum;
            break;
    }

    // Apply amount scaling
    if (mapping.mode == MacroMode::Offset || mapping.mode == MacroMode::Scale) {
        // Amount is the full range for these modes
    } else {
        // For other modes, amount scales the overall effect
        // result = mapping.minimum + (result - mapping.minimum) * mapping.amount;
    }

    return juce::jlimit(mapping.minimum, mapping.maximum, result);
}

juce::ValueTree MacroControl::saveToValueTree() const {
    juce::ValueTree tree("MACRO");
    tree.setProperty("index", index_, nullptr);
    tree.setProperty("name", name_, nullptr);
    tree.setProperty("color", static_cast<int>(color_), nullptr);
    tree.setProperty("value", getValue(), nullptr);
    tree.setProperty("default", defaultValue_, nullptr);

    juce::ValueTree mappingsTree("MAPPINGS");
    int count = numMappings_.load(std::memory_order_relaxed);
    for (int i = 0; i < count; ++i) {
        const auto& mapping = mappings_[static_cast<size_t>(i)];
        juce::ValueTree mappingTree("MAPPING");
        mappingTree.setProperty("paramId", mapping.parameterId, nullptr);
        mappingTree.setProperty("paramName", mapping.parameterName, nullptr);
        mappingTree.setProperty("min", mapping.minimum, nullptr);
        mappingTree.setProperty("max", mapping.maximum, nullptr);
        mappingTree.setProperty("center", mapping.center, nullptr);
        mappingTree.setProperty("curve", static_cast<int>(mapping.curve), nullptr);
        mappingTree.setProperty("mode", static_cast<int>(mapping.mode), nullptr);
        mappingTree.setProperty("inverted", mapping.isInverted, nullptr);
        mappingTree.setProperty("amount", mapping.amount, nullptr);
        mappingTree.setProperty("steps", mapping.numSteps, nullptr);
        mappingTree.setProperty("skew", mapping.skew, nullptr);
        mappingsTree.appendChild(mappingTree, nullptr);
    }
    tree.appendChild(mappingsTree, nullptr);

    return tree;
}

void MacroControl::loadFromValueTree(const juce::ValueTree& tree) {
    if (!tree.hasType("MACRO")) return;

    name_ = tree.getProperty("name", "Macro " + juce::String(index_ + 1)).toString();
    color_ = static_cast<juce::uint32>(tree.getProperty("color", getDefaultColor(index_)));
    defaultValue_ = tree.getProperty("default", 0.5f);
    setValue(tree.getProperty("value", defaultValue_), false);

    clearMappings();
    juce::ValueTree mappingsTree = tree.getChildWithName("MAPPINGS");
    if (mappingsTree.isValid()) {
        for (const auto& mappingTree : mappingsTree) {
            if (mappingTree.hasType("MAPPING")) {
                MacroMapping mapping;
                mapping.parameterId = mappingTree.getProperty("paramId").toString();
                mapping.parameterName = mappingTree.getProperty("paramName").toString();
                mapping.minimum = mappingTree.getProperty("min", 0.0f);
                mapping.maximum = mappingTree.getProperty("max", 1.0f);
                mapping.center = mappingTree.getProperty("center", 0.5f);
                mapping.curve = static_cast<MacroCurve>(static_cast<int>(mappingTree.getProperty("curve", 0)));
                mapping.mode = static_cast<MacroMode>(static_cast<int>(mappingTree.getProperty("mode", 0)));
                mapping.isInverted = mappingTree.getProperty("inverted", false);
                mapping.amount = mappingTree.getProperty("amount", 1.0f);
                mapping.numSteps = mappingTree.getProperty("steps", 8);
                mapping.skew = mappingTree.getProperty("skew", 0.5f);
                addMapping(mapping);
            }
        }
    }
}

//==============================================================================
// MacroControlManager Implementation
//==============================================================================

MacroControlManager::MacroControlManager() {
    // Initialize macro controls
    for (size_t i = 0; i < numMacros; ++i) {
        macros_[i] = MacroControl(static_cast<int>(i));
        targetValues_[i] = 0.5f;
        currentValues_[i] = 0.5f;
        smoothingCoefficients_[i] = 1.0f;
        midiMappings_[i] = MidiMapping{};
    }
}

void MacroControlManager::mapMidiCC(int macroIndex, int ccNumber, int channel) {
    if (auto* macro = getMacro(macroIndex)) {
        midiMappings_[static_cast<size_t>(macroIndex)] = {ccNumber, -1, channel, true};
    }
}

void MacroControlManager::mapMidiNote(int macroIndex, int noteNumber, int channel) {
    if (auto* macro = getMacro(macroIndex)) {
        midiMappings_[static_cast<size_t>(macroIndex)] = {-1, noteNumber, channel, true};
    }
}

void MacroControlManager::clearMidiMapping(int macroIndex) {
    if (macroIndex >= 0 && macroIndex < numMacros) {
        midiMappings_[static_cast<size_t>(macroIndex)] = MidiMapping{};
    }
}

int MacroControlManager::getMacroForMidiCC(int ccNumber, int channel) const {
    for (size_t i = 0; i < numMacros; ++i) {
        const auto& mapping = midiMappings_[i];
        if (mapping.isValid && mapping.ccNumber == ccNumber &&
            (mapping.channel == channel || mapping.channel == 0)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int MacroControlManager::getMacroForMidiNote(int noteNumber, int channel) const {
    for (size_t i = 0; i < numMacros; ++i) {
        const auto& mapping = midiMappings_[i];
        if (mapping.isValid && mapping.noteNumber == noteNumber &&
            (mapping.channel == channel || mapping.channel == 0)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void MacroControlManager::processMidi(const juce::MidiBuffer& midi) {
    for (const auto& metadata : midi) {
        auto message = metadata.getMessage();

        if (message.isController()) {
            int ccNumber = message.getControllerNumber();
            int channel = message.getChannel();
            int macroIndex = getMacroForMidiCC(ccNumber, channel);

            if (macroIndex >= 0) {
                setMacroFromMidi(macroIndex, message.getControllerValue());
            }
        } else if (message.isNoteOn()) {
            int noteNumber = message.getNoteNumber();
            int channel = message.getChannel();
            int macroIndex = getMacroForMidiNote(noteNumber, channel);

            if (macroIndex >= 0) {
                setMacroFromMidi(macroIndex, message.getVelocity());
            }
        }
    }
}

void MacroControlManager::setMacroFromMidi(int macroIndex, uint8 midiValue) {
    if (auto* macro = getMacro(macroIndex)) {
        float normalized = static_cast<float>(midiValue) / 127.0f;
        targetValues_[static_cast<size_t>(macroIndex)] = normalized;

        // If smoothing is disabled, set directly
        if (smoothingTime_ < 0.001f) {
            macro->setValue(normalized);
        }
    }
}

void MacroControlManager::processSmoothing(double sampleRate, int numSamples) {
    if (smoothingTime_ < 0.001f) return;

    double sampleTime = numSamples / sampleRate;

    for (size_t i = 0; i < numMacros; ++i) {
        float& current = currentValues_[i];
        float target = targetValues_[i];

        if (std::abs(current - target) > 0.0001f) {
            // Calculate smoothing coefficient
            float coef = static_cast<float>(sampleTime / smoothingTime_);
            coef = juce::jlimit(0.0f, 1.0f, coef);

            // Interpolate toward target
            current += (target - current) * coef;

            // Update macro value
            if (auto* macro = getMacro(static_cast<int>(i))) {
                macro->setValue(current);
            }
        } else {
            current = target;
        }
    }
}

juce::ValueTree MacroControlManager::saveToValueTree() const {
    juce::ValueTree tree("MACROS");

    for (size_t i = 0; i < numMacros; ++i) {
        tree.appendChild(macros_[i].saveToValueTree(), nullptr);
    }

    // Save MIDI mappings
    juce::ValueTree midiTree("MIDI_MAPPINGS");
    for (size_t i = 0; i < numMacros; ++i) {
        const auto& mapping = midiMappings_[i];
        if (mapping.isValid) {
            juce::ValueTree mappingTree("MAPPING");
            mappingTree.setProperty("macroIndex", static_cast<int>(i), nullptr);
            mappingTree.setProperty("ccNumber", mapping.ccNumber, nullptr);
            mappingTree.setProperty("noteNumber", mapping.noteNumber, nullptr);
            mappingTree.setProperty("channel", mapping.channel, nullptr);
            midiTree.appendChild(mappingTree, nullptr);
        }
    }
    tree.appendChild(midiTree, nullptr);

    tree.setProperty("smoothingTime", smoothingTime_, nullptr);

    return tree;
}

void MacroControlManager::loadFromValueTree(const juce::ValueTree& tree) {
    if (!tree.hasType("MACROS")) return;

    for (size_t i = 0; i < numMacros; ++i) {
        auto macroTree = tree.getChild(static_cast<int>(i));
        if (macroTree.hasType("MACRO")) {
            macros_[i].loadFromValueTree(macroTree);
        }
    }

    // Load MIDI mappings
    auto midiTree = tree.getChildWithName("MIDI_MAPPINGS");
    if (midiTree.isValid()) {
        for (const auto& mappingTree : midiTree) {
            if (mappingTree.hasType("MAPPING")) {
                int macroIndex = mappingTree.getProperty("macroIndex", -1);
                int ccNumber = mappingTree.getProperty("ccNumber", -1);
                int noteNumber = mappingTree.getProperty("noteNumber", -1);
                int channel = mappingTree.getProperty("channel", 0);

                if (macroIndex >= 0 && macroIndex < numMacros) {
                    if (ccNumber >= 0) {
                        mapMidiCC(macroIndex, ccNumber, channel);
                    }
                    if (noteNumber >= 0) {
                        mapMidiNote(macroIndex, noteNumber, channel);
                    }
                }
            }
        }
    }

    smoothingTime_ = tree.getProperty("smoothingTime", 0.01f);
}

juce::StringArray MacroControlManager::getAvailablePresets() {
    return {
        "Init",
        "Classic",
        "Modern",
        "Performance",
        "Creative",
        "Filter Sweep",
        "Motion"
    };
}

void MacroControlManager::applyPreset(const juce::String& presetName) {
    if (presetName == "Init") {
        // Clear all mappings
        for (size_t i = 0; i < numMacros; ++i) {
            macros_[i].clearMappings();
            macros_[i].setValue(0.5f);
        }
    } else if (presetName == "Classic") {
        // Classic macro layout
        if (auto* m0 = getMacro(0)) {
            m0->setName("Filter");
            m0->setColor(0xFF4CAF50);
        }
        if (auto* m1 = getMacro(1)) {
            m1->setName("Resonance");
            m1->setColor(0xFF8BC34A);
        }
        if (auto* m2 = getMacro(2)) {
            m2->setName("Env Amt");
            m2->setColor(0xFFCDDC39);
        }
        if (auto* m3 = getMacro(3)) {
            m3->setName("Vibrato");
            m3->setColor(0xFFFFEB3B);
        }
    } else if (presetName == "Modern") {
        // Modern macro layout (Vital-style)
        if (auto* m0 = getMacro(0)) {
            m0->setName("Macro 1");
            m0->setColor(0xFF2196F3);
        }
        if (auto* m1 = getMacro(1)) {
            m1->setName("Macro 2");
            m1->setColor(0xFF03A9F4);
        }
        if (auto* m2 = getMacro(2)) {
            m2->setName("Macro 3");
            m2->setColor(0xFF00BCD4);
        }
        if (auto* m3 = getMacro(3)) {
            m3->setName("Macro 4");
            m3->setColor(0xFF009688);
        }
    } else if (presetName == "Filter Sweep") {
        // Filter-focused macros
        if (auto* m0 = getMacro(0)) {
            m0->setName("Cutoff");
            m0->setValue(0.3f);
            m0->setColor(0xFFFF5722);
        }
        if (auto* m1 = getMacro(1)) {
            m1->setName("Res");
            m1->setValue(0.2f);
            m1->setColor(0xFFFF9800);
        }
        if (auto* m2 = getMacro(2)) {
            m2->setName("Drive");
            m2->setValue(0.0f);
            m2->setColor(0xFFFFC107);
        }
        if (auto* m3 = getMacro(3)) {
            m3->setName("Env");
            m3->setValue(0.5f);
            m3->setColor(0xFFFFEB3B);
        }
    } else if (presetName == "Motion") {
        // Motion/animation focused
        if (auto* m0 = getMacro(0)) {
            m0->setName("LFO 1");
            m0->setColor(0xFF9C27B0);
        }
        if (auto* m1 = getMacro(1)) {
            m1->setName("LFO 2");
            m1->setColor(0xFF673AB7);
        }
        if (auto* m2 = getMacro(2)) {
            m2->setName("Time");
            m2->setColor(0xFF3F51B5);
        }
        if (auto* m3 = getMacro(3)) {
            m3->setName("Mix");
            m3->setColor(0xFF2196F3);
        }
    }
}

juce::uint32 MacroControl::getDefaultColor(int index) {
    // Vital-inspired macro colors
    static const juce::uint32 colors[] = {
        0xFF2196F3, // Blue
        0xFF03A9F4, // Light Blue
        0xFF00BCD4, // Cyan
        0xFF009688, // Teal
        0xFF4CAF50, // Green
        0xFF8BC34A, // Light Green
        0xFFCDDC39, // Lime
        0xFFFFEB3B  // Yellow
    };
    return colors[index % 8];
}

juce::uint32 MacroControlManager::getDefaultColor(int index) {
    return MacroControl::getDefaultColor(index);
}

void MacroControlManager::assignCommonParameters(int macroIndex, CommonParamType type) {
    auto* macro = getMacro(macroIndex);
    if (!macro) return;

    macro->clearMappings();

    switch (type) {
        case CommonParamType::Filter: {
            MacroMapping cutoff;
            cutoff.parameterId = "filter_cutoff";
            cutoff.parameterName = "Cutoff";
            cutoff.minimum = 20.0f;
            cutoff.maximum = 20000.0f;
            cutoff.curve = MacroCurve::Logarithmic;
            cutoff.mode = MacroMode::Absolute;
            macro->addMapping(cutoff);

            MacroMapping resonance;
            resonance.parameterId = "filter_resonance";
            resonance.parameterName = "Resonance";
            resonance.minimum = 0.0f;
            resonance.maximum = 1.0f;
            resonance.curve = MacroCurve::Exponential;
            resonance.mode = MacroMode::Absolute;
            macro->addMapping(resonance);
            break;
        }

        case CommonParamType::Envelope: {
            // ADSR envelope assignments
            const char* envParams[] = {"env1_attack", "env1_decay", "env1_sustain", "env1_release"};
            const char* envNames[] = {"Attack", "Decay", "Sustain", "Release"};
            float envMins[] = {0.001f, 0.001f, 0.0f, 0.001f};
            float envMaxs[] = {10.0f, 10.0f, 1.0f, 10.0f};

            for (int i = 0; i < 4; ++i) {
                MacroMapping env;
                env.parameterId = envParams[i];
                env.parameterName = envNames[i];
                env.minimum = envMins[i];
                env.maximum = envMaxs[i];
                env.curve = (i == 2) ? MacroCurve::Linear : MacroCurve::Logarithmic;
                env.mode = MacroMode::Absolute;
                env.amount = 0.5f;
                macro->addMapping(env);
            }
            break;
        }

        case CommonParamType::Oscillator: {
            // Oscillator detune and mix
            MacroMapping detune;
            detune.parameterId = "osc1_detune";
            detune.parameterName = "Detune";
            detune.minimum = -100.0f;
            detune.maximum = 100.0f;
            detune.curve = MacroCurve::Linear;
            detune.mode = MacroMode::Bipolar;
            macro->addMapping(detune);

            MacroMapping mix;
            mix.parameterId = "osc_mix";
            mix.parameterName = "Osc Mix";
            mix.minimum = 0.0f;
            mix.maximum = 1.0f;
            mix.curve = MacroCurve::Linear;
            mix.mode = MacroMode::Absolute;
            macro->addMapping(mix);
            break;
        }

        case CommonParamType::Effects: {
            // Effect send levels
            const char* fxParams[] = {"fx_reverb", "fx_delay", "fx_chorus", "fx_distortion"};
            const char* fxNames[] = {"Reverb", "Delay", "Chorus", "Distortion"};

            for (int i = 0; i < 4; ++i) {
                MacroMapping fx;
                fx.parameterId = fxParams[i];
                fx.parameterName = fxNames[i];
                fx.minimum = 0.0f;
                fx.maximum = 1.0f;
                fx.curve = MacroCurve::Linear;
                fx.mode = MacroMode::Absolute;
                fx.amount = 0.25f; // Lower amount for subtle control
                macro->addMapping(fx);
            }
            break;
        }

        case CommonParamType::LFO: {
            // LFO rate and amount
            MacroMapping lfo1Rate;
            lfo1Rate.parameterId = "lfo1_rate";
            lfo1Rate.parameterName = "LFO 1 Rate";
            lfo1Rate.minimum = 0.1f;
            lfo1Rate.maximum = 20.0f;
            lfo1Rate.curve = MacroCurve::Exponential;
            lfo1Rate.mode = MacroMode::Absolute;
            macro->addMapping(lfo1Rate);

            MacroMapping lfo1Amt;
            lfo1Amt.parameterId = "lfo1_amount";
            lfo1Amt.parameterName = "LFO 1 Amount";
            lfo1Amt.minimum = 0.0f;
            lfo1Amt.maximum = 1.0f;
            lfo1Amt.curve = MacroCurve::Linear;
            lfo1Amt.mode = MacroMode::Absolute;
            macro->addMapping(lfo1Amt);
            break;
        }

        case CommonParamType::All: {
            // Comprehensive assignment
            assignCommonParameters(macroIndex, CommonParamType::Filter);
            assignCommonParameters(macroIndex, CommonParamType::Oscillator);
            assignCommonParameters(macroIndex, CommonParamType::Envelope);
            break;
        }

        default:
            break;
    }
}

} // namespace zenith
