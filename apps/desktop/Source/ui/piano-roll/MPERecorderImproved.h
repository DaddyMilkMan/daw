/*
  ==============================================================================

    MPERecorderImproved.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Improved MPE recorder with proper note tracking and thread safety.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../../Source/ui/piano-roll/MPEExpressionHelpers.h"
#include <map>

namespace zenith {

class PianoRollComponent;

/**
 * Active note tracking for MPE recording
 */
struct ActiveNote {
  juce::String noteId;
  int midiChannel;
  int noteNumber;
  double startTime;
  bool isActive;

  ActiveNote() : midiChannel(0), noteNumber(0), startTime(0.0), isActive(false) {}
  ActiveNote(const juce::String& id, int ch, int note, double start)
    : noteId(id), midiChannel(ch), noteNumber(note), startTime(start), isActive(true) {}
};

/**
 * Recording state for a single expression lane
 */
struct LaneRecordingState {
  bool isArmed = false;
  bool isRecording = false;
  juce::String noteId;
  double startTime = 0.0;
  std::map<double, float> recordedData;
};

/**
 * Improved MPE recorder with proper note tracking
 */
class MPERecorderImproved {
public:
  MPERecorderImproved();
  ~MPERecorderImproved();

  void setPianoRoll(PianoRollComponent* pianoRoll);
  void setSampleRate(double sampleRate);

  void setLaneArmed(ExpressionType type, bool armed);
  bool isLaneArmed(ExpressionType type) const;

  void startRecording();
  void stopRecording();

  void processMidiBuffer(const juce::MidiBuffer& midiBuffer, double currentTime);

  const LaneRecordingState& getLaneState(ExpressionType type) const;

  // Note tracking
  void noteOnStarted(const juce::String& noteId, int channel, int noteNumber, double time);
  void noteOffEnded(int channel, int noteNumber);

private:
  void handlePressure(const juce::MidiMessage& message, double time);
  void handleTimbre(const juce::MidiMessage& message, double time);
  void handlePitchbend(const juce::MidiMessage& message, double time);

  juce::String findNoteIdForChannel(int channel, int noteNumber) const;
  double quantizeToGrid(double timeOffset, double gridSize) const;

  juce::CriticalSection lock_;
  std::map<ExpressionType, LaneRecordingState> laneStates_;
  std::map<std::pair<int, int>, ActiveNote> activeNotes_; // (channel, note) -> ActiveNote

  bool isRecording_;
  double recordingStartTime_;
  double gridSize_;
  double sampleRate_;

  PianoRollComponent* pianoRoll_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPERecorderImproved)
};

} // namespace zenith
