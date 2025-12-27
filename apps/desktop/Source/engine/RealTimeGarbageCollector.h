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
  // Singleton instance for global access (optional but convenient)
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
  // Access protected by lock (Message Thread writes, Timer Thread reads/writes)
  // Since Timer usually runs on Message Thread, this lock might be redundant if single-threaded,
  // but we enforce safety.
  std::vector<TrashItem> trash_;
  juce::CriticalSection trashLock_;

  // Safety buffer duration in milliseconds
  static constexpr uint32_t kSafetyDurationMs = 1000;
};

} // namespace zenith
