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

==============================================================================
// MODULATION MATRIX IMPLEMENTATION
//==============================================================================
*/

#include "ZenithModulationMatrix.h"
#include <cmath>

namespace zenith {

//==============================================================================
// MODULATION STATE
//==============================================================================

float ModulationStateExpanded::getSourceValue(ModulationSource source) const {
    switch (source) {
        case ModulationSource::LFO1: return lfo1;
        case ModulationSource::LFO2: return lfo2;
        case ModulationSource::StepLFO1: return stepLFO[0];
        case ModulationSource::StepLFO2: return stepLFO[1];
        case ModulationSource::StepLFO3: return stepLFO[2];
        case ModulationSource::StepLFO4: return stepLFO[3];
        case ModulationSource::ModEnvelope: return modEnv;
        case ModulationSource::AmpEnvelope: return ampEnv;
        case ModulationSource::Velocity: return velocity;
        case ModulationSource::ModWheel: return modWheel;
        case ModulationSource::Aftertouch: return aftertouch;
        case ModulationSource::PitchBend: return pitchBend;
        case ModulationSource::Timbre: return timbre;
        case ModulationSource::NoteNumber: return noteNumber;
        default: return 0.0f;
    }
}

//==============================================================================
// MODULATION MATRIX
//==============================================================================

void ZenithModulationMatrix::setSlot(int index, ModulationSource source,
                                    ModulationDestination dest, float amount,
                                    float min, float max,
                                    bool bipolar, int curve) {
    if (index < 0 || index >= MAX_SLOTS) return;

    slots_[index].source = source;
    slots_[index].destination = dest;
    slots_[index].amount = amount;
    slots_[index].min = min;
    slots_[index].max = max;
    slots_[index].bipolar = bipolar;
    slots_[index].curve = curve;
    slots_[index].active = (amount != 0.0f);
}

void ZenithModulationMatrix::clearSlot(int index) {
    if (index < 0 || index >= MAX_SLOTS) return;
    slots_[index] = ModulationSlotExpanded();
}

void ZenithModulationMatrix::setMacroValue(int macroIndex, float value) {
    value = juce::jlimit(0.0f, 1.0f, value);

    switch (macroIndex) {
        case 0: state_.macro1 = value; break;
        case 1: state_.macro2 = value; break;
        case 2: state_.macro3 = value; break;
        case 3: state_.macro4 = value; break;
    }
}

void ZenithModulationMatrix::updateSources(float midiNote, float velocity,
                                      const juce::MPENote& mpeNote) {
    state_.noteNumber = midiNote;
    state_.velocity = velocity;
    state_.pitchBend = static_cast<float>(mpeNote.pitchbend);
    state_.pressure = static_cast<float>(mpeNote.pressure);
    state_.timbre = static_cast<float>(mpeNote.timbre);
}

float ZenithModulationMatrix::getModulationFor(ModulationDestination dest) const {
    float total = 0.0f;

    for (const auto& slot : slots_) {
        if (!slot.active || slot.destination != dest) continue;

        float sourceValue = state_.getSourceValue(slot.source);

        // Apply scaling
        float scaled = scaleValue(sourceValue, slot.amount, slot.min, slot.max);

        // Apply curve
        scaled = applyCurve(scaled, slot.curve);

        total += scaled;
    }

    return total;
}

void ZenithModulationMatrix::clearAll() {
    slots_.fill(ModulationSlotExpanded());
}

float ZenithModulationMatrix::applyCurve(float value, int curve) {
    switch (curve) {
        case 0:  // Linear
            return value;
        case 1:  // Concave (log-like)
            // Reshape -1..1 to be more gradual near 0
            return (value < 0)
                ? -std::sqrt(-value)
                : std::sqrt(value);
        case 2:  // Convex (exp-like)
            // Reshape to be steeper near 0
            return value * std::abs(value);
        default:
            return value;
    }
}

float ZenithModulationMatrix::scaleValue(float value, float amount, float min, float max) {
    // value is assumed to be -1..1 (bipolar) or 0..1 (unipolar)
    // amount scales the modulation depth
    // min/max define the output range

    float scaled = value * amount;

    // Clamp to range
    return juce::jlimit(min, max, scaled);
}

juce::var ZenithModulationMatrix::toJSON() const {
    juce::var::Array* slotArray = new juce::var::Array();

    for (const auto& slot : slots_) {
        if (!slot.active) continue;

        juce::DynamicObject::Ptr slotObj = new juce::DynamicObject();
        slotObj->setProperty("source", static_cast<int>(slot.source));
        slotObj->setProperty("destination", static_cast<int>(slot.destination));
        slotObj->setProperty("amount", slot.amount);
        slotObj->setProperty("min", slot.min);
        slotObj->setProperty("max", slot.max);
        slotObj->setProperty("bipolar", slot.bipolar);
        slotObj->setProperty("curve", slot.curve);

        slotArray->add(slotObj.get());
    }

    return juce::var(slotArray);
}

void ZenithModulationMatrix::fromJSON(const juce::var& json) {
    if (!json.isArray()) return;

    juce::var::Array* slotArray = json.getArray();
    int index = 0;

    for (const auto& slotVar : *slotArray) {
        if (index >= MAX_SLOTS) break;
        if (!slotVar.isObject()) continue;

        auto slotObj = slotVar.getDynamicObject();
        if (!slotObj) continue;

        ModulationSource source = static_cast<ModulationSource>(
            slotObj->getProperty("source", static_cast<int>(ModulationSource::None)));
        ModulationDestination dest = static_cast<ModulationDestination>(
            slotObj->getProperty("destination", static_cast<int>(ModulationDestination::None)));
        float amount = slotObj->getProperty("amount", 0.0f);
        float min = slotObj->getProperty("min", -1.0f);
        float max = slotObj->getProperty("max", 1.0f);
        bool bipolar = slotObj->getProperty("bipolar", true);
        int curve = slotObj->getProperty("curve", 0);

        setSlot(index++, source, dest, amount, min, max, bipolar, curve);
    }
}

//==============================================================================
// ENVELOPE FOLLOWER
//==============================================================================

void EnvelopeFollower::setAttack(float seconds) {
    float samples = static_cast<float>(seconds * sampleRate_);
    attackCoeff_ = samples > 0 ? 1.0f / samples : 1.0f;
}

void EnvelopeFollower::setRelease(float seconds) {
    float samples = static_cast<float>(seconds * sampleRate_);
    releaseCoeff_ = samples > 0 ? 1.0f / samples : 1.0f;
}

float EnvelopeFollower::processSample(float input) {
    float inputAbs = std::abs(input);

    if (inputAbs > envelope_) {
        // Attack phase
        envelope_ += (inputAbs - envelope_) * attackCoeff_;
    } else {
        // Release phase
        envelope_ += (inputAbs - envelope_) * releaseCoeff_;
    }

    return envelope_;
}

//==============================================================================
// STEP LFO
//==============================================================================

StepLFO::StepLFO() {
    // Initialize with simple pattern
    steps_.fill(0.5f);
}

void StepLFO::setStep(int index, float value) {
    if (index >= 0 && index < NUM_STEPS) {
        steps_[index] = juce::jlimit(0.0f, 1.0f, value);
    }
}

float StepLFO::getStep(int index) const {
    if (index >= 0 && index < NUM_STEPS) {
        return steps_[index];
    }
    return 0.0f;
}

void StepLFO::setRate(float hz) {
    rate_ = juce::jmax(0.01f, hz);
}

void StepLFO::setSmooth(float amount) {
    smooth_ = juce::jlimit(0.0f, 1.0f, amount);
}

void StepLFO::process(int numSamples) {
    double phaseInc = rate_ / sampleRate_;
    phase_ = std::fmod(phase_ + phaseInc * numSamples, 1.0);

    float rawValue = getInterpolatedStep(phase_);

    // Apply smoothing
    float smoothFactor = smooth_;
    currentValue_ = currentValue_ * smoothFactor + rawValue * (1.0f - smoothFactor);
}

float StepLFO::getInterpolatedStep(double phase) const {
    float pos = phase * NUM_STEPS;
    int index1 = static_cast<int>(pos) % NUM_STEPS;
    int index2 = (index1 + 1) % NUM_STEPS;
    float frac = pos - static_cast<float>(index1);

    // Linear interpolation between steps
    return steps_[index1] + frac * (steps_[index2] - steps_[index1]);
}

} // namespace zenith
