/*
  ==============================================================================

    AIEventBus.h
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Pub/Sub event bus for inter-agent communication.
    Allows agents to notify each other of events without tight coupling.

  ==============================================================================
*/

#pragma once

#include <functional>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <map>
#include <vector>


namespace zenith {
namespace ai {

//==============================================================================
/**
    Event types that can be published on the AI event bus
*/
enum class AIEventType {
  // Sample Hunter Events
  SamplesFound,     // New samples discovered
  SampleDownloaded, // A sample was downloaded
  SampleImported,   // A sample was imported to pool

  // Session Debugger Events
  CpuHotspotDetected, // High CPU track detected
  LatencyIssue,       // High latency plugin detected
  GainStagingIssue,   // Clipping or gain issue

  // Preset Geneticist Events
  PresetEvolved,    // New generation completed
  ElitePresetFound, // High-fitness preset discovered

  // UX Director Events
  LayoutFixed,     // Layout issue was fixed
  ComponentStyled, // Component was styled
  BindingCreated,  // UI-Data binding created

  // Project Refactorer Events
  TracksRenamed,       // Tracks were renamed
  SamplesConsolidated, // Samples moved to project folder
  DeadClipsArchived,   // Unused clips archived

  // Mastering Events
  MasteringComplete, // AI mastering finished
  AnalysisComplete,  // Audio analysis finished

  // General
  AgentError,        // An agent encountered an error
  AgentStatusChanged // Agent status changed
};

//==============================================================================
/**
    An event published on the AI event bus
*/
struct AIEvent {
  AIEventType type;
  juce::String sourceAgent; // Name of publishing agent
  juce::var payload;        // Event-specific data
  juce::Time timestamp;

  AIEvent() : timestamp(juce::Time::getCurrentTime()) {}

  AIEvent(AIEventType t, const juce::String &source)
      : type(t), sourceAgent(source), timestamp(juce::Time::getCurrentTime()) {}

  AIEvent(AIEventType t, const juce::String &source, const juce::var &data)
      : type(t), sourceAgent(source), payload(data),
        timestamp(juce::Time::getCurrentTime()) {}

  juce::String getTypeName() const {
    switch (type) {
    case AIEventType::SamplesFound:
      return "SamplesFound";
    case AIEventType::SampleDownloaded:
      return "SampleDownloaded";
    case AIEventType::SampleImported:
      return "SampleImported";
    case AIEventType::CpuHotspotDetected:
      return "CpuHotspotDetected";
    case AIEventType::LatencyIssue:
      return "LatencyIssue";
    case AIEventType::GainStagingIssue:
      return "GainStagingIssue";
    case AIEventType::PresetEvolved:
      return "PresetEvolved";
    case AIEventType::ElitePresetFound:
      return "ElitePresetFound";
    case AIEventType::LayoutFixed:
      return "LayoutFixed";
    case AIEventType::ComponentStyled:
      return "ComponentStyled";
    case AIEventType::BindingCreated:
      return "BindingCreated";
    case AIEventType::TracksRenamed:
      return "TracksRenamed";
    case AIEventType::SamplesConsolidated:
      return "SamplesConsolidated";
    case AIEventType::DeadClipsArchived:
      return "DeadClipsArchived";
    case AIEventType::MasteringComplete:
      return "MasteringComplete";
    case AIEventType::AnalysisComplete:
      return "AnalysisComplete";
    case AIEventType::AgentError:
      return "AgentError";
    case AIEventType::AgentStatusChanged:
      return "AgentStatusChanged";
    default:
      return "Unknown";
    }
  }
};

//==============================================================================
/**
    Callback type for event subscriptions
*/
using AIEventCallback = std::function<void(const AIEvent &)>;

//==============================================================================
/**
    Represents a subscription to the event bus
*/
struct AIEventSubscription {
  int id;
  AIEventType eventType;
  AIEventCallback callback;
  juce::String subscriberName;
};

//==============================================================================
/**
    Singleton event bus for inter-agent communication.

    Usage (Publishing):
      AIEventBus::getInstance().publish(
          AIEventType::SamplesFound,
          "SampleHunter",
          {{"count", 12}, {"query", "bass loop"}}
      );

    Usage (Subscribing):
      int subId = AIEventBus::getInstance().subscribe(
          AIEventType::SamplesFound,
          "BrowserComponent",
          [this](const AIEvent& e) {
            int count = e.payload["count"];
            refreshBrowser();
          }
      );

      // Later: unsubscribe
      AIEventBus::getInstance().unsubscribe(subId);
*/
class AIEventBus : public juce::WeakReference::Target {
public:
  //============================================================================
  // Singleton Access
  //============================================================================

  static AIEventBus &getInstance() {
    static AIEventBus instance;
    return instance;
  }

  //============================================================================
  // Publishing
  //============================================================================

  /**
   * Publish an event to all subscribers.
   * Callbacks are invoked asynchronously on the message thread.
   */
  void publish(const AIEvent &event);

  /**
   * Convenience overload for publishing with payload.
   */
  void publish(AIEventType type, const juce::String &sourceAgent,
               const juce::var &payload = juce::var());

  //============================================================================
  // Subscriptions
  //============================================================================

  /**
   * Subscribe to an event type.
   * @param eventType The event type to subscribe to
   * @param subscriberName Name of the subscriber (for debugging)
   * @param callback Function to call when event is published
   * @return Subscription ID for unsubscribing
   */
  int subscribe(AIEventType eventType, const juce::String &subscriberName,
                AIEventCallback callback);

  /**
   * Unsubscribe from events.
   * @param subscriptionId ID returned from subscribe()
   */
  void unsubscribe(int subscriptionId);

  /**
   * Unsubscribe all subscriptions for a subscriber.
   */
  void unsubscribeAll(const juce::String &subscriberName);

  //============================================================================
  // Query
  //============================================================================

  /**
   * Get count of subscribers for an event type.
   */
  int getSubscriberCount(AIEventType eventType) const;

  /**
   * Get recent events (last 50).
   */
  std::vector<AIEvent> getRecentEvents() const;

  //============================================================================
  // Statistics
  //============================================================================

  struct Stats {
    int totalPublished = 0;
    int totalDelivered = 0;
    int activeSubscriptions = 0;
  };

  Stats getStats() const;
  void resetStats();

private:
  AIEventBus();
  ~AIEventBus() = default;

  mutable juce::CriticalSection lock_;
  std::map<AIEventType, std::vector<AIEventSubscription>> subscriptions_;
  std::vector<AIEvent> recentEvents_;
  int nextSubscriptionId_ = 1;
  mutable Stats stats_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIEventBus)
};

} // namespace ai
} // namespace zenith
