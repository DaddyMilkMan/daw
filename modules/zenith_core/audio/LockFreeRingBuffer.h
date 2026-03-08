/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <chrono>
#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace zenith {
namespace audio {

// Lock-free ring buffer for real-time audio
template<typename T, size_t Size>
class LockFreeRingBuffer {
public:
    LockFreeRingBuffer() : writePos(0), readPos(0) {}

    bool push(const T& item) {
        size_t nextWrite = (writePos.load() + 1) % Size;
        if (nextWrite == readPos.load()) {
            return false;  // Buffer full
        }

        buffer[writePos.load()] = item;
        writePos.store(nextWrite);
        return true;
    }

    bool pop(T& item) {
        if (readPos.load() == writePos.load()) {
            return false;  // Buffer empty
        }

        item = buffer[readPos.load()];
        readPos.store((readPos.load() + 1) % Size);
        return true;
    }

    size_t size() const {
        size_t w = writePos.load();
        size_t r = readPos.load();
        return (w >= r) ? (w - r) : (Size - r + w);
    }

    bool isEmpty() const {
        return readPos.load() == writePos.load();
    }

    bool isFull() const {
        size_t nextWrite = (writePos.load() + 1) % Size;
        return nextWrite == readPos.load();
    }

    void clear() {
        writePos.store(0);
        readPos.store(0);
    }

private:
    std::array<T, Size> buffer;
    std::atomic<size_t> writePos;
    std::atomic<size_t> readPos;
};

// Real-time audio buffer with channel management

} // namespace
