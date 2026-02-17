/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "PitchCorrectionOverlay.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace ui {

//==============================================================================
PitchCorrectionOverlay::PitchCorrectionOverlay()
{
    // Initialize Skia paints
    setInterceptsMouseClicks(true, false);
}

PitchCorrectionOverlay::~PitchCorrectionOverlay()
{
}

//==============================================================================
void PitchCorrectionOverlay::setAutoTuneEffect(engine::NativeAutoTuneEffect* effect)
{
    autoTuneEffect_ = effect;

    if (effect)
    {
        // Get note corrections from effect
        const auto& corrections = effect->getNoteCorrections();
        notes_.clear();
        notes_.reserve(corrections.size());

        for (const auto& corr : corrections)
        {
            Note note;
            note.startTime = corr.startTime;
            note.endTime = corr.endTime;
            note.pitchHz = corr.originalPitch;
            note.correctedPitch = corr.correctedPitch;
            note.isSelected = corr.isSelected;
            notes_.push_back(note);
        }
    }

    markDirty();
}

void PitchCorrectionOverlay::setTimeRange(double startTime, double endTime)
{
    viewStartTime_ = startTime;
    viewEndTime_ = endTime;
    markDirty();
}

void PitchCorrectionOverlay::setPitchRange(float minPitch, float maxPitch)
{
    minPitchHz_ = minPitch;
    maxPitchHz_ = maxPitch;
    markDirty();
}

void PitchCorrectionOverlay::setOverlayEnabled(bool enabled)
{
    overlayEnabled_ = enabled;
    markDirty();
}

//==============================================================================
void PitchCorrectionOverlay::drawSkia(SkCanvas* canvas)
{
    if (!overlayEnabled_)
        return;

    // Clear background
    canvas->clear(bgColor_);

    // Draw components in order
    drawGrid(canvas);
    drawPitchCurve(canvas);
    drawNotes(canvas);

    // Draw playhead if we have effect
    if (autoTuneEffect_)
    {
        drawPlayhead(canvas);
    }
}

void PitchCorrectionOverlay::drawGrid(SkCanvas* canvas)
{
    auto bounds = getLocalBounds();
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    SkPaint gridPaint;
    gridPaint.setColor(gridColor_);
    gridPaint.setAlpha(100);
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setStrokeWidth(1.0f);

    // Draw horizontal lines for musical notes (semitones)
    float logMin = std::log2(minPitchHz_);
    float logMax = std::log2(maxPitchHz_);

    for (int semitone = -24; semitone <= 24; ++semitone)
    {
        float pitch = 440.0f * std::pow(2.0f, semitone / 12.0f);

        if (pitch >= minPitchHz_ && pitch <= maxPitchHz_)
        {
            float y = pitchToY(pitch);

            // Highlight C notes
            int noteClass = static_cast<int>(std::round(12.0f * std::log2(pitch / 440.0f))) % 12;
            if (noteClass == 0)  // C
            {
                SkPaint cLinePaint = gridPaint;
                cLinePaint.setColor({100, 100, 120, 255});
                cLinePaint.setStrokeWidth(2.0f);
                canvas->drawLine(0, y, width, y, cLinePaint);
            }
            else
            {
                canvas->drawLine(0, y, width, y, gridPaint);
            }
        }
    }
}

void PitchCorrectionOverlay::drawPitchCurve(SkCanvas* canvas)
{
    if (!autoTuneEffect_)
        return;

    SkPaint detectedPaint;
    detectedPaint.setColor(detectedPitchColor_);
    detectedPaint.setStyle(SkPaint::kStroke_Style);
    detectedPaint.setStrokeWidth(2.0f);
    detectedPaint.setAntiAlias(true);

    SkPaint correctedPaint;
    correctedPaint.setColor(correctedPitchColor_);
    correctedPaint.setStyle(SkPaint::kStroke_Style);
    correctedPaint.setStrokeWidth(2.5f);
    correctedPaint.setAntiAlias(true);

    // Draw detected pitch curve (red)
    SkPath detectedPath;
    bool first = true;

    for (const auto& [time, pitch] : pitchCurve_)
    {
        float x = timeToX(time);
        float y = pitchToY(pitch);

        if (x >= 0 && x <= getWidth() && y >= 0 && y <= getHeight())
        {
            if (first)
            {
                detectedPath.moveTo(x, y);
                first = false;
            }
            else
            {
                detectedPath.lineTo(x, y);
            }
        }
    }

    if (!first)
        canvas->drawPath(detectedPath, detectedPaint);

    // Draw corrected pitch (green) - for now, same curve offset
    // In real implementation, this would show the actual corrected curve
}

void PitchCorrectionOverlay::drawNotes(SkCanvas* canvas)
{
    SkPaint notePaint;
    notePaint.setStyle(SkPaint::kFill_Style);
    notePaint.setAntiAlias(true);

    for (const auto& note : notes_)
    {
        // Update screen rect
        float x1 = timeToX(note.startTime);
        float x2 = timeToX(note.endTime);
        float y = pitchToY(note.correctedPitch);

        note.screenRect = SkRect::MakeXYXY(x1, y - 10, x2, y + 10);

        // Skip if off screen
        if (note.screenRect.isEmpty() ||
            note.screenRect.right() < 0 ||
            note.screenRect.left() > getWidth())
            continue;

        // Draw note with glow
        notePaint.setColor(note.isSelected ? noteSelectedColor_ : noteColor_);
        notePaint.setAlpha(180);

        canvas->drawRect(note.screenRect, notePaint);

        // Border
        SkPaint borderPaint;
        borderPaint.setColor(notePaint.getColor());
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(2.0f);
        canvas->drawRect(note.screenRect, borderPaint);
    }
}

void PitchCorrectionOverlay::drawPlayhead(SkCanvas* canvas)
{
    // Draw current detected pitch as playhead indicator
    float currentPitch = autoTuneEffect_->getDetectedPitch();
    if (currentPitch <= 0.0f)
        return;

    float y = pitchToY(currentPitch);

    SkPaint playheadPaint;
    playheadPaint.setColor({255, 255, 100, 255});  // Yellow
    playheadPaint.setStyle(SkPaint::kStroke_Style);
    playheadPaint.setStrokeWidth(3.0f);

    canvas->drawLine(0, y, getWidth(), y, playheadPaint);
}

//==============================================================================
float PitchCorrectionOverlay::timeToX(double time) const
{
    double timeRange = viewEndTime_ - viewStartTime_;
    if (timeRange <= 0) return 0;

    return static_cast<float>((time - viewStartTime_) / timeRange * getWidth());
}

double PitchCorrectionOverlay::xToTime(float x) const
{
    double timeRange = viewEndTime_ - viewStartTime_;
    double normalized = x / getWidth();
    return viewStartTime_ + normalized * timeRange;
}

float PitchCorrectionOverlay::pitchToY(float pitchHz) const
{
    float logMin = std::log2(minPitchHz_);
    float logMax = std::log2(maxPitchHz_);
    float logPitch = std::log2(pitchHz);

    float normalized = (logPitch - logMin) / (logMax - logMin);
    return getHeight() * (1.0f - normalized);  // Invert Y
}

float PitchCorrectionOverlay::yToPitch(float y) const
{
    float normalized = 1.0f - (y / getHeight());

    float logMin = std::log2(minPitchHz_);
    float logMax = std::log2(maxPitchHz_);
    float logPitch = logMin + normalized * (logMax - logMin);

    return std::pow(2.0f, logPitch);
}

//==============================================================================
int PitchCorrectionOverlay::getNoteAt(float x, float y) const
{
    for (int i = 0; i < static_cast<int>(notes_.size()); ++i)
    {
        if (notes_[i].screenRect.contains(x, y))
            return i;
    }
    return -1;
}

void PitchCorrectionOverlay::mouseDown(const MouseEvent& event)
{
    int noteIndex = getNoteAt(event.position.x, event.position.y);

    if (noteIndex >= 0)
    {
        // Select this note, deselect others
        for (auto& note : notes_)
            note.isSelected = false;

        notes_[noteIndex].isSelected = true;
        startNoteDrag(noteIndex, event);
    }
}

void PitchCorrectionOverlay::mouseDrag(const MouseEvent& event)
{
    if (isDragging_)
        continueNoteDrag(event);
}

void PitchCorrectionOverlay::mouseUp(const MouseEvent& event)
{
    (void)event;
    endNoteDrag();
}

void PitchCorrectionOverlay::mouseDoubleClick(const MouseEvent& event)
{
    // Double-click to split note at this position
    int noteIndex = getNoteAt(event.position.x, event.position.y);

    if (noteIndex >= 0)
    {
        auto& note = notes_[noteIndex];

        // Calculate split time
        double splitTime = xToTime(event.position.x);

        if (splitTime > note.startTime && splitTime < note.endTime)
        {
            // Create two notes from one
            Note secondHalf = note;
            secondHalf.startTime = splitTime;
            note.endTime = splitTime;

            notes_.insert(notes_.begin() + noteIndex + 1, secondHalf);
        }
    }
}

void PitchCorrectionOverlay::startNoteDrag(int noteIndex, const MouseEvent& event)
{
    draggedNoteIndex_ = noteIndex;
    dragStartPos_ = event.position;
    dragStartPitch_ = notes_[noteIndex].correctedPitch;
    isDragging_ = true;
}

void PitchCorrectionOverlay::continueNoteDrag(const MouseEvent& event)
{
    if (draggedNoteIndex_ < 0)
        return;

    auto& note = notes_[draggedNoteIndex_];

    // Calculate pitch change based on Y movement
    float deltaY = dragStartPos_.y - event.position.y;  // Up is positive
    float pitchChange = yToPitch(getHeight() / 2.0f + deltaY) -
                       yToPitch(getHeight() / 2.0f);

    note.correctedPitch = juce::jlimit(minPitchHz_, maxPitchHz_, dragStartPitch_ + pitchChange);

    // Update correction in effect
    if (autoTuneEffect_ && draggedNoteIndex_ < static_cast<int>(autoTuneEffect_->getNoteCorrections().size()))
    {
        auto corrections = autoTuneEffect_->getNoteCorrections();
        corrections[draggedNoteIndex_].correctedPitch = note.correctedPitch;
        // Note: would need non-const getter to actually modify
    }

    markDirty();
}

void PitchCorrectionOverlay::endNoteDrag()
{
    draggedNoteIndex_ = -1;
    isDragging_ = false;
}

//==============================================================================
std::unique_ptr<PitchCorrectionOverlay> PitchCorrectionOverlayFactory::create()
{
    return std::make_unique<PitchCorrectionOverlay>();
}

} // namespace ui
} // namespace zenith
