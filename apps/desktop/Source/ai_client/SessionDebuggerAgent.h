/*
  ==============================================================================

    SessionDebuggerAgent.h
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    "The Session Debugger" - Technical Integrity Agent

    Role: The System Admin. It treats the audio engine like a compile target
    and fixes "runtime errors." It monitors the "health" of the session
    (CPU, Latency, Gain Staging) and autonomously fixes technical issues
    that ruin the audio.

    Features:
    - CPU Spike Detection & Track Freezing
    - Gain Staging Analysis & Auto-Trim
    - Latency Chain Analysis & Low Latency Mode
    - Real-time session health monitoring
    - Autonomous fix application

  ==============================================================================
*/

#pragma once

#include <zenith_core/engine/RoutingGraph.h>
#include <zenith_core/engine/Track.h>
#include <zenith_core/engine/Engine.h>
#include <atomic>
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>


namespace zenith {
namespace ai {

//==============================================================================
/**
    Session Issue Types detected by the debugger
*/
enum class IssueType {
  CPUSpike,          // Track consuming excessive CPU
  InputClipping,     // Signal clipping before a processor
  OutputClipping,    // Signal clipping after processing
  HighLatency,       // Plugin introducing excessive latency
  FeedbackLoop,      // Routing cycle detected
  SilentTrack,       // Track producing no audio despite content
  PhaseCancellation, // Possible phase issues detected
  None
};

/**
    Issue severity levels
*/
enum class IssueSeverity {
  Info,    // Informational, no action needed
  Warning, // Potential issue, fix recommended
  Critical // Must be fixed for proper playback
};

//==============================================================================
/**
    Represents a detected session issue
*/
struct SessionIssue {
  IssueType type = IssueType::None;
  IssueSeverity severity = IssueSeverity::Info;

  juce::String trackId;
  juce::String trackName;
  int trackIndex = -1;

  juce::String pluginName; // If issue is plugin-related
  int pluginIndex = -1;

  juce::String description;  // Human-readable description
  juce::String suggestedFix; // Suggested remediation

  float value = 0.0f;     // Numeric value (CPU%, dB, ms, etc.)
  float threshold = 0.0f; // Threshold that was exceeded

  juce::Time detectedAt;   // When the issue was detected
  bool isFixed = false;    // Whether the issue has been resolved
  juce::String fixApplied; // Description of the fix applied

  SessionIssue() : detectedAt(juce::Time::getCurrentTime()) {}

  // Helper to get a short status string
  juce::String getStatusString() const {
    switch (type) {
    case IssueType::CPUSpike:
      return "CPU: " + juce::String(value, 0) + "%";
    case IssueType::InputClipping:
    case IssueType::OutputClipping:
      return "Clip: +" + juce::String(value, 1) + "dB";
    case IssueType::HighLatency:
      return "Latency: " + juce::String(value, 0) + "ms";
    case IssueType::FeedbackLoop:
      return "Feedback Loop";
    case IssueType::SilentTrack:
      return "Silent";
    case IssueType::PhaseCancellation:
      return "Phase Issue";
    default:
      return "Unknown";
    }
  }
};

//==============================================================================
/**
    Represents a fix action taken by the debugger
*/
struct FixAction {
  IssueType issueType;
  juce::String description;
  juce::String trackName;
  juce::Time appliedAt;
  bool successful;

  // Undo information
  bool canUndo = true;
  std::function<void()> undoFunction;

  FixAction() : appliedAt(juce::Time::getCurrentTime()), successful(true) {}
};

//==============================================================================
/**
    GainPlugin - Simple gain adjustment processor for gain staging fixes
*/
class GainPlugin : public juce::AudioProcessor {
public:
  GainPlugin()
      : AudioProcessor(
            BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo())
                .withOutput("Output", juce::AudioChannelSet::stereo())) {}

  ~GainPlugin() override = default;

  // AudioProcessor interface
  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
  }

  void releaseResources() override {}

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midi) override {
    juce::ignoreUnused(midi);
    float gain = juce::Decibels::decibelsToGain(gainDb_.load());
    buffer.applyGain(gain);
  }

  // Plugin identity
  const juce::String getName() const override { return "Zenith Gain Trim"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  // Programs (not used)
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String &) override {}

  // State
  void getStateInformation(juce::MemoryBlock &destData) override {
    juce::MemoryOutputStream(destData, true).writeFloat(gainDb_.load());
  }

  void setStateInformation(const void *data, int sizeInBytes) override {
    if (sizeInBytes >= sizeof(float)) {
      auto stream = juce::MemoryInputStream(
          data, static_cast<size_t>(sizeInBytes), false);
      gainDb_.store(stream.readFloat());
    }
  }

  bool hasEditor() const override { return false; }
  juce::AudioProcessorEditor *createEditor() override { return nullptr; }

  // Gain control
  void setGainDb(float db) { gainDb_.store(db); }
  float getGainDb() const { return gainDb_.load(); }

private:
  std::atomic<float> gainDb_{0.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainPlugin)
};

//==============================================================================
/**
    Session analysis configuration
*/
struct AnalysisConfig {
  // CPU thresholds
  float cpuWarningThreshold = 50.0f;      // % CPU usage for warning
  float cpuCriticalThreshold = 80.0f;     // % CPU usage for critical
  float trackCpuWarningThreshold = 30.0f; // % CPU per track for warning

  // Gain staging thresholds
  float inputClipThreshold = 0.0f;  // dB above which is clipping
  float outputClipThreshold = 0.0f; // dB above which is clipping
  float headroomTarget = -6.0f;     // Target headroom in dB
  float silenceThreshold = -80.0f;  // dB below which is silence

  // Latency thresholds
  float latencyWarningMs = 20.0f;  // ms latency for warning
  float latencyCriticalMs = 50.0f; // ms latency for critical

  // Analysis timing
  int analysisIntervalMs = 500; // How often to run analysis
  int peakHoldTimeMs = 1000;    // How long to hold peak values

  // Auto-fix settings
  bool autoFixCpuSpikes = true; // Auto-freeze high CPU tracks
  bool autoFixClipping = true;  // Auto-adjust gain staging
  bool autoFixLatency =
      false; // Auto-bypass high latency plugins (during recording only)
};

//==============================================================================
/**
    Track CPU Usage Metrics
*/
struct TrackCpuMetrics {
  int trackIndex = -1;
  juce::String trackName;
  float averageCpuPercent = 0.0f;
  float peakCpuPercent = 0.0f;
  int numPlugins = 0;
  std::vector<float> pluginCpuUsage; // Per-plugin CPU usage

  void reset() {
    averageCpuPercent = 0.0f;
    peakCpuPercent = 0.0f;
    pluginCpuUsage.clear();
  }
};

//==============================================================================
/**
    Main Session Debugger Agent

    Monitors session health and autonomously fixes technical issues.
*/
class SessionDebuggerAgent : public juce::Timer,
                             public juce::ChangeBroadcaster {
public:
  //==========================================================================
  explicit SessionDebuggerAgent(Engine &engine);
  ~SessionDebuggerAgent() override;

  //==========================================================================
  // Analysis Control
  //==========================================================================

  /**
   * @brief Start monitoring the session
   * @param intervalMs Analysis interval in milliseconds
   */
  void startMonitoring(int intervalMs = 500);

  /**
   * @brief Stop monitoring the session
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
  const std::vector<SessionIssue> &getIssues() const { return issues_; }

  /**
   * @brief Get issues filtered by type
   */
  std::vector<SessionIssue> getIssuesByType(IssueType type) const;

  /**
   * @brief Get issues filtered by severity
   */
  std::vector<SessionIssue> getIssuesBySeverity(IssueSeverity severity) const;

  /**
   * @brief Get unresolved issues count
   */
  int getUnresolvedIssueCount() const;

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
   * @brief Get history of fix actions taken
   */
  const std::vector<FixAction> &getFixHistory() const { return fixHistory_; }

  /**
   * @brief Get count of fixes applied in current session
   */
  int getFixCount() const { return static_cast<int>(fixHistory_.size()); }

  /**
   * @brief Get summary of fixes applied
   * @return Human-readable summary like "3 Issues Fixed: CPU Optimized,
   * Clipping Resolved"
   */
  juce::String getFixSummary() const;

  /**
   * @brief Undo the last fix action if possible
   * @return true if undo was successful
   */
  bool undoLastFix();

  //==========================================================================
  // Manual Fix Triggers
  //==========================================================================

  /**
   * @brief Manually trigger CPU optimization for a track
   * @param trackIndex Track to optimize
   * @param freeze If true, freeze the track; if false, reduce plugin count
   */
  bool optimizeTrackCpu(int trackIndex, bool freeze = true);

  /**
   * @brief Manually fix gain staging for a track
   * @param trackIndex Track to fix
   * @param targetHeadroom Target headroom in dB
   */
  bool fixTrackGainStaging(int trackIndex, float targetHeadroom = -6.0f);

  /**
   * @brief Enable low latency mode for a track (bypass high-latency plugins)
   * @param trackIndex Track to optimize
   * @param enable Enable or disable low latency mode
   */
  bool setTrackLowLatencyMode(int trackIndex, bool enable);

  /**
   * @brief Bypass a specific high-latency plugin
   * @param trackIndex Track containing the plugin
   * @param pluginIndex Plugin to bypass
   */
  bool bypassHighLatencyPlugin(int trackIndex, int pluginIndex);

  /**
   * @brief Lock a track to prevent the debugger from making changes
   * @param trackIndex Track to lock/unlock
   * @param locked true to lock, false to unlock
   */
  void setTrackDebuggingLocked(int trackIndex, bool locked);

  /**
   * @brief Check if a track is locked
   */
  bool isTrackDebuggingLocked(int trackIndex) const;

  //==========================================================================
  // Configuration
  //==========================================================================

  /**
   * @brief Get current analysis configuration
   */
  AnalysisConfig &getConfig() { return config_; }
  const AnalysisConfig &getConfig() const { return config_; }

  /**
   * @brief Set analysis configuration
   */
  void setConfig(const AnalysisConfig &config) { config_ = config; }

  //==========================================================================
  // Session Statistics
  //==========================================================================

  /**
   * @brief Get overall session health score (0-100)
   * 100 = Perfect health, 0 = Critical issues
   */
  float getSessionHealthScore() const;

  /**
   * @brief Get current total CPU usage
   */
  float getTotalCpuUsage() const { return totalCpuUsage_.load(); }

  /**
   * @brief Get per-track CPU metrics
   */
  const std::vector<TrackCpuMetrics> &getTrackCpuMetrics() const {
    return trackCpuMetrics_;
  }

  /**
   * @brief Get total session latency in ms
   */
  float getTotalLatencyMs() const { return totalLatencyMs_.load(); }

  /**
   * @brief Get number of tracks with clipping
   */
  int getClippingTrackCount() const { return clippingTrackCount_.load(); }

  //==========================================================================
  // Listeners
  //==========================================================================

  class Listener {
  public:
    virtual ~Listener() = default;
    virtual void issueDetected(const SessionIssue &issue) = 0;
    virtual void issueResolved(const SessionIssue &issue,
                               const FixAction &fix) = 0;
    virtual void sessionHealthChanged(float newHealthScore) = 0;
  };

  void addListener(Listener *listener);
  void removeListener(Listener *listener);

private:
  //==========================================================================
  // Timer callback
  void timerCallback() override;

  //==========================================================================
  // Analysis Methods
  //==========================================================================

  void analyzeCpuUsage();
  void analyzeGainStaging();
  void analyzeLatency();
  void analyzeRoutingHealth();

  //==========================================================================
  // Fix Methods
  //==========================================================================

  void applyAutomaticFixes();

  bool freezeTrack(int trackIndex);
  bool unfreezeTrack(int trackIndex);
  bool insertGainTrim(int trackIndex, float gainDb);
  bool removeGainTrim(int trackIndex);
  bool bypassPlugin(int trackIndex, int pluginIndex);
  bool enablePlugin(int trackIndex, int pluginIndex);

  void recordFix(IssueType type, const juce::String &description,
                 const juce::String &trackName,
                 std::function<void()> undoFunc = nullptr);

  //==========================================================================
  // Helpers
  //==========================================================================

  void addIssue(const SessionIssue &issue);
  void resolveIssue(int issueIndex, const juce::String &fixDescription);
  void notifyListeners(const SessionIssue &issue, bool resolved,
                       const FixAction *fix = nullptr);
  void updateHealthScore();

  float measureTrackPeakLevel(int trackIndex) const;
  float measureTrackRmsLevel(int trackIndex) const;
  int getPluginLatencySamples(juce::AudioPluginInstance *plugin) const;
  float samplesToMs(int samples) const;
  bool isPluginIntentionalDistortion(const juce::String &pluginName) const;

  //==========================================================================
  // Member Variables
  //==========================================================================

  Engine &engine_;
  AnalysisConfig config_;

  // Monitoring state
  std::atomic<bool> isMonitoring_{false};

  // Issues and fixes
  std::vector<SessionIssue> issues_;
  std::vector<FixAction> fixHistory_;
  juce::CriticalSection issuesLock_;

  // Real-time metrics (atomics for thread safety)
  std::atomic<float> totalCpuUsage_{0.0f};
  std::atomic<float> totalLatencyMs_{0.0f};
  std::atomic<int> clippingTrackCount_{0};
  std::atomic<float> sessionHealthScore_{100.0f};

  // Per-track CPU tracking
  std::vector<TrackCpuMetrics> trackCpuMetrics_;

  // Track frozen state (for undo)
  std::unordered_map<int, bool> originalFrozenState_;

  // Inserted gain plugins (for undo)
  std::unordered_map<int, GainPlugin *> insertedGainPlugins_;

  // Bypassed plugins (for undo)
  std::vector<std::pair<int, int>>
      bypassedPlugins_; // (trackIndex, pluginIndex)

  // Locked tracks (user manually excluded from debugging)
  std::unordered_set<int> lockedTracks_;
  mutable juce::CriticalSection lockedTracksLock_;

  // Whitelist for plugins that intentionally clip (Distortion, Bitcrusher,
  // etc.)
  std::vector<juce::String> intentionalClippingKeywords_;

  // Listeners
  juce::ListenerList<Listener> listeners_;

  //==========================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionDebuggerAgent)
};

} // namespace ai
} // namespace zenith
