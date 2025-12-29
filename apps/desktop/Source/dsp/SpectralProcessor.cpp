/*
  ==============================================================================

    SpectralProcessor.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#include "SpectralProcessor.h"
#include <cmath>
#include <algorithm>

namespace zenith {
namespace dsp {

SpectralProcessor::SpectralProcessor() {
    fftBuffer_.resize(fftSize_, {0.0f, 0.0f});
    windowBuffer_.resize(fftSize_, 0.0f);
}

SpectralProcessor::~SpectralProcessor() {}

//==============================================================================
// Noise Profile Capture
//==============================================================================

void SpectralProcessor::captureNoiseProfile(const juce::AudioBuffer<float>& audio,
                                           int startSample, int endSample,
                                           double sampleRate) {
    if (startSample >= endSample || audio.getNumSamples() == 0)
        return;
    
    startSample = std::max(0, startSample);
    endSample = std::min(audio.getNumSamples(), endSample);
    
    int numBins = fftSize_ / 2 + 1;
    noiseProfile_.assign(numBins, 0.0f);
    noiseProfileSampleRate_ = sampleRate;
    
    const float* data = audio.getReadPointer(0);
    int numFrames = 0;
    
    // Analyze overlapping windows and accumulate magnitude
    for (int pos = startSample; pos + fftSize_ <= endSample; pos += hopSize_) {
        // Copy and window the frame
        for (int i = 0; i < fftSize_; ++i) {
            windowBuffer_[i] = data[pos + i];
        }
        window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
        
        // Prepare FFT buffer (interleaved complex)
        for (int i = 0; i < fftSize_; ++i) {
            fftBuffer_[i] = {windowBuffer_[i], 0.0f};
        }
        
        // Perform FFT
        fft_.perform(fftBuffer_.data(), fftBuffer_.data(), false);
        
        // Accumulate magnitudes
        for (int k = 0; k < numBins; ++k) {
            float mag = std::abs(fftBuffer_[k]);
            noiseProfile_[k] += mag;
        }
        
        numFrames++;
    }
    
    // Average the accumulated magnitudes
    if (numFrames > 0) {
        float invFrames = 1.0f / static_cast<float>(numFrames);
        for (int k = 0; k < numBins; ++k) {
            noiseProfile_[k] *= invFrames;
        }
    }
    
    DBG("[SpectralProcessor] Captured noise profile from " + 
        juce::String(numFrames) + " frames, " + 
        juce::String(numBins) + " bins");
}

//==============================================================================
// Noise Reduction via Spectral Subtraction
//==============================================================================

void SpectralProcessor::applyNoiseReduction(juce::AudioBuffer<float>& audio,
                                           int startSample, int endSample,
                                           float strength,
                                           float thresholdDb) {
    if (!hasNoiseProfile()) {
        DBG("[SpectralProcessor] No noise profile captured");
        return;
    }
    
    startSample = std::max(0, startSample);
    endSample = std::min(audio.getNumSamples(), endSample);
    
    int numBins = fftSize_ / 2 + 1;
    float thresholdLinear = std::pow(10.0f, thresholdDb / 20.0f);
    
    // Process each channel
    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float* data = audio.getWritePointer(ch);
        
        // Output accumulation buffer
        std::vector<float> outputBuffer(endSample - startSample, 0.0f);
        std::vector<float> windowSum(endSample - startSample, 0.0f);
        
        // STFT analysis-synthesis
        for (int pos = startSample; pos + fftSize_ <= endSample; pos += hopSize_) {
            int outOffset = pos - startSample;
            
            // Copy and window the frame
            for (int i = 0; i < fftSize_; ++i) {
                windowBuffer_[i] = data[pos + i];
            }
            window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
            
            // Prepare FFT buffer
            for (int i = 0; i < fftSize_; ++i) {
                fftBuffer_[i] = {windowBuffer_[i], 0.0f};
            }
            
            // Forward FFT
            fft_.perform(fftBuffer_.data(), fftBuffer_.data(), false);
            
            // Spectral subtraction
            for (int k = 0; k < numBins; ++k) {
                float real = fftBuffer_[k].real();
                float imag = fftBuffer_[k].imag();
                float mag = std::abs(fftBuffer_[k]);
                float phase = std::arg(fftBuffer_[k]);
                
                // Subtract noise magnitude with over-subtraction factor
                float noiseMag = noiseProfile_[k] * strength;
                float newMag = mag - noiseMag;
                
                // Spectral flooring (prevent musical noise)
                float floor = thresholdLinear * noiseProfile_[k];
                newMag = std::max(newMag, floor);
                
                // Wiener filtering for smoother results
                if (mag > 1e-10f) {
                    float gain = newMag / mag;
                    gain = std::max(0.0f, std::min(1.0f, gain));
                    newMag = mag * gain;
                }
                
                // Reconstruct complex
                fftBuffer_[k] = std::polar(newMag, phase);
                
                // Mirror for IFFT
                if (k > 0 && k < fftSize_ / 2) {
                    int mirrorK = fftSize_ - k;
                    fftBuffer_[mirrorK] = std::conj(fftBuffer_[k]);
                }
            }
            
            // Inverse FFT
            fft_.perform(fftBuffer_.data(), fftBuffer_.data(), true);
            
            // Extract real part and apply synthesis window
            for (int i = 0; i < fftSize_; ++i) {
                windowBuffer_[i] = fftBuffer_[i].real() / static_cast<float>(fftSize_);
            }
            window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
            
            // Overlap-add
            for (int i = 0; i < fftSize_; ++i) {
                int outIdx = outOffset + i;
                if (outIdx >= 0 && outIdx < static_cast<int>(outputBuffer.size())) {
                    outputBuffer[outIdx] += windowBuffer_[i];
                    // Accumulate window sum for normalization
                    float w = 1.0f; // Hann window squared sums to ~1.5 for 4x overlap
                    windowSum[outIdx] += w;
                }
            }
        }
        
        // Normalize and copy back
        for (int i = 0; i < static_cast<int>(outputBuffer.size()); ++i) {
            if (windowSum[i] > 0.01f) {
                data[startSample + i] = outputBuffer[i] / (windowSum[i] * 0.375f);
            }
        }
    }
    
    DBG("[SpectralProcessor] Applied noise reduction (strength=" + 
        juce::String(strength) + ", threshold=" + juce::String(thresholdDb) + "dB)");
}

//==============================================================================
// Frequency Band Editing
//==============================================================================

void SpectralProcessor::applyFrequencyGain(juce::AudioBuffer<float>& audio,
                                          int startSample, int endSample,
                                          float lowHz, float highHz,
                                          float gainDb,
                                          double sampleRate) {
    startSample = std::max(0, startSample);
    endSample = std::min(audio.getNumSamples(), endSample);
    
    int lowBin = frequencyToBin(lowHz, sampleRate);
    int highBin = frequencyToBin(highHz, sampleRate);
    lowBin = std::max(0, lowBin);
    highBin = std::min(fftSize_ / 2, highBin);
    
    float gainLinear = std::pow(10.0f, gainDb / 20.0f);
    
    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float* data = audio.getWritePointer(ch);
        
        std::vector<float> outputBuffer(endSample - startSample, 0.0f);
        std::vector<float> windowSum(endSample - startSample, 0.0f);
        
        for (int pos = startSample; pos + fftSize_ <= endSample; pos += hopSize_) {
            int outOffset = pos - startSample;
            
            // Copy and window
            for (int i = 0; i < fftSize_; ++i) {
                windowBuffer_[i] = data[pos + i];
            }
            window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
            
            // FFT
            for (int i = 0; i < fftSize_; ++i) {
                fftBuffer_[i] = {windowBuffer_[i], 0.0f};
            }
            
            fft_.perform(fftBuffer_.data(), fftBuffer_.data(), false);
            
            // Apply gain to selected frequency range
            for (int k = lowBin; k <= highBin; ++k) {
                fftBuffer_[k] *= gainLinear;
                
                // Mirror
                if (k > 0 && k < fftSize_ / 2) {
                    int mirrorK = fftSize_ - k;
                    fftBuffer_[mirrorK] = std::conj(fftBuffer_[k]);
                }
            }
            
            // IFFT
            fft_.perform(fftBuffer_.data(), fftBuffer_.data(), true);
            
            for (int i = 0; i < fftSize_; ++i) {
                windowBuffer_[i] = fftBuffer_[i].real() / static_cast<float>(fftSize_);
            }
            window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
            
            // Overlap-add
            for (int i = 0; i < fftSize_; ++i) {
                int outIdx = outOffset + i;
                if (outIdx >= 0 && outIdx < static_cast<int>(outputBuffer.size())) {
                    outputBuffer[outIdx] += windowBuffer_[i];
                    windowSum[outIdx] += 1.0f;
                }
            }
        }
        
        // Normalize and copy back
        for (int i = 0; i < static_cast<int>(outputBuffer.size()); ++i) {
            if (windowSum[i] > 0.01f) {
                data[startSample + i] = outputBuffer[i] / (windowSum[i] * 0.375f);
            }
        }
    }
    
    DBG("[SpectralProcessor] Applied frequency gain: " + juce::String(lowHz) + 
        "-" + juce::String(highHz) + "Hz @ " + juce::String(gainDb) + "dB");
}

void SpectralProcessor::applySpectralGate(juce::AudioBuffer<float>& audio,
                                         int startSample, int endSample,
                                         float thresholdDb) {
    startSample = std::max(0, startSample);
    endSample = std::min(audio.getNumSamples(), endSample);
    
    float thresholdLinear = std::pow(10.0f, thresholdDb / 20.0f);
    int numBins = fftSize_ / 2 + 1;
    
    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float* data = audio.getWritePointer(ch);
        
        std::vector<float> outputBuffer(endSample - startSample, 0.0f);
        std::vector<float> windowSum(endSample - startSample, 0.0f);
        
        for (int pos = startSample; pos + fftSize_ <= endSample; pos += hopSize_) {
            int outOffset = pos - startSample;
            
            for (int i = 0; i < fftSize_; ++i) {
                windowBuffer_[i] = data[pos + i];
            }
            window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
            
            for (int i = 0; i < fftSize_; ++i) {
                fftBuffer_[i] = {windowBuffer_[i], 0.0f};
            }
            
            fft_.perform(fftBuffer_.data(), fftBuffer_.data(), false);
            
            // Gate bins below threshold
            for (int k = 0; k < numBins; ++k) {
                float mag = std::abs(fftBuffer_[k]);
                
                if (mag < thresholdLinear) {
                    fftBuffer_[k] = {0.0f, 0.0f};
                }
                
                if (k > 0 && k < fftSize_ / 2) {
                    int mirrorK = fftSize_ - k;
                    fftBuffer_[mirrorK] = std::conj(fftBuffer_[k]);
                }
            }
            
            fft_.perform(fftBuffer_.data(), fftBuffer_.data(), true);
            
            for (int i = 0; i < fftSize_; ++i) {
                windowBuffer_[i] = fftBuffer_[i].real() / static_cast<float>(fftSize_);
            }
            window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
            
            for (int i = 0; i < fftSize_; ++i) {
                int outIdx = outOffset + i;
                if (outIdx >= 0 && outIdx < static_cast<int>(outputBuffer.size())) {
                    outputBuffer[outIdx] += windowBuffer_[i];
                    windowSum[outIdx] += 1.0f;
                }
            }
        }
        
        for (int i = 0; i < static_cast<int>(outputBuffer.size()); ++i) {
            if (windowSum[i] > 0.01f) {
                data[startSample + i] = outputBuffer[i] / (windowSum[i] * 0.375f);
            }
        }
    }
    
    DBG("[SpectralProcessor] Applied spectral gate at " + juce::String(thresholdDb) + "dB");
}

//==============================================================================
// Filters
//==============================================================================

void SpectralProcessor::applyHighPassFilter(juce::AudioBuffer<float>& audio,
                                           int startSample, int endSample,
                                           float cutoffHz, double sampleRate) {
    // Zero out bins below cutoff
    applyFrequencyGain(audio, startSample, endSample, 0.0f, cutoffHz, -96.0f, sampleRate);
}

void SpectralProcessor::applyLowPassFilter(juce::AudioBuffer<float>& audio,
                                          int startSample, int endSample,
                                          float cutoffHz, double sampleRate) {
    // Zero out bins above cutoff (up to Nyquist)
    float nyquist = static_cast<float>(sampleRate / 2.0);
    applyFrequencyGain(audio, startSample, endSample, cutoffHz, nyquist, -96.0f, sampleRate);
}

void SpectralProcessor::applyBandPassFilter(juce::AudioBuffer<float>& audio,
                                           int startSample, int endSample,
                                           float lowHz, float highHz,
                                           double sampleRate) {
    float nyquist = static_cast<float>(sampleRate / 2.0);
    // Remove below low and above high
    applyFrequencyGain(audio, startSample, endSample, 0.0f, lowHz, -96.0f, sampleRate);
    applyFrequencyGain(audio, startSample, endSample, highHz, nyquist, -96.0f, sampleRate);
}

//==============================================================================
// Spectral Blur
//==============================================================================

void SpectralProcessor::applySpectralBlur(juce::AudioBuffer<float>& audio,
                                         int startSample, int endSample,
                                         float amount) {
    startSample = std::max(0, startSample);
    endSample = std::min(audio.getNumSamples(), endSample);
    
    int numBins = fftSize_ / 2 + 1;
    int blurWidth = static_cast<int>(amount * 32.0f) + 1; // 1 to 33 bins wide
    
    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float* data = audio.getWritePointer(ch);
        
        std::vector<float> outputBuffer(endSample - startSample, 0.0f);
        std::vector<float> windowSum(endSample - startSample, 0.0f);
        
        for (int pos = startSample; pos + fftSize_ <= endSample; pos += hopSize_) {
            int outOffset = pos - startSample;
            
            for (int i = 0; i < fftSize_; ++i) {
                windowBuffer_[i] = data[pos + i];
            }
            window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
            
            for (int i = 0; i < fftSize_; ++i) {
                fftBuffer_[i] = {windowBuffer_[i], 0.0f};
            }
            
            fft_.perform(fftBuffer_.data(), fftBuffer_.data(), false);
            
            // Blur magnitudes by averaging with neighbors
            std::vector<float> blurredMags(numBins);
            std::vector<float> originalPhases(numBins);
            
            for (int k = 0; k < numBins; ++k) {
                originalPhases[k] = std::arg(fftBuffer_[k]);
                
                // Average magnitude over blur window
                float sumMag = 0.0f;
                int count = 0;
                for (int j = -blurWidth; j <= blurWidth; ++j) {
                    int idx = k + j;
                    if (idx >= 0 && idx < numBins) {
                        sumMag += std::abs(fftBuffer_[idx]);
                        count++;
                    }
                }
                blurredMags[k] = sumMag / count;
            }
            
            // Reconstruct with blurred magnitudes
            for (int k = 0; k < numBins; ++k) {
                fftBuffer_[k] = std::polar(blurredMags[k], originalPhases[k]);
                
                if (k > 0 && k < fftSize_ / 2) {
                    int mirrorK = fftSize_ - k;
                    fftBuffer_[mirrorK] = std::conj(fftBuffer_[k]);
                }
            }
            
            fft_.perform(fftBuffer_.data(), fftBuffer_.data(), true);
            
            for (int i = 0; i < fftSize_; ++i) {
                windowBuffer_[i] = fftBuffer_[i].real() / static_cast<float>(fftSize_);
            }
            window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
            
            for (int i = 0; i < fftSize_; ++i) {
                int outIdx = outOffset + i;
                if (outIdx >= 0 && outIdx < static_cast<int>(outputBuffer.size())) {
                    outputBuffer[outIdx] += windowBuffer_[i];
                    windowSum[outIdx] += 1.0f;
                }
            }
        }
        
        for (int i = 0; i < static_cast<int>(outputBuffer.size()); ++i) {
            if (windowSum[i] > 0.01f) {
                data[startSample + i] = outputBuffer[i] / (windowSum[i] * 0.375f);
            }
        }
    }
    
    DBG("[SpectralProcessor] Applied spectral blur (amount=" + juce::String(amount) + ")");
}

//==============================================================================
// Spectrogram Computation
//==============================================================================

std::vector<std::vector<float>> SpectralProcessor::computeSpectrogram(
    const juce::AudioBuffer<float>& audio,
    int startSample, int endSample,
    int numTimeBins,
    double sampleRate) {
    
    juce::ignoreUnused(sampleRate);
    
    startSample = std::max(0, startSample);
    endSample = std::min(audio.getNumSamples(), endSample);
    
    int numFreqBins = fftSize_ / 2 + 1;
    int totalSamples = endSample - startSample;
    int hop = std::max(1, totalSamples / numTimeBins);
    
    std::vector<std::vector<float>> spectrogram(numTimeBins, std::vector<float>(numFreqBins, 0.0f));
    
    if (audio.getNumChannels() == 0 || totalSamples < fftSize_)
        return spectrogram;
    
    const float* data = audio.getReadPointer(0);
    
    for (int t = 0; t < numTimeBins; ++t) {
        int pos = startSample + t * hop;
        if (pos + fftSize_ > endSample)
            break;
        
        // Copy and window
        for (int i = 0; i < fftSize_; ++i) {
            windowBuffer_[i] = data[pos + i];
        }
        window_.multiplyWithWindowingTable(windowBuffer_.data(), fftSize_);
        
        // FFT
        for (int i = 0; i < fftSize_; ++i) {
            fftBuffer_[i] = {windowBuffer_[i], 0.0f};
        }
        
        fft_.perform(fftBuffer_.data(), fftBuffer_.data(), false);
        
        // Extract magnitudes
        for (int k = 0; k < numFreqBins; ++k) {
            float mag = std::abs(fftBuffer_[k]) / static_cast<float>(fftSize_);
            
            // Convert to dB and normalize to 0-1 range
            float db = 20.0f * std::log10(mag + 1e-10f);
            float normalized = (db + 80.0f) / 80.0f; // -80dB to 0dB -> 0 to 1
            spectrogram[t][k] = juce::jlimit(0.0f, 1.0f, normalized);
        }
    }
    
    return spectrogram;
}

} // namespace dsp
} // namespace zenith
