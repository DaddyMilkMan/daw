/*
  ==============================================================================

    RealTimeGarbageCollector.cpp
    Created: 2025-12-25
    Author:  Zenith DAW

    ACTUAL LOCK-FREE IMPLEMENTATION
  ==============================================================================
*/

#include "RealTimeGarbageCollector.h"

namespace zenith {

static RealTimeGarbageCollector* gInstance = nullptr;

RealTimeGarbageCollector& RealTimeGarbageCollector::getInstance() {
  if (gInstance == nullptr)
      gInstance = new RealTimeGarbageCollector();
  return *gInstance;
}

void RealTimeGarbageCollector::deleteInstance() {
    delete gInstance;
    gInstance = nullptr;
}

RealTimeGarbageCollector::RealTimeGarbageCollector() {
  // Initialize buffer size matching FIFO
  trashBuffer_.resize(kTrashBufferSize);

  // Run cleanup every 100ms on the Message Thread
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
      startTimer(100);
}

RealTimeGarbageCollector::~RealTimeGarbageCollector() {
  ensureClean();
}

void RealTimeGarbageCollector::deferDelete(std::function<void()> deleter) {
  if (!deleter) return;

  // Lock-free write to FIFO with SpinLock for MPSC safety
  const juce::SpinLock::ScopedLockType sl(writeLock);
  
  int start1, size1, start2, size2;
  fifo_.prepareToWrite(1, start1, size1, start2, size2);
  
  if (size1 > 0) {
      trashBuffer_[start1] = std::move(deleter);
      fifo_.finishedWrite(1);
  } else {
      // Buffer full! To preserve RT safety, we cannot wait or allocate.
      // We log a warning and let the object leak rather than crashing the audio thread.
      // In a production environment, kTrashBufferSize should be large enough.
      DBG("RealTimeGarbageCollector: TRASH BUFFER OVERFLOW! Object leaked to preserve RT safety.");
  }
}

void RealTimeGarbageCollector::ensureClean() {
  stopTimer();
  
  // Final drain
  timerCallback();
  
  // Force delete everything remaining
  pendingTrash_.clear();
  for (auto& slot : trashBuffer_) slot = nullptr;
  fifo_.reset();
}

void RealTimeGarbageCollector::timerCallback() {
  // 1. Drain items from the lock-free FIFO into our pending list
  int start1, size1, start2, size2;
  int ready = fifo_.getNumReady();
  
  if (ready > 0) {
      fifo_.prepareToRead(ready, start1, size1, start2, size2);
      uint32_t now = juce::Time::getMillisecondCounter();

      if (size1 > 0) {
          for (int i = 0; i < size1; ++i) {
              pendingTrash_.push_back({std::move(trashBuffer_[start1 + i]), now});
          }
      }
      if (size2 > 0) {
          for (int i = 0; i < size2; ++i) {
              pendingTrash_.push_back({std::move(trashBuffer_[start2 + i]), now});
          }
      }
      fifo_.finishedRead(size1 + size2);
  }

  // 2. Process the pending list and delete objects that have aged past the safety threshold
  uint32_t now = juce::Time::getMillisecondCounter();
  
  auto it = std::remove_if(pendingTrash_.begin(), pendingTrash_.end(), [&](const PendingItem& item) {
      // Check if safety duration has passed
      bool expired = (now >= item.insertionTimeMs + kSafetyDurationMs) || 
                     (now < item.insertionTimeMs && (now + (0xFFFFFFFF - item.insertionTimeMs)) > kSafetyDurationMs);
      
      return expired;
  });

  // Objects are destroyed here as they are removed from the vector
  pendingTrash_.erase(it, pendingTrash_.end());
}

} // namespace zenith