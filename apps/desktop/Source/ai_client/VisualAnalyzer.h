/*
  ==============================================================================
    VisualAnalyzer.h
    Multi-modal audio analysis with visual waveform and spectral heatmaps
    Phase 2: Context-Aware AI (9/10)
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

namespace zenith {
namespace ai {

struct WaveformFeatures {
    std::vector<float> peaks;           // Peak positions
    std::vector<float> troughs;         // Trough positions  
    std::vector<float> rmsEnvelope;     // RMS envelope
    std::vector<float> zeroCrossings;   // Zero crossing rate
    float dynamicRange;                 // Peak-to-RMS ratio
    float crestFactor;                  // Peak to average ratio
    float attackTime;                   // Attack estimation
    float decayTime;                    // Decay estimation
};

struct SpectralHeatmap {
    std::vector<std::vector<float>> frequencyBands;  // [time][frequency]
    std::vector<float> timeAxis;                     // Time positions
    std::vector<float> frequencyAxis;                 // Frequency bins
    float sampleRate;
    int fftSize;
    int hopSize;
};

struct SpectralFeatures {
    float spectralCentroid = 0.0f;      // Center of spectral mass
    float spectralSpread = 0.0f;        // Spectral bandwidth
    float spectralRolloff = 0.0f;       // Frequency below which 85% of energy is contained
    float spectralFlatness = 0.0f;      // Measure of noise-like vs tonal
    float spectralFlux = 0.0f;          // Rate of spectral change
    std::vector<float> mfcc;            // Mel-frequency cepstral coefficients
};

struct VisualAnalysisResult {
    WaveformFeatures waveform;
    SpectralHeatmap heatmap;
    SpectralFeatures spectral;          // Added to fix AI file dependencies
    juce::String visualDescription;  // AI-readable description
    juce::var visualFeatures;         // Structured data for AI
};

class VisualAnalyzer {
public:
    VisualAnalyzer();
    ~VisualAnalyzer();

    // Main analysis entry point
    VisualAnalysisResult analyzeAudio(const juce::AudioBuffer<float>& audio,
                                     double sampleRate);

    // Individual analysis components
    WaveformFeatures extractWaveformFeatures(const juce::AudioBuffer<float>& audio,
                                            double sampleRate);
    
    SpectralHeatmap generateSpectralHeatmap(const juce::AudioBuffer<float>& audio,
                                           double sampleRate);

    // AI integration
    juce::String generateVisualDescription(const VisualAnalysisResult& result);
    juce::var extractVisualFeatures(const VisualAnalysisResult& result);

    // Configuration
    void setAnalysisResolution(int fftSize = 2048, int hopSize = 512);
    void setFrequencyRange(float minFreq = 20.0f, float maxFreq = 20000.0f);
    
    // Spectral utility (public for GenreDetector etc.)
    std::vector<float> computeSpectrum(const float* channelData, int numSamples);

private:
    // FFT processing
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    
    // Analysis parameters
    int fftSize = 2048;
    int hopSize = 512;
    float minFrequency = 20.0f;
    float maxFrequency = 20000.0f;
    
    // Helper methods
    std::vector<float> computeRMSEnvelope(const juce::AudioBuffer<float>& audio);
    std::vector<float> findPeaks(const std::vector<float>& signal, float threshold = 0.1f);
    std::vector<float> findTroughs(const std::vector<float>& signal, float threshold = 0.1f);
    std::vector<float> computeZeroCrossingRate(const juce::AudioBuffer<float>& audio);
    
    float estimateAttackTime(const std::vector<float>& rmsEnvelope, double sampleRate);
    float estimateDecayTime(const std::vector<float>& rmsEnvelope, double sampleRate);
    
    // Spectral analysis
    std::vector<std::vector<float>> createHeatmapData(const juce::AudioBuffer<float>& audio,
                                                      double sampleRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualAnalyzer)
};

} // namespace ai
} // namespace zenith
