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

#include "ZenithFilter.h"
#include <cmath>

namespace zenith {

void ZenithFilter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    cutoffSmoothed_.reset(sampleRate, 0.05);
    resonanceSmoothed_.reset(sampleRate, 0.05);
    
    // Initialize oversamplers if not already created
    if (!oversampler2x_) {
        oversampler2x_.reset(new juce::dsp::Oversampling<float>(2));
    }
    if (!oversampler4x_) {
        oversampler4x_.reset(new juce::dsp::Oversampling<float>(4));
    }
    
    // Prepare oversamplers
    oversampler2x_->initProcessing(512);
    oversampler4x_->initProcessing(512);
    
    // Initialize circuit filters if they exist
    if (moogFilter_) moogFilter_->prepare(sampleRate);
    if (ms20Filter_) ms20Filter_->prepare(sampleRate);
    if (prophetFilter_) prophetFilter_->prepare(sampleRate);
    if (semFilter_) semFilter_->prepare(sampleRate);
    if (tb303Filter_) tb303Filter_->prepare(sampleRate);
}

void ZenithFilter::setCutoff(float cutoffHz) {
    float cutoff = juce::jlimit(20.0f, 20000.0f, cutoffHz);
    cutoffSmoothed_.setTargetValue(cutoff);
    
    // Update circuit filters
    if (moogFilter_) moogFilter_->setCutoff(cutoff);
    if (ms20Filter_) ms20Filter_->setCutoff(cutoff);
    if (prophetFilter_) prophetFilter_->setCutoff(cutoff);
    if (semFilter_) semFilter_->setCutoff(cutoff);
    if (tb303Filter_) tb303Filter_->setCutoff(cutoff);
}

void ZenithFilter::setResonance(float resonance) {
    float res = juce::jlimit(0.0f, 1.0f, resonance);
    resonanceSmoothed_.setTargetValue(res);
    
    // Update circuit filters
    if (moogFilter_) moogFilter_->setResonance(res);
    if (ms20Filter_) ms20Filter_->setResonance(res);
    if (prophetFilter_) prophetFilter_->setResonance(res);
    if (semFilter_) semFilter_->setResonance(res);
    if (tb303Filter_) tb303Filter_->setResonance(res);
}

void ZenithFilter::reset() {
    ic1eq_ = ic2eq_ = 0.0f;
    cutoffSmoothed_.setCurrentAndTargetValue(1000.0f);
    resonanceSmoothed_.setCurrentAndTargetValue(0.0f);
    
    // Reset circuit filters
    if (moogFilter_) moogFilter_->reset();
    if (ms20Filter_) ms20Filter_->reset();
    if (prophetFilter_) prophetFilter_->reset();
    if (semFilter_) semFilter_->reset();
    if (tb303Filter_) tb303Filter_->reset();
}

void ZenithFilter::initializeFilters() {
    // Lazy initialization of circuit filters
    if (!moogFilter_) {
        moogFilter_.reset(new MoogLadderFilter());
        moogFilter_->prepare(sampleRate_);
    }
    if (!ms20Filter_) {
        ms20Filter_.reset(new MS20LowpassFilter());
        ms20Filter_->prepare(sampleRate_);
    }
    if (!prophetFilter_) {
        prophetFilter_.reset(new Prophet5Filter());
        prophetFilter_->prepare(sampleRate_);
    }
    if (!semFilter_) {
        semFilter_.reset(new SEMFilter());
        semFilter_->prepare(sampleRate_);
    }
    if (!tb303Filter_) {
        tb303Filter_.reset(new TB303Filter());
        tb303Filter_->prepare(sampleRate_);
    }
}

float ZenithFilter::processSample(float input) {
    // Apply oversampling if enabled
    if (oversamplingFactor_ > 1) {
        return processWithOversampling(input);
    }
    
    // Route to appropriate filter model
    switch (static_cast<FilterModelType>(model_)) {
        case FilterModelType::MoogLadder:
            initializeFilters();
            if (moogFilter_) {
                moogFilter_->setDrive(drive_);
                return moogFilter_->processSample(input);
            }
            break;
            
        case FilterModelType::MS20:
            initializeFilters();
            if (ms20Filter_) {
                ms20Filter_->setDrive(drive_);
                return ms20Filter_->processSample(input);
            }
            break;
            
        case FilterModelType::Prophet:
            initializeFilters();
            if (prophetFilter_) {
                prophetFilter_->setDrive(drive_);
                return prophetFilter_->processSample(input);
            }
            break;
            
        case FilterModelType::SEM:
            initializeFilters();
            if (semFilter_) {
                semFilter_->setType(type_);
                semFilter_->setDrive(drive_);
                return semFilter_->processSample(input);
            }
            break;
            
        case FilterModelType::TB303:
            initializeFilters();
            if (tb303Filter_) {
                tb303Filter_->setDrive(drive_);
                return tb303Filter_->processSample(input);
            }
            break;
            
        case FilterModelType::Ladder:
            // Legacy ladder (backward compatibility)
            return processSVF(input); // Fallback to SVF for now
            
        case FilterModelType::SVF:
        default:
            return processSVF(input);
    }
    
    return input; // Should not reach here
}

float ZenithFilter::processWithOversampling(float input) {
    // Simplified oversampling for now - just run filter at higher rate
    // TODO: Implement proper oversampling with downsampling filters
    
    // For now, just return the non-oversampled version
    // The 4x oversampling is already built into the circuit filters
    return processSample(input);
}

void ZenithFilter::processBlock(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    float* writePtr = buffer.getWritePointer(0);
    
    for (int i = 0; i < numSamples; ++i) {
        writePtr[i] = processSample(writePtr[i]);
    }
}

float ZenithFilter::processSVF(float input) {
    // State Variable Filter (transistor ladder approximation)
    // Good for CPU efficiency, but not circuit-accurate
    
    float cutoff = cutoffSmoothed_.getNextValue();
    float resonance = resonanceSmoothed_.getNextValue();
    
    // Apply drive
    if (drive_ > 1.0f) {
        input *= drive_;
        input = juce::jlimit(input, -1.0f, 1.0f); // Simple hard clip
        if (input > 0.9f) input = 0.9f;
        if (input < -0.9f) input = -0.9f;
    }
    
    // Simplified calculation - for full accuracy we'd use proper tan()
    // For now use approximation that works for typical filter ranges
    float wc = juce::MathConstants<float>::pi * cutoff;
    float T = 1.0f / static_cast<float>(sampleRate_);
    float g = wc * T; // First order approximation for small wc*T
    
    float k = 2.0f - 2.0f * resonance;
    
    float gk = g + k;
    float a1 = 1.0f / (1.0f + g * gk);
    float a2 = g * a1;
    float a3 = g * a2;
    
    float v0 = input;
    float v1 = a1 * ic1eq_ + a2 * (v0 - ic2eq_);
    float v2 = ic2eq_ + a2 * ic1eq_ + a3 * (v0 - ic2eq_);
    
    ic1eq_ = 2.0f * v1 - ic1eq_;
    ic2eq_ = 2.0f * v2 - ic2eq_;
    
    switch (type_) {
        case FilterType::Lowpass:  return v2;
        case FilterType::Bandpass: return v1;
        case FilterType::Highpass: return v0 - k * v1 - v2;
        default: return v2;
    }
}

} // namespace zenith
