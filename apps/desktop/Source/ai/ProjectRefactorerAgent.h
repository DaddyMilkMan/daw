/*
  ==============================================================================

    ProjectRefactorerAgent.h
    Created: 2025-12-07
    Author:  Zenith DAW

    "The Refactorer" - Project Organization Agent

    Role: The Repo Maintainer. Treats the arrangement like messy source code
    and performs "Cleanup/Formatting" operations.

    Features:
    - Detects "spaghetti tracks": Generic names like "Audio 1", "Audio 2"
    - Finds "dead code": Muted clips or clips outside song boundaries
    - Resolves "dependencies": Samples scattered across multiple folders
    - Auto-renames tracks based on spectral analysis
    - Colors tracks by instrument category
    - Groups related tracks into TrackGroups (submix buses)
    - Consolidates all external samples to project folder

  ==============================================================================
*/

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include "../network/AudioAnalysisService.h"
#include <functional>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>
#include <map>
#include <memory>
#include <vector>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Track category detected via spectral analysis
*/
enum class InstrumentCategory {
  Unknown,
  Kick,
  Snare,
  HiHat,
  Cymbal,
  Toms,
  Percussion,
  Bass,
  Guitar,
  Keys,
  Synth,
  Pad,
  Lead,
  Vocals,
  Strings,
  Brass,
  Woodwinds,
  FX,
  Ambient
};

/**
    Get display name for instrument category
*/
inline juce::String getCategoryName(InstrumentCategory cat) {
  switch (cat) {
  case InstrumentCategory::Kick:
    return "Kick";
  case InstrumentCategory::Snare:
    return "Snare";
  case InstrumentCategory::HiHat:
    return "Hi-Hat";
  case InstrumentCategory::Cymbal:
    return "Cymbal";
  case InstrumentCategory::Toms:
    return "Toms";
  case InstrumentCategory::Percussion:
    return "Percussion";
  case InstrumentCategory::Bass:
    return "Bass";
  case InstrumentCategory::Guitar:
    return "Guitar";
  case InstrumentCategory::Keys:
    return "Keys";
  case InstrumentCategory::Synth:
    return "Synth";
  case InstrumentCategory::Pad:
    return "Pad";
  case InstrumentCategory::Lead:
    return "Lead";
  case InstrumentCategory::Vocals:
    return "Vocals";
  case InstrumentCategory::Strings:
    return "Strings";
  case InstrumentCategory::Brass:
    return "Brass";
  case InstrumentCategory::Woodwinds:
    return "Woodwinds";
  case InstrumentCategory::FX:
    return "FX";
  case InstrumentCategory::Ambient:
    return "Ambient";
  default:
    return "Unknown";
  }
}

/**
    Get color for instrument category (Neon Noir palette)
*/
inline juce::Colour getCategoryColor(InstrumentCategory cat) {
  switch (cat) {
  // Drums - Red/Orange spectrum
  case InstrumentCategory::Kick:
    return juce::Colour(0xFFFF3232); // Red
  case InstrumentCategory::Snare:
    return juce::Colour(0xFFFF6432); // Orange-Red
  case InstrumentCategory::HiHat:
    return juce::Colour(0xFFFFA032); // Orange
  case InstrumentCategory::Cymbal:
    return juce::Colour(0xFFFFD232); // Gold
  case InstrumentCategory::Toms:
    return juce::Colour(0xFFFF5050); // Light Red
  case InstrumentCategory::Percussion:
    return juce::Colour(0xFFE64A19); // Deep Orange

  // Bass - Purple spectrum
  case InstrumentCategory::Bass:
    return juce::Colour(0xFF9C27B0); // Purple

  // Melodic - Blue/Cyan spectrum
  case InstrumentCategory::Guitar:
    return juce::Colour(0xFF2196F3); // Blue
  case InstrumentCategory::Keys:
    return juce::Colour(0xFF00BCD4); // Cyan
  case InstrumentCategory::Synth:
    return juce::Colour(0xFF00FFFF); // Bright Cyan
  case InstrumentCategory::Pad:
    return juce::Colour(0xFF3F51B5); // Indigo
  case InstrumentCategory::Lead:
    return juce::Colour(0xFFFF00FF); // Magenta

  // Vocals - Green spectrum
  case InstrumentCategory::Vocals:
    return juce::Colour(0xFF00FF64); // Neon Green

  // Orchestral - Teal/Emerald spectrum
  case InstrumentCategory::Strings:
    return juce::Colour(0xFF009688); // Teal
  case InstrumentCategory::Brass:
    return juce::Colour(0xFFCDDC39); // Lime
  case InstrumentCategory::Woodwinds:
    return juce::Colour(0xFF8BC34A); // Light Green

  // Effects - Gray spectrum
  case InstrumentCategory::FX:
    return juce::Colour(0xFF607D8B); // Blue Grey
  case InstrumentCategory::Ambient:
    return juce::Colour(0xFF455A64); // Dark Blue Grey

  default:
    return juce::Colour(0xFF808080); // Gray
  }
}

//==============================================================================
/**
    Information about an issue detected in the project
*/
struct RefactorIssue {
  enum class Type {
    GenericTrackName,      // "Audio 1", "Track 1", etc.
    MutedClip,             // Clip is muted
    ClipOutOfBounds,       // Clip is outside song start/end
    ScatteredSamples,      // Samples from external folders
    NoTrackColor,          // Track has no color assigned
    DuplicateSamples,      // Same sample used multiple times
    EmptyTrack,            // Track with no clips
    UnorganizedDrums,      // Drum tracks not grouped
    UnorganizedInstruments // Related instruments not grouped
  };

  Type type;
  juce::String trackId;
  juce::String clipId;
  juce::String description;
  juce::File externalFile; // For scattered samples

  RefactorIssue(Type t, const juce::String &track, const juce::String &desc)
      : type(t), trackId(track), description(desc) {}

  RefactorIssue(Type t, const juce::String &track, const juce::String &clip,
                const juce::String &desc)
      : type(t), trackId(track), clipId(clip), description(desc) {}
};

//==============================================================================
/**
    Track group definition for organizing related tracks
*/
struct TrackGroupDef {
  juce::String name;
  InstrumentCategory primaryCategory;
  std::vector<juce::String> trackIds;
  juce::Colour color;

  TrackGroupDef(const juce::String &n, InstrumentCategory cat)
      : name(n), primaryCategory(cat), color(getCategoryColor(cat)) {}
};

//==============================================================================
/**
    Refactoring plan - describes what changes will be made
*/
struct RefactorPlan {
  // Track renames
  struct TrackRename {
    juce::String trackId;
    juce::String oldName;
    juce::String newName;
    InstrumentCategory category;
  };
  std::vector<TrackRename> trackRenames;

  // Track color changes
  struct TrackColorChange {
    juce::String trackId;
    juce::Colour newColor;
  };
  std::vector<TrackColorChange> colorChanges;

  // Track groups to create
  std::vector<TrackGroupDef> groupsToCreate;

  // Clips to archive (dead code)
  struct ClipArchive {
    juce::String trackId;
    juce::String clipId;
    juce::String reason;
    juce::String targetTrackId; // ID of the quarantine track
  };
  std::vector<ClipArchive> clipsToArchive;

  // Samples to consolidate
  struct SampleConsolidation {
    juce::File sourceFile;
    juce::File destFile;
    juce::String clipId;
  };
  std::vector<SampleConsolidation> samplesToConsolidate;

  // Summary statistics
  int totalIssuesFound = 0;
  int tracksToRename = 0;
  int tracksToColor = 0;
  int groupsToCreate_ = 0;
  int clipsToArchiveCount = 0;
  int samplesToMove = 0;

  bool isEmpty() const {
    return trackRenames.empty() && colorChanges.empty() &&
           groupsToCreate.empty() && clipsToArchive.empty() &&
           samplesToConsolidate.empty();
  }

  juce::String getSummary() const {
    juce::StringArray lines;
    lines.add("=== Project Refactoring Plan ===");
    lines.add("");

    if (!trackRenames.empty()) {
      lines.add("Track Renames (" + juce::String(trackRenames.size()) + "):");
      for (const auto &r : trackRenames)
        lines.add("  • " + r.oldName + " → " + r.newName);
      lines.add("");
    }

    if (!colorChanges.empty()) {
      lines.add("Color Assignments (" + juce::String(colorChanges.size()) +
                "):");
      lines.add("  (Tracks will be colored by instrument type)");
      lines.add("");
    }

    if (!groupsToCreate.empty()) {
      lines.add("Track Groups (" + juce::String(groupsToCreate.size()) + "):");
      for (const auto &g : groupsToCreate)
        lines.add("  • " + g.name + " (" + juce::String(g.trackIds.size()) +
                  " tracks)");
      lines.add("");
    }

    if (!clipsToArchive.empty()) {
      lines.add("Dead Clips to Archive (" +
                juce::String(clipsToArchive.size()) + "):");
      for (const auto &c : clipsToArchive)
        lines.add("  • " + c.reason + " (Moving to Quarantine)");
      lines.add("");
    }

    if (!samplesToConsolidate.empty()) {
      lines.add("Samples to Consolidate (" +
                juce::String(samplesToConsolidate.size()) + "):");
      lines.add("  (External samples will be copied to project folder)");
      lines.add("");
    }

    if (isEmpty()) {
      lines.add("✓ Project is already clean - no refactoring needed!");
    }

    return lines.joinIntoString("\n");
  }
};

//==============================================================================
/**
    Refactoring progress callback
*/
struct RefactorProgress {
  int currentStep = 0;
  int totalSteps = 0;
  juce::String currentOperation;
  float progressPercent = 0.0f;
};

//==============================================================================
/**
    Main Project Refactorer Agent

    Usage:
    1. Call analyze() to detect issues and create a plan
    2. Review the plan with getPlan()
    3. Call execute() to apply the changes
*/
class ProjectRefactorerAgent {
public:
  //==========================================================================
  explicit ProjectRefactorerAgent(Engine &engine, ProjectState &projectState);
  ~ProjectRefactorerAgent();

  //==========================================================================
  // Configuration
  //==========================================================================

  struct Options {
    bool renameGenericTracks = true; // Rename "Audio 1" → "Kick"
    bool assignTrackColors = true;   // Color tracks by category
    bool createTrackGroups = true;   // Group related tracks
    bool archiveDeadClips =
        true; // Move muted/out-of-bounds clips to quarantine
    bool consolidateSamples = true; // Copy external samples to project
    bool removeEmptyTracks = false; // Delete tracks with no clips

    double songStartBeats = 0.0; // Song start point (for dead clip detection)
    double songEndBeats = -1.0;  // Song end point (-1 = auto-detect)
  };

  void setOptions(const Options &options);
  const Options &getOptions() const { return options_; }

  //==========================================================================
  // Analysis
  //==========================================================================

  /**
      Analyze the project and detect issues.

      This is an async operation - it analyzes audio files on a background
     thread.

      @param onProgress Progress callback (can be nullptr)
      @param onComplete Completion callback with the generated plan
  */
  void analyze(std::function<void(const RefactorProgress &)> onProgress,
               std::function<void(const RefactorPlan &)> onComplete);

  /**
      Quick analysis without audio content analysis.
      Synchronous - returns immediately with results.
  */
  RefactorPlan analyzeQuick();

  /**
      Get the current/last analysis plan
  */
  const RefactorPlan &getPlan() const { return currentPlan_; }

  /**
      Get the list of detected issues
  */
  const std::vector<RefactorIssue> &getIssues() const { return issues_; }

  //==========================================================================
  // Execution
  //==========================================================================

  /**
      Execute the refactoring plan.

      @param plan The plan to execute (default: current plan)
      @param onProgress Progress callback
      @param onComplete Completion callback with success/failure
  */
  void execute(const RefactorPlan &plan,
               std::function<void(const RefactorProgress &)> onProgress,
               std::function<void(bool success, const juce::String &message)>
                   onComplete);

  /**
      Execute specific parts of the plan
  */
  void executeTrackRenames(const RefactorPlan &plan);
  void executeColorChanges(const RefactorPlan &plan);
  void executeClipArchival(const RefactorPlan &plan);
  void executeSampleConsolidation(const RefactorPlan &plan);

  /**
      Cancel ongoing analysis/execution
  */
  void cancel();

  //==========================================================================
  // Individual Operations (can be called directly)
  //==========================================================================

  /**
      Classify a track based on audio analysis results
  */
  InstrumentCategory classifyFromAnalysis(const AudioAnalysisResults &results);

  /**
      Classify a track based on its name (heuristic fallback)
  */
  InstrumentCategory classifyFromName(const juce::String &name);

  /**
      Generate a descriptive name for a track based on its instrument category
      and position (e.g., "Kick 1", "Snare 2")
  */
  juce::String generateTrackName(InstrumentCategory category, int number);

  /**
      Check if a track name is "generic" (needs renaming)
  */
  bool isGenericName(const juce::String &name);

  /**
      Get the project folder for sample consolidation
  */
  juce::File getProjectSamplesFolder();

private:
  //==========================================================================
  Engine &engine_;
  ProjectState &projectState_;
  AudioAnalysisService analysisService_;

  Options options_;
  RefactorPlan currentPlan_;
  std::vector<RefactorIssue> issues_;

  std::atomic<bool> cancelled_{false};
  std::atomic<bool> analyzing_{false};

  // Analysis helpers
  void detectGenericNames();
  void detectDeadClips();
  void detectScatteredSamples();
  void detectMissingColors();
  void detectGroupableEntities();
  void buildRefactorPlan();

  // Category tracking for name generation
  std::map<InstrumentCategory, int> categoryCounters_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectRefactorerAgent)
};

} // namespace ai
} // namespace zenith
