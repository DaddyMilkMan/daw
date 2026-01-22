/*
  ==============================================================================

    RealTimeGarbageCollector.cpp
    Created: 2025-12-25
    Author:  Zenith DAW

  ==============================================================================
*/

#include "RealTimeGarbageCollector.h"

namespace zenith {

static std::atomic<RealTimeGarbageCollector*> gInstance{nullptr};
static std::mutex gInstanceMutex;

RealTimeGarbageCollector& RealTimeGarbageCollector::getInstance() {
  RealTimeGarbageCollector* instance = gInstance.load(std::memory_order_acquire);
  if (instance == nullptr) {
      std::lock_guard<std::mutex> lock(gInstanceMutex);
      instance = gInstance.load(std::memory_order_relaxed);
      if (instance == nullptr) {
          instance = new RealTimeGarbageCollector();
          gInstance.store(instance, std::memory_order_release);
      }
  }
  return *instance;
}

void RealTimeGarbageCollector::deleteInstance() {
    std::lock_guard<std::mutex> lock(gInstanceMutex);
    RealTimeGarbageCollector* instance = gInstance.load(std::memory_order_relaxed);
    if (instance != nullptr) {
        delete instance;
        gInstance.store(nullptr, std::memory_order_release);
    }
}

RealTimeGarbageCollector::RealTimeGarbageCollector() {
  trashBuffer_.resize(kMaxTrashItems);
  // Run cleanup every 100ms
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
    startTimer(100);
}

RealTimeGarbageCollector::~RealTimeGarbageCollector() {
  ensureClean();
}

void RealTimeGarbageCollector::deferDelete(std::function<void()> deleter) {
  if (!deleter) return;

  const uint32_t now = juce::Time::getMillisecondCounter();
  
  // Multiple producers: synchronize access to FIFO write
  const juce::SpinLock::ScopedLockType sl(writeLock_);
  
  int start1, size1, start2, size2;
  fifo_.prepareToWrite(1, start1, size1, start2, size2);
  
  if (size1 > 0) {
    trashBuffer_[start1] = { std::move(deleter), now };
    fifo_.finishedWrite(1);
  } else {
    // Buffer full - this is bad, but at least we don't crash.
    // In a production DAW we'd want to log this.
    jassertfalse; 
  }
}

void RealTimeGarbageCollector::ensureClean() {
  stopTimer();
  
  // 1. Flush all items from FIFO into pendingDestruction_
  int start1, size1, start2, size2;
  int numReady = fifo_.getNumReady();
  
  if (numReady > 0) {
    fifo_.prepareToRead(numReady, start1, size1, start2, size2);
    
    // Move items from trashBuffer_ to pendingDestruction_
    for (int i = 0; i < size1; ++i)
      pendingDestruction_.push_back(std::move(trashBuffer_[start1 + i]));
      
    for (int i = 0; i < size2; ++i)
      pendingDestruction_.push_back(std::move(trashBuffer_[start2 + i]));
      
    fifo_.finishedRead(size1 + size2);
  }
  
  // 2. Execute all deleters by clearing pendingDestruction_
  // The TrashItem destructors will run, executing the deleter functions
  pendingDestruction_.clear();
  
  // 3. Clear trash buffer (should already be moved from)
  trashBuffer_.clear();
}

void RealTimeGarbageCollector::timerCallback() {
  const uint32_t now = juce::Time::getMillisecondCounter();
  
  // 1. Pull new items from FIFO into pendingDestruction_
  int start1, size1, start2, size2;
  int numReady = fifo_.getNumReady();
  
  if (numReady > 0) {
    fifo_.prepareToRead(numReady, start1, size1, start2, size2);
    
    for (int i = 0; i < size1; ++i)
      pendingDestruction_.push_back(std::move(trashBuffer_[start1 + i]));
      
    for (int i = 0; i < size2; ++i)
      pendingDestruction_.push_back(std::move(trashBuffer_[start2 + i]));
      
    fifo_.finishedRead(size1 + size2);
  }

  // 2. Separate expired items
  std::vector<TrashItem> toDestroy;
  std::vector<TrashItem> kept;
  kept.reserve(pendingDestruction_.size());
  
  for (auto& item : pendingDestruction_) {
    // Handle wraparound
    bool expired = (now >= item.insertionTimeMs + kSafetyDurationMs) || 
                   (now < item.insertionTimeMs && (now + (UINT32_MAX - item.insertionTimeMs)) > kSafetyDurationMs);
                   
    if (expired) {
      toDestroy.push_back(std::move(item));
    } else {
      kept.push_back(std::move(item));
    }
  }
  
  pendingDestruction_ = std::move(kept);
  
  // 3. Destructors run here as toDestroy goes out of scope
  toDestroy.clear();
}

} // namespace zenith
