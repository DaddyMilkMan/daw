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

#pragma once

// ThreadSafeAudioProcessor.h



namespace zenith {

/**
 * @class LockFreeRingBuffer
 // Brief: RT-SAFE Lock-free ring buffer for audio data
 * Single-producer/single-consumer with proper memory ordering
 */
template<typename T, size_t Size>
class LockFreeRingBuffer {
public:
    bool push(const T& item) {
        size_t currentWrite = writePos_.load(std::memory_order_relaxed);
        size_t nextWrite = (currentWrite + 1) % Size;
        
        // Check if buffer is full (acquire to sync with consumer)
        if (nextWrite == readPos_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        
        // Write data
        buffer_[currentWrite] = item;
        
        // Publish write position (release to make data visible to consumer)
        writePos_.store(nextWrite, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        size_t currentRead = readPos_.load(std::memory_order_relaxed);
        
        // Check if buffer is empty (acquire to sync with producer)
        if (currentRead == writePos_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        
        // Read data
        item = buffer_[currentRead];
        
        // Publish read position (release to make space visible to producer)
        readPos_.store((currentRead + 1) % Size, std::memory_order_release);
        return true;
    }
    
private:
    std::array<T, Size> buffer_;
    std::atomic<size_t> writePos_{0};
    std::atomic<size_t> readPos_{0};
};

/**
 * @class AtomicPlayhead
 // Brief: Thread-safe playhead position tracking
 */
class AtomicPlayhead {
public:
    void advance(int numSamples, double sampleRate) {
        juce::int64 current = position_.load(std::memory_order_relaxed);
        position_.store(current + numSamples, std::memory_order_relaxed);
        
        // Update time in seconds (approximate, good enough for UI)
        timeInSeconds_.store(current / sampleRate, std::memory_order_relaxed);
    }
    
    void setPosition(juce::int64 pos, double sampleRate) {
        position_.store(pos, std::memory_order_relaxed);
        timeInSeconds_.store(pos / sampleRate, std::memory_order_relaxed);
    }
    
    juce::int64 getPositionSamples() const {
        return position_.load(std::memory_order_relaxed);
    }
    
    double getPositionSeconds() const {
        return timeInSeconds_.load(std::memory_order_relaxed);
    }
    
private:
    std::atomic<juce::int64> position_{0};
    std::atomic<double> timeInSeconds_{0.0};
};

/**
 * @class RTAudioBuffer
 // Brief: Real-time safe audio buffer with pre-allocation
 */
class RTAudioBuffer {
public:
    RTAudioBuffer(int channels, int samples) 
        : buffer_(channels, samples) {}
    
    void clear() {
        buffer_.clear();
    }
    
    void copyFrom(int destChannel, int destStartSample, 
                  const float* source, int numSamples) {
        buffer_.copyFrom(destChannel, destStartSample, source, numSamples);
    }
    
    void addFrom(int destChannel, int destStartSample,
                 const float* source, int numSamples, float gain = 1.0f) {
        buffer_.addFrom(destChannel, destStartSample, source, numSamples, gain);
    }
    
    float* getWritePointer(int channel, int sample = 0) {
        return buffer_.getWritePointer(channel, sample);
    }
    
    const float* getReadPointer(int channel, int sample = 0) const {
        return buffer_.getReadPointer(channel, sample);
    }
    
    int getNumChannels() const { return buffer_.getNumChannels(); }
    int getNumSamples() const { return buffer_.getNumSamples(); }
    
private:
    juce::AudioBuffer<float> buffer_;
};

} // namespace zenith
