/*
  ==============================================================================

    AdvancedFilters.h
    Created: 2025-01-28
    Author: Zenith DAW

    Advanced filter models for ZenithPolySynth:
    - Diode Ladder (accurate diode simulation)
    - Korg MS-20 (classic filter)
    - Moog Ladder (4-pole ladder)
    - Comb filter (for phaser/flanger)
    - Filter slopes (12/24/36/48 dB)

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace zenith {

//==============================================================================
// Filter Slopes
//==============================================================================

enum class FilterSlope {
    _12dB = 0,
    _24dB,
    _36dB,
    _48dB
};

//==============================================================================
// Saturation Models
//==============================================================================

enum class SaturationType {
    None = 0,
    SoftClip,
    HardClip,
    Tanh,
    Sigmoid,
    Bitcrush
};

class Saturation {
public:
    Saturation();

    void setType(SaturationType type);
    void setAmount(float amount);

    float processSample(float input);

private:
    SaturationType type_;
    float amount_;

    float softClip(float x) const;
    float hardClip(float x) const;
    float tanhSat(float x) const;
    float sigmoidSat(float x) const;
    float bitcrush(float x) const;
};

//==============================================================================
// Diode Ladder Filter
//==============================================================================

class DiodeLadderFilter {
public:
    DiodeLadderFilter();

    void setCutoff(float freq);
    void setResonance(float res);
    void setSlope(FilterSlope slope);
    void setDrive(float drive);

    void reset();

    float processSample(float input);
    void processBlock(float* buffer, int numSamples);

private:
    juce::Array<float> stages_;
    juce::Array<float> stagesOut_;
    float drive_;
    FilterSlope slope_;
    Saturation saturation_;
    double sampleRate_;
    float cutoff_;
    float resonance_;

    void updateCutoffCoefficients();
    float processDiodeStage(float input, float resonanceFeedback, int stage);
};

//==============================================================================
// MS-20 Filter
//==============================================================================

class MS20Filter {
public:
    MS20Filter();

    void setCutoff(float freq);
    void setResonance(float res);
    void setSlope(FilterSlope slope);
    void setPeak(float peak);

    void reset();

    float processSample(float input);
    void processBlock(float* buffer, int numSamples);

private:
    juce::Array<float> stages_;
    float peak_;
    float cutoff_;
    float resonance_;
    double sampleRate_;

    void updateCutoffCoefficients();
    float processStage(float input, float resonanceFeedback);
};

//==============================================================================
// Moog Ladder Filter
//==============================================================================

class MoogLadderFilter {
public:
    MoogLadderFilter();

    void setCutoff(float freq);
    void setResonance(float res);
    void setSlope(FilterSlope slope);

    void reset();

    float processSample(float input);
    void processBlock(float* buffer, int numSamples);

private:
    juce::Array<float> stages_;
    float feedback_;
    float cutoff_;
    float resonance_;
    FilterSlope slope_;
    double sampleRate_;

    void updateCutoffCoefficients();
    float process4Pole(float input);
    float process3Pole(float input);
    float process2Pole(float input);
};

//==============================================================================
// Comb Filter
//==============================================================================

class CombFilter {
public:
    CombFilter();

    void setCutoff(float freq);
    void setResonance(float res);
    void setSlope(FilterSlope slope);

    void setDelayTime(float seconds);
    void setDamping(float damping);
    void setFeedback(float feedback);

    void reset();

    float processSample(float input);
    void processBlock(float* buffer, int numSamples);

private:
    juce::AudioBuffer<float> delayBuffer_;
    int writePos_;
    float feedback_;
    float damping_;
    float delaySamples_;
    float lastOutput_;
    double sampleRate_;

    void updateDelayTime();
};

//==============================================================================
// Helper Functions
//==============================================================================

inline float fastTanh(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float sigmoid(float x) {
    return 2.0f / (1.0f + std::exp(-x)) - 1.0f;
}

} // namespace zenith
