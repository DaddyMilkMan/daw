/*
  ==============================================================================

    AIEventBus.cpp
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Implementation of the AI event bus.

  ==============================================================================
*/

#include "AIEventBus.h"

namespace zenith {
namespace ai {

//==============================================================================
// Constructor
//==============================================================================

AIEventBus::AIEventBus() {
  // Reserve space for recent events
  recentEvents_.reserve(50);
}

//==============================================================================
// Publishing
//==============================================================================

void AIEventBus::publish(const AIEvent &event) {
  std::vector<AIEventCallback> callbacksToInvoke;

  {
    juce::ScopedLock sl(lock_);

    // Store in recent events
    recentEvents_.push_back(event);
    if (recentEvents_.size() > 50) {
      recentEvents_.erase(recentEvents_.begin());
    }

    stats_.totalPublished++;

    // Collect callbacks for this event type
    auto it = subscriptions_.find(event.type);
    if (it != subscriptions_.end()) {
      for (const auto &sub : it->second) {
        callbacksToInvoke.push_back(sub.callback);
      }
    }
  }

  // Invoke callbacks asynchronously on message thread (outside lock)
  if (!callbacksToInvoke.empty()) {
    juce::MessageManager::callAsync([callbacksToInvoke, event, this]() {
      int deliveredCount = 0;
      for (const auto &callback : callbacksToInvoke) {
        if (callback) {
          callback(event);
          deliveredCount++;
        }
      }
      
      if (deliveredCount > 0) {
        juce::ScopedLock sl(lock_);
        stats_.totalDelivered += deliveredCount;
      }
    });
  }

  DBG("AIEventBus: Published " + event.getTypeName() + " from " +
      event.sourceAgent + " (" + juce::String(callbacksToInvoke.size()) +
      " subscribers)");
}

void AIEventBus::publish(AIEventType type, const juce::String &sourceAgent,
                         const juce::var &payload) {
  AIEvent event(type, sourceAgent, payload);
  publish(event);
}

//==============================================================================
// Subscriptions
//==============================================================================

int AIEventBus::subscribe(AIEventType eventType,
                          const juce::String &subscriberName,
                          AIEventCallback callback) {
  juce::ScopedLock sl(lock_);

  AIEventSubscription sub;
  sub.id = nextSubscriptionId_++;
  sub.eventType = eventType;
  sub.callback = std::move(callback);
  sub.subscriberName = subscriberName;

  subscriptions_[eventType].push_back(sub);
  stats_.activeSubscriptions++;

  DBG("AIEventBus: " + subscriberName + " subscribed to " +
      AIEvent(eventType, "").getTypeName() + " (ID: " + juce::String(sub.id) +
      ")");

  return sub.id;
}

void AIEventBus::unsubscribe(int subscriptionId) {
  juce::ScopedLock sl(lock_);

  for (auto &[eventType, subs] : subscriptions_) {
    for (auto it = subs.begin(); it != subs.end(); ++it) {
      if (it->id == subscriptionId) {
        DBG("AIEventBus: Unsubscribed " + it->subscriberName +
            " (ID: " + juce::String(subscriptionId) + ")");
        subs.erase(it);
        stats_.activeSubscriptions--;
        return;
      }
    }
  }
}

void AIEventBus::unsubscribeAll(const juce::String &subscriberName) {
  juce::ScopedLock sl(lock_);

  int removed = 0;

  for (auto &[eventType, subs] : subscriptions_) {
    for (auto it = subs.begin(); it != subs.end();) {
      if (it->subscriberName == subscriberName) {
        it = subs.erase(it);
        stats_.activeSubscriptions--;
        removed++;
      } else {
        ++it;
      }
    }
  }

  DBG("AIEventBus: Unsubscribed all for " + subscriberName + " (" +
      juce::String(removed) + " subscriptions)");
}

//==============================================================================
// Query
//==============================================================================

int AIEventBus::getSubscriberCount(AIEventType eventType) const {
  juce::ScopedLock sl(lock_);

  auto it = subscriptions_.find(eventType);
  if (it != subscriptions_.end()) {
    return static_cast<int>(it->second.size());
  }

  return 0;
}

std::vector<AIEvent> AIEventBus::getRecentEvents() const {
  juce::ScopedLock sl(lock_);
  return recentEvents_;
}

//==============================================================================
// Statistics
//==============================================================================

AIEventBus::Stats AIEventBus::getStats() const {
  juce::ScopedLock sl(lock_);
  return stats_;
}

void AIEventBus::resetStats() {
  juce::ScopedLock sl(lock_);
  stats_.totalPublished = 0;
  stats_.totalDelivered = 0;
  // Don't reset activeSubscriptions as it's a live count
}

} // namespace ai
} // namespace zenith
