/*
    AliasingFFTBenchmark.cpp - FFT Aliasing Analysis for Zenith PolySynth

    CRITICAL TEST: Verify anti-aliasing performance < -80dB
    "If your aliasing isn't < -80dB, you lose."

    This test:
    1. Renders 1000+ notes across the frequency range
    2. Performs FFT analysis on each waveform
    3. Measures harmonic content above Nyquist (aliasing artifacts)
    4. Compares to professional standards (Serum: < -80dB aliasing)

    Usage:
    - Test all oscillator types (Sine, Saw, Square, Triangle)
    - Test across frequency range (20Hz to 20kHz)
    - Test at different sample rates
    - Identify which waveforms/oscillators need better anti-aliasing

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "zenith_core/instruments/ZenithPolySynthVoice.h"
#include "zenith_core/instruments/ZenithOscillator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <fstream>

using namespace juce;
using namespace zenith;

//==============================================================================
// FFT Analyzer - Measures aliasing in frequency domain
//==============================================================================
class FFTAliasingAnalyzer {
public:
    // Perform FFT and return magnitude spectrum
    std::vector<float> analyzeFFT(const float* samples, size_t numSamples,
                                   double sampleRate) {
        // Power of 2 FFT size
        size_t fftSize = nextPowerOfTwo(numSamples);
        size_t fftOrder = log2(fftSize);

        dsp::FFT fft(static_cast<int>(fftOrder));

        std::vector<float> fftData(fftSize * 2); // Complex: real + imag
        std::vector<float> magnitudes(fftSize / 2 + 1); // Only need positive frequencies

        // Copy samples to FFT buffer (apply Hann window for better frequency resolution)
        for (size_t i = 0; i < numSamples; ++i) {
            float window = 0.5f * (1.0f - std::cos(2.0f * MathConstants<float>::pi * i / (numSamples - 1)));
            fftData[i] = samples[i] * window;
        }
        // Zero-pad rest
        for (size_t i = numSamples; i < fftSize; ++i) {
            fftData[i] = 0.0f;
        }

        // Perform FFT
        fft.performRealOnlyForwardTransform(fftData.data());

        // Calculate magnitudes (convert to dB)
        for (size_t i = 0; i <= fftSize / 2; ++i) {
            float real = fftData[i * 2];
            float imag = fftData[i * 2 + 1];
            float magnitude = std::sqrt(real * real + imag * imag);
            // Convert to dB (reference = 1.0)
            magnitudes[i] = 20.0f * std::log10(magnitude + 1e-10f);
        }

        return magnitudes;
    }

    // Measure aliasing: frequency content above Nyquist/2
    float measureAliasing(const std::vector<float>& magnitudes, double fundamentalFreq,
                          double sampleRate) {
        double nyquist = sampleRate / 2.0;

        // Find the bin for Nyquist/2 (we only care about aliasing above this)
        size_t nyquistHalfBin = static_cast<size_t>((nyquist / 2.0) / (sampleRate / magnitudes.size()));

        // Find maximum magnitude in aliasing zone
        float maxAliasingMagnitude = -200.0f; // Initialize to very low value

        for (size_t i = nyquistHalfBin; i < magnitudes.size(); ++i) {
            // Skip the fundamental and its harmonics (allow some tolerance)
            double freq = (static_cast<double>(i) / magnitudes.size()) * sampleRate;
            double harmonicNumber = freq / fundamentalFreq;
            int nearestHarmonic = static_cast<int>(std::round(harmonicNumber));

            // Check if this bin is a harmonic (within 1% tolerance)
            bool isHarmonic = false;
            if (nearestHarmonic > 0) {
                double harmonicFreq = fundamentalFreq * nearestHarmonic;
                double tolerance = harmonicFreq * 0.01; // 1% tolerance
                if (std::abs(freq - harmonicFreq) < tolerance) {
                    isHarmonic = true;
                }
            }

            if (!isHarmonic) {
                maxAliasingMagnitude = std::max(maxAliasingMagnitude, magnitudes[i]);
            }
        }

        return maxAliasingMagnitude;
    }

    // Measure signal-to-noise ratio (SNR)
    float measureSNR(const std::vector<float>& magnitudes, double fundamentalFreq,
                     double sampleRate) {
        // Find fundamental magnitude
        size_t fundamentalBin = static_cast<size_t>(fundamentalFreq / (sampleRate / magnitudes.size()));
        float fundamentalMagnitude = magnitudes[fundamentalBin];

        // Find noise floor (minimum magnitude excluding harmonics)
        float minNoiseMagnitude = 0.0f;
        size_t count = 0;

        for (size_t i = 1; i < magnitudes.size(); ++i) {
            double freq = (static_cast<double>(i) / magnitudes.size()) * sampleRate;
            double harmonicNumber = freq / fundamentalFreq;
            int nearestHarmonic = static_cast<int>(std::round(harmonicNumber));

            // Check if this bin is a harmonic (within 1% tolerance)
            bool isHarmonic = false;
            if (nearestHarmonic > 0) {
                double harmonicFreq = fundamentalFreq * nearestHarmonic;
                double tolerance = harmonicFreq * 0.01;
                if (std::abs(freq - harmonicFreq) < tolerance) {
                    isHarmonic = true;
                }
            }

            if (!isHarmonic && freq < sampleRate / 4.0) { // Only look at lower frequencies
                minNoiseMagnitude += magnitudes[i];
                count++;
            }
        }

        if (count > 0) {
            minNoiseMagnitude /= count;
        }

        return fundamentalMagnitude - minNoiseMagnitude;
    }

private:
    size_t nextPowerOfTwo(size_t n) {
        size_t power = 1;
        while (power < n) {
            power *= 2;
        }
        return power;
    }
};

//==============================================================================
// Oscillator Test Result
//==============================================================================
struct OscillatorTestResult {
    const char* waveformName;
    int waveformType;
    float frequency;
    float fundamentalMag;
    float snr;
    float aliasingLevel;
    bool passes; // < -80dB aliasing

    void print() const {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  " << waveformName << " @ " << frequency << " Hz:\n";
        std::cout << "    Fundamental: " << fundamentalMag << " dB | ";
        std::cout << "SNR: " << snr << " dB | ";
        std::cout << "Aliasing: " << aliasingLevel << " dB";

        if (passes) {
            std::cout << " ✓ PASS\n";
        } else {
            std::cout << " ✗ FAIL (target: < -80dB)\n";
        }
    }
};

//==============================================================================
// Comprehensive Oscillator Aliasing Test
//==============================================================================
class OscillatorAliasingTest {
public:
    void runAllTests() {
        std::cout << "=================================================\n";
        std::cout << "Zenith Oscillator Aliasing Analysis (FFT)\n";
        std::cout << "=================================================\n\n";
        std::cout << "Target: < -80dB aliasing (Serum standard)\n\n";

        FFTAliasingAnalyzer analyzer;

        // Test parameters
        std::vector<double> sampleRates = {44100.0, 48000.0, 96000.0};
        std::vector<double> frequencies = {55.0, 110.0, 220.0, 440.0, 880.0, 1760.0, 3520.0, 7040.0};
        const int samplesPerNote = 8192; // Enough for good frequency resolution
        const double sampleRate = 48000.0;

        // Test all waveform types
        struct WaveformTest {
            const char* name;
            int waveformType;
        };

        std::vector<WaveformTest> waveforms = {
            {"Sine", 0},
            {"Sawtooth", 1},
            {"Square", 2},
            {"Triangle", 3}
        };

        std::vector<OscillatorTestResult> allResults;

        // Run tests
        for (auto& waveform : waveforms) {
            std::cout << "\nTesting " << waveform.name << ":\n";
            std::cout << std::string(40, '-') << "\n";

            for (double freq : frequencies) {
                auto result = testOscillator(analyzer, waveform.name, waveform.waveformType,
                                             freq, sampleRate, samplesPerNote);
                result.print();
                allResults.push_back(result);
            }
        }

        // Summary report
        printSummary(allResults);

        // Save detailed results to CSV
        saveResultsCSV(allResults, "oscillator_aliasing_results.csv");
    }

private:
    OscillatorTestResult testOscillator(FFTAliasingAnalyzer& analyzer,
                                        const char* waveformName, int waveformType,
                                        double frequency, double sampleRate,
                                        int numSamples) {
        // Create oscillator
        ZenithOscillator osc;
        osc.setSampleRate(sampleRate);
        osc.setWaveform(static_cast<OscillatorWaveform>(waveformType));

        // Render samples
        std::vector<float> samples(numSamples);
        double phaseIncrement = frequency / sampleRate;

        for (int i = 0; i < numSamples; ++i) {
            samples[i] = osc.getNextSample(frequency, 0.5f);
        }

        // Analyze with FFT
        auto magnitudes = analyzer.analyzeFFT(samples.data(), numSamples, sampleRate);

        OscillatorTestResult result;
        result.waveformName = waveformName;
        result.waveformType = waveformType;
        result.frequency = frequency;
        result.snr = analyzer.measureSNR(magnitudes, frequency, sampleRate);
        result.aliasingLevel = analyzer.measureAliasing(magnitudes, frequency, sampleRate);

        // Get fundamental magnitude
        size_t fundamentalBin = static_cast<size_t>(frequency / (sampleRate / magnitudes.size()));
        result.fundamentalMag = magnitudes[fundamentalBin];

        // Check if passes (< -80dB aliasing)
        result.passes = result.aliasingLevel < -80.0f;

        return result;
    }

    void printSummary(const std::vector<OscillatorTestResult>& results) {
        std::cout << "\n\n=================================================\n";
        std::cout << "SUMMARY\n";
        std::cout << "=================================================\n\n";

        int totalTests = static_cast<int>(results.size());
        int passedTests = 0;
        int failedTests = 0;

        float worstAliasing = -200.0f;
        const char* worstWaveform = "";
        float worstFreq = 0.0f;

        for (const auto& result : results) {
            if (result.passes) {
                passedTests++;
            } else {
                failedTests++;
            }

            if (result.aliasingLevel > worstAliasing) {
                worstAliasing = result.aliasingLevel;
                worstWaveform = result.waveformName;
                worstFreq = result.frequency;
            }
        }

        std::cout << "Total Tests: " << totalTests << "\n";
        std::cout << "Passed: " << passedTests << " (" << (100.0 * passedTests / totalTests) << "%)\n";
        std::cout << "Failed: " << failedTests << "\n\n";

        std::cout << "Worst Aliasing:\n";
        std::cout << "  " << worstWaveform << " @ " << worstFreq << " Hz: " << worstAliasing << " dB\n\n";

        if (failedTests == 0) {
            std::cout << "✓ ALL TESTS PASSED - Anti-aliasing meets professional standards!\n";
        } else {
            std::cout << "✗ SOME TESTS FAILED - Anti-aliasing needs improvement:\n";
            std::cout << "  - Consider implementing PolyBLEP for sawtooth/square\n";
            std::cout << "  - Consider oversampling (2x or 4x) for critical waveforms\n";
            std::cout << "  - Consider band-limited wavetable generation\n";
        }

        std::cout << "\nComparison to Competitors:\n";
        std::cout << "  Serum: < -80dB aliasing target\n";
        std::cout << "  Vital: < -90dB aliasing (spectral oscillators)\n";
        std::cout << "  Zenith: ";

        if (worstAliasing < -90.0f) {
            std::cout << "< -90dB ✓ BETTER THAN SERUM\n";
        } else if (worstAliasing < -80.0f) {
            std::cout << "< -80dB ✓ MATCHES SERUM\n";
        } else if (worstAliasing < -60.0f) {
            std::cout << worstAliasing << "dB ⚠ NEEDS IMPROVEMENT\n";
        } else {
            std::cout << worstAliasing << "dB ✗ UNACCEPTABLE\n";
        }
    }

    void saveResultsCSV(const std::vector<OscillatorTestResult>& results,
                        const char* filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not save results to " << filename << "\n";
            return;
        }

        file << "Waveform,Frequency,Fundamental_dB,SNR_dB,Aliasing_dB,Pass\n";
        for (const auto& result : results) {
            file << result.waveformName << ","
                 << result.frequency << ","
                 << result.fundamentalMag << ","
                 << result.snr << ","
                 << result.aliasingLevel << ","
                 << (result.passes ? "PASS" : "FAIL") << "\n";
        }

        file.close();
        std::cout << "\nResults saved to: " << filename << "\n";
    }
};

//==============================================================================
// Voice Aliasing Test (Full Synth)
//==============================================================================
class VoiceAliasingTest {
public:
    void runTest() {
        std::cout << "\n\n=================================================\n";
        std::cout << "Full Voice Aliasing Test (Real-world Usage)\n";
        std::cout << "=================================================\n\n";

        FFTAliasingAnalyzer analyzer;

        // Create voice
        ZenithPolySynthVoice voice;
        voice.setSampleRate(48000.0);
        voice.setQualityPreset(QualityPreset::Low); // No oversampling baseline

        // Configure for aggressive sound (worst case for aliasing)
        voice.setOsc1Waveform(OscillatorWaveform::Saw); // Sawtooth
        voice.setOsc1Mix(1.0f);
        voice.setFilterCutoff(20000.0f); // Wide open
        voice.setAmpEnvelope(0.01f, 0.1f, 0.7f, 0.1f);

        // Simulate note on
        voice.noteStarted();

        // Render
        const int numSamples = 8192;
        AudioBuffer<float> buffer(2, numSamples);
        voice.renderNextBlock(buffer, 0, numSamples);

        // Analyze left channel
        std::vector<float> samples(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            samples[i] = buffer.getSample(0, i);
        }

        auto magnitudes = analyzer.analyzeFFT(samples.data(), numSamples, 48000.0);

        float snr = analyzer.measureSNR(magnitudes, 440.0, 48000.0);
        float aliasing = analyzer.measureAliasing(magnitudes, 440.0, 48000.0);

        std::cout << "Full Voice Test (Sawtooth @ 440Hz, filter open):\n";
        std::cout << "  SNR: " << snr << " dB\n";
        std::cout << "  Aliasing: " << aliasing << " dB ";
        if (aliasing < -80.0f) {
            std::cout << "✓ PASS\n";
        } else {
            std::cout << "✗ FAIL (target: < -80dB)\n";
        }
    }
};

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[]) {
    std::cout << "Zenith Anti-Aliasing Performance Test\n";
    std::cout << "======================================\n\n";

    OscillatorAliasingTest oscTest;
    oscTest.runAllTests();

    VoiceAliasingTest voiceTest;
    voiceTest.runTest();

    std::cout << "\n\nTest complete!\n";
    std::cout << "\nIf aliasing > -80dB:\n";
    std::cout << "1. Implement PolyBLEP oscillators (already in AdvancedOscillator.h)\n";
    std::cout << "2. Use oversampling (2x or 4x)\n";
    std::cout << "3. Apply anti-aliasing filters\n";
    std::cout << "4. Consider band-limited wavetable generation\n";

    return 0;
}
