/*
    AudioCallbackCache.h - Optimized cache for audio callback performance

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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>

namespace zenith {

/**
 * @class AudioCallbackCache
 * @brief Optimized cache for audio callback to reduce atomic operations
 *
 * This class caches frequently accessed data to minimize atomic loads
 * in the audio callback hot path, improving performance while maintaining
 * thread safety through proper cache invalidation.
 */
class AudioCallbackCache {
public:
    /**
     * @brief Track snapshot structure for caching
     */
    struct CachedTrackSnapshot {
        juce::int64 snapshotVersion{0};
        std::vector<std::shared_ptr<zenith::Track>> tracks;
        std::vector<std::shared_ptr<zenith::AuxBusTrack>> auxBuses;
        bool isValid{false};
    };

    /**
     * @brief Master plugins snapshot structure for caching
     */
    struct CachedMasterSnapshot {
        juce::int64 snapshotVersion{0};
        std::span<const std::shared_ptr<juce::AudioPluginInstance>> plugins;
        std::span<const std::shared_ptr<juce::AudioPluginInstance>> masterEffects;
        bool isValid{false};
    };

    /**
     * @brief Transport cache structure for reducing atomic loads
     */
    struct CachedTransport {
        juce::int64 lastUpdateVersion{0};
        bool isPlaying{false};
        bool isRecording{false};
        bool isLooping{false};
        juce::int64 playheadPosition{0};
        juce::int64 loopStart{0};
        juce::int64 loopEnd{0};
        bool isValid{false};
    };

    /**
     * @brief Constructor
     */
    AudioCallbackCache();

    /**
     * @brief Destructor
     */
    ~AudioCallbackCache() = default;

    /**
     * @brief Update track snapshot cache
     * @param snapshot New track snapshot to cache
     * @return true if cache was updated, false if unchanged
     */
    bool updateTrackSnapshot(const std::shared_ptr<CachedTrackSnapshot>& snapshot);

    /**
     * @brief Update master plugins snapshot cache
     * @param snapshot New master plugins snapshot to cache
     * @return true if cache was updated, false if unchanged
     */
    bool updateMasterSnapshot(const std::shared_ptr<CachedMasterSnapshot>& snapshot);

    /**
     * @brief Update transport cache
     * @param transport New transport state to cache
     * @return true if cache was updated, false if unchanged
     */
    bool updateTransport(const std::shared_ptr<CachedTransport>& transport);

    /**
     * @brief Get cached track snapshot
     * @return Pointer to cached track snapshot, or nullptr if invalid
     */
    const CachedTrackSnapshot* getTrackSnapshot() const;

    /**
     * @brief Get cached master plugins snapshot
     * @return Pointer to cached master plugins snapshot, or nullptr if invalid
     */
    const CachedMasterSnapshot* getMasterSnapshot() const;

    /**
     * @brief Get cached transport state
     * @return Pointer to cached transport state, or nullptr if invalid
     */
    const CachedTransport* getTransport() const;

    /**
     * @brief Invalidate all caches (call from message thread when data changes)
     */
    void invalidateAll();

    /**
     * @brief Invalidate specific cache by type
     * @param cacheType Type of cache to invalidate
     */
    void invalidateCache(CacheType cacheType);

    /**
     * @brief Check if track snapshot cache needs update
     * @param currentVersion Current version from source
     * @return true if cache needs update
     */
    bool needsTrackSnapshotUpdate(juce::int64 currentVersion) const;

    /**
     * @brief Check if master snapshot cache needs update
     * @param currentVersion Current version from source
     * @return true if cache needs update
     */
    bool needsMasterSnapshotUpdate(juce::int64 currentVersion) const;

    /**
     * @brief Check if transport cache needs update
     * @param currentVersion Current version from source
     * @return true if cache needs update
     */
    bool needsTransportUpdate(juce::int64 currentVersion) const;

    /**
     * @brief Get cache hit/miss statistics
     * @return Pair of (hits, misses)
     */
    std::pair<size_t, size_t> getStatistics() const;

    /**
     * @brief Reset cache statistics
     */
    void resetStatistics();

private:
    /**
     * @brief Cache types for selective invalidation
     */
    enum class CacheType {
        TrackSnapshot,
        MasterSnapshot,
        Transport,
        All
    };

    // Cache storage
    std::shared_ptr<CachedTrackSnapshot> trackSnapshotCache_;
    std::shared_ptr<CachedMasterSnapshot> masterSnapshotCache_;
    std::shared_ptr<CachedTransport> transportCache_;

    // Cache version tracking
    std::atomic<juce::int64> trackSnapshotVersion_{0};
    std::atomic<juce::int64> masterSnapshotVersion_{0};
    std::atomic<juce::int64> transportVersion_{0};

    // Statistics
    mutable std::atomic<size_t> cacheHits_{0};
    mutable std::atomic<size_t> cacheMisses_{0};

    // Cache lock for updates
    mutable juce::CriticalSection cacheLock_;

    /**
     * @brief Check if cache needs update based on version
     */
    template<typename T>
    bool needsUpdate(const std::shared_ptr<T>& cache, juce::int64 currentVersion,
                     std::atomic<juce::int64>& version) const {
        if (!cache || !cache->isValid) {
            return true;
        }
        return currentVersion != version.load();
    }

    /**
     * @brief Update cache with new data
     */
    template<typename T>
    void updateCache(std::shared_ptr<T>& cache,
                     const std::shared_ptr<T>& newData,
                     std::atomic<juce::int64>& version) {
        juce::ScopedLock lock(cacheLock_);

        if (newData && newData->isValid) {
            cache = newData;
            version.store(newData->snapshotVersion);
            cacheHits_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    /**
     * @brief Get cache with statistics tracking
     */
    template<typename T>
    const T* getCached(const std::shared_ptr<T>& cache) const {
        if (cache && cache->isValid) {
            cacheHits_.fetch_add(1, std::memory_order_relaxed);
            return cache.get();
        }
        cacheMisses_.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
    }
};

/**
 * @class OptimizedAudioCallback
 * @brief Audio callback using cache for improved performance
 */
class OptimizedAudioCallback {
public:
    /**
     * @brief Constructor
     * @param cache Reference to the cache instance
     */
    OptimizedAudioCallback(AudioCallbackCache& cache);

    /**
     * @brief Process audio with cached data
     * @param outputChannelData Output buffer data
     * @param numOutputChannels Number of output channels
     * @param numSamples Number of samples to process
     * @param midiBuffer MIDI buffer for processing
     */
    void processAudio(float** outputChannelData, int numOutputChannels, int numSamples,
                     juce::MidiBuffer& midiBuffer) noexcept;

private:
    AudioCallbackCache& cache_;

    /**
     * @brief Process tracks with cached data
     */
    void processTracks(float** outputChannelData, int numOutputChannels, int numSamples) const;

    /**
     * @brief Process master plugins with cached data
     */
    void processMasterPlugins(float** outputChannelData, int numOutputChannels, int numSamples) const;

    /**
     * @brief Process MIDI with cached transport data
     */
    void processMIDI(juce::MidiBuffer& midiBuffer, int numSamples) const;

    /**
     * @brief Apply transport effects with cached state
     */
    void applyTransportEffects(float** outputChannelData, int numOutputChannels, int numSamples) const;
};

} // namespace zenith