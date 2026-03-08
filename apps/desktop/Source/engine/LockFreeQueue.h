/*
  ==============================================================================

    LockFreeQueue.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #8)

    Lock-free single-producer/single-consumer queue for real-time safety.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief Lock-free SPSC (Single Producer Single Consumer) queue
 *
 * This is a lock-free queue suitable for real-time audio applications.
 * It's thread-safe for one thread pushing and another thread popping.
 *
 * Features:
 * - Wait-free operations (no mutexes, no condition variables)
 * - Cache-friendly for performance
 * - Fixed size (no allocations during push/pop)
 * - Memory ordering guarantees
 *
 * Template Parameters:
 * @param T Type of elements to store
 * @param Capacity Maximum number of elements (must be power of 2)
 */
template <typename T, size_t Capacity>
class LockFreeQueue {
public:
    //==========================================================================
    static_assert((Capacity & (Capacity - 1)) == 0,
                 "Capacity must be a power of 2");

    //==========================================================================
    LockFreeQueue()
        : writeIndex_(0)
        , readIndex_(0)
    {
        buffer_.resize(Capacity);
    }

    //==========================================================================
    /**
     * @brief Push an element (producer side)
     * @param item Item to push
     * @return true if successful, false if queue is full
     */
    bool push(const T& item) {
        const size_t writePos = writeIndex_.load(std::memory_order_relaxed);
        const size_t nextWrite = (writePos + 1) & (Capacity - 1);

        // Check if full
        if (nextWrite == readIndex_.load(std::memory_order_acquire)) {
            return false;  // Queue is full
        }

        // Write item
        buffer_[writePos] = item;

        // Update write index (release semantics ensures item is written first)
        writeIndex_.store(nextWrite, std::memory_order_release);

        return true;
    }

    //==========================================================================
    /**
     * @brief Push an element (move semantics)
     * @param item Item to move
     * @return true if successful, false if queue is full
     */
    bool push(T&& item) {
        const size_t writePos = writeIndex_.load(std::memory_order_relaxed);
        const size_t nextWrite = (writePos + 1) & (Capacity - 1);

        // Check if full
        if (nextWrite == readIndex_.load(std::memory_order_acquire)) {
            return false;  // Queue is full
        }

        // Move item
        buffer_[writePos] = std::move(item);

        // Update write index
        writeIndex_.store(nextWrite, std::memory_order_release);

        return true;
    }

    //==========================================================================
    /**
     * @brief Pop an element (consumer side)
     * @param item [out] Popped item
     * @return true if successful, false if queue is empty
     */
    bool pop(T& item) {
        const size_t readPos = readIndex_.load(std::memory_order_relaxed);

        // Check if empty
        if (readPos == writeIndex_.load(std::memory_order_acquire)) {
            return false;  // Queue is empty
        }

        // Read item
        item = buffer_[readPos];

        // Update read index (release semantics ensures item is read first)
        const size_t nextRead = (readPos + 1) & (Capacity - 1);
        readIndex_.store(nextRead, std::memory_order_release);

        return true;
    }

    //==========================================================================
    /**
     * @brief Check if queue is empty
     * May return false positive if producer is pushing
     * @return true if appears empty
     */
    bool isEmpty() const {
        return readIndex_.load(std::memory_order_acquire) ==
               writeIndex_.load(std::memory_order_acquire);
    }

    //==========================================================================
    /**
     * @brief Check if queue is full
     * May return false positive if consumer is popping
     * @return true if appears full
     */
    bool isFull() const {
        const size_t nextWrite = (writeIndex_.load(std::memory_order_acquire) + 1) & (Capacity - 1);
        return nextWrite == readIndex_.load(std::memory_order_acquire);
    }

    //==========================================================================
    /**
     * @brief Get approximate size
     * May be inaccurate if producer/consumer are active
     * @return Approximate number of elements
     */
    size_t size() const {
        const size_t write = writeIndex_.load(std::memory_order_acquire);
        const size_t read = readIndex_.load(std::memory_order_acquire);
        return (write - read) & (Capacity - 1);
    }

    //==========================================================================
    /**
     * @brief Get capacity
     * @return Maximum number of elements
     */
    size_t capacity() const {
        return Capacity;
    }

    //==========================================================================
    /**
     * @brief Clear the queue
     * Note: Not thread-safe! Only call when producer and consumer are stopped.
     */
    void clear() {
        writeIndex_.store(0, std::memory_order_relaxed);
        readIndex_.store(0, std::memory_order_relaxed);
    }

private:
    //==========================================================================
    std::vector<T> buffer_;
    std::atomic<size_t> writeIndex_;  // Where producer writes
    std::atomic<size_t> readIndex_;   // Where consumer reads

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LockFreeQueue)
};

//==============================================================================
/**
 * @brief Lock-free SPSC queue optimized for pointers
 * Slightly more efficient than generic version for pointer types
 */
template <typename T>
class LockFreePointerQueue {
public:
    //==========================================================================
    explicit LockFreePointerQueue(size_t capacity)
        : capacity_(capacity)
        , mask_(capacity - 1)
        , writeIndex_(0)
        , readIndex_(0)
    {
        jassert((capacity & (capacity - 1)) == 0);  // Must be power of 2
        buffer_.resize(capacity, nullptr);
    }

    //==========================================================================
    /**
     * @brief Push a pointer
     */
    bool push(T* item) {
        const size_t writePos = writeIndex_.load(std::memory_order_relaxed);
        const size_t nextWrite = (writePos + 1) & mask_;

        if (nextWrite == readIndex_.load(std::memory_order_acquire)) {
            return false;  // Full
        }

        buffer_[writePos] = item;
        writeIndex_.store(nextWrite, std::memory_order_release);

        return true;
    }

    //==========================================================================
    /**
     * @brief Pop a pointer
     */
    bool pop(T*& item) {
        const size_t readPos = readIndex_.load(std::memory_order_relaxed);

        if (readPos == writeIndex_.load(std::memory_order_acquire)) {
            return false;  // Empty
        }

        item = buffer_[readPos];
        const size_t nextRead = (readPos + 1) & mask_;
        readIndex_.store(nextRead, std::memory_order_release);

        return true;
    }

    //==========================================================================
    bool isEmpty() const {
        return readIndex_.load(std::memory_order_acquire) ==
               writeIndex_.load(std::memory_order_acquire);
    }

    //==========================================================================
    size_t size() const {
        const size_t write = writeIndex_.load(std::memory_order_acquire);
        const size_t read = readIndex_.load(std::memory_order_acquire);
        return (write - read) & mask_;
    }

private:
    //==========================================================================
    std::vector<T*> buffer_;
    const size_t capacity_;
    const size_t mask_;
    std::atomic<size_t> writeIndex_;
    std::atomic<size_t> readIndex_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LockFreePointerQueue)
};

//==============================================================================
/**
 * @ convenience typedef for common queue sizes
 */
template <typename T>
using LockFreeQueue256 = LockFreeQueue<T, 256>;

template <typename T>
using LockFreeQueue512 = LockFreeQueue<T, 512>;

template <typename T>
using LockFreeQueue1024 = LockFreeQueue<T, 1024>;

template <typename T>
using LockFreeQueue2048 = LockFreeQueue<T, 2048>;

} // namespace zenith
