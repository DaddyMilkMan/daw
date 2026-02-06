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

// MasterLimiter.cpp - Master limiter implementation

#include "MasterLimiter.h"

namespace zenith {

MasterLimiter::MasterLimiter() {
    DBG("MasterLimiter: Constructor - Simple peak limiter");
}

MasterLimiter::~MasterLimiter() {
    DBG("MasterLimiter: Destructor");
}

void MasterLimiter::process(const float* input, float* output, int numSamples) {
    if (!input || !output || numSamples <= 0)
        return;

    // Simple peak limiting implementation
    for (int sample = 0; sample < numSamples; ++sample) {
        float sampleValue = input[sample];

        // Calculate RMS (simplified)
        rms_ += juce::jlimit(0.0f, 1.0f, std::abs(sampleValue));

        // Simple gain reduction based on threshold
        float level = juce::Decibels::decibelsToGain(threshold_);
        if (std::abs(sampleValue) > level) {
            gainReduction_ = level / std::abs(sampleValue);
        } else {
            gainReduction_ = 1.0f;
        }

        // Apply gain reduction with smoothing
        float targetGain = gainReduction_;
        float gainDiff = targetGain - gainReduction_;
        gainReduction_ += gainDiff * 0.1f; // Simple smoothing

        // Apply ceiling
        float ceilingLevel = juce::Decibels::decibelsToGain(ceiling_);
        output[sample] = sampleValue * gainReduction_ * ceilingLevel;
    }

    // Decay RMS
    rms_ *= 0.999f;

    // Update reduction amount for metering
    reduction_ = juce::Decibels::gainToDecibels(gainReduction_);
}

void MasterLimiter::process(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumSamples() <= 0)
        return;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        process(buffer.getReadPointer(channel),
                buffer.getWritePointer(channel),
                buffer.getNumSamples());
    }
}

void MasterLimiter::setThreshold(float threshold) {
    threshold_ = threshold;
    DBG("MasterLimiter: Threshold set to " + juce::String(threshold) + " dB");
}

void MasterLimiter::setCeiling(float ceiling) {
    ceiling_ = ceiling;
    DBG("MasterLimiter: Ceiling set to " + juce::String(ceiling) + " dB");
}

void MasterLimiter::setAttack(float attack) {
    attack_ = attack;
    DBG("MasterLimiter: Attack set to " + juce::String(attack) + " ms");
}

void MasterLimiter::setRelease(float release) {
    release_ = release;
    DBG("MasterLimiter: Release set to " + juce::String(release) + " ms");
}

float MasterLimiter::getReduction() const {
    return reduction_;
}

void MasterLimiter::reset() {
    rms_ = 0.0f;
    envelope_ = 0.0f;
    gainReduction_ = 1.0f;
    reduction_ = 0.0f;
    DBG("MasterLimiter: Reset");
}

void MasterLimiter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    samplesPerBlock_ = samplesPerBlock;
    reset();
    DBG("MasterLimiter: Prepared to play - Sample rate: " + juce::String(sampleRate) +
        ", Block size: " + juce::String(samplesPerBlock));
}

} // namespace zenith