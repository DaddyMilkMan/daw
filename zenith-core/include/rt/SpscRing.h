// SPDX-License-Identifier: MIT
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace rt {
// Lock-free single-producer/single-consumer ring (power-of-two capacity).
template <typename T, std::size_t CapacityPow2>
class SpscRing {
    static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0, "Capacity must be power of two");
public:
    SpscRing() : head_(0), tail_(0) {}
    bool push(const T& v) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = (head + 1) & mask();
        if (next == tail_.load(std::memory_order_acquire)) return false; // full
        buf_[head] = v;
        head_.store(next, std::memory_order_release);
        return true;
    }
    bool pop(T& out) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) return false; // empty
        out = buf_[tail];
        tail_.store((tail + 1) & mask(), std::memory_order_release);
        return true;
    }
    std::size_t size() const noexcept {
        const auto h = head_.load(std::memory_order_acquire);
        const auto t = tail_.load(std::memory_order_acquire);
        return (h + CapacityPow2 - t) & mask();
    }
    static constexpr std::size_t capacity() noexcept { return CapacityPow2 - 1; }
private:
    static constexpr std::size_t mask() noexcept { return CapacityPow2 - 1; }
    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
    alignas(64) T buf_[CapacityPow2]{};
};
} // namespace rt
