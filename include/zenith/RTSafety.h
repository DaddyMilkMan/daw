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
 * @file RTSafety.h
 * @brief Real-time (RT) safety macro system for Zenith DAW
 *
 * This header provides macros and annotations to clearly mark real-time (RT)
 * vs. non-real-time (non-RT) function boundaries, along with runtime
 * assertions for debug builds.
 *
 * ## Usage
 *
 * ### Annotating functions
 * Place the annotation in the declaration comment and optionally as a
 * compile-time attribute (no-op on release builds):
 *
 * @code{.cpp}
 * // RT-SAFE: called from audio callback
 * ZENITH_RT_SAFE void processBlock(juce::AudioBuffer<float>& buf);
 *
 * // NON-RT: may allocate / block
 * ZENITH_NONRT_SAFE void loadPreset(const juce::File& file);
 * @endcode
 *
 * ### Asserting thread context at runtime (debug builds only)
 * @code{.cpp}
 * void processBlock(...) {
 *     ZENITH_ASSERT_RT_THREAD();    // asserts we ARE on the audio thread
 *     ...
 * }
 *
 * void createTrack(...) {
 *     ZENITH_ASSERT_NONRT_THREAD(); // asserts we are NOT on the audio thread
 *     ...
 * }
 * @endcode
 *
 * ### Checking at call sites
 * @code{.cpp}
 * void someHelper() {
 *     ZENITH_WARN_IF_RT_THREAD();  // emits DBG warning if called from RT
 *     ...
 * }
 * @endcode
 *
 * ## Rules enforced by the static analysis script
 *
 * The script `tools/lint/check_rt_forbidden.sh` scans any function annotated
 * with `ZENITH_RT_SAFE` (or inside a region marked with
 * `ZENITH_RT_REGION_BEGIN` / `ZENITH_RT_REGION_END`) for forbidden constructs:
 *   - Heap allocation: new / delete / malloc / free
 *   - Mutexes / locks: std::mutex, std::lock_guard, juce::CriticalSection
 *   - Condition variables / sleep
 *   - File / network I/O
 *   - Logging: DBG(), std::cout, printf, juce::Logger
 */

#include <atomic>
#include <thread>

// ---------------------------------------------------------------------------
// 1. ANNOTATION MACROS
//    These are documentation-style attributes: they express intent and are
//    used by the static-analysis script.  They compile to nothing at runtime.
// ---------------------------------------------------------------------------

/** Mark a function as real-time safe (may be called from the audio thread). */
#define ZENITH_RT_SAFE   /* RT-SAFE */

/** Mark a function as NOT real-time safe (must NOT be called from audio thread). */
#define ZENITH_NONRT_SAFE   /* NON-RT-SAFE */

/**
 * Mark a function as a real-time thread entry point (top-level audio callback).
 * This is the strongest form of ZENITH_RT_SAFE.
 */
#define ZENITH_RT_THREAD   /* RT-THREAD */

/**
 * Mark a function as message-thread only (UI / event processing).
 * Equivalent to ZENITH_NONRT_SAFE but semantically stronger.
 */
#define ZENITH_NONRT_THREAD   /* NONRT-THREAD */

// ---------------------------------------------------------------------------
// 2. REGION DELIMITERS
//    Use these to mark a block of code as RT-safe or non-RT-safe without
//    wrapping it in a helper function.  The static analysis script
//    uses these tokens to scope forbidden-construct searches.
//
//    Example:
//        ZENITH_RT_REGION_BEGIN
//        // only RT-safe code here
//        ZENITH_RT_REGION_END
// ---------------------------------------------------------------------------

#define ZENITH_RT_REGION_BEGIN     /* RT-REGION-BEGIN */
#define ZENITH_RT_REGION_END       /* RT-REGION-END */

// ---------------------------------------------------------------------------
// 3. RUNTIME ASSERTION MACROS
//    Active in debug builds; optimized away in release builds.
// ---------------------------------------------------------------------------

namespace zenith {
namespace rt {

#if JUCE_DEBUG || defined(ZENITH_ENABLE_RT_CHECKS)

    /**
     * @brief Thread ID of the active audio callback thread.
     *
     * Set by calling zenith::rt::markAsAudioThread() inside
     * audioDeviceAboutToStart() or the first audioDeviceIOCallback().
     */
    inline std::atomic<std::thread::id> gAudioThreadId{};

    /** Register the calling thread as the real-time audio thread. */
    inline void markAsAudioThread() noexcept {
        gAudioThreadId.store(std::this_thread::get_id(),
                             std::memory_order_relaxed);
    }

    /** Unregister the audio thread (call from audioDeviceStopped()). */
    inline void unmarkAudioThread() noexcept {
        gAudioThreadId.store(std::thread::id{}, std::memory_order_relaxed);
    }

    /** @returns true if the calling thread is the registered audio thread. */
    inline bool isAudioThread() noexcept {
        return std::this_thread::get_id() ==
               gAudioThreadId.load(std::memory_order_relaxed);
    }

    /**
     * @brief Assert that the current thread IS the audio (RT) thread.
     *
     * Use inside functions annotated ZENITH_RT_SAFE / ZENITH_RT_THREAD to
     * catch accidental calls from non-RT threads during development.
     */
    #define ZENITH_ASSERT_RT_THREAD() \
        jassert(zenith::rt::isAudioThread() && \
                "ZENITH_ASSERT_RT_THREAD: Expected audio thread!")

    /**
     * @brief Assert that the current thread is NOT the audio (RT) thread.
     *
     * Use inside functions annotated ZENITH_NONRT_SAFE / ZENITH_NONRT_THREAD
     * to catch accidental calls from the audio thread.
     */
    #define ZENITH_ASSERT_NONRT_THREAD() \
        jassert(!zenith::rt::isAudioThread() && \
                "ZENITH_ASSERT_NONRT_THREAD: Must NOT be called from audio thread!")

    // Internal helper: stringify a macro argument (two-level expansion for __LINE__)
    #define ZENITH_DETAIL_STR(x)  #x
    #define ZENITH_DETAIL_TOSTR(x) ZENITH_DETAIL_STR(x)

    /**
     * @brief Emit a debug warning (non-fatal) if called from the audio thread.
     *
     * Useful for functions that are "best-effort" RT-safe but have known
     * edge cases.
     */
    #define ZENITH_WARN_IF_RT_THREAD() \
        do { \
            if (zenith::rt::isAudioThread()) { \
                DBG("ZENITH_WARN_IF_RT_THREAD: Potentially RT-unsafe call from " \
                    "audio thread at " __FILE__ ":" ZENITH_DETAIL_TOSTR(__LINE__)); \
            } \
        } while (false)

    /**
     * @brief Assert / check condition that must hold on the RT thread.
     *
     * Analogous to jassert but self-documents that the assertion is inside
     * real-time code.
     *
     * @param condition Expression that must evaluate to true.
     */
    #define ZENITH_RT_ASSERT(condition) \
        jassert((condition) && "ZENITH_RT_ASSERT failed in RT code!")

#else // Release build — all macros are no-ops

    inline void markAsAudioThread() noexcept {}
    inline void unmarkAudioThread() noexcept {}
    inline bool isAudioThread() noexcept { return false; }

    #define ZENITH_ASSERT_RT_THREAD()     ((void)0)
    #define ZENITH_ASSERT_NONRT_THREAD()  ((void)0)
    #define ZENITH_WARN_IF_RT_THREAD()    ((void)0)
    #define ZENITH_RT_ASSERT(condition)   ((void)0)

#endif // JUCE_DEBUG || ZENITH_ENABLE_RT_CHECKS

} // namespace rt
} // namespace zenith
