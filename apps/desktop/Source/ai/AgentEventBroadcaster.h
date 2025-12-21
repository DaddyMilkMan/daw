/*
  ==============================================================================

    AgentEventBroadcaster.h
    Created: 2025-12-19
    Author:  Antigravity AI

    Central hub for broadcasting agent events (completion, progress, errors)
    to the Wingman AI and other interested listeners.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace zenith {
namespace ai {

/**
 * Represents an event emitted by an autonomous agent.
 */
struct AgentEvent {
  juce::String agentName; // e.g., "SampleHunter", "UXDirector"
  juce::String eventType; // e.g., "COMPLETED", "PROGRESS", "ERROR"
  juce::String message;   // Human-readable summary
  juce::var data;         // Structured payload for Grok context
  juce::Time timestamp = juce::Time::getCurrentTime();

  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("agent", agentName);
    obj->setProperty("type", eventType);
    obj->setProperty("message", message);
    obj->setProperty("data", data);
    obj->setProperty("timestamp", timestamp.toISO8601(true));
    return juce::var(obj);
  }
};

/**
 * Thread-safe singleton for broadcasting and tracking agent history.
 * GrokDAWController listens to this to update Wingman and its own context.
 */
class AgentEventBroadcaster : public juce::ChangeBroadcaster {
public:
  static AgentEventBroadcaster &getInstance() {
    static AgentEventBroadcaster instance;
    return instance;
  }

  /**
   * Broadcast a new event.
   */
  void broadcast(const AgentEvent &event) {
    const juce::ScopedLock lock(eventLock_);
    history_.add(event);

    // Keep history manageable
    if (history_.size() > 100)
      history_.remove(0);

    sendChangeMessage();
  }

  /**
   * Overload for common event types
   */
  void broadcast(const juce::String &agent, const juce::String &type,
                 const juce::String &message,
                 const juce::var &data = juce::var()) {
    AgentEvent event;
    event.agentName = agent;
    event.eventType = type;
    event.message = message;
    event.data = data;
    broadcast(event);
  }

  /**
   * Get recently broadcasted events.
   */
  juce::Array<AgentEvent> getHistory() const {
    const juce::ScopedLock lock(eventLock_);
    return history_;
  }

  /**
   * Get history as a JUCE var (for JSON serialization)
   */
  juce::var getHistoryAsVar() const {
    const juce::ScopedLock lock(eventLock_);
    juce::Array<juce::var> arr;
    for (const auto &event : history_) {
      arr.add(event.toVar());
    }
    return juce::var(arr);
  }

  void clearHistory() {
    const juce::ScopedLock lock(eventLock_);
    history_.clear();
    sendChangeMessage();
  }

private:
  AgentEventBroadcaster() = default;

  juce::Array<AgentEvent> history_;
  mutable juce::CriticalSection eventLock_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgentEventBroadcaster)
};

} // namespace ai
} // namespace zenith
