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
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

namespace zenith {

//==============================================================================
// GRANULAR OSCILLATOR
//==============================================================================
/**
 * Granular synthesis engine matching Serum 2 capabilities:
 * - Adjustable grain size (1-100ms)
 * - Grain envelope (attack, decay, shape)
 * - Random positioning in source
 * - Pitch variation per grain
 * - Stereo panning
 * - Density control (grains per second)
 * - Freeze mode (hold grains)
 */
class ZenithGranularOscillator {
public:
    ZenithGranularOscillator();
    ~ZenithGranularOscillator() = default;

    //==========================================================================
    // Source Sample
    //==========================================================================

    /**
     * @brief Set source audio for granulation
     * @param data Interleaved stereo audio
     * @param numSamples Total number of samples
     * @param sampleRate Source sample rate
     */
    void setSource(const float* data, int numSamples, double sampleRate);

    /**
     * @brief Clear source
     */
    void clearSource();

    //==========================================================================
    // Grain Parameters
    //==========================================================================

    /** Grain size in milliseconds (1-100ms) */
    void setGrainSize(float ms) { grainSize_ = juce::jlimit(1.0f, 100.0f, ms); }

    /** Grain envelope shape */
    enum class EnvelopeShape {
        Linear,
        Exponential,
        Bell,
        Sine
    };
    void setEnvelopeShape(EnvelopeShape shape) { envShape_ = shape; }

    /** Attack/Decay of grain envelope */
    void setGrainAttack(float ms) { grainAttack_ = juce::jlimit(0.0f, 100.0f, ms); }
    void setGrainDecay(float ms) { grainDecay_ = juce::jlimit(0.0f, 100.0f, ms); }

    //==========================================================================
    // Granulation Parameters
    //==========================================================================

    /** Density (grains per second) */
    void setDensity(float grainsPerSec) { density_ = juce::jlimit(1.0f, 100.0f, grainsPerSec); }

    /** Random position spread */
    void setRandomSpread(float spread) { randomSpread_ = juce::jlimit(0.0f, 1.0f, spread); }

    /** Pitch variation (semitones) */
    void setPitchVariation(float semitones) { pitchVar_ = semitones; }

    /** Freeze mode (hold current grains) */
    void setFreeze(bool freeze) { freeze_ = freeze; }

    //==========================================================================
    // Stereo Options
    //==========================================================================

    /** Stereo spread (0-1) */
    void setStereoSpread(float spread) { stereoSpread_ = juce::jlimit(0.0f, 1.0f, spread); }

    //==========================================================================
    // Audio Generation
    //==========================================================================

    /**
     * @brief Generate next sample
     * @param frequency Playback frequency multiplier
     * @return Stereo sample value
     */
    void getNextSample(float frequency, float& left, float& right);

    /**
     * @brief Process block
     */
    void process(float* left, float* right, int numSamples, float frequency = 1.0f);

    //==========================================================================
    // State Query
    //==========================================================================

    bool hasSource() const { return !sourceData_.empty(); }

private:
    //==========================================================================
    // Source Data
    //==========================================================================

    std::vector<float> sourceData_;     // Stereo interleaved
    int sourceSamples_ = 0;
    double sourceRate_ = 44100.0;

    //==========================================================================
    // Grain State
    //==========================================================================

    struct Grain {
        double position = 0.0;        // Current position in source
        double phase = 0.0;            // Envelope phase (0-1)
        float pan = 0.0f;               // Stereo pan (-1 to 1)
        float pitch = 0.0f;              // Pitch offset (semitones)
        float speed = 1.0f;              // Playback speed
        bool active = false;
    };

    static constexpr int MAX_GRAINS = 32;
    std::array<Grain, MAX_GRAINS> grains_;
    int nextGrainIndex_ = 0;

    //==========================================================================
    // Parameters
    //==========================================================================

    float grainSize_ = 20.0f;         // ms
    EnvelopeShape envShape_ = EnvelopeShape::Bell;
    float grainAttack_ = 5.0f;        // ms
    float grainDecay_ = 30.0f;        // ms
    float density_ = 10.0f;            // grains/sec
    float randomSpread_ = 0.5f;         // 0-1
    float pitchVar_ = 0.0f;             // semitones
    bool freeze_ = false;
    float stereoSpread_ = 0.5f;        // 0-1

    //==========================================================================
    // Timing
    //==========================================================================

    double sampleRate_ = 44100.0;
    double timeSinceLastGrain_ = 0.0;

    //==========================================================================
    // Random
    //==========================================================================

    juce::Random random_;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void spawnGrain();
    float getGrainEnvelope(double phase, EnvelopeShape shape);
    float readSourceStereo(double pos, float pan, float& outL, float& outR);
};

} // namespace zenith
