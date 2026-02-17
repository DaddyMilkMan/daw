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
struct MacroMetadata {
  juce::String id;          // Stable ID (e.g., "macro_warmth")
  juce::String name;        // Human-readable name (e.g., "Warmth")
  juce::String description; // What this macro does

  std::vector<MacroTarget> targets; // Parameters affected by this macro

  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("id", id);
    obj->setProperty("name", name);
    obj->setProperty("description", description);

    juce::Array<juce::var> targetsArray;
    for (const auto &target : targets)
      targetsArray.add(target.toVar());
    obj->setProperty("targets", targetsArray);

    return juce::var(obj);
  }
};

//==============================================================================
/**
    Instrument Preset Structure
*/

} // namespace
