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

#include "UnisonManager.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// UnisonVoice Implementation
//==============================================================================

UnisonVoice::UnisonVoice()
    : phase_(0.0)
    , phaseIncrement_(0.0)
    , detuneCents_(0.0f)
    , pan_(0.0f)
    , leftGain_(0.707f)
    , rightGain_(0.707f)
    , gain_(1.0f)
{
}

void UnisonVoice::setDetune(float cents) {
    detuneCents_ = cents;
    updateFrequencyRatio();
}

void UnisonVoice::setPan(float pan) {
    pan_ = juce::jlimit(-1.0f, 1.0f, pan);
    updateGains();
}

void UnisonVoice::setGain(float gain) {
    gain_ = juce::jlimit(0.0f, 1.0f, gain);
    updateGains();
}

void UnisonVoice::setPhase(float phase) {
    phase_ = juce::jlimit(0.0f, 1.0f, phase);
}

void UnisonVoice::reset() {
    phase_ = 0.0f;
    detuneCents_ = 0.0f;
    pan_ = 0.0f;
    gain_ = 1.0f;
    updateFrequencyRatio();
    updateGains();
}

float UnisonVoice::processSample(float frequency, double sampleRate) {
    // Calculate phase increment for this frequency
    phaseIncrement_ = static_cast<float>(frequency / sampleRate);

    // Advance phase
    phase_ += phaseIncrement_;
    if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
    }

    // Output is the phase (0 to 1) scaled to -1 to 1 for oscillator input
    // This represents a normalized saw/square phase that oscillators can use
    return phase_ * 2.0f - 1.0f;
}

void UnisonVoice::updateFrequencyRatio() {
    // Convert cents to frequency ratio
    // ratio = 2^(cents/1200)
    frequencyRatio_ = std::exp2(detuneCents_ / 1200.0f);
}

void UnisonVoice::updateGains() {
    // Equal power panning: use sin/cos for constant power
    float angle = (pan_ + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    leftGain_ = std::cos(angle) * gain_;
    rightGain_ = std::sin(angle) * gain_;
}

//==============================================================================
// UnisonManager Implementation
//==============================================================================

UnisonManager::UnisonManager()
    : voices_()
    , numVoices_(1)
    , detune_(5.0f)
    , spread_(0.5f)
    , panRandom_(false)
    , sampleRate_(44100.0)
{
    // voices_ is already fixed-size std::array<UnisonVoice, 16>

    // Initialize default pan positions for even stereo spread
    for (int i = 0; i < 16; ++i) {
        float t = (16.0f > 1) ? static_cast<float>(i) / 15.0f : 0.5f;
        float pan = t * 2.0f - 1.0f;
        voices_[i].setPan(pan);
    }

    // Initialize default detune spread
    updateVoiceDetunes();
}

void UnisonManager::setNumVoices(int voices) {
    numVoices_ = juce::jlimit(1, 16, voices);

    // Recalculate detune and pan distributions
    updateVoiceDetunes();
    updateVoicePans();
}

void UnisonManager::setDetune(float cents) {
    detune_ = juce::jlimit(0.0f, 100.0f, cents);
    updateVoiceDetunes();
}

void UnisonManager::setSpread(float spread) {
    spread_ = juce::jlimit(0.0f, 1.0f, spread);
    updateVoicePans();
}

void UnisonManager::setPanRandom(bool randomize) {
    panRandom_ = randomize;
    updateVoicePans();
}

void UnisonManager::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void UnisonManager::reset() {
    for (int i = 0; i < 16; ++i) {
        voices_[i].reset();
    }
    updateVoiceDetunes();
    updateVoicePans();
}

void UnisonManager::process(float baseFrequency, float* left, float* right, int numSamples) {
    if (numVoices_ <= 0 || numVoices_ > 16 || numSamples <= 0) {
        return;
    }

    // Calculate volume compensation to prevent clipping
    // With N voices, each should be scaled by 1/sqrt(N) to maintain RMS level
    float compensation = calculateVolumeCompensation();

    // Clear output buffers
    juce::FloatVectorOperations::clear(left, numSamples);
    juce::FloatVectorOperations::clear(right, numSamples);

    // Process each unison voice for the full block
    for (int v = 0; v < numVoices_; ++v) {
        auto& voice = voices_[v];

        // Get detuned frequency for this voice
        float detuneFreq = baseFrequency * voice.getFrequencyRatio();

        // Process all samples for this voice
        for (int i = 0; i < numSamples; ++i) {
            // Get the phase output from this unison voice
            // This provides a normalized saw/square wave (-1 to +1)
            float sample = voice.processSample(detuneFreq, sampleRate_);

            // Accumulate with stereo panning
            left[i] += sample * voice.getLeftGain() * compensation;
            right[i] += sample * voice.getRightGain() * compensation;
        }
    }
}

float UnisonManager::calculateVolumeCompensation() const {
    // Scale by 1/sqrt(N) to maintain RMS level with multiple voices
    return 1.0f / std::sqrt(static_cast<float>(numVoices_));
}

void UnisonManager::updateVoiceDetunes() {
    // Calculate detune for each voice
    // Voices are detuned symmetrically around the center frequency
    // For example, with 4 voices and 10 cents spread:
    // Voice 0: -15 cents, Voice 1: -5 cents, Voice 2: +5 cents, Voice 3: +15 cents

    if (numVoices_ <= 1) {
        for (int i = 0; i < 16; ++i) {
            voices_[i].setDetune(0.0f);
        }
        return;
    }

    // Spread detune evenly across voices
    float totalSpread = detune_;
    float step = totalSpread / static_cast<float>(juce::jmax(1, numVoices_ - 1));

    for (int i = 0; i < numVoices_; ++i) {
        float offset = (static_cast<float>(i) - static_cast<float>(numVoices_ - 1) * 0.5f) * totalSpread;
        voices_[i].setDetune(offset);
    }
}

void UnisonManager::updateVoicePans() {
    if (panRandom_) {
        // Random pan positions
        juce::Random rng;
        for (int i = 0; i < numVoices_; ++i) {
            float pan = rng.nextFloat() * 2.0f - 1.0f;
            voices_[i].setPan(pan);
        }
    } else {
        // Even spread from left to right
        for (int i = 0; i < numVoices_; ++i) {
            float t = (numVoices_ > 1) ? static_cast<float>(i) / static_cast<float>(numVoices_ - 1) : 0.5f;
            float pan = t * 2.0f - 1.0f;
            voices_[i].setPan(pan);
        }
    }

    // Apply spread control: when spread is 0, all voices center
    // When spread is 1, use full stereo width
    if (spread_ < 1.0f) {
        for (int i = 0; i < numVoices_; ++i) {
            float currentPan = voices_[i].getPan();
            voices_[i].setPan(currentPan * spread_);
        }
    }
}

} // namespace zenith
