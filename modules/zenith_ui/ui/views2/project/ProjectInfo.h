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
struct ProjectInfo {
    juce::String name;
    juce::String filePath;
    juce::Time created;
    juce::Time lastModified;
    juce::Time lastSaved;
    juce::String author;
    juce::String description;
    juce::String genre;
    float tempo = 120.0f;
    juce::String key = "C";
    int sampleRate = 44100;
    int bitDepth = 24;
    float duration = 0.0f;  // seconds
    size_t fileSize = 0;    // bytes
    bool isModified = false;
    bool autoSaveEnabled = true;
    int autoSaveInterval = 300;  // seconds
};

// Track information

} // namespace
