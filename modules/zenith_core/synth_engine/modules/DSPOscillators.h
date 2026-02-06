/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

//==============================================================================
// Base class for all DSP modules
class DSPModule {

public:
    virtual ~DSPModule() = default;
    virtual void prepare(double sampleRate, int samplesPerBlock) = 0;
    virtual void reset() = 0;
};

//==============================================================================
// Oscillator Modules
//==============================================================================

class ClassicAnalogOscillator : public DSPModule {
public:
    enum Waveform { Sine, Saw, Square, Triangle };
    
    ClassicAnalogOscillator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    // Set waveform
    void setWaveform(Waveform wave);
    
    // Set pitch (in semitones relative to A4)
    void setPitch(float semitones);
    
    // Set detune (in cents, -100 to +100)
    void setDetune(float cents);
    
    // Set pulse width for square wave (0.0 to 1.0)
    void setPulseWidth(float pw);
    
    // Process one sample
    float processSample();
    
    // Process block (SIMD optimized)
    void processBlock(float* output, int numSamples);
    
private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    double sampleRate = 44100.0;
    Waveform waveform = Waveform::Saw;
    float pitchSemitones = 0.0f;
    float detuneCents = 0.0f;
    float pulseWidth = 0.5f;
    
    // Bandlimiting tables
    static constexpr int BLEP_SIZE = 1024;
    static float blepTable[BLEP_SIZE];
    
    void updatePhaseIncrement();
    float blep(float phase, float step);
};

//==============================================================================

class WavetableOscillator : public DSPModule {
public:
    WavetableOscillator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    // Load wavetable from memory
    bool loadWavetable(const float* data, int numFrames);
    
    // Set morph position between two tables (0.0 to 1.0)
    void setMorph(float position);
    
    // Set pitch
    void setPitch(float semitones);
    
    // Process
    float processSample();
    void processBlock(float* output, int numSamples);
    
private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    double sampleRate = 44100.0;
    float morphPosition = 0.0f;
    
    // Wavetable data
    const float* wavetableA = nullptr;
    const float* wavetableB = nullptr;
    int numFrames = 256;
    
    void updatePhaseIncrement();
    float interpolate(float phase);
};

//==============================================================================

class FMOperator : public DSPModule {
public:
    FMOperator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    // Set frequency ratio (relative to base frequency)
    void setRatio(float ratio);
    
    // Set modulation index (depth of FM)
    void setIndex(float index);
    
    // Set feedback amount
    void setFeedback(float fb);
    
    // Set waveform
    void setWaveform(int wave);
    
    // Process with modulation input
    float processSample(float modulation);
    void processBlock(float* output, const float* modulation, int numSamples);
    
private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    double sampleRate = 44100.0;
    float ratio = 1.0f;
    float index = 0.0f;
    float feedback = 0.0f;
    float lastOutput = 0.0f;
    int waveform = 0; // 0 = sine
    
    void updatePhaseIncrement();
};

//==============================================================================

class AdditiveOscillator : public DSPModule {
public:
    static constexpr int MAX_PARTIALS = 64;
    
    AdditiveOscillator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    // Set partial level (0.0 to 1.0)
    void setPartialLevel(int partial, float level);
    
    // Set partial ratio (harmonic or inharmonic)
    void setPartialRatio(int partial, float ratio);
    
    // Process
    float processSample();
    void processBlock(float* output, int numSamples);
    
private:
    struct Partial {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;
        float level = 0.0f;
        float ratio = 1.0f;
    };
    
    juce::Array<Partial> partials;
    double sampleRate = 44100.0;
    float baseFreq = 440.0f;
    
    void updatePhaseIncrements(float baseFreq);
};

//==============================================================================

class NoiseGenerator : public DSPModule {
public:
    enum Type { White, Pink };
    
    NoiseGenerator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    void setType(Type type);
    float processSample();
    void processBlock(float* output, int numSamples);
    
private:
    Type type = Type::White;
    juce::Random random;
    
    // Pink noise filter state
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
};

//==============================================================================

class SubOscillator : public DSPModule {
public:
    enum Waveform { Square, Sine };
    
    SubOscillator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    void setWaveform(Waveform wave);
    void setLevel(float level);
    
    // Process (always -1 octave from input frequency)
    float processSample(float inputPhase);
    void processBlock(float* output, int numSamples);
    
private:
    Waveform waveform = Waveform::Square;
    float level = 0.5f;
    double sampleRate = 44100.0;
};

//==============================================================================

class GranularEngine : public DSPModule {
public:
    static constexpr int MAX_GRAINS = 16;
    
    GranularEngine();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    // Load sample buffer
    void setSample(const float* data, int numSamples);
    
    // Grain parameters
    void setGrainSize(float sizeSeconds);
    void setGrainDensity(float grainsPerSecond);
    void setPosition(float position); // 0.0 to 1.0
    void setPitch(float semitones);
    void setRandomness(float amount); // 0.0 to 1.0
    
    float processSample();
    void processBlock(float* output, int numSamples);
    
private:
    struct Grain {
        bool active = false;
        float position = 0.0f; // In sample frames
        float age = 0.0f; // In seconds
        float duration = 0.1f; // In seconds
        float pitchOffset = 0.0f;
        float pan = 0.5f;
        float amplitude = 1.0f;
    };
    
    juce::Array<Grain> grains;
    const float* sampleData = nullptr;
    int sampleLength = 0;
    double sampleRate = 44100.0;
    
    float grainSize = 0.1f;
    float grainDensity = 10.0f;
    float position = 0.0f;
    float pitch = 0.0f;
    float randomness = 0.0f;
    
    float accumulatedDensity = 0.0f;
    
    void triggerGrain();
    float processGrain(Grain& grain);
};

//==============================================================================

class UnisonDetuner : public DSPModule {
public:
    UnisonDetuner();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    void setNumVoices(int voices);
    void setDetune(float cents);
    void setSpread(float amount); // Stereo spread
    void setMix(float mix); // 0.0 = original, 1.0 = full unison
    
    // Process (modifies input buffer)
    void processBlock(float* left, float* right, int numSamples);
    
private:
    struct Voice {
        float phase = 0.0f;
        float detune = 0.0f;
        float pan = 0.5f;
    };
    
    juce::Array<Voice> voices;
    int numVoices = 4;
    float detuneAmount = 5.0f;
    float spread = 0.5f;
    float mix = 0.5f;
    double sampleRate = 44100.0;
};

//==============================================================================

class RingModulator : public DSPModule {
public:
    RingModulator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    void setCarrierFrequency(float freq);
    void setCarrierWaveform(int wave);
    
    float processSample(float input);
    void processBlock(float* output, int numSamples);
    
private:
    float carrierPhase = 0.0f;
    float carrierFreq = 440.0f;
    int waveform = 0; // 0 = sine
    double sampleRate = 44100.0;
    
    float getCarrier();
};

//==============================================================================
// Filter Modules
//==============================================================================

class LadderFilter : public DSPModule {
public:
    LadderFilter();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    void setCutoff(float freqHz);
    void setResonance(float res); // 0.0 to 1.0
    void setDrive(float drive); // Soft clipping
    
    float processSample(float input);
    void processBlock(float* output, int numSamples);
    
private:
    float cutoff = 1000.0f;
    float resonance = 0.5f;
    float drive = 0.0f;
    double sampleRate = 44100.0;
    
    // 4-pole ladder state
    float z0 = 0.0f, z1 = 0.0f, z2 = 0.0f, z3 = 0.0f, z4 = 0.0f;
    
    void updateCoefficients();
};

//==============================================================================

class StateVariableFilter : public DSPModule {
public:
    enum Type { Lowpass, Highpass, Bandpass, Notch };
    
    StateVariableFilter();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    void setType(Type type);
    void setCutoff(float freqHz);
    void setResonance(float res); // 0.0 to 1.0 (self-oscillation near 1.0)
    void setSlope(int slope); // 12 or 24 dB/oct
    
    float processSample(float input);
    void processBlock(float* output, int numSamples);
    
private:
    Type type = Type::Lowpass;
    float cutoff = 1000.0f;
    float resonance = 0.5f;
    int slope = 12; // dB/oct
    double sampleRate = 44100.0;
    
    // Two SVF stages for 24dB slope
    float lpf1 = 0.0f, hpf1 = 0.0f, bpf1 = 0.0f;
    float lpf2 = 0.0f, hpf2 = 0.0f, bpf2 = 0.0f;
    
    void updateCoefficients();
};

//==============================================================================

class CombFilter : public DSPModule {
public:
    CombFilter();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    void setDelayTime(float timeSeconds);
    void setFeedback(float feedback); // -1.0 to 1.0
    void setBlend(float blend); // 0.0 = dry, 1.0 = wet
    
    float processSample(float input);
    void processBlock(float* output, int numSamples);
    
private:
    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    int delayInSamples = 0;
    float feedback = 0.5f;
    float blend = 0.5f;
    double sampleRate = 44100.0;
};

//==============================================================================

class FormantFilter : public DSPModule {
public:
    FormantFilter();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    // Vowel presets
    enum Vowel { A, E, I, O, U };
    void setVowel(Vowel vowel);
    
    // Custom formants (up to 4)
    void setFormant(int index, float freqHz, float bandwidth);
    
    float processSample(float input);
    void processBlock(float* output, int numSamples);
    
private:
    struct Formant {
        float freq = 1000.0f;
        float bandwidth = 100.0f;
    };
    
    juce::Array<Formant> formants;
    
    // Each formant is a bandpass filter
    juce::Array<StateVariableFilter> filters;
    double sampleRate = 44100.0;
};

} // namespace zenith
