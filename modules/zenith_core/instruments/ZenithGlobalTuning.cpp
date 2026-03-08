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

#include "ZenithGlobalTuning.h"

namespace zenith {

//==============================================================================
// CONSTRUCTOR/DESTRUCTOR
//==============================================================================

ZenithGlobalTuning::ZenithGlobalTuning() = default;

ZenithGlobalTuning::~ZenithGlobalTuning() = default;

//==============================================================================
// CONFIGURATION
//==============================================================================

void ZenithGlobalTuning::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void ZenithGlobalTuning::reset() {
    transpose_ = 0;
    masterTune_ = 0.0f;
    oscFineTune_.fill(0.0f);
}

//==============================================================================
// MASTER TRANSPOSE
//==============================================================================

void ZenithGlobalTuning::setTranspose(int semitones) {
    transpose_ = juce::jlimit(-12, 12, semitones);
}

int ZenithGlobalTuning::getTranspose() const {
    return transpose_;
}

//==============================================================================
// MASTER TUNE
//==============================================================================

void ZenithGlobalTuning::setMasterTune(float cents) {
    masterTune_ = juce::jlimit(-50.0f, 50.0f, cents);
}

float ZenithGlobalTuning::getMasterTune() const {
    return masterTune_;
}

//==============================================================================
// PER-OSCILLATOR FINE TUNE
//==============================================================================

void ZenithGlobalTuning::setOscillatorFineTune(int oscIndex, float cents) {
    if (oscIndex >= 0 && oscIndex < 3) {
        oscFineTune_[oscIndex] = juce::jlimit(-100.0f, 100.0f, cents);
    }
}

float ZenithGlobalTuning::getOscillatorFineTune(int oscIndex) const {
    if (oscIndex >= 0 && oscIndex < 3) {
        return oscFineTune_[oscIndex];
    }
    return 0.0f;
}

//==============================================================================
// RANDOMIZATION
//==============================================================================

/**
 * @brief Smart randomization of synth parameters
 * @param scope What to randomize
 * @param excludeCritical Don't randomize master tune/volume
 * @return Randomized parameter set
 */
RandomizedParams ZenithGlobalTuning::randomize(RandomizeScope scope, bool excludeCritical = true) {
    RandomizedParams params;
    juce::Random random;

    // Randomize oscillator waveforms
    if (scope.testFlag(RandomizeScope::Oscillators)) {
        int r = random.nextInt(5);
        params.oscWaveform_[0] = static_cast<OscillatorWaveform>(r);
        params.oscWaveform_[1] = static_cast<OscillatorWaveform>(r);
        params.oscWaveform_[2] = static_cast<OscillatorWaveform>(r);
    }

    // Randomize oscillator mix levels
    if (scope.testFlag(RandomizeScope::Oscillators)) {
        params.oscMix_[0] = random.nextFloat() * 0.5f + 0.5f;
        params.oscMix_[1] = random.nextFloat() * 0.5f + 0.5f;
        params.oscMix_[2] = random.nextFloat() * 0.5f + 0.5f;
    }

    // Randomize filter cutoff (logarithmic distribution)
    if (scope.testFlag(RandomizeScope::Filters)) {
        // 100Hz - 8kHz logarithmic
        float minLog = std::log(100.0f);
        float maxLog = std::log(8000.0f);
        params.filterCutoff_ = std::exp(minLog + random.nextFloat() * (maxLog - minLog));
    }

    // Randomize filter resonance
    if (scope.testFlag(RandomizeScope::Filters)) {
        params.filterResonance_ = random.nextFloat() * 0.5f + 0.5f;
    }

    // Randomize envelope times
    if (scope.testFlag(RandomizeScope::Envelopes)) {
        params.ampAttack_ = 0.5f + random.nextFloat() * 2.0f;
        params.ampDecay_ = 0.5f + random.nextFloat() * 2.0f;
        params.ampRelease_ = 0.5f + random.nextFloat() * 2.0f;
        params.modAttack_ = 0.5f + random.nextFloat() * 2.0f;
        params.modDecay_ = 0.5f + random.nextFloat() * 2.0f;
        params.modRelease_ = 0.5f + random.nextFloat() * 2.0f;
    }

    // Randomize LFO rates
    if (scope.testFlag(RandomizeScope::LFOs)) {
        params.lfo1Rate_ = 1.0f + random.nextFloat() * 19.0f;
        params.lfo2Rate_ = 1.0f + random.nextFloat() * 19.0f;
    }

    // Randomize LFO depths
    if (scope.testFlag(RandomizeScope::LFOs)) {
        params.lfo1Depth_ = random.nextFloat();
        params.lfo2Depth_ = random.nextFloat();
    }

    // Randomize oscillator fine tune
    if (scope.testFlag(RandomizeScope::Oscillators)) {
        params.osc1FineTune_ = random.nextFloat() * 200.0f - 100.0f;
        params.osc2FineTune_ = random.nextFloat() * 200.0f - 100.0f;
        params.osc3FineTune_ = random.nextFloat() * 200.0f - 100.0f;
    }

    // Randomize unison voices
    if (scope.testFlag(RandomizeScope::Unison)) {
        params.unisonVoices_ = random.nextInt({1, 2, 4, 8, 16}) + 1;
    }

    // Randomize panning
    if (scope.testFlag(RandomizeScope::Pan)) {
        params.pan_ = random.nextFloat() * 2.0f - 1.0f;
    }

    return params;
}

} // namespace zenith
