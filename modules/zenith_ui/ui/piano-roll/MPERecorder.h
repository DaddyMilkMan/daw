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

    MPERecorder.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Records MPE (MIDI Polyphonic Expression) messages as automation
    in expression lanes. Captures pressure, timbre, and pitchbend in real-time
    during recording.


  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <map>
#include <memory>
#include <vector>

namespace zenith {

class PianoRollComponent;

/**
 * Recording state for a single expression lane
 */
struct LaneRecordingState {
  bool isArmed = false;        // Ready to record
  bool isRecording = false;    // Currently recording
  juce::String noteId;        // Note being recorded (for single-note mode)
  
  // Recording start time
  double startTime = 0.0;
  
  // Recorded data (temporary, until recording stops)
  std::map<double, float> recordedData;  // timeOffset -> value
};

/**
 * Records MPE messages as expression automation
 *
 * Features:
 * - Per-lane recording arm
 * - Auto-capture MPE messages during recording
 * - Quantize to grid (optional)
 * - Create ExpressionPoint objects
 */
class MPERecorder {
public:
  MPERecorder();
  ~MPERecorder();

  /**
   * Set piano roll component (for reading/writing expression data)
   */
  void setPianoRoll(PianoRollComponent* pianoRoll);

  /**
   * Arm/disarm recording for a specific lane
   */
  void setLaneArmed(ExpressionType type, bool armed);

  /**
   * Check if a lane is armed for recording
   */
  bool isLaneArmed(ExpressionType type) const;

  /**
   * Start recording (called when transport starts)
   */
  void startRecording();

  /**
   * Stop recording (called when transport stops)
   */
  void stopRecording();

  /**
   * Process MIDI buffer for MPE messages
   * Called every audio callback
   */
  void processMidiBuffer(const juce::MidiBuffer& midiBuffer,
                         double currentTime);

  /**
   * Get recording state for a lane
   */
  const LaneRecordingState& getLaneState(ExpressionType type) const;

private:
  /**
   * Handle MPE pressure message
   */
  void handlePressure(const juce::MidiMessage& message, double time);

  /**
   * Handle MPE timbre message (CC74)
   */
  void handleTimbre(const juce::MidiMessage& message, double time);

  /**
   * Handle MPE pitchbend message
   */
  void handlePitchbend(const juce::MidiMessage& message, double time);

  /**
   * Find note ID for a MIDI channel and note number
   */
  juce::String findNoteIdForChannel(int channel, int noteNumber) const;

  /**
   * Quantize time offset to grid
   */
  double quantizeToGrid(double timeOffset, double gridSize) const;

  // State
  std::map<ExpressionType, LaneRecordingState> laneStates_;
  bool isRecording_ = false;
  double recordingStartTime_ = 0.0;
  double gridSize_ = 0.25;  // Quantize to 16th notes by default

  // Access
  PianoRollComponent* pianoRoll_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPERecorder)
};

} // namespace zenith
