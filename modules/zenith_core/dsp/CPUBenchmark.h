/*
    CPU Optimization and Profiling for Zenith Ultra Synth
    SIMD optimizations, cache efficiency, performance monitoring
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <chrono>
#include <atomic>
#include <vector>

namespace zenith {

//==============================================================================
// CPU Performance Monitor
//==============================================================================

class CPUMonitor {
public:
    CPUMonitor() : startTime_(clock::now()) {}

    void startBlock() {
        blockStartTime_ = clock::now();
    }

    void endBlock(int samplesProcessed) {
        auto endTime = clock::now();
        auto duration = std::chrono::duration_cast<microseconds>(endTime - blockStartTime_);

        totalMicroseconds_ += duration.count();
        totalSamples_ += samplesProcessed;
        blockCount_++;
    }

    float getCPUUsage() const {
        if (totalMicroseconds_ == 0) return 0.0f;

        // CPU% = (processing time / real time) * 100
        float realTimeUs = (totalSamples_ / sampleRate_) * 1'000'000.0f;
        return (static_cast<float>(totalMicroseconds_) / realTimeUs) * 100.0f;
    }

    float getAverageTimePerSample() const {
        if (totalSamples_ == 0) return 0.0f;
        return static_cast<float>(totalMicroseconds_) / totalSamples_;
    }

    int getTotalSamples() const { return totalSamples_; }
    int getBlockCount() const { return blockCount_; }

    void reset() {
        totalMicroseconds_ = 0;
        totalSamples_ = 0;
        blockCount_ = 0;
        startTime_ = clock::now();
    }

    void setSampleRate(float sampleRate) { sampleRate_ = sampleRate; }

    // Get time since last reset
    float getElapsedSeconds() const {
        auto now = clock::now();
        auto duration = std::chrono::duration_cast<seconds>(now - startTime_);
        return static_cast<float>(duration.count());
    }

private:
    using clock = std::chrono::high_resolution_clock;
    using microseconds = std::chrono::microseconds;
    using seconds = std::chrono::seconds;

    clock::time_point startTime_;
    clock::time_point blockStartTime_;
    int64_t totalMicroseconds_ = 0;
    int totalSamples_ = 0;
    int blockCount_ = 0;
    float sampleRate_ = 44100.0f;
};

//==============================================================================
// SIMD Vector Operations
//==============================================================================

class SIMDOps {
public:
#if JUCE_USE_SIMD
    using FloatVector = juce::dsp::SIMDRegister<float>;

    // Process 4 samples at once using SSE/NEON
    static inline void process4Samples(float* output, const float* input,
                                       float gain, size_t numSamples) {
        size_t simd = numSamples / 4;

        FloatVector gainVec(gain);

        for (size_t i = 0; i < simd; ++i) {
            FloatVector in = FloatVector::fromRawArray(input + i * 4);
            FloatVector out = in * gainVec;
            out.copyToRawArray(output + i * 4);
        }

        // Process remaining samples
        for (size_t i = simd * 4; i < numSamples; ++i) {
            output[i] = input[i] * gain;
        }
    }

    // Vector add
    static inline void vectorAdd(float* dst, const float* src, size_t numSamples) {
        size_t simd = numSamples / 4;

        for (size_t i = 0; i < simd; ++i) {
            FloatVector a = FloatVector::fromRawArray(dst + i * 4);
            FloatVector b = FloatVector::fromRawArray(src + i * 4);
            FloatVector result = a + b;
            result.copyToRawArray(dst + i * 4);
        }

        for (size_t i = simd * 4; i < numSamples; ++i) {
            dst[i] += src[i];
        }
    }

    // Vector multiply
    static inline void vectorMultiply(float* dst, const float* src, size_t numSamples) {
        size_t simd = numSamples / 4;

        for (size_t i = 0; i < simd; ++i) {
            FloatVector a = FloatVector::fromRawArray(dst + i * 4);
            FloatVector b = FloatVector::fromRawArray(src + i * 4);
            FloatVector result = a * b;
            result.copyToRawArray(dst + i * 4);
        }

        for (size_t i = simd * 4; i < numSamples; ++i) {
            dst[i] *= src[i];
        }
    }

    // Vector multiply-add (fma)
    static inline void vectorMultiplyAdd(float* dst, const float* src1,
                                         const float* src2, size_t numSamples) {
        size_t simd = numSamples / 4;

        for (size_t i = 0; i < simd; ++i) {
            FloatVector a = FloatVector::fromRawArray(dst + i * 4);
            FloatVector b = FloatVector::fromRawArray(src1 + i * 4);
            FloatVector c = FloatVector::fromRawArray(src2 + i * 4);
            FloatVector result = juce::dsp::SIMDRegister<float>::fmuladd(b, c, a);
            result.copyToRawArray(dst + i * 4);
        }

        for (size_t i = simd * 4; i < numSamples; ++i) {
            dst[i] += src1[i] * src2[i];
        }
    }

    // Fast sine approximation (SIMD)
    static inline FloatVector fastSin(FloatVector x) {
        // Parabolic approximation
        FloatVector pi(3.14159265f);
        FloatVector twoPi(6.28318531f);

        // Wrap to [-pi, pi]
        x = x - twoPi * juce::dsp::SIMDRegister<float>::round(x / twoPi);

        // Fast sin approximation
        FloatVector k = 4.0f / pi;
        FloatVector result = k * x;

        FloatVector absX = juce::dsp::SIMDRegister<float>::abs(x);
        FloatVector mask = absX < (pi / 2.0f);

        result = juce::dsp::SIMDRegister<float>::select(mask, result, -result);

        return result;
    }

    // Fast tanh (SIMD) - for soft clipping
    static inline FloatVector fastTanh(FloatVector x) {
        // Approximation: tanh(x) ≈ x * (27 + x^2) / (27 + 9*x^2)
        FloatVector x2 = x * x;
        FloatVector numerator = x * (27.0f + x2);
        FloatVector denominator = 27.0f + 9.0f * x2;
        return numerator / denominator;
    }

#else
    // Scalar fallbacks
    static inline void process4Samples(float* output, const float* input,
                                       float gain, size_t numSamples) {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = input[i] * gain;
        }
    }

    static inline void vectorAdd(float* dst, const float* src, size_t numSamples) {
        for (size_t i = 0; i < numSamples; ++i) {
            dst[i] += src[i];
        }
    }

    static inline void vectorMultiply(float* dst, const float* src, size_t numSamples) {
        for (size_t i = 0; i < numSamples; ++i) {
            dst[i] *= src[i];
        }
    }

    static inline void vectorMultiplyAdd(float* dst, const float* src1,
                                         const float* src2, size_t numSamples) {
        for (size_t i = 0; i < numSamples; ++i) {
            dst[i] += src1[i] * src2[i];
        }
    }
#endif

    // Check if SIMD is available
    static bool hasSIMD() {
#if JUCE_USE_SIMD
        return true;
#else
        return false;
#endif
    }

    // Get SIMD width
    static int getSIMDWidth() {
#if JUCE_USE_SIMD
        return 4;  // SSE/NEON processes 4 floats at once
#else
        return 1;
#endif
    }
};

//==============================================================================
// Optimized Oscillator (with SIMD)
//==============================================================================

class OptimizedOscillator {
public:
    enum class Type {
        Sine,
        Saw,
        Square,
        Triangle,
        Supersaw
    };

    OptimizedOscillator() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        phaseIncrement_ = (440.0f / sampleRate_) * juce::MathConstants<float>::twoPi;
    }

    void setFrequency(float frequency) {
        frequency_ = frequency;
        phaseIncrement_ = (frequency / sampleRate_) * juce::MathConstants<float>::twoPi;
    }

    void setType(Type type) {
        type_ = type;
    }

    void setDetune(float detuneCents) {
        float detuneRatio = std::pow(2.0f, detuneCents / 1200.0f);
        setFrequency(frequency_ * detuneRatio);
    }

    void setPulseWidth(float pulseWidth) {
        pulseWidth_ = juce::jlimit(0.01f, 0.99f, pulseWidth);
    }

    // Generate samples with SIMD optimization
    void generate(float* output, size_t numSamples) {
        switch (type_) {
            case Type::Sine:
                generateSine(output, numSamples);
                break;
            case Type::Saw:
                generateSaw(output, numSamples);
                break;
            case Type::Square:
                generateSquare(output, numSamples);
                break;
            case Type::Triangle:
                generateTriangle(output, numSamples);
                break;
            case Type::Supersaw:
                generateSupersaw(output, numSamples);
                break;
        }
    }

    // Generate with frequency modulation
    void generateFM(float* output, const float* modulation, size_t numSamples, float fmDepth) {
        float phase = phase_;

        for (size_t i = 0; i < numSamples; ++i) {
            float modFreq = phaseIncrement_ * (1.0f + modulation[i] * fmDepth);
            phase += modFreq;

            if (phase >= juce::MathConstants<float>::twoPi) {
                phase -= juce::MathConstants<float>::twoPi;
            }

            output[i] = std::sin(phase);
        }

        phase_ = phase;
    }

private:
    void generateSine(float* output, size_t numSamples) {
#if JUCE_USE_SIMD
        size_t simd = numSamples / 4;

        SIMDOps::FloatVector phaseVec(phase_);
        SIMDOps::FloatVector incVec(phaseIncrement_);

        for (size_t i = 0; i < simd; ++i) {
            SIMDOps::FloatVector result = SIMDOps::fastSin(phaseVec);
            result.copyToRawArray(output + i * 4);
            phaseVec = phaseVec + incVec;

            // Wrap phase
            SIMDOps::FloatVector twoPi(juce::MathConstants<float>::twoPi);
            phaseVec = phaseVec - twoPi * SIMDOps::FloatVector::floor(phaseVec / twoPi);
        }

        phase_ = phaseVec.get(0);

        // Process remaining
        for (size_t i = simd * 4; i < numSamples; ++i) {
            phase_ += phaseIncrement_;
            if (phase_ >= juce::MathConstants<float>::twoPi) {
                phase_ -= juce::MathConstants<float>::twoPi;
            }
            output[i] = std::sin(phase_);
        }
#else
        for (size_t i = 0; i < numSamples; ++i) {
            phase_ += phaseIncrement_;
            if (phase_ >= juce::MathConstants<float>::twoPi) {
                phase_ -= juce::MathConstants<float>::twoPi;
            }
            output[i] = std::sin(phase_);
        }
#endif
    }

    void generateSaw(float* output, size_t numSamples) {
        float phase = phase_;

        for (size_t i = 0; i < numSamples; ++i) {
            // Normalized phase 0-1
            float normPhase = phase / juce::MathConstants<float>::twoPi;
            output[i] = 2.0f * normPhase - 1.0f;

            phase += phaseIncrement_;
            if (phase >= juce::MathConstants<float>::twoPi) {
                phase -= juce::MathConstants<float>::twoPi;
            }
        }

        phase_ = phase;
    }

    void generateSquare(float* output, size_t numSamples) {
        float phase = phase_;

        for (size_t i = 0; i < numSamples; ++i) {
            // Normalized phase 0-1
            float normPhase = phase / juce::MathConstants<float>::twoPi;
            output[i] = (normPhase < pulseWidth_) ? 1.0f : -1.0f;

            phase += phaseIncrement_;
            if (phase >= juce::MathConstants<float>::twoPi) {
                phase -= juce::MathConstants<float>::twoPi;
            }
        }

        phase_ = phase;
    }

    void generateTriangle(float* output, size_t numSamples) {
        float phase = phase_;

        for (size_t i = 0; i < numSamples; ++i) {
            // Triangle wave
            float normPhase = phase / juce::MathConstants<float>::twoPi;
            output[i] = 2.0f * std::abs(2.0f * normPhase - 1.0f) - 1.0f;

            phase += phaseIncrement_;
            if (phase >= juce::MathConstants<float>::twoPi) {
                phase -= juce::MathConstants<float>::twoPi;
            }
        }

        phase_ = phase;
    }

    void generateSupersaw(float* output, size_t numSamples) {
        // Generate 7 detuned saw waves
        float detuneAmount = 0.02f;
        float gains[7] = {1.0f, 0.6f, 0.4f, 0.3f, 0.2f, 0.15f, 0.1f};
        float detunes[7] = {-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f};

        for (size_t i = 0; i < numSamples; ++i) {
            float sample = 0.0f;

            for (int j = 0; j < 7; ++j) {
                float detuneRatio = std::pow(2.0f, detunes[j] * detuneAmount / 12.0f);
                float phaseInc = phaseIncrement_ * detuneRatio;

                float wavePhase = phase_ * detuneRatio;
                while (wavePhase >= juce::MathConstants<float>::twoPi) {
                    wavePhase -= juce::MathConstants<float>::twoPi;
                }

                float normPhase = wavePhase / juce::MathConstants<float>::twoPi;
                sample += (2.0f * normPhase - 1.0f) * gains[j];
            }

            output[i] = sample / 3.65f;  // Normalize
        }

        phase_ += phaseIncrement_ * numSamples;
        while (phase_ >= juce::MathConstants<float>::twoPi) {
            phase_ -= juce::MathConstants<float>::twoPi;
        }
    }

    float sampleRate_ = 44100.0f;
    float frequency_ = 440.0f;
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    float pulseWidth_ = 0.5f;
    Type type_ = Type::Sine;
};

//==============================================================================
// Optimized Filter (biquad cascade)
//==============================================================================

class OptimizedFilter {
public:
    enum class Type {
        Lowpass,
        Highpass,
        Bandpass,
        Notch,
        Allpass,
        Peak,
        Lowshelf,
        Highshelf
    };

    OptimizedFilter() {
        setSampleRate(44100.0f);
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        coefficients_.resize(0);
        states_.resize(0);
    }

    void setCutoff(float cutoff) {
        cutoff_ = juce::jlimit(20.0f, sampleRate_ * 0.49f, cutoff);
        updateCoefficients();
    }

    void setResonance(float resonance) {
        resonance_ = juce::jlimit(0.1f, 20.0f, resonance);
        updateCoefficients();
    }

    void setType(Type type) {
        type_ = type;
        updateCoefficients();
    }

    void setOversample(int factor) {
        oversampleFactor_ = juce::jlimit(1, 4, factor);
    }

    float processSample(float input) {
        float output = input;

        for (int i = 0; i < oversampleFactor_; ++i) {
            output = processBiquad(output, 0);
        }

        return output;
    }

    void process(float* output, const float* input, size_t numSamples) {
        if (oversampleFactor_ == 1) {
            // Direct processing
            for (size_t i = 0; i < numSamples; ++i) {
                output[i] = processBiquad(input[i], 0);
            }
        } else {
            // Oversampled processing
            for (size_t i = 0; i < numSamples; ++i) {
                output[i] = processSample(input[i]);
            }
        }
    }

    void reset() {
        std::fill(states_.begin(), states_.end(), 0.0f);
    }

private:
    float processBiquad(float input, int stage) {
        size_t offset = stage * 4;

        // Direct form I transposed
        float output = coefficients_[offset] * input +
                      coefficients_[offset + 1] * states_[offset] +
                      coefficients_[offset + 2] * states_[offset + 1];

        states_[offset + 1] = states_[offset];
        states_[offset] = input - coefficients_[offset + 3] * output -
                          coefficients_[offset + 4] * states_[offset + 1];

        return output;
    }

    void updateCoefficients() {
        // Allocate for 4 cascaded biquads
        coefficients_.resize(20);
        states_.resize(16);

        // Calculate coefficients for 4 cascaded 24dB/oct filter
        float omega = 2.0f * juce::MathConstants<float>::pi * cutoff_ / sampleRate_;
        float sinOmega = std::sin(omega);
        float cosOmega = std::cos(omega);
        float alpha = sinOmega / (2.0f * resonance_);

        // Each biquad gets 1/4 of the total slope
        float qPerStage = std::sqrt(resonance_);

        for (int i = 0; i < 4; ++i) {
            size_t offset = i * 5;

            float b0, b1, b2, a0, a1, a2;

            switch (type_) {
                case Type::Lowpass:
                    b0 = (1.0f - cosOmega) / 2.0f;
                    b1 = 1.0f - cosOmega;
                    b2 = (1.0f - cosOmega) / 2.0f;
                    a0 = 1.0f + alpha;
                    a1 = -2.0f * cosOmega;
                    a2 = 1.0f - alpha;
                    break;

                case Type::Highpass:
                    b0 = (1.0f + cosOmega) / 2.0f;
                    b1 = -(1.0f + cosOmega);
                    b2 = (1.0f + cosOmega) / 2.0f;
                    a0 = 1.0f + alpha;
                    a1 = -2.0f * cosOmega;
                    a2 = 1.0f - alpha;
                    break;

                case Type::Bandpass:
                    b0 = alpha;
                    b1 = 0.0f;
                    b2 = -alpha;
                    a0 = 1.0f + alpha;
                    a1 = -2.0f * cosOmega;
                    a2 = 1.0f - alpha;
                    break;

                default:
                    // Lowpass fallback
                    b0 = (1.0f - cosOmega) / 2.0f;
                    b1 = 1.0f - cosOmega;
                    b2 = (1.0f - cosOmega) / 2.0f;
                    a0 = 1.0f + alpha;
                    a1 = -2.0f * cosOmega;
                    a2 = 1.0f - alpha;
                    break;
            }

            // Normalize
            coefficients_[offset] = b0 / a0;
            coefficients_[offset + 1] = b1 / a0;
            coefficients_[offset + 2] = b2 / a0;
            coefficients_[offset + 3] = a1 / a0;
            coefficients_[offset + 4] = a2 / a0;
        }
    }

    float sampleRate_ = 44100.0f;
    float cutoff_ = 1000.0f;
    float resonance_ = 1.0f;
    Type type_ = Type::Lowpass;
    int oversampleFactor_ = 1;

    std::vector<float> coefficients_;  // 5 coefficients per biquad
    std::vector<float> states_;        // 4 states per biquad
};

//==============================================================================
// Cache-Friendly Voice Allocation
//==============================================================================

class VoiceAllocator {
public:
    struct Voice {
        int note = -1;
        float velocity = 0.0f;
        int age = 0;
        bool active = false;
        float phase = 0.0f;
    };

    VoiceAllocator(int maxVoices = 16) {
        voices_.resize(maxVoices);
    }

    void noteOn(int note, float velocity) {
        // Find free voice
        int freeVoice = findFreeVoice();

        if (freeVoice >= 0) {
            voices_[freeVoice].note = note;
            voices_[freeVoice].velocity = velocity;
            voices_[freeVoice].active = true;
            voices_[freeVoice].age = ageCounter_++;
        } else {
            // Steal oldest voice
            int oldest = findOldestVoice();
            voices_[oldest].note = note;
            voices_[oldest].velocity = velocity;
            voices_[oldest].age = ageCounter_++;
        }
    }

    void noteOff(int note) {
        for (auto& voice : voices_) {
            if (voice.active && voice.note == note) {
                voice.active = false;
                voice.note = -1;
            }
        }
    }

    Voice* getVoice(int index) {
        if (index >= 0 && index < static_cast<int>(voices_.size())) {
            return &voices_[index];
        }
        return nullptr;
    }

    int getActiveVoiceCount() const {
        int count = 0;
        for (const auto& voice : voices_) {
            if (voice.active) count++;
        }
        return count;
    }

    std::vector<Voice>& getVoices() {
        return voices_;
    }

private:
    int findFreeVoice() {
        for (int i = 0; i < static_cast<int>(voices_.size()); ++i) {
            if (!voices_[i].active) {
                return i;
            }
        }
        return -1;
    }

    int findOldestVoice() {
        int oldest = 0;
        int minAge = voices_[0].age;

        for (int i = 1; i < static_cast<int>(voices_.size()); ++i) {
            if (voices_[i].active && voices_[i].age < minAge) {
                minAge = voices_[i].age;
                oldest = i;
            }
        }

        return oldest;
    }

    std::vector<Voice> voices_;
    int ageCounter_ = 0;
};

//==============================================================================
// Performance Profiler
//==============================================================================

class PerformanceProfiler {
public:
    struct ProfileEntry {
        juce::String name;
        int64_t totalCycles = 0;
        int callCount = 0;
        float averageCycles = 0.0f;
    };

    void begin(const juce::String& name) {
        auto& entry = profiles_[name];
        entry.name = name;
        entry.startTime = juce::Time::getHighResolutionTicks();
    }

    void end(const juce::String& name) {
        auto& entry = profiles_[name];
        int64_t elapsed = juce::Time::getHighResolutionTicks() - entry.startTime;

        entry.totalCycles += elapsed;
        entry.callCount++;
        entry.averageCycles = static_cast<float>(entry.totalCycles) / entry.callCount;
    }

    juce::String getReport() const {
        juce::String report = "=== Performance Report ===\n";

        for (const auto& [name, entry] : profiles_) {
            report += juce::String::formatted("%s: %.2f cycles/call (%d calls)\n",
                                             name.toRawUTF8(),
                                             entry.averageCycles,
                                             entry.callCount);
        }

        return report;
    }

    void reset() {
        profiles_.clear();
    }

private:
    struct ExtendedProfileEntry : ProfileEntry {
        int64_t startTime = 0;
    };

    juce::HashMap<juce::String, ExtendedProfileEntry> profiles_;
};

//==============================================================================
// Unified Optimization Manager
//==============================================================================

class OptimizationManager {
public:
    OptimizationManager() {
        // Detect CPU capabilities
        detectCPUFeatures();
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        cpuMonitor.setSampleRate(sampleRate);
    }

    // Access to subsystems
    CPUMonitor& getCPUMonitor() { return cpuMonitor; }
    PerformanceProfiler& getProfiler() { return profiler; }

    // CPU feature detection
    bool hasSSE() const { return hasSSE_; }
    bool hasSSE2() const { return hasSSE2_; }
    bool hasAVX() const { return hasAVX_; }
    bool hasAVX2() const { return hasAVX2_; }
    bool hasNEON() const { return hasNEON_; }

    juce::String getCPUInfoString() const {
        juce::String info = "CPU Features: ";

        if (hasSSE_) info += "SSE ";
        if (hasSSE2_) info += "SSE2 ";
        if (hasAVX_) info += "AVX ";
        if (hasAVX2_) info += "AVX2 ";
        if (hasNEON_) info += "NEON ";

        info += juce::String::formatted("\nSIMD Width: %d", SIMDOps::getSIMDWidth());

        return info;
    }

    // Performance targets
    bool meetsTarget(float maxCpuPercentPerVoice = 5.0f) const {
        float cpuPerVoice = cpuMonitor.getCPUUsage() /
                           static_cast<float>(std::max(1, getVoiceCount()));
        return cpuPerVoice <= maxCpuPercentPerVoice;
    }

    int getVoiceCount() const { return voiceCount_; }
    void setVoiceCount(int count) { voiceCount_ = juce::jlimit(1, 256, count); }

private:
    void detectCPUFeatures() {
#if JUCE_INTEL
        hasSSE_ = true;
        hasSSE2_ = true;
        // AVX detection would go here
        hasAVX_ = false;  // Set based on CPUID
        hasAVX2_ = false;
#elif JUCE_ARM
        hasNEON_ = true;
#endif
    }

    float sampleRate_ = 44100.0f;
    int voiceCount_ = 16;

    CPUMonitor cpuMonitor;
    PerformanceProfiler profiler;

    bool hasSSE_ = false;
    bool hasSSE2_ = false;
    bool hasAVX_ = false;
    bool hasAVX2_ = false;
    bool hasNEON_ = false;
};

} // namespace zenith
