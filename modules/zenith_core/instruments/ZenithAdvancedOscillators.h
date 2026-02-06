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

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <cmath>

namespace zenith {

//==============================================================================
// Soft clipping and shaping functions
//==============================================================================

namespace WaveShapers {
    // Symmetrical soft clipper
    inline float softClip(float x) {
        if (x > 1.0f) return 1.0f;
        if (x < -1.0f) return -1.0f;
        return 1.5f * x - 0.5f * x * x * x;
    }
    
    // Wavefolder core function
    inline float fold(float x) {
        // Wrap the signal to create harmonics
        while (x > 1.0f) x = 2.0f - x;
        while (x < -1.0f) x = -2.0f - x;
        return x;
    }
    
    // Buchla-style wavefolder with asymmetry
    inline float buchlaFold(float x, float asymmetry = 0.0f) {
        x += asymmetry;
        
        // Multiple folding stages for richer harmonics
        for (int i = 0; i < 4; ++i) {
            x = fold(x);
        }
        
        return x;
    }
}

//==============================================================================
/**
    Buchla 259 Complex Waveform Generator
    West-coast synthesis with wavefolding and timbre modulation.
*/
class BuchlaWavefolder {
public:
    BuchlaWavefolder() {
        reset();
    }
    
    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }
    
    void reset() {
        phase_ = 0.0;
        foldState_ = 0.0f;
    }
    
    void setFolds(float folds) {
        // Number of folding stages (0.0 to 4.0)
        folds_ = juce::jlimit(0.0f, 4.0f, folds);
    }
    
    void setSymmetry(float symmetry) {
        // Asymmetry of folding (-1.0 to 1.0)
        symmetry_ = juce::jlimit(-1.0f, 1.0f, symmetry);
    }
    
    void setAmount(float amount) {
        // Amount of wavefolding to apply (0.0 to 1.0)
        amount_ = juce::jlimit(0.0f, 1.0f, amount);
    }
    
    float processSample(float input, float phase) {
        // Mix original and folded signal
        float folded = processWavefolder(input);
        return input * (1.0f - amount_) + folded * amount_;
    }
    
private:
    float processWavefolder(float x) {
        if (folds_ <= 0.0f) return x;
        
        // Apply gain before folding
        x *= (1.0f + folds_);
        
        // Add asymmetry
        x += symmetry_ * 0.5f;
        
        // Wavefolding stages
        int numStages = static_cast<int>(folds_);
        for (int i = 0; i < numStages; ++i) {
            x = WaveShapers::fold(x);
        }
        
        // Soft clip output
        return WaveShapers::softClip(x);
    }
    
    double phase_ = 0.0;
    double sampleRate_ = 44100.0;
    float folds_ = 2.0f;
    float symmetry_ = 0.0f;
    float amount_ = 1.0f;
    float foldState_ = 0.0f;
};

//==============================================================================
/**
    Casio CZ-Style Phase Distortion Oscillator
    Mimics the phase distortion synthesis from Casio CZ synthesizers.
*/
class PhaseDistortionOscillator {
public:
    PhaseDistortionOscillator() {
        reset();
    }
    
    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }
    
    void reset() {
        phase_ = 0.0;
    }
    
    void setDistortion(float amount) {
        // Distortion amount (0.0 = sine, 1.0 = fully distorted)
        distortion_ = juce::jlimit(0.0f, 1.0f, amount);
    }
    
    void setWaveform(int wave) {
        // 0 = sine to saw, 1 = sine to square, 2 = sine to triangle
        waveform_ = juce::jlimit(0, 2, wave);
    }
    
    float processSample(float frequency) {
        // Calculate phase increment
        double phaseInc = frequency / sampleRate_;
        
        // Apply phase distortion
        double distortedPhase = applyPhaseDistortion(phase_);
        
        // Generate output based on distorted phase
        float output = 0.0f;
        
        switch (waveform_) {
            case 0: // Sine to Saw
                output = static_cast<float>(std::sin(distortedPhase * 6.28318530718));
                // Interpolate between sine and saw based on distortion
                if (distortion_ > 0.0f) {
                    float saw = static_cast<float>(2.0 * distortedPhase - 1.0);
                    output = output * (1.0f - distortion_) + saw * distortion_;
                }
                break;
                
            case 1: // Sine to Square
                output = static_cast<float>(std::sin(distortedPhase * 6.28318530718));
                if (distortion_ > 0.0f) {
                    float square = (distortedPhase < 0.5) ? 1.0f : -1.0f;
                    output = output * (1.0f - distortion_) + square * distortion_;
                }
                break;
                
            case 2: // Sine to Triangle with variable duty
                output = static_cast<float>(std::sin(distortedPhase * 6.28318530718));
                if (distortion_ > 0.0f) {
                    float tri = 2.0f * std::abs(2.0f * distortedPhase - 1.0f) - 1.0f;
                    output = output * (1.0f - distortion_) + tri * distortion_;
                }
                break;
        }
        
        // Advance phase
        phase_ += phaseInc;
        if (phase_ >= 1.0) phase_ -= 1.0;
        
        return output;
    }
    
private:
    double applyPhaseDistortion(double phase) {
        if (distortion_ <= 0.0f) return phase;
        
        // Reshape phase based on distortion amount
        // This creates the characteristic CZ "bend" in the waveform
        double d = distortion_;
        
        // Phase distortion function
        if (phase < 0.5) {
            // Compress first half
            return phase * (1.0 - d * 0.5);
        } else {
            // Expand second half
            return 0.5 + (phase - 0.5) * (1.0 + d * 0.5);
        }
    }
    
    double phase_ = 0.0;
    double sampleRate_ = 44100.0;
    float distortion_ = 0.0f;
    int waveform_ = 0;
};

//==============================================================================
/**
    Additive Synthesis Oscillator
    64 partials with individual level and ratio control.
*/
class AdditiveOscillator {
public:
    static constexpr int MAX_PARTIALS = 64;
    
    struct Partial {
        float level = 0.0f;        // Amplitude (0.0 to 1.0)
        float ratio = 1.0f;        // Frequency ratio (harmonic or inharmonic)
        float phase = 0.0f;        // Current phase
        float pan = 0.5f;          // Pan position (0.0 = left, 1.0 = right)
    };
    
    AdditiveOscillator() {
        // Initialize with harmonic series
        for (int i = 0; i < MAX_PARTIALS; ++i) {
            partials_[i].ratio = static_cast<float>(i + 1);
            partials_[i].level = (i == 0) ? 1.0f : 0.0f; // Fundamental only
            partials_[i].phase = 0.0f;
            partials_[i].pan = 0.5f;
        }
    }
    
    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
    }
    
    void reset() {
        for (auto& partial : partials_) {
            partial.phase = 0.0f;
        }
    }
    
    void setPartialLevel(int index, float level) {
        if (index >= 0 && index < MAX_PARTIALS) {
            partials_[index].level = juce::jlimit(0.0f, 1.0f, level);
        }
    }
    
    void setPartialRatio(int index, float ratio) {
        if (index >= 0 && index < MAX_PARTIALS) {
            partials_[index].ratio = juce::jlimit(0.0f, 16.0f, ratio);
        }
    }
    
    // Set harmonic/inharmonic spectrum
    void setHarmonicSeries(float decay = 0.5f) {
        for (int i = 0; i < MAX_PARTIALS; ++i) {
            partials_[i].ratio = static_cast<float>(i + 1);
            partials_[i].level = std::pow(decay, static_cast<float>(i));
        }
    }
    
    // Set inharmonic spectrum (bells, metallic)
    void setInharmonicSeries(float inharmonicity = 0.1f) {
        for (int i = 0; i < MAX_PARTIALS; ++i) {
            partials_[i].ratio = static_cast<float>(i + 1) * (1.0f + i * inharmonicity);
            partials_[i].level = 1.0f / static_cast<float>(i + 1);
        }
    }
    
    float processSample(float fundamentalFreq) {
        float output = 0.0f;
        
        for (int i = 0; i < MAX_PARTIALS; ++i) {
            if (partials_[i].level > 0.001f) {
                // Calculate phase increment for this partial
                double phaseInc = (fundamentalFreq * partials_[i].ratio) / sampleRate_;
                
                // Generate sine wave for this partial
                float partialOutput = partials_[i].level * 
                    static_cast<float>(std::sin(partials_[i].phase * 6.28318530718));
                
                output += partialOutput;
                
                // Advance phase
                partials_[i].phase += phaseInc;
                if (partials_[i].phase >= 1.0) partials_[i].phase -= 1.0;
            }
        }
        
        // Normalize to prevent clipping
        return output * 0.5f;
    }
    
    // Get partial for editing
    Partial& getPartial(int index) {
        return partials_[juce::jlimit(0, MAX_PARTIALS - 1, index)];
    }
    
private:
    std::array<Partial, MAX_PARTIALS> partials_;
    double sampleRate_ = 44100.0;
};

//==============================================================================
/**
    Granular Synthesis Engine
    Processes audio samples into small grains for texture and ambience.
*/
class GranularEngine {
public:
    static constexpr int MAX_GRAINS = 16;
    
    struct Grain {
        bool active = false;
        float position = 0.0f;       // Position in sample (0.0 to 1.0)
        float age = 0.0f;            // Current age in seconds
        float duration = 0.1f;       // Grain duration in seconds
        float pitchOffset = 0.0f;    // Pitch offset in semitones
        float pan = 0.5f;            // Pan position (0.0 = left, 1.0 = right)
        float amplitude = 1.0f;      // Grain amplitude
        float phase = 0.0f;          // Internal phase for window
    };
    
    GranularEngine() {
        reset();
    }
    
    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
    }
    
    void reset() {
        for (auto& grain : grains_) {
            grain.active = false;
        }
    }
    
    void setSample(const float* data, int numSamples) {
        sampleData_ = data;
        sampleLength_ = numSamples;
    }
    
    void setGrainSize(float sizeSeconds) {
        grainSize_ = juce::jlimit(0.001f, 2.0f, sizeSeconds);
    }
    
    void setGrainDensity(float grainsPerSecond) {
        grainDensity_ = juce::jlimit(1.0f, 50.0f, grainsPerSecond);
    }
    
    void setPosition(float position) {
        // Base position in sample (0.0 to 1.0)
        position_ = juce::jlimit(0.0f, 1.0f, position);
    }
    
    void setPitch(float semitones) {
        pitchOffset_ = juce::jlimit(-24.0f, 24.0f, semitones);
    }
    
    void setRandomness(float amount) {
        randomness_ = juce::jlimit(0.0f, 1.0f, amount);
    }
    
    float processSample(float trigger) {
        if (!sampleData_ || sampleLength_ == 0) return 0.0f;
        
        float output = 0.0f;
        
        // Trigger new grains based on density
        if (trigger > 0.5f) {
            triggerGrain();
        }
        
        // Process all active grains
        for (auto& grain : grains_) {
            if (grain.active) {
                output += processGrain(grain);
            }
        }
        
        return output;
    }
    
private:
    void triggerGrain() {
        // Find inactive grain or replace oldest
        int grainIndex = -1;
        float oldestAge = 0.0f;
        
        for (int i = 0; i < MAX_GRAINS; ++i) {
            if (!grains_[i].active) {
                grainIndex = i;
                break;
            }
            if (grains_[i].age > oldestAge) {
                oldestAge = grains_[i].age;
                grainIndex = i;
            }
        }
        
        if (grainIndex >= 0) {
            auto& grain = grains_[grainIndex];
            grain.active = true;
            grain.age = 0.0f;
            grain.duration = grainSize_;
            grain.amplitude = 1.0f;
            
            // Randomize position
            float posOffset = (random_.nextFloat() - 0.5f) * randomness_;
            grain.position = juce::jlimit(0.0f, 1.0f, position_ + posOffset);
            
            // Randomize pitch
            float pitchOffset = (random_.nextFloat() - 0.5f) * randomness_ * 12.0f;
            grain.pitchOffset = pitchOffset_ + pitchOffset;
            
            // Randomize pan
            float panOffset = (random_.nextFloat() - 0.5f) * randomness_ * 0.5f;
            grain.pan = juce::jlimit(0.0f, 1.0f, 0.5f + panOffset);
            
            grain.phase = 0.0f;
        }
    }
    
    float processGrain(Grain& grain) {
        // Calculate pitch multiplier
        float pitchMult = std::pow(2.0f, grain.pitchOffset / 12.0f);
        
        // Calculate sample position
        float readPos = grain.position * sampleLength_;
        float phaseInc = pitchMult / sampleLength_;
        
        // Gaussian window for smooth grain envelope
        float t = grain.age / grain.duration;
        float window = std::exp(-std::pow(t - 0.5f, 2.0f) / 0.15f);
        
        // Read sample with linear interpolation
        int index0 = static_cast<int>(readPos) % sampleLength_;
        int index1 = (index0 + 1) % sampleLength_;
        float frac = readPos - std::floor(readPos);
        
        float sample = sampleData_[index0] * (1.0f - frac) + sampleData_[index1] * frac;
        
        // Update grain state
        grain.age += 1.0f / sampleRate_;
        grain.position += phaseInc / sampleRate_;
        
        // Deactivate if grain is finished
        if (grain.age >= grain.duration) {
            grain.active = false;
            return 0.0f;
        }
        
        // Apply window and amplitude
        return sample * window * grain.amplitude;
    }
    
    std::array<Grain, MAX_GRAINS> grains_;
    const float* sampleData_ = nullptr;
    int sampleLength_ = 0;
    double sampleRate_ = 44100.0;
    float grainSize_ = 0.1f;
    float grainDensity_ = 10.0f;
    float position_ = 0.0f;
    float pitchOffset_ = 0.0f;
    float randomness_ = 0.0f;
    juce::Random random_;
};

//==============================================================================
/**
    Wavetable Importer
    Loads wavetables from .wav files with automatic crossfade table generation.
*/
class WavetableImporter {
public:
    static constexpr int WAVETABLE_SIZE = 2048;
    static constexpr int MAX_FRAMES = 256;
    
    struct WavetableFile {
        juce::String name;
        std::vector<std::vector<float>> frames;
        int numFrames = 0;
    };
    
    WavetableImporter() = default;
    
    // Load wavetable from .wav file
    bool loadFromWav(juce::File file, WavetableFile& output) {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(file));
        
        if (!reader) return false;
        
        // Check if file has enough samples
        int64_t totalSamples = reader->lengthInSamples;
        int framesToRead = static_cast<int>(std::min(
            static_cast<int64_t>(MAX_FRAMES),
            totalSamples / WAVETABLE_SIZE));
        
        if (framesToRead == 0) return false;
        
        output.frames.resize(framesToRead);
        output.numFrames = framesToRead;
        output.name = file.getFileNameWithoutExtension();
        
        // Read each frame
        for (int frame = 0; frame < framesToRead; ++frame) {
            output.frames[frame].resize(WAVETABLE_SIZE);
            
            // Read WAVETABLE_SIZE samples for this frame (mono)
            std::vector<float> tempBuffer(WAVETABLE_SIZE);
            float* channelPtr = &tempBuffer[0];
            reader->read(&channelPtr, 1, frame * WAVETABLE_SIZE, WAVETABLE_SIZE);
            
            // Normalize and store
            float maxVal = 0.0f;
            for (int i = 0; i < WAVETABLE_SIZE; ++i) {
                maxVal = std::max(maxVal, std::abs(tempBuffer[i]));
            }
            
            if (maxVal > 0.0001f) {
                for (int i = 0; i < WAVETABLE_SIZE; ++i) {
                    output.frames[frame][i] = tempBuffer[i] / maxVal;
                }
            }
        }
        
        // Generate crossfade tables
        generateCrossfadeTables(output);
        
        return true;
    }
    
    // Generate interpolated tables between frames
    void generateCrossfadeTables(WavetableFile& wavetable) {
        if (wavetable.numFrames < 2) return;
        
        // For now, we'll do linear interpolation between frames during playback
        // More advanced: generate intermediate tables here
    }
    
    // Export wavetable to file format
    bool exportToFile(juce::File file, const WavetableFile& wavetable) {
        // TODO: Implement export to custom format
        return false;
    }
    
private:
};

//==============================================================================
/**
    Multi-Oscillator Engine
    Combines all advanced oscillator types into one unified interface.
*/
class AdvancedOscillatorEngine {
public:
    enum OscType {
        Standard = 0,    // Basic waveforms
        Wavefolder,      // Buchla wavefolding
        PhaseDist,       // Casio CZ phase distortion
        Additive,        // Additive synthesis
        Granular,        // Granular synthesis
        WavetableImport, // Imported wavetables
        NumTypes
    };
    
    AdvancedOscillatorEngine() {
        prepare(44100.0);
    }
    
    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        wavefolder_.prepare(sampleRate);
        phaseDist_.prepare(sampleRate);
        additive_.prepare(sampleRate);
        granular_.prepare(sampleRate);
    }
    
    void reset() {
        phase_ = 0.0;
        wavefolder_.reset();
        phaseDist_.reset();
        additive_.reset();
        granular_.reset();
    }
    
    void setType(OscType type) {
        type_ = type;
    }
    
    void setFrequency(float freq) {
        frequency_ = juce::jlimit(20.0f, 20000.0f, freq);
    }
    
    float processSample() {
        switch (type_) {
            case Standard:
                return processStandard();
                
            case Wavefolder: {
                // Generate sine, then apply wavefolding
                float sine = static_cast<float>(std::sin(phase_ * 6.28318530718));
                float folded = wavefolder_.processSample(sine, static_cast<float>(phase_));
                advancePhase();
                return folded;
            }
                
            case PhaseDist:
                return phaseDist_.processSample(frequency_);
                
            case Additive:
                return additive_.processSample(frequency_);
                
            case Granular:
                // Granular needs trigger signal
                return granular_.processSample(0.0f);
                
            case WavetableImport:
                // TODO: Implement imported wavetable playback
                return 0.0f;
                
            default:
                return 0.0f;
        }
    }
    
    // Accessors for specific engines
    BuchlaWavefolder& getWavefolder() { return wavefolder_; }
    PhaseDistortionOscillator& getPhaseDist() { return phaseDist_; }
    AdditiveOscillator& getAdditive() { return additive_; }
    GranularEngine& getGranular() { return granular_; }
    
private:
    float processStandard() {
        // Basic saw/square/triangle for comparison
        float saw = 2.0f * static_cast<float>(phase_) - 1.0f;
        advancePhase();
        return saw;
    }
    
    void advancePhase() {
        phase_ += frequency_ / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
    }
    
    double phase_ = 0.0;
    double sampleRate_ = 44100.0;
    float frequency_ = 440.0f;
    OscType type_ = Standard;
    
    BuchlaWavefolder wavefolder_;
    PhaseDistortionOscillator phaseDist_;
    AdditiveOscillator additive_;
    GranularEngine granular_;
};

} // namespace zenith
