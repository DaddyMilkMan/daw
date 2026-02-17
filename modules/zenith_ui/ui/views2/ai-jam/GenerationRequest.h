/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../../framework/SkiaComponent.h"
#include "../../framework/GlassmorphicPanel.h"
#include "../../design-system/ZenithTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>

// Forward declarations
namespace zenith {
    struct GenerationRequest {
        juce::String prompt;
        juce::Time timestamp;
    };
    std::vector<GenerationRequest> requestHistory_;

    // Generate stems based on AI response
    void generateStemsFromAIResponse(const juce::String& response);
    void addSystemMessage(const juce::String& text);
    std::vector<float> generateWaveformFromSeed(int seed);
    std::vector<float> generateWaveformFromAudio(const juce::File& audioFile);
    std::vector<float> getWaveformForStem(const GeneratedStem& stem);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaAIJamView)
};

} // namespace zenith::ui
