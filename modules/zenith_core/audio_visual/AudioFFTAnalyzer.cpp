#include "AudioFFTAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace Zenith {

AudioFFTAnalyzer::AudioFFTAnalyzer()
    : sampleRate(44100)
    , fftSize(2048)
    , hopSize(512)
    , sampleCounter(0)
    , smoothingTimeMs(50.0f)
    , decayRate(0.3f)
    , smoothingCoeff(0.8f)
    , peakDetectorState(0.0f) {

    initialize(44100, 2048);
}

AudioFFTAnalyzer::~AudioFFTAnalyzer() {
    clearHistory();
}

void AudioFFTAnalyzer::initialize(int newSampleRate, int newFFTSize) {
    std::lock_guard<std::mutex> lock(featuresMutex);

    sampleRate = newSampleRate;
    fftSize = newFFTSize;
    hopSize = fftSize / 4; // 25% overlap for smooth transitions

    // Initialize FFT buffer
    fftBuffer.resize(fftSize);
    windowFunction.resize(fftSize);
    magnitudeSpectrum.resize(fftSize / 2 + 1);
    phaseSpectrum.resize(fftSize / 2 + 1);

    // Initialize peak history
    peakHistory.resize(fftSize / 2 + 1);
    std::fill(peakHistory.begin(), peakHistory.end(), 0.0f);

    // Compute window function (Hann window)
    computeWindowFunction();

    // Reset features
    reset();
}

void AudioFFTAnalyzer::setSmoothingTime(float milliseconds) {
    std::lock_guard<std::mutex> lock(featuresMutex);
    smoothingTimeMs = milliseconds;

    // Update smoothing coefficient based on time constant
    // Smoothing time constant: τ = 1 / (2π * f) where f is cutoff frequency
    float smoothingCutoff = 1.0f / (smoothingTimeMs / 1000.0f);
    smoothingCoeff = std::exp(-1.0f / (smoothingCutoff * hopSize / (float)sampleRate));
}

void AudioFFTAnalyzer::setDecayRate(float decayPerSecond) {
    std::lock_guard<std::mutex> lock(featuresMutex);
    decayRate = decayPerSecond;
}

void AudioFFTAnalyzer::processAudio(const float* audioData, int numSamples) {
    std::lock_guard<std::mutex> lock(featuresMutex);

    for (int i = 0; i < numSamples; ++i) {
        // Shift buffer and add new sample
        for (int j = fftSize - 1; j > 0; --j) {
            fftBuffer[j] = fftBuffer[j - 1];
        }

        fftBuffer[0] = audioData[i] * windowFunction[0];

        sampleCounter++;

        // Process when hop size is reached
        if (sampleCounter >= hopSize) {
            performFFT(&fftBuffer[0].real());
            analyzeFrequencyBands();
            detectPeaks();
            computeSpectralFeatures();
            applySmoothing();
            applyDecay();

            sampleCounter = 0;
        }
    }
}

void AudioFFTAnalyzer::performFFT(const float* audioData) {
    // Apply window function to input
    std::vector<float> windowedBuffer(fftSize);
    for (int i = 0; i < fftSize; ++i) {
        windowedBuffer[i] = audioData[i] * windowFunction[i];
    }

    // Perform FFT (simplified implementation)
    // In production, use a high-performance FFT library like FFTW or kissfft
    for (int k = 0; k < fftSize / 2 + 1; ++k) {
        std::complex<float> sum(0.0f, 0.0f);

        for (int n = 0; n < fftSize; ++n) {
            float angle = 2.0f * M_PI * k * n / fftSize;
            std::complex<float> twiddle(std::cos(angle), -std::sin(angle));
            sum += windowedBuffer[n] * twiddle;
        }

        magnitudeSpectrum[k] = std::abs(sum);
        phaseSpectrum[k] = std::arg(sum);
    }
}

void AudioFFTAnalyzer::computeWindowFunction() {
    for (int i = 0; i < fftSize; ++i) {
        float angle = 2.0f * M_PI * i / (fftSize - 1);
        windowFunction[i] = 0.5f * (1.0f - std::cos(angle));
    }
}

void AudioFFTAnalyzer::analyzeFrequencyBands() {
    if (magnitudeSpectrum.empty()) return;

    // Reset band accumulators
    currentFeatures.frequencyBands.bass = 0.0f;
    currentFeatures.frequencyBands.lowMid = 0.0f;
    currentFeatures.frequencyBands.highMid = 0.0f;
    currentFeatures.frequencyBands.treble = 0.0f;

    // Convert frequency bins to Hz and accumulate energy
    float maxAmplitude = 0.0f;
    float weightedSum = 0.0f;

    for (size_t i = 1; i < magnitudeSpectrum.size(); ++i) {
        float frequency = (float)i * sampleRate / fftSize;
        float amplitude = magnitudeSpectrum[i];

        if (frequency >= BASS_LOW && frequency <= BASS_HIGH) {
            currentFeatures.frequencyBands.bass += amplitude;
        }
        if (frequency >= LOWMID_LOW && frequency <= LOWMID_HIGH) {
            currentFeatures.frequencyBands.lowMid += amplitude;
        }
        if (frequency >= HIGHMID_LOW && frequency <= HIGHMID_HIGH) {
            currentFeatures.frequencyBands.highMid += amplitude;
        }
        if (frequency >= TREBLE_LOW && frequency <= TREBLE_HIGH) {
            currentFeatures.frequencyBands.treble += amplitude;
        }

        // Track peak
        if (amplitude > maxAmplitude) {
            maxAmplitude = amplitude;
            currentFeatures.frequencyBands.peakFrequency = frequency;
            currentFeatures.frequencyBands.peakAmplitude = amplitude;
        }

        weightedSum += frequency * amplitude;
    }

    // Normalize band energies
    float bassNorm = (BASS_HIGH - BASS_LOW) / (float)fftSize;
    float lowMidNorm = (LOWMID_HIGH - LOWMID_LOW) / (float)fftSize;
    float highMidNorm = (HIGHMID_HIGH - HIGHMID_LOW) / (float)fftSize;
    float trebleNorm = (TREBLE_HIGH - TREBLE_LOW) / (float)fftSize;

    currentFeatures.frequencyBands.bass *= bassNorm;
    currentFeatures.frequencyBands.lowMid *= lowMidNorm;
    currentFeatures.frequencyBands.highMid *= highMidNorm;
    currentFeatures.frequencyBands.treble *= trebleNorm;

    // Calculate full spectrum energy
    currentFeatures.frequencyBands.fullSpectrum = std::accumulate(
        magnitudeSpectrum.begin(), magnitudeSpectrum.end(), 0.0f
    );
}

void AudioFFTAnalyzer::detectPeaks() {
    // Simple peak detection with decay
    for (size_t i = 1; i < magnitudeSpectrum.size() - 1; ++i) {
        float current = magnitudeSpectrum[i];
        float prev = magnitudeSpectrum[i - 1];
        float next = magnitudeSpectrum[i + 1];

        // Check if current sample is a local maximum
        if (current > prev && current > next && current > 0.1f) {
            peakHistory[i] = std::max(current, peakHistory[i] * (1.0f + decayRate / 60.0f));
        } else {
            peakHistory[i] *= (1.0f - decayRate / 60.0f);
        }
    }
}

void AudioFFTAnalyzer::computeSpectralFeatures() {
    if (magnitudeSpectrum.empty()) return;

    // Compute RMS
    float rmsSum = 0.0f;
    for (float magnitude : magnitudeSpectrum) {
        rmsSum += magnitude * magnitude;
    }
    currentFeatures.rmsLevel = std::sqrt(rmsSum / magnitudeSpectrum.size());

    // Compute peak level
    currentFeatures.peakLevel = *std::max_element(magnitudeSpectrum.begin(), magnitudeSpectrum.end());

    // Compute crest factor
    currentFeatures.crestFactor = currentFeatures.rmsLevel > 0.0f ?
        currentFeatures.peakLevel / currentFeatures.rmsLevel : 0.0f;

    // Compute spectral centroid (brightness)
    float weightedSum = 0.0f;
    float totalMagnitude = 0.0f;

    for (size_t i = 0; i < magnitudeSpectrum.size(); ++i) {
        float frequency = (float)i * sampleRate / fftSize;
        weightedSum += frequency * magnitudeSpectrum[i];
        totalMagnitude += magnitudeSpectrum[i];
    }

    currentFeatures.spectralCentroid = totalMagnitude > 0.0f ?
        weightedSum / totalMagnitude : 0.0f;

    // Compute spectral spread
    float weightedSum2 = 0.0f;
    for (size_t i = 0; i < magnitudeSpectrum.size(); ++i) {
        float frequency = (float)i * sampleRate / fftSize;
        float diff = frequency - currentFeatures.spectralCentroid;
        weightedSum2 += diff * diff * magnitudeSpectrum[i];
    }

    currentFeatures.spectralSpread = totalMagnitude > 0.0f ?
        std::sqrt(weightedSum2 / totalMagnitude) : 0.0f;
}

void AudioFFTAnalyzer::applySmoothing() {
    // Apply exponential smoothing to visual features
    float alpha = 1.0f - smoothingCoeff;

    currentFeatures.smoothedBass = smoothingCoeff * currentFeatures.smoothedBass +
                                  alpha * currentFeatures.frequencyBands.bass;
    currentFeatures.smoothedMid = smoothingCoeff * currentFeatures.smoothedMid +
                                  alpha * (currentFeatures.frequencyBands.lowMid + currentFeatures.frequencyBands.highMid);
    currentFeatures.smoothedTreble = smoothingCoeff * currentFeatures.smoothedTreble +
                                     alpha * currentFeatures.frequencyBands.treble;
    currentFeatures.smoothedPeak = smoothingCoeff * currentFeatures.smoothedPeak +
                                  alpha * currentFeatures.peakLevel;
}

void AudioFFTAnalyzer::applyDecay() {
    // Apply decay to peak amplitudes
    currentFeatures.frequencyBands.peakAmplitude *= (1.0f - decayRate / 60.0f);

    // Decay peak history
    for (float& peak : peakHistory) {
        peak *= (1.0f - decayRate / 60.0f);
    }
}

void AudioFFTAnalyzer::reset() {
    currentFeatures = {};
    previousFeatures = {};
    sampleCounter = 0;
    peakDetectorState = 0.0f;
    std::fill(peakHistory.begin(), peakHistory.end(), 0.0f);
}

void AudioFFTAnalyzer::clearHistory() {
    magnitudeSpectrum.clear();
    phaseSpectrum.clear();
    windowFunction.clear();
    peakHistory.clear();
}

} // namespace Zenith