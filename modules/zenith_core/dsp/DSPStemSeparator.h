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

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {

class DSPStemSeparator {
public:
    enum class StemType {
        Vocals, // Mid channel > 200Hz
        Bass,   // Mid channel < 200Hz
        Drums,  // Transient heavy (simulated via EQ)
        Other   // Side channel (Stereo width)
    };

    DSPStemSeparator();
    ~DSPStemSeparator();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    /**
     * @brief Processes a block of audio and extracts the requested stem.
     * @param inputBlock Input audio buffer
     * @param outputBlock Output audio buffer (will be overwritten)
     * @param stemType The type of stem to extract
     */
    void process(const juce::dsp::AudioBlock<const float>& inputBlock,
                 juce::dsp::AudioBlock<float>& outputBlock,
                 StemType stemType);

private:
    // Filter chains for crossover
    using Filter = juce::dsp::IIR::Filter<float>;
    using FilterChain = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>; // 4th order

    FilterChain lowPassFilter_;
    FilterChain highPassFilter_;

    double sampleRate_ = 44100.0;
    
    // Helper to calculate Mid-Side
    void computeMidSide(const float* left, const float* right, int numSamples, 
                       std::vector<float>& mid, std::vector<float>& side);
};

} // namespace zenith
