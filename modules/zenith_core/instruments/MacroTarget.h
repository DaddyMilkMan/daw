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
struct MacroTarget {
  juce::String parameterId; // Parameter this macro affects
  float amount = 1.0f;      // Amount of influence (0-1)

  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("parameterId", parameterId);
    obj->setProperty("amount", amount);
    return juce::var(obj);
  }
};

//==============================================================================
/**
    Macro metadata - high-level control that affects multiple parameters
*/

} // namespace
