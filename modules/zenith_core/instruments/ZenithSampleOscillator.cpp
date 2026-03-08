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

#include "ZenithSampleOscillator.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithSampleOscillator::ZenithSampleOscillator() {
    resetPosition();
}

//==============================================================================
// SAMPLE LOADING
//==============================================================================

juce::Result ZenithSampleOscillator::loadSample(const juce::File& file) {
    if (!file.existsAsFile()) {
        return juce::Result::fail("File not found");
    }

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        formatManager.createReaderFor(file)
    );

    if (!reader) {
        return juce::Result::fail("Could not read audio file");
    }

    numChannels_ = reader->numChannels;
    numSamples_ = static_cast<int>(reader->lengthInSamples);
    sampleRate_ = reader->sampleRate;

    if (numChannels_ > 2) {
        return juce::Result::fail("Only mono and stereo supported");
    }

    sampleData_.resize(static_cast<size_t>(numSamples_ * numChannels_));

    // Read all samples
    juce::AudioBuffer<float> buffer(numChannels_, numSamples_);
    if (!reader->read(&buffer, 0, numSamples_, 0)) {
        return juce::Result::fail("Failed to read samples");
    }

    // Copy interleaved
    for (int ch = 0; ch < numChannels_; ++ch) {
        const float* src = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples_; ++i) {
            sampleData_[i * numChannels_ + ch] = src[i];
        }
    }

    // Auto-detect loop points from silence
    // Find start: skip leading silence
    int start = 0;
    while (start < numSamples_ && std::abs(sampleData_[start * numChannels_]) < 0.001f) {
        start++;
    }

    // Find end: skip trailing silence
    int end = numSamples_ - 1;
    while (end > start && std::abs(sampleData_[end * numChannels_]) < 0.001f) {
        end--;
    }

    if (start < end) {
        startPoint_ = static_cast<float>(start) / numSamples_;
        endPoint_ = static_cast<float>(end) / numSamples_;
    }

    return juce::Result::success();
}

void ZenithSampleOscillator::loadSample(const float* data, int numChannels,
                                     int numSamples, double sampleRate) {
    numChannels_ = juce::jlimit(1, 2, numChannels);
    numSamples_ = numSamples;
    sampleRate_ = sampleRate;

    sampleData_.resize(static_cast<size_t>(numSamples_ * numChannels_));
    std::copy(data, data + numSamples * numChannels, sampleData_.begin());
}

void ZenithSampleOscillator::clearSample() {
    sampleData_.clear();
    numSamples_ = 0;
    numChannels_ = 0;
    resetPosition();
}

//==============================================================================
// PLAYBACK CONTROL
//==============================================================================

void ZenithSampleOscillator::resetPosition() {
    position_ = startPoint_;
    phase_ = 0.0;
    lastOut_[0] = lastOut_[1] = 0.0f;
}

float ZenithSampleOscillator::getPosition() const {
    return static_cast<float>(position_);
}

//==============================================================================
// AUDIO GENERATION
//==============================================================================

float ZenithSampleOscillator::getNextSample(float frequency, float loopStart) {
    if (!hasSample()) {
        return 0.0f;
    }

    // Calculate effective loop range
    float effectiveStart = juce::jmax(startPoint_, loopStart);
    float effectiveEnd = endPoint_;
    float loopLength = effectiveEnd - effectiveStart;

    if (loopLength <= 0.0f) {
        return 0.0f;
    }

    // Advance phase by frequency
    double speed = frequency;
    phase_ += speed / sampleRate_;

    // Calculate new position
    double newPos = effectiveStart + phase_;

    // Handle wrapping
    bool wrapped = false;
    while (newPos >= effectiveEnd) {
        newPos -= loopLength;
        wrapped = true;
    }
    while (newPos < effectiveStart) {
        newPos += loopLength;
        wrapped = true;
    }

    position_ = static_cast<float>(newPos);

    // Get sample for each channel
    float out[2];
    for (int ch = 0; ch < numChannels_; ++ch) {
        out[ch] = readInterpolated(newPos, ch);
    }

    // Apply crossfade at loop points
    if (wrapped && xfadeLength_ > 0.0f) {
        // Crossfade between end and start
        float xfadePos = (newPos - effectiveStart) / loopLength;
        if (xfadePos < xfadeLength_) {
            float mix = xfadePos / xfadeLength_;
            float invMix = 1.0f - mix;

            for (int ch = 0; ch < numChannels_; ++ch) {
                // Read from start
                float startSample = readInterpolated(effectiveStart, ch);
                out[ch] = out[ch] * invMix + startSample * mix;
            }
        }
    }

    // Update previous outputs
    for (int ch = 0; ch < numChannels_; ++ch) {
        lastOut_[ch] = out[ch];
    }

    // Return mono sum (stereo handled externally)
    if (numChannels_ == 2) {
        return (out[0] + out[1]) * 0.5f;
    }
    return out[0];
}

void ZenithSampleOscillator::process(float* output, int numSamples, float frequency) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = getNextSample(frequency, 0.0f);
    }
}

//==============================================================================
// INTERNAL HELPERS
//==============================================================================

float ZenithSampleOscillator::readSample(double pos, int channel) const {
    if (!hasSample() || numChannels_ == 0) {
        return 0.0f;
    }

    int index = static_cast<int>(pos) % numSamples_;
    if (index < 0) {
        index += numSamples_;
    }

    int ch = juce::jlimit(0, numChannels_ - 1, channel);
    return sampleData_[index * numChannels_ + ch];
}

float ZenithSampleOscillator::readInterpolated(double pos, int channel) const {
    if (!hasSample() || numChannels_ == 0) {
        return 0.0f;
    }

    int index1 = static_cast<int>(pos) % numSamples_;
    if (index1 < 0) index1 += numSamples_;
    int index2 = (index1 + 1) % numSamples_;

    int ch = juce::jlimit(0, numChannels_ - 1, channel);

    float s1 = sampleData_[index1 * numChannels_ + ch];
    float s2 = sampleData_[index2 * numChannels_ + ch];

    // Linear interpolation fraction
    float frac = static_cast<float>(pos) - index1;

    return s1 + frac * (s2 - s1);
}

} // namespace zenith
