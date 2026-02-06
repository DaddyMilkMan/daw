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


 * @file SampleEditorInput.cpp
 * @brief Interaction handlers for SampleEditorComponent (Mouse/Keyboard/Navigation)


void SampleEditorComponent::mouseDown(const juce::MouseEvent& e) {
    if (isInToolbar(e.position.y)) {
        // Handle toolbar clicks
    } else if (isInOverview(e.position.y)) {
        isDraggingOverview_ = true;
        setPlayheadPosition(pixelsToTime(e.position.x, (float)getWidth()));
    } else if (isInRuler(e.position.y)) {
        isDraggingPlayhead_ = true;
        setPlayheadPosition(pixelsToTime(e.position.x, (float)getWidth()));
    } else if (isInWaveform(e.position.y)) {
        if (currentTool_ == SampleEditorTool::Select) {
            isSelecting_ = true;
            selectionAnchor_ = pixelsToTime(e.position.x, (float)getWidth());
            setSelection(selectionAnchor_, selectionAnchor_);
        } else if (currentTool_ == SampleEditorTool::Pencil) {
            pencilDraw(e.position.x, e.position.y);
        }
    } else if (isInScrollbar(e.position.y)) {
        // Handle scrollbar
    }
    repaint();
}

void SampleEditorComponent::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingOverview_ || isDraggingPlayhead_) {
        setPlayheadPosition(pixelsToTime(e.position.x, (float)getWidth()));
    } else if (isSelecting_) {
        double currentTime = pixelsToTime(e.position.x, (float)getWidth());
        setSelection(std::min(selectionAnchor_, currentTime), std::max(selectionAnchor_, currentTime));
    } else if (currentTool_ == SampleEditorTool::Pencil && pencilToolEnabled_) {
        pencilDraw(e.position.x, e.position.y);
    }
    repaint();
}

void SampleEditorComponent::mouseUp(const juce::MouseEvent& e) {
    isDraggingOverview_ = false;
    isDraggingPlayhead_ = false;
    isSelecting_ = false;
    repaint();
}

void SampleEditorComponent::mouseMove(const juce::MouseEvent& e) {
    // Update cursor based on position
}

void SampleEditorComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    if (isInWaveform(e.position.y)) {
        selectAll();
    }
}

void SampleEditorComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (wheel.deltaY != 0) {
        float factor = (wheel.deltaY > 0) ? 1.1f : 0.9f;
        zoomHorizontal(factor, e.position.x);
    } else if (wheel.deltaX != 0) {
        scrollHorizontal(wheel.deltaX * 10.0f);
    }
}

bool SampleEditorComponent::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress(' ')) {
        if (isPlaying_) stop(); else play();
        return true;
    }
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        deleteSelection();
        return true;
    }
    // Shortcuts...
    return false;
}

//==============================================================================
// Zoom/Scroll
//==============================================================================

void SampleEditorComponent::zoomHorizontal(float factor, float centerX) {
    double centerTime = pixelsToTime(centerX, (float)getWidth());
    viewWidthSeconds_ /= factor;
    // Keep minimum view width
    viewWidthSeconds_ = std::max(0.001, viewWidthSeconds_);
    
    // Adjust offset to keep centerTime at centerX
    timeOffset_ = centerTime - (centerX / (float)getWidth() * viewWidthSeconds_);
    timeOffset_ = std::max(0.0, timeOffset_);
    
    repaint();
}

void SampleEditorComponent::zoomVertical(float factor) {
    verticalZoom_ *= factor;
    verticalZoom_ = std::max(0.1f, std::min(10.0f, verticalZoom_));
    repaint();
}

void SampleEditorComponent::scrollHorizontal(float deltaPixels) {
    double deltaTime = pixelsToTime(deltaPixels, (float)getWidth()) - pixelsToTime(0, (float)getWidth());
    timeOffset_ += deltaTime;
    timeOffset_ = std::max(0.0, timeOffset_);
    repaint();
}

void SampleEditorComponent::fitToWindow() {
    timeOffset_ = 0.0;
    if (audioHandle_) {
        viewWidthSeconds_ = audioHandle_->buffer.getNumSamples() / audioHandle_->sampleRate;
    } else {
        viewWidthSeconds_ = 10.0;
    }
    repaint();
}

void SampleEditorComponent::zoomToSelection() {
    if (selection_.isEmpty()) return;
    timeOffset_ = selection_.getStart();
    viewWidthSeconds_ = selection_.getLength();
    repaint();
}

//==============================================================================
// Selection
//==============================================================================

void SampleEditorComponent::setSelection(double s, double e) {
    selection_ = juce::Range<double>(s, e);
    // Snap to zero crossing if enabled
    if (snapToZeroCrossing_) {
        double startSnap = snapToNearestZeroCrossing(selection_.getStart());
        double endSnap = snapToNearestZeroCrossing(selection_.getEnd());
        selection_ = juce::Range<double>(startSnap, endSnap);
    }
    repaint();
}

void SampleEditorComponent::selectAll() {
    if (audioHandle_) {
        setSelection(0.0, audioHandle_->buffer.getNumSamples() / audioHandle_->sampleRate);
    }
}

void SampleEditorComponent::clearSelection() {
    selection_ = juce::Range<double>();
    repaint();
}

} // namespace zenith
