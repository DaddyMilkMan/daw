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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    TimeStretcher.h
    Created: 2025-12-25
    Author:  Zenith DAW

    High-quality Phase Vocoder for pitch-invariant time stretching.
    Uses 4x overlap STFT with phase locking for transient preservation.


  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <complex>

namespace zenith {
namespace dsp {

/**
 * @class TimeStretcher
 * @brief Offline Phase Vocoder for elastic audio processing.
 */
class TimeStretcher {
public:
    TimeStretcher();
    ~TimeStretcher();

    /**
     * @brief Process an audio buffer and return a time-stretched version.
     * 
     * @param input Input audio buffer
     * @param ratio Time stretch ratio (e.g. 2.0 = double duration/slower, 0.5 = half duration/faster)
     *              Pitch remains constant.
     * @return Stretched audio buffer
     */
    juce::AudioBuffer<float> process(const juce::AudioBuffer<float>& input, float ratio);

private:
    static constexpr int fftOrder = 11;         // 2048 point FFT
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = fftSize / 4; // 4x overlap (512 samples)

    juce::dsp::FFT fft_{fftOrder};
    juce::dsp::WindowingFunction<float> window_{fftSize, juce::dsp::WindowingFunction<float>::hann};

    // Internal processing state for a single channel
    struct ChannelState {
        std::vector<float> lastInputPhase;
        std::vector<float> lastOutputPhase;
        std::vector<float> analysisBuffer;
        std::vector<float> synthesisBuffer;
        
        ChannelState() {
            lastInputPhase.resize(fftSize / 2 + 1, 0.0f);
            lastOutputPhase.resize(fftSize / 2 + 1, 0.0f);
            analysisBuffer.resize(fftSize * 2, 0.0f);  // *2 for complex
            synthesisBuffer.resize(fftSize * 2, 0.0f);
        }
    };

    void processChannel(int channel, const float* input, float* output, int numInputSamples, int numOutputSamples, float ratio);
};

} // namespace dsp
} // namespace zenith
