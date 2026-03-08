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

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

//==============================================================================
// GLOBAL TUNING
//==============================================================================
/**
 * Master tuning controls matching Serum 2:
 * - Master transpose (-12 to +12 semitones)
 * - Master tune (±50 cents)
 * - Per-oscillator fine tune
 *
 * These affect all oscillators globally
 */
class ZenithGlobalTuning {
public:
    ZenithGlobalTuning() = default;

    //==========================================================================
    // Master Transpose
    //==========================================================================

    /**
     * @brief Set master transpose in semitones
     * @param semitones -12 to +12
     */
    void setTranspose(int semitones) {
        transpose_ = juce::jlimit(-12, 12, semitones);
    }

    int getTranspose() const { return transpose_; }

    //==========================================================================
    // Master Tune
    //==========================================================================

    /**
     * @brief Set master tune in cents
     * @param cents -50 to +50
     */
    void setMasterTune(float cents) {
        masterTune_ = juce::jlimit(-50.0f, 50.0f, cents);
    }

    float getMasterTune() const { return masterTune_; }

    //==========================================================================
    // Per-Oscillator Fine Tune
    //==========================================================================

    /**
     * @brief Set fine tune for specific oscillator
     * @param oscIndex 1-3
     * @param cents -100 to +100 cents
     */
    void setOscillatorFineTune(int oscIndex, float cents) {
        if (oscIndex >= 0 && oscIndex < 3) {
            oscFineTune_[oscIndex] = juce::jlimit(-100.0f, 100.0f, cents);
        }
    }

    float getOscillatorFineTune(int oscIndex) const {
        if (oscIndex >= 0 && oscIndex < 3) {
            return oscFineTune_[oscIndex];
        }
        return 0.0f;
    }

    //==========================================================================
    // Apply to Sample
    //==========================================================================

    /**
     * @brief Apply global tuning to frequency
     * @param baseFreq Base frequency in Hz
     * @return Tuned frequency
     */
    float applyTuning(float baseFreq) {
        // Apply master tune (±50 cents = ±1/2 octave)
        float tuned = baseFreq * std::exp2(masterTune_ / 1200.0f);

        // Apply transpose (±12 semitones = ±1 octave)
        tuned = tuned * std::exp2(transpose_ / 12.0f);

        return tuned;
    }

private:
    //==========================================================================
    // State
    //==========================================================================

    int transpose_ = 0;              // Master transpose in semitones
    float masterTune_ = 0.0f;       // Master tune in cents
    std::array<float, 3> oscFineTune_ = {-0.0f, 0.0f, 0.0f};

};

//==============================================================================
// RANDOMIZATION
//==============================================================================
/**
 * Smart randomization for synth parameters
 *
 * FEATURES:
 * - Randomize single parameter
 * - Randomize group (oscillators, envelopes, etc.)
 * - Randomize all (chaos mode)
 * - Exclude critical parameters (master tune, volume)
 *
 * Matches Serum 2 randomize quality
 */
class ZenithRandomizer {
public:
    ZenithRandomizer() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set random seed for reproducibility
     */
    void setSeed(uint32_t seed) {
        random_.setSeed(seed);
    }

    //==========================================================================
    // Randomize Options
    //==========================================================================

    enum class RandomizeScope {
        Single,              // Single parameter only
        Group,              // Related parameters (e.g., all osc levels)
        All,                // All parameters (chaos mode)
        ExcludeCritical    // All except master tune/volume
    };

    /**
     * @brief Smart randomization of parameters
     *
     * Uses musical ranges:
     * - Waveforms: sine/triangle/saw/square/noise
     * - Filter cutoff: 100Hz - 8kHz
     * - Filter resonance: 0 - 90%
     * - Envelopes: attack 1ms - 2s, release 10ms - 1s
     * - LFO rates: 0.1Hz - 20Hz
     * - Oscillator mix: 0 - 100%
     * - Detune: ±50 cents
     */
    struct RandomizedParams {
        // Oscillators
        std::array<OscillatorWaveform, 3> oscWaveforms;
        std::array<float, 3> oscMix;
        std::array<float, 3> oscDetune;
        std::array<float, 3> oscPulseWidth;

        // Filters
        float filterCutoff;
        float filterResonance;
        float filterDrive;
        FilterType filterType;

        // Envelopes
        float ampAttack;
        float ampDecay;
        float ampSustain;
        float ampRelease;
        float modAttack;
        float modDecay;
        float modSustain;
        float modRelease;

        // LFOs
        std::array<float, 2> lfoRate;
        std::array<float, 2> lfoAmount;

        // Global
        float masterTune;
    };

    /**
     * @brief Randomize parameters
     * @param scope What to randomize
     * @param excludeCritical Don't randomize master tune
     * @return Randomized parameter set
     */
    RandomizedParams randomize(RandomizeScope scope, bool excludeCritical = true);

private:
    //==========================================================================
    // Internal Helpers
    //==========================================================================

    juce::Random random_;

    // Oscillator waveforms
    OscillatorWaveform randomWaveform() {
        int r = random_.nextInt(5);
        switch (r) {
            case 0: return OscillatorWaveform::Sine;
            case 1: return OscillatorWaveform::Triangle;
            case 2: return OscillatorWaveform::Saw;
            case 3: return OscillatorWaveform::Square;
            case 4: return OscillatorWaveform::Noise;
            default: return OscillatorWaveform::Sine;
        }
    }

    // Random value in range [min, max]
    float randomRange(float min, float max) {
        return min + random_.nextFloat() * (max - min);
    }

    // Random cutoff (100Hz - 8kHz, logarithmic)
    float randomCutoff() {
        // Logarithmic distribution sounds more natural
        float minLog = std::log(100.0);
        float maxLog = std::log(8000.0);
        float logValue = minLog + random_.nextFloat() * (maxLog - minLog);
        return std::exp(logValue);
    }

    // Random resonance (0 - 90%)
    float randomResonance() {
        return random_.nextFloat() * 0.9f;
    }

    // Random envelope times (musical ranges)
    float randomAttack() {
        // 1ms - 2s
        return 0.001f + random_.nextFloat() * 0.001f;
    }

    float randomDecay() {
        // 50ms - 500ms
        return 0.05f + random_.nextFloat() * 0.45f;
    }

    float randomRelease() {
        // 10ms - 500ms
        return 0.01f + random_.nextFloat() * 0.49f;
    }

    // Random LFO rate (0.1Hz - 20Hz)
    float randomLFOrate() {
        // Logarithmic distribution
        float minLog = std::log(0.1f);
        float maxLog = std::log(20.0f);
        float logValue = minLog + random_.nextFloat() * (maxLog - minLog);
        return std::exp(logValue);
    }
};

} // namespace zenith
