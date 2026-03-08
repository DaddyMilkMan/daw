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
    Get color for instrument category (Zenith dark palette)
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
// Forward declarations
struct ClipArchive;
struct SampleConsolidation;

//==============================================================================
/**
    Track rename operation
*/
struct TrackRename {
  juce::String trackId;
  juce::String oldName;
  juce::String newName;
  InstrumentCategory category;
};

/**
    Track color change operation
*/
struct TrackColorChange {
  juce::String trackId;
  juce::Colour color;
  InstrumentCategory category;
};

/**
    Track group definition
*/
struct TrackGroupDef {
  juce::String name;
  std::vector<juce::String> trackIds;
  juce::Colour color;
};

//==============================================================================
/**
    Refactoring plan - contains all detected issues and proposed fixes
*/
struct RefactorPlan {
  // Track renames
  std::vector<TrackRename> trackRenames;

  // Color changes
  std::vector<TrackColorChange> colorChanges;

  // Track groups to create
  std::vector<TrackGroupDef> groupsToCreate;

  // Clips to archive (dead/muted clips)
  std::vector<ClipArchive> clipsToArchive;

  // Samples to consolidate (external samples)
  std::vector<SampleConsolidation> samplesToConsolidate;

  // Summary statistics
  int totalIssuesFound = 0;
  int tracksToRename = 0;
  int tracksToColor = 0;
  int groupsToCreate_ = 0;
  int clipsToArchiveCount = 0;
  int samplesToMove = 0;

  // Helper methods
  juce::String getSummary() const;
  bool isEmpty() const;
  void clear();
  void calculateStatistics();
  juce::var toJSON() const;
  bool fromJSON(const juce::var& json);
};

//==============================================================================
/**
    Clip archival information
*/
struct ClipArchive {
  juce::String trackId;
  juce::String clipId;
  juce::String reason;
  juce::String targetTrackId; // ID of the quarantine track

  // Helper methods
  juce::String getReason() const;
  juce::String toString() const;
  bool isValid() const;
};

//==============================================================================
/**
    Sample consolidation information
*/
struct SampleConsolidation {
  juce::File sourceFile;
  juce::File destFile;
  juce::String clipId;

  // Helper methods
  juce::String toString() const;
  bool isValid() const;
  bool needsCopy() const;
  juce::int64 getSourceFileSize() const;
  juce::int64 getEstimatedDiskSpaceSaved() const;
};

} // namespace ai
} // namespace zenith
