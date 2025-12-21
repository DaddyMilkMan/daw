/*
  ==============================================================================

    AIStatusManager.h
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Centralized status/error tracking for all AI operations.
    Provides user-facing notifications and progress tracking.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <map>
#include <optional>
#include <vector>


namespace zenith {
namespace ai {

//==============================================================================
/**
    Status levels for AI operations
*/
enum class AIOperationStatus {
  Idle,    // Not doing anything
  Working, // In progress
  Success, // Completed successfully
  Warning, // Completed with warnings
  Error    // Failed
};

//==============================================================================
/**
    Represents a single AI operation (request, analysis, etc.)
*/
struct AIOperation {
  juce::String id;          // Unique operation ID
  juce::String agentName;   // e.g. "SampleHunter", "Mastering"
  juce::String description; // e.g. "Searching Freesound..."
  AIOperationStatus status = AIOperationStatus::Idle;
  float progress = 0.0f;     // 0.0 - 1.0
  juce::String errorMessage; // Populated on error/warning
  juce::Time startedAt;
  juce::Time completedAt;

  bool isActive() const { return status == AIOperationStatus::Working; }

  juce::String getStatusString() const {
    switch (status) {
    case AIOperationStatus::Idle:
      return "Idle";
    case AIOperationStatus::Working:
      return "Working";
    case AIOperationStatus::Success:
      return "Success";
    case AIOperationStatus::Warning:
      return "Warning";
    case AIOperationStatus::Error:
      return "Error";
    default:
      return "Unknown";
    }
  }
};

//==============================================================================
/**
    Listener interface for AI status updates
*/
class AIStatusListener {
public:
  virtual ~AIStatusListener() = default;

  virtual void onOperationStarted(const AIOperation &operation) {
    juce::ignoreUnused(operation);
  }
  virtual void onOperationProgress(const AIOperation &operation) {
    juce::ignoreUnused(operation);
  }
  virtual void onOperationCompleted(const AIOperation &operation) {
    juce::ignoreUnused(operation);
  }
  virtual void onOperationError(const AIOperation &operation) {
    juce::ignoreUnused(operation);
  }
};

//==============================================================================
/**
    Singleton manager for tracking all AI operations across agents.

    Usage:
      auto& mgr = AIStatusManager::getInstance();
      auto opId = mgr.beginOperation("SampleHunter", "Searching...");
      mgr.updateProgress(opId, 0.5f, "Downloading samples...");
      mgr.completeOperation(opId, true, "Found 12 samples");
*/
class AIStatusManager : public juce::Timer {
public:
  //============================================================================
  // Singleton Access
  //============================================================================

  static AIStatusManager &getInstance() {
    static AIStatusManager instance;
    return instance;
  }

  //============================================================================
  // Operation Lifecycle
  //============================================================================

  /**
   * Start tracking a new operation.
   * @param agentName Name of the AI agent (e.g., "SampleHunter")
   * @param description Human-readable description
   * @return Unique operation ID for subsequent updates
   */
  juce::String beginOperation(const juce::String &agentName,
                              const juce::String &description);

  /**
   * Update operation progress.
   * @param operationId ID returned from beginOperation
   * @param progress Progress value 0.0 - 1.0
   * @param description Optional updated description
   */
  void updateProgress(const juce::String &operationId, float progress,
                      const juce::String &description = "");

  /**
   * Mark operation as completed.
   * @param operationId ID returned from beginOperation
   * @param success True for success, false for error
   * @param message Optional completion/error message
   */
  void completeOperation(const juce::String &operationId, bool success,
                         const juce::String &message = "");

  /**
   * Mark operation as warning (completed but with issues).
   */
  void warnOperation(const juce::String &operationId,
                     const juce::String &warningMessage);

  //============================================================================
  // Query Operations
  //============================================================================

  /**
   * Get all currently active operations.
   */
  std::vector<AIOperation> getActiveOperations() const;

  /**
   * Get recent operations (last 10).
   */
  std::vector<AIOperation> getRecentOperations() const;

  /**
   * Get operation by ID. Returns a copy of the operation.
   */
  std::optional<AIOperation> getOperation(const juce::String &operationId);

  /**
   * Check if any operation is active.
   */
  bool hasActiveOperations() const;

  /**
   * Get total active operation count.
   */
  int getActiveOperationCount() const;

  //============================================================================
  // Statistics
  //============================================================================

  struct Stats {
    int totalOperations = 0;
    int successfulOperations = 0;
    int failedOperations = 0;
    int warningOperations = 0;
    juce::int64 totalDurationMs = 0;
  };

  Stats getStats() const;
  void resetStats();

  //============================================================================
  // Listeners
  //============================================================================

  void addListener(AIStatusListener *listener);
  void removeListener(AIStatusListener *listener);

  //============================================================================
  // Configuration
  //============================================================================

  /**
   * Set timeout for stale operations (default: 60 seconds).
   * Operations older than this without updates are auto-failed.
   */
  void setStaleTimeoutSeconds(int seconds) { staleTimeoutSeconds_ = seconds; }

private:
  AIStatusManager();
  ~AIStatusManager() override;

  // juce::Timer
  void timerCallback() override;

  // Generate unique operation ID
  juce::String generateOperationId();

  // Clean up stale operations
  void cleanupStaleOperations();

  // Notify listeners (thread-safe)
  void notifyStarted(const AIOperation &op);
  void notifyProgress(const AIOperation &op);
  void notifyCompleted(const AIOperation &op);
  void notifyError(const AIOperation &op);

  mutable juce::CriticalSection lock_;
  std::map<juce::String, AIOperation> operations_;
  std::vector<AIOperation> recentOperations_; // Last 10 completed
  juce::ListenerList<AIStatusListener> listeners_;

  Stats stats_;
  std::atomic<int> operationCounter_{0};
  int staleTimeoutSeconds_ = 60;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIStatusManager)
};

} // namespace ai
} // namespace zenith
