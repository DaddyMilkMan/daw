/*
  ==============================================================================

    StepLFO.h
    Created: 2025-01-28
    Updated: 2025-02-02 - S-tier implementation
    Author:  Zenith DAW

    Professional step sequencer LFO with sample-accurate phase accumulation.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../sequencer/Arpeggiator.h"  // For ArpSyncRate enum

namespace zenith {

//==============================================================================
// Step LFO Class
//==============================================================================

class StepLFO {
public:
    StepLFO();

    // Configuration
    void setNumSteps(int numSteps);
    void setStep(int index, float value);
    void setSmoothing(float smooth);       // 0=Step, 1=Linear, 2=Cubic
    void setRate(float rateHz);
    void setSyncRate(ArpSyncRate rate);
    void setBPM(double bpm);
    void setSampleRate(double sampleRate);
    void setPhase(float phase);

    // State management
    void reset();
    void resetPhase();

    // Processing
    float processSample();                  // Single sample
    void processBlock(float* buffer, int numSamples);  // Block processing

    // Query
    float getCurrentOutput() const;
    const juce::Array<float>& getSteps() const { return steps_; }
    int getNumSteps() const { return steps_.size(); }

private:
    // Step data
    juce::Array<float> steps_;
    int numSteps_;

    // Phase state - KEY: Phase persists across processBlock calls
    float phase_;              // Current phase position (0 to numSteps)
    float phaseIncrement_;     // Phase advance per sample
    double phaseAccumulator_;  // For potential future use
    int currentStep_;          // Current step index

    // Parameters
    float smoothing_;
    float rateHz_;
    ArpSyncRate syncRate_;
    double bpm_;
    double sampleRate_;

    // Output (not atomic because only written in audio thread, read by audio thread)
    float currentOutput_;

    // Internal helpers
    void updatePhaseIncrement();
    float getInterpolatedOutput(float phase);
};

} // namespace zenith
