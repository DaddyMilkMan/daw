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

/**
 * @file LockFreeCommandQueue.h
 * @brief Lock-free command/message queues for UI/AI/service → RT domain
 *        communication in Zenith DAW.
 *
 * ## Overview
 *
 * These queues are the **only sanctioned mechanism** for passing data across
 * the RT boundary.  All other inter-thread communication on the audio thread
 * must go through `std::atomic` for scalar values or one of the queues below
 * for structured commands.
 *
 * ## API boundary contract
 *
 * | Producer side (non-RT)     | Consumer side (RT)          |
 * |----------------------------|-----------------------------|
 * | `push()` — never blocks    | `pop()` — never blocks      |
 * | May be called from any     | Must only be called from    |
 * | non-RT thread (UI, AI,     | the audio callback thread.  |
 * | background services).      |                             |
 *
 * ## Provided types
 *
 * - **SPSCCommandQueue<T, Capacity>**
 *   Single-producer / single-consumer.  The fastest choice for the common
 *   case where there is exactly one writer (e.g. the UI parameter thread)
 *   and one reader (the audio thread).
 *
 * - **MPSCCommandQueue<T, Capacity>**
 *   Multi-producer / single-consumer.  Use when multiple threads (e.g. UI
 *   thread *and* AI service thread) must post commands to the same audio
 *   thread queue.
 *
 * ## Example — UI → RT parameter change
 *
 * @code{.cpp}
 * struct ParamChange { int paramId; float value; };
 *
 * // Shared queue (created on message thread, lifetime >= audio device open)
 * zenith::SPSCCommandQueue<ParamChange, 512> paramQueue;
 *
 * // UI thread (non-RT producer)
 * void sliderMoved(int id, float v) {
 *     paramQueue.push({ id, v });   // lock-free, non-blocking
 * }
 *
 * // Audio callback (RT consumer)
 * void processBlock(...) {
 *     ParamChange cmd;
 *     while (paramQueue.pop(cmd)) {
 *         applyParam(cmd.paramId, cmd.value);
 *     }
 *     // ... render audio ...
 * }
 * @endcode
 *
 * ## Example — multiple producers (AI + UI) → RT
 *
 * @code{.cpp}
 * struct EngineCommand { ... };
 * zenith::MPSCCommandQueue<EngineCommand, 256> cmdQueue;
 *
 * // AI service thread
 * void aiAgent() { cmdQueue.push({ CMD_LOAD_PRESET, presetId }); }
 *
 * // UI thread
 * void onButtonClick() { cmdQueue.push({ CMD_PLAY }); }
 *
 * // Audio thread
 * void processBlock(...) {
 *     EngineCommand cmd;
 *     while (cmdQueue.pop(cmd)) { dispatch(cmd); }
 * }
 * @endcode
 */

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <type_traits>

#include "RTSafety.h"

namespace zenith {

// ---------------------------------------------------------------------------
// SPSCCommandQueue — Single-Producer / Single-Consumer
// ---------------------------------------------------------------------------

/**
 * @class SPSCCommandQueue
 * @brief Lock-free SPSC queue for crossing the RT boundary.
 *
 * Thread safety:
 * - Exactly **one** thread may call `push()` at a time.
 * - Exactly **one** thread may call `pop()` at a time.
 * - The producer and consumer may be different threads simultaneously.
 *
 * @tparam T        The command/message type.  Must be trivially copyable.
 * @tparam Capacity Maximum number of pending commands.  Should be a power
 *                  of two for best performance (but not required).
 */
template <typename T, std::size_t Capacity>
class SPSCCommandQueue {
    static_assert(std::is_trivially_copyable_v<T>,
                  "SPSCCommandQueue<T>: T must be trivially copyable "
                  "(no heap allocation on copy).");
    static_assert(Capacity >= 2,
                  "SPSCCommandQueue: Capacity must be at least 2.");

public:
    SPSCCommandQueue() noexcept = default;

    // Non-copyable, non-movable (atomics are not movable)
    SPSCCommandQueue(const SPSCCommandQueue&) = delete;
    SPSCCommandQueue& operator=(const SPSCCommandQueue&) = delete;

    // -----------------------------------------------------------------------
    // Producer API (NON-RT side)
    // -----------------------------------------------------------------------

    /**
     * @brief Push a command into the queue.
     *
     * Lock-free, wait-free.  Safe to call from any non-RT thread.
     * Must not be called from more than one thread concurrently (SPSC).
     *
     * @param  item  Command to enqueue (copied by value).
     * @returns true  if the command was enqueued successfully.
     * @returns false if the queue is full (command dropped).
     */
    ZENITH_NONRT_SAFE
    bool push(const T& item) noexcept {
        const std::size_t w = writePos_.load(std::memory_order_relaxed);
        const std::size_t nextW = advance(w);

        // Full: consumer hasn't caught up yet
        if (nextW == readPos_.load(std::memory_order_acquire))
            return false;

        buffer_[w] = item;
        writePos_.store(nextW, std::memory_order_release);
        return true;
    }

    // -----------------------------------------------------------------------
    // Consumer API (RT side)
    // -----------------------------------------------------------------------

    /**
     * @brief Pop the oldest command from the queue.
     *
     * Lock-free, wait-free.  RT-safe: no allocation, no blocking.
     * Must not be called from more than one thread concurrently (SPSC).
     *
     * @param[out] item  Receives the dequeued command.
     * @returns true  if a command was available and written to @p item.
     * @returns false if the queue was empty.
     */
    ZENITH_RT_SAFE
    bool pop(T& item) noexcept {
        const std::size_t r = readPos_.load(std::memory_order_relaxed);

        // Empty
        if (r == writePos_.load(std::memory_order_acquire))
            return false;

        item = buffer_[r];
        readPos_.store(advance(r), std::memory_order_release);
        return true;
    }

    // -----------------------------------------------------------------------
    // Query helpers
    // -----------------------------------------------------------------------

    /** @returns true if the queue contains no commands. */
    ZENITH_RT_SAFE
    bool empty() const noexcept {
        return readPos_.load(std::memory_order_acquire) ==
               writePos_.load(std::memory_order_acquire);
    }

    /** @returns approximate number of pending commands (may be stale). */
    std::size_t size_approx() const noexcept {
        const std::size_t w = writePos_.load(std::memory_order_relaxed);
        const std::size_t r = readPos_.load(std::memory_order_relaxed);
        return (w >= r) ? (w - r) : (Capacity + 1 - r + w);
    }

    /** @returns the maximum number of commands the queue can hold. */
    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    static constexpr std::size_t kBufSize = Capacity + 1; // one slot wasted as sentinel

    static constexpr std::size_t advance(std::size_t pos) noexcept {
        return (pos + 1) % kBufSize;
    }

    // Cache-line pad to avoid false sharing between producer and consumer
    static constexpr std::size_t kCacheLineSize = 64;

    alignas(kCacheLineSize) std::atomic<std::size_t> writePos_{0};
    alignas(kCacheLineSize) std::atomic<std::size_t> readPos_{0};

    std::array<T, kBufSize> buffer_{};
};

// ---------------------------------------------------------------------------
// MPSCCommandQueue — Multi-Producer / Single-Consumer
// ---------------------------------------------------------------------------

/**
 * @class MPSCCommandQueue
 * @brief Lock-free MPSC queue for crossing the RT boundary.
 *
 * Allows **multiple** non-RT threads (UI, AI agents, background services)
 * to post commands that are consumed by the single audio thread.
 *
 * Thread safety:
 * - Any number of threads may call `push()` concurrently (multi-producer).
 * - Exactly **one** thread may call `pop()` at a time (audio thread).
 *
 * Implementation uses a power-of-two ring with a CAS-based write reservation.
 * Each slot has a sequence counter so the consumer can detect when the
 * producer has finished writing.
 *
 * @tparam T        The command/message type.  Must be trivially copyable.
 * @tparam Capacity Maximum pending commands.  Must be a power of two.
 */
template <typename T, std::size_t Capacity>
class MPSCCommandQueue {
    static_assert(std::is_trivially_copyable_v<T>,
                  "MPSCCommandQueue<T>: T must be trivially copyable.");
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0,
                  "MPSCCommandQueue: Capacity must be a power of two >= 2.");

public:
    MPSCCommandQueue() noexcept {
        for (std::size_t i = 0; i < Capacity; ++i)
            slots_[i].sequence.store(i, std::memory_order_relaxed);
        enqueue_.store(0, std::memory_order_relaxed);
        dequeue_.store(0, std::memory_order_relaxed);
    }

    MPSCCommandQueue(const MPSCCommandQueue&) = delete;
    MPSCCommandQueue& operator=(const MPSCCommandQueue&) = delete;

    // -----------------------------------------------------------------------
    // Producer API (NON-RT side)
    // -----------------------------------------------------------------------

    /**
     * @brief Push a command (thread-safe, many producers allowed).
     *
     * Uses a CAS loop to reserve a slot, then stores the data and marks
     * the slot as ready for consumption.  Lock-free in practice (bounded
     * CAS retries under light contention; falls back to spinning under heavy
     * contention, which should not occur in normal DAW operation).
     *
     * @returns true  if enqueued, false if the queue is full.
     */
    ZENITH_NONRT_SAFE
    bool push(const T& item) noexcept {
        Slot* slot = nullptr;
        std::size_t pos = enqueue_.load(std::memory_order_relaxed);

        for (;;) {
            slot = &slots_[pos & kMask];
            std::size_t seq = slot->sequence.load(std::memory_order_acquire);
            std::intptr_t diff = static_cast<std::intptr_t>(seq) -
                                 static_cast<std::intptr_t>(pos);

            if (diff == 0) {
                // Slot is free; try to claim it
                if (enqueue_.compare_exchange_weak(pos, pos + 1,
                                                   std::memory_order_relaxed))
                    break;
            } else if (diff < 0) {
                // Queue is full
                return false;
            } else {
                // Another producer got there first; reload
                pos = enqueue_.load(std::memory_order_relaxed);
            }
        }

        slot->data = item;
        slot->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    // -----------------------------------------------------------------------
    // Consumer API (RT side)
    // -----------------------------------------------------------------------

    /**
     * @brief Pop the oldest command.  RT-safe.
     *
     * @param[out] item  Receives the dequeued command.
     * @returns true  if a command was available.
     * @returns false if the queue was empty.
     */
    ZENITH_RT_SAFE
    bool pop(T& item) noexcept {
        const std::size_t pos = dequeue_.load(std::memory_order_relaxed);
        Slot* slot = &slots_[pos & kMask];
        const std::size_t seq = slot->sequence.load(std::memory_order_acquire);
        const std::intptr_t diff = static_cast<std::intptr_t>(seq) -
                                   static_cast<std::intptr_t>(pos + 1);

        if (diff != 0)
            return false; // empty or slot not yet written

        item = slot->data;
        slot->sequence.store(pos + Capacity, std::memory_order_release);
        dequeue_.store(pos + 1, std::memory_order_relaxed);
        return true;
    }

    /** @returns true if the queue appears empty. */
    ZENITH_RT_SAFE
    bool empty() const noexcept {
        const std::size_t pos = dequeue_.load(std::memory_order_relaxed);
        const Slot& slot = slots_[pos & kMask];
        const std::size_t seq = slot.sequence.load(std::memory_order_acquire);
        return static_cast<std::intptr_t>(seq) -
               static_cast<std::intptr_t>(pos + 1) != 0;
    }

    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    static constexpr std::size_t kMask = Capacity - 1;
    static constexpr std::size_t kCacheLineSize = 64;

    struct alignas(kCacheLineSize) Slot {
        std::atomic<std::size_t> sequence;
        T data;
    };

    alignas(kCacheLineSize) std::atomic<std::size_t> enqueue_{0};
    alignas(kCacheLineSize) std::atomic<std::size_t> dequeue_{0};
    std::array<Slot, Capacity> slots_;
};

} // namespace zenith
