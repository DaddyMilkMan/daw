#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <memory>

// Mock JUCE types and constants
namespace juce {
    template <typename T>
    struct MathConstants {
        static const T twoPi;
        static const T pi;
    };
    template <> const float MathConstants<float>::twoPi = 6.283185307179586476925286766559f;
    template <> const float MathConstants<float>::pi = 3.1415926535897932384626433832795f;

    template <typename T>
    T jlimit(T min, T max, T val) {
        if (val < min) return min;
        if (val > max) return max;
        return val;
    }
}

constexpr int WAVETABLE_FRAME_SIZE = 2048;

// ============================================================================
// BASELINE IMPLEMENTATIONS
// ============================================================================

float generateSaw_Baseline(float phase) {
    float sample = 0.0f;
    int maxHarmonic = 64;
    for (int h = 1; h <= maxHarmonic; ++h) {
        sample += std::sin(h * phase * juce::MathConstants<float>::twoPi) / h;
    }
    sample *= 0.5f;
    return sample;
}

float generateSquare_Baseline(float phase) {
    float sample = 0.0f;
    int maxHarmonic = 32;
    for (int h = 1; h <= maxHarmonic; h += 2) {
        sample += std::sin(h * phase * juce::MathConstants<float>::twoPi) / h;
    }
    sample *= 0.6f;
    return sample;
}

float generateTriangle_Baseline(float phase) {
    float sample = 0.0f;
    int maxHarmonic = 32;
    int sign = 1;
    for (int h = 1; h <= maxHarmonic; h += 2) {
        sample += sign * std::sin(h * phase * juce::MathConstants<float>::twoPi) / (h * h);
        sign = -sign;
    }
    sample *= 0.8f;
    return sample;
}

float generatePWM_Baseline(float phase, float morphAmount) {
    float sample = 0.0f;
    float pw = 0.1f + morphAmount * 0.8f;
    int maxHarmonic = 32;
    for (int h = 1; h <= maxHarmonic; ++h) {
        float harmPhase = h * phase * juce::MathConstants<float>::twoPi;
        float coeff = 2.0f / (h * juce::MathConstants<float>::pi);
        sample += coeff * std::sin(h * pw * juce::MathConstants<float>::pi) *
                  std::cos(harmPhase - h * pw * juce::MathConstants<float>::pi);
    }
    sample *= 0.6f;
    return sample;
}

// ============================================================================
// OPTIMIZED IMPLEMENTATIONS
// ============================================================================

float generateSaw_Optimized(float phase) {
    float sample = 0.0f;
    int maxHarmonic = 64;

    float angle = phase * juce::MathConstants<float>::twoPi;
    float c1 = std::cos(angle);
    float s1 = std::sin(angle);

    float currentSin = s1;
    float currentCos = c1;

    // h=1 case
    sample += currentSin;

    for (int h = 2; h <= maxHarmonic; ++h) {
        // Recurrence: sin((n+1)x) = sin(nx)cos(x) + cos(nx)sin(x)
        //             cos((n+1)x) = cos(nx)cos(x) - sin(nx)sin(x)
        float nextSin = currentSin * c1 + currentCos * s1;
        float nextCos = currentCos * c1 - currentSin * s1;

        currentSin = nextSin;
        currentCos = nextCos;

        sample += currentSin / static_cast<float>(h);
    }
    sample *= 0.5f;
    return sample;
}

float generateSquare_Optimized(float phase) {
    float sample = 0.0f;
    int maxHarmonic = 32;

    float angle = phase * juce::MathConstants<float>::twoPi;

    // Initial state (h=1)
    float currentSin = std::sin(angle);
    float currentCos = std::cos(angle);

    // Step 2 coefficients (2*angle)
    float stepAngle = 2.0f * angle;
    float c2 = std::cos(stepAngle);
    float s2 = std::sin(stepAngle);

    // h=1
    sample += currentSin;

    for (int h = 3; h <= maxHarmonic; h += 2) {
        float nextSin = currentSin * c2 + currentCos * s2;
        float nextCos = currentCos * c2 - currentSin * s2;

        currentSin = nextSin;
        currentCos = nextCos;

        sample += currentSin / static_cast<float>(h);
    }
    sample *= 0.6f;
    return sample;
}

float generateTriangle_Optimized(float phase) {
    float sample = 0.0f;
    int maxHarmonic = 32;

    float angle = phase * juce::MathConstants<float>::twoPi;

    // Initial state (h=1)
    float currentSin = std::sin(angle);
    float currentCos = std::cos(angle);

    // Step 2 coefficients (2*angle)
    float stepAngle = 2.0f * angle;
    float c2 = std::cos(stepAngle);
    float s2 = std::sin(stepAngle);

    // h=1, sign=1
    sample += currentSin; // / 1^2

    int sign = -1; // Next is 3 (sign -1)

    for (int h = 3; h <= maxHarmonic; h += 2) {
        float nextSin = currentSin * c2 + currentCos * s2;
        float nextCos = currentCos * c2 - currentSin * s2;

        currentSin = nextSin;
        currentCos = nextCos;

        sample += sign * currentSin / static_cast<float>(h * h);
        sign = -sign;
    }
    sample *= 0.8f;
    return sample;
}

// Helper for PWM precomputation
struct PWMCoeffs {
    std::vector<float> X;
    std::vector<float> Y;
};

PWMCoeffs precomputePWM(float morphAmount, int maxHarmonic) {
    PWMCoeffs coeffs;
    coeffs.X.resize(maxHarmonic + 1);
    coeffs.Y.resize(maxHarmonic + 1);

    float pw = 0.1f + morphAmount * 0.8f;

    for (int h = 1; h <= maxHarmonic; ++h) {
        float b = h * pw * juce::MathConstants<float>::pi;
        float sb = std::sin(b);
        float cb = std::cos(b);
        float term = 2.0f / (h * juce::MathConstants<float>::pi) * sb;

        // cos(h*theta - b) = cos(h*theta)cos(b) + sin(h*theta)sin(b)
        // Coeff * ... = Term * (cos(h*theta)cos(b) + sin(h*theta)sin(b))
        //             = (Term * cos(b)) * cos(h*theta) + (Term * sin(b)) * sin(h*theta)

        coeffs.X[h] = term * cb; // Coeff for Cos
        coeffs.Y[h] = term * sb; // Coeff for Sin
    }
    return coeffs;
}

float generatePWM_Optimized_Precomputed(float phase, const PWMCoeffs& coeffs) {
    float sample = 0.0f;
    int maxHarmonic = 32;

    float angle = phase * juce::MathConstants<float>::twoPi;
    float c1 = std::cos(angle);
    float s1 = std::sin(angle);

    float currentSin = s1;
    float currentCos = c1;

    // h=1
    sample += coeffs.X[1] * currentCos + coeffs.Y[1] * currentSin;

    for (int h = 2; h <= maxHarmonic; ++h) {
        float nextSin = currentSin * c1 + currentCos * s1;
        float nextCos = currentCos * c1 - currentSin * s1;

        currentSin = nextSin;
        currentCos = nextCos;

        sample += coeffs.X[h] * currentCos + coeffs.Y[h] * currentSin;
    }

    sample *= 0.6f;
    return sample;
}


// ============================================================================
// BENCHMARK UTILS
// ============================================================================

int main() {
    std::cout << std::fixed << std::setprecision(6);

    // 1. SAW TEST
    {
        std::cout << "\n--- SAW WAVE BENCHMARK ---\n";

        std::vector<float> buffer(WAVETABLE_FRAME_SIZE);

        // Measure Baseline
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; ++i) { // Run 1000 frames
            for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
                float phase = static_cast<float>(s) / WAVETABLE_FRAME_SIZE;
                buffer[s] = generateSaw_Baseline(phase);
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        double baseTime = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Baseline Time: " << baseTime << " ms\n";

        // Measure Optimized
        std::vector<float> optBuffer(WAVETABLE_FRAME_SIZE);
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; ++i) {
            for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
                float phase = static_cast<float>(s) / WAVETABLE_FRAME_SIZE;
                optBuffer[s] = generateSaw_Optimized(phase);
            }
        }
        end = std::chrono::high_resolution_clock::now();
        double optTime = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Optimized Time: " << optTime << " ms\n";
        std::cout << "Speedup: " << baseTime / optTime << "x\n";

        // Validation
        float maxError = 0.0f;
        for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
            float diff = std::abs(buffer[s] - optBuffer[s]);
            if (diff > maxError) maxError = diff;
        }
        std::cout << "Max Error: " << maxError << "\n";
    }

    // 2. SQUARE TEST
    {
        std::cout << "\n--- SQUARE WAVE BENCHMARK ---\n";

        // Measure Baseline
        auto start = std::chrono::high_resolution_clock::now();
        volatile float sink = 0;
        for (int i = 0; i < 1000; ++i) {
            for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
                float phase = static_cast<float>(s) / WAVETABLE_FRAME_SIZE;
                sink += generateSquare_Baseline(phase);
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        double baseTime = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Baseline Time: " << baseTime << " ms\n";

        // Measure Optimized
        start = std::chrono::high_resolution_clock::now();
        sink = 0;
        for (int i = 0; i < 1000; ++i) {
            for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
                float phase = static_cast<float>(s) / WAVETABLE_FRAME_SIZE;
                sink += generateSquare_Optimized(phase);
            }
        }
        end = std::chrono::high_resolution_clock::now();
        double optTime = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Optimized Time: " << optTime << " ms\n";
        std::cout << "Speedup: " << baseTime / optTime << "x\n";

        // Validation
        float maxError = 0.0f;
        for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
            float phase = static_cast<float>(s) / WAVETABLE_FRAME_SIZE;
            float diff = std::abs(generateSquare_Baseline(phase) - generateSquare_Optimized(phase));
            if (diff > maxError) maxError = diff;
        }
        std::cout << "Max Error: " << maxError << "\n";
    }

    // 3. TRIANGLE TEST
    {
        std::cout << "\n--- TRIANGLE WAVE BENCHMARK ---\n";
        // Similar test structure...
        auto start = std::chrono::high_resolution_clock::now();
        volatile float sink = 0;
        for (int i = 0; i < 1000; ++i) {
            for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
                sink += generateTriangle_Baseline(static_cast<float>(s) / WAVETABLE_FRAME_SIZE);
            }
        }
        double baseTime = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();

        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; ++i) {
            for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
                sink += generateTriangle_Optimized(static_cast<float>(s) / WAVETABLE_FRAME_SIZE);
            }
        }
        double optTime = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();

        std::cout << "Baseline: " << baseTime << " ms, Optimized: " << optTime << " ms, Speedup: " << baseTime/optTime << "x\n";

        float maxError = 0.0f;
        for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
             float diff = std::abs(generateTriangle_Baseline(static_cast<float>(s) / WAVETABLE_FRAME_SIZE) -
                                   generateTriangle_Optimized(static_cast<float>(s) / WAVETABLE_FRAME_SIZE));
             if (diff > maxError) maxError = diff;
        }
        std::cout << "Max Error: " << maxError << "\n";
    }

    // 4. PWM TEST
    {
        std::cout << "\n--- PWM WAVE BENCHMARK ---\n";
        float morphAmount = 0.5f;

        // Measure Baseline
        auto start = std::chrono::high_resolution_clock::now();
        volatile float sink = 0;
        for (int i = 0; i < 1000; ++i) { // 1000 frames
            for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
                sink += generatePWM_Baseline(static_cast<float>(s) / WAVETABLE_FRAME_SIZE, morphAmount);
            }
        }
        double baseTime = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();

        // Measure Optimized
        start = std::chrono::high_resolution_clock::now();
        sink = 0;
        for (int i = 0; i < 1000; ++i) { // 1000 frames
             // Precompute once per frame
             auto coeffs = precomputePWM(morphAmount, 32);
             for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
                sink += generatePWM_Optimized_Precomputed(static_cast<float>(s) / WAVETABLE_FRAME_SIZE, coeffs);
            }
        }
        double optTime = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();

        std::cout << "Baseline: " << baseTime << " ms, Optimized: " << optTime << " ms, Speedup: " << baseTime/optTime << "x\n";

        auto coeffs = precomputePWM(morphAmount, 32);
        float maxError = 0.0f;
        for (int s = 0; s < WAVETABLE_FRAME_SIZE; ++s) {
             float diff = std::abs(generatePWM_Baseline(static_cast<float>(s) / WAVETABLE_FRAME_SIZE, morphAmount) -
                                   generatePWM_Optimized_Precomputed(static_cast<float>(s) / WAVETABLE_FRAME_SIZE, coeffs));
             if (diff > maxError) maxError = diff;
        }
        std::cout << "Max Error: " << maxError << "\n";
    }

    return 0;
}
