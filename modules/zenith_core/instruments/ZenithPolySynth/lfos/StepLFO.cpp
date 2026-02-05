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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    StepLFO.cpp
    Created: 2025-01-28
    Updated: 2025-02-02 - S-tier implementation
    Author:  Zenith DAW

    Professional step sequencer LFO with sample-accurate phase accumulation.
    Properly integrates with modulation matrix providing block-level outputs.


  ==============================================================================
*/

#include "StepLFO.h"
#include <cmath>

namespace zenith {

//==============================================================================
// StepLFO Implementation
//==============================================================================

namespace {
    // Convert sync rate to samples per step at given BPM and sample rate
    double getSamplesPerStep(ArpSyncRate syncRate, double bpm, double rateHz, double sampleRate) {
        if (syncRate == ArpSyncRate::Free) {
            // Free running in Hz
            if (rateHz > 0.0f && sampleRate > 0.0) {
                return sampleRate / rateHz;
            }
            return sampleRate / 1.0; // Default 1 Hz
        }

        // BPM synced
        double divisor = 0.0;
        switch (syncRate) {
            case ArpSyncRate::_1_64: divisor = 1.0 / 64.0; break;
            case ArpSyncRate::_1_32: divisor = 1.0 / 32.0; break;
            case ArpSyncRate::_1_16: divisor = 1.0 / 16.0; break;
            case ArpSyncRate::_1_8:  divisor = 1.0 / 8.0;  break;
            case ArpSyncRate::_1_4:  divisor = 1.0 / 4.0;  break;
            case ArpSyncRate::_1_2:  divisor = 1.0 / 2.0;  break;
            case ArpSyncRate::_1_1:  divisor = 1.0;         break;
            case ArpSyncRate::_2_1:  divisor = 2.0;         break;
            case ArpSyncRate::_4_1:  divisor = 4.0;         break;
            case ArpSyncRate::_8_1:  divisor = 8.0;         break;
            case ArpSyncRate::_16_1: divisor = 16.0;        break;
            default: divisor = 0.25; break;
        }

        double beatTime = 60.0 / bpm;
        return beatTime * sampleRate * divisor;
    }

    // Cubic interpolation (4-point)
    float cubicInterpolate(float y0, float y1, float y2, float y3, float mu) {
        float mu2 = mu * mu;
        float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
        float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float a2 = -0.5f * y0 + 0.5f * y2;
        float a3 = y1;
        return a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3;
    }

    // Linear interpolation
    float linearInterpolate(float y1, float y2, float mu) {
        return y1 + (y2 - y1) * mu;
    }
}

StepLFO::StepLFO()
    : numSteps_(16)
    , currentStep_(0)
    , phase_(0.0)
    , phaseIncrement_(0.0)
    , smoothing_(0.0f)
    , currentOutput_(0.0f)
    , rateHz_(1.0f)
    , syncRate_(ArpSyncRate::_1_4)
    , bpm_(120.0)
    , sampleRate_(44100.0)
    , phaseAccumulator_(0.0)  // KEY: Phase persists across blocks
{
    // Initialize with a simple ramp pattern
    for (int i = 0; i < 16; ++i) {
        steps_.add(static_cast<float>(i) / 15.0f * 2.0f - 1.0f);
    }
}

void StepLFO::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    updatePhaseIncrement();
}

void StepLFO::setNumSteps(int numSteps) {
    numSteps_ = juce::jlimit(2, 64, numSteps);

    // Resize steps array
    while (steps_.size() < numSteps_) {
        steps_.add(steps_.isEmpty() ? 0.0f : steps_.getLast());
    }
    while (steps_.size() > numSteps_) {
        steps_.removeLast();
    }

    // Reset phase to prevent out-of-bounds
    phase_ = std::fmod(phase_, static_cast<float>(numSteps_));
}

void StepLFO::setStep(int index, float value) {
    if (index >= 0 && index < steps_.size()) {
        steps_.set(index, juce::jlimit(-1.0f, 1.0f, value));
    }
}

void StepLFO::setSmoothing(float smooth) {
    smoothing_ = juce::jlimit(0.0f, 2.0f, smooth);
}

void StepLFO::setRate(float rateHz) {
    rateHz_ = juce::jlimit(0.01f, 50.0f, rateHz);
    updatePhaseIncrement();
}

void StepLFO::setSyncRate(ArpSyncRate rate) {
    syncRate_ = rate;
    updatePhaseIncrement();
}

void StepLFO::setBPM(double bpm) {
    bpm_ = juce::jlimit(20.0, 300.0, bpm);
    updatePhaseIncrement();
}

void StepLFO::setPhase(float phase) {
    phase_ = juce::jlimit(0.0f, 1.0f, phase);
    // Reset phase accumulator to match
    phaseAccumulator_ = 0.0;
}

void StepLFO::reset() {
    currentStep_ = 0;
    phase_ = 0.0f;
    phaseAccumulator_ = 0.0;
    currentOutput_ = 0.0f;
}

void StepLFO::resetPhase() {
    phase_ = 0.0f;
    phaseAccumulator_ = 0.0;
    currentStep_ = 0;
}

float StepLFO::getCurrentOutput() const {
    return currentOutput_;
}

float StepLFO::processSample() {
    // Advance phase
    phase_ += phaseIncrement_;
    if (phase_ >= static_cast<float>(numSteps_)) {
        phase_ -= static_cast<float>(numSteps_);
    }

    // Get interpolated output
    currentOutput_ = getInterpolatedOutput(phase_);
    return currentOutput_;
}

void StepLFO::processBlock(float* buffer, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Advance phase for each sample
        phase_ += phaseIncrement_;

        // Wrap phase
        if (phase_ >= static_cast<float>(numSteps_)) {
            phase_ -= static_cast<float>(numSteps_);
        }

        // Get interpolated output
        buffer[i] = getInterpolatedOutput(phase_);
    }

    // Store last output for query
    currentOutput_ = buffer[numSamples - 1];
}

void StepLFO::updatePhaseIncrement() {
    double samplesPerStep = getSamplesPerStep(syncRate_, bpm_, rateHz_, sampleRate_);

    // Phase increment per sample = numSteps / samplesPerStep
    // This means phase_ increases by numSteps over one full cycle
    if (samplesPerStep > 0.0) {
        phaseIncrement_ = static_cast<float>(numSteps_ / samplesPerStep);
    } else {
        phaseIncrement_ = 1.0f / 44100.0f;
    }
}

float StepLFO::getInterpolatedOutput(float phase) {
    int smoothingMode = static_cast<int>(std::round(smoothing_));

    // Clamp phase to valid range
    phase = std::fmod(phase, static_cast<float>(numSteps_));
    if (phase < 0.0f) phase += static_cast<float>(numSteps_);

    int index0 = static_cast<int>(phase);
    int index1 = (index0 + 1) % numSteps_;
    float frac = phase - std::floor(phase);

    // Clamp indices
    index0 = juce::jlimit(0, steps_.size() - 1, index0);
    index1 = juce::jlimit(0, steps_.size() - 1, index1);

    switch (smoothingMode) {
        case 0: // Step (no interpolation)
            return steps_[index0];

        case 1: // Linear interpolation
            return linearInterpolate(steps_[index0], steps_[index1], frac);

        case 2: // Cubic interpolation
        default: {
            int indexPrev = (index0 - 1 + numSteps_) % numSteps_;
            int indexNext = (index1 + 1) % numSteps_;

            indexPrev = juce::jlimit(0, steps_.size() - 1, indexPrev);
            indexNext = juce::jlimit(0, steps_.size() - 1, indexNext);

            return cubicInterpolate(steps_[indexPrev], steps_[index0],
                                  steps_[index1], steps_[indexNext], frac);
        }
    }
}

} // namespace zenith
