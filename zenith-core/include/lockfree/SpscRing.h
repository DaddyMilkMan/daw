/**
 * @file SpscRing.h
 * @brief Lock-free single-producer single-consumer ring buffer
 *
 * Real-time safe SPSC queue using atomic operations.
 * Supports peek-then-pop pattern for non-destructive reads.
 *
 * RT Safety:
 * - No locks, no allocations after construction
 * - Wait-free for single producer/consumer
 * - Cache-line aligned indices to prevent false sharing
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

namespace zenith {

/**
 * @brief Lock-free SPSC ring buffer
 * @tparam T Element type (must be trivially copyable for RT safety)
 *
 * Usage:
 *   SpscRing<TransportEvent> ring(4096);
 *
 *   // Producer thread:
 *   ring.tryPush(event);
 *
 *   // Consumer thread (RT-safe):
 *   TransportEvent ev;
 *   while (ring.tryPeek(ev)) {
 *       if (shouldDefer(ev)) break;  // Leave in queue
 *       ring.tryPop(ev);             // Consume now
 *       process(ev);
 *   }
 */
template <typename T>
class SpscRing
{
public:
    static_assert(std::is_trivially_copyable_v<T>,
                  "SPSC ring requires trivially copyable type for RT safety");

    /**
     * @brief Construct ring with power-of-2 capacity
     * @param capacity Number of slots (will be rounded up to next power of 2)
     */
    explicit SpscRing(size_t capacity)
        : capacity_(roundUpPow2(capacity))
        , buffer_(new T[capacity_])
        , readIndex_(0)
        , writeIndex_(0)
    {
    }

    ~SpscRing() = default;

    // Non-copyable, non-movable (indices are atomic)
    SpscRing(const SpscRing&) = delete;
    SpscRing& operator=(const SpscRing&) = delete;
    SpscRing(SpscRing&&) = delete;
    SpscRing& operator=(SpscRing&&) = delete;

    /**
     * @brief Peek at front element without consuming
     * @param out Output parameter for peeked element
     * @return true if element was peeked, false if queue is empty
     *
     * RT-safe: No locks, no allocations, wait-free
     */
    bool tryPeek(T& out) const noexcept
    {
        const auto r = readIndex_.load(std::memory_order_acquire);
        const auto w = writeIndex_.load(std::memory_order_acquire);

        if (r == w)
            return false;  // Queue is empty

        out = buffer_[r & (capacity_ - 1)];
        return true;
    }

    /**
     * @brief Pop front element from queue
     * @param out Output parameter for popped element
     * @return true if element was popped, false if queue is empty
     *
     * RT-safe: No locks, no allocations, wait-free
     *
     * Note: Use tryPeek() first if you need non-destructive read
     */
    bool tryPop(T& out) noexcept
    {
        const auto r = readIndex_.load(std::memory_order_acquire);
        const auto w = writeIndex_.load(std::memory_order_acquire);

        if (r == w)
            return false;  // Queue is empty

        out = buffer_[r & (capacity_ - 1)];
        readIndex_.store(r + 1, std::memory_order_release);
        return true;
    }

    /**
     * @brief Push element to back of queue
     * @param item Element to push
     * @return true if pushed, false if queue is full
     *
     * RT-safe: No locks, no allocations, wait-free
     */
    bool tryPush(const T& item) noexcept
    {
        const auto w = writeIndex_.load(std::memory_order_relaxed);
        const auto r = readIndex_.load(std::memory_order_acquire);

        if (w - r >= capacity_)
            return false;  // Queue is full

        buffer_[w & (capacity_ - 1)] = item;
        writeIndex_.store(w + 1, std::memory_order_release);
        return true;
    }

    /**
     * @brief Get number of elements currently in queue (approximate)
     * @return Approximate size (may be stale in concurrent context)
     */
    size_t size() const noexcept
    {
        const auto r = readIndex_.load(std::memory_order_acquire);
        const auto w = writeIndex_.load(std::memory_order_acquire);
        return static_cast<size_t>(w - r);
    }

    /**
     * @brief Check if queue is empty (approximate)
     */
    bool empty() const noexcept
    {
        return size() == 0;
    }

    /**
     * @brief Get capacity of queue
     */
    size_t capacity() const noexcept
    {
        return capacity_;
    }

private:
    static constexpr size_t roundUpPow2(size_t n) noexcept
    {
        if (n <= 1) return 1;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        if constexpr (sizeof(size_t) > 4)
            n |= n >> 32;
        return n + 1;
    }

    const size_t capacity_;
    std::unique_ptr<T[]> buffer_;

    // Cache-line aligned to prevent false sharing
    alignas(64) std::atomic<uint64_t> readIndex_;
    alignas(64) std::atomic<uint64_t> writeIndex_;
};

} // namespace zenith
