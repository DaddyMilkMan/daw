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
struct EnvelopeVisualState {
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.2f;
    float currentValue = 0.0f;     // Current output level
    int currentStage = 0;          // 0=attack, 1=decay, 2=sustain, 3=release, 4=idle
    float stageProgress = 0.0f;    // Progress through current stage (0-1)
    bool isActive = false;         // Is envelope currently running
};

//==============================================================================
/**
    Modulation routing visualization
*/

} // namespace
