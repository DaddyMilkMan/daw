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

    PitchGraphEditor.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Graph Mode pitch editor implementation.

  ==============================================================================

*/

#include "PitchGraphEditor.h"
#include "../../design-system/ZenithTheme.h"
#include <algorithm>

namespace zenith {
namespace ui {

//==============================================================================
// GraphNote implementation
juce::Rectangle<float> GraphNote::getBounds(double pixelsPerSecond, float pixelsPerSemitone, 
                                              float centerPitch) const
{
    float x = static_cast<float>(startTime * pixelsPerSecond);
    float width = static_cast<float>((endTime - startTime) * pixelsPerSecond);
    
    float semitonesFromCenter = 12.0f * std::log2(targetPitchHz / centerPitch);
    float y = 200.0f - semitonesFromCenter * pixelsPerSemitone;  // Center is at y=200
    float height = 20.0f;  // Fixed height for note blocks
    
    return { x, y - height/2, width, height };
}

//==============================================================================
PitchGraphEditor::PitchGraphEditor()
{
    pitchDetector_ = std::make_unique<dsp::PitchDetector>();
    
    startTimerHz(30);  // 30fps refresh for playhead
}

PitchGraphEditor::~PitchGraphEditor()
{
    stopTimer();
}

//==============================================================================
void PitchGraphEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(juce::Colours::black);
    
    // Grid
    if (showGrid_)
        drawGrid(g);
    
    // Pitch curve
    if (showPitchCurve_)
        drawPitchCurve(g);
    
    // Notes
    if (showNoteBlocks_)
        drawNotes(g);
    
    // Playhead
    drawPlayhead(g);
}

void PitchGraphEditor::resized()
{
    repaint();
}

//==============================================================================
void PitchGraphEditor::drawGrid(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Time grid (vertical lines)
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
    
    double pixelsPerSecond = bounds.getWidth() / (viewEndTime_ - viewStartTime_);
    double gridInterval = 60.0 / (beatsPerBar_ * gridDivisions_);  // Assuming 60 BPM for now
    
    for (double t = viewStartTime_; t <= viewEndTime_; t += gridInterval)
    {
        float x = timeToX(t);
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }
    
    // Pitch grid (horizontal lines - semitones)
    g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
    
    float pixelsPerSemitone = bounds.getHeight() / semitoneRange_;
    
    for (int i = -12; i <= 12; ++i)
    {
        float y = semitonesToY(static_cast<float>(i));
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
        
        // Note names for C notes
        if (i % 12 == 0)
        {
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.setFont(10.0f);
            g.drawText("C" + juce::String(i/12 + 4), 5, static_cast<int>(y) - 6, 30, 12, 
                       juce::Justification::left);
            g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
        }
    }
}

void PitchGraphEditor::drawPitchCurve(juce::Graphics& g)
{
    if (pitchCurve_.empty())
        return;
    
    auto bounds = getLocalBounds().toFloat();
    
    // Draw detected pitch curve
    juce::Path pitchPath;
    bool firstPoint = true;
    
    float pixelsPerSecond = bounds.getWidth() / (viewEndTime_ - viewStartTime_);
    float pixelsPerSemitone = bounds.getHeight() / semitoneRange_;
    
    double timePerFrame = 0.01;  // 10ms per frame (simplified)
    
    for (size_t i = 0; i < pitchCurve_.size(); ++i)
    {
        float pitch = pitchCurve_[i];
        if (pitch <= 0.0f)
            continue;  // No pitch detected
        
        double time = i * timePerFrame;
        if (time < viewStartTime_ || time > viewEndTime_)
            continue;
        
        float x = timeToX(time);
        float y = pitchToY(pitch);
        
        if (firstPoint)
        {
            pitchPath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            pitchPath.lineTo(x, y);
        }
    }
    
    g.setColour(juce::Colours::lime.withAlpha(0.5f));
    g.strokePath(pitchPath, juce::PathStrokeType(1.5f));
}

void PitchGraphEditor::drawNotes(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float pixelsPerSecond = bounds.getWidth() / (viewEndTime_ - viewStartTime_);
    float pixelsPerSemitone = bounds.getHeight() / semitoneRange_;
    
    for (size_t i = 0; i < notes_.size(); ++i)
    {
        const auto& note = notes_[i];
        
        auto noteBounds = note.getBounds(pixelsPerSecond, pixelsPerSemitone, centerPitchHz_);
        
        // Intersect with visible area
        if (!noteBounds.intersects(bounds))
            continue;
        
        // Color based on correction/confidence
        juce::Colour noteColor;
        if (note.isBypassed)
        {
            noteColor = juce::Colours::grey;
        }
        else if (note.confidence > 0.8f)
        {
            noteColor = juce::Colours::cyan;
        }
        else if (note.confidence > 0.5f)
        {
            noteColor = juce::Colours::orange;
        }
        else
        {
            noteColor = juce::Colours::red;
        }
        
        // Selected notes are brighter
        if (note.isSelected)
        {
            noteColor = noteColor.brighter(0.3f);
        }
        
        // Draw note block
        g.setColour(noteColor.withAlpha(0.6f));
        g.fillRect(noteBounds);
        
        g.setColour(noteColor);
        g.drawRect(noteBounds, 1.5f);
        
        // Draw note name
        g.setColour(juce::Colours::white);
        g.setFont(11.0f);
        
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        juce::String noteName = noteNames[note.midiNote % 12];
        juce::String octave = juce::String(note.midiNote / 12 - 1);
        
        g.drawText(noteName + octave, noteBounds.reduced(4), juce::Justification::centredLeft);
        
        // Draw correction indicator if corrected
        if (std::abs(note.targetPitchHz - note.pitchHz) > 1.0f)
        {
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            float centerY = noteBounds.getCentreY();
            float originalY = pitchToY(note.pitchHz);
            g.drawLine(noteBounds.getX() + 4, originalY, noteBounds.getRight() - 4, originalY, 1.0f);
        }
    }
}

void PitchGraphEditor::drawPlayhead(juce::Graphics& g)
{
    if (playheadTime_ < viewStartTime_ || playheadTime_ > viewEndTime_)
        return;
    
    float x = timeToX(playheadTime_);
    
    g.setColour(juce::Colours::white);
    g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(getHeight()));
    
    // Playhead triangle
    juce::Path triangle;
    triangle.addTriangle(x - 6, 0.0f, x + 6, 0.0f, x, 8.0f);
    g.fillPath(triangle);
}

//==============================================================================
void PitchGraphEditor::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.position;
    
    switch (editMode_)
    {
        case EditMode::Select:
        {
            int noteIndex = getNoteAtPosition(pos);
            if (noteIndex >= 0)
            {
                if (!event.mods.isShiftDown())
                    setAllNotesSelected(false);
                
                setNoteSelected(noteIndex, true);
                startDraggingNote(noteIndex, pos);
            }
            else
            {
                // Clicked on empty space
                if (!event.mods.isShiftDown())
                    setAllNotesSelected(false);
            }
            break;
        }
        
        case EditMode::DrawPitch:
        {
            // Start drawing pitch curve
            break;
        }
        
        case EditMode::Split:
        {
            int noteIndex = getNoteAtPosition(pos);
            if (noteIndex >= 0)
            {
                double time = xToTime(pos.x);
                splitNoteAt(noteIndex, time);
            }
            break;
        }
        
        case EditMode::Erase:
        {
            int noteIndex = getNoteAtPosition(pos);
            if (noteIndex >= 0)
            {
                deleteNote(noteIndex);
            }
            break;
        }
    }
}

void PitchGraphEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (editMode_ == EditMode::Select && isDragging_)
    {
        dragNote(event.position);
    }
}

void PitchGraphEditor::mouseUp(const juce::MouseEvent& /*event*/)
{
    if (isDragging_)
    {
        finishDraggingNote();
    }
}

void PitchGraphEditor::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Double-click to edit note parameters (could open a dialog)
    int noteIndex = getNoteAtPosition(event.position);
    if (noteIndex >= 0)
    {
        // For now, just select it
        setAllNotesSelected(false);
        setNoteSelected(noteIndex, true);
    }
}

void PitchGraphEditor::mouseWheelMove(const juce::MouseEvent& /*event*/, 
                                       const juce::MouseWheelDetails& wheel)
{
    // Zoom time axis
    double zoomFactor = (wheel.deltaY > 0) ? 0.9 : 1.1;
    
    double range = viewEndTime_ - viewStartTime_;
    double center = (viewStartTime_ + viewEndTime_) / 2.0;
    double newRange = range * zoomFactor;
    
    viewStartTime_ = center - newRange / 2.0;
    viewEndTime_ = center + newRange / 2.0;
    
    viewStartTime_ = juce::jmax(0.0, viewStartTime_);
    
    repaint();
}

//==============================================================================
void PitchGraphEditor::analyzeAudio(const juce::AudioBuffer<float>& audio, double sampleRate)
{
    sampleRate_ = sampleRate;
    audioDuration_ = audio.getNumSamples() / sampleRate;
    
    // Clear previous analysis
    pitchCurve_.clear();
    notes_.clear();
    
    // Prepare pitch detector
    pitchDetector_->prepare(sampleRate, 2048);
    
    // Analyze in blocks
    const int blockSize = 2048;
    const int hopSize = 512;  // 75% overlap
    
    juce::AudioBuffer<float> blockBuffer(1, blockSize);
    
    for (int pos = 0; pos + blockSize < audio.getNumSamples(); pos += hopSize)
    {
        // Fill buffer
        blockBuffer.copyFrom(0, 0, audio, 0, pos, blockSize);
        
        // Detect pitch
        float pitch = pitchDetector_->processBlock(blockBuffer);
        pitchCurve_.push_back(pitch);
    }
    
    // Detect notes from pitch curve
    detectNotesFromPitchCurve();
    
    // Auto-fit view
    zoomToFit();
    
    repaint();
}

void PitchGraphEditor::detectNotesFromPitchCurve()
{
    notes_.clear();
    
    if (pitchCurve_.empty())
        return;
    
    // Simple note detection - group consecutive pitched frames
    const float minConfidence = 0.5f;
    const int minFrames = 10;  // Minimum note duration
    
    bool inNote = false;
    int noteStart = 0;
    std::vector<float> notePitches;
    
    for (size_t i = 0; i < pitchCurve_.size(); ++i)
    {
        float pitch = pitchCurve_[i];
        bool isVoiced = pitch > 0.0f;
        
        if (!inNote && isVoiced)
        {
            // Start of note
            inNote = true;
            noteStart = static_cast<int>(i);
            notePitches.clear();
            notePitches.push_back(pitch);
        }
        else if (inNote && isVoiced)
        {
            // Continue note
            notePitches.push_back(pitch);
        }
        else if (inNote && !isVoiced)
        {
            // End of note
            if (static_cast<int>(notePitches.size()) >= minFrames)
            {
                GraphNote note;
                
                double timePerFrame = 0.01;  // Simplified
                note.startTime = noteStart * timePerFrame;
                note.endTime = i * timePerFrame;
                
                // Average pitch
                float sum = 0.0f;
                for (float p : notePitches) sum += p;
                note.pitchHz = sum / notePitches.size();
                note.targetPitchHz = note.pitchHz;  // Start with no correction
                
                // Calculate confidence (pitch stability)
                float variance = 0.0f;
                for (float p : notePitches)
                    variance += (p - note.pitchHz) * (p - note.pitchHz);
                variance /= notePitches.size();
                note.confidence = juce::jlimit(0.0f, 1.0f, 1.0f - variance / 100.0f);
                
                // Calculate MIDI note
                note.midiNote = static_cast<int>(69 + 12 * std::log2(note.pitchHz / 440.0f) + 0.5f);
                
                notes_.push_back(note);
            }
            
            inNote = false;
        }
    }
}

void PitchGraphEditor::clearAnalysis()
{
    pitchCurve_.clear();
    notes_.clear();
    repaint();
}

//==============================================================================
void PitchGraphEditor::addNote(const GraphNote& note)
{
    notes_.push_back(note);
    repaint();
}

void PitchGraphEditor::deleteNote(int index)
{
    if (index >= 0 && index < static_cast<int>(notes_.size()))
    {
        notes_.erase(notes_.begin() + index);
        repaint();
    }
}

void PitchGraphEditor::updateNotePitch(int index, float newPitchHz)
{
    if (index >= 0 && index < static_cast<int>(notes_.size()))
    {
        notes_[index].targetPitchHz = newPitchHz;
        repaint();
    }
}

void PitchGraphEditor::updateNoteTiming(int index, double newStart, double newEnd)
{
    if (index >= 0 && index < static_cast<int>(notes_.size()))
    {
        notes_[index].startTime = newStart;
        notes_[index].endTime = newEnd;
        repaint();
    }
}

void PitchGraphEditor::setNoteSelected(int index, bool selected)
{
    if (index >= 0 && index < static_cast<int>(notes_.size()))
    {
        notes_[index].isSelected = selected;
        repaint();
    }
}

void PitchGraphEditor::setAllNotesSelected(bool selected)
{
    for (auto& note : notes_)
        note.isSelected = selected;
    repaint();
}

//==============================================================================
void PitchGraphEditor::setNoteCorrectionAmount(int index, float amount)
{
    if (index >= 0 && index < static_cast<int>(notes_.size()))
    {
        notes_[index].correctionAmount = amount;
    }
}

void PitchGraphEditor::setNoteFormantShift(int index, float semitones)
{
    if (index >= 0 && index < static_cast<int>(notes_.size()))
    {
        notes_[index].formantShift = semitones;
    }
}

void PitchGraphEditor::setNoteGain(int index, float gain)
{
    if (index >= 0 && index < static_cast<int>(notes_.size()))
    {
        notes_[index].gain = gain;
    }
}

void PitchGraphEditor::setNoteBypass(int index, bool bypass)
{
    if (index >= 0 && index < static_cast<int>(notes_.size()))
    {
        notes_[index].isBypassed = bypass;
        repaint();
    }
}

//==============================================================================
void PitchGraphEditor::setViewRange(double startTime, double endTime)
{
    viewStartTime_ = startTime;
    viewEndTime_ = endTime;
    repaint();
}

void PitchGraphEditor::setPitchRange(float centerPitch, float semitoneRange)
{
    centerPitchHz_ = centerPitch;
    semitoneRange_ = semitoneRange;
    repaint();
}

void PitchGraphEditor::zoomToFit()
{
    if (notes_.empty())
    {
        viewStartTime_ = 0.0;
        viewEndTime_ = audioDuration_ > 0.0 ? audioDuration_ : 10.0;
    }
    else
    {
        viewStartTime_ = notes_.front().startTime - 0.5;
        viewEndTime_ = notes_.back().endTime + 0.5;
        
        viewStartTime_ = juce::jmax(0.0, viewStartTime_);
    }
    
    repaint();
}

void PitchGraphEditor::zoomIn()
{
    double range = viewEndTime_ - viewStartTime_;
    setViewRange(viewStartTime_ + range * 0.1, viewEndTime_ - range * 0.1);
}

void PitchGraphEditor::zoomOut()
{
    double range = viewEndTime_ - viewStartTime_;
    setViewRange(viewStartTime_ - range * 0.1, viewEndTime_ + range * 0.1);
}

void PitchGraphEditor::scrollToTime(double time)
{
    double range = viewEndTime_ - viewStartTime_;
    setViewRange(time - range / 2.0, time + range / 2.0);
}

//==============================================================================
void PitchGraphEditor::setPlayheadPosition(double time)
{
    playheadTime_ = time;
    // Don't repaint on every playhead update - timer handles that
}

void PitchGraphEditor::timerCallback()
{
    // Update for playhead animation
    repaint();
}

//==============================================================================
// Coordinate conversion
float PitchGraphEditor::timeToX(double time) const
{
    auto bounds = getLocalBounds().toFloat();
    double pixelsPerSecond = bounds.getWidth() / (viewEndTime_ - viewStartTime_);
    return static_cast<float>((time - viewStartTime_) * pixelsPerSecond);
}

double PitchGraphEditor::xToTime(float x) const
{
    auto bounds = getLocalBounds().toFloat();
    double pixelsPerSecond = bounds.getWidth() / (viewEndTime_ - viewStartTime_);
    return viewStartTime_ + x / pixelsPerSecond;
}

float PitchGraphEditor::pitchToY(float pitchHz) const
{
    auto bounds = getLocalBounds().toFloat();
    float centerY = bounds.getCentreY();
    
    float semitones = 12.0f * std::log2(pitchHz / centerPitchHz_);
    float pixelsPerSemitone = bounds.getHeight() / semitoneRange_;
    
    return centerY - semitones * pixelsPerSemitone;
}

float PitchGraphEditor::semitonesToY(float semitones) const
{
    auto bounds = getLocalBounds().toFloat();
    float centerY = bounds.getCentreY();
    float pixelsPerSemitone = bounds.getHeight() / semitoneRange_;
    
    return centerY - semitones * pixelsPerSemitone;
}

float PitchGraphEditor::yToPitch(float y) const
{
    auto bounds = getLocalBounds().toFloat();
    float centerY = bounds.getCentreY();
    float pixelsPerSemitone = bounds.getHeight() / semitoneRange_;
    
    float semitones = (centerY - y) / pixelsPerSemitone;
    return centerPitchHz_ * std::pow(2.0f, semitones / 12.0f);
}

//==============================================================================
int PitchGraphEditor::getNoteAtPosition(juce::Point<float> pos) const
{
    auto bounds = getLocalBounds().toFloat();
    float pixelsPerSecond = bounds.getWidth() / (viewEndTime_ - viewStartTime_);
    float pixelsPerSemitone = bounds.getHeight() / semitoneRange_;
    
    for (int i = 0; i < static_cast<int>(notes_.size()); ++i)
    {
        auto noteBounds = notes_[i].getBounds(pixelsPerSecond, pixelsPerSemitone, centerPitchHz_);
        if (noteBounds.contains(pos))
            return i;
    }
    
    return -1;
}

void PitchGraphEditor::startDraggingNote(int noteIndex, juce::Point<float> startPos)
{
    draggingNoteIndex_ = noteIndex;
    dragStartPos_ = startPos;
    isDragging_ = true;
    
    dragStartTime_ = notes_[noteIndex].startTime;
    dragStartPitch_ = notes_[noteIndex].targetPitchHz;
}

void PitchGraphEditor::dragNote(juce::Point<float> currentPos)
{
    if (draggingNoteIndex_ < 0)
        return;
    
    auto& note = notes_[draggingNoteIndex_];
    
    // Calculate delta
    float deltaX = currentPos.x - dragStartPos_.x;
    float deltaY = currentPos.y - dragStartPos_.y;
    
    auto bounds = getLocalBounds().toFloat();
    float pixelsPerSecond = bounds.getWidth() / (viewEndTime_ - viewStartTime_);
    
    // Time shift
    double timeDelta = deltaX / pixelsPerSecond;
    double newStart = dragStartTime_ + timeDelta;
    double duration = note.endTime - note.startTime;
    
    if (snapToGrid_)
    {
        newStart = snapTimeToGrid(newStart);
    }
    
    note.startTime = newStart;
    note.endTime = newStart + duration;
    
    // Pitch shift
    float newPitch = yToPitch(dragStartPos_.y + deltaY);
    
    if (snapToScale_)
    {
        newPitch = snapPitchToScale(newPitch);
    }
    
    note.targetPitchHz = newPitch;
    
    repaint();
}

void PitchGraphEditor::finishDraggingNote()
{
    isDragging_ = false;
    draggingNoteIndex_ = -1;
    
    // Notify listeners that notes changed
    sendChangeMessage();
}

void PitchGraphEditor::splitNoteAt(int noteIndex, double time)
{
    if (noteIndex < 0 || noteIndex >= static_cast<int>(notes_.size()))
        return;
    
    auto& note = notes_[noteIndex];
    
    if (time <= note.startTime || time >= note.endTime)
        return;
    
    // Create second half
    GraphNote secondHalf = note;
    secondHalf.startTime = time;
    
    // Shorten first half
    note.endTime = time;
    
    // Insert second half
    notes_.insert(notes_.begin() + noteIndex + 1, secondHalf);
    
    repaint();
}

float PitchGraphEditor::snapPitchToScale(float pitchHz) const
{
    // Simple snap to nearest semitone
    float semitones = 12.0f * std::log2(pitchHz / 440.0f);
    int rounded = static_cast<int>(std::round(semitones));
    return 440.0f * std::pow(2.0f, rounded / 12.0f);
}

float PitchGraphEditor::snapTimeToGrid(double time) const
{
    double gridInterval = 60.0 / (beatsPerBar_ * gridDivisions_);
    return std::round(time / gridInterval) * gridInterval;
}

//==============================================================================
void PitchGraphEditor::applyCorrectionsToBuffer(juce::AudioBuffer<float>& /*buffer*/, 
                                                  double /*sampleRate*/)
{
    // Apply the graph edits to the audio buffer
    // This would process each note with the pitch corrector
    // Implementation depends on how you want to integrate with the engine
}

juce::ValueTree PitchGraphEditor::exportToValueTree() const
{
    juce::ValueTree tree("PitchGraph");
    
    for (const auto& note : notes_)
    {
        juce::ValueTree noteTree("Note");
        noteTree.setProperty("start", note.startTime, nullptr);
        noteTree.setProperty("end", note.endTime, nullptr);
        noteTree.setProperty("pitch", note.pitchHz, nullptr);
        noteTree.setProperty("targetPitch", note.targetPitchHz, nullptr);
        noteTree.setProperty("confidence", note.confidence, nullptr);
        noteTree.setProperty("midiNote", note.midiNote, nullptr);
        noteTree.setProperty("correctionAmount", note.correctionAmount, nullptr);
        noteTree.setProperty("formantShift", note.formantShift, nullptr);
        noteTree.setProperty("gain", note.gain, nullptr);
        noteTree.setProperty("bypass", note.isBypassed, nullptr);
        
        tree.appendChild(noteTree, nullptr);
    }
    
    return tree;
}

void PitchGraphEditor::importFromValueTree(const juce::ValueTree& tree)
{
    notes_.clear();
    
    if (!tree.hasType("PitchGraph"))
        return;
    
    for (const auto& noteTree : tree)
    {
        if (!noteTree.hasType("Note"))
            continue;
        
        GraphNote note;
        note.startTime = noteTree.getProperty("start", 0.0);
        note.endTime = noteTree.getProperty("end", 0.0);
        note.pitchHz = noteTree.getProperty("pitch", 0.0f);
        note.targetPitchHz = noteTree.getProperty("targetPitch", note.pitchHz);
        note.confidence = noteTree.getProperty("confidence", 0.0f);
        note.midiNote = noteTree.getProperty("midiNote", 0);
        note.correctionAmount = noteTree.getProperty("correctionAmount", 1.0f);
        note.formantShift = noteTree.getProperty("formantShift", 0.0f);
        note.gain = noteTree.getProperty("gain", 1.0f);
        note.isBypassed = noteTree.getProperty("bypass", false);
        
        notes_.push_back(note);
    }
    
    repaint();
}

} // namespace ui
} // namespace zenith
