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

#include "ZenithDualFilter.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithDualFilter::ZenithDualFilter() {
    tempBuffer_ = juce::AudioBuffer<float>(2, 256);
}

void ZenithDualFilter::setSampleRate(double sr) {
    sampleRate_ = sr;
    filter1_.setSampleRate(sr);
    filter2_.setSampleRate(sr);
}

void ZenithDualFilter::reset() {
    filter1_.reset();
    filter2_.reset();
    lastLeft_ = 0.0f;
    lastRight_ = 0.0f;
}

//==============================================================================
// PROCESSING
//==============================================================================

float ZenithDualFilter::processSample(float& left, float& right) {
    float mono = (left + right) * 0.5f;
    float midiNote = 60.0f;

    float outLeft = 0.0f;
    float outRight = 0.0f;

    switch (routing_) {
        case RoutingMode::Serial:
            outLeft = outRight = processSerial(mono, midiNote);
            break;

        case RoutingMode::Parallel:
            outLeft = outRight = processParallel(mono, midiNote);
            break;

        case RoutingMode::Split:
            processSplit(mono, midiNote, outLeft, outRight);
            break;

        case RoutingMode::Stereo:
            processStereo(left, right, midiNote);
            break;

        case RoutingMode::WetDry:
            float dry = processSerial(mono, midiNote);
            outLeft = outRight = dry;
            break;
    }

    left = outLeft;
    right = outRight;

    return outLeft;
}

void ZenithDualFilter::process(juce::AudioBuffer<float>& buffer) {
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i) {
        float& left = buffer.getWritePointer(0)[i];
        float& right = buffer.getWritePointer(1)[i];
        left = right = processSample(left, right);
    }
}

//==============================================================================
// ROUTING MODES
//==============================================================================

float ZenithDualFilter::processSerial(float input, float midiNote) {
    float f1 = filter1_.processSample(input, midiNote);
    float f2 = filter2_.processSample(f1, midiNote);
    return f2;
}

float ZenithDualFilter::processParallel(float input, float midiNote) {
    float f1 = filter1_.processSample(input, midiNote);
    float f2 = filter2_.processSample(input, midiNote);

    float mixF1 = f1 * mix_;
    float mixF2 = f2 * (1.0f - mix_);

    return mixF1 + mixF2;
}

void ZenithDualFilter::processSplit(float input, float midiNote, float& outLow, float& outHigh) {
    // Split based on frequency
    float splitGain = 1.0f;

    // Simple 2-way crossover at split frequency
    float low = filter1_.processSample(input, midiNote);
    float high = filter2_.processSample(input, midiNote);

    outLow = low;
    outHigh = high;
}

void ZenithDualFilter::processStereo(float left, float right, float midiNote) {
    float stereo = juce::jmax(0.01f, stereoSpread_);

    // Process each channel through each filter
    float f1Left = filter1_.processSample(left, midiNote);
    float f1Right = filter1_.processSample(right, midiNote);
    float f2Left = filter2_.processSample(left, midiNote);
    float f2Right = filter2_.processSample(right, midiNote);

    // Blend filters based on spread
    float outLeft = f1Left * (1.0f - stereo) + f2Left * stereo;
    float outRight = f1Right * (1.0f - stereo) + f2Right * (1.0f - stereo);

    lastLeft_ = outLeft;
    lastRight_ = outRight;
}

void ZenithDualFilter::processWetDry(float input, float midiNote) {
    float dry = input;
    float wet = dry;

    float wetMix = juce::jlimit(0.0f, 1.0f, mix_);

    if (wetMix > 0.0f) {
        wet = filter1_.processSample(dry, midiNote);
        wet = filter2_.processSample(wet, midiNote);
    }

    return dry * (1.0f - wetMix) + wet * wetMix;
}

} // namespace zenith
