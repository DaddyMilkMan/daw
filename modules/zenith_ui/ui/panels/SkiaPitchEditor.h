/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../../dsp/PitchDetector.h"
#include <core/SkCanvas.h>
#include <core/SkPath.h>
#include <core/SkPaint.h>
#include <core/SkFont.h>
#include <vector>
#include <memory>

namespace zenith {
namespace ui {

//==============================================================================
/**
    GPU-rendered pitch data point.
*/
struct PitchDataPoint
{
    double time = 0.0;
    float pitchHz = 0.0f;
    float confidence = 0.0f;
    SkPoint screenPos;
};

//==============================================================================
/**
    GPU-rendered note block for editing.
*/
struct EditableNoteBlock
{
    double startTime = 0.0;
    double endTime = 0.0;
    float pitchHz = 0.0f;
    int midiNote = 0;
    bool isSelected = false;
    bool isDragging = false;
    
    // Visual properties
    SkRect screenRect;
    SkColor color;
};

//==============================================================================
/**
    PROFESSIONAL Skia GPU pitch editor.
    
    60fps GPU-accelerated pitch editing with:
    - Smooth waveform background
    - Anti-aliased pitch curves
    - Glow effects on notes
    - Smooth zoom/pan
    - Modern dark theme
*/
class SkiaPitchEditor : public SkiaComponent
{
public:
    //==============================================================================
    SkiaPitchEditor();
    ~SkiaPitchEditor() override;

    //==============================================================================
    // SkiaComponent overrides
    void drawSkia(SkCanvas* canvas) override;
    void onResize() override;
    void onMouseDown(const juce::MouseEvent& event) override;
    void onMouseDrag(const juce::MouseEvent& event) override;
    void onMouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, 
                        const juce::MouseWheelDetails& wheel) override;

    //==============================================================================
    // Audio loading and analysis
    void loadAudio(const juce::AudioBuffer<float>& audio, double sampleRate);
    void analyzePitch();
    void clear();
    
    //==============================================================================
    // View control
    void zoomToFit();
    void zoomIn();
    void zoomOut();
    void setViewRange(double startTime, double endTime);
    void scrollToTime(double time);
    
    //==============================================================================
    // Playback
    void setPlayhead(double time);
    double getPlayhead() const { return playheadTime_; }
    
    //==============================================================================
    // Tools
    enum class Tool { Pointer, Pencil, Split, Hand };
    void setTool(Tool tool);
    Tool getTool() const { return currentTool_; }

private:
    //==============================================================================
    // Rendering (GPU-accelerated)
    void drawBackground(SkCanvas* canvas);
    void drawGrid(SkCanvas* canvas);
    void drawWaveform(SkCanvas* canvas);
    void drawPitchCurve(SkCanvas* canvas);
    void drawNotes(SkCanvas* canvas);
    void drawPlayhead(SkCanvas* canvas);
    
    // GPU-optimized primitives
    void drawSmoothPath(SkCanvas* canvas, const SkPath& path, SkColor color, 
                        float width, bool glow = false);
    void drawGlowRect(SkCanvas* canvas, const SkRect& rect, SkColor color);
    
    //==============================================================================
    // Coordinate conversion
    float timeToX(double time) const;
    double xToTime(float x) const;
    float pitchToY(float pitchHz) const;
    float yToPitch(float y) const;
    
    //==============================================================================
    // Hit testing
    int getNoteAt(float x, float y) const;
    bool isOverNoteEdge(int noteIndex, float x, float y, bool& leftEdge) const;
    
    //==============================================================================
    // Interaction
    void startDrag(const juce::MouseEvent& event);
    void continueDrag(const juce::MouseEvent& event);
    void endDrag();
    void handleZoom(float delta, float centerX);
    void handlePan(float deltaX, float deltaY);
    
    //==============================================================================
    // Data
    juce::AudioBuffer<float> audioBuffer_;
    std::vector<float> waveformCache_;
    std::vector<PitchDataPoint> pitchPoints_;
    std::vector<EditableNoteBlock> notes_;
    double sampleRate_ = 44100.0;
    double audioDuration_ = 0.0;
    
    // Pitch detection
    std::unique_ptr<dsp::PitchDetector> pitchDetector_;
    
    // View state
    double viewStartTime_ = 0.0;
    double viewEndTime_ = 10.0;
    float minPitchHz_ = 80.0f;
    float maxPitchHz_ = 1000.0f;
    
    // Interaction state
    Tool currentTool_ = Tool::Pointer;
    bool isDragging_ = false;
    bool isPanning_ = false;
    int draggedNoteIndex_ = -1;
    bool draggingLeftEdge_ = false;
    bool draggingRightEdge_ = false;
    juce::Point<float> dragStartPos_;
    juce::Point<float> lastMousePos_;
    double dragStartNoteTime_ = 0.0;
    
    // Playback
    double playheadTime_ = 0.0;
    
    // Cached Skia paints (for performance)
    SkPaint linePaint_;
    SkPaint fillPaint_;
    SkPaint glowPaint_;
    SkPaint textPaint_;
    SkFont labelFont_;
    
    // Theme colors
    SkColor bgColor_ = SkColorSetRGB(18, 18, 18);
    SkColor gridColor_ = SkColorSetRGB(40, 40, 40);
    SkColor waveColor_ = SkColorSetRGB(60, 60, 60);
    SkColor pitchColor_ = SkColorSetRGB(0, 255, 200);    // Cyan
    SkColor noteColor_ = SkColorSetRGB(0, 150, 255);     // Blue
    SkColor noteSelectedColor_ = SkColorSetRGB(255, 100, 0); // Orange
    SkColor playheadColor_ = SkColorSetRGB(255, 255, 255);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPitchEditor)
};

} // namespace ui
} // namespace zenith
