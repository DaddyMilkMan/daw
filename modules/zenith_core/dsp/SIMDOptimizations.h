/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#if JUCE_USE_SIMD
#include <juce_dsp/juce_dsp.h>
using namespace juce::dsp;
#endif

namespace zenith {

//==============================================================================
// SIMD-Optimized Audio Processing Utilities
//==============================================================================

/**
    SIMD-accelerated audio buffer operations

    Provides vectorized implementations for common audio processing tasks
    including mixing, gain application, fading, and crossfading.
*/
class SIMDBufferOps {
public:
    //==========================================================================
    // Single Channel Operations
    //==========================================================================

    /** Apply gain to samples with SIMD acceleration */
    static void applyGain(float* samples, int numSamples, float gain);

    /** Apply gain with ramp (linear fade) */
    static void applyGainRamp(float* samples, int numSamples, float startGain, float endGain);

    /** Add samples from source to destination with scaling */
    static void addWithGain(float* dest, const float* src, int numSamples, float gain);

    /** Copy samples from source to destination with gain */
    static void copyWithGain(float* dest, const float* src, int numSamples, float gain);

    /** Fade samples in */
    static void fadeIn(float* samples, int numSamples);

    /** Fade samples out */
    static void fadeOut(float* samples, int numSamples);

    //==========================================================================
    // Stereo/Interleaved Operations
    //==========================================================================

    /** Apply gain to stereo interleaved buffer */
    static void applyGainStereo(float* samples, int numSamples, float gainLeft, float gainRight);

    /** Mix stereo to mono */
    static void stereoToMono(float* dest, const float* srcLeft, const float* srcRight, int numSamples);

    /** Mono to stereo with different gains */
    static void monoToStereo(float* destLeft, float* destRight, const float* src, int numSamples,
                            float gainLeft = 1.0f, float gainRight = 1.0f);

    //==========================================================================
    // Crossfade Operations
    //==========================================================================

    /** Equal power crossfade between two buffers */
    static void crossfadeEqualPower(float* dest, const float* srcA, const float* srcB,
                                   int numSamples, float crossfadeAmount);

    /** Linear crossfade between two buffers */
    static void crossfadeLinear(float* dest, const float* srcA, const float* srcB,
                               int numSamples, float crossfadeAmount);

    //==========================================================================
    // Utility Functions
    //==========================================================================

    /** Calculate RMS level */
    static float calculateRMS(const float* samples, int numSamples);

    /** Find peak value */
    static float findPeak(const float* samples, int numSamples);

    /** Count samples above threshold */
    static int countAboveThreshold(const float* samples, int numSamples, float threshold);

    /** Clamp samples to range */
    static void clamp(float* samples, int numSamples, float min, float max);

    /** Apply soft clipping (tanh) */
    static void softClip(float* samples, int numSamples, float drive = 1.0f);

    /** Apply hard clipping */
    static void hardClip(float* samples, int numSamples, float min = -1.0f, float max = 1.0f);
};

//==============================================================================
// SIMD-Oscillator
//==============================================================================

/**
    SIMD-accelerated oscillator for generating multiple waveforms simultaneously

    Useful for unison voices, supersaw, and other multi-oscillator techniques.
*/
class SIMDOscillator {
public:
    SIMDOscillator();
    ~SIMDOscillator() = default;

    /** Set sample rate */
    void setSampleRate(double sampleRate);

    /** Set frequency for all voices */
    void setFrequency(float frequencyHz);

    /** Set individual voice frequencies (for detuning) */
    void setVoiceFrequencies(const float* frequencies, int numVoices);

    /** Generate samples for all voices */
    void process(float** outputs, int numVoices, int numSamples, OscillatorWaveform waveform);

    /** Generate sine waves for all voices (most optimized) */
    void processSine(float** outputs, int numVoices, int numSamples);

    /** Reset all phases */
    void reset();

    /** Set phase for individual voice */
    void setPhase(int voiceIndex, float phase);

private:
    double sampleRate_ = 44100.0;
    float baseFrequency_ = 440.0f;
    std::array<std::array<double, 16>, 4> phases_; // Up to 64 voices (4x16)
    juce::Random random_;
};

//==============================================================================
// SIMD-Filter
//==============================================================================

/**
    SIMD-accelerated filter bank for processing multiple channels or voices
*/
class SIMDFilterBank {
public:
    static constexpr int maxFilters = 16;

    enum class FilterType {
        Lowpass,
        Highpass,
        Bandpass,
        Notch,
        Allpass
    };

    SIMDFilterBank();
    ~SIMDFilterBank() = default;

    /** Set sample rate */
    void setSampleRate(double sampleRate);

    /** Set cutoff for all filters */
    void setCutoff(float cutoffHz);

    /** Set resonance for all filters */
    void setResonance(float resonance);

    /** Set filter type */
    void setFilterType(FilterType type);

    /** Process multiple filters simultaneously */
    void process(float** inputs, float** outputs, int numFilters, int numSamples);

    /** Reset all filter states */
    void reset();

private:
    struct FilterState {
#if JUCE_USE_SIMD
        juce::dsp::SIMDRegister<float> z1, z2;  // State variables (4x float)
#else
        float z1[4], z2[4];
#endif
    };

    double sampleRate_ = 44100.0;
    float cutoff_ = 1000.0f;
    float resonance_ = 0.0f;
    FilterType type_ = FilterType::Lowpass;

    std::array<FilterState, maxFilters / 4> states_;  // 4 filters per state
    juce::SmoothedValue<float> cutoffSmoothed_;
    juce::SmoothedValue<float> resonanceSmoothed_;
};

//==============================================================================
// SIMD-Envelope
//==============================================================================

/**
    SIMD-accelerated envelope generator for multiple voices
*/
class SIMDEnvelope {
public:
    static constexpr int maxVoices = 16;

    enum class Stage {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    struct Parameters {
        float attack = 0.01f;
        float decay = 0.3f;
        float sustain = 0.7f;
        float release = 0.5f;
        float attackCurve = 0.5f;  // 0=linear, 1=exponential
        float decayCurve = 0.5f;
        float releaseCurve = 0.5f;
    };

    SIMDEnvelope();
    ~SIMDEnvelope() = default;

    /** Set sample rate */
    void setSampleRate(double sampleRate);

    /** Set envelope parameters */
    void setParameters(const Parameters& params);

    /** Trigger attack for voice */
    void noteOn(int voiceIndex);

    /** Trigger release for voice */
    void noteOff(int voiceIndex);

    /** Process all voices */
    void process(float* outputs, int numVoices, int numSamples);

    /** Get current stage for voice */
    Stage getStage(int voiceIndex) const;

    /** Reset all voices */
    void reset();

private:
    struct VoiceState {
        float current = 0.0f;
        float target = 0.0f;
        Stage stage = Stage::Idle;
        float phase = 0.0f;  // For curved segments
    };

    double sampleRate_ = 44100.0;
    Parameters params_;
    std::array<VoiceState, maxVoices> voices_;

    // Coefficients calculated from parameters
    float attackCoeff_ = 0.0f;
    float decayCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
};

//==============================================================================
// SIMD-Mixer
//==============================================================================

/**
    SIMD-accelerated audio mixer for combining multiple signals
*/
class SIMDMixer {
public:
    static constexpr int maxInputs = 16;

    SIMDMixer();
    ~SIMDMixer() = default;

    /** Set gain for input */
    void setInputGain(int inputIndex, float gain);

    /** Set pan for input (0=left, 0.5=center, 1=right) */
    void setInputPan(int inputIndex, float pan);

    /** Set master output gain */
    void setMasterGain(float gain);

    /** Set stereo width (-1 to 1) */
    void setStereoWidth(float width);

    /** Mix all inputs to stereo output */
    void process(const float* const* inputs, int numInputs,
                float* leftOutput, float* rightOutput, int numSamples);

    /** Reset all gain smoothing */
    void reset();

private:
    struct InputState {
        float gain = 1.0f;
        float pan = 0.5f;      // 0=left, 0.5=center, 1=right
        float leftGain = 0.707f;
        float rightGain = 0.707f;
        juce::SmoothedValue<float> smoothedGain;
        juce::SmoothedValue<float> smoothedPan;
    };

    std::array<InputState, maxInputs> inputs_;
    juce::SmoothedValue<float> masterGain_;
    float stereoWidth_ = 1.0f;
    double sampleRate_ = 44100.0;

    void updateInputGains(int inputIndex);
};

//==============================================================================
// SIMD-DSP Utilities
//==============================================================================

/**
    Various DSP operations accelerated with SIMD
*/
class SIMDDSP {
public:
    //==========================================================================
    // Math Operations
    //==========================================================================

    /** Vectorized sin calculation for arrays */
    static void sin(const float* input, float* output, int numSamples);

    /** Vectorized cos calculation for arrays */
    static void cos(const float* input, float* output, int numSamples);

    /** Vectorized tanh (soft clipping) */
    static void tanh(const float* input, float* output, int numSamples);

    /** Vectorized pow (base^exponent) */
    static void pow(const float* input, float exponent, float* output, int numSamples);

    /** Vectorized exp (e^x) */
    static void exp(const float* input, float* output, int numSamples);

    /** Vectorized log (natural logarithm) */
    static void log(const float* input, float* output, int numSamples);

    /** Vectorized sqrt */
    static void sqrt(const float* input, float* output, int numSamples);

    //==========================================================================
    // Signal Processing
    //==========================================================================

    /** Linear interpolation between two arrays */
    static void lerp(const float* a, const float* b, float* output, int numSamples, float t);

    /** Sample-wise multiplication (amplitude modulation) */
    static void multiply(const float* a, const float* b, float* output, int numSamples);

    /** Sample-wise addition (mixing) */
    static void add(const float* a, const float* b, float* output, int numSamples);

    /** Scale and add: output = a * scale + b */
    static void scaleAdd(const float* a, float scale, const float* b, float* output, int numSamples);

    //==========================================================================
    // Window Functions
    //==========================================================================

    /** Apply Hann window */
    static void applyHannWindow(float* samples, int numSamples);

    /** Apply Hamming window */
    static void applyHammingWindow(float* samples, int numSamples);

    /** Apply Blackman window */
    static void applyBlackmanWindow(float* samples, int numSamples);

    /** Generate Hann window */
    static void generateHannWindow(float* output, int numSamples);

    //==========================================================================
    // FFT Helpers
    //==========================================================================

    /** Interleave real and imaginary for FFT input */
    static void interleaveRealImag(const float* real, const float* imag, float* complex, int numSamples);

    /** De-interleave real and imaginary from FFT output */
    static void deinterleaveRealImag(const float* complex, float* real, float* imag, int numSamples);

    /** Convert polar to rectangular */
    static void polarToRectangular(const float* magnitude, const float* phase,
                                  float* real, float* imag, int numSamples);

    /** Convert rectangular to polar */
    static void rectangularToPolar(const float* real, const float* imag,
                                  float* magnitude, float* phase, int numSamples);

    //==========================================================================
    // Denormal Protection
    //==========================================================================

    /** Flush denormals to zero */
    static void flushDenormals(float* samples, int numSamples);

    /** Check if value is denormal */
    static bool isDenormal(float value) {
        return (std::abs(value) < 1e-10f);
    }

    /** Safe denorm-protected addition */
    static float safeAdd(float a, float b) {
        float result = a + b;
        if (isDenormal(result)) return 0.0f;
        return result;
    }

    /** Safe denorm-protected multiplication */
    static float safeMultiply(float a, float b) {
        float result = a * b;
        if (isDenormal(result)) return 0.0f;
        return result;
    }
};

//==============================================================================
// CPU Feature Detection
//==============================================================================

/**
    Detect available CPU SIMD features
*/
class CPUFeatures {
public:
    static bool hasSSE()      { return juce::SystemStats::hasSSE(); }
    static bool hasSSE2()     { return juce::SystemStats::hasSSE2(); }
    static bool hasSSE3()     { return juce::SystemStats::hasSSE3(); }
    static bool hasSSE41()    { return juce::SystemStats::hasSSE41(); }
    static bool hasSSE42()    { return juce::SystemStats::hasSSE42(); }
    static bool hasAVX()      { return juce::SystemStats::hasAVX(); }
    static bool hasAVX2()     { return juce::SystemStats::hasAVX2(); }
    static bool hasAVX512()   { return juce::SystemStats::hasAVX512F(); }
    static bool hasNEON()     { return juce::SystemStats::hasNEON(); }

    /** Get human-readable feature string */
    static juce::String getFeatureString();

    /** Get recommended SIMD width for this CPU */
    static int getSIMDWidth();  // Returns 1, 4, 8, or 16

    /** Check if JUCE SIMD is available */
    static bool hasJUCESIMD() {
#if JUCE_USE_SIMD
        return true;
#else
        return false;
#endif
    }
};

//==============================================================================
// Runtime SIMD Selection
//==============================================================================

/**
    Automatically selects the best available implementation at runtime
*/
class SIMDDispatcher {
public:
    /** Function pointer type for processing function */
    using ProcessFunc = void(*)(float*, const float*, int);

    /** Set up dispatch for a processing function */
    template<typename ScalarFunc, typename SSE2Func, typename AVXFunc, typename AVX512Func>
    static void dispatch(float* output, const float* input, int numSamples,
                        ScalarFunc scalarFn, SSE2Func sse2Fn, AVXFunc avxFn, AVX512Func avx512Fn);

    /** Get optimized buffer operations for current CPU */
    static std::unique_ptr<SIMDBufferOps> createBufferOps();

    /** Get optimized oscillator for current CPU */
    static std::unique_ptr<SIMDOscillator> createOscillator();
};

} // namespace zenith
