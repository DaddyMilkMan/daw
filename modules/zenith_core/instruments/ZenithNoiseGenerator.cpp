/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "ZenithNoiseGenerator.h"

namespace zenith {

//==============================================================================
// CONSTRUCTOR/DESTRUCTOR
//==============================================================================

ZenithNoiseGenerator::ZenithNoiseGenerator() {
    pinkState_.fill(0.0f);
}

ZenithNoiseGenerator::~ZenithNoiseGenerator() = default;

//==============================================================================
// CONFIGURATION
//==============================================================================

void ZenithNoiseGenerator::reset() {
    pinkState_.fill(0.0f);
    pinkIndex_ = 0;
    whiteSeed_ = 0;
    brownState_ = 0.0f;
}

//==============================================================================
// PROCESSING
//==============================================================================

void ZenithNoiseGenerator::getNextSample(float& left, float& right) {
    float mono = 0.0f;

    switch (color_) {
        case NoiseColor::White:
            mono = generateWhite();
            break;
        case NoiseColor::Pink:
            mono = generatePink();
            break;
        case NoiseColor::Brown:
            mono = generateBrown();
            break;
        case NoiseColor::Sampled:
            mono = generateWhite();
            break;
    }

    mono *= gain_;
    applyStereoWidth(mono, left, right);
}

//==============================================================================
// GENERATORS
//==============================================================================

float ZenithNoiseGenerator::generateWhite() {
    // Linear congruential generator (fast, RT-safe)
    whiteSeed_ = whiteSeed_ * 1664525u + 1013904223u;

    // Convert to float range [-1, 1]
    return (static_cast<float>(whiteSeed_ >> 16) / 32767.0f) * gain_;
}

float ZenithNoiseGenerator::generatePink() {
    // 7-point Paul Kellet method for -3dB/octave
    float white = generateWhite();

    pinkState_[pinkIndex_] = white;
    pinkIndex_ = (pinkIndex_ + 1) % 7;

    // Sum and scale
    float sum = 0.0f;
    for (int i = 0; i < 7; ++i) {
        sum += pinkState_[i];
    }

    return sum * 0.1f;  // Scale for proper level
}

float ZenithNoiseGenerator::generateBrown() {
    // Brown noise is integrated white noise (leaky integrator)
    float white = generateWhite() * 2.0f - 1.0f;

    // Leaky integrator with coefficient 0.98
    brownState_ = brownState_ * 0.98f + white;

    return juce::jlimit(-1.0f, 1.0f, brownState_);
}

} // namespace zenith
