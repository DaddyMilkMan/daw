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
// POLYPHONIC SYNTH VOICE IMPLEMENTATION
//==============================================================================
*/

#include "ZenithPolySynthVoice.h"
#include <cmath>

namespace zenith {

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithPolySynthVoice::ZenithPolySynthVoice() {
    // Initialize oscillators
    osc1_.setWaveform(OscillatorWaveform::Saw);
    osc2_.setWaveform(OscillatorWaveform::Saw);
    osc3_.setWaveform(OscillatorWaveform::Saw);

    // Initialize envelopes
    ampEnvelope_.setParameters(ampEnvParams_);
    modEnvelope_.setParameters(modEnvParams_);

    // Initialize filters
    filter1_.setType(FilterType::LowPass);
    filter2_.setType(FilterType::LowPass);
}

//==============================================================================
// SAMPLE RATE
//==============================================================================

void ZenithPolySynthVoice::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    osc1_.setSampleRate(sampleRate);
    osc2_.setSampleRate(sampleRate);
    osc3_.setSampleRate(sampleRate);
    filter1_.setSampleRate(sampleRate);
    filter2_.setSampleRate(sampleRate);
    ampEnvelope_.setSampleRate(sampleRate);
    modEnvelope_.setSampleRate(sampleRate);
}

//==============================================================================
// MPE OVERRIDES
//==============================================================================

void ZenithPolySynthVoice::noteStarted() {
    isActive_ = true;
    ampEnvelope_.noteOn();
    modEnvelope_.noteOn();
    currentAmplitude_ = 0.0f;
}

void ZenithPolySynthVoice::noteStopped(bool allowTailOff) {
    if (allowTailOff) {
        ampEnvelope_.noteOff();
        modEnvelope_.noteOff();
    } else {
        isActive_ = false;
        ampEnvelope_.reset();
        modEnvelope_.reset();
    }
}

void ZenithPolySynthVoice::notePressureChanged() {
    // MPE pressure (aftertouch) is handled in render via getNotePressure()
}

void ZenithPolySynthVoice::notePitchbendChanged() {
    // MPE pitch bend is handled in render via getNotePitchbend()
}

void ZenithPolySynthVoice::noteTimbreChanged() {
    // MPE timbre (slide) is handled in render via getNoteTimbre()
}

void ZenithPolySynthVoice::noteKeyStateChanged() {
    // Handle sustain pedal, etc.
}

//==============================================================================
// RENDERING
//==============================================================================

void ZenithPolySynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                       int startSample, int numSamples) {
    if (!isActive_) return;

    // Update MPE values
    float mpePressure = static_cast<float>(getNotePressure());
    float mpePitchBend = static_cast<float>(getNotePitchbend());
    float mpeTimbre = static_cast<float>(getNoteTimbre());

    // Apply MPE to internal state
    aftertouch_ = juce::jlimit(0.0f, 1.0f, mpePressure);
    // timbre_ = juce::jlimit(0.0f, 1.0f, mpeTimbre);

    auto* leftOut = outputBuffer.getWritePointer(0, startSample);
    auto* rightOut = outputBuffer.getWritePointer(1, startSample);

    for (int i = 0; i < numSamples; ++i) {
        // Compute modulation
        computeModulation();

        // Update envelopes
        float amp = ampEnvelope_.getNextSample();
        float mod = modEnvelope_.getNextSample();

        // Update LFO phases
        double lfo1Inc = lfo1Rate_ / sampleRate_;
        double lfo2Inc = lfo2Rate_ / sampleRate_;
        lfo1Phase_ = std::fmod(lfo1Phase_ + lfo1Inc, 1.0);
        lfo2Phase_ = std::fmod(lfo2Phase_ + lfo2Inc, 1.0);

        // Get LFO values
        float lfo1 = computeLFOValue(lfo1Phase_, lfo1Waveform_);
        float lfo2 = computeLFOValue(lfo2Phase_, lfo2Waveform_);

        // Calculate base frequency from MIDI note
        float midiNote = static_cast<float>(getCurrentlyPlayingNote());
        float frequency = 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);

        // Apply pitch bend (MPE)
        float bendRange = 2.0f; // ±2 semitones
        float bendCents = mpePitchBend * bendRange * 100.0f;
        frequency *= std::pow(2.0f, bendCents / 1200.0f);

        // Generate oscillator samples
        float osc1Sample = osc1_.getNextSample(frequency, osc1Shape_.getNextValue());
        float osc2Sample = osc2_.getNextSample(frequency, osc2Shape_.getNextValue());
        float osc3Sample = osc3_.getNextSample(frequency, osc3Shape_.getNextValue());

        // Apply Osc2 FM (osc1 modulates osc2's phase)
        if (osc2FM_ > 0.0f) {
            // FM is handled inside oscillator via phase modulation
            // For now, we apply it as amplitude modulation of the FM amount
            float fmDepth = osc2FM_ * 100.0f; // cents
            // Real FM synthesis would be done inside the oscillator
        }

        // Apply ring modulation (osc1 * osc2)
        float ringSample = 0.0f;
        if (ringMod_ > 0.0f) {
            ringSample = osc1Sample * osc2Sample * ringMod_;
        }

        // Mix oscillators
        float mix1 = osc1Mix_.getNextValue();
        float mix2 = osc2Mix_.getNextValue();
        float mix3 = osc3Mix_.getNextValue();

        float sample = osc1Sample * mix1 + osc2Sample * mix2 + osc3Sample * mix3;

        // Add ring mod
        sample += ringSample;

        // Calculate modulated filter cutoff
        float modEnvCutoff = filter1Cutoff_ * std::pow(2.0f, mod * filterEnvAmount_ * 4.0f);

        // Apply LFO1 to filter if targeted
        float lfo1Mod = 0.0f;
        if (lfo1Target_ == LFOTarget::FilterCutoff) {
            lfo1Mod = lfo1 * lfo1Amount_ * 2000.0f;
        }

        float cutoff1 = juce::jlimit(20.0f, 20000.0f, modEnvCutoff + lfo1Mod);
        filter1_.setCutoff(cutoff1);

        // Process through filters
        float filtered;
        if (filtersSerial_) {
            // Serial routing
            float afterFilter1 = filter1_.processSample(sample, midiNote);
            filtered = filter2_.processSample(afterFilter1, midiNote);
        } else {
            // Parallel routing
            float f1 = filter1_.processSample(sample, midiNote);
            float f2 = filter2_.processSample(sample, midiNote);
            filtered = (f1 + f2) * 0.5f;
        }

        // Apply amplitude envelope
        float masterGain = 0.5f; // Prevent clipping
        float finalSample = filtered * amp * masterGain;

        // Output (stereo)
        leftOut[i] += finalSample;
        rightOut[i] += finalSample;

        currentAmplitude_ = std::abs(finalSample);
    }

    // Check if voice has finished: the amp envelope reports inactive once its
    // release stage has fully decayed (noteOff() is driven from noteStopped()).
    if (!ampEnvelope_.isActive()) {
        isActive_ = false;
        clearCurrentNote();
    }
}

//==============================================================================
// MODULATION
//==============================================================================

void ZenithPolySynthVoice::computeModulation() {
    // Apply modulation matrix
    for (const auto& slot : modulationMatrix_) {
        if (!slot.active) continue;

        float sourceValue = getModulationSourceValue(slot.source);
        applyModulationToDestination(slot.destination, sourceValue * slot.amount);
    }
}

float ZenithPolySynthVoice::getModulationSourceValue(ModulationSource source) {
    switch (source) {
        case ModulationSource::LFO1:
            return computeLFOValue(lfo1Phase_, lfo1Waveform_);
        case ModulationSource::LFO2:
            return computeLFOValue(lfo2Phase_, lfo2Waveform_);
        case ModulationSource::ModEnvelope:
            return modEnvelope_.getNextSample();
        case ModulationSource::AmpEnvelope:
            return ampEnvelope_.getNextSample();
        case ModulationSource::Velocity:
            return static_cast<float>(getNotePressure()); // Use pressure as velocity proxy
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
            // Apply as cents offset
            return value;
        case ModulationDestination::Osc2Pitch:
            return value;
        case ModulationDestination::Osc3Pitch:
            return value;
        case ModulationDestination::FilterCutoff:
            // Add to filter cutoff
            return juce::jlimit(20.0f, 20000.0f, filter1Cutoff_ + value);
        default:
            return value;
    }
}

float ZenithPolySynthVoice::computeLFOValue(double phase, LFOWaveform waveform) {
    double twoPiPhase = phase * juce::MathConstants<double>::twoPi;

    switch (waveform) {
        case LFOWaveform::Sine:
            return static_cast<float>(std::sin(twoPiPhase));

        case LFOWaveform::Triangle:
            return static_cast<float>(2.0 * std::abs(2.0 * phase - 1.0) - 1.0);

        case LFOWaveform::Square:
            return (phase < 0.5) ? 1.0f : -1.0f;

        case LFOWaveform::Saw:
            return static_cast<float>(2.0 * phase - 1.0);

        case LFOWaveform::SampleAndHold:
            // Would need state for this
            return static_cast<float>(std::sin(twoPiPhase));

        case LFOWaveform::Random:
            return static_cast<float>(std::sin(twoPiPhase));

        default:
            return 0.0f;
    }
}

//==============================================================================
// ENVELOPES
//==============================================================================

void ZenithPolySynthVoice::setAmpEnvelope(float attack, float decay, float sustain, float release) {
    ampEnvParams_.attack = attack;
    ampEnvParams_.decay = decay;
    ampEnvParams_.sustain = sustain;
    ampEnvParams_.release = release;
    ampEnvelope_.setParameters(ampEnvParams_);
}

void ZenithPolySynthVoice::setModEnvelope(float attack, float decay, float sustain, float release) {
    modEnvParams_.attack = attack;
    modEnvParams_.decay = decay;
    modEnvParams_.sustain = sustain;
    modEnvParams_.release = release;
    modEnvelope_.setParameters(modEnvParams_);
}

//==============================================================================
// LFOs
//==============================================================================

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

//==============================================================================
// MODULATION MATRIX
//==============================================================================

void ZenithPolySynthVoice::setModulationSlot(int index, ModulationSource source,
                                          ModulationDestination dest, float amount) {
    if (index >= 0 && index < static_cast<int>(modulationMatrix_.size())) {
        modulationMatrix_[index] = ModulationSlot(source, dest, amount);
    }
}

} // namespace zenith
