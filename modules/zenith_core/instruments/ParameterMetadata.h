/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zenith {

//==============================================================================
/**
    Parameter metadata for an instrument parameter
*/
struct ParameterMetadata {
  juce::String id;       // Stable ID (e.g., "filter_cutoff")
  juce::String name;     // Human-readable name (e.g., "Filter Cutoff")
  juce::String category; // Category for grouping (e.g., "Filter", "Envelope")

  enum class Type {
    Float, // Continuous value
    Bool,  // On/Off
    Choice // Discrete choices
  };

  Type type = Type::Float;

  float defaultValue = 0.5f; // Default normalized value (0-1)
  float minValue = 0.0f;     // Minimum value
  float maxValue = 1.0f;     // Maximum value

  juce::String units; // Display units (e.g., "Hz", "dB", "%")

  // For Choice parameters
  juce::StringArray choices;

  // Extended metadata for AI preset design
  juce::String group; // Functional group (e.g., "Oscillator", "Filter",
                      // "Envelope", "LFO", "FX")
  juce::String role;  // Semantic role (e.g., "tone", "mod", "time", "level",
                      // "stereo", "distortion")
  float recommendedStep = 0.01f; // Recommended step size for UI/automation

  // Safe range for randomization (optional, defaults to full range)
  float minSafeRange = -1.0f; // -1 means use minValue
  float maxSafeRange = -1.0f; // -1 means use maxValue

  /**
   * @brief Convert to JSON var for CommandAPI
   */
  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("id", id);
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("type", typeToString(type));
    obj->setProperty("defaultValue", defaultValue);
    obj->setProperty("minValue", minValue);
    obj->setProperty("maxValue", maxValue);
    obj->setProperty("units", units);

    if (type == Type::Choice && !choices.isEmpty()) {
      juce::Array<juce::var> choicesArray;
      for (const auto &choice : choices)
        choicesArray.add(choice);
      obj->setProperty("choices", choicesArray);
    }

    // Extended metadata for AI preset design
    if (group.isNotEmpty())
      obj->setProperty("group", group);
    if (role.isNotEmpty())
      obj->setProperty("role", role);
    obj->setProperty("recommendedStep", recommendedStep);

    // Safe range for randomization (only include if specified)
    if (minSafeRange >= 0.0f || maxSafeRange >= 0.0f) {
      auto *safeRange = new juce::DynamicObject();
      safeRange->setProperty("min",
                             minSafeRange >= 0.0f ? minSafeRange : minValue);
      safeRange->setProperty("max",
                             maxSafeRange >= 0.0f ? maxSafeRange : maxValue);
      obj->setProperty("safeRangeForRandomisation", juce::var(safeRange));
    }

    return juce::var(obj);
  }

private:
  static juce::String typeToString(Type t) {
    switch (t) {
    case Type::Float:
      return "float";
    case Type::Bool:
      return "bool";
    case Type::Choice:
      return "choice";
    default:
      return "float";
    }
  }
};

//==============================================================================
/**
    Macro target - defines which parameter a macro affects and by how much
*/

} // namespace
