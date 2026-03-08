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

#include "ZenithFilterEnhanced.h"

namespace zenith {

//==============================================================================
// CONSTRUCTOR/DESTRUCTOR
//==============================================================================

ZenithFilterEnhanced::ZenithFilterEnhanced() = default;

ZenithFilterEnhanced::~ZenithFilterEnhanced() = default;

//==============================================================================
// SAMPLE RATE
//==============================================================================

void ZenithFilterEnhanced::setSampleRate(double sr) {
    sampleRate_ = sr;
    reset();
}

void ZenithFilterEnhanced::reset() {
    svf_ = {};
    oversamplingFactor_ = 1;
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void ZenithFilterEnhanced::setCutoff(float cutoffHz) {
    cutoff_ = juce::jlimit(20.0f, 20000.0f, cutoffHz);
}

void ZenithFilterEnhanced::setResonance(float res) {
    resonance_ = juce::jlimit(0.0f, 1.0f, res);
}

void ZenithFilterEnhanced::setDrive(float drive, SaturationCurve curve) {
    drive_ = juce::jlimit(0.0f, 1.0f, drive);
    driveCurve_ = curve;
}

void ZenithFilterEnhanced::setKeytrack(float amount, KeytrackCurve curve) {
    keytrackAmount_ = juce::jlimit(0.0f, 1.0f, amount);
    keytrackCurve_ = curve;
}

void ZenithFilterEnhanced::setOutput(FilterOutput output) {
    output_ = output;
}

void ZenithFilterEnhanced::setOversampling(int factor) {
    oversamplingFactor_ = juce::jlimit(1, 4, factor);
}

//==============================================================================
// PROCESSING
//==============================================================================

float ZenithFilterEnhanced::processSample(float input, float midiNote) {
    float adjustedCutoff = calculateCutoffWithKeytrack(midiNote);
    float driven = applySaturation(input);

    switch (model_) {
        case FilterModelType::SVF:
            return processSVF(driven);
        case FilterModelType::Moog:
            return processMoog(driven);
        case FilterModelType::MS20:
            return processMS20(driven);
        case FilterModelType::SEM:
            return processSEM(driven);
        case FilterModelType::TB303:
            return processTB303(driven);
        default:
            return driven;
    }
}

void ZenithFilterEnhanced::process(juce::AudioBuffer<float>& buffer, float midiNote) {
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channel = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            channel[i] = processSample(channel[i], midiNote);
        }
    }
}

//==============================================================================
// INTERNAL HELPERS
//==============================================================================

float ZenithFilterEnhanced::calculateCutoffWithKeytrack(float midiNote) {
    float offset = (midiNote - 60.0f) * keytrackAmount_;

    switch (keytrackCurve_) {
        case KeytrackCurve::Linear:
            return cutoff_ * std::exp2(offset / 12.0f);
        case KeytrackCurve::Exponential:
            return cutoff_ * std::exp2(offset / 12.0f * std::abs(offset / 12.0f));
        case KeytrackCurve::ReverseExp:
            return cutoff_ / std::exp2(offset / 12.0f * std::abs(offset / 12.0f));
        default:
            return cutoff_ * std::exp2(offset / 12.0f);
    }
}

float ZenithFilterEnhanced::applySaturation(float sample) {
    float driven = sample * (1.0f + drive_ * 9.0f);

    switch (driveCurve_) {
        case SaturationCurve::Soft:
            return softClip(driven);
        case SaturationCurve::Hard:
            return hardClip(driven);
        case SaturationCurve::Wavefold:
            return wavefold(driven);
        case SaturationCurve::Sine:
            return sineShape(driven);
        case SaturationCurve::Cubic:
            return cubicClip(driven);
        default:
            return softClip(driven);
    }
}

//==============================================================================
// SATURATION FUNCTIONS
//==============================================================================

float ZenithFilterEnhanced::softClip(float x) {
    return std::tanh(x);
}

float ZenithFilterEnhanced::hardClip(float x) {
    return juce::jlimit(-1.0f, 1.0f, x);
}

float ZenithFilterEnhanced::wavefold(float x) {
    // Simple wavefolder: folds signal back on itself
    while (x > 1.0f || x < -1.0f) {
        x = (x > 0.0f) ? 2.0f - x : -2.0f - x;
    }
    return x * 0.5f;
}

float ZenithFilterEnhanced::sineShape(float x) {
    // Sine shaping for smooth saturation
    return x * (1.5f - 0.5f * x * x);
}

float ZenithFilterEnhanced::cubicClip(float x) {
    // Cubic soft clipping
    return std::clamp(x, -1.0f, 1.0f) - 0.5f * std::pow(x, 3.0f);
}

//==============================================================================
// FILTER PROCESSORS
//==============================================================================

float ZenithFilterEnhanced::processSVF(float input) {
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

    return getOutput(lowOut, highOut, bandOut, input);
}

float ZenithFilterEnhanced::processMoog(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double wc = 2.0 * juce::MathConstants<double>::pi * cutoff_ / fs;
    double g = std::tan(wc / (4.0 * fs));

    double feedback = resonance_ * 4.0 * (moogState_.s4 - moogState_.s3);
    double y0 = input - feedback;

    double y1 = moogState_.s1 + g * (y0 - moogState_.s1);
    double y2 = moogState_.s2 + g * (y1 - moogState_.s2);
    double y3 = moogState_.s3 + g * (y2 - moogState_.s3);
    double y4 = moogState_.s4 + g * (y3 - moogState_.s4);

    moogState_.s1 = y1;
    moogState_.s2 = y2;
    moogState_.s3 = y3;
    moogState_.s4 = y4;

    return getOutput(y4, y4, y4, input);
}

float ZenithFilterEnhanced::processMS20(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double g = std::tan(juce::MathConstants<double>::pi * cutoff_ / fs);

    double hp = input - ms20State_.hp - (ms20State_.lp1 * resonance_ * 0.95);
    double lp1 = ms20State_.lp1 + g * hp;
    double lp2 = ms20State_.lp2 + g * lp1;

    ms20State_.hp = hp;
    ms20State_.lp1 = lp1;
    ms20State_.lp2 = lp2;

    return getOutput(lp2, lp2, lp2, input);
}

float ZenithFilterEnhanced::processSEM(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double f = std::tan(juce::MathConstants<double>::pi * cutoff_ / fs);

    double high = semState_.high + f * (input - semState_.low - semState_.high);
    double low = semState_.low + f * high;
    double band = high - low;

    semState_.high = high;
    semState_.low = low;

    return getOutput(low, high, band, input);
}

float ZenithFilterEnhanced::processTB303(float input) {
    double fs = sampleRate_ * oversamplingFactor_;
    double wc = 2.0 * juce::MathConstants<double>::pi * cutoff_ / fs;
    double g = std::tan(wc / (4.0 * fs));

    double feedback = resonance_ * 0.99 * tb303State_.s4;
    double y0 = input - feedback;

    double y1 = tb303State_.s1 + g * (y0 - tb303State_.s1);
    double y2 = tb303State_.s2 + g * (y1 - tb303State_.s2);
    double y3 = tb303State_.s3 + g * (y2 - tb303State_.s3);
    double y4 = tb303State_.s4 + g * (y3 - tb303State_.s4);

    tb303State_.s1 = y1;
    tb303State_.s2 = y2;
    tb303State_.s3 = y3;
    tb303State_.s4 = y4;

    return getOutput(y4, y4, y4, input);
}

float ZenithFilterEnhanced::getOutput(float low, float high, float band, float input) {
    switch (output_) {
        case FilterOutput::Low:
            return static_cast<float>(low);
        case FilterOutput::High:
            return static_cast<float>(high);
        case FilterOutput::Band:
            return static_cast<float>(band);
        case FilterOutput::Notch:
            return static_cast<float>(low + high);
        case FilterOutput::All:
            return static_cast<float>((low + high) * 0.5f);
        default:
            return static_cast<float>(low);
    }
}

} // namespace zenith
