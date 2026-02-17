/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "SkiaComponent.h"
#include "SkiaLayout.h"
#include "ZenithDesignSystem.h"
#include <juce_core/juce_core.h>

// Forward declarations for UI components
namespace zenith {
namespace layout {
  struct Category {
    juce::String name;
    juce::String icon;
    juce::Array<juce::String> settings;
  };

} // namespace
