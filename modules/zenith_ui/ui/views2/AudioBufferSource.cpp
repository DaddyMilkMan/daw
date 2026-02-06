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

#include "AudioBufferSource.h"
#include <algorithm>
#include <cstring>

namespace zenith::ui {

//==============================================================================
// AudioBufferSource Implementation
//==============================================================================

AudioBufferSource::AudioBufferSource() {
    buffer_.setSize(2, 4096);
    buffer_.clear();
}

AudioBufferSource::AudioBufferSource(int numChannels)
    : numChannels_(numChannels) {
    buffer_.setSize(numChannels, 4096);
    buffer_.clear();
}

AudioBufferSource::~AudioBufferSource() {
    // JUCE AudioBuffer handles its own cleanup
}

void AudioBufferSource::prepare(int samplesPerBlock, double sampleRate) {
    bufferSize_ = juce::nextPowerOfTwo(samplesPerBlock * 4); // 4 blocks of storage
    sampleRate_ = sampleRate;

    // Resize buffer while preserving content if possible
    if (buffer_.getNumSamples() < bufferSize_) {
        buffer_.setSize(numChannels_, bufferSize_, true, true, true);
    }

    buffer_.clear();
    writePosition_.store(0, std::memory_order_relaxed);
    prepared_ = true;
}

void AudioBufferSource::reset() {
    buffer_.clear();
    writePosition_.store(0, std::memory_order_relaxed);
}

void AudioBufferSource::resize(int numChannels, int numSamples) {
    numChannels_ = numChannels;
    bufferSize_ = juce::nextPowerOfTwo(numSamples);

    buffer_.setSize(numChannels, bufferSize_, true, true, true);
    buffer_.clear();
    writePosition_.store(0, std::memory_order_relaxed);
}

void AudioBufferSource::writeAudio(const float* const* audio, int numChannels, int numSamples) {
    if (!prepared_ || numChannels > buffer_.getNumChannels()) {
        return;
    }

    int writePos = writePosition_.load(std::memory_order_relaxed);

    for (int ch = 0; ch < numChannels && ch < numChannels_; ++ch) {
        float* dest = buffer_.getWritePointer(ch);

        // Handle wrap-around case
        int samplesUntilWrap = bufferSize_ - writePos;
        int samplesToWrite = std::min(numSamples, samplesUntilWrap);

        // Write first chunk (until wrap or end)
        std::copy_n(audio[ch], samplesToWrite, dest + writePos);

        // Write remaining samples if wrapped
        if (numSamples > samplesUntilWrap) {
            int remainingSamples = numSamples - samplesUntilWrap;
            std::copy_n(audio[ch] + samplesUntilWrap, remainingSamples, dest);
        }
    }

    incrementWritePosition(numSamples);
}

void AudioBufferSource::writeInterleavedAudio(const float* interleavedData, int numChannels, int numSamples) {
    if (!prepared_ || numChannels > buffer_.getNumChannels()) {
        return;
    }

    int writePos = writePosition_.load(std::memory_order_relaxed);

    for (int ch = 0; ch < numChannels && ch < numChannels_; ++ch) {
        float* dest = buffer_.getWritePointer(ch);
        int samplesUntilWrap = bufferSize_ - writePos;
        int samplesToWrite = std::min(numSamples, samplesUntilWrap);

        // Deinterleave and write first chunk
        for (int i = 0; i < samplesToWrite; ++i) {
            dest[writePos + i] = interleavedData[i * numChannels + ch];
        }

        // Handle wrap-around
        if (numSamples > samplesUntilWrap) {
            int remainingSamples = numSamples - samplesUntilWrap;
            for (int i = 0; i < remainingSamples; ++i) {
                dest[i] = interleavedData[(samplesUntilWrap + i) * numChannels + ch];
            }
        }
    }

    incrementWritePosition(numSamples);
}

void AudioBufferSource::readAudio(float* destBuffer, int numSamples, int channel) const {
    if (!prepared_ || channel >= buffer_.getNumChannels()) {
        std::fill_n(destBuffer, numSamples, 0.0f);
        return;
    }

    const float* src = buffer_.getReadPointer(channel);
    int writePos = writePosition_.load(std::memory_order_acquire);

    // Calculate read position (look back from write position)
    int readPos = wrapPosition(writePos - numSamples);

    int samplesUntilWrap = bufferSize_ - readPos;
    int samplesToRead = std::min(numSamples, samplesUntilWrap);

    // Read first chunk
    std::copy_n(src + readPos, samplesToRead, destBuffer);

    // Handle wrap-around
    if (numSamples > samplesUntilWrap) {
        int remainingSamples = numSamples - samplesUntilWrap;
        std::copy_n(src, remainingSamples, destBuffer + samplesToRead);
    }
}

float AudioBufferSource::getLatestSample(int channel) const {
    if (!prepared_ || channel >= buffer_.getNumChannels()) {
        return 0.0f;
    }

    int writePos = writePosition_.load(std::memory_order_acquire);
    int readPos = wrapPosition(writePos - 1);

    return buffer_.getSample(channel, readPos);
}

void AudioBufferSource::incrementWritePosition(int numSamples) {
    int currentPos = writePosition_.load(std::memory_order_relaxed);
    int newPos = wrapPosition(currentPos + numSamples);
    writePosition_.store(newPos, std::memory_order_release);
}

int AudioBufferSource::wrapPosition(int position) const {
    return (position + bufferSize_) % bufferSize_;
}

//==============================================================================
// AudioBufferQueue Implementation
//==============================================================================

AudioBufferQueue::AudioBufferQueue() {
    // Pre-allocate buffers
    for (int i = 0; i < kQueueSize; ++i) {
        buffers_.push_back(std::make_unique<juce::AudioBuffer<float>>(2, 512));
    }
}

AudioBufferQueue::AudioBufferQueue(int numChannels, int numSamples) {
    for (int i = 0; i < kQueueSize; ++i) {
        buffers_.push_back(std::make_unique<juce::AudioBuffer<float>>(numChannels, numSamples));
    }
}

AudioBufferQueue::~AudioBufferQueue() = default;

bool AudioBufferQueue::push(const float* const* audio, int numChannels, int numSamples) {
    int currentAvailable = available_.load(std::memory_order_acquire);

    if (currentAvailable >= kQueueSize) {
        return false; // Queue full
    }

    int writeIdx = writeIndex_.load(std::memory_order_relaxed);
    auto& buffer = buffers_[writeIdx];

    // Ensure buffer is the right size
    if (buffer->getNumChannels() != numChannels || buffer->getNumSamples() != numSamples) {
        buffer->setSize(numChannels, numSamples, false, true, true);
    }

    // Copy audio data
    for (int ch = 0; ch < numChannels; ++ch) {
        buffer->copyFrom(ch, 0, audio[ch], numSamples);
    }

    // Advance write pointer
    writeIndex_.store((writeIdx + 1) % kQueueSize, std::memory_order_release);
    available_.fetch_add(1, std::memory_order_acq_rel);

    return true;
}

bool AudioBufferQueue::pop(float* const* dest, int numChannels, int numSamples) {
    int currentAvailable = available_.load(std::memory_order_acquire);

    if (currentAvailable == 0) {
        return false; // Queue empty
    }

    int readIdx = readIndex_.load(std::memory_order_relaxed);
    auto& buffer = buffers_[readIdx];

    // Check buffer size compatibility
    if (buffer->getNumChannels() != numChannels || buffer->getNumSamples() != numSamples) {
        // Skip this buffer if size doesn't match
        readIndex_.store((readIdx + 1) % kQueueSize, std::memory_order_release);
        available_.fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }

    // Copy audio data
    for (int ch = 0; ch < numChannels; ++ch) {
        std::copy_n(buffer->getReadPointer(ch), numSamples, dest[ch]);
    }

    // Advance read pointer
    readIndex_.store((readIdx + 1) % kQueueSize, std::memory_order_release);
    available_.fetch_sub(1, std::memory_order_acq_rel);

    return true;
}

} // namespace zenith::ui
