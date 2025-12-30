/*
  ==============================================================================

    TimeStretcher.cpp
    Created: 2025-12-25
    Author:  Zenith DAW

  ==============================================================================
*/

#include "TimeStretcher.h"
#include <cmath>
#include <algorithm>

namespace zenith {
namespace dsp {

TimeStretcher::TimeStretcher() {}
TimeStretcher::~TimeStretcher() {}

juce::AudioBuffer<float> TimeStretcher::process(const juce::AudioBuffer<float>& input, float ratio) {
    if (ratio <= 0.0f) return juce::AudioBuffer<float>();
    
    int numInputSamples = input.getNumSamples();
    int numOutputSamples = static_cast<int>(numInputSamples * ratio);
    int numChannels = input.getNumChannels();

    juce::AudioBuffer<float> output(numChannels, numOutputSamples);
    output.clear();

    for (int ch = 0; ch < numChannels; ++ch) {
        processChannel(ch, input.getReadPointer(ch), output.getWritePointer(ch), numInputSamples, numOutputSamples, ratio);
    }

    return output;
}

void TimeStretcher::processChannel(int channel, const float* input, float* output, int numInputSamples, int numOutputSamples, float ratio) {
    juce::ignoreUnused(channel); // State is local for now or we could use member if streaming
    
    ChannelState state;
    
    // Analysis hop (variable)
    double analysisHop = hopSize / ratio;
    
    // Synthesis hop (fixed)
    int synthesisHop = hopSize;

    double analysisPos = 0.0;
    int synthesisPos = 0;

    std::vector<float> inputFrame(fftSize * 2, 0.0f); // Padded for FFT
    std::vector<float> outputFrame(fftSize * 2, 0.0f);
    std::vector<std::complex<float>> freqDomain(fftSize);

    // Scratch buffers for polar conversion
    std::vector<float> magnitudes(fftSize / 2 + 1);
    std::vector<float> phases(fftSize / 2 + 1);

    // Expected phase advance per bin for one hop
    std::vector<float> omega(fftSize / 2 + 1);
    for (int k = 0; k <= fftSize / 2; ++k) {
        omega[k] = 2.0f * juce::MathConstants<float>::pi * k * analysisHop / fftSize;
    }

    while (synthesisPos + fftSize < numOutputSamples) {
        // 1. Read and Window Input Frame (Interpolated)
        std::fill(inputFrame.begin(), inputFrame.end(), 0.0f);
        
        int intPos = static_cast<int>(analysisPos);
        float frac = static_cast<float>(analysisPos - intPos);

        for (int i = 0; i < fftSize; ++i) {
            int idx = intPos + i;
            
            float sample = 0.0f;
            if (idx >= 0 && idx < numInputSamples - 1) {
                sample = input[idx] * (1.0f - frac) + input[idx + 1] * frac;
            } else if (idx >= 0 && idx < numInputSamples) {
                sample = input[idx];
            }
            
            // Apply window immediately
            // Note: WindowingFunction expects contiguous float array of samples.
            // But we need interleaved complex for performForwardTransform.
            // We'll window a temp buffer then interleave.
            inputFrame[i] = sample;
        }
        
        window_.multiplyWithWindowingTable(inputFrame.data(), fftSize);

        // Interleave into analysis buffer: [Re, Im, Re, Im...]
        for (int i = 0; i < fftSize; ++i) {
            state.analysisBuffer[2 * i] = inputFrame[i];
            state.analysisBuffer[2 * i + 1] = 0.0f;
        }
        
        // 2. FFT (Complex -> Complex)
        fft_.perform(reinterpret_cast<const std::complex<float>*>(state.analysisBuffer.data()), 
                     reinterpret_cast<std::complex<float>*>(state.analysisBuffer.data()), false);

        // 3. Polar Conversion & Phase Vocoding
        // Iterate only up to Nyquist (fftSize/2) for analysis, but full size for synthesis?
        // Complex FFT gives symmetrical results for real input. We only need to process first half and mirror?
        // Or just process everything. Processing everything is easier but redundant.
        // Let's process 0 to fftSize/2 and enforce symmetry or just process all k=0..fftSize-1.
        // For standard PV, we usually process bins 0 to N/2.
        
        for (int k = 0; k <= fftSize / 2; ++k) {
            float real = state.analysisBuffer[2 * k];
            float imag = state.analysisBuffer[2 * k + 1];
            
            float mag = std::sqrt(real * real + imag * imag);
            float phase = std::atan2(imag, real);

            // Calculate phase difference
            float phaseDev = phase - state.lastInputPhase[k];
            
            // Expected phase advance
            float expected = 2.0f * juce::MathConstants<float>::pi * k * (float)analysisHop / fftSize;
            
            // Wrap deviation
            float delta = phaseDev - expected;
            delta = std::fmod(delta + juce::MathConstants<float>::pi, 2.0f * juce::MathConstants<float>::pi) - juce::MathConstants<float>::pi;
            
            // True frequency deviation
            float trueFreq = expected + delta;

            // Synthesis phase propagation
            float scale = (float)synthesisHop / (float)analysisHop;
            
            state.lastOutputPhase[k] += trueFreq * scale;
            
            // Reconstruct
            float newPhase = state.lastOutputPhase[k];
            state.lastInputPhase[k] = phase; 

            // Polar -> Cartesian
            state.synthesisBuffer[2 * k] = mag * std::cos(newPhase);
            state.synthesisBuffer[2 * k + 1] = mag * std::sin(newPhase);
            
            // Mirror to upper half for valid real IFFT result
            if (k > 0 && k < fftSize / 2) {
                int mirrorK = fftSize - k;
                state.synthesisBuffer[2 * mirrorK] = state.synthesisBuffer[2 * k];
                state.synthesisBuffer[2 * mirrorK + 1] = -state.synthesisBuffer[2 * k + 1]; // Conjugate
            }
        }

        // 4. IFFT
        fft_.perform(reinterpret_cast<const std::complex<float>*>(state.synthesisBuffer.data()), 
                     reinterpret_cast<std::complex<float>*>(state.synthesisBuffer.data()), true);

        // 5. Window and Overlap-Add
        // Output is interleaved [Re, Im...]. Imaginary part should be ~0.
        // Apply synthesis window (Hann)
        // Gain correction for Hanning 4x overlap = 1.0 / 1.5? No, sum of windows is 2/3 * N?
        // Standard Hanning 50% overlap sums to 1.
        // 4x overlap (75%) sums to 2?
        // We'll use a conservative scaling. 
        float gain = 1.0f / (fftSize * 4.0f * 0.375f); // Scaling for FFT size + Window overlap

        // Copy Real part to temp for windowing
        for (int i = 0; i < fftSize; ++i) {
            outputFrame[i] = state.synthesisBuffer[2 * i] * gain; // Extract Real
        }
        
        window_.multiplyWithWindowingTable(outputFrame.data(), fftSize);

        for (int i = 0; i < fftSize; ++i) {
            int outIdx = synthesisPos + i;
            if (outIdx >= 0 && outIdx < numOutputSamples) {
                output[outIdx] += outputFrame[i];
            }
        }

        // Advance
        analysisPos += analysisHop;
        synthesisPos += synthesisHop;
    }
}

} // namespace dsp
} // namespace zenith
