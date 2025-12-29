/*
  ==============================================================================

    UXDirectorAgent.cpp
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    Implementation of the UX Director Agent.

  ==============================================================================
*/

#include "UXDirectorAgent.h"
#include "../ui/arranger/ClipComponent.h"
#include "../ui/mixer/MixerChannelComponent.h"
#include "../ui/framework/SkiaComponent.h"
#include "ZenithStyleApplicator.h"
#include <algorithm>
#include <typeinfo>

namespace zenith {
namespace ai {

//==============================================================================
// Constructor / Destructor
//==============================================================================

UXDirectorAgent::UXDirectorAgent(Engine &engine, ProjectState &projectState,
                                 juce::Component &rootComponent)
    : engine_(engine), projectState_(projectState),
      rootComponent_(rootComponent) {
  // Initial analysis on construction
  rebuildBindingsFromTracks();

  DBG("UXDirectorAgent: Initialized with root component: " +
      rootComponent.getName());
}

UXDirectorAgent::~UXDirectorAgent() { stopMonitoring(); }

//==============================================================================
// Monitoring Control
//==============================================================================

void UXDirectorAgent::startMonitoring(int intervalMs) {
  if (isMonitoring_.load())
    return;

  config_.analysisIntervalMs = intervalMs;
  isMonitoring_.store(true);
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(intervalMs);

  DBG("UXDirectorAgent: Started monitoring at " + juce::String(intervalMs) +
      "ms intervals");
}

void UXDirectorAgent::stopMonitoring() {
  isMonitoring_.store(false);
  stopTimer();

  DBG("UXDirectorAgent: Stopped monitoring");
}

void UXDirectorAgent::runAnalysis() {
  // Debounce rapid calls
  auto now = juce::Time::currentTimeMillis();
  if (now - lastAnalysisTime_ < minAnalysisIntervalMs_)
    return;
  lastAnalysisTime_ = now;

  // Clear previous analysis
  orphanComponents_.clear();
  unstyledComponents_.clear();
  layoutViolations_.clear();
  staleDataComponents_.clear();
  analyzedComponents_.clear();

  // Scan the entire UI tree
  scanComponentTree(&rootComponent_);

  // Run specific analyzers
  if (config_.detectOrphans)
    analyzeOrphanComponents();
  if (config_.detectUnstyled)
    analyzeUnstyledComponents();
  if (config_.detectLayoutIssues)
    analyzeLayoutIssues();
  if (config_.detectStaleData)
    analyzeNameConsistency();

  // Update health score
  updateHealthScore();

  // Notify listeners
  sendChangeMessage();
}

//==============================================================================
// Timer Callback
//==============================================================================

void UXDirectorAgent::timerCallback() {
  // 1. Process pending UI tasks (Time Slicing)
  processPendingTasks();

  // 2. Run analysis periodically
  runAnalysis();

  // 3. Schedule auto-fixes if configured
  if (config_.autoBindOrphans || config_.autoStyleComponents ||
      config_.autoFixLayout || config_.autoUpdateNames) {
    applyAutomaticFixes();
  }
}

//==============================================================================
// Async Task Queue
//==============================================================================

void UXDirectorAgent::scheduleTask(std::function<void()> task) {
  // OVERFLOW PROTECTION: If queue is too large, something is wrong.
  // Either issues are being generated faster than fixed, or fixes are failing.
  // Clear the queue to prevent unbounded memory growth.
  static constexpr size_t maxQueueSize = 100;

  if (uiTaskQueue_.size() >= maxQueueSize) {
    DBG("UXDirectorAgent: Task queue overflow! Clearing " +
        juce::String(uiTaskQueue_.size()) + " pending tasks.");
    uiTaskQueue_.clear();

    // Also clear fixPending flags so issues can be re-scheduled
    juce::ScopedLock sl(issuesLock_);
    for (auto &issue : issues_) {
      issue.fixPending = false;
    }
  }

  uiTaskQueue_.push_back(std::move(task));
}

void UXDirectorAgent::processPendingTasks() {
  if (uiTaskQueue_.empty())
    return;

  // Execute a small batch of tasks to avoid freezing the UI
  int executedCount = 0;

  // Consume tasks from front of queue
  while (!uiTaskQueue_.empty() && executedCount < maxTasksPerFrame_) {
    auto task = std::move(uiTaskQueue_.front());
    uiTaskQueue_.erase(uiTaskQueue_.begin());

    // Execute task - lambda already has SafePointer for component safety
    if (task) {
      task();
    }

    executedCount++;
  }

  // Log queue status if it's building up (early warning)
  if (uiTaskQueue_.size() > 50) {
    DBG("UXDirectorAgent: Warning - task queue has " +
        juce::String(uiTaskQueue_.size()) + " pending items");
  }
}

void UXDirectorAgent::clearPendingTasks() {
  uiTaskQueue_.clear();

  // Clear pending flags so issues can be re-processed
  juce::ScopedLock sl(issuesLock_);
  for (auto &issue : issues_) {
    issue.fixPending = false;
  }
}

//==============================================================================
// ChangeListener (for Track changes)
//==============================================================================

void UXDirectorAgent::changeListenerCallback(juce::ChangeBroadcaster *source) {
  // Check if a track we're bound to has changed
  for (auto &[comp, binding] : bindings_) {
    if (binding.linkedTrack == source) {
      // Track data changed, sync to UI
      syncBindingToUI(binding);
    }
  }

  // Re-analyze on next timer tick
  lastAnalysisTime_ = 0; // Force immediate re-analysis
}

//==============================================================================
// Component Tree Scanning
//==============================================================================

void UXDirectorAgent::scanComponentTree(juce::Component *comp) {
  if (comp == nullptr)
    return;

  // Skip already-analyzed components
  if (analyzedComponents_.count(comp) > 0)
    return;
  analyzedComponents_.insert(comp);

  // Skip trivial components if configured
  if (config_.skipTrivialComponents && isTrivialComponent(comp))
    return;

  // Check for orphan status
  if (isOrphanComponent(comp)) {
    orphanComponents_.push_back(comp);
  }

  // Check for unstyled status
  if (isUnstyledComponent(comp)) {
    unstyledComponents_.push_back(comp);
  }

  // Check for layout issues
  if (hasLayoutIssue(comp)) {
    layoutViolations_.push_back(comp);
  }

  // Check for stale data
  if (hasStaleData(comp)) {
    staleDataComponents_.push_back(comp);
  }

  // Recurse into children
  for (int i = 0; i < comp->getNumChildComponents(); ++i) {
    scanComponentTree(comp->getChildComponent(i));
  }
}

//==============================================================================
// Issue Detection
//==============================================================================

bool UXDirectorAgent::isOrphanComponent(juce::Component *comp) const {
  if (comp == nullptr)
    return false;

  // Skip if already bound
  if (bindings_.count(comp) > 0)
    return false;

  // Check if name and ID are both empty
  bool hasName = comp->getName().isNotEmpty();
  bool hasId = comp->getComponentID().isNotEmpty();

  if (hasName || hasId)
    return false;

  // Only flag specific component types that should have bindings
  // Use RTTI to check type
  if (dynamic_cast<zenith::MixerChannelComponent *>(comp) != nullptr)
    return true;
  if (dynamic_cast<ClipComponent *>(comp) != nullptr)
    return true;

  return false;
}

bool UXDirectorAgent::isUnstyledComponent(juce::Component *comp) const {
  if (comp == nullptr)
    return false;

  // Check if it's a SkiaComponent (already using our system)
  if (dynamic_cast<zenith::SkiaComponent *>(comp) != nullptr)
    return false;

  // Check if component has default JUCE look
  // We consider it "unstyled" if it's a button/slider/etc using default LAF
  if (auto *button = dynamic_cast<juce::Button *>(comp)) {
    // Check if using default LookAndFeel
    auto &laf = button->getLookAndFeel();
    // If it's the default JUCE LAF, it's unstyled
    if (typeid(laf) == typeid(juce::LookAndFeel_V4))
      return true;
  }

  if (auto *slider = dynamic_cast<juce::Slider *>(comp)) {
    auto &laf = slider->getLookAndFeel();
    if (typeid(laf) == typeid(juce::LookAndFeel_V4))
      return true;
  }

  return false;
}

bool UXDirectorAgent::hasLayoutIssue(juce::Component *comp) const {
  if (comp == nullptr)
    return false;

  auto bounds = comp->getBounds();

  // Check for zero-size components (invisible)
  if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
    return true;

  // Check for off-screen components
  auto *parent = comp->getParentComponent();
  if (parent != nullptr) {
    auto parentBounds = parent->getLocalBounds();
    // Check if completely outside parent bounds
    if (!parentBounds.intersects(bounds))
      return true;
  }

  // Check for overlapping with siblings (simplified check)
  if (parent != nullptr) {
    for (int i = 0; i < parent->getNumChildComponents(); ++i) {
      auto *sibling = parent->getChildComponent(i);
      if (sibling == comp)
        continue;

      // Check for significant overlap (more than 50% of area)
      auto intersection = bounds.getIntersection(sibling->getBounds());
      int intersectionArea = intersection.getWidth() * intersection.getHeight();
      int compArea = bounds.getWidth() * bounds.getHeight();

      if (compArea > 0 && intersectionArea > compArea / 2)
        return true;
    }
  }

  return false;
}

bool UXDirectorAgent::hasStaleData(juce::Component *comp) const {
  if (comp == nullptr)
    return false;

  // Check if component is bound to a track
  auto it = bindings_.find(comp);
  if (it == bindings_.end())
    return false;

  const auto &binding = it->second;
  if (binding.linkedTrack == nullptr)
    return false;

  // Compare displayed name with track name
  juce::String displayedName = comp->getName();
  juce::String trackName = binding.linkedTrack->getName();

  if (displayedName != trackName && trackName.isNotEmpty())
    return true;

  return false;
}

bool UXDirectorAgent::isTrivialComponent(juce::Component *comp) const {
  if (comp == nullptr)
    return true;

  // Skip very small components
  auto bounds = comp->getBounds();
  if (bounds.getWidth() < config_.minComponentSize ||
      bounds.getHeight() < config_.minComponentSize)
    return true;

  // Skip generic container types
  juce::String typeName = getTypeName(comp);
  if (typeName.contains("Viewport") || typeName.contains("ScrollBar") ||
      typeName.contains("Resizer"))
    return true;

  return false;
}

juce::String UXDirectorAgent::getTypeName(juce::Component *comp) const {
  if (comp == nullptr)
    return "null";

  // Use RTTI to get readable type name
  const char *mangledName = typeid(*comp).name();
  return juce::String(mangledName);
}

//==============================================================================
// Analysis Methods
//==============================================================================

void UXDirectorAgent::analyzeOrphanComponents() {
  juce::ScopedLock sl(issuesLock_);

  for (auto *comp : orphanComponents_) {
    UIIssue issue;
    issue.type = UIIssueType::OrphanComponent;
    issue.severity = UIIssueSeverity::Functional;
    issue.component = comp;
    issue.componentType = getTypeName(comp);
    issue.description =
        "Component has no name or ID - not connected to data source";
    issue.suggestedFix = "Bind to a track or assign a unique ID";

    addIssue(issue);
  }
}

void UXDirectorAgent::analyzeUnstyledComponents() {
  juce::ScopedLock sl(issuesLock_);

  for (auto *comp : unstyledComponents_) {
    UIIssue issue;
    issue.type = UIIssueType::UnstyledComponent;
    issue.severity = UIIssueSeverity::Cosmetic;
    issue.component = comp;
    issue.componentName = comp->getName();
    issue.componentType = getTypeName(comp);
    issue.description = "Component using default JUCE styling";
    issue.suggestedFix = "Apply ZenithDesignSystem LookAndFeel";

    addIssue(issue);
  }
}

void UXDirectorAgent::analyzeLayoutIssues() {
  juce::ScopedLock sl(issuesLock_);

  for (auto *comp : layoutViolations_) {
    UIIssue issue;
    issue.component = comp;
    issue.componentName = comp->getName();
    issue.componentType = getTypeName(comp);

    auto bounds = comp->getBounds();

    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0) {
      issue.type = UIIssueType::ZeroSizeComponent;
      issue.severity = UIIssueSeverity::Functional;
      issue.description = "Component has zero size (invisible)";
      issue.suggestedFix = "Set valid bounds or remove component";
    } else {
      issue.type = UIIssueType::LayoutOverlap;
      issue.severity = UIIssueSeverity::Usability;
      issue.description = "Component overlapping with siblings";
      issue.suggestedFix = "Adjust layout to prevent overlap";
    }

    addIssue(issue);
  }
}

void UXDirectorAgent::analyzeNameConsistency() {
  juce::ScopedLock sl(issuesLock_);

  for (auto *comp : staleDataComponents_) {
    UIIssue issue;
    issue.type = UIIssueType::StaleData;
    issue.severity = UIIssueSeverity::Usability;
    issue.component = comp;
    issue.componentName = comp->getName();
    issue.componentType = getTypeName(comp);
    issue.description = "Component name doesn't match linked track name";
    issue.suggestedFix = "Sync name from track data";

    addIssue(issue);
  }
}

//==============================================================================
// Issue Management
//==============================================================================

std::vector<UIIssue> UXDirectorAgent::getIssuesByType(UIIssueType type) const {
  std::vector<UIIssue> filtered;
  for (const auto &issue : issues_) {
    if (issue.type == type && !issue.isFixed)
      filtered.push_back(issue);
  }
  return filtered;
}

std::vector<UIIssue>
UXDirectorAgent::getIssuesBySeverity(UIIssueSeverity severity) const {
  std::vector<UIIssue> filtered;
  for (const auto &issue : issues_) {
    if (issue.severity == severity && !issue.isFixed)
      filtered.push_back(issue);
  }
  return filtered;
}

int UXDirectorAgent::getUnresolvedIssueCount() const {
  int count = 0;
  for (const auto &issue : issues_) {
    if (!issue.isFixed)
      ++count;
  }
  return count;
}

juce::String UXDirectorAgent::getIssueSummary() const {
  int orphans = 0, unstyled = 0, layout = 0, stale = 0;

  for (const auto &issue : issues_) {
    if (issue.isFixed)
      continue;

    switch (issue.type) {
    case UIIssueType::OrphanComponent:
    case UIIssueType::MissingBinding:
      ++orphans;
      break;
    case UIIssueType::UnstyledComponent:
      ++unstyled;
      break;
    case UIIssueType::LayoutOverlap:
    case UIIssueType::LayoutOffscreen:
    case UIIssueType::ZeroSizeComponent:
      ++layout;
      break;
    case UIIssueType::StaleData:
    case UIIssueType::InconsistentState:
      ++stale;
      break;
    default:
      break;
    }
  }

  juce::StringArray parts;
  if (orphans > 0)
    parts.add(juce::String(orphans) + " Orphans");
  if (unstyled > 0)
    parts.add(juce::String(unstyled) + " Unstyled");
  if (layout > 0)
    parts.add(juce::String(layout) + " Layout Issues");
  if (stale > 0)
    parts.add(juce::String(stale) + " Stale");

  if (parts.isEmpty())
    return "[OK] UI looks good!";

  return parts.joinIntoString(", ");
}

void UXDirectorAgent::clearResolvedIssues() {
  juce::ScopedLock sl(issuesLock_);

  issues_.erase(std::remove_if(issues_.begin(), issues_.end(),
                               [](const UIIssue &i) { return i.isFixed; }),
                issues_.end());
}

void UXDirectorAgent::clearAllIssues() {
  juce::ScopedLock sl(issuesLock_);
  issues_.clear();
}

void UXDirectorAgent::addIssue(const UIIssue &issue) {
  // Check if we already have this issue for this component
  for (auto &existing : issues_) {
    if (existing.component == issue.component && existing.type == issue.type &&
        !existing.isFixed) {
      // Update existing issue
      existing.detectedAt = juce::Time::getCurrentTime();
      return;
    }
  }

  issues_.push_back(issue);
}

void UXDirectorAgent::resolveIssue(size_t issueIndex,
                                   const juce::String &fixDescription) {
  if (issueIndex < issues_.size()) {
    issues_[issueIndex].isFixed = true;
    issues_[issueIndex].fixApplied = fixDescription;
  }
}

//==============================================================================
// Fix Methods
//==============================================================================

int UXDirectorAgent::applyAllFixes() {
  int fixCount = 0;

  if (config_.autoBindOrphans)
    fixCount += applyFixesForType(UIIssueType::OrphanComponent);

  if (config_.autoStyleComponents)
    fixCount += applyFixesForType(UIIssueType::UnstyledComponent);

  if (config_.autoFixLayout) {
    fixCount += applyFixesForType(UIIssueType::LayoutOverlap);
    fixCount += applyFixesForType(UIIssueType::ZeroSizeComponent);
  }

  if (config_.autoUpdateNames)
    fixCount += applyFixesForType(UIIssueType::StaleData);

  return fixCount;
}

int UXDirectorAgent::applyFixesForType(UIIssueType type) {
  int scheduledCount = 0;
  juce::ScopedLock sl(issuesLock_);

  for (auto &issue : issues_) {
    if (issue.isFixed || issue.fixPending || issue.type != type)
      continue;

    // Mark as pending immediately to avoid double-scheduling
    issue.fixPending = true;

    // Capture state for the task
    // We use a WeakReference or SafePointer equivalent if available,
    // but Component::SafePointer is best.
    // Since we are inside the agent, we can capture 'this'.
    juce::Component::SafePointer<juce::Component> safeComp(issue.component);

    scheduleTask([this, safeComp, type]() {
      // 1. Verify component still exists
      if (safeComp == nullptr)
        return;

      juce::Component *comp = safeComp;
      bool success = false;

      // 2. Perform the fix
      switch (type) {
      case UIIssueType::OrphanComponent:
      case UIIssueType::MissingBinding:
        success = applyOrphanBinding(comp);
        break;

      case UIIssueType::UnstyledComponent:
        success = applyStyleFix(comp);
        break;

      case UIIssueType::LayoutOverlap:
      case UIIssueType::ZeroSizeComponent:
      case UIIssueType::LayoutOffscreen:
        success = applyLayoutFix(comp);
        break;

      case UIIssueType::StaleData:
      case UIIssueType::InconsistentState:
        success = applyNameSync(comp);
        break;

      default:
        break;
      }

      // 3. Update Issue Status (Thread-safe)
      {
        juce::ScopedLock sl(issuesLock_);
        for (auto &issue : issues_) {
          if (issue.component == comp && issue.type == type) {
            if (success) {
              issue.isFixed = true;
              issue.fixApplied = "Auto-fixed by UX Director (Async)";
            }
            // Always clear pending flag so it can be re-evaluated if failed
            issue.fixPending = false;
            break;
          }
        }
      }
    });

    ++scheduledCount;
  }

  return scheduledCount;
}

void UXDirectorAgent::applyAutomaticFixes() {
  // Only apply if there are issues
  if (getUnresolvedIssueCount() == 0)
    return;

  // This now returns scheduled count
  int scheduled = applyAllFixes();

  if (scheduled > 0) {
    DBG("UXDirectorAgent: Scheduled " + juce::String(scheduled) + " fixes");
    // Don't send change message yet, wait for fixes?
    // Actually sending it is fine, listeners see "Pending".
  }
}

bool UXDirectorAgent::applyOrphanBinding(juce::Component *component) {
  if (component == nullptr)
    return false;

  // Find a track to bind this component to
  Track *track = findTrackForComponent(component);
  if (track == nullptr)
    return false;

  return bindComponentToTrack(component, track);
}

bool UXDirectorAgent::applyStyleFix(juce::Component *component) {
  if (component == nullptr)
    return false;

  // Use ZenithStyleApplicator for proper styling
  auto result = ZenithStyleApplicator::applyToComponent(component);

  if (result.success) {
    recordFix(UIIssueType::UnstyledComponent,
              "Style applied: " + result.appliedStyle, component->getName());
    return true;
  }

  // Log failure but don't record as fix (honest failure)
  DBG("UXDirectorAgent: Failed to style component: " + result.reason);
  return false;
}

bool UXDirectorAgent::applyLayoutFix(juce::Component *component) {
  if (component == nullptr)
    return false;

  auto *parent = component->getParentComponent();
  if (parent == nullptr)
    return false;

  auto bounds = component->getBounds();

  // Fix zero-size components
  if (bounds.getWidth() <= 0) {
    bounds.setWidth(100); // Default minimum width
  }
  if (bounds.getHeight() <= 0) {
    bounds.setHeight(30); // Default minimum height
  }

  // Fix off-screen components
  auto parentBounds = parent->getLocalBounds();
  if (!parentBounds.intersects(bounds)) {
    // Move component into visible bounds
    bounds.setPosition(
        juce::jlimit(0, parentBounds.getWidth() - bounds.getWidth(),
                     bounds.getX()),
        juce::jlimit(0, parentBounds.getHeight() - bounds.getHeight(),
                     bounds.getY()));
  }

  // Fix overlaps by nudging components
  for (int i = 0; i < parent->getNumChildComponents(); ++i) {
    auto *sibling = parent->getChildComponent(i);
    if (sibling == component)
      continue;

    auto siblingBounds = sibling->getBounds();
    if (bounds.intersects(siblingBounds)) {
      // Simple fix: move this component below the overlapping sibling
      if (bounds.getY() < siblingBounds.getBottom()) {
        bounds.setY(siblingBounds.getBottom() + 2);
      }
    }
  }

  // Use message thread for UI updates - SafePointer to prevent dangling access
  juce::Component::SafePointer<juce::Component> safeComp(component);
  juce::MessageManager::callAsync([safeComp, bounds]() {
    if (safeComp != nullptr) {
      safeComp->setBounds(bounds);
    }
  });

  recordFix(UIIssueType::LayoutOverlap, "Layout adjusted",
            component->getName());

  return true;
}

bool UXDirectorAgent::applyNameSync(juce::Component *component) {
  if (component == nullptr)
    return false;

  auto it = bindings_.find(component);
  if (it == bindings_.end())
    return false;

  const auto &binding = it->second;
  if (binding.linkedTrack == nullptr)
    return false;

  // Set the component name from track
  juce::String trackName = binding.linkedTrack->getName();
  component->setName(trackName);

  // If it's a MixerChannelComponent, also update its internal state
  if (auto *mixer = dynamic_cast<zenith::MixerChannelComponent *>(component)) {
    mixer->updateFromTrack();
  }

  recordFix(UIIssueType::StaleData, "Name synced: " + trackName,
            component->getName());

  return true;
}

bool UXDirectorAgent::bindComponentToTrack(juce::Component *component,
                                           Track *track) {
  if (component == nullptr || track == nullptr)
    return false;

  // Create the binding
  DataBinding binding(component, track);
  bindings_[component] = binding;

  // Set component name
  component->setName(track->getName());
  component->setComponentID("track_" + track->getTrackId());

  // Add ourselves as a listener to the track for future changes
  track->addChangeListener(this);

  // If it's a MixerChannelComponent, notify it of the binding
  if (auto *mixer = dynamic_cast<zenith::MixerChannelComponent *>(component)) {
    mixer->updateFromTrack();
  }

  recordFix(UIIssueType::OrphanComponent, "Bound to track: " + track->getName(),
            component->getName());

  DBG("UXDirectorAgent: Bound " + getTypeName(component) +
      " to track: " + track->getName());

  return true;
}

bool UXDirectorAgent::applyZenithStyle(juce::Component *component) {
  return applyStyleFix(component);
}

bool UXDirectorAgent::fixComponentLayout(juce::Component *component) {
  return applyLayoutFix(component);
}

void UXDirectorAgent::syncAllNames() {
  for (auto &[comp, binding] : bindings_) {
    if (binding.linkedTrack != nullptr && binding.uiComponent != nullptr) {
      binding.uiComponent->setName(binding.linkedTrack->getName());
      binding.uiComponent->repaint();
    }
  }
}

//==============================================================================
// Binding Helpers
//==============================================================================

Track *UXDirectorAgent::findTrackForComponent(juce::Component *component) {
  if (component == nullptr)
    return nullptr;

  const auto &tracks = engine_.tracks();
  if (tracks.empty())
    return nullptr;

  // 1. Existing Association Check
  // For MixerChannelComponent, try to match by internal pointer
  if (auto *mixer = dynamic_cast<zenith::MixerChannelComponent *>(component)) {
    if (mixer->getTrack() != nullptr)
      return mixer->getTrack();
  }

  // 2. ID-Based Binding (Primary Heuristic)
  // Check if component has an ID like "track_{GUID}"
  juce::String compId = component->getComponentID();
  if (compId.startsWith("track_")) {
    juce::String trackId = compId.substring(6); // Remove "track_"

    // Scan tracks for this ID
    for (const auto &track : tracks) {
      if (track->getTrackId() == trackId) {
        return track.get();
      }
    }
    // Explicit ID detected but not found in engine.
    // Do NOT fall back to heuristics as this leads to incorrect bindings.
    return nullptr;
  }

  // 3. Fallback: Sequential Binding (Risky but necessary for initial setup)
  // Heuristic: assign to next unbound track in order
  for (const auto &track : tracks) {
    bool isBound = false;
    for (const auto &[comp, binding] : bindings_) {
      if (binding.linkedTrack == track.get()) {
        isBound = true;
        break;
      }
    }
    if (!isBound)
      return track.get();
  }

  // 4. Last Resort: Index-based
  if (nextOrphanTrackIndex_ < static_cast<int>(tracks.size())) {
    return tracks[nextOrphanTrackIndex_++].get();
  }

  return nullptr;
}

void UXDirectorAgent::rebuildBindingsFromTracks() {
  bindings_.clear();

  // Scan for existing MixerChannelComponents and bind them
  std::function<void(juce::Component *)> scan = [&](juce::Component *comp) {
    if (auto *mixer = dynamic_cast<zenith::MixerChannelComponent *>(comp)) {
      Track *track = mixer->getTrack();
      if (track != nullptr) {
        DataBinding binding(mixer, track);
        bindings_[mixer] = binding;
        track->addChangeListener(this);
      }
    }

    for (int i = 0; i < comp->getNumChildComponents(); ++i) {
      scan(comp->getChildComponent(i));
    }
  };

  scan(&rootComponent_);

  DBG("UXDirectorAgent: Rebuilt bindings from " +
      juce::String(bindings_.size()) + " existing components");
}

void UXDirectorAgent::syncBindingToUI(const DataBinding &binding) {
  if (binding.uiComponent == nullptr || binding.linkedTrack == nullptr)
    return;

  // Update name
  binding.uiComponent->setName(binding.linkedTrack->getName());

  // Update MixerChannelComponent
  if (auto *mixer =
          dynamic_cast<zenith::MixerChannelComponent *>(binding.uiComponent)) {
    mixer->updateFromTrack();
  }

  binding.uiComponent->repaint();
}

DataBinding *UXDirectorAgent::getBinding(juce::Component *component) {
  auto it = bindings_.find(component);
  if (it != bindings_.end())
    return &it->second;
  return nullptr;
}

int UXDirectorAgent::getActiveBindingCount() const {
  int count = 0;
  for (const auto &[comp, binding] : bindings_) {
    if (binding.isValid)
      ++count;
  }
  return count;
}

//==============================================================================
// Health Score
//==============================================================================

float UXDirectorAgent::getUIHealthScore() const {
  return uiHealthScore_.load();
}

UXDirectorAgent::HealthBreakdown UXDirectorAgent::getHealthBreakdown() const {
  return healthBreakdown_;
}

void UXDirectorAgent::updateHealthScore() {
  int totalComponents = static_cast<int>(analyzedComponents_.size());
  if (totalComponents == 0) {
    healthBreakdown_ = {};
    uiHealthScore_.store(100.0f);
    return;
  }

  // Calculate percentages
  int orphanCount = static_cast<int>(orphanComponents_.size());
  int unstyledCount = static_cast<int>(unstyledComponents_.size());
  int layoutCount = static_cast<int>(layoutViolations_.size());
  int staleCount = static_cast<int>(staleDataComponents_.size());

  healthBreakdown_.dataBindingHealth =
      100.0f * (1.0f - static_cast<float>(orphanCount) / totalComponents);
  healthBreakdown_.styleConsistency =
      100.0f * (1.0f - static_cast<float>(unstyledCount) / totalComponents);
  healthBreakdown_.layoutHealth =
      100.0f * (1.0f - static_cast<float>(layoutCount) / totalComponents);
  healthBreakdown_.dataFreshness =
      100.0f * (1.0f - static_cast<float>(staleCount) / totalComponents);

  // Overall health is weighted average
  float overall = (healthBreakdown_.dataBindingHealth * 0.3f +
                   healthBreakdown_.styleConsistency * 0.2f +
                   healthBreakdown_.layoutHealth * 0.3f +
                   healthBreakdown_.dataFreshness * 0.2f);

  uiHealthScore_.store(overall);
}

//==============================================================================
// Fix Recording
//==============================================================================

void UXDirectorAgent::recordFix(UIIssueType type,
                                const juce::String &description,
                                const juce::String &componentName,
                                std::function<void()> undoFunc) {
  UIFixAction fix;
  fix.issueType = type;
  fix.description = description;
  fix.componentName = componentName;
  fix.successful = true;

  if (undoFunc) {
    fix.canUndo = true;
    fix.undoFunction = undoFunc;
  }

  fixHistory_.push_back(fix);
}

juce::String UXDirectorAgent::getFixSummary() const {
  if (fixHistory_.empty())
    return "No fixes applied yet";

  int bindings = 0, styles = 0, layouts = 0, names = 0;

  for (const auto &fix : fixHistory_) {
    switch (fix.issueType) {
    case UIIssueType::OrphanComponent:
    case UIIssueType::MissingBinding:
      ++bindings;
      break;
    case UIIssueType::UnstyledComponent:
      ++styles;
      break;
    case UIIssueType::LayoutOverlap:
    case UIIssueType::ZeroSizeComponent:
    case UIIssueType::LayoutOffscreen:
      ++layouts;
      break;
    case UIIssueType::StaleData:
    case UIIssueType::InconsistentState:
      ++names;
      break;
    default:
      break;
    }
  }

  juce::StringArray parts;
  if (bindings > 0)
    parts.add(juce::String(bindings) + " bindings");
  if (styles > 0)
    parts.add(juce::String(styles) + " styled");
  if (layouts > 0)
    parts.add(juce::String(layouts) + " layouts");
  if (names > 0)
    parts.add(juce::String(names) + " names synced");

  return juce::String(fixHistory_.size()) +
         " fixes applied: " + parts.joinIntoString(", ");
}

//==============================================================================
// Notifications
//==============================================================================

void UXDirectorAgent::notifyListeners() { sendChangeMessage(); }

} // namespace ai
} // namespace zenith
