/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "ZenithPolySynthVoice.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithPolySynthVoice::ZenithPolySynthVoice() {
    // Initialize oscillators
    osc1_.setWaveform(OscillatorWaveform::Saw);
    osc2_.setWaveform(OscillatorWaveform::Saw);
    osc3_.setWaveform(OscillatorWaveform::Triangle);

    // Initialize filters
    filter1_.setType(FilterType::LowPass);
    filter2_.setType(FilterType::LowPass);

    // Initialize LFO phases
    lfo1Phase_ = 0.0;
    lfo2Phase_ = 0.0;
}

ZenithPolySynthVoice::~ZenithPolySynthVoice() = default;

//==============================================================================
// MPE OVERRIDES - RT-SAFE
//==============================================================================

void ZenithPolySynthVoice::noteStarted() {
    isActive_ = true;
    currentAmplitude_ = 0.0f;

    // Reset envelopes
    ampEnvelope_.noteOn();
    modEnvelope_.noteOn();

    // Reset LFO phases
    lfo1Phase_ = 0.0;
    lfo2Phase_ = 0.0;

    // Initialize modulation matrix
    for (auto& slot : modulationMatrix_) {
        slot.active = (slot.amount != 0.0f);
    }
}

void ZenithPolySynthVoice::noteStopped(bool allowTailOff) {
    if (allowTailOff) {
        ampEnvelope_.noteOff();
    } else {
        ampEnvelope_.reset();
        modEnvelope_.reset();
        isActive_ = false;
        currentAmplitude_ = 0.0f;
    }
}

void ZenithPolySynthVoice::notePressureChanged() {
    // MPE pressure (aftertouch) is handled during render
}

void ZenithPolySynthVoice::notePitchbendChanged() {
    // MPE pitch bend is handled during render
}

void ZenithPolySynthVoice::noteTimbreChanged() {
    // MPE timbre is handled during render
}

void ZenithPolySynthVoice::noteKeyStateChanged() {
    // MPE key state is handled during render
}

//==============================================================================
// RENDER - RT-SAFE
//==============================================================================

void ZenithPolySynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                         int startSample, int numSamples) {
    if (!isActive_) {
        return;
    }

    const int numChannels = outputBuffer.getNumChannels();
    const float* left = outputBuffer.getWritePointer(0);
    const float* right = numChannels > 1 ? outputBuffer.getWritePointer(1) : left;

    // Get current note values from MPE
    const auto& mpeNote = getCurrentlyPlayingNote();
    const float noteNumber = mpeNote.initialNote;
    const float velocity = mpeNote.noteOnVelocity.convertFrom0to1();
    const float pitchBend = mpeNote.pitchbend;
    const float pressure = mpeNote.pressure;
    const float timbre = mpeNote.timbre;

    // Update modulation sources
    updateModulationSources(noteNumber, velocity, pitchBend, pressure, timbre);

    for (int i = 0; i < numSamples; ++i) {
        // Calculate current amplitude from envelope
        float ampEnv = ampEnvelope_.getNextSample();

        // Calculate modulation envelope
        float modEnv = modEnvelope_.getNextSample();

        // Get LFO values
        float lfo1 = computeLFOValue(lfo1Phase_, lfo1Waveform_);
        float lfo2 = computeLFOValue(lfo2Phase_, lfo2Waveform_);

        // Advance LFO phases
        double sampleRateInv = 1.0 / sampleRate_;
        lfo1Phase_ = std::fmod(lfo1Phase_ + lfo1Rate_ * sampleRateInv, 1.0);
        lfo2Phase_ = std::fmod(lfo2Phase_ + lfo2Rate_ * sampleRateInv, 1.0);

        // Calculate unison voices for each oscillator
        float osc1Sample = osc1_.getSample(noteNumber, velocity);
        float osc2Sample = osc2_.getSample(noteNumber, velocity);
        float osc3Sample = osc3_.getSample(noteNumber, velocity);

        // Apply oscillator mix
        float mixed = osc1Sample * osc1Mix_ + osc2Sample * osc2Mix_ + osc3Sample * osc3Mix_;

        // Apply modulation
        float modulated = applyModulation(mixed, modEnv, lfo1, lfo2, velocity);

        // Process through filters
        float filtered = processFilters(modulated);

        // Apply amplitude envelope
        float output = filtered * ampEnv;

        // Store current amplitude
        currentAmplitude_ = ampEnv;

        // Write to output
        left[startSample + i] = output;
        if (numChannels > 1) {
            right[startSample + i] = output;
        }
    }
}

//==============================================================================
// MODULATION PROCESSING
//==============================================================================

void ZenithPolySynthVoice::computeModulation() {
    // Compute total modulation for each destination
    for (int dest = 0; dest < static_cast<int>(ModulationDestination::Pan) ++dest) {
        float totalMod = 0.0f;

        for (const auto& slot : modulationMatrix_) {
            if (slot.active && slot.destination == dest) {
                float sourceValue = getModulationSourceValue(slot.source);
                totalMod += slot.amount * sourceValue;
            }
        }

        // Apply to destination
        applyModulationToDestination(static_cast<ModulationDestination>(dest), totalMod);
    }
}

float ZenithPolySynthVoice::getModulationSourceValue(ModulationSource source) {
    switch (source) {
        case ModulationSource::LFO1:
            return computeLFOValue(lfo1Phase_, lfo1Waveform_);
        case ModulationSource::LFO2:
            return computeLFOValue(lfo2Phase_, lfo2Waveform_);
        case ModulationSource::AmpEnvelope:
            return ampEnvelope_.getNextSample();
        case ModulationSource::ModEnvelope:
            return modEnvelope_.getNextSample();
        case ModulationSource::Velocity:
            return getCurrentAmplitude();
        case ModulationSource::ModWheel:
            return modWheel_;
        case ModulationSource::Aftertouch:
            return aftertouch_;
        default:
            return 0.0f;
    }
}

float ZenithPolySynthVoice::applyModulationToDestination(ModulationDestination dest, float value) {
    switch (dest) {
        case ModulationDestination::Osc1Pitch:
            osc1_.setPitchOffset(value);
            break;
        case ModulationDestination::Osc2Pitch:
            osc2_.setPitchOffset(value);
            break;
        case ModulationDestination::Osc3Pitch:
            osc3_.setPitchOffset(value);
            break;
        case ModulationDestination::Osc1Mix:
            osc1Mix_ = value;
            break;
        case ModulationDestination::Osc2Mix:
            osc2Mix_ = value;
            break;
        case ModulationDestination::Osc3Mix:
            osc3Mix_ = value;
            break;
        case ModulationDestination::FilterCutoff:
            filter1Cutoff_ += value;
            break;
        case ModulationDestination::FilterResonance:
            filter1_.setResonance(filter1Resonance_ + value);
            break;
        default:
            break;
    }
}

float ZenithPolySynthVoice::computeLFOValue(double phase, LFOWaveform waveform) {
    float phasePos = static_cast<float>(phase);

    switch (waveform) {
        case LFOWaveform::Sine:
            return std::sin(phasePos * juce::MathConstants<float>::twoPi);
        case LFOWaveform::Triangle:
            return 2.0f * std::abs(phasePos - 0.5f) - 1.0f;
        case LFOWaveform::Square:
            return (phasePos < 0.5f) ? 1.0f : -1.0f;
        case LFOWaveform::Saw:
            return 2.0f * phasePos - 1.0f;
        case LFOWaveform::SampleAndHold:
            return (phasePos < 0.5f) ? 1.0f : 0.0f;
        default:
            return 0.0f;
    }
}

float ZenithPolySynthVoice::applyModulation(float input, float modEnv,
                                          float lfo1, float lfo2, float velocity) {
    float output = input;

    // Apply envelope modulation
    output += modEnv * 0.5f;

    // Apply LFO1
    output += lfo1 * lfo1Amount_;

    // Apply LFO2
    output += lfo2 * lfo2Amount_;

    // Apply velocity scaling
    output *= (0.5f + velocity * 0.5f);

    return output;
}

float ZenithPolySynthVoice::processFilters(float input) {
    // Process through serial filters
    float output = input;

    // Apply filter 1
    output = filter1_.processSample(output, filter1Cutoff_, filter1Resonance_);

    // Apply filter 2 (if serial routing)
    if (filtersSerial_) {
        output = filter2_.processSample(output, filter2Cutoff_, filter2Resonance_);
    }

    return output;
}

//==============================================================================
// PARAMETER SETTERS
//==============================================================================

void ZenithPolySynthVoice::setAmpEnv(float attack, float decay, float sustain, float release) {
    juce::ADSR::Parameters params;
    params.attack = attack;
    params.decay = decay;
    params.sustain = sustain;
    params.release = release;
    ampEnvelope_.setParameters(params);
}

void ZenithPolySynthVoice::setModEnv(float attack, float decay, float sustain, float release) {
    juce::ADSR::Parameters params;
    params.attack = attack;
    params.decay = decay;
    params.sustain = sustain;
    params.release = release;
    modEnvelope_.setParameters(params);
}

void ZenithPolySynthVoice::setLFO1(float rate, float amount, LFOTarget target, LFOWaveform waveform) {
    lfo1Rate_ = rate;
    lfo1Amount_ = amount;
    lfo1Target_ = target;
    lfo1Waveform_ = waveform;
}

void ZenithPolySynthVoice::setLFO2(float rate, float amount, LFOTarget target, LFOWaveform waveform) {
    lfo2Rate_ = rate;
    lfo2Amount_ = amount;
    lfo2Target_ = target;
    lfo2Waveform_ = waveform;
}

void ZenithPolySynthVoice::setModulationSlot(int index, ModulationSource source,
                                        ModulationDestination dest, float amount) {
    if (index >= 0 && index < 8) {
        modulationMatrix_[index].source = source;
        modulationMatrix_[index].destination = dest;
        modulationMatrix_[index].amount = amount;
        modulationMatrix_[index].active = (amount != 0.0f);
    }
}

void ZenithPolySynthVoice::setModWheel(float value) {
    modWheel_ = juce::jlimit(0.0f, 1.0f, value);
}

void ZenithPolySynthVoice::setAftertouch(float value) {
    aftertouch_ = juce::jlimit(0.0f, 1.0f, value);
}

void ZenithPolySynthVoice::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;

    // Update all components
    osc1_.setSampleRate(sampleRate);
    osc2_.setSampleRate(sampleRate);
    osc3_.setSampleRate(sampleRate);
    filter1_.setSampleRate(sampleRate);
    filter2_.setSampleRate(sampleRate);
    ampEnvelope_.setSampleRate(sampleRate);
    modEnvelope_.setSampleRate(sampleRate);
}

} // namespace zenith
