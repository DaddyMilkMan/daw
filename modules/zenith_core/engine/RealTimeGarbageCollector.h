/*
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
  // Singleton instance for global access
  static RealTimeGarbageCollector& getInstance();
  static void deleteInstance();

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
    
    // Destructor explicitly invokes the deleter
    ~TrashItem() {
      if (deleter) {
        deleter();
      }
    }
    
    // Move constructor/assignment to properly transfer ownership
    TrashItem() = default;
    TrashItem(std::function<void()> deleterIn, uint32_t timeMs)
      : deleter(std::move(deleterIn)), insertionTimeMs(timeMs) {}
    TrashItem(TrashItem&& other) noexcept 
      : deleter(std::move(other.deleter)), 
        insertionTimeMs(other.insertionTimeMs) {
      other.deleter = nullptr; // Prevent double-deletion
    }
    TrashItem& operator=(TrashItem&& other) noexcept {
      if (this != &other) {
        // Invoke old deleter if present before replacing
        if (deleter) {
          deleter();
        }
        deleter = std::move(other.deleter);
        insertionTimeMs = other.insertionTimeMs;
        other.deleter = nullptr; // Prevent double-deletion
      }
      return *this;
    }
    
    // Delete copy constructor/assignment
    TrashItem(const TrashItem&) = delete;
    TrashItem& operator=(const TrashItem&) = delete;
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
