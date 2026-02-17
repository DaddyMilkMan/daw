/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../../engine/NativeAutoTuneEffect.h"
#include <vector>
#include <memory>

namespace zenith {
namespace ui {

//==============================================================================
/**
 * Pitch correction overlay for arranger view.
 *
 * Shows pitch curves overlaid on clips like Melodyne:
 * - Detected pitch in red/yellow
 * - Corrected pitch in green
 * - Note blocks with editable pitch
 * - Drag notes to change pitch
 * - Double-click to split notes
 *
 * This is a native DAW view, NOT a plugin UI.
 */
class PitchCorrectionOverlay : public SkiaComponent
{
public:
    //==============================================================================
    PitchCorrectionOverlay();
    ~PitchCorrectionOverlay() override;

    //==============================================================================
    /**
     * @brief Set the Auto-Tune effect to visualize
     */
    void setAutoTuneEffect(engine::NativeAutoTuneEffect* effect);

    /**
     * @brief Set the time range to display
     */
    void setTimeRange(double startTime, double endTime);

    /**
     * @brief Set pitch range (Hz)
     */
    void setPitchRange(float minPitch, float maxPitch);

    /**
     * @brief Enable/disable pitch correction overlay
     */
    void setOverlayEnabled(bool enabled);
    bool isOverlayEnabled() const { return overlayEnabled_; }

    //==============================================================================
    // SkiaComponent overrides
    void drawSkia(SkCanvas* canvas) override;

    //==============================================================================
    // Mouse interaction
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;
    void mouseDoubleClick(const MouseEvent& event) override;

private:
    //==============================================================================
    // Drawing methods
    void drawPitchCurve(SkCanvas* canvas);
    void drawNotes(SkCanvas* canvas);
    void drawGrid(SkCanvas* canvas);
    void drawPlayhead(SkCanvas* canvas);

    //==============================================================================
    // Coordinate conversion
    float timeToX(double time) const;
    double xToTime(float x) const;
    float pitchToY(float pitchHz) const;
    float yToPitch(float y) const;

    //==============================================================================
    // Note editing
    struct Note
    {
        double startTime = 0.0;
        double endTime = 0.0;
        float pitchHz = 0.0f;
        float correctedPitch = 0.0f;
        bool isSelected = false;
        SkRect screenRect;
    };

    int getNoteAt(float x, float y) const;
    void startNoteDrag(int noteIndex, const MouseEvent& event);
    void continueNoteDrag(const MouseEvent& event);
    void endNoteDrag();

    //==============================================================================
    // State
    engine::NativeAutoTuneEffect* autoTuneEffect_ = nullptr;
    bool overlayEnabled_ = true;

    double viewStartTime_ = 0.0;
    double viewEndTime_ = 10.0;
    float minPitchHz_ = 80.0f;    // C2
    float maxPitchHz_ = 1000.0f;  // B5

    std::vector<Note> notes_;
    std::vector<std::pair<double, float>> pitchCurve_;

    // Dragging state
    bool isDragging_ = false;
    int draggedNoteIndex_ = -1;
    juce::Point<float> dragStartPos_;
    float dragStartPitch_ = 0.0f;

    //==============================================================================
    // Visual settings
    SkColor bgColor_{20, 20, 25, 255};
    SkColor gridColor_{60, 60, 70, 255};
    SkColor detectedPitchColor_{255, 100, 100, 255};  // Red for detected
    SkColor correctedPitchColor_{100, 255, 100, 255};  // Green for corrected
    SkColor noteColor_{80, 150, 255, 255};
    SkColor noteSelectedColor_{255, 200, 50, 255};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchCorrectionOverlay)
};

//==============================================================================
/**
 * Factory for creating pitch correction overlays
 */
class PitchCorrectionOverlayFactory
{
public:
    static std::unique_ptr<PitchCorrectionOverlay> create();
};

} // namespace ui
} // namespace zenith
