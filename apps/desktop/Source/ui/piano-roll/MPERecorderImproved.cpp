/*
  ==============================================================================

    MPERecorderImproved.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Improved MPE recorder implementation with proper note tracking.

  ==============================================================================
*/

#include "MPERecorderImproved.h"
#include "PianoRollComponent.h"
#include <cmath>

namespace zenith {

MPERecorderImproved::MPERecorderImproved()
    : pianoRoll_(nullptr),
      isRecording_(false),
      recordingStartTime_(0.0),
      gridSize_(0.25),
      sampleRate_(44100.0) {
}

MPERecorderImproved::~MPERecorderImproved() {
}

void MPERecorderImproved::setPianoRoll(PianoRollComponent* pianoRoll) {
  juce::ScopedLock lock(lock_);
  pianoRoll_ = pianoRoll;
}

void MPERecorderImproved::setSampleRate(double sampleRate) {
  juce::ScopedLock lock(lock_);
  sampleRate_ = sampleRate;
}

void MPERecorderImproved::setLaneArmed(ExpressionType type, bool armed) {
  juce::ScopedLock lock(lock_);
  LaneRecordingState& state = laneStates_[type];
  state.isArmed = armed;

  if (!armed && state.isRecording) {
    state.isRecording = false;
  }
}

bool MPERecorderImproved::isLaneArmed(ExpressionType type) const {
  juce::ScopedLock lock(lock_);
  auto it = laneStates_.find(type);
  return it != laneStates_.end() && it->second.isArmed;
}

void MPERecorderImproved::startRecording() {
  juce::ScopedLock lock(lock_);
  if (isRecording_) return;

  isRecording_ = true;
  recordingStartTime_ = 0.0;

  for (auto& entry : laneStates_) {
    if (entry.second.isArmed) {
      entry.second.isRecording = true;
      entry.second.startTime = 0.0;
      entry.second.recordedData.clear();
    }
  }
}

void MPERecorderImproved::stopRecording() {
  juce::ScopedLock lock(lock_);
  if (!isRecording_) return;

  isRecording_ = false;

  for (auto& entry : laneStates_) {
    if (entry.second.isRecording && !entry.second.recordedData.empty()) {
      ExpressionType type = entry.first;
      const LaneRecordingState& state = entry.second;

      if (pianoRoll_ != nullptr && state.noteId.isNotEmpty()) {
        // Convert to expression points
        juce::Array<PianoRollComponent::ExpressionPoint> points;
        for (const auto& dataEntry : state.recordedData) {
          PianoRollComponent::ExpressionPoint point;
          point.timeOffset = dataEntry.first;
          point.value = dataEntry.second;
          point.tension = 0.0f;
          points.add(point);
        }

        pianoRoll_->setNoteExpression(state.noteId, type, std::vector<PianoRollComponent::ExpressionPoint>(
          points.begin(), points.end()));
      }
    }

    entry.second.isRecording = false;
    entry.second.recordedData.clear();
  }
}

void MPERecorderImproved::processMidiBuffer(const juce::MidiBuffer& midiBuffer,
                                              double currentTime) {
  juce::ScopedLock lock(lock_);
  if (!isRecording_ || pianoRoll_ == nullptr) return;

  if (recordingStartTime_ == 0.0 && currentTime > 0.0) {
    recordingStartTime_ = currentTime;
  }

  for (auto it = midiBuffer.begin(); it != midiBuffer.end(); ++it) {
    const juce::MidiMessage& message = (*it).getMessage();
    int samplePosition = (*it).samplePosition;
    double time = currentTime + (static_cast<double>(samplePosition) / sampleRate_);

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

const MPERecorderImproved::LaneRecordingState&
MPERecorderImproved::getLaneState(ExpressionType type) const {
  static MPERecorderImproved::LaneRecordingState emptyState;
  juce::ScopedLock lock(lock_);
  auto it = laneStates_.find(type);
  if (it != laneStates_.end()) {
    return it->second;
  }
  return emptyState;
}

void MPERecorderImproved::noteOnStarted(const juce::String& noteId,
                                       int channel,
                                       int noteNumber,
                                       double time) {
  juce::ScopedLock lock(lock_);
  std::pair<int, int> key(channel, noteNumber);
  activeNotes_[key] = ActiveNote(noteId, channel, noteNumber, time);
}

void MPERecorderImproved::noteOffEnded(int channel, int noteNumber) {
  juce::ScopedLock lock(lock_);
  std::pair<int, int> key(channel, noteNumber);
  activeNotes_.erase(key);
}

void MPERecorderImproved::handlePressure(const juce::MidiMessage& message, double time) {
  int channel = message.getChannel();
  float pressure = message.getChannelPressureValue() / 127.0f;

  juce::String noteId = findNoteIdForChannel(channel, -1);
  if (noteId.isEmpty()) return;

  auto it = laneStates_.find(ExpressionType::Pressure);
  if (it != laneStates_.end() && it->second.isRecording) {
    double timeOffset = time - recordingStartTime_;
    timeOffset = quantizeToGrid(timeOffset, gridSize_);

    LaneRecordingState& state = it->second;
    state.noteId = noteId;
    state.recordedData[timeOffset] = pressure;
  }
}

void MPERecorderImproved::handleTimbre(const juce::MidiMessage& message, double time) {
  int channel = message.getChannel();
  float timbre = message.getControllerValue() / 127.0f;

  juce::String noteId = findNoteIdForChannel(channel, -1);
  if (noteId.isEmpty()) return;

  auto it = laneStates_.find(ExpressionType::Slide);
  if (it != laneStates_.end() && it->second.isRecording) {
    double timeOffset = time - recordingStartTime_;
    timeOffset = quantizeToGrid(timeOffset, gridSize_);

    LaneRecordingState& state = it->second;
    state.noteId = noteId;
    state.recordedData[timeOffset] = timbre;
  }
}

void MPERecorderImproved::handlePitchbend(const juce::MidiMessage& message, double time) {
  int channel = message.getChannel();
  int pitchbendValue = message.getPitchWheelValue();
  float normalizedPitchbend = (static_cast<float>(pitchbendValue) - 8192.0f) / 8192.0f;
  float normalized = (normalizedPitchbend + 1.0f) / 2.0f;

  juce::String noteId = findNoteIdForChannel(channel, -1);
  if (noteId.isEmpty()) return;

  auto it = laneStates_.find(ExpressionType::PitchBend);
  if (it != laneStates_.end() && it->second.isRecording) {
    double timeOffset = time - recordingStartTime_;
    timeOffset = quantizeToGrid(timeOffset, gridSize_);

    LaneRecordingState& state = it->second;
    state.noteId = noteId;
    state.recordedData[timeOffset] = normalized;
  }
}

juce::String MPERecorderImproved::findNoteIdForChannel(int channel, int noteNumber) const {
  std::pair<int, int> key(channel, noteNumber >= 0 ? noteNumber : -1);

  // First try exact match
  auto it = activeNotes_.find(key);
  if (it != activeNotes_.end() && it->second.isActive) {
    return it->second.noteId;
  }

  // If noteNumber is -1, search for any note on this channel
  if (noteNumber < 0) {
    for (const auto& entry : activeNotes_) {
      if (entry.first.first == channel && entry.second.isActive) {
        return entry.second.noteId;
      }
    }
  }

  return juce::String();
}

double MPERecorderImproved::quantizeToGrid(double timeOffset, double gridSize) const {
  if (gridSize <= 0.0) return timeOffset;

  double quotient = timeOffset / gridSize;
  double rounded = std::floor(quotient + 0.5);
  double quantized = rounded * gridSize;
  return quantized;
}

} // namespace zenith
