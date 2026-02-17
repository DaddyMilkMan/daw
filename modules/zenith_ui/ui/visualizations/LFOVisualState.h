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
struct LFOVisualState {
    float phase = 0.0f;           // Current phase (0-1)
    float rate = 1.0f;            // Rate in Hz
    float amount = 0.5f;          // Modulation amount
    LFOWaveform waveform = LFOWaveform::Sine;
    bool bpmSynced = false;
    int syncRate = static_cast<int>(SyncRate::_1_4);

    // For visualization path
    std::vector<float> waveformBuffer;  // Pre-calculated waveform
};

//==============================================================================
/**
    Envelope state for visualization
*/

} // namespace
