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

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithFilter::ZenithFilter() {
    svf_ = {};
}

void ZenithFilter::reset() {
    svf_ = {};
    oversampler_.reset();
}

void ZenithFilter::setOversampling(int factor) {
    factor = juce::jlimit(1, 4, factor);
    oversamplingFactor_ = factor;

    if (factor >= 2) {
        oversampler2x_ = std::make_unique<juce::dsp::Oversampling<float>>(
            2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
        );
    }
    if (factor >= 4) {
        oversampler4x_ = std::make_unique<juce::dsp::Oversampling<float>>(
            4, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
        );
    }
}

//==============================================================================
// PROCESSING
//==============================================================================

float ZenithFilter::processSample(float input, float midiNote) {
    float adjustedCutoff = calculateCutoffWithKeyTrack(midiNote);

    // Apply drive (pre-filter saturation)
    float driven = applyDrive(input);

    // Process at base rate
    float output;
    switch (model_) {
        case FilterModelType::SVF:   output = processSVF(driven); break;
        case FilterModelType::Moog:  output = processMoog(driven); break;
        case FilterModelType::MS20:  output = processMS20(driven); break;
        case FilterModelType::SEM:   output = processSEM(driven); break;
        case FilterModelType::TB303: output = processTB303(driven); break;
        default: output = processSVF(driven);
    }

    return output;
}

float ZenithFilter::calculateCutoffWithKeyTrack(float midiNote) {
    float offset = (midiNote - 60.0f) * keyTrackAmount_;
    return juce::jlimit(20.0f, 20000.0f, cutoff_ * std::exp2(offset / 12.0f));
}

float ZenithFilter::applyDrive(float sample) {
    if (drive_ <= 0.0f) return sample;
    float gain = 1.0f + drive_ * 9.0f;
    return softClip(sample * gain) / gain;
}

//==============================================================================
// SVF (State Variable Filter) - CHAMBERLIN
//==============================================================================

float ZenithFilter::processSVF(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double f = cutoff_ / fs;
    double q = resonance_ * 10.0 + 1.0;
    double r = 1.0 / q;

    double& low = svf_.low;
    double& high = svf_.high;
    double& band = svf_.band;

    // Chamberlin SVF (improved)
    double low1 = low + f * band;
    double high1 = input - low1 - r * band;
    double band1 = band + f * high1;

    low = low1;
    high = high1;
    band = band1;

    switch (type_) {
        case FilterType::LowPass:  return static_cast<float>(low);
        case FilterType::HighPass: return static_cast<float>(high);
        case FilterType::BandPass: return static_cast<float>(band);
        case FilterType::Notch:    return static_cast<float>(high + low);
        default: return static_cast<float>(low);
    }
}

//==============================================================================
// MOOG LADDER - TRANSPARENT LADDER
//==============================================================================

float ZenithFilter::processMoog(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double wc = 2.0 * juce::MathConstants<double>::pi * cutoff_;
    double g = std::tan(wc / (4.0 * fs));
    double g2 = g * g;
    double g3 = g2 * g;
    double g4 = g3 * g;

    double res = resonance_;
    double feedback = res * 0.5;

    struct { double s1 = 0, double s2 = 0, double s3 = 0, double s4 = 0; } st;

    double y = input - feedback * st.s4;

    double s1_in = y;
    st.s1 += g * (s1_in - st.s1);
    double s1_out = st.s1;

    double s2_in = s1_out;
    st.s2 += g * (s2_in - st.s2);
    double s2_out = st.s2;

    double s3_in = s2_out;
    st.s3 += g * (s3_in - st.s3);
    double s3_out = st.s3;

    double s4_in = s3_out;
    st.s4 += g * (s4_in - st.s4);
    double s4_out = st.s4;

    // Nonlinear feedback
    double tanh4 = std::tanh(s4_out);
    double yOut = s4_out + feedback * tanh4;

    switch (type_) {
        case FilterType::LowPass:  return static_cast<float>(yOut);
        case FilterType::HighPass: return static_cast<float>(input - yOut);
        case FilterType::BandPass: return static_cast<float>(s2_out - s4_out);
        default: return static_cast<float>(yOut);
    }
}

//==============================================================================
// KORG MS-20 - HIGH PASS + LOW PASS
//==============================================================================

float ZenithFilter::processMS20(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double g = std::tan(juce::MathConstants<double>::pi * cutoff_ / fs);

    struct { double hp = 0, double lp1 = 0, double lp2 = 0; } st;

    // Highpass section
    double hp = input - st.hp;
    st.hp += g * hp;

    // Lowpass sections (2-pole)
    double lp1 = st.lp1 + g * (hp - st.lp1);
    st.lp1 = lp1;

    double lp2 = st.lp2 + g * (lp1 - st.lp2);
    st.lp2 = lp2;

    // Resonance
    double res = resonance_ * 0.95;
    double feedback = res * lp2;
    double output = lp2 - feedback;

    return static_cast<float>(output);
}

//==============================================================================
// OBERHEIM SEM - STATE VARIABLE
//==============================================================================

float ZenithFilter::processSEM(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double f = std::tan(juce::MathConstants<double>::pi * cutoff_ / (2.0 * fs));

    struct { double low = 0, double high = 0; } st;

    double lowOut = st.low + f * high;
    double bandOut = high - f * (high + lowOut - input);
    st.low = lowOut;
    st.high = bandOut;

    double res = resonance_ * 2.0;
    double output = lowOut + res * bandOut;

    switch (type_) {
        case FilterType::LowPass:  return static_cast<float>(output);
        case FilterType::HighPass: return static_cast<float>(st.high);
        case FilterType::BandPass: return static_cast<float>(bandOut);
        default: return static_cast<float>(output);
    }
}

//==============================================================================
// ROLAND TB-303 - DIODE LADDER
//==============================================================================

float ZenithFilter::processTB303(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double wc = 2.0 * juce::MathConstants<double>::pi * cutoff_;
    double g = std::tan(wc / (4.0 * fs));

    struct { double s1 = 0, double s2 = 0, double s3 = 0, double s4 = 0; } st;

    double res = resonance_ * 0.99;

    double y = input;
    for (int i = 0; i < 4; ++i) {
        double& si = (i == 0) ? st.s1 : (i == 1) ? st.s2 : (i == 2) ? st.s3 : st.s4;
        double siIn = si + g * (y - si);
        si = siIn;
        y = siIn;
    }

    double tanhOut = std::tanh(st.s4);
    double output = (input - res * tanhOut);

    switch (type_) {
        case FilterType::LowPass:  return static_cast<float>(st.s4);
        case FilterType::HighPass:  return static_cast<float>(input - st.s4);
        default: return static_cast<float>(st.s4);
    }
}

//==============================================================================
// BLOCK PROCESSING
//==============================================================================

void ZenithFilter::process(juce::AudioBuffer<float>& buffer, float midiNote) {
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
