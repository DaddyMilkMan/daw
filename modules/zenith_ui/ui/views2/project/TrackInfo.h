/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "../../design-system/ZenithTheme.h"
#include "engine/Engine.h"
#include "engine/ProjectState.h"
#include "engine/RecentProjectManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace zenith {
namespace ui {

// Project information
struct TrackInfo {
    juce::String id;
    juce::String name;
    juce::String type;  // "audio", "midi", "bus", "master"
    bool isMuted = false;
    bool isSoloed = false;
    bool isArmed = false;
    float volume = 0.0f;  // dB
    float pan = 0.0f;     // -1 to 1
    juce::Colour color;
    juce::Array<juce::PluginDescription> plugins;
    juce::StringArray clips;
};

// Template information

} // namespace
