#pragma once

#include <deque>
#include <functional>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <memory>
#include <mutex>

namespace zenith {

/**
 * @class RealTimeGarbageCollector
 * @brief Thread-safe utility to defer object deletion from the audio thread.
 */
class RealTimeGarbageCollector : public juce::Timer {
public:
  RealTimeGarbageCollector() { startTimer(100); }

  ~RealTimeGarbageCollector() override {
    stopTimer();
    clear();
  }

  /** Defer deletion of a raw pointer */
  template <typename T> void push(T *object) {
    if (object == nullptr)
      return;
    const std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back([object]() { delete object; });
  }

  /** Defer deletion of a shared pointer */
  template <typename T> void push(std::shared_ptr<T> ptr) {
    if (!ptr)
      return;
    const std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back([ptr]() { /* deleted when lambda is destroyed */ });
  }

  /** Defer deletion of a JUCE ReferenceCountedObjectPtr */
  template <typename T> void push(juce::ReferenceCountedObjectPtr<T> ptr) {
    if (!ptr)
      return;
    const std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back([ptr]() { /* deleted when lambda is destroyed */ });
  }

  /** Message thread only: clear the queue and perform deletions */
  void clear() {
    std::deque<std::function<void()>> toDelete;
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      toDelete.swap(queue_);
    }
    toDelete.clear();
  }

  static RealTimeGarbageCollector &getInstance() {
    static RealTimeGarbageCollector instance;
    return instance;
  }

private:
  void timerCallback() override { clear(); }

  std::deque<std::function<void()>> queue_;
  std::mutex mutex_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeGarbageCollector)
};

} // namespace zenith
