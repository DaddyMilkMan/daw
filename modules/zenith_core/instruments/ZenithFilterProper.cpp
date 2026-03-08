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

#include "ZenithFilterProper.h"

namespace zenith {

//==============================================================================
// CONSTRUCTOR/DESTRUCTOR
//==============================================================================

ZenithFilterProper::~ZenithFilterProper() = default;

//==============================================================================
// SAMPLE RATE
//==============================================================================

void ZenithFilterProper::setSampleRate(double sr) {
    sampleRate_ = sr;
    reset();
}

void ZenithFilterProper::reset() {
    svf_.low = svf_.band = svf_.high = 0.0;
    moogState_ = {};
    ms20State_ = {};
    semState_ = {};
    tb303State_ = {};
    if (oversampler2x_) oversampler2x_->reset();
    if (oversampler4x_) oversampler4x_->reset();
    oversamplerBuffer_.clear();
}

//==============================================================================
// OVERSAMPLING SETUP
//==============================================================================

void ZenithFilterProper::setOversampling(int factor) {
    factor = juce::jlimit(1, 4, factor);
    if (factor == oversamplingFactor_) return;

    oversamplingFactor_ = factor;

    if (factor >= 2 && !oversampler2x_) {
        oversampler2x_ = std::make_unique<juce::dsp::Oversampling<float>>(
            1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);
    }

    if (factor >= 4 && !oversampler4x_) {
        oversampler4x_ = std::make_unique<juce::dsp::Oversampling<float>>(
            1, 4, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);
    }

    reset();
}

//==============================================================================
// PROCESSING
//==============================================================================

float ZenithFilterProper::processSample(float input, float midiNote) {
    float adjustedCutoff = calculateCutoffWithKeyTrack(midiNote);
    float driven = applyDrive(input);

    if (oversamplingFactor_ == 1) {
        return processSVF(driven);
    }

    // Select appropriate filter model at oversampled rate
    switch (model_) {
        case FilterModelType::Moog:    return processMoog(driven);
        case FilterModelType::MS20:    return processMS20(driven);
        case FilterModelType::SEM:     return processSEM(driven);
        case FilterModelType::TB303:   return processTB303(driven);
        default: return processSVF(driven);
    }
}

float ZenithFilterProper::calculateCutoffWithKeyTrack(float midiNote) {
    float offset = (midiNote - 60.0f) * keyTrackAmount_;
    return juce::jlimit(20.0f, 20000.0f, cutoff_ * std::exp2(offset / 12.0f));
}

float ZenithFilterProper::applyDrive(float sample) {
    if (drive_ <= 0.0f) return sample;
    float gain = 1.0f + drive_ * 9.0f;
    return softClip(sample * gain) / gain;
}

//==============================================================================
// SVF - Chamberlin State Variable Filter
//==============================================================================

float ZenithFilterProper::processSVF(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double f = std::tan(juce::MathConstants<double>::pi * cutoff_ / fs);
    double r = juce::jlimit(0.0, 2.0, 1.0 - resonance_ * 0.99);

    double low = svf_.low;
    double high = svf_.high;
    double band = svf_.band;

    double lowOut = low + f * band;
    double highOut = input - lowOut - r * band;
    double bandOut = band + f * highOut;

    svf_.low = lowOut;
    svf_.high = highOut;
    svf_.band = bandOut;

    switch (type_) {
        case FilterType::LowPass:  return static_cast<float>(lowOut);
        case FilterType::HighPass: return static_cast<float>(highOut);
        case FilterType::BandPass: return static_cast<float>(bandOut);
        case FilterType::Notch:    return static_cast<float>(lowOut + highOut);
        default: return static_cast<float>(lowOut);
    }
}

//==============================================================================
// MOOG LADDER
//==============================================================================

float ZenithFilterProper::processMoog(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double wc = 2.0 * juce::MathConstants<double>::pi * cutoff_;
    double g = std::tan(wc / (4.0 * fs));
    double g2 = g * g;
    double g3 = g2 * g;
    double g4 = g3 * g;

    double res = resonance_ * 4.0;

    double feedback = res * (moogState_.s4 - moogState_.s3);

    double y0 = input - feedback;
    double y1 = moogState_.s1 + g * (y0 - moogState_.s1);
    double y2 = moogState_.s2 + g * (y1 - moogState_.s2);
    double y3 = moogState_.s3 + g * (y2 - moogState_.s3);
    double y4 = moogState_.s4 + g * (y3 - moogState_.s4);

    moogState_.s1 = y1;
    moogState_.s2 = y2;
    moogState_.s3 = y3;
    moogState_.s4 = y4;

    return static_cast<float>(y4);
}

//==============================================================================
// MS-20
//==============================================================================

float ZenithFilterProper::processMS20(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double g = std::tan(juce::MathConstants<double>::pi * cutoff_ / fs);

    double hp = input - ms20State_.hp - (ms20State_.lp1 * resonance_ * 0.95);
    double lp1 = ms20State_.lp1 + g * hp;
    double lp2 = ms20State_.lp2 + g * lp1;

    ms20State_.hp = hp;
    ms20State_.lp1 = lp1;
    ms20State_.lp2 = lp2;

    return static_cast<float>(lp2);
}

//==============================================================================
// SEM
//==============================================================================

float ZenithFilterProper::processSEM(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double f = std::tan(juce::MathConstants<double>::pi * cutoff_ / fs);

    double high = semState_.high + f * (input - semState_.low - semState_.high);
    double low = semState_.low + f * high;
    double band = high - low;

    semState_.high = high;
    semState_.low = low;

    switch (type_) {
        case FilterType::LowPass:  return static_cast<float>(low);
        case FilterType::HighPass: return static_cast<float>(high);
        case FilterType::BandPass: return static_cast<float>(band);
        case FilterType::Notch:    return static_cast<float>(low + high);
        default: return static_cast<float>(low);
    }
}

//==============================================================================
// TB-303
//==============================================================================

float ZenithFilterProper::processTB303(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double wc = 2.0 * juce::MathConstants<double>::pi * cutoff_;
    double g = std::tan(wc / (4.0 * fs));

    double res = resonance_ * 0.99;

    double feedback = res * tb303State_.s4;
    double y0 = input - feedback;

    double y1 = tb303State_.s1 + g * (y0 - tb303State_.s1);
    double y2 = tb303State_.s2 + g * (y1 - tb303State_.s2);
    double y3 = tb303State_.s3 + g * (y2 - tb303State_.s3);
    double y4 = tb303State_.s4 + g * (y3 - tb303State_.s4);

    tb303State_.s1 = y1;
    tb303State_.s2 = y2;
    tb303State_.s3 = y3;
    tb303State_.s4 = y4;

    return static_cast<float>(y4);
}

//==============================================================================
// BLOCK PROCESSING
//==============================================================================

void ZenithFilterProper::process(juce::AudioBuffer<float>& buffer, float midiNote) {
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channel = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            channel[i] = processSample(channel[i], midiNote);
        }
    }
}

} // namespace zenith
