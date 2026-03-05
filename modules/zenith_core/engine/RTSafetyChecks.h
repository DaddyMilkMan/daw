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

// RTSafetyChecks.h
//
// This header re-exports the canonical RT-safety macro system defined in
// include/zenith/RTSafety.h and adds engine-internal utilities (spinlock,
// CPU pause).  New code should #include <zenith/RTSafety.h> directly; this
// file exists for backward compatibility with existing engine includes.

#include <atomic>
#include <thread>
#include <juce_core/juce_core.h>

// Pull in the canonical macro/annotation definitions.
// The path is relative to the project root; ensure the project's include
// directories contain the repo root (see CMakeLists.txt).
#if __has_include(<zenith/RTSafety.h>)
    #include <zenith/RTSafety.h>
#else
    // Fallback: resolve relative to this file's own directory tree
    #include "../../../../include/zenith/RTSafety.h"
#endif

namespace zenith {
namespace rt {

// ---------------------------------------------------------------------------
// Backward-compatibility aliases
// ---------------------------------------------------------------------------
// The canonical names in RTSafety.h are ZENITH_ASSERT_NONRT_THREAD() and
// ZENITH_WARN_IF_RT_THREAD().  The old name ZENITH_ASSERT_NOT_RT_THREAD()
// is preserved here so existing call sites continue to compile.
#ifndef ZENITH_ASSERT_NOT_RT_THREAD
    #define ZENITH_ASSERT_NOT_RT_THREAD()  ZENITH_ASSERT_NONRT_THREAD()
#endif

//==============================================================================
// RT-Safe Utility Functions
//==============================================================================

/**
 // Brief: CPU pause instruction for spinlocks
 * 
 * This is more efficient than a busy loop and doesn't cause
 * power/thermal issues on modern CPUs. Also better for hyperthreading.
 */
inline void cpuPause() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #ifdef _MSC_VER
        _mm_pause();
    #else
        __builtin_ia32_pause();
    #endif
#elif defined(__arm__) || defined(__aarch64__)
    __asm__ __volatile__("yield" ::: "memory");
#endif
}

/**
 // Brief: RT-safe spinlock using atomic flag
 * 
 * Unlike std::mutex, this won't cause priority inversion.
 * Only use for very short critical sections.
 */
class RTSpinLock {
public:
    void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            cpuPause();
        }
    }
    
    bool try_lock() noexcept {
        return !flag_.test_and_set(std::memory_order_acquire);
    }
    
    void unlock() noexcept {
        flag_.clear(std::memory_order_release);
    }
    
private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

/**
 // Brief: RAII lock guard for RTSpinLock
 */
class ScopedRTLock {
public:
    explicit ScopedRTLock(RTSpinLock& lock) noexcept : lock_(lock) {
        lock_.lock();
    }
    
    ~ScopedRTLock() noexcept {
        lock_.unlock();
    }
    
    ScopedRTLock(const ScopedRTLock&) = delete;
    ScopedRTLock& operator=(const ScopedRTLock&) = delete;
    
private:
    RTSpinLock& lock_;
};

/**
 // Brief: Try-lock guard that doesn't block
 * 
 * If the lock is not immediately available, acquired() returns false.
 * This is RT-safe because it never blocks.
 */
class TryRTLock {
public:
    explicit TryRTLock(RTSpinLock& lock) noexcept 
        : lock_(lock), acquired_(lock.try_lock()) {}
    
    ~TryRTLock() noexcept {
        if (acquired_) lock_.unlock();
    }
    
    bool acquired() const noexcept { return acquired_; }
    
    TryRTLock(const TryRTLock&) = delete;
    TryRTLock& operator=(const TryRTLock&) = delete;
    
private:
    RTSpinLock& lock_;
    bool acquired_;
};

} // namespace rt
} // namespace zenith
