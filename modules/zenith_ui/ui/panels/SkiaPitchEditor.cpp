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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    SkiaPitchEditor.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    PROFESSIONAL Skia GPU pitch editor implementation.
    
    60fps, GPU-accelerated, beautiful.


  ==============================================================================
*/

#include "SkiaPitchEditor.h"
#include "../design-system/ZenithDesignSystem.h"
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>

namespace zenith {
namespace ui {

//==============================================================================
SkiaPitchEditor::SkiaPitchEditor()
{
    pitchDetector_ = std::make_unique<dsp::PitchDetector>();
    
    // Initialize Skia paints
    linePaint_.setAntiAlias(true);
    linePaint_.setStrokeCap(SkPaint::kRound_Cap);
    linePaint_.setStrokeJoin(SkPaint::kRound_Join);
    
    fillPaint_.setAntiAlias(true);
    
    glowPaint_.setAntiAlias(true);
    glowPaint_.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 10.0f));
    
    textPaint_.setAntiAlias(true);
    textPaint_.setColor(SK_ColorWHITE);
    
    setGlowEnabled(true);
    setGlowColor(noteSelectedColor_);
    setGlowRadius(15.0f);
}

SkiaPitchEditor::~SkiaPitchEditor()
{
}

//==============================================================================
void SkiaPitchEditor::drawSkia(SkCanvas* canvas)
{
    // Clear background
    canvas->clear(bgColor_);
    
    // Draw components in order
    drawGrid(canvas);
    drawWaveform(canvas);
    drawPitchCurve(canvas);
    drawNotes(canvas);
    drawPlayhead(canvas);
}

//==============================================================================
void SkiaPitchEditor::drawBackground(SkCanvas* canvas)
{
    // Already cleared in drawSkia
}

void SkiaPitchEditor::drawGrid(SkCanvas* canvas)
{
    auto bounds = getLocalBounds();
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    
    // Time grid (vertical lines) - every beat
    linePaint_.setColor(gridColor_);
    linePaint_.setStrokeWidth(1.0f);
    
    double timeRange = viewEndTime_ - viewStartTime_;
    double beatInterval = 0.5;  // 1/2 second grid
    
    for (double t = viewStartTime_; t <= viewEndTime_; t += beatInterval)
    {
        float x = timeToX(t);
        if (x >= 0 && x <= width)
        {
            canvas->drawLine(x, 0, x, height, linePaint_);
        }
    }
    
    // Pitch grid (horizontal lines) - every semitone
    float pixelsPerSemitone = height / ((maxPitchHz_ - minPitchHz_) / 60.0f);  // ~60Hz per semitone avg
    
    for (float semitones = -24; semitones <= 24; semitones += 1)
    {
        float centerPitch = (minPitchHz_ + maxPitchHz_) / 2.0f;
        float pitchHz = centerPitch * std::pow(2.0f, semitones / 12.0f);
        
        if (pitchHz >= minPitchHz_ && pitchHz <= maxPitchHz_)
        {
            float y = pitchToY(pitchHz);
            
            // Highlight C notes
            int noteClass = (static_cast<int>(69 + semitones) % 12 + 12) % 12;
            if (noteClass == 0)  // C
            {
                linePaint_.setColor(SkColorSetRGB(80, 80, 80));
                linePaint_.setStrokeWidth(2.0f);
                
                // Draw note name
                textPaint_.setTextSize(10);
                std::string noteName = "C" + std::to_string(static_cast<int>(69 + semitones) / 12 - 1);
                canvas->drawString(noteName.c_str(), 5, y + 3, labelFont_, textPaint_);
            }
            else
            {
                linePaint_.setColor(gridColor_);
                linePaint_.setStrokeWidth(1.0f);
            }
            
            canvas->drawLine(0, y, width, y, linePaint_);
        }
    }
}

void SkiaPitchEditor::drawWaveform(SkCanvas* canvas)
{
    if (waveformCache_.empty())
        return;
    
    auto bounds = getLocalBounds();
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float centerY = height / 2.0f;
    
    // Build waveform path
    SkPath wavePath;
    bool first = true;
    
    float xStep = width / static_cast<float>(waveformCache_.size());
    
    for (size_t i = 0; i < waveformCache_.size(); ++i)
    {
        float x = i * xStep;
        float sample = waveformCache_[i];
        float y = centerY + sample * centerY * 0.8f;  // Scale to 80% height
        
        if (first)
        {
            wavePath.moveTo(x, y);
            first = false;
        }
        else
        {
            wavePath.lineTo(x, y);
        }
    }
    
    // Draw with glow effect
    linePaint_.setColor(waveColor_);
    linePaint_.setStrokeWidth(1.5f);
    linePaint_.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
    canvas->drawPath(wavePath, linePaint_);
    linePaint_.setMaskFilter(nullptr);
}

void SkiaPitchEditor::drawPitchCurve(SkCanvas* canvas)
{
    if (pitchPoints_.empty())
        return;
    
    // Build pitch path
    SkPath pitchPath;
    bool inCurve = false;
    
    for (const auto& point : pitchPoints_)
    {
        if (!point.pitchHz || point.pitchHz < minPitchHz_ || point.pitchHz > maxPitchHz_)
        {
            inCurve = false;
            continue;
        }
        
        float x = timeToX(point.time);
        float y = pitchToY(point.pitchHz);
        
        if (!inCurve)
        {
            pitchPath.moveTo(x, y);
            inCurve = true;
        }
        else
        {
            pitchPath.lineTo(x, y);
        }
    }
    
    // Draw with glow
    drawSmoothPath(canvas, pitchPath, pitchColor_, 2.0f, true);
}

void SkiaPitchEditor::drawNotes(SkCanvas* canvas)
{
    for (size_t i = 0; i < notes_.size(); ++i)
    {
        const auto& note = notes_[i];
        
        // Skip if out of view
        if (note.endTime < viewStartTime_ || note.startTime > viewEndTime_)
            continue;
        
        // Determine color
        SkColor color = note.isSelected ? noteSelectedColor_ : noteColor_;
        if (note.isDragging)
        {
            color = SkColorSetRGB(255, 255, 100);  // Yellow while dragging
        }
        
        // Draw note block with glow
        drawGlowRect(canvas, note.screenRect, color);
        
        // Draw pitch correction line if corrected
        if (note.isSelected)
        {
            float centerY = note.screenRect.centerY();
            linePaint_.setColor(SkColorSetRGB(255, 255, 255));
            linePaint_.setStrokeWidth(1.0f);
            linePaint_.setPathEffect(SkDashPathEffect::Make(new float[]{4, 4}, 2, 0));
            canvas->drawLine(note.screenRect.left() + 5, centerY, 
                           note.screenRect.right() - 5, centerY, linePaint_);
            linePaint_.setPathEffect(nullptr);
        }
        
        // Draw note name
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int octave = note.midiNote / 12 - 1;
        std::string label = noteNames[note.midiNote % 12] + std::to_string(octave);
        
        textPaint_.setColor(SK_ColorWHITE);
        textPaint_.setTextSize(11);
        canvas->drawString(label.c_str(), 
                          note.screenRect.left() + 5, 
                          note.screenRect.top() + 14, 
                          labelFont_, textPaint_);
    }
}

void SkiaPitchEditor::drawPlayhead(SkCanvas* canvas)
{
    if (playheadTime_ < viewStartTime_ || playheadTime_ > viewEndTime_)
        return;
    
    float x = timeToX(playheadTime_);
    auto bounds = getLocalBounds();
    
    // Draw playhead line
    linePaint_.setColor(playheadColor_);
    linePaint_.setStrokeWidth(2.0f);
    linePaint_.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    canvas->drawLine(x, 0, x, bounds.getHeight(), linePaint_);
    linePaint_.setMaskFilter(nullptr);
    
    // Draw triangle at top
    SkPath triangle;
    triangle.moveTo(x, 0);
    triangle.lineTo(x - 8, 0);
    triangle.lineTo(x, 10);
    triangle.lineTo(x + 8, 0);
    triangle.close();
    
    fillPaint_.setColor(playheadColor_);
    canvas->drawPath(triangle, fillPaint_);
}

//==============================================================================
void SkiaPitchEditor::drawSmoothPath(SkCanvas* canvas, const SkPath& path, 
                                      SkColor color, float width, bool glow)
{
    if (glow)
    {
        // Draw glow first
        linePaint_.setColor(color);
        linePaint_.setStrokeWidth(width * 3);
        linePaint_.setAlpha(80);
        linePaint_.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
        canvas->drawPath(path, linePaint_);
        linePaint_.setMaskFilter(nullptr);
        linePaint_.setAlpha(255);
    }
    
    // Draw main line
    linePaint_.setColor(color);
    linePaint_.setStrokeWidth(width);
    canvas->drawPath(path, linePaint_);
}

void SkiaPitchEditor::drawGlowRect(SkCanvas* canvas, const SkRect& rect, SkColor color)
{
    // Outer glow
    fillPaint_.setColor(color);
    fillPaint_.setAlpha(60);
    fillPaint_.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 10.0f));
    canvas->drawRect(rect, fillPaint_);
    fillPaint_.setMaskFilter(nullptr);
    fillPaint_.setAlpha(255);
    
    // Main rect
    fillPaint_.setColor(SkColorSetA(color, 180));
    canvas->drawRect(rect, fillPaint_);
    
    // Border
    linePaint_.setColor(color);
    linePaint_.setStrokeWidth(2.0f);
    canvas->drawRect(rect, linePaint_);
}

//==============================================================================
// Coordinate conversion
float SkiaPitchEditor::timeToX(double time) const
{
    auto bounds = getLocalBounds();
    double timeRange = viewEndTime_ - viewStartTime_;
    if (timeRange <= 0) return 0;
    
    return static_cast<float>((time - viewStartTime_) / timeRange * bounds.getWidth());
}

double SkiaPitchEditor::xToTime(float x) const
{
    auto bounds = getLocalBounds();
    double timeRange = viewEndTime_ - viewStartTime_;
    double normalized = x / bounds.getWidth();
    return viewStartTime_ + normalized * timeRange;
}

float SkiaPitchEditor::pitchToY(float pitchHz) const
{
    auto bounds = getLocalBounds();
    float logMin = std::log2(minPitchHz_);
    float logMax = std::log2(maxPitchHz_);
    float logPitch = std::log2(pitchHz);
    
    float normalized = (logPitch - logMin) / (logMax - logMin);
    return bounds.getHeight() * (1.0f - normalized);  // Invert Y
}

float SkiaPitchEditor::yToPitch(float y) const
{
    auto bounds = getLocalBounds();
    float normalized = 1.0f - (y / bounds.getHeight());
    
    float logMin = std::log2(minPitchHz_);
    float logMax = std::log2(maxPitchHz_);
    float logPitch = logMin + normalized * (logMax - logMin);
    
    return std::pow(2.0f, logPitch);
}

//==============================================================================
void SkiaPitchEditor::onResize()
{
    // Recalculate screen positions
    for (auto& note : notes_)
    {
        note.screenRect = SkRect::MakeLTRB(
            timeToX(note.startTime),
            pitchToY(note.pitchHz) - 10,
            timeToX(note.endTime),
            pitchToY(note.pitchHz) + 10
        );
    }
    
    markDirty();
}

//==============================================================================
void SkiaPitchEditor::onMouseDown(const juce::MouseEvent& event)
{
    lastMousePos_ = event.position;
    
    if (currentTool_ == Tool::Hand)
    {
        isPanning_ = true;
        return;
    }
    
    if (currentTool_ == Tool::Pointer)
    {
        startDrag(event);
    }
}

void SkiaPitchEditor::onMouseDrag(const juce::MouseEvent& event)
{
    if (isPanning_)
    {
        float deltaX = event.position.x - lastMousePos_.x;
        handlePan(deltaX, 0);
        lastMousePos_ = event.position;
        return;
    }
    
    if (isDragging_)
    {
        continueDrag(event);
    }
}

void SkiaPitchEditor::onMouseUp(const juce::MouseEvent& /*event*/)
{
    if (isDragging_)
    {
        endDrag();
    }
    isPanning_ = false;
}

void SkiaPitchEditor::mouseWheelMove(const juce::MouseEvent& event, 
                                      const juce::MouseWheelDetails& wheel)
{
    if (wheel.isSmooth)
    {
        // Pan horizontally
        handlePan(-wheel.deltaX * 50, 0);
    }
    else
    {
        // Zoom
        float zoomFactor = (wheel.deltaY > 0) ? 0.9f : 1.1f;
        handleZoom(zoomFactor, event.position.x);
    }
}

//==============================================================================
void SkiaPitchEditor::startDrag(const juce::MouseEvent& event)
{
    int noteIndex = getNoteAt(event.position.x, event.position.y);
    
    if (noteIndex >= 0)
    {
        // Deselect all
        for (auto& note : notes_)
            note.isSelected = false;
        
        // Select and start dragging
        notes_[noteIndex].isSelected = true;
        notes_[noteIndex].isDragging = true;
        draggedNoteIndex_ = noteIndex;
        dragStartPos_ = event.position;
        dragStartNoteTime_ = notes_[noteIndex].startTime;
        isDragging_ = true;
        
        markDirty();
    }
}

void SkiaPitchEditor::continueDrag(const juce::MouseEvent& event)
{
    if (draggedNoteIndex_ < 0)
        return;
    
    auto& note = notes_[draggedNoteIndex_];
    
    float deltaX = event.position.x - dragStartPos_.x;
    double timeDelta = (deltaX / getLocalBounds().getWidth()) * (viewEndTime_ - viewStartTime_);
    
    double newStart = dragStartNoteTime_ + timeDelta;
    double duration = note.endTime - note.startTime;
    
    note.startTime = newStart;
    note.endTime = newStart + duration;
    
    // Update screen rect
    note.screenRect = SkRect::MakeLTRB(
        timeToX(note.startTime),
        note.screenRect.top(),
        timeToX(note.endTime),
        note.screenRect.bottom()
    );
    
    markDirty();
}

void SkiaPitchEditor::endDrag()
{
    if (draggedNoteIndex_ >= 0)
    {
        notes_[draggedNoteIndex_].isDragging = false;
        draggedNoteIndex_ = -1;
    }
    isDragging_ = false;
}

void SkiaPitchEditor::handleZoom(float factor, float centerX)
{
    double timeRange = viewEndTime_ - viewStartTime_;
    double centerTime = xToTime(centerX);
    
    double newRange = timeRange * factor;
    viewStartTime_ = centerTime - (centerTime - viewStartTime_) * factor;
    viewEndTime_ = viewStartTime_ + newRange;
    
    viewStartTime_ = juce::jmax(0.0, viewStartTime_);
    
    onResize();  // Recalculate positions
}

void SkiaPitchEditor::handlePan(float deltaX, float /*deltaY*/)
{
    double timeRange = viewEndTime_ - viewStartTime_;
    double timeDelta = -(deltaX / getLocalBounds().getWidth()) * timeRange;
    
    viewStartTime_ += timeDelta;
    viewEndTime_ += timeDelta;
    
    viewStartTime_ = juce::jmax(0.0, viewStartTime_);
    
    onResize();
}

//==============================================================================
int SkiaPitchEditor::getNoteAt(float x, float y) const
{
    for (int i = 0; i < static_cast<int>(notes_.size()); ++i)
    {
        if (notes_[i].screenRect.contains(x, y))
            return i;
    }
    return -1;
}

//==============================================================================
void SkiaPitchEditor::loadAudio(const juce::AudioBuffer<float>& audio, double sampleRate)
{
    audioBuffer_.makeCopyOf(audio);
    sampleRate_ = sampleRate;
    audioDuration_ = audio.getNumSamples() / sampleRate;
    
    // Generate waveform overview
    int overviewSize = 2000;  // Samples in overview
    waveformCache_.resize(overviewSize);
    
    int samplesPerOverviewSample = audio.getNumSamples() / overviewSize;
    const float* data = audio.getReadPointer(0);
    
    for (int i = 0; i < overviewSize; ++i)
    {
        float maxSample = 0.0f;
        int startSample = i * samplesPerOverviewSample;
        int endSample = std::min(startSample + samplesPerOverviewSample, audio.getNumSamples());
        
        for (int s = startSample; s < endSample; ++s)
        {
            maxSample = std::max(maxSample, std::abs(data[s]));
        }
        
        waveformCache_[i] = maxSample;
    }
    
    analyzePitch();
    zoomToFit();
}

void SkiaPitchEditor::analyzePitch()
{
    pitchPoints_.clear();
    notes_.clear();
    
    if (audioBuffer_.getNumSamples() == 0)
        return;
    
    // Analyze pitch
    pitchDetector_->prepare(sampleRate_, 2048);
    
    const int hopSize = 512;
    juce::AudioBuffer<float> block(1, 2048);
    
    for (int pos = 0; pos + 2048 < audioBuffer_.getNumSamples(); pos += hopSize)
    {
        block.copyFrom(0, 0, audioBuffer_, 0, pos, 2048);
        
        float pitch = pitchDetector_->processBlock(block);
        
        PitchDataPoint point;
        point.time = pos / sampleRate_;
        point.pitchHz = pitch;
        point.confidence = pitchDetector_->getConfidence();
        
        pitchPoints_.push_back(point);
    }
    
    // Simple note detection from pitch points
    // Group consecutive pitched regions
    bool inNote = false;
    EditableNoteBlock currentNote;
    std::vector<float> notePitches;
    
    for (const auto& point : pitchPoints_)
    {
        if (point.pitchHz > 0 && point.confidence > 0.6f)
        {
            if (!inNote)
            {
                inNote = true;
                currentNote.startTime = point.time;
                notePitches.clear();
            }
            notePitches.push_back(point.pitchHz);
        }
        else
        {
            if (inNote && notePitches.size() > 10)
            {
                currentNote.endTime = point.time;
                
                // Average pitch
                float sum = 0;
                for (float p : notePitches) sum += p;
                currentNote.pitchHz = sum / notePitches.size();
                
                // Calculate MIDI note
                currentNote.midiNote = static_cast<int>(69 + 12 * std::log2(currentNote.pitchHz / 440.0f) + 0.5f);
                
                // Initial screen rect (will be updated in onResize)
                currentNote.screenRect = SkRect::MakeEmpty();
                currentNote.color = noteColor_;
                
                notes_.push_back(currentNote);
            }
            inNote = false;
        }
    }
    
    markDirty();
}

void SkiaPitchEditor::clear()
{
    audioBuffer_.clear();
    waveformCache_.clear();
    pitchPoints_.clear();
    notes_.clear();
    markDirty();
}

//==============================================================================
void SkiaPitchEditor::zoomToFit()
{
    if (audioDuration_ > 0)
    {
        viewStartTime_ = 0;
        viewEndTime_ = audioDuration_;
    }
    else
    {
        viewStartTime_ = 0;
        viewEndTime_ = 10.0;
    }
    
    onResize();
}

void SkiaPitchEditor::zoomIn()
{
    handleZoom(0.8f, getLocalBounds().getWidth() / 2.0f);
}

void SkiaPitchEditor::zoomOut()
{
    handleZoom(1.25f, getLocalBounds().getWidth() / 2.0f);
}

void SkiaPitchEditor::setViewRange(double startTime, double endTime)
{
    viewStartTime_ = startTime;
    viewEndTime_ = endTime;
    onResize();
}

void SkiaPitchEditor::scrollToTime(double time)
{
    double range = viewEndTime_ - viewStartTime_;
    viewStartTime_ = time - range / 2.0;
    viewEndTime_ = viewStartTime_ + range;
    onResize();
}

//==============================================================================
void SkiaPitchEditor::setPlayhead(double time)
{
    playheadTime_ = time;
    markDirty();
}

void SkiaPitchEditor::setTool(Tool tool)
{
    currentTool_ = tool;
}

} // namespace ui
} // namespace zenith
