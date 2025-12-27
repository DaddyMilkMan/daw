/*
  ==============================================================================

    RealTimeGarbageCollector.cpp
    Created: 2025-12-25
    Author:  Zenith DAW

  ==============================================================================
*/

#include "RealTimeGarbageCollector.h"

namespace zenith {

RealTimeGarbageCollector& RealTimeGarbageCollector::getInstance() {
  static RealTimeGarbageCollector instance;
  return instance;
}

RealTimeGarbageCollector::RealTimeGarbageCollector() {
  // Run cleanup every 100ms
  startTimer(100);
}

RealTimeGarbageCollector::~RealTimeGarbageCollector() {
  ensureClean();
}

void RealTimeGarbageCollector::deferDelete(std::function<void()> deleter) {
  if (!deleter) return;

  const juce::ScopedLock sl(trashLock_);
  trash_.push_back({std::move(deleter), juce::Time::getMillisecondCounter()});
}

void RealTimeGarbageCollector::ensureClean() {
  stopTimer();
  const juce::ScopedLock sl(trashLock_);
  trash_.clear(); // Destructors run here
}

void RealTimeGarbageCollector::timerCallback() {
  const uint32_t now = juce::Time::getMillisecondCounter();
  
  // We need to remove items, so we can't iterate simply.
  // Move items to keep into a new vector? Or just remove_if.
  // Actually, destructors running inside the lock is fine if they are message thread safe.
  
  // Safe extraction to a temp list to destroy OUTSIDE the lock (optional but good practice)
  std::vector<TrashItem> toDestroy;

  {
    const juce::ScopedLock sl(trashLock_);
    
    // Partition items into 'expired' and 'keep'
    // Items are roughly ordered by time, so we could just pop front.
    // But let's be robust.
    
    auto it = std::remove_if(trash_.begin(), trash_.end(), [&](const TrashItem& item) {
      // Handle wraparound of tick count (unlikely to matter for 1s difference but strict correctness)
      return (now >= item.insertionTimeMs + kSafetyDurationMs) || 
             (now < item.insertionTimeMs && (now + (UINT32_MAX - item.insertionTimeMs)) > kSafetyDurationMs);
    });

    // Move expired items to toDestroy (manually, since remove_if just shifts)
    // Actually standard remove_if doesn't move to another container.
    // Let's just create a new list for kept items, it's easier.
    
    std::vector<TrashItem> kept;
    kept.reserve(trash_.size());
    
    for (auto& item : trash_) {
      if ((now >= item.insertionTimeMs + kSafetyDurationMs) || 
          (now < item.insertionTimeMs && (now + (UINT32_MAX - item.insertionTimeMs)) > kSafetyDurationMs)) {
        toDestroy.push_back(std::move(item));
      } else {
        kept.push_back(std::move(item));
      }
    }
    
    trash_ = std::move(kept);
  }

  // toDestroy goes out of scope here, running destructors (releasing sharedprobs)
}

} // namespace zenith
