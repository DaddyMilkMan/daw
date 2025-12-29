/*
  ==============================================================================

    AIResponseCache.h
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    SQLite-backed cache for expensive AI API responses.
    Reduces API calls and improves response times.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <map>
#include <optional>
#include <thread>

namespace zenith {
namespace ai {

//==============================================================================
/**
    A cached response entry
*/
struct CacheEntry {
  juce::String promptHash; // SHA256 hash of prompt
  juce::String response;   // The cached response
  juce::int64 cachedAt;    // Timestamp when cached (ms since epoch)
  juce::int64 expiresAt;   // Expiration timestamp
  int hitCount = 0;        // Number of cache hits

  bool isExpired() const { return juce::Time::currentTimeMillis() > expiresAt; }
};

//==============================================================================
/**
    Cache statistics for monitoring
*/
struct CacheStats {
  int totalEntries = 0;
  int hits = 0;
  int misses = 0;
  juce::int64 totalSizeBytes = 0;
  float hitRate = 0.0f; // 0.0 - 1.0

  void updateHitRate() {
    int total = hits + misses;
    hitRate = total > 0 ? static_cast<float>(hits) / total : 0.0f;
  }
};

//==============================================================================
/**
    SQLite-backed cache for AI API responses.

    Features:
    - SHA256 hash keys for prompt deduplication
    - Configurable TTL per entry
    - LRU eviction when cache exceeds size limit
    - Thread-safe operations

    Usage:
      auto& cache = AIResponseCache::getInstance();

      // Check cache first
      auto cached = cache.get(promptHash);
      if (cached.has_value()) {
        return *cached; // Cache hit!
      }

      // Make API call...
      juce::String response = callAPI(prompt);

      // Store in cache
      cache.put(promptHash, response, 3600); // 1 hour TTL
*/
class AIResponseCache {
public:
  //============================================================================
  // Singleton Access
  //============================================================================

  static AIResponseCache &getInstance() {
    static AIResponseCache instance;
    return instance;
  }

  //============================================================================
  // Core API
  //============================================================================

  /**
   * Get a cached response by prompt hash.
   * @param promptHash SHA256 hash of the prompt
   * @return Optional containing response if found and not expired
   */
  std::optional<juce::String> get(const juce::String &promptHash);

  /**
   * Store a response in the cache.
   * @param promptHash SHA256 hash of the prompt
   * @param response The response to cache
   * @param ttlSeconds Time-to-live in seconds
   */
  void put(const juce::String &promptHash, const juce::String &response,
           int ttlSeconds = 3600);

  /**
   * Check if a prompt is cached (without updating hit count).
   */
  bool has(const juce::String &promptHash) const;

  /**
   * Invalidate entries matching a pattern.
   * @param pattern Glob pattern for prompt hashes (empty = all)
   */
  void invalidate(const juce::String &pattern = "");

  /**
   * Clear the entire cache.
   */
  void clear();

  //============================================================================
  // Hash Generation
  //============================================================================

  /**
   * Generate a hash key from system message and prompt.
   * @param systemMessage The system message
   * @param prompt The user prompt
   * @return SHA256 hash as hex string
   */
  static juce::String generateHash(const juce::String &systemMessage,
                                   const juce::String &prompt);

  //============================================================================
  // Statistics
  //============================================================================

  CacheStats getStats() const;
  void resetStats();

  //============================================================================
  // Configuration
  //============================================================================

  /**
   * Set maximum cache size in bytes.
   * When exceeded, LRU eviction is triggered.
   */
  void setMaxSizeBytes(juce::int64 maxBytes) { maxSizeBytes_ = maxBytes; }

  /**
   * Set default TTL for new entries.
   */
  void setDefaultTTL(int seconds) { defaultTTLSeconds_ = seconds; }

  /**
   * Enable or disable the cache.
   */
  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool isEnabled() const { return enabled_; }

private:
  AIResponseCache();
  ~AIResponseCache();

  // Database operations
  void initDatabase();
  void evictLRU();
  void persistCache();
  void persistCache(const std::map<juce::String, ai::CacheEntry>& snapshot);
  juce::int64 calculateTotalSize() const;

  juce::File getCacheFile() const;

  mutable juce::CriticalSection cacheLock_;
  mutable juce::CriticalSection diskLock_;

  // In-memory cache (backed by SQLite for persistence)
  std::map<juce::String, ai::CacheEntry> cache_;

  // SQLite database handle (using juce::File for now, could use raw SQLite)
  juce::File cacheDir_;

  mutable CacheStats stats_;
  std::atomic<bool> enabled_{true};
  juce::int64 maxSizeBytes_ = 100 * 1024 * 1024; // 100 MB default
  int defaultTTLSeconds_ = 3600;                 // 1 hour

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIResponseCache)
};

} // namespace ai
} // namespace zenith
