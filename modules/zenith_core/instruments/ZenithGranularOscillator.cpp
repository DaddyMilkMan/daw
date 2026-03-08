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

#include "ZenithGranularOscillator.h"
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithGranularOscillator::ZenithGranularOscillator() {
    grains_.fill({});
}

//==============================================================================
// SOURCE SAMPLE
//==============================================================================

void ZenithGranularOscillator::setSource(const float* data, int numChannels,
                                        int numSamples, double sampleRate) {
    // Convert to stereo if mono
    int targetChannels = juce::jmax(numChannels, 2);
    targetSamples_ = numSamples;
    sourceRate_ = sampleRate;

    sourceData_.resize(static_cast<size_t>(numSamples * targetChannels));
    for (int i = 0; i < numSamples; ++i) {
        for (int ch = 0; ch < targetChannels; ++ch) {
            float value = (ch < numChannels) ? data[i] : 0.0f;
            sourceData_[i * targetChannels + ch] = value;
        }
    }
}

void ZenithGranularOscillator::clearSource() {
    sourceData_.clear();
    targetSamples_ = 0;
}

//==============================================================================
// PARAMETERS
//==============================================================================

void ZenithGranularOscillator::setGrainSize(float ms) {
    grainSize_ = juce::jlimit(1.0f, 100.0f, ms);
}

void ZenithGranularOscillator::setEnvelopeShape(EnvelopeShape shape) {
    envShape_ = shape;
}

void ZenithGranularOscillator::setGrainAttack(float ms) {
    grainAttack_ = juce::jlimit(0.0f, 100.0f, ms);
}

void ZenithGranularOscillator::setGrainDecay(float ms) {
    grainDecay_ = juce::jlimit(0.0f, 100.0f, ms);
}

void ZenithGranularOscillator::setDensity(float grainsPerSec) {
    density_ = juce::jlimit(0.1f, 100.0f, grainsPerSec);
}

void ZenithGranularOscillator::setRandomSpread(float spread) {
    randomSpread_ = juce::jlimit(0.0f, 1.0f, spread);
}

void ZenithGranularOscillator::setPitchVariation(float semitones) {
    pitchVar_ = semitones;
}

void ZenithGranularOscillator::setFreeze(bool freeze) {
    freeze_ = freeze;
}

void ZenithGranularOscillator::setStereoSpread(float spread) {
    stereoSpread_ = juce::jlimit(0.0f, 1.0f, spread);
}

//==============================================================================
// AUDIO GENERATION
//==============================================================================

void ZenithGranularOscillator::spawnGrain() {
    if (sourceData_.empty()) return;

    // Pick random position based on random spread
    double range = targetSamples_ * (1.0 - randomSpread_);
    double pos = random_.nextDouble() * range;

    // Clamp to available samples
    pos = juce::jmax(0.0, juce::jmin(pos, static_cast<double>(targetSamples_ - 1)));

    // Calculate grain parameters
    double grainLen = (grainSize_ / 1000.0) * sourceRate_;
    double grainSamples = grainLen * sourceRate_;

    // Setup grain
    Grain& grain = grains_[nextGrainIndex_];
    grain.position = pos;
    grain.phase = 0.0;
    grain.pan = (random_.nextDouble() * 2.0 - 1.0) * stereoSpread_;
    grain.pitch = (random_.nextDouble() * 2.0 - 1.0) * pitchVar_;
    grain.speed = 1.0;
    grain.active = true;

    // Envelope from duration
    grain.envPhase = 0.0;
    double attackSamples = (grainAttack_ / 1000.0) * sourceRate_;
    double decaySamples = (grainDecay_ / 1000.0) * sourceRate_;
    grain.envTotal = attackSamples + decaySamples;
}

float ZenithGranularOscillator::getGrainEnvelope(double phase, EnvelopeShape shape) {
    double t = juce::jmax(0.0, juce::jmin(phase, 1.0));

    switch (shape) {
        case EnvelopeShape::Linear:
            return static_cast<float>(t);

        case EnvelopeShape::Exponential:
            return static_cast<float>(t * t);

        case EnvelopeShape::Bell:
            // Bell: fast attack, slow decay
            if (t < 0.5) {
                return static_cast<float>(2.0 * t - t * t);
            } else {
                return static_cast<float>(2.0 * (1.0 - t) * (1.0 - t));
            }

        case EnvelopeShape::Sine:
            return static_cast<float>(0.5 - 0.5 * std::cos(t * juce::MathConstants<double>::pi));

        default:
            return static_cast<float>(t);
    }
}

float ZenithGranularOscillator::readSourceStereo(double pos, float pan, float& outL, float& outR) {
    int index1 = static_cast<int>(pos);
    int index2 = juce::jmin(index1 + 1, targetSamples_ - 1);

    if (index1 < 0 || index2 >= targetSamples_) {
        outL = outR = 0.0f;
        return;
    }

    // Linear interpolation
    float frac = pos - index1;
    outL = sourceData_[index1 * 2 + 0] * (1.0f - frac) + sourceData_[index2 * 2 + 0] * frac;
    outR = sourceData_[index1 * 2 + 1] * (1.0f - frac) + sourceData_[index2 * 2 + 1] * frac;

    // Apply stereo pan
    float leftGain = std::cos(pan * juce::MathConstants<float>::pi * 0.25f);
    float rightGain = std::sin(pan * juce::MathConstants<float>::pi * 0.25f);

    float left = outL * leftGain;
    float right = outR * rightGain;

    return left + right;
}

void ZenithGranularOscillator::process(float* left, float* right, int numSamples, float frequency) {
    // Process at output rate (44100 typically)
    double outputSampleRate = 44100.0;
    double timePerSample = 1.0 / outputSampleRate;
    double timeAccumulator = 0.0;
    double samplesUntilNextGrain = outputSampleRate / density_;

    for (int i = 0; i < numSamples; ++i) {
        // Check if new grain needed
        bool activeGrains = false;
        for (auto& g : grains_) {
            if (g.active) {
                activeGrains = true;
                break;
            }
        }

        if (!activeGrains || freeze_) {
            spawnGrain();
        }

        // Process active grains
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (auto& g : grains_) {
            if (!g.active) continue;

            // Get source sample
            float srcL, srcR;
            readSourceStereo(g.position, g.pan, srcL, srcR);

            // Get envelope value
            float env = getGrainEnvelope(g.envPhase, envShape_);

            // Pitch shift
            double pitchMult = std::exp2(g.pitch * pitchVar_ / 12.0);
            double pitchFactor = pitchMult * frequency;

            // Mix into output
            sampleL += srcL * env * g.speed * pitchFactor;
            sampleR += srcR * env * g.speed * pitchFactor;

            // Advance grain
            double phaseInc = g.speed / sourceRate_;
            g.envPhase = juce::jmin(1.0, g.envPhase + phaseInc);
            if (g.envPhase >= 1.0) {
                g.active = false;
            }
        }

        // Normalize by active grains
        if (activeGrains > 0) {
            float norm = 1.0f / std::sqrt(static_cast<float>(activeGrains));
            left[i] = sampleL * norm;
            right[i] = sampleR * norm;
        } else {
            left[i] = 0.0f;
            right[i] = 0.0f;
        }

        // Advance time
        timeAccumulator += timePerSample;
        samplesUntilNextGrain -= 1.0;
    }
}

} // namespace zenith