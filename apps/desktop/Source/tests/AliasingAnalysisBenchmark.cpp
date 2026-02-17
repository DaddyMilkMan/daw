/*
    AliasingAnalysisBenchmark.cpp - FFT Aliasing Measurement

    Measures aliasing distortion in oscillators using FFT analysis.
    Target: < -80dB worst-case aliasing (Serum standard)

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>

//==============================================================================
// Simple Oscillator Implementations
//==============================================================================
class SimpleOscillator {
private:
    double phase_ = 0.0;
    double sampleRate_;

public:
    SimpleOscillator(double sr) : sampleRate_(sr) {}

    void setSampleRate(double sr) { sampleRate_ = sr; }

    // Naive sawtooth (has aliasing)
    float sawNaive(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return 2.0f * static_cast<float>(phase_) - 1.0f;
    }

    // Naive square (has aliasing)
    float squareNaive(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return (phase_ < 0.5) ? 1.0f : -1.0f;
    }

    // Naive triangle (bandlimited, minimal aliasing)
    float triangleNaive(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        double t = phase_ * 4.0;
        if (t < 1.0) return static_cast<float>(t);
        if (t < 3.0) return static_cast<float>(2.0 - t);
        return static_cast<float>(t - 4.0);
    }

    // Sine (no aliasing)
    float sine(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return std::sin(phase_ * 6.28318530718);
    }
};

//==============================================================================
// Simple FFT Implementation (Cooley-Tukey)
//==============================================================================
class SimpleFFT {
private:
    int size_;

public:
    SimpleFFT(int n) : size_(n) {
        // Must be power of 2
    }

    // Compute FFT of real signal
    void fft(const std::vector<float>& input, std::vector<std::complex<double>>& output) {
        int n = input.size();
        output.resize(n / 2 + 1);

        // Convert to complex
        std::vector<std::complex<double>> data(n);
        for (int i = 0; i < n; ++i) {
            data[i] = std::complex<double>(input[i], 0.0);
        }

        // Cooley-Tukey FFT (recursive)
        fftRecursive(data);

        // Keep only positive frequencies
        for (int i = 0; i <= n / 2; ++i) {
            output[i] = data[i];
        }
    }

private:
    void fftRecursive(std::vector<std::complex<double>>& data) {
        int n = data.size();
        if (n <= 1) return;

        // Split into even/odd
        std::vector<std::complex<double>> even(n / 2);
        std::vector<std::complex<double>> odd(n / 2);
        for (int i = 0; i < n / 2; ++i) {
            even[i] = data[2 * i];
            odd[i] = data[2 * i + 1];
        }

        // Recursive FFT
        fftRecursive(even);
        fftRecursive(odd);

        // Combine
        for (int k = 0; k < n / 2; ++k) {
            std::complex<double> t = std::exp(std::complex<double>(0, -2.0 * 3.14159265359 * k / n)) * odd[k];
            data[k] = even[k] + t;
            data[k + n / 2] = even[k] - t;
        }
    }
};

//==============================================================================
// Aliasing Analyzer
//==============================================================================
struct AliasingResult {
    const char* waveform;
    float frequency;
    double fundamentalDB;
    double worstAliasingDB;
    double aliasedHarmonics;
    const char* assessment;

    void print() const {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  " << waveform << " @ " << frequency << " Hz:\n";
        std::cout << "    Fundamental: " << fundamentalDB << " dB\n";
        std::cout << "    Worst Aliasing: " << worstAliasingDB << " dB\n";
        std::cout << "    Aliased Harmonics: " << aliasedHarmonics << "\n";
        std::cout << "    " << assessment << "\n\n";
    }
};

AliasingResult analyzeAliasing(const std::vector<float>& signal, int sampleRate, float testFreq, const char* waveform) {
    AliasingResult result;
    result.waveform = waveform;
    result.frequency = testFreq;

    // Perform FFT
    SimpleFFT fft(signal.size());
    std::vector<std::complex<double>> spectrum;
    fft.fft(signal, spectrum);

    // Convert to dB
    std::vector<double> magnitudeDB(spectrum.size());
    for (size_t i = 0; i < spectrum.size(); ++i) {
        double mag = std::abs(spectrum[i]);
        magnitudeDB[i] = 20.0 * std::log10(mag + 1e-10);
    }

    // Find fundamental bin
    int fundamentalBin = static_cast<int>(testFreq * signal.size() / sampleRate);
    result.fundamentalDB = magnitudeDB[fundamentalBin];

    // Analyze aliased frequencies (above Nyquist)
    int nyquistBin = signal.size() / 2;
    double maxAliasing = -200.0;
    int aliasedCount = 0;

    for (int i = fundamentalBin + 1; i < nyquistBin; ++i) {
        // Check for significant non-harmonic content
        // (simplify: check everything above fundamental)
        if (magnitudeDB[i] > maxAliasing) {
            maxAliasing = magnitudeDB[i];
        }

        // Count significant aliased components
        if (magnitudeDB[i] > -80.0) {
            aliasedCount++;
        }
    }

    result.worstAliasingDB = maxAliasing;
    result.aliasedHarmonics = aliasedCount;

    // Assessment
    if (maxAliasing < -90.0) {
        result.assessment = "✓ EXCELLENT - Better than Serum";
    } else if (maxAliasing < -80.0) {
        result.assessment = "✓ GOOD - Matches Serum standard";
    } else if (maxAliasing < -60.0) {
        result.assessment = "⚠ ACCEPTABLE - Below professional standard";
    } else {
        result.assessment = "✗ POOR - Needs anti-aliasing";
    }

    return result;
}

//==============================================================================
// Benchmark Aliasing
//==============================================================================
std::vector<AliasingResult> benchmarkAliasing(int sampleRate, int fftSize) {
    std::vector<AliasingResult> results;

    // Test frequencies (spanning the range)
    std::vector<float> testFrequencies = {110.0f, 440.0f, 1000.0f, 5000.0f, 10000.0f};

    std::cout << "Testing aliasing at " << testFrequencies.size() << " frequencies...\n\n";

    // Test each waveform
    for (auto freq : testFrequencies) {
        SimpleOscillator osc(sampleRate);

        // Generate test signal (several cycles for FFT)
        std::vector<float> signal(fftSize);

        // Sawtooth
        for (size_t i = 0; i < signal.size(); ++i) {
            signal[i] = osc.sawNaive(freq);
        }
        results.push_back(analyzeAliasing(signal, sampleRate, freq, "Sawtooth (naive)"));

        // Square
        for (size_t i = 0; i < signal.size(); ++i) {
            signal[i] = osc.squareNaive(freq);
        }
        results.push_back(analyzeAliasing(signal, sampleRate, freq, "Square (naive)"));

        // Triangle
        for (size_t i = 0; i < signal.size(); ++i) {
            signal[i] = osc.triangleNaive(freq);
        }
        results.push_back(analyzeAliasing(signal, sampleRate, freq, "Triangle (naive)"));

        // Sine (reference - should have no aliasing)
        for (size_t i = 0; i < signal.size(); ++i) {
            signal[i] = osc.sine(freq);
        }
        results.push_back(analyzeAliasing(signal, sampleRate, freq, "Sine (reference)"));
    }

    return results;
}

//==============================================================================
// Main
//==============================================================================
int main() {
    std::cout << "========================================\n";
    std::cout << "Aliasing Analysis Benchmark\n";
    std::cout << "========================================\n\n";

    std::cout << "Measuring aliasing distortion using FFT analysis\n";
    std::cout << "Target: < -80dB worst aliasing (Serum standard)\n\n";

    const int sampleRate = 48000;
    const int fftSize = 8192; // Must be power of 2

    std::cout << "Test parameters:\n";
    std::cout << "  Sample rate: " << sampleRate << " Hz\n";
    std::cout << "  FFT size: " << fftSize << " samples\n";
    std::cout << "  Frequency resolution: " << (static_cast<float>(sampleRate) / fftSize) << " Hz/bin\n\n";

    // Run benchmark
    auto results = benchmarkAliasing(sampleRate, fftSize);

    // Summary
    std::cout << "========================================\n";
    std::cout << "SUMMARY - Worst Case Aliasing\n";
    std::cout << "========================================\n\n";

    struct WaveformSummary {
        const char* name;
        double worstDB;
        int count;
    };

    std::vector<WaveformSummary> summaries = {
        {"Sawtooth (naive)", -200.0, 0},
        {"Square (naive)", -200.0, 0},
        {"Triangle (naive)", -200.0, 0},
        {"Sine (reference)", -200.0, 0}
    };

    // Find worst case for each waveform
    for (const auto& r : results) {
        for (auto& s : summaries) {
            if (std::string(r.waveform) == s.name) {
                if (r.worstAliasingDB > s.worstDB) {
                    s.worstDB = r.worstAliasingDB;
                }
                s.count++;
                break;
            }
        }
    }

    // Print summaries
    std::cout << "Worst-case aliasing by waveform:\n\n";
    for (const auto& s : summaries) {
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  " << s.name << ": " << s.worstDB << " dB";

        if (s.worstDB < -90.0) {
            std::cout << " ✓ EXCELLENT - Better than Serum\n";
        } else if (s.worstDB < -80.0) {
            std::cout << " ✓ GOOD - Matches Serum\n";
        } else if (s.worstDB < -60.0) {
            std::cout << " ⚠ ACCEPTABLE - Below standard\n";
        } else {
            std::cout << " ✗ POOR - Needs improvement\n";
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "PROFESSIONAL ASSESSMENT\n";
    std::cout << "========================================\n\n";

    // Overall assessment
    double overallWorst = -200.0;
    for (const auto& s : summaries) {
        if (std::string(s.name) != "Sine (reference)") {
            if (s.worstDB > overallWorst) {
                overallWorst = s.worstDB;
            }
        }
    }

    std::cout << "Overall worst aliasing: " << overallWorst << " dB\n\n";

    if (overallWorst < -90.0) {
        std::cout << "✅ EXCELLENT - Better than Serum standard\n";
        std::cout << "→ No anti-aliasing work needed\n";
        std::cout << "→ Proceed to Phase 2B\n";
    } else if (overallWorst < -80.0) {
        std::cout << "✅ GOOD - Matches Serum standard\n";
        std::cout << "→ Acceptable for production\n";
        std::cout << "→ Consider PolyBLEP if time permits\n";
        std::cout << "→ Proceed to Phase 2B\n";
    } else if (overallWorst < -60.0) {
        std::cout << "⚠️ ACCEPTABLE - Below professional standard\n";
        std::cout << "→ Implement PolyBLEP (AdvancedOscillator.h)\n";
        std::cout << "→ Add 2x oversampling option\n";
        std::cout << "→ Re-test after fixes\n";
    } else {
        std::cout << "❌ POOR - Unacceptable for production\n";
        std::cout << "→ MUST FIX before proceeding\n";
        std::cout << "→ Implement PolyBLEP + oversampling\n";
        std::cout << "→ Re-test until < -80dB achieved\n";
    }

    std::cout << "\nNOTE: These are NAIVE oscillators (no anti-aliasing).\n";
    std::cout << "Real Zenith oscillators may have better or worse performance.\n";
    std::cout << "This test establishes the baseline for naive waveforms.\n";

    return 0;
}
