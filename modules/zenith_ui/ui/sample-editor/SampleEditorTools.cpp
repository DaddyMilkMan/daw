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

 * @file SampleEditorTools.cpp
 * @brief DSP and editing tools for SampleEditorComponent (Normalize, Reverse, Effects, etc.)
 */



//==============================================================================
// Editing Operations
//==============================================================================

void SampleEditorComponent::cutSelection() {
    copySelection();
    deleteSelection();
}

void SampleEditorComponent::copySelection() {
    if (selection_.isEmpty() || !audioHandle_) return;
    
    juce::int64 start = timeToSamples(selection_.getStart());
    juce::int64 length = timeToSamples(selection_.getLength());
    
    clipboard_ = std::make_unique<juce::AudioBuffer<float>>(audioHandle_->buffer.getNumChannels(), (int)length);
    for (int i = 0; i < audioHandle_->buffer.getNumChannels(); ++i) {
        clipboard_->copyFrom(i, 0, audioHandle_->buffer, i, (int)start, (int)length);
    }
    clipboardSampleRate_ = audioHandle_->sampleRate;
}

void SampleEditorComponent::paste() {
    if (!clipboard_ || !audioHandle_) return;
    
    pushUndoState("Paste");
    
    // Paste logic... insertion at playhead or overwriting selection
}

void SampleEditorComponent::deleteSelection() {
    if (selection_.isEmpty() || !audioHandle_) return;
    
    pushUndoState("Delete");
    
    // Delete logic... splice buffer
}

void SampleEditorComponent::trimToSelection() {
    if (selection_.isEmpty() || !audioHandle_) return;
    
    pushUndoState("Trim");
    
    // Trim logic... keep only selection
}

void SampleEditorComponent::splitAtCursor() {
    // Split logic...
}

//==============================================================================
// Processing Operations
//==============================================================================

void SampleEditorComponent::normalize(float targetDb) {
    if (!audioHandle_) return;
    pushUndoState("Normalize");
    
    float maxAmp = 0.0f;
    for (int ch = 0; ch < audioHandle_->buffer.getNumChannels(); ++ch) {
        maxAmp = std::max(maxAmp, audioHandle_->buffer.getMagnitude(ch, 0, audioHandle_->buffer.getNumSamples()));
    }
    
    if (maxAmp > 0.00001f) {
        float gain = juce::Decibels::decibelsToGain(targetDb) / maxAmp;
        audioHandle_->buffer.applyGain(gain);
    }
    repaint();
}

void SampleEditorComponent::reverse() {
    if (!audioHandle_) return;
    pushUndoState("Reverse");
    
    int numSamples = audioHandle_->buffer.getNumSamples();
    for (int ch = 0; ch < audioHandle_->buffer.getNumChannels(); ++ch) {
        float* data = audioHandle_->buffer.getWritePointer(ch);
        std::reverse(data, data + numSamples);
    }
    repaint();
}

void SampleEditorComponent::fadeIn(double durationSeconds) {
    if (!audioHandle_ || selection_.isEmpty()) return;
    pushUndoState("Fade In");
    
    juce::int64 start = timeToSamples(selection_.getStart());
    juce::int64 length = timeToSamples(durationSeconds);
    
    audioHandle_->buffer.applyGainRamp((int)start, (int)length, 0.0f, 1.0f);
    repaint();
}

void SampleEditorComponent::fadeOut(double durationSeconds) {
    if (!audioHandle_ || selection_.isEmpty()) return;
    pushUndoState("Fade Out");
    
    juce::int64 length = timeToSamples(durationSeconds);
    juce::int64 end = timeToSamples(selection_.getEnd());
    juce::int64 start = end - length;
    
    audioHandle_->buffer.applyGainRamp((int)start, (int)length, 1.0f, 0.0f);
    repaint();
}

void SampleEditorComponent::adjustGain(float db) {
    if (!audioHandle_) return;
    pushUndoState("Adjust Gain");
    
    float gain = juce::Decibels::decibelsToGain(db);
    if (!selection_.isEmpty()) {
        audioHandle_->buffer.applyGain((int)timeToSamples(selection_.getStart()), (int)timeToSamples(selection_.getLength()), gain);
    } else {
        audioHandle_->buffer.applyGain(gain);
    }
    repaint();
}

void SampleEditorComponent::silenceSelection() {
    if (!audioHandle_ || selection_.isEmpty()) return;
    pushUndoState("Silence");
    audioHandle_->buffer.clear((int)timeToSamples(selection_.getStart()), (int)timeToSamples(selection_.getLength()));
    repaint();
}

void SampleEditorComponent::removeOffset() {
    if (!audioHandle_) return;
    pushUndoState("Remove DC Offset");
    // DC offset removal logic...
}

//==============================================================================
// Advanced Processing
//==============================================================================

void SampleEditorComponent::timeStretch(float ratio) {
    // Implement time stretching via RubberBand or internal DSP...
}

void SampleEditorComponent::pitchShift(int semitones) {
    // Implement pitch shifting...
}

void SampleEditorComponent::detectTransients(float sensitivity) {
    // Transient detection logic...
}

void SampleEditorComponent::autoSlice(float sensitivity) {
    // Auto slice logic...
}

void SampleEditorComponent::sliceToMidi() {
    // Export to MIDI...
}

//==============================================================================
// Analysis
//==============================================================================

void SampleEditorComponent::generateWaveformCache() {
    // Pre-compute peaks...
}

float SampleEditorComponent::getRMSLevel() const {
    if (!audioHandle_) return 0.0f;
    return audioHandle_->buffer.getRMSLevel(0, 0, audioHandle_->buffer.getNumSamples());
}

float SampleEditorComponent::getPeakLevel() const {
    if (!audioHandle_) return 0.0f;
    return audioHandle_->buffer.getMagnitude(0, 0, audioHandle_->buffer.getNumSamples());
}

//==============================================================================
// Markers
//==============================================================================

void SampleEditorComponent::addMarker(double t, const juce::String& name) {
    markers_.push_back({juce::Uuid().toString(), name, t, SkColorSetRGB(255, 200, 50)});
    repaint();
}

void SampleEditorComponent::removeMarker(const juce::String& id) {
    markers_.erase(std::remove_if(markers_.begin(), markers_.end(), [&](const auto& m){ return m.id == id; }), markers_.end());
    repaint();
}

void SampleEditorComponent::clearMarkers() {
    markers_.clear();
    repaint();
}

//==============================================================================
// Serialization
//==============================================================================

void SampleEditorComponent::saveToFile() {
    // Save logic...
}

void SampleEditorComponent::saveAsNewFile(const juce::File& targetFile) {
    // Save as logic...
}

void SampleEditorComponent::exportSelection(const juce::File& targetFile) {
    // Export selection logic...
}

//==============================================================================
// Snap
//==============================================================================

double SampleEditorComponent::snapToNearestZeroCrossing(double t) const {
    if (!audioHandle_) return t;
    juce::int64 sample = timeToSamples(t);
    // Zero crossing detection logic...
    return t; 
}

} // namespace zenith
