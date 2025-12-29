/*
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
        std::vector<std::complex<float>> analysisBuffer;
        std::vector<std::complex<float>> synthesisBuffer;
        
        ChannelState() {
            lastInputPhase.resize(fftSize / 2 + 1, 0.0f);
            lastOutputPhase.resize(fftSize / 2 + 1, 0.0f);
            analysisBuffer.resize(fftSize, {0.0f, 0.0f});
            synthesisBuffer.resize(fftSize, {0.0f, 0.0f});
        }
    };

    void processChannel(int channel, const float* input, float* output, int numInputSamples, int numOutputSamples, float ratio);
};

} // namespace dsp
} // namespace zenith
