/*
  ==============================================================================

    RealTimeGarbageCollector.h
    Created: 2025-12-25
    Author:  Zenith DAW

    A release-pool garbage collector for real-time audio threads.
    Allows the message thread to safely delete objects that might still be
    referenced by the audio thread, by deferring deletion until a safe
    interval has passed.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace zenith {

/**
 * @class RealTimeGarbageCollector
 * @brief Manages deferred deletion of objects used by real-time threads.
 *
 * This class ensures that objects replaced on the message thread are not
 * deleted while the audio thread (or other RT threads) might still be
 * reading them. It uses a time-based approach (or generation-based in future)
 * to keep objects alive for a safety period (default 1 second).
 *
 * Usage:
 * 1. Instantiate on Message Thread (or Engine).
 * 2. Start the cleanup timer (e.g. startTimer(500)).
 * 3. When replacing a shared object:
 *    auto oldObj = activeObj.exchange(newObj);
 *    gc.deferDelete(oldObj);
 */
class RealTimeGarbageCollector : private juce::Timer {
public:
  /**
   * @brief Get the singleton instance (thread-safe Meyer's singleton)
   * 
   * This uses C++11's thread-safe static initialization. The instance is
   * created on first access and lives for the entire program lifetime.
   * 
   * @note The garbage collector should be accessed early in the application
   * lifecycle to ensure proper initialization before any audio threads start.
   */
  static RealTimeGarbageCollector& getInstance();

  RealTimeGarbageCollector();
  ~RealTimeGarbageCollector() override;

  /**
   * @brief Defer deletion of a shared pointer
   * @param object The object to hold alive
   */
  template <typename T>
  void deferDelete(std::shared_ptr<T> object) {
    if (!object) return;
    
    // Type erasure using std::function/lambda to hold the shared_ptr
    deferDelete([object]() mutable { 
        // Object will be destroyed when this lambda is destroyed
        (void)object; 
    });
  }

  /**
   * @brief Defer deletion of a JUCE ReferenceCountedObjectPtr
   * @param object The object to hold alive
   */
  template <typename T>
  void deferDelete(juce::ReferenceCountedObjectPtr<T> object) {
    if (!object) return;
    
    deferDelete([object]() mutable { 
        (void)object; 
    });
  }

  /**
   * @brief Defer execution of a cleanup function
   * @param deleter Function to execute (or object to destroy)
   */
  void deferDelete(std::function<void()> deleter);

  /**
   * @brief Force cleanup of all pending objects (Message Thread Only)
   * Call this on shutdown.
   */
  void ensureClean();

private:
  void timerCallback() override;

  struct TrashItem {
    std::function<void()> deleter;
    uint32_t insertionTimeMs;
  };

  // Queue of items to delete
  // Using a larger fixed-size buffer with AbstractFifo for MPSC safety
  static constexpr int kMaxTrashItems = 4096;
  std::vector<TrashItem> trashBuffer_;
  juce::AbstractFifo fifo_{kMaxTrashItems};
  
  // Multiple producers (Audio Threads) need to synchronize their writes
  juce::SpinLock writeLock_;

  // Items currently waiting for the safety duration to pass
  std::vector<TrashItem> pendingDestruction_;

  // Safety buffer duration in milliseconds
  static constexpr uint32_t kSafetyDurationMs = 1000;
};

} // namespace zenith
