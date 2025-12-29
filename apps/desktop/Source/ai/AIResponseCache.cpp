/*
  ==============================================================================

    AIResponseCache.cpp
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Implementation of the AI response cache.

  ==============================================================================
*/

#include "AIResponseCache.h"

namespace zenith {
namespace ai {

//==============================================================================
// Constructor / Destructor
//==============================================================================

AIResponseCache::AIResponseCache() {
  // Get cache directory in user's app data
  cacheDir_ =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("ZenithDAW")
          .getChildFile("AICache");

  if (!cacheDir_.exists()) {
    cacheDir_.createDirectory();
  }

  // Load existing cache from disk
  initDatabase();

  DBG("AIResponseCache: Initialized at " + cacheDir_.getFullPathName());
}

AIResponseCache::~AIResponseCache() {
  // Persist cache to disk on shutdown
  // (In a full implementation, we'd write to SQLite here)
}

//==============================================================================
// Database Initialization
//==============================================================================

void AIResponseCache::initDatabase() {
  // Load cached entries from JSON file (simpler than SQLite for now)
  juce::File cacheFile = getCacheFile();

  if (cacheFile.existsAsFile()) {
    auto json = juce::JSON::parse(cacheFile);

    if (json.isObject()) {
      auto *obj = json.getDynamicObject();

      for (auto &prop : obj->getProperties()) {
        auto *entryObj = prop.value.getDynamicObject();
        if (entryObj == nullptr)
          continue;

        CacheEntry entry;
        entry.promptHash = prop.name.toString();
        entry.response = entryObj->getProperty("response").toString();
        entry.cachedAt =
            static_cast<juce::int64>(entryObj->getProperty("cachedAt"));
        entry.expiresAt =
            static_cast<juce::int64>(entryObj->getProperty("expiresAt"));
        entry.hitCount = static_cast<int>(entryObj->getProperty("hitCount"));

        // Skip expired entries
        if (!entry.isExpired()) {
          // Initialize LRU for loaded entries (append to front)
          lruList_.push_front(entry.promptHash);
          entry.lruIterator = lruList_.begin();
          cache_[entry.promptHash] = entry;
        }
      }

      stats_.totalEntries = static_cast<int>(cache_.size());
      DBG("AIResponseCache: Loaded " + juce::String(cache_.size()) +
          " cached entries");
    }
  }
}

juce::File AIResponseCache::getCacheFile() const {
  return cacheDir_.getChildFile("cache.json");
}

//==============================================================================
// Core API
//==============================================================================

std::optional<juce::String>
AIResponseCache::get(const juce::String &promptHash) {
  if (!enabled_)
    return std::nullopt;

  juce::ScopedLock sl(cacheLock_);

  auto it = cache_.find(promptHash);
  if (it == cache_.end() || it->second.isExpired()) {
    stats_.misses++;
    stats_.updateHitRate();
    return std::nullopt;
  }

  // Cache hit!
  stats_.hits++;
  stats_.updateHitRate();
  it->second.hitCount++;

  // Move to front of LRU list (O(1))
  lruList_.splice(lruList_.begin(), lruList_, it->second.lruIterator);
  it->second.lruIterator = lruList_.begin();

  DBG("AIResponseCache: HIT for " + promptHash.substring(0, 16) + "...");

  return it->second.response;
}

void AIResponseCache::put(const juce::String &promptHash,
                          const juce::String &response, int ttlSeconds) {
  if (!enabled_)
    return;

  juce::ScopedLock sl(cacheLock_);

  CacheEntry entry;
  entry.promptHash = promptHash;
  entry.response = response;
  entry.cachedAt = juce::Time::currentTimeMillis();
  entry.expiresAt = entry.cachedAt + (ttlSeconds * 1000);
  entry.hitCount = 0;

  // Add to front of LRU list
  lruList_.push_front(promptHash);
  entry.lruIterator = lruList_.begin();

  cache_[promptHash] = entry;
  stats_.totalEntries = static_cast<int>(cache_.size());

  // Check if we need to evict
  if (calculateTotalSize() > maxSizeBytes_) {
    evictLRU();
  }

  // Persist to disk (async would be better in production)
  persistCache();

  DBG("AIResponseCache: PUT " + promptHash.substring(0, 16) +
      "... (TTL: " + juce::String(ttlSeconds) + "s)");
}

bool AIResponseCache::has(const juce::String &promptHash) const {
  if (!enabled_)
    return false;

  juce::ScopedLock sl(cacheLock_);

  auto it = cache_.find(promptHash);
  if (it == cache_.end())
    return false;

  return !it->second.isExpired();
}

void AIResponseCache::invalidate(const juce::String &pattern) {
  juce::ScopedLock sl(cacheLock_);

  if (pattern.isEmpty()) {
    // Clear all
    cache_.clear();
    lruList_.clear();
    stats_.totalEntries = 0;
  } else {
    // Remove matching entries
    for (auto it = cache_.begin(); it != cache_.end();) {
      if (it->first.matchesWildcard(pattern, true)) {
        lruList_.erase(it->second.lruIterator);
        it = cache_.erase(it);
      } else {
        ++it;
      }
    }
    stats_.totalEntries = static_cast<int>(cache_.size());
  }

  persistCache();
}

void AIResponseCache::clear() { invalidate(""); }

//==============================================================================
// Hash Generation
//==============================================================================

juce::String AIResponseCache::generateHash(const juce::String &systemMessage,
                                           const juce::String &prompt) {
  juce::String combined = systemMessage + "\n---\n" + prompt;
  return juce::SHA256(combined.toUTF8()).toHexString();
}

//==============================================================================
// Statistics
//==============================================================================

CacheStats AIResponseCache::getStats() const {
  juce::ScopedLock sl(cacheLock_);
  CacheStats statsCopy = stats_;
  statsCopy.totalSizeBytes = calculateTotalSize();
  return statsCopy;
}

void AIResponseCache::resetStats() {
  juce::ScopedLock sl(cacheLock_);
  stats_.hits = 0;
  stats_.misses = 0;
  stats_.hitRate = 0.0f;
}

//==============================================================================
// LRU Eviction
//==============================================================================

void AIResponseCache::evictLRU() {
  // O(1) Eviction: Remove from back of list
  // Remove entries until size is under limit
  int removedCount = 0;
  while (calculateTotalSize() > maxSizeBytes_ && !lruList_.empty()) {
      juce::String keyToRemove = lruList_.back();
      lruList_.pop_back();
      cache_.erase(keyToRemove);
      removedCount++;
  }

  stats_.totalEntries = static_cast<int>(cache_.size());

  DBG("AIResponseCache: Evicted " + juce::String(removedCount) + " entries");
}

juce::int64 AIResponseCache::calculateTotalSize() const {
  juce::int64 total = 0;
  for (const auto &pair : cache_) {
    total += pair.second.response.getNumBytesAsUTF8();
    total += pair.second.promptHash.getNumBytesAsUTF8();
  }
  return total;
}

//==============================================================================
// Persistence
//==============================================================================

void AIResponseCache::persistCache() {
  // Build JSON object
  juce::DynamicObject::Ptr root = new juce::DynamicObject();

  for (const auto &pair : cache_) {
    juce::DynamicObject::Ptr entryObj = new juce::DynamicObject();
    entryObj->setProperty("response", pair.second.response);
    entryObj->setProperty("cachedAt", pair.second.cachedAt);
    entryObj->setProperty("expiresAt", pair.second.expiresAt);
    entryObj->setProperty("hitCount", pair.second.hitCount);

    root->setProperty(pair.first, juce::var(entryObj.get()));
  }

  // Write to file
  juce::File cacheFile = getCacheFile();
  juce::String jsonStr = juce::JSON::toString(root.get());
  cacheFile.replaceWithText(jsonStr);
}

} // namespace ai
} // namespace zenith
