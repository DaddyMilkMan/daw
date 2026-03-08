/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
// ALGORITHMIC REVERB (Schroeder/Moorer)
//==============================================================================

/**
 * @class AlgorithmicReverb
 * @brief Professional algorithmic reverb using Schroeder/Moorer design
 *
 * Features:
 * - 8 parallel comb filters for reverb tail
 * - 4 series allpass filters for diffusion
 * - Pre-delay section
 * - Frequency-dependent damping
 *
 * Uses pimpl pattern to hide implementation details.
 */
class AlgorithmicReverb
{
public:
    AlgorithmicReverb();
    ~AlgorithmicReverb();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float wetLevel);

    // Parameters
    void setRoomSize(float size);
    void setDamping(float damping);
    void setWetLevel(float wet);
    void setDecayTime(float decay);
    void setPreDelay(float predelay);
    void setDiffusion(float diffusion);
    void setModulation(float mod);

    juce::String getName() const { return "Algorithmic Reverb"; }

private:
    // Pimpl pattern to hide implementation
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmicReverb)
};

} // namespace zenith
