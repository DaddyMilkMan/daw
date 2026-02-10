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

  cache_[promptHash] = entry;
  stats_.totalEntries = static_cast<int>(cache_.size());

  // Check if we need to evict
  if (calculateTotalSize() > maxSizeBytes_) {
    evictLRU();
  }
  
  // Create snapshot for async persistence
  auto cacheSnapshot = cache_;

  // Persist to disk asynchronously to avoid blocking
  std::thread([this, snapshot = std::move(cacheSnapshot)]() {
      persistCache(snapshot);
  }).detach();

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
    stats_.totalEntries = 0;
  } else {
    // Remove matching entries
    for (auto it = cache_.begin(); it != cache_.end();) {
      if (it->first.matchesWildcard(pattern, true)) {
        it = cache_.erase(it);
      } else {
        ++it;
      }
    }
    stats_.totalEntries = static_cast<int>(cache_.size());
  }

  // Create snapshot for async persistence
  auto cacheSnapshot = cache_;

  // Persist to disk asynchronously
  std::thread([this, snapshot = std::move(cacheSnapshot)]() {
      persistCache(snapshot);
  }).detach();
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
  // Note: cacheLock_ should already be held by caller

  // Find entries with lowest hit count
  std::vector<std::pair<juce::String, int>> entries;
  for (const auto &pair : cache_) {
    entries.emplace_back(pair.first, pair.second.hitCount);
  }

  // Sort by hit count (ascending)
  std::sort(entries.begin(), entries.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; });

  // Remove lowest 25%
  size_t toRemove = entries.size() / 4;
  if (toRemove == 0)
    toRemove = 1;

  for (size_t i = 0; i < toRemove && i < entries.size(); ++i) {
    cache_.erase(entries[i].first);
  }

  stats_.totalEntries = static_cast<int>(cache_.size());

  DBG("AIResponseCache: Evicted " + juce::String(toRemove) + " entries");
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

void AIResponseCache::persistCache(const std::map<juce::String, ai::CacheEntry>& snapshot) {
  juce::ScopedLock sl(diskLock_);

  // Build JSON object from snapshot
  juce::DynamicObject::Ptr root = new juce::DynamicObject();

  for (const auto &pair : snapshot) {
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
  
  // Use a temporary file and move for atomicity
  auto tempFile = cacheFile.getSiblingFile(cacheFile.getFileName() + ".tmp");
  tempFile.replaceWithText(jsonStr);
  tempFile.moveFileTo(cacheFile);
}

void AIResponseCache::persistCache() {
    // Overload for internal use if needed, but we prefer the snapshot version
    // This is kept if existing code calls it, though we updated call sites.
    // For safety, we'll take a lock and snapshot here if called directly.
    std::map<juce::String, ai::CacheEntry> snapshot;
    {
        juce::ScopedLock sl(cacheLock_);
        snapshot = cache_;
    }
    persistCache(snapshot);
}

} // namespace ai
} // namespace zenith
