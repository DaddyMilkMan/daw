/*
  ==============================================================================

    AIStatusManager.cpp
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Implementation of the centralized AI status manager.

  ==============================================================================
*/

#include "AIStatusManager.h"

namespace zenith {
namespace ai {

//==============================================================================
// Constructor / Destructor
//==============================================================================

AIStatusManager::AIStatusManager() {
  // Start timer for stale operation cleanup (every 5 seconds)
  startTimer(5000);
}

AIStatusManager::~AIStatusManager() { stopTimer(); }

//==============================================================================
// Operation Lifecycle
//==============================================================================

juce::String AIStatusManager::generateOperationId() {
  int counter = operationCounter_.fetch_add(1);
  return "op_" + juce::String(juce::Time::currentTimeMillis()) + "_" +
         juce::String(counter);
}

juce::String AIStatusManager::beginOperation(const juce::String &agentName,
                                             const juce::String &description) {
  AIOperation op;
  op.id = generateOperationId();
  op.agentName = agentName;
  op.description = description;
  op.status = AIOperationStatus::Working;
  op.progress = 0.0f;
  op.startedAt = juce::Time::getCurrentTime();

  {
    juce::ScopedLock sl(lock_);
    operations_[op.id] = op;
    stats_.totalOperations++;
  }

  notifyStarted(op);

  DBG("AIStatusManager: Started [" + agentName + "] " + description);

  return op.id;
}

void AIStatusManager::updateProgress(const juce::String &operationId,
                                     float progress,
                                     const juce::String &description) {
  juce::ScopedLock sl(lock_);

  auto it = operations_.find(operationId);
  if (it == operations_.end())
    return;

  it->second.progress = juce::jlimit(0.0f, 1.0f, progress);
  if (description.isNotEmpty()) {
    it->second.description = description;
  }

  notifyProgress(it->second);
}

void AIStatusManager::completeOperation(const juce::String &operationId,
                                        bool success,
                                        const juce::String &message) {
  AIOperation completedOp;

  {
    juce::ScopedLock sl(lock_);

    auto it = operations_.find(operationId);
    if (it == operations_.end())
      return;

    it->second.status =
        success ? AIOperationStatus::Success : AIOperationStatus::Error;
    it->second.progress = 1.0f;
    it->second.completedAt = juce::Time::getCurrentTime();

    if (message.isNotEmpty()) {
      if (success) {
        it->second.description = message;
      } else {
        it->second.errorMessage = message;
      }
    }

    // Update stats
    if (success) {
      stats_.successfulOperations++;
    } else {
      stats_.failedOperations++;
    }

    juce::int64 duration =
        (it->second.completedAt - it->second.startedAt).inMilliseconds();
    stats_.totalDurationMs += duration;

    completedOp = it->second;

    // Move to recent operations
    recentOperations_.insert(recentOperations_.begin(), completedOp);
    if (recentOperations_.size() > 10) {
      recentOperations_.pop_back();
    }

    // Remove from active
    operations_.erase(it);
  }

  if (completedOp.status == AIOperationStatus::Error) {
    notifyError(completedOp);
    DBG("AIStatusManager: ERROR [" + completedOp.agentName + "] " +
        completedOp.errorMessage);
  } else {
    notifyCompleted(completedOp);
    DBG("AIStatusManager: Completed [" + completedOp.agentName + "] " +
        completedOp.description);
  }
}

void AIStatusManager::warnOperation(const juce::String &operationId,
                                    const juce::String &warningMessage) {
  AIOperation warnedOp;

  {
    juce::ScopedLock sl(lock_);

    auto it = operations_.find(operationId);
    if (it == operations_.end())
      return;

    it->second.status = AIOperationStatus::Warning;
    it->second.progress = 1.0f;
    it->second.completedAt = juce::Time::getCurrentTime();
    it->second.errorMessage = warningMessage;

    stats_.warningOperations++;

    juce::int64 duration =
        (it->second.completedAt - it->second.startedAt).inMilliseconds();
    stats_.totalDurationMs += duration;

    warnedOp = it->second;

    // Move to recent operations
    recentOperations_.insert(recentOperations_.begin(), warnedOp);
    if (recentOperations_.size() > 10) {
      recentOperations_.pop_back();
    }

    operations_.erase(it);
  }

  notifyCompleted(warnedOp);
  DBG("AIStatusManager: WARNING [" + warnedOp.agentName + "] " +
      warnedOp.errorMessage);
}

//==============================================================================
// Query Operations
//==============================================================================

std::vector<AIOperation> AIStatusManager::getActiveOperations() const {
  juce::ScopedLock sl(lock_);

  std::vector<AIOperation> result;
  result.reserve(operations_.size());

  for (const auto &[id, op] : operations_) {
    if (op.isActive()) {
      result.push_back(op);
    }
  }

  return result;
}

std::vector<AIOperation> AIStatusManager::getRecentOperations() const {
  juce::ScopedLock sl(lock_);
  return recentOperations_;
}

AIOperation *AIStatusManager::getOperation(const juce::String &operationId) {
  juce::ScopedLock sl(lock_);

  auto it = operations_.find(operationId);
  if (it != operations_.end()) {
    return &it->second;
  }

  return nullptr;
}

bool AIStatusManager::hasActiveOperations() const {
  juce::ScopedLock sl(lock_);

  for (const auto &[id, op] : operations_) {
    if (op.isActive()) {
      return true;
    }
  }

  return false;
}

int AIStatusManager::getActiveOperationCount() const {
  juce::ScopedLock sl(lock_);

  int count = 0;
  for (const auto &[id, op] : operations_) {
    if (op.isActive()) {
      count++;
    }
  }

  return count;
}

//==============================================================================
// Statistics
//==============================================================================

AIStatusManager::Stats AIStatusManager::getStats() const {
  juce::ScopedLock sl(lock_);
  return stats_;
}

void AIStatusManager::resetStats() {
  juce::ScopedLock sl(lock_);
  stats_ = Stats();
}

//==============================================================================
// Listeners
//==============================================================================

void AIStatusManager::addListener(AIStatusListener *listener) {
  listeners_.add(listener);
}

void AIStatusManager::removeListener(AIStatusListener *listener) {
  listeners_.remove(listener);
}

//==============================================================================
// Timer Callback
//==============================================================================

void AIStatusManager::timerCallback() { cleanupStaleOperations(); }

void AIStatusManager::cleanupStaleOperations() {
  std::vector<juce::String> staleIds;

  {
    juce::ScopedLock sl(lock_);

    auto now = juce::Time::getCurrentTime();

    for (auto &[id, op] : operations_) {
      if (op.isActive()) {
        auto elapsed = (now - op.startedAt).inSeconds();
        if (elapsed > staleTimeoutSeconds_) {
          staleIds.push_back(id);
        }
      }
    }
  }

  // Complete stale operations as errors (outside lock to avoid deadlock)
  for (const auto &id : staleIds) {
    completeOperation(id, false, "Operation timed out");
  }
}

//==============================================================================
// Notification Helpers
//==============================================================================

void AIStatusManager::notifyStarted(const AIOperation &op) {
  juce::MessageManager::callAsync([this, op]() {
    listeners_.call(&AIStatusListener::onOperationStarted, op);
  });
}

void AIStatusManager::notifyProgress(const AIOperation &op) {
  juce::MessageManager::callAsync([this, op]() {
    listeners_.call(&AIStatusListener::onOperationProgress, op);
  });
}

void AIStatusManager::notifyCompleted(const AIOperation &op) {
  juce::MessageManager::callAsync([this, op]() {
    listeners_.call(&AIStatusListener::onOperationCompleted, op);
  });
}

void AIStatusManager::notifyError(const AIOperation &op) {
  juce::MessageManager::callAsync([this, op]() {
    listeners_.call(&AIStatusListener::onOperationError, op);
  });
}

} // namespace ai
} // namespace zenith
