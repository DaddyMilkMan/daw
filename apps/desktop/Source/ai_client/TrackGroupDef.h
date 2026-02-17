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
#include <network/AudioAnalysisService.h>
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

} // namespace
