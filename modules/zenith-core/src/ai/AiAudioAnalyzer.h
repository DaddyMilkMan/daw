/*
  ==============================================================================

    AiAudioAnalyzer.h
    Created: 2025-11-30
    Authors: Dr. Aris Vokos (DSP Lead)

    FEATURE 3: Audio Feature Extraction
    Real spectral analysis using FFT.

  ==============================================================================
*/

#pragma once

#include "AiDataStructures.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {
namespace ai {

class AudioAnalyzer {
public:
    /**
     * Extracts audio features from a buffer.
     * Uses real FFT for spectral analysis.
     */
    static AudioFeatures analyzeAudio(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        AudioFeatures features;
        
        if (buffer.getNumSamples() == 0) {
            return features;
        }
        
        // Calculate RMS energy
        features.averageEnergy = calculateRMS(buffer);
        
        // Calculate spectral centroid
        features.spectralCentroid = calculateSpectralCentroid(buffer, sampleRate);
        
        // Detect tempo
        features.tempo = detectTempo(buffer, sampleRate);
        
        // Detect onsets
        features.onsets = detectOnsets(buffer, sampleRate);
        
        return features;
    }

private:
    static float calculateRMS(const juce::AudioBuffer<float>& buffer) {
        float sumSquares = 0.0f;
        int totalSamples = 0;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                sumSquares += data[i] * data[i];
                totalSamples++;
            }
        }
        
        return std::sqrt(sumSquares / totalSamples);
    }
    
    static float calculateSpectralCentroid(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        // Use first channel for analysis
        if (buffer.getNumChannels() == 0) return 0.0f;
        
        const int fftSize = 2048;
        juce::dsp::FFT fft(11); // 2^11 = 2048
        
        std::vector<float> fftData(fftSize * 2, 0.0f);
        
        // Copy audio data
        int samplesToCopy = std::min(fftSize, buffer.getNumSamples());
        for (int i = 0; i < samplesToCopy; ++i) {
            fftData[i] = buffer.getSample(0, i);
        }
        
        // Apply Hann window
        for (int i = 0; i < fftSize; ++i) {
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize));
            fftData[i] *= window;
        }
        
        // Perform FFT
        fft.performFrequencyOnlyForwardTransform(fftData.data());
        
        // Calculate centroid
        float numerator = 0.0f;
        float denominator = 0.0f;
        
        for (int i = 0; i < fftSize / 2; ++i) {
            float magnitude = fftData[i];
            float frequency = (i * sampleRate) / fftSize;
            
            numerator += frequency * magnitude;
            denominator += magnitude;
        }
        
        return denominator > 0.0f ? numerator / denominator : 0.0f;
    }
    
    static float detectTempo(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        // Simplified tempo detection using autocorrelation
        // Real implementation would use onset detection + autocorrelation
        
        const int hopSize = 512;
        const int numHops = buffer.getNumSamples() / hopSize;
        
        std::vector<float> onsetStrength(numHops, 0.0f);
        
        // Calculate onset strength envelope
        for (int hop = 0; hop < numHops - 1; ++hop) {
            int startSample = hop * hopSize;
            float energy = 0.0f;
            
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < hopSize && (startSample + i) < buffer.getNumSamples(); ++i) {
                    energy += std::abs(data[startSample + i]);
                }
            }
            
            onsetStrength[hop] = energy;
        }
        
        // Find peaks in onset strength
        std::vector<int> peaks;
        for (int i = 1; i < numHops - 1; ++i) {
            if (onsetStrength[i] > onsetStrength[i-1] && 
                onsetStrength[i] > onsetStrength[i+1] &&
                onsetStrength[i] > 0.1f) {
                peaks.push_back(i);
            }
        }
        
        // Calculate average interval between peaks
        if (peaks.size() < 2) {
            return 120.0f; // Default
        }
        
        float avgInterval = 0.0f;
        for (size_t i = 1; i < peaks.size(); ++i) {
            avgInterval += (peaks[i] - peaks[i-1]);
        }
        avgInterval /= (peaks.size() - 1);
        
        // Convert to BPM
        float intervalSeconds = (avgInterval * hopSize) / sampleRate;
        float bpm = 60.0f / intervalSeconds;
        
        // Clamp to reasonable range
        return juce::jlimit(60.0f, 200.0f, bpm);
    }
    
    static std::vector<double> detectOnsets(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        std::vector<double> onsets;
        
        const int hopSize = 512;
        const int numHops = buffer.getNumSamples() / hopSize;
        const float threshold = 0.3f;
        
        float prevEnergy = 0.0f;
        
        for (int hop = 0; hop < numHops; ++hop) {
            int startSample = hop * hopSize;
            float energy = 0.0f;
            
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < hopSize && (startSample + i) < buffer.getNumSamples(); ++i) {
                    energy += data[startSample + i] * data[startSample + i];
                }
            }
            
            energy = std::sqrt(energy / hopSize);
            
            // Detect onset if energy increase is significant
            if (energy > prevEnergy * 1.5f && energy > threshold) {
                double timeSeconds = (hop * hopSize) / sampleRate;
                onsets.push_back(timeSeconds);
            }
            
            prevEnergy = energy;
        }
        
        return onsets;
    }
};

} // namespace ai
} // namespace zenith
