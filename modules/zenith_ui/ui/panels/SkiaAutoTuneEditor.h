/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "SkiaPitchEditor.h"
#include "../../zenith_core/effects/ZenithUltraLowLatencyAutoTune.h"
#include <array>

namespace zenith {
namespace ui {
namespace panels {

//==============================================================================
/**
 * Professional Auto-Tune editor panel with complete UI.
 *
 * Features:
 * - Real-time pitch visualization (via SkiaPitchEditor)
 * - All parameter controls (retune speed, humanize, correction amount, formant preservation)
 * - Key/scale selection interface
 * - 5 factory presets (Natural, Transparent, Tight, Robot, Subtle)
 * - Latency mode selector (Turbo to HighQuality)
 * - Algorithm selection (AutoCorrelation, YIN, Hybrid)
 * - Real-time feedback (detected pitch, target pitch, confidence, algorithm used)
 * - Downsampling toggle
 * - Pitch prediction toggle
 * - Preset browser integration
 *
 * Latency modes (at 44.1kHz):
 * - Turbo:    16 samples = 0.36ms - SUB-2MS TOTAL! Fastest, quality tradeoff
 * - Extreme:   32 samples = 0.73ms - BEATS Auto-Tune Pro!
 * - UltraLow:  64 samples = 1.45ms
 * - Low:       128 samples = 2.90ms
 * - Standard:  256 samples = 5.80ms
 * - HighQuality:512 samples = 11.6ms
 */
class SkiaAutoTuneEditor : public SkiaComponent
{
public:
    //==============================================================================
    SkiaAutoTuneEditor(effects::ZenithUltraLowLatencyAutoTune& processor);
    ~SkiaAutoTuneEditor() override;

    //==============================================================================
    // SkiaComponent overrides
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;

    //==============================================================================
    // Mouse interaction for preset selection
    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

private:
    //==============================================================================
    // Layout sections
    enum class LayoutSection
    {
        Header,           // Title and algorithm selector
        Visualization,     // Pitch display
        Latency,         // Mode selector
        MainControls,     // Retune speed, humanize, correction amount
        ScaleKey,         // Key and scale selection
        Formant,          // Formant preservation
        Presets,          // Preset buttons
        Advanced          // Downsampling, pitch prediction toggles
        Feedback           // Real-time metrics display
    };

    //==============================================================================
    // Components
    effects::ZenithUltraLowLatencyAutoTune& autoTune_;

    // Pitch visualization (background layer)
    SkiaPitchEditor pitchEditor_;

    // Preset buttons (5 factory presets)
    struct PresetButton
    {
        const char* name;
        effects::PresetType type;
        bool isActive;
        SkRect bounds;
    };
    std::array<PresetButton, 5> presetButtons_;

    // Layout bounds
    std::array<SkRect, 10> sectionBounds_;

    // UI state
    effects::LatencyMode currentLatencyMode_ = effects::LatencyMode::Low;
    bool showAdvanced_ = false;

    //==============================================================================
    // Drawing helpers
    void drawPresetButtons(SkCanvas* canvas);
    void drawSection(SkCanvas* canvas, LayoutSection section, const char* title);
    void drawPresetButton(SkCanvas* canvas, const PresetButton& btn);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaAutoTuneEditor)
};

} // namespace panels
} // namespace ui
} // namespace zenith
