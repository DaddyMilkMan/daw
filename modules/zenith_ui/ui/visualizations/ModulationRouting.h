/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../../instruments/ZenithPolySynthDefs.h"
#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include <complex>

#ifdef ZENITH_USE_SKIA
extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <core/SkFont.h>
#include <core/SkShader.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#pragma clang diagnostic pop
}
#endif

namespace zenith {

//==============================================================================
/**
    LFO state for visualization
*/
struct ModulationRouting {
    ModulationSource source;
    ModulationDestination dest;
    float amount = 0.0f;
    juce::String sourceName;
    juce::String destName;
    SkColor color;

    bool isActive() const {
        return source != ModulationSource::None && dest != ModulationDestination::None;
    }
};

//==============================================================================
/**
    Real-time modulation visualization component

    Features:
    - LFO waveform display with animated phase indicator
    - Envelope display with current stage indicator
    - Modulation matrix with animated connection lines
    - Real-time modulation value display
*/

} // namespace
