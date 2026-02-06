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

// MemoryManager.h



#include <mutex>
#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @class ObjectPool
 // Brief: Pre-allocated object pool to avoid real-time allocations
 * 
 * Note: The standard acquire()/release() methods use std::mutex and are NOT RT-safe.
 * For audio thread use, use tryAcquireRT()/releaseRT() which use try_lock.
 */
template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t initialSize = 16) {
        for (size_t i = 0; i < initialSize; ++i) {
            available_.push(std::make_unique<T>());
        }
    }
    
    // =========================================================================
    // NON-RT-SAFE methods (use from message thread only)
    // =========================================================================
    
    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (available_.empty()) {
            return std::make_unique<T>();
        }
        
        auto obj = std::move(available_.top());
        available_.pop();
        return obj;
    }
    
    void release(std::unique_ptr<T> obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        available_.push(std::move(obj));
    }
    
    size_t availableCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return available_.size();
    }
    
    // =========================================================================
    // RT-SAFE methods (safe for audio thread)
    // =========================================================================
    
    /**
     // Brief: RT-SAFE: Try to acquire an object without blocking
     * @return Raw pointer to object, or nullptr if pool is empty or lock unavailable
     // Note: Caller takes ownership and must call releaseRT() when done
     */
    T* tryAcquireRT() noexcept {
        if (!mutex_.try_lock()) {
            return nullptr;  // Lock not available - don't block
        }
        
        T* result = nullptr;
        if (!available_.empty()) {
            result = available_.top().release();  // Transfer ownership
            available_.pop();
        }
        
        mutex_.unlock();
        return result;
    }
    
    /**
     // Brief: RT-SAFE: Try to release an object without blocking
     * @param obj Raw pointer to release (pool takes ownership)
     * @return true if released to pool, false if lock unavailable (caller keeps ownership)
     // Note: If this returns false, the object may leak - acceptable for RT safety
     */
    bool releaseRT(T* obj) noexcept {
        if (!obj) return true;
        
        if (!mutex_.try_lock()) {
            // Can't get lock - caller must handle this
            // In RT context, leaking is preferable to blocking
            return false;
        }
        
        available_.push(std::unique_ptr<T>(obj));
        mutex_.unlock();
        return true;
    }
    
private:
    mutable std::mutex mutex_;
    std::stack<std::unique_ptr<T>> available_;
};

/**
 * @class AudioMemoryManager
 // Brief: Centralized memory management for audio processing
 */
class AudioMemoryManager {
public:
    static AudioMemoryManager& getInstance() {
        static AudioMemoryManager instance;
        return instance;
    }
    
    // Pre-allocated buffers for real-time use
    struct RTBuffers {
        std::vector<float> channelBuffer;
        std::vector<float*> channelPointers;
        std::vector<juce::AudioBuffer<float>> tempBuffers;
    };
    
    RTBuffers& getRTBuffers() {
        thread_local RTBuffers buffers;
        return buffers;
    }
    
    // Memory pool for audio objects
    template<typename T>
    ObjectPool<T>& getPool() {
        static ObjectPool<T> pool;
        return pool;
    }
    
    // Statistics
    struct MemoryStats {
        size_t totalAllocations = 0;
        size_t totalDeallocations = 0;
        size_t currentUsage = 0;
        size_t peakUsage = 0;
    };
    
    const MemoryStats& getStats() const {
        return stats_;
    }
    
    void recordAllocation(size_t size) {
        stats_.totalAllocations++;
        stats_.currentUsage += size;
        stats_.peakUsage = std::max(stats_.peakUsage, stats_.currentUsage);
    }
    
    void recordDeallocation(size_t size) {
        stats_.totalDeallocations++;
        stats_.currentUsage -= size;
    }
    
private:
    AudioMemoryManager() = default;
    ~AudioMemoryManager() = default;
    
    mutable MemoryStats stats_;
};

/**
 * @class ScopedRTMemory
 // Brief: RAII wrapper for real-time memory allocation
 */
template<typename T>
class ScopedRTMemory {
public:
    ScopedRTMemory() : obj_(AudioMemoryManager::getInstance().getPool<T>().acquire()) {
        AudioMemoryManager::getInstance().recordAllocation(sizeof(T));
    }
    
    ~ScopedRTMemory() {
        AudioMemoryManager::getInstance().getPool<T>().release(std::move(obj_));
        AudioMemoryManager::getInstance().recordDeallocation(sizeof(T));
    }
    
    T* get() const { return obj_.get(); }
    T* operator->() const { return obj_.get(); }
    T& operator*() const { return *obj_; }
    
    // Non-copyable, movable
    ScopedRTMemory(const ScopedRTMemory&) = delete;
    ScopedRTMemory& operator=(const ScopedRTMemory&) = delete;
    ScopedRTMemory(ScopedRTMemory&&) = default;
    ScopedRTMemory& operator=(ScopedRTMemory&&) = default;
    
private:
    std::unique_ptr<T> obj_;
};

/**
 * @class TrackSnapshot
 // Brief: Lock-free track state snapshot for audio thread
 */
class TrackSnapshot {
public:
    struct TrackState {
        float volume = 1.0f;
        float pan = 0.0f;
        bool muted = false;
        bool soloed = false;
        void* processor = nullptr;  // Raw pointer for RT access
    };
    
    void updateTrack(const juce::String& trackId, const TrackState& state) {
        // Use atomic swap for thread safety
        auto newSnapshot = std::make_unique<std::unordered_map<juce::String, TrackState>>(*currentSnapshot_.load());
        (*newSnapshot)[trackId] = state;
        
        // BUG FIX #3: Properly save old pointer before replacing using exchange
        auto* oldPtr = currentSnapshot_.exchange(newSnapshot.release());
        
        // Clean up previous old snapshot
        if (oldSnapshot_) {
            delete oldSnapshot_;
        }
        // Store the just-replaced pointer for deferred deletion on next call
        oldSnapshot_ = oldPtr;
    }
    
    const TrackState* getTrackState(const juce::String& trackId) const {
        auto snapshot = currentSnapshot_.load();
        auto it = snapshot->find(trackId);
        return (it != snapshot->end()) ? &it->second : nullptr;
    }
    
    ~TrackSnapshot() {
        delete currentSnapshot_.load();
        delete oldSnapshot_;
    }
    
private:
    std::atomic<std::unordered_map<juce::String, TrackState>*> currentSnapshot_{new std::unordered_map<juce::String, TrackState>()};
    std::unordered_map<juce::String, TrackState>* oldSnapshot_ = nullptr;
};

/**
 * @class SafeTrackHandle
 // Brief: Safe handle to track data without ownership
 */
class SafeTrackHandle {
public:
    SafeTrackHandle(const juce::String& trackId, TrackSnapshot& snapshot)
        : trackId_(trackId), snapshot_(snapshot) {}
    
    const TrackSnapshot::TrackState* getState() const {
        return snapshot_.getTrackState(trackId_);
    }
    
    bool isValid() const {
        return getState() != nullptr;
    }
    
private:
    juce::String trackId_;
    TrackSnapshot& snapshot_;
};

} // namespace zenith
