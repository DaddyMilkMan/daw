/*
  ==============================================================================

    UXDirectorAgent.h
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    "The UX Director" - Interface Integrity Agent

    Role: The Frontend Lead. Manages the visual layer and data binding.
    It connects "Dumb" UI components to the "Smart" Audio Engine and forces
    them to look correct and be properly connected.

    Features:
    - Style Enforcement: Ensures all components use ZenithDesignSystem
    - Data Binding: Auto-connects orphan UI elements to engine tracks
    - Layout Fixing: Resolves overlapping/misaligned components
    - Name Injection: Updates component names from engine data
    - Orphan Detection: Finds UI elements with no data source

    This agent treats your UI hierarchy like a DOM tree that needs constant
    supervision. Instead of manually hardcoding every connection, this agent
    runs in the background to enforce consistency, bind missing data, and
    fix layout glitches on the fly.

  ==============================================================================
*/

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include "../engine/Track.h"
#include "SkiaComponent.h"
#include "ZenithDesignSystem.h"
#include <atomic>
#include <functional>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Issue types detected by the UX Director
*/
enum class UIIssueType {
  OrphanComponent,   // Component has no name or ID (disconnected)
  UnstyledComponent, // Component using default JUCE look
  LayoutOverlap,     // Components overlapping unexpectedly
  LayoutOffscreen,   // Component partially or fully off-screen
  ZeroSizeComponent, // Component with 0 width or height (invisible)
  MissingBinding,    // UI element not bound to data source
  StaleData,         // Name/value doesn't match engine data
  InconsistentState, // UI shows different state than engine
  None
};

/**
    Severity levels for UI issues
*/
enum class UIIssueSeverity {
  Cosmetic,  // Just looks wrong, doesn't affect functionality
  Usability, // Makes the UI harder to use
  Functional // Breaks actual functionality
};

//==============================================================================
/**
    Represents a detected UI issue
*/
struct UIIssue {
  UIIssueType type = UIIssueType::None;
  UIIssueSeverity severity = UIIssueSeverity::Cosmetic;

  juce::Component *component = nullptr;
  juce::String componentName;
  juce::String componentId;
  juce::String componentType; // RTTI type name

  juce::String description;
  juce::String suggestedFix;

  juce::Time detectedAt;
  bool isFixed = false;
  bool fixPending = false; // Fix is scheduled in queue
  juce::String fixApplied;

  UIIssue() : detectedAt(juce::Time::getCurrentTime()) {}

  juce::String getStatusString() const {
    switch (type) {
    case UIIssueType::OrphanComponent:
      return "Orphan: " + componentType;
    case UIIssueType::UnstyledComponent:
      return "Unstyled: " + componentType;
    case UIIssueType::LayoutOverlap:
      return "Overlap detected";
    case UIIssueType::LayoutOffscreen:
      return "Offscreen element";
    case UIIssueType::ZeroSizeComponent:
      return "Invisible (0 size)";
    case UIIssueType::MissingBinding:
      return "Missing data binding";
    case UIIssueType::StaleData:
      return "Stale data: " + componentName;
    case UIIssueType::InconsistentState:
      return "State mismatch";
    default:
      return "Unknown issue";
    }
  }
};

//==============================================================================
/**
    Fix action taken by the UX Director
*/
struct UIFixAction {
  UIIssueType issueType;
  juce::String description;
  juce::String componentName;
  juce::Time appliedAt;
  bool successful;

  // Undo information
  bool canUndo = false;
  std::function<void()> undoFunction;

  UIFixAction() : appliedAt(juce::Time::getCurrentTime()), successful(true) {}
};

//==============================================================================
/**
    Analysis configuration for the UX Director
*/
struct UXAnalysisConfig {
  // Analysis mode
  bool lazyMode = true;         // Only analyze when visible components change
  int analysisIntervalMs = 500; // How often to run in non-lazy mode

  // What to detect
  bool detectOrphans = true;      // Find components with no name/ID
  bool detectUnstyled = true;     // Find components not using ZenithLookAndFeel
  bool detectLayoutIssues = true; // Find overlapping/offscreen components
  bool detectStaleData = true;    // Find name/value mismatches

  // What to auto-fix
  bool autoBindOrphans = true;     // Auto-bind orphan mixers to tracks
  bool autoStyleComponents = true; // Auto-apply ZenithDesignSystem styles
  bool autoFixLayout = true;       // Auto-fix overlapping components
  bool autoUpdateNames = true;     // Auto-update stale names

  // Component filtering
  bool skipTrivialComponents = true; // Skip basic containers, viewports, etc.
  int minComponentSize = 10;         // Skip components smaller than this
};

//==============================================================================
/**
    Binding map entry - links UI component to data source
*/
struct DataBinding {
  juce::Component *uiComponent = nullptr;
  Track *linkedTrack = nullptr;
  juce::String trackId;
  juce::String paramId; // For automation bindings
  bool isValid = false;

  DataBinding() = default;
  DataBinding(juce::Component *ui, Track *track)
      : uiComponent(ui), linkedTrack(track),
        trackId(track ? track->getTrackId() : ""), isValid(track != nullptr) {}
};

//==============================================================================
/**
    UX Director Agent

    Monitors and maintains UI integrity. Runs analysis to find issues and
    can automatically apply fixes to keep the UI consistent.

    Usage:
    1. Construct with root component and engine reference
    2. Call startMonitoring() to begin automatic analysis
    3. Access issues via getIssues()
    4. Use applyAllFixes() or individual fix methods
*/
class UXDirectorAgent : public juce::Timer,
                        public juce::ChangeBroadcaster,
                        public juce::ChangeListener {
public:
  //==========================================================================
  explicit UXDirectorAgent(Engine &engine, ProjectState &projectState,
                           juce::Component &rootComponent);
  ~UXDirectorAgent() override;

  //==========================================================================
  // Monitoring Control
  //==========================================================================

  /**
   * @brief Start monitoring the UI tree
   * @param intervalMs Analysis interval in milliseconds
   */
  void startMonitoring(int intervalMs = 500);

  /**
   * @brief Stop monitoring
   */
  void stopMonitoring();

  /**
   * @brief Check if monitoring is active
   */
  bool isMonitoring() const { return isMonitoring_.load(); }

  /**
   * @brief Run a single analysis pass (can be called manually)
   */
  void runAnalysis();

  //==========================================================================
  // Issue Management
  //==========================================================================

  /**
   * @brief Get all detected issues
   */
  const std::vector<UIIssue> &getIssues() const { return issues_; }

  /**
   * @brief Get issues filtered by type
   */
  std::vector<UIIssue> getIssuesByType(UIIssueType type) const;

  /**
   * @brief Get issues filtered by severity
   */
  std::vector<UIIssue> getIssuesBySeverity(UIIssueSeverity severity) const;

  /**
   * @brief Get unresolved issue count
   */
  int getUnresolvedIssueCount() const;

  /**
   * @brief Get issue summary for UI display
   * @return Human-readable summary like "3 Orphans, 2 Unstyled"
   */
  juce::String getIssueSummary() const;

  /**
   * @brief Clear all resolved issues from the list
   */
  void clearResolvedIssues();

  /**
   * @brief Clear all issues
   */
  void clearAllIssues();

  //==========================================================================
  // Fix Actions
  //==========================================================================

  /**
   * @brief Apply all queued fixes automatically
   * @return Number of fixes applied
   */
  int applyAllFixes();

  /**
   * @brief Apply fixes for a specific issue type
   * @return Number of fixes applied
   */
  int applyFixesForType(UIIssueType type);

  /**
   * @brief Get history of fix actions taken
   */
  const std::vector<UIFixAction> &getFixHistory() const { return fixHistory_; }

  /**
   * @brief Get count of fixes applied
   */
  int getFixCount() const { return static_cast<int>(fixHistory_.size()); }

  /**
   * @brief Get summary of fixes applied
   * @return Human-readable summary
   */
  juce::String getFixSummary() const;

  //==========================================================================
  // Manual Fix Triggers
  //==========================================================================

  /**
   * @brief Bind a specific component to a track
   * @param component The UI component to bind
   * @param track The track to bind it to
   */
  bool bindComponentToTrack(juce::Component *component, Track *track);

  /**
   * @brief Force style update on a component
   * @param component The component to style
   */
  bool applyZenithStyle(juce::Component *component);

  /**
   * @brief Force layout recalculation for a component
   * @param component The component to fix
   */
  bool fixComponentLayout(juce::Component *component);

  /**
   * @brief Sync all component names with engine data
   */
  void syncAllNames();

  //==========================================================================
  // Data Binding
  //==========================================================================

  /**
   * @brief Get binding for a component
   */
  DataBinding *getBinding(juce::Component *component);

  /**
   * @brief Get all current bindings
   */
  const std::unordered_map<juce::Component *, DataBinding> &
  getBindings() const {
    return bindings_;
  }

  /**
   * @brief Get number of active bindings
   */
  int getActiveBindingCount() const;

  //==========================================================================
  // Configuration
  //==========================================================================

  /**
   * @brief Get current configuration
   */
  UXAnalysisConfig &getConfig() { return config_; }
  const UXAnalysisConfig &getConfig() const { return config_; }

  /**
   * @brief Set configuration
   */
  void setConfig(const UXAnalysisConfig &config) { config_ = config; }

  //==========================================================================
  // UI Health Score
  //==========================================================================

  /**
   * @brief Get overall UI health score (0-100)
   * 100 = Perfect UI, 0 = Everything is broken
   */
  float getUIHealthScore() const;

  /**
   * @brief Get detailed health breakdown
   */
  struct HealthBreakdown {
    float styleConsistency = 100.0f;  // % using ZenithDesignSystem
    float dataBindingHealth = 100.0f; // % of components properly bound
    float layoutHealth = 100.0f;      // % without layout issues
    float dataFreshness = 100.0f;     // % showing current data
  };
  HealthBreakdown getHealthBreakdown() const;

  //==========================================================================
  // ChangeListener (for Track changes)
  //==========================================================================
  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

private:
  //==========================================================================
  // Timer callback
  void timerCallback() override;

  //==========================================================================
  // Analysis Methods
  //==========================================================================

  void scanComponentTree(juce::Component *comp);
  void analyzeOrphanComponents();
  void analyzeUnstyledComponents();
  void analyzeLayoutIssues();
  void analyzeDataBindings();
  void analyzeNameConsistency();

  bool isOrphanComponent(juce::Component *comp) const;
  bool isUnstyledComponent(juce::Component *comp) const;
  bool hasLayoutIssue(juce::Component *comp) const;
  bool hasStaleData(juce::Component *comp) const;
  bool isTrivialComponent(juce::Component *comp) const;

  juce::String getTypeName(juce::Component *comp) const;

  //==========================================================================
  // Fix Methods
  //==========================================================================

  void applyAutomaticFixes();

  bool applyOrphanBinding(juce::Component *component);
  bool applyStyleFix(juce::Component *component);
  bool applyLayoutFix(juce::Component *component);
  bool applyNameSync(juce::Component *component);

  void recordFix(UIIssueType type, const juce::String &description,
                 const juce::String &componentName,
                 std::function<void()> undoFunc = nullptr);

  //==========================================================================
  // Binding Helpers
  //==========================================================================

  Track *findTrackForComponent(juce::Component *component);
  void rebuildBindingsFromTracks();
  void syncBindingToUI(const DataBinding &binding);

  //==========================================================================
  // Helpers
  //==========================================================================

  void addIssue(const UIIssue &issue);
  void resolveIssue(size_t issueIndex, const juce::String &fixDescription);
  void notifyListeners();
  void updateHealthScore();

  //==========================================================================
  // Async Task Queue (Time Slicing)
  //==========================================================================

  /**
   * @brief Schedule a UI task to be executed on the message thread
   * Avoids freezing the UI by spreading heavy tasks across multiple frames.
   */
  void scheduleTask(std::function<void()> task);

  /**
   * @brief Process a batch of pending tasks
   * Called by timerCallback.
   */
  void processPendingTasks();

  /**
   * @brief Clear pending tasks
   */
  void clearPendingTasks();

  //==========================================================================
  // Member Variables
  //==========================================================================

  Engine &engine_;
  ProjectState &projectState_;
  juce::Component &rootComponent_;

  UXAnalysisConfig config_;

  // Monitoring state
  std::atomic<bool> isMonitoring_{false};

  // Analysis results
  std::vector<juce::Component *> orphanComponents_;
  std::vector<juce::Component *> unstyledComponents_;
  std::vector<juce::Component *> layoutViolations_;
  std::vector<juce::Component *> staleDataComponents_;

  // Tracked components (to avoid duplicates)
  std::unordered_set<juce::Component *> analyzedComponents_;

  // Data bindings
  std::unordered_map<juce::Component *, DataBinding> bindings_;

  // Issues and fixes
  std::vector<UIIssue> issues_;
  std::vector<UIFixAction> fixHistory_;
  juce::CriticalSection issuesLock_;

  // Task Queue
  std::vector<std::function<void()>> uiTaskQueue_;
  const int maxTasksPerFrame_ = 3; // Limit tasks/frame to prevent stutter

  // Health metrics
  std::atomic<float> uiHealthScore_{100.0f};
  HealthBreakdown healthBreakdown_;

  // Track indexing for orphan binding heuristics
  int nextOrphanTrackIndex_ = 0;

  // Debounce for change notifications
  juce::int64 lastAnalysisTime_ = 0;
  static constexpr int minAnalysisIntervalMs_ = 100;

  //==========================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UXDirectorAgent)
};

} // namespace ai
} // namespace zenith
