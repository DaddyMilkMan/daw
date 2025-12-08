/*
  ==============================================================================

    SessionDebuggerAgent.cpp
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    Implementation of the Session Debugger Agent.

  ==============================================================================
*/

#include "SessionDebuggerAgent.h"
#include "../engine/PluginHost.h"

namespace zenith {
namespace ai {

//==============================================================================
// Constructor / Destructor
//==============================================================================

SessionDebuggerAgent::SessionDebuggerAgent(Engine &engine) : engine_(engine) {
  DBG("SessionDebuggerAgent: Initialized");
}

SessionDebuggerAgent::~SessionDebuggerAgent() {
  stopMonitoring();
  DBG("SessionDebuggerAgent: Destroyed");
}

//==============================================================================
// Monitoring Control
//==============================================================================

void SessionDebuggerAgent::startMonitoring(int intervalMs) {
  if (isMonitoring_.load())
    return;

  config_.analysisIntervalMs = intervalMs;
  isMonitoring_.store(true);

  // Initialize track CPU metrics
  trackCpuMetrics_.clear();
  auto &tracks = engine_.tracks();
  for (size_t i = 0; i < tracks.size(); ++i) {
    TrackCpuMetrics metrics;
    metrics.trackIndex = static_cast<int>(i);
    if (tracks[i]) {
      metrics.trackName = tracks[i]->getName();
      metrics.numPlugins = tracks[i]->getNumPlugins();
    }
    trackCpuMetrics_.push_back(metrics);
  }

  startTimer(intervalMs);
  DBG("SessionDebuggerAgent: Started monitoring (interval: " +
      juce::String(intervalMs) + "ms)");
}

void SessionDebuggerAgent::stopMonitoring() {
  if (!isMonitoring_.load())
    return;

  stopTimer();
  isMonitoring_.store(false);
  DBG("SessionDebuggerAgent: Stopped monitoring");
}

void SessionDebuggerAgent::runAnalysis() {
  // Run all analysis passes
  analyzeCpuUsage();
  analyzeGainStaging();
  analyzeLatency();
  analyzeRoutingHealth();

  // Update overall health score
  updateHealthScore();

  // Apply automatic fixes if enabled
  applyAutomaticFixes();

  // Notify listeners of any changes
  sendChangeMessage();
}

void SessionDebuggerAgent::timerCallback() { runAnalysis(); }

//==============================================================================
// Issue Management
//==============================================================================

std::vector<SessionIssue>
SessionDebuggerAgent::getIssuesByType(IssueType type) const {
  std::vector<SessionIssue> filtered;
  juce::ScopedLock lock(issuesLock_);
  for (const auto &issue : issues_) {
    if (issue.type == type) {
      filtered.push_back(issue);
    }
  }
  return filtered;
}

std::vector<SessionIssue>
SessionDebuggerAgent::getIssuesBySeverity(IssueSeverity severity) const {
  std::vector<SessionIssue> filtered;
  juce::ScopedLock lock(issuesLock_);
  for (const auto &issue : issues_) {
    if (issue.severity == severity) {
      filtered.push_back(issue);
    }
  }
  return filtered;
}

int SessionDebuggerAgent::getUnresolvedIssueCount() const {
  juce::ScopedLock lock(issuesLock_);
  int count = 0;
  for (const auto &issue : issues_) {
    if (!issue.isFixed) {
      ++count;
    }
  }
  return count;
}

void SessionDebuggerAgent::clearResolvedIssues() {
  juce::ScopedLock lock(issuesLock_);
  issues_.erase(std::remove_if(issues_.begin(), issues_.end(),
                               [](const SessionIssue &i) { return i.isFixed; }),
                issues_.end());
}

void SessionDebuggerAgent::clearAllIssues() {
  juce::ScopedLock lock(issuesLock_);
  issues_.clear();
}

//==============================================================================
// Fix Summary
//==============================================================================

juce::String SessionDebuggerAgent::getFixSummary() const {
  if (fixHistory_.empty()) {
    return "No issues fixed";
  }

  int cpuFixes = 0;
  int clippingFixes = 0;
  int latencyFixes = 0;
  int otherFixes = 0;

  for (const auto &fix : fixHistory_) {
    switch (fix.issueType) {
    case IssueType::CPUSpike:
      ++cpuFixes;
      break;
    case IssueType::InputClipping:
    case IssueType::OutputClipping:
      ++clippingFixes;
      break;
    case IssueType::HighLatency:
      ++latencyFixes;
      break;
    default:
      ++otherFixes;
      break;
    }
  }

  juce::StringArray parts;

  if (cpuFixes > 0) {
    parts.add("CPU Optimized");
  }
  if (clippingFixes > 0) {
    parts.add("Clipping Resolved");
  }
  if (latencyFixes > 0) {
    parts.add("Latency Fixed");
  }
  if (otherFixes > 0) {
    parts.add(juce::String(otherFixes) + " Other Fixes");
  }

  return juce::String(fixHistory_.size()) +
         " Issues Fixed: " + parts.joinIntoString(", ");
}

bool SessionDebuggerAgent::undoLastFix() {
  if (fixHistory_.empty()) {
    return false;
  }

  auto &lastFix = fixHistory_.back();
  if (!lastFix.canUndo || !lastFix.undoFunction) {
    return false;
  }

  // Execute undo
  lastFix.undoFunction();

  DBG("SessionDebuggerAgent: Undid fix: " + lastFix.description);

  fixHistory_.pop_back();
  return true;
}

//==============================================================================
// Manual Fix Triggers
//==============================================================================

bool SessionDebuggerAgent::optimizeTrackCpu(int trackIndex, bool freeze) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  if (freeze) {
    return freezeTrack(trackIndex);
  }

  // Alternative: bypass the heaviest plugin
  auto track = tracks[trackIndex];
  if (!track)
    return false;

  // Find the heaviest plugin (by latency as a proxy for complexity)
  int heaviestPlugin = -1;
  int maxLatency = 0;

  for (int i = 0; i < track->getNumPlugins(); ++i) {
    auto *plugin = track->getPlugin(i);
    if (plugin) {
      int latency = plugin->getLatencySamples();
      if (latency > maxLatency) {
        maxLatency = latency;
        heaviestPlugin = i;
      }
    }
  }

  if (heaviestPlugin >= 0) {
    return bypassPlugin(trackIndex, heaviestPlugin);
  }

  return false;
}

bool SessionDebuggerAgent::fixTrackGainStaging(int trackIndex,
                                               float targetHeadroom) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  auto track = tracks[trackIndex];
  if (!track)
    return false;

  // Measure current peak level
  float peakDb = juce::Decibels::gainToDecibels(track->getPeakLevel());

  // Calculate needed gain adjustment
  float adjustment = targetHeadroom - peakDb;

  // Only insert gain trim if adjustment is significant
  if (std::abs(adjustment) < 0.5f) {
    return false; // No significant adjustment needed
  }

  return insertGainTrim(trackIndex, adjustment);
}

bool SessionDebuggerAgent::setTrackLowLatencyMode(int trackIndex, bool enable) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  auto track = tracks[trackIndex];
  if (!track)
    return false;

  double sampleRate = engine_.getSampleRate();
  float latencyThresholdSamples =
      (config_.latencyCriticalMs / 1000.0f) * static_cast<float>(sampleRate);

  bool anyBypassed = false;

  for (int i = 0; i < track->getNumPlugins(); ++i) {
    auto *plugin = track->getPlugin(i);
    if (plugin) {
      int latency = plugin->getLatencySamples();
      if (latency > latencyThresholdSamples) {
        if (enable) {
          bypassPlugin(trackIndex, i);
        } else {
          enablePlugin(trackIndex, i);
        }
        anyBypassed = true;
      }
    }
  }

  return anyBypassed;
}

bool SessionDebuggerAgent::bypassHighLatencyPlugin(int trackIndex,
                                                   int pluginIndex) {
  return bypassPlugin(trackIndex, pluginIndex);
}

//==============================================================================
// Session Statistics
//==============================================================================

float SessionDebuggerAgent::getSessionHealthScore() const {
  return sessionHealthScore_.load();
}

//==============================================================================
// Listener Management
//==============================================================================

void SessionDebuggerAgent::addListener(Listener *listener) {
  listeners_.add(listener);
}

void SessionDebuggerAgent::removeListener(Listener *listener) {
  listeners_.remove(listener);
}

//==============================================================================
// Analysis Methods
//==============================================================================

void SessionDebuggerAgent::analyzeCpuUsage() {
  double totalCpu = engine_.getCpuUsage();
  totalCpuUsage_.store(static_cast<float>(totalCpu));

  // Check overall CPU
  if (totalCpu > config_.cpuCriticalThreshold) {
    SessionIssue issue;
    issue.type = IssueType::CPUSpike;
    issue.severity = IssueSeverity::Critical;
    issue.description =
        "Total CPU usage is critically high: " + juce::String(totalCpu, 1) +
        "%";
    issue.suggestedFix = "Freeze tracks or reduce plugin count";
    issue.value = static_cast<float>(totalCpu);
    issue.threshold = config_.cpuCriticalThreshold;
    addIssue(issue);
  } else if (totalCpu > config_.cpuWarningThreshold) {
    SessionIssue issue;
    issue.type = IssueType::CPUSpike;
    issue.severity = IssueSeverity::Warning;
    issue.description =
        "Total CPU usage is high: " + juce::String(totalCpu, 1) + "%";
    issue.suggestedFix = "Consider freezing tracks with heavy plugins";
    issue.value = static_cast<float>(totalCpu);
    issue.threshold = config_.cpuWarningThreshold;
    addIssue(issue);
  }

  // Per-track CPU analysis (estimated based on plugin count and latency)
  auto &tracks = engine_.tracks();

  // Update track metrics with estimated CPU usage
  for (size_t i = 0; i < tracks.size() && i < trackCpuMetrics_.size(); ++i) {
    auto track = tracks[i];
    if (!track)
      continue;

    auto &metrics = trackCpuMetrics_[i];
    metrics.numPlugins = track->getNumPlugins();

    // Estimate CPU per plugin (rough heuristic)
    float estimatedCpu = 0.0f;
    for (int p = 0; p < track->getNumPlugins(); ++p) {
      auto *plugin = track->getPlugin(p);
      if (plugin) {
        // More complex plugins typically have higher latency
        int latency = plugin->getLatencySamples();
        float pluginCpu =
            2.0f + (latency / 1000.0f); // Base 2% + latency factor
        estimatedCpu += pluginCpu;
      }
    }

    metrics.averageCpuPercent = estimatedCpu;
    metrics.peakCpuPercent = std::max(metrics.peakCpuPercent, estimatedCpu);

    // Check for individual track CPU spikes
    if (estimatedCpu > config_.trackCpuWarningThreshold) {
      SessionIssue issue;
      issue.type = IssueType::CPUSpike;
      issue.severity = (estimatedCpu > config_.cpuCriticalThreshold)
                           ? IssueSeverity::Critical
                           : IssueSeverity::Warning;
      issue.trackIndex = static_cast<int>(i);
      issue.trackName = track->getName();
      issue.description = "Track \"" + track->getName() + "\" is consuming " +
                          juce::String(estimatedCpu, 0) + "% CPU";
      issue.suggestedFix = "Freeze Track " +
                           juce::String(static_cast<int>(i) + 1) +
                           " to save CPU";
      issue.value = estimatedCpu;
      issue.threshold = config_.trackCpuWarningThreshold;
      addIssue(issue);
    }
  }
}

void SessionDebuggerAgent::analyzeGainStaging() {
  auto &tracks = engine_.tracks();
  int clippingCount = 0;

  for (size_t i = 0; i < tracks.size(); ++i) {
    auto track = tracks[i];
    if (!track)
      continue;

    // Get peak level
    float peakLevel = track->getPeakLevel();
    float peakDb = juce::Decibels::gainToDecibels(peakLevel);

    // Check for clipping (output level)
    if (peakDb > config_.outputClipThreshold) {
      ++clippingCount;

      SessionIssue issue;
      issue.type = IssueType::OutputClipping;
      issue.severity =
          (peakDb > 3.0f) ? IssueSeverity::Critical : IssueSeverity::Warning;
      issue.trackIndex = static_cast<int>(i);
      issue.trackName = track->getName();

      // Estimate which plugin might be causing the issue
      // If track has a compressor, the input might be clipping
      bool hasCompressor = false;
      for (int p = 0; p < track->getNumPlugins(); ++p) {
        auto *plugin = track->getPlugin(p);
        if (plugin && plugin->getName().containsIgnoreCase("compressor")) {
          hasCompressor = true;
          issue.pluginName = plugin->getName();
          issue.pluginIndex = p;
          issue.type = IssueType::InputClipping;
          issue.description = "The input to \"" + plugin->getName() +
                              "\" on Track \"" + track->getName() +
                              "\" is clipping (+" + juce::String(peakDb, 1) +
                              "dB)";
          break;
        }
      }

      if (!hasCompressor) {
        issue.description = "Track \"" + track->getName() +
                            "\" output is clipping (+" +
                            juce::String(peakDb, 1) + "dB)";
      }

      float neededReduction = peakDb - config_.headroomTarget;
      issue.suggestedFix = "Lower Trim on Track \"" + track->getName() +
                           "\" by " + juce::String(neededReduction, 1) + "dB";
      issue.value = peakDb;
      issue.threshold = config_.outputClipThreshold;
      addIssue(issue);
    }

    // Check for silence (possible issue)
    if (track->getCurrentLevel() <
        juce::Decibels::decibelsToGain(config_.silenceThreshold)) {
      // Only flag if track should be producing audio
      if (!track->isMuted() && !track->isSilencedBySolo() &&
          track->getNumClips() > 0) {
        SessionIssue issue;
        issue.type = IssueType::SilentTrack;
        issue.severity = IssueSeverity::Info;
        issue.trackIndex = static_cast<int>(i);
        issue.trackName = track->getName();
        issue.description =
            "Track \"" + track->getName() + "\" is silent but has clips";
        issue.suggestedFix = "Check if track is routed correctly";
        addIssue(issue);
      }
    }
  }

  clippingTrackCount_.store(clippingCount);
}

void SessionDebuggerAgent::analyzeLatency() {
  auto &tracks = engine_.tracks();
  double sampleRate = engine_.getSampleRate();

  float maxLatencyMs = 0.0f;

  for (size_t i = 0; i < tracks.size(); ++i) {
    auto track = tracks[i];
    if (!track)
      continue;

    int trackLatency = engine_.getTrackLatency(static_cast<int>(i));
    float trackLatencyMs = samplesToMs(trackLatency);
    maxLatencyMs = std::max(maxLatencyMs, trackLatencyMs);

    // Check individual plugins for high latency
    for (int p = 0; p < track->getNumPlugins(); ++p) {
      auto *plugin = track->getPlugin(p);
      if (!plugin)
        continue;

      int pluginLatency = plugin->getLatencySamples();
      float pluginLatencyMs = samplesToMs(pluginLatency);

      if (pluginLatencyMs > config_.latencyCriticalMs) {
        SessionIssue issue;
        issue.type = IssueType::HighLatency;
        issue.severity = IssueSeverity::Critical;
        issue.trackIndex = static_cast<int>(i);
        issue.trackName = track->getName();
        issue.pluginName = plugin->getName();
        issue.pluginIndex = p;
        issue.description = "The \"" + plugin->getName() + "\" on \"" +
                            track->getName() + "\" is introducing " +
                            juce::String(pluginLatencyMs, 0) + "ms of latency";
        issue.suggestedFix =
            "Enable 'Low Latency Mode' for track \"" + track->getName() + "\"";
        issue.value = pluginLatencyMs;
        issue.threshold = config_.latencyCriticalMs;
        addIssue(issue);
      } else if (pluginLatencyMs > config_.latencyWarningMs) {
        SessionIssue issue;
        issue.type = IssueType::HighLatency;
        issue.severity = IssueSeverity::Warning;
        issue.trackIndex = static_cast<int>(i);
        issue.trackName = track->getName();
        issue.pluginName = plugin->getName();
        issue.pluginIndex = p;
        issue.description = "\"" + plugin->getName() + "\" on \"" +
                            track->getName() + "\" has " +
                            juce::String(pluginLatencyMs, 0) + "ms latency";
        issue.suggestedFix = "Consider bypassing during recording";
        issue.value = pluginLatencyMs;
        issue.threshold = config_.latencyWarningMs;
        addIssue(issue);
      }
    }
  }

  totalLatencyMs_.store(maxLatencyMs);
}

void SessionDebuggerAgent::analyzeRoutingHealth() {
  // Check for feedback loops in the routing graph
  auto &routingGraph = engine_.getRoutingGraph();

  // Get processing order - if this fails, there might be a cycle
  auto processingOrder = routingGraph.getProcessingOrder();

  if (processingOrder.empty() && engine_.getNumTracks() > 0) {
    SessionIssue issue;
    issue.type = IssueType::FeedbackLoop;
    issue.severity = IssueSeverity::Critical;
    issue.description = "Possible feedback loop detected in routing graph";
    issue.suggestedFix = "Check track routing for circular connections";
    addIssue(issue);
  }
}

//==============================================================================
// Automatic Fix Application
//==============================================================================

void SessionDebuggerAgent::applyAutomaticFixes() {
  juce::ScopedLock lock(issuesLock_);

  for (auto &issue : issues_) {
    if (issue.isFixed)
      continue;

    switch (issue.type) {
    case IssueType::CPUSpike:
      if (config_.autoFixCpuSpikes &&
          issue.severity == IssueSeverity::Critical && issue.trackIndex >= 0) {
        if (freezeTrack(issue.trackIndex)) {
          issue.isFixed = true;
          issue.fixApplied = "Froze track to save CPU";

          FixAction fix;
          fix.issueType = IssueType::CPUSpike;
          fix.description = "Froze track \"" + issue.trackName + "\"";
          fix.trackName = issue.trackName;
          fix.successful = true;

          int trackIdx = issue.trackIndex;
          fix.undoFunction = [this, trackIdx]() { unfreezeTrack(trackIdx); };
          fixHistory_.push_back(fix);

          notifyListeners(issue, true, &fix);
        }
      }
      break;

    case IssueType::InputClipping:
    case IssueType::OutputClipping:
      if (config_.autoFixClipping && issue.severity >= IssueSeverity::Warning &&
          issue.trackIndex >= 0) {

        float neededReduction = -(issue.value - config_.headroomTarget);

        if (insertGainTrim(issue.trackIndex, neededReduction)) {
          issue.isFixed = true;
          issue.fixApplied =
              "Inserted gain trim (" + juce::String(neededReduction, 1) + "dB)";

          FixAction fix;
          fix.issueType = issue.type;
          fix.description = "Inserted " + juce::String(neededReduction, 1) +
                            "dB trim on \"" + issue.trackName + "\"";
          fix.trackName = issue.trackName;
          fix.successful = true;

          int trackIdx = issue.trackIndex;
          fix.undoFunction = [this, trackIdx]() { removeGainTrim(trackIdx); };
          fixHistory_.push_back(fix);

          notifyListeners(issue, true, &fix);
        }
      }
      break;

    case IssueType::HighLatency:
      if (config_.autoFixLatency && engine_.isRecording() &&
          issue.severity == IssueSeverity::Critical && issue.trackIndex >= 0 &&
          issue.pluginIndex >= 0) {

        if (bypassPlugin(issue.trackIndex, issue.pluginIndex)) {
          issue.isFixed = true;
          issue.fixApplied = "Bypassed high-latency plugin during recording";

          FixAction fix;
          fix.issueType = IssueType::HighLatency;
          fix.description = "Bypassed \"" + issue.pluginName + "\" on \"" +
                            issue.trackName + "\"";
          fix.trackName = issue.trackName;
          fix.successful = true;

          int trackIdx = issue.trackIndex;
          int pluginIdx = issue.pluginIndex;
          fix.undoFunction = [this, trackIdx, pluginIdx]() {
            enablePlugin(trackIdx, pluginIdx);
          };
          fixHistory_.push_back(fix);

          notifyListeners(issue, true, &fix);
        }
      }
      break;

    default:
      break;
    }
  }
}

//==============================================================================
// Fix Implementation Methods
//==============================================================================

bool SessionDebuggerAgent::freezeTrack(int trackIndex) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  auto track = tracks[trackIndex];
  if (!track)
    return false;

  // Store original state for undo
  // Note: Track doesn't have setFrozen yet, so we simulate by muting
  // In a full implementation, this would call track->setFrozen(true)
  // which would render the track to a temporary file and bypass all plugins

  // For now, we'll disable the track's plugins
  bool anyBypassed = false;
  for (int i = 0; i < track->getNumPlugins(); ++i) {
    auto *plugin = track->getPlugin(i);
    if (plugin && !plugin->isSuspended()) {
      plugin->suspendProcessing(true);
      anyBypassed = true;
    }
  }

  if (anyBypassed) {
    originalFrozenState_[trackIndex] = false; // Was not frozen before
    DBG("SessionDebuggerAgent: Froze track " + track->getName());
    return true;
  }

  return false;
}

bool SessionDebuggerAgent::unfreezeTrack(int trackIndex) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  auto track = tracks[trackIndex];
  if (!track)
    return false;

  // Re-enable all plugins
  for (int i = 0; i < track->getNumPlugins(); ++i) {
    auto *plugin = track->getPlugin(i);
    if (plugin && plugin->isSuspended()) {
      plugin->suspendProcessing(false);
    }
  }

  originalFrozenState_.erase(trackIndex);
  DBG("SessionDebuggerAgent: Unfroze track " + track->getName());
  return true;
}

bool SessionDebuggerAgent::insertGainTrim(int trackIndex, float gainDb) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  auto track = tracks[trackIndex];
  if (!track)
    return false;

  // Check if we already have a gain trim inserted
  auto it = insertedGainPlugins_.find(trackIndex);
  if (it != insertedGainPlugins_.end() && it->second != nullptr) {
    // Update existing gain trim
    it->second->setGainDb(it->second->getGainDb() + gainDb);
    DBG("SessionDebuggerAgent: Updated gain trim on track " + track->getName() +
        " to " + juce::String(it->second->getGainDb(), 1) + "dB");
    return true;
  }

  // For now, simulate by adjusting track volume
  // In a full implementation, we would insert a GainPlugin at the start of the
  // chain
  float currentVolume = track->getVolume();
  float gainLinear = juce::Decibels::decibelsToGain(gainDb);
  track->setVolume(currentVolume * gainLinear);

  DBG("SessionDebuggerAgent: Applied " + juce::String(gainDb, 1) +
      "dB trim to track " + track->getName());

  return true;
}

bool SessionDebuggerAgent::removeGainTrim(int trackIndex) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  auto it = insertedGainPlugins_.find(trackIndex);
  if (it != insertedGainPlugins_.end()) {
    // In a full implementation, we would remove the GainPlugin from the chain
    insertedGainPlugins_.erase(it);
    return true;
  }

  return false;
}

bool SessionDebuggerAgent::bypassPlugin(int trackIndex, int pluginIndex) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  auto track = tracks[trackIndex];
  if (!track || pluginIndex < 0 || pluginIndex >= track->getNumPlugins()) {
    return false;
  }

  auto *plugin = track->getPlugin(pluginIndex);
  if (!plugin || plugin->isSuspended()) {
    return false;
  }

  plugin->suspendProcessing(true);
  bypassedPlugins_.push_back({trackIndex, pluginIndex});

  DBG("SessionDebuggerAgent: Bypassed plugin " + plugin->getName() +
      " on track " + track->getName());

  return true;
}

bool SessionDebuggerAgent::enablePlugin(int trackIndex, int pluginIndex) {
  auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    return false;
  }

  auto track = tracks[trackIndex];
  if (!track || pluginIndex < 0 || pluginIndex >= track->getNumPlugins()) {
    return false;
  }

  auto *plugin = track->getPlugin(pluginIndex);
  if (!plugin) {
    return false;
  }

  plugin->suspendProcessing(false);

  // Remove from bypassed list
  bypassedPlugins_.erase(std::remove(bypassedPlugins_.begin(),
                                     bypassedPlugins_.end(),
                                     std::make_pair(trackIndex, pluginIndex)),
                         bypassedPlugins_.end());

  DBG("SessionDebuggerAgent: Enabled plugin " + plugin->getName() +
      " on track " + track->getName());

  return true;
}

void SessionDebuggerAgent::recordFix(IssueType type,
                                     const juce::String &description,
                                     const juce::String &trackName,
                                     std::function<void()> undoFunc) {
  FixAction fix;
  fix.issueType = type;
  fix.description = description;
  fix.trackName = trackName;
  fix.successful = true;
  fix.canUndo = (undoFunc != nullptr);
  fix.undoFunction = undoFunc;

  fixHistory_.push_back(fix);
}

//==============================================================================
// Helper Methods
//==============================================================================

void SessionDebuggerAgent::addIssue(const SessionIssue &issue) {
  juce::ScopedLock lock(issuesLock_);

  // Check if we already have this issue
  for (const auto &existing : issues_) {
    if (existing.type == issue.type &&
        existing.trackIndex == issue.trackIndex &&
        existing.pluginIndex == issue.pluginIndex && !existing.isFixed) {
      return; // Already tracking this issue
    }
  }

  issues_.push_back(issue);

  DBG("SessionDebuggerAgent: Issue detected - " + issue.description);

  // Notify listeners
  listeners_.call([&issue](Listener &l) { l.issueDetected(issue); });
}

void SessionDebuggerAgent::resolveIssue(int issueIndex,
                                        const juce::String &fixDescription) {
  juce::ScopedLock lock(issuesLock_);

  if (issueIndex >= 0 && issueIndex < static_cast<int>(issues_.size())) {
    issues_[issueIndex].isFixed = true;
    issues_[issueIndex].fixApplied = fixDescription;
  }
}

void SessionDebuggerAgent::notifyListeners(const SessionIssue &issue,
                                           bool resolved,
                                           const FixAction *fix) {
  if (resolved && fix) {
    listeners_.call(
        [&issue, fix](Listener &l) { l.issueResolved(issue, *fix); });
  }
}

void SessionDebuggerAgent::updateHealthScore() {
  float score = 100.0f;

  juce::ScopedLock lock(issuesLock_);

  for (const auto &issue : issues_) {
    if (issue.isFixed)
      continue;

    switch (issue.severity) {
    case IssueSeverity::Critical:
      score -= 25.0f;
      break;
    case IssueSeverity::Warning:
      score -= 10.0f;
      break;
    case IssueSeverity::Info:
      score -= 2.0f;
      break;
    }
  }

  score = std::max(0.0f, score);

  float oldScore = sessionHealthScore_.exchange(score);

  if (std::abs(oldScore - score) > 1.0f) {
    listeners_.call([score](Listener &l) { l.sessionHealthChanged(score); });
  }
}

float SessionDebuggerAgent::measureTrackPeakLevel(int trackIndex) const {
  auto &tracks = engine_.tracks();
  if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size())) {
    if (tracks[trackIndex]) {
      return tracks[trackIndex]->getPeakLevel();
    }
  }
  return 0.0f;
}

float SessionDebuggerAgent::measureTrackRmsLevel(int trackIndex) const {
  // RMS level would require access to the audio buffer
  // For now, use peak as a proxy
  return measureTrackPeakLevel(trackIndex);
}

int SessionDebuggerAgent::getPluginLatencySamples(
    juce::AudioPluginInstance *plugin) const {
  if (plugin) {
    return plugin->getLatencySamples();
  }
  return 0;
}

float SessionDebuggerAgent::samplesToMs(int samples) const {
  double sampleRate = engine_.getSampleRate();
  if (sampleRate > 0.0) {
    return static_cast<float>(samples) / static_cast<float>(sampleRate) *
           1000.0f;
  }
  return 0.0f;
}

} // namespace ai
} // namespace zenith
