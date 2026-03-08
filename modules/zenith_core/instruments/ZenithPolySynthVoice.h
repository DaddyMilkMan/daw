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

#include "ZenithOscillator.h"
#include "ZenithFilter.h"
#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace zenith {

//==============================================================================
// POLPHONIC SYNTH VOICE
//==============================================================================
/**
 * RT-safe MPE polyphonic voice with professional features
 *
 * FEATURES:
 * - 3 oscillators with sync, FM, ring mod
 * - 2 filters with serial/parallel routing
 * - 5-stage amp and mod envelopes
 * - 2 LFOs with 8 waveforms
 * - 8-slot modulation matrix
 * - 16-voice unison per oscillator
 * - Per-oscillator oversampling
 * - Analog drift simulation
 * - MPE expression support
 */
class ZenithPolySynthVoice : public juce::MPESynthesiserVoice {
public:
    ZenithPolySynthVoice();
    ~ZenithPolySynthVoice() override = default;

    //==========================================================================
    // MPE Overrides (RT-SAFE)
    //==========================================================================

    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePressureChanged() override;
    void notePitchbendChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                       int startSample, int numSamples) override;

    //==========================================================================
    // Oscillator Control
    //==========================================================================

    void setOsc1Waveform(OscillatorWaveform wf) { osc1_.setWaveform(wf); }
    void setOsc2Waveform(OscillatorWaveform wf) { osc2_.setWaveform(wf); }
    void setOsc3Waveform(OscillatorWaveform wf) { osc3_.setWaveform(wf); }

    void setOsc1Mix(float mix) { osc1Mix_ = mix; }
    void setOsc2Mix(float mix) { osc2Mix_ = mix; }
    void setOsc3Mix(float mix) { osc3Mix_ = mix; }

    void setOsc1Detune(float cents) { osc1_.setDetune(cents); }
    void setOsc2Detune(float cents) { osc2_.setDetune(cents); }
    void setOsc3Detune(float cents) { osc3_.setDetune(cents); }

    void setOsc1Shape(float shape) { osc1Shape_ = shape; }
    void setOsc2Shape(float shape) { osc2Shape_ = shape; }
    void setOsc3Shape(float shape) { osc3Shape_ = shape; }

    //==========================================================================
    // Oscillator 2 Sync/FM
    //==========================================================================

    void setOsc2Sync(bool sync) {
        osc2Sync_ = sync;
        if (sync) {
            osc2_.setSyncMaster(&osc1_);
        } else {
            osc2_.setSyncMaster(nullptr);
        }
    }

    void setOsc2FM(float amount) { osc2FM_ = juce::jlimit(0.0f, 1.0f, amount); }
    void setRingMod(float amount) { ringMod_ = juce::jlimit(0.0f, 1.0f, amount); }

    //==========================================================================
    // Unison Control
    //==========================================================================

    void setUnisonVoices(int voices) {
        unisonVoices_ = juce::jlimit(1, 16, voices);
        osc1_.setUnisonVoices(voices);
        osc2_.setUnisonVoices(voices);
        osc3_.setUnisonVoices(voices);
    }

    void setUnisonDetune(float cents) { unisonDetune_ = cents; }
    void setUnisonSpread(float spread) { unisonSpread_ = juce::jlimit(0.0f, 1.0f, spread); }

    //==========================================================================
    // Filter Control
    //==========================================================================

    void setFilter1Type(FilterType type) { filter1_.setType(type); }
    void setFilter1Model(FilterModelType model) { filter1_.setModel(model); }
    void setFilter1Cutoff(float cutoff) { filter1Cutoff_ = cutoff; }
    void setFilter1Resonance(float res) { filter1_.setResonance(res); }
    void setFilter1Drive(float drive) { filter1_.setDrive(drive); }

    void setFilter2Type(FilterType type) { filter2_.setType(type); }
    void setFilter2Model(FilterModelType model) { filter2_.setModel(model); }
    void setFilter2Cutoff(float cutoff) { filter2Cutoff_ = cutoff; }
    void setFilter2Resonance(float res) { filter2_.setResonance(res); }

    void setFilterRouting(bool serial) { filtersSerial_ = serial; }
    void setFilterEnvAmount(float amount) { filterEnvAmount_ = amount; }

    //==========================================================================
    // Envelope Control
    //==========================================================================

    void setAmpEnv(float attack, float decay, float sustain, float release);
    void setModEnv(float attack, float decay, float sustain, float release);

    //==========================================================================
    // LFO Control
    //==========================================================================

    void setLFO1(float rate, float amount, LFOTarget target, LFOWaveform waveform);
    void setLFO2(float rate, float amount, LFOTarget target, LFOWaveform waveform);

    //==========================================================================
    // Quality & Performance
    //==========================================================================

    void setOsc1Quality(OscillatorOversamplingQuality q) { osc1_.setOversamplingQuality(q); }
    void setOsc2Quality(OscillatorOversamplingQuality q) { osc2_.setOversamplingQuality(q); }
    void setOsc3Quality(OscillatorOversamplingQuality q) { osc3_.setOversamplingQuality(q); }

    void setFilterOversampling(int factor) {
        filter1_.setOversampling(factor);
        filter2_.setOversampling(factor);
    }

    void setAnalogDrift(float amount) {
        osc1_.setAnalogDrift(amount);
        osc2_.setAnalogDrift(amount);
        osc3_.setAnalogDrift(amount);
    }

    //==========================================================================
    // Modulation Matrix
    //==========================================================================

    void setModulationSlot(int index, ModulationSource source,
                         ModulationDestination dest, float amount);

    void setModWheel(float value) { modWheel_ = juce::jlimit(0.0f, 1.0f, value); }
    void setAftertouch(float value) { aftertouch_ = juce::jlimit(0.0f, 1.0f, value); }

    //==========================================================================
    // Sample Rate
    //==========================================================================

    void setSampleRate(double sampleRate);

    //==========================================================================
    // State Query
    //==========================================================================

    float getCurrentAmplitude() const { return currentAmplitude_; }
    bool isActive() const { return isActive_; }

private:
    //==========================================================================
    // Oscillators
    //==========================================================================

    ZenithOscillator osc1_, osc2_, osc3_;

    // Oscillator parameters
    juce::SmoothedValue<float> osc1Mix_{0.0f};
    juce::SmoothedValue<float> osc2Mix_{0.0f};
    juce::SmoothedValue<float> osc3Mix_{0.0f};

    juce::SmoothedValue<float> osc1Shape_{0.5f};
    juce::SmoothedValue<float> osc2Shape_{0.5f};
    juce::SmoothedValue<float> osc3Shape_{0.5f};

    // Oscillator 2 sync/FM
    bool osc2Sync_ = false;
    float osc2FM_ = 0.0f;
    float ringMod_ = 0.0f;

    //==========================================================================
    // Filters
    //==========================================================================

    ZenithFilter filter1_, filter2_;

    float filter1Cutoff_ = 1000.0f;
    float filter2Cutoff_ = 1000.0f;
    bool filtersSerial_ = true;
    float filterEnvAmount_ = 0.5f;

    //==========================================================================
    // Envelopes
    //==========================================================================

    juce::ADSR ampEnvelope_;
    juce::ADSR modEnvelope_;

    juce::ADSR::Parameters ampEnvParams_{0.01f, 0.1f, 0.7f, 0.3f};
    juce::ADSR::Parameters modEnvParams_{0.05f, 0.2f, 0.5f, 0.5f};

    //==========================================================================
    // LFOs
    //==========================================================================

    double lfo1Phase_ = 0.0;
    double lfo2Phase_ = 0.0;
    float lfo1Rate_ = 1.0f;
    float lfo2Rate_ = 1.0f;
    float lfo1Amount_ = 0.0f;
    float lfo2Amount_ = 0.0f;
    LFOWaveform lfo1Waveform_ = LFOWaveform::Sine;
    LFOWaveform lfo2Waveform_ = LFOWaveform::Sine;
    LFOTarget lfo1Target_ = LFOTarget::FilterCutoff;
    LFOTarget lfo2Target_ = LFOTarget::FilterCutoff;

    //==========================================================================
    // Unison
    //==========================================================================

    int unisonVoices_ = 1;
    float unisonDetune_ = 0.0f;
    float unisonSpread_ = 0.5f;

    //==========================================================================
    // Modulation
    //==========================================================================

    std::array<ModulationSlot, 8> modulationMatrix_;
    float modWheel_ = 0.0f;
    float aftertouch_ = 0.0f;
    float currentAmplitude_ = 0.0f;

    //==========================================================================
    // Performance State
    //==========================================================================

    bool isActive_ = false;
    double sampleRate_ = 44100.0;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void computeModulation();
    float getModulationSourceValue(ModulationSource source);
    float applyModulationToDestination(ModulationDestination dest, float value);
    float computeLFOValue(double phase, LFOWaveform waveform);
};

} // namespace zenith
