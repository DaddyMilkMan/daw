#pragma once

#include "modules/zenith_core/dsp/RealTimeAudioBuffer.h"
#include "modules/zenith_core/engine/AudioConstants.h"
#include <vector>
#include <complex>
#include <mutex>

namespace Zenith {

class AudioFFTAnalyzer {
public:
    struct FrequencyBand {
        float bass;        // 20-250 Hz
        float lowMid;      // 250-2000 Hz
        float highMid;     // 2000-4000 Hz
        float treble;      // 4000-20000 Hz
        float fullSpectrum; // 20-20000 Hz

        // Peak detection
        float peakFrequency;
        float peakAmplitude;
        float peakDecay; // Smoothed decay factor
    };

    struct AudioFeatures {
        FrequencyBand frequencyBands;
        float rmsLevel;     // Root mean square level
        float peakLevel;    // Peak level
        float crestFactor;  // Peak/RMS ratio
        float spectralCentroid; // Brightness measure
        float spectralSpread;  // Width of spectrum
        float zeroCrossingRate; // Frequency content measure

        // Smoothed values for visual smoothing
        float smoothedBass;
        float smoothedMid;
        float smoothedTreble;
        float smoothedPeak;
    };

    AudioFFTAnalyzer();
    ~AudioFFTAnalyzer();

    // Configuration
    void initialize(int sampleRate, int fftSize = 2048);
    void setSmoothingTime(float milliseconds);
    void setDecayRate(float decayPerSecond);

    // Processing
    void processAudio(const float* audioData, int numSamples);
    const AudioFeatures& getAudioFeatures() const { return currentFeatures; }

    // Frequency domain access
    const std::vector<float>& getFrequencies() const { return magnitudeSpectrum; }
    const std::vector<float>& getPhases() const { return phaseSpectrum; }

    // Analysis
    void analyzeFrequencyBands();
    void detectPeaks();
    void computeSpectralFeatures();

    // Visual triggers
    float getBassEnergy() const { return currentFeatures.frequencyBands.bass; }
    float getMidEnergy() const { return currentFeatures.frequencyBands.lowMid + currentFeatures.frequencyBands.highMid; }
    float getTrebleEnergy() const { return currentFeatures.frequencyBands.treble; }
    float getPeakFrequency() const { return currentFeatures.frequencyBands.peakFrequency; }

    // Reset
    void reset();
    void clearHistory();

private:
    // FFT processing
    void performFFT(const float* audioData);
    void computeWindowFunction();
    void applyWindowFunction(float* buffer) const;

    // Feature extraction
    void computeRMS(const float* audioData, int numSamples);
    void computePeakDetection();
    void computeSpectralCentroid();
    void computeSpectralSpread();
    void computeZeroCrossingRate(const float* audioData, int numSamples);

    // Smoothing
    void applySmoothing();
    void applyDecay();

    // Members
    std::vector<std::complex<float>> fftBuffer;
    std::vector<float> windowFunction;
    std::vector<float> magnitudeSpectrum;
    std::vector<float> phaseSpectrum;

    AudioFeatures currentFeatures;
    AudioFeatures previousFeatures;

    // Configuration
    int sampleRate;
    int fftSize;
    int hopSize;
    int sampleCounter;

    // Smoothing parameters
    float smoothingCoeff;
    float smoothingTimeMs;
    float decayRate;

    // Peak detection
    float peakDetectorState;
    std::vector<float> peakHistory;

    // Thread safety
    mutable std::mutex featuresMutex;

    // Constants
    static constexpr float BASS_LOW = 20.0f;
    static constexpr float BASS_HIGH = 250.0f;
    static constexpr float LOWMID_LOW = 250.0f;
    static constexpr float LOWMID_HIGH = 2000.0f;
    static constexpr float HIGHMID_LOW = 2000.0f;
    static constexpr float HIGHMID_HIGH = 4000.0f;
    static constexpr float TREBLE_LOW = 4000.0f;
    static constexpr float TREBLE_HIGH = 20000.0f;
};

} // namespace Zenith