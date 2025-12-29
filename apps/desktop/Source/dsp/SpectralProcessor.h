/*
  ==============================================================================

    SpectralProcessor.h
    Created: 2025-12-26
    Author:  Zenith DAW

    FFT-based spectral analysis and editing utilities for the Sample Editor.
    Provides noise reduction, frequency band editing, and spectral gating.

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
 * @class SpectralProcessor
 * @brief FFT-based spectral processing for sample editing.
 */
class SpectralProcessor {
public:
    SpectralProcessor();
    ~SpectralProcessor();

    //==============================================================================
    // Noise Reduction
    //==============================================================================
    
    /**
     * @brief Capture noise profile from an audio selection.
     * 
     * Analyzes the selection using overlapping FFT windows and stores
     * the average magnitude spectrum as a noise profile.
     * 
     * @param audio Source audio buffer
     * @param startSample Start sample of noise region
     * @param endSample End sample of noise region
     * @param sampleRate Audio sample rate
     */
    void captureNoiseProfile(const juce::AudioBuffer<float>& audio,
                            int startSample, int endSample,
                            double sampleRate);
    
    /**
     * @brief Apply spectral subtraction to remove noise.
     * 
     * Uses the captured noise profile to attenuate frequency bins
     * matching the noise characteristics.
     * 
     * @param audio Audio buffer to process (modified in-place)
     * @param startSample Start sample to process
     * @param endSample End sample to process
     * @param strength Noise reduction strength (0.0 to 2.0, default 1.0)
     * @param threshold Spectral gate threshold in dB (default -60dB)
     */
    void applyNoiseReduction(juce::AudioBuffer<float>& audio,
                            int startSample, int endSample,
                            float strength = 1.0f,
                            float thresholdDb = -60.0f);
    
    /**
     * @brief Check if a noise profile has been captured.
     */
    bool hasNoiseProfile() const { return !noiseProfile_.empty(); }
    
    /**
     * @brief Clear the captured noise profile.
     */
    void clearNoiseProfile() { noiseProfile_.clear(); }

    //==============================================================================
    // Frequency Band Editing
    //==============================================================================
    
    /**
     * @brief Apply gain to a specific frequency range.
     * 
     * @param audio Audio buffer to process (modified in-place)
     * @param startSample Start sample to process
     * @param endSample End sample to process
     * @param lowHz Low frequency bound
     * @param highHz High frequency bound  
     * @param gainDb Gain to apply in dB
     * @param sampleRate Audio sample rate
     */
    void applyFrequencyGain(juce::AudioBuffer<float>& audio,
                           int startSample, int endSample,
                           float lowHz, float highHz,
                           float gainDb,
                           double sampleRate);
    
    /**
     * @brief Apply a spectral gate (silence frequencies below threshold).
     * 
     * @param audio Audio buffer to process
     * @param startSample Start sample
     * @param endSample End sample
     * @param thresholdDb Gate threshold in dB
     */
    void applySpectralGate(juce::AudioBuffer<float>& audio,
                          int startSample, int endSample,
                          float thresholdDb);

    //==============================================================================
    // High-Pass / Low-Pass Filters (FFT-based brick-wall)
    //==============================================================================
    
    void applyHighPassFilter(juce::AudioBuffer<float>& audio,
                            int startSample, int endSample,
                            float cutoffHz, double sampleRate);
    
    void applyLowPassFilter(juce::AudioBuffer<float>& audio,
                           int startSample, int endSample,
                           float cutoffHz, double sampleRate);
    
    void applyBandPassFilter(juce::AudioBuffer<float>& audio,
                            int startSample, int endSample,
                            float lowHz, float highHz,
                            double sampleRate);

    //==============================================================================
    // Spectral Blur (Edison-style)
    //==============================================================================
    
    /**
     * @brief Apply spectral blur effect.
     * 
     * Smooths the frequency spectrum using a moving average,
     * creating a "smeared" effect useful for ambient/pad sounds.
     * 
     * @param audio Audio buffer to process
     * @param startSample Start sample
     * @param endSample End sample
     * @param amount Blur amount (0.0 to 1.0)
     */
    void applySpectralBlur(juce::AudioBuffer<float>& audio,
                          int startSample, int endSample,
                          float amount);

    //==============================================================================
    // Analysis
    //==============================================================================
    
    /**
     * @brief Compute spectrogram data for visualization.
     * 
     * @param audio Source audio
     * @param startSample Start sample
     * @param endSample End sample
     * @param numTimeBins Number of time slices
     * @param sampleRate Audio sample rate
     * @return 2D vector [time][frequency] of magnitude values (0.0-1.0)
     */
    std::vector<std::vector<float>> computeSpectrogram(
        const juce::AudioBuffer<float>& audio,
        int startSample, int endSample,
        int numTimeBins,
        double sampleRate);

private:
    static constexpr int fftOrder_ = 11;          // 2048 point FFT
    static constexpr int fftSize_ = 1 << fftOrder_;
    static constexpr int hopSize_ = fftSize_ / 4; // 4x overlap
    
    juce::dsp::FFT fft_{fftOrder_};
    juce::dsp::WindowingFunction<float> window_{fftSize_, 
        juce::dsp::WindowingFunction<float>::hann};
    
    // Noise profile storage (magnitude per bin)
    std::vector<float> noiseProfile_;
    double noiseProfileSampleRate_ = 44100.0;
    
    // Internal buffers
    std::vector<std::complex<float>> fftBuffer_;
    std::vector<float> windowBuffer_;
    
    // Helper methods
    void processSTFT(juce::AudioBuffer<float>& audio,
                    int startSample, int endSample,
                    std::function<void(float* magnitudes, float* phases, int numBins)> processor);
    
    int frequencyToBin(float frequency, double sampleRate) const {
        return static_cast<int>(frequency * fftSize_ / sampleRate);
    }
    
    float binToFrequency(int bin, double sampleRate) const {
        return static_cast<float>(bin) * sampleRate / fftSize_;
    }
};

} // namespace dsp
} // namespace zenith
