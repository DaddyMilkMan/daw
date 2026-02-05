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

    MPERecorder.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Implementation of MPE message recording as automation.

  ==============================================================================

*/

#include "MPERecorder.h"
#include "PianoRollComponent.h"
#include "MPEExpressionHelpers.h"

namespace zenith {

//==============================================================================
// MPERecorder Implementation
//==============================================================================

MPERecorder::MPERecorder()
    : pianoRoll_(nullptr),
      isRecording_(false),
      recordingStartTime_(0.0),
      gridSize_(0.25) {
}

MPERecorder::~MPERecorder() = default;

void MPERecorder::setPianoRoll(PianoRollComponent *pianoRoll) {
  pianoRoll_ = pianoRoll;
}

void MPERecorder::setLaneArmed(ExpressionType type, bool armed) {
  LaneRecordingState &state = laneStates_[type];
  state.isArmed = armed;

  if (!armed && state.isRecording) {
    // Disarm stops recording for that lane
    state.isRecording = false;
  }
}

bool MPERecorder::isLaneArmed(ExpressionType type) const {
  auto it = laneStates_.find(type);
  if (it != laneStates_.end()) {
    return it->second.isArmed;
  }
  return false;
}

void MPERecorder::startRecording() {
  if (isRecording_) return;

  isRecording_ = true;
  recordingStartTime_ = 0.0; // Will be set on first message

  // Start recording for all armed lanes
  for (auto &entry : laneStates_) {
    if (entry.second.isArmed) {
      entry.second.isRecording = true;
      entry.second.startTime = 0.0;
      entry.second.recordedData.clear();
    }
  }
}

void MPERecorder::stopRecording() {
  if (!isRecording_) return;

  isRecording_ = false;

  // Commit recorded data to expression points for each lane
  for (auto &entry : laneStates_) {
    if (entry.second.isRecording && !entry.second.recordedData.empty()) {
      ExpressionType type = entry.first;
      const LaneRecordingState &state = entry.second;

      // Convert recorded data to ExpressionPoint objects
      juce::Array<PianoRollComponent::ExpressionPoint> points;

      for (const auto &dataEntry : state.recordedData) {
        PianoRollComponent::ExpressionPoint point;
        point.timeOffset = dataEntry.first;
        point.value = dataEntry.second;
        point.tension = 0.0f; // Linear by default
        points.add(point);
      }

      // Apply to selected notes or all notes in the note
      // For now, we'll store this and let the piano roll handle application
      if (pianoRoll_ && state.noteId.isNotEmpty()) {
        pianoRoll_->setNoteExpression(state.noteId, type, points);
      }
    }

    // Reset recording state
    entry.second.isRecording = false;
    entry.second.recordedData.clear();
  }
}

void MPERecorder::processMidiBuffer(const juce::MidiBuffer &midiBuffer,
                                     double currentTime) {
  if (!isRecording_ || !pianoRoll_) return;

  // Set recording start time on first message
  if (recordingStartTime_ == 0.0 && currentTime > 0.0) {
    recordingStartTime_ = currentTime;
  }

  for (auto it = midiBuffer.begin(); it != midiBuffer.end(); ++it) {
    const juce::MidiMessage &message = (*it).getMessage();
    int samplePosition = (*it).samplePosition;
    double time = currentTime + (double)samplePosition / 44100.0;

    // Check for MPE messages
    if (message.isChannelPressure()) {
      handlePressure(message, time);
    } else if (message.isController()) {
      int controllerNumber = message.getControllerNumber();
      if (controllerNumber == 74) {
        handleTimbre(message, time);
      }
    } else if (message.isPitchWheel()) {
      handlePitchbend(message, time);
    }
  }
}

const MPERecorder::LaneRecordingState&
MPERecorder::getLaneState(ExpressionType type) const {
  static MPERecorder::LaneRecordingState emptyState;
  auto it = laneStates_.find(type);
  if (it != laneStates_.end()) {
    return it->second;
  }
  return emptyState;
}

void MPERecorder::handlePressure(const juce::MidiMessage &message, double time) {
  int channel = message.getChannel();
  float pressure = message.getChannelPressureValue() / 127.0f;

  // Find which note this channel belongs to
  juce::String noteId = findNoteIdForChannel(channel, -1);
  if (noteId.isEmpty()) return;

  // Record if lane is armed
  auto it = laneStates_.find(ExpressionType::Pressure);
  if (it != laneStates_.end() && it->second.isRecording) {
    double timeOffset = time - recordingStartTime_;
    timeOffset = quantizeToGrid(timeOffset, gridSize_);

    LaneRecordingState &state = it->second;
    state.noteId = noteId;
    state.recordedData[timeOffset] = pressure;
  }
}

void MPERecorder::handleTimbre(const juce::MidiMessage &message, double time) {
  int channel = message.getChannel();
  float timbre = message.getControllerValue() / 127.0f;

  // Find which note this channel belongs to
  juce::String noteId = findNoteIdForChannel(channel, -1);
  if (noteId.isEmpty()) return;

  // Record if lane is armed
  auto it = laneStates_.find(ExpressionType::Slide);
  if (it != laneStates_.end() && it->second.isRecording) {
    double timeOffset = time - recordingStartTime_;
    timeOffset = quantizeToGrid(timeOffset, gridSize_);

    LaneRecordingState &state = it->second;
    state.noteId = noteId;
    state.recordedData[timeOffset] = timbre;
  }
}

void MPERecorder::handlePitchbend(const juce::MidiMessage &message, double time) {
  int channel = message.getChannel();
  int pitchbendValue = message.getPitchWheelValue();
  float normalizedPitchbend = (pitchbendValue - 8192) / 8192.0f; // -1.0 to 1.0
  float normalized = (normalizedPitchbend + 1.0f) / 2.0f; // 0.0 to 1.0

  // Find which note this channel belongs to
  juce::String noteId = findNoteIdForChannel(channel, -1);
  if (noteId.isEmpty()) return;

  // Record if lane is armed
  auto it = laneStates_.find(ExpressionType::PitchBend);
  if (it != laneStates_.end() && it->second.isRecording) {
    double timeOffset = time - recordingStartTime_;
    timeOffset = quantizeToGrid(timeOffset, gridSize_);

    LaneRecordingState &state = it->second;
    state.noteId = noteId;
    state.recordedData[timeOffset] = normalized;
  }
}

juce::String MPERecorder::findNoteIdForChannel(int channel,
                                                int noteNumber) const {
  // This is a simplified implementation
  // In a real scenario, you'd need to track which notes are active on which channels
  // For now, return empty string - this would need to be integrated with the
  // active note tracking in the audio engine
  return juce::String();
}

double MPERecorder::quantizeToGrid(double timeOffset, double gridSize) const {
  if (gridSize <= 0.0) return timeOffset;

  // Manual rounding to avoid std::round
  double quotient = timeOffset / gridSize;
  double rounded = (double)(int)(quotient + 0.5);
  double quantized = rounded * gridSize;
  return quantized;
}

} // namespace zenith
