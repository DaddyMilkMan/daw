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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <map>
#include <memory>
#include <vector>

#include "EngineConstants.h"
#include "AudioRecorder.h" // Needed for RecordingResult

namespace zenith {

class Track;
class ProjectState;
class TempoMap;

namespace tests { class RecordingTempoTest; }

//==============================================================================
/**
    MIDI recording session data.
*/
struct MidiRecordingSession {
  juce::MidiMessageSequence sequence;
  juce::String trackId;
  int trackIndex = -1;
  juce::int64 startSamplePosition = 0;
  bool isActive = false;
};

//==============================================================================
/**
    Manages audio and MIDI recording for the engine.

    Features:
    - Multi-track audio recording with RT-safe disk writing
    - MIDI recording with lock-free fifo
    - Automatic clip creation after recording
    - Async recording preparation for glitch-free punches
*/
class RecordingManager {
public:
  //==========================================================================
  RecordingManager();
  friend class tests::RecordingTempoTest;
  ~RecordingManager();

  //==========================================================================
  // Configuration
  //==========================================================================

  /**
   * @brief Set the project state for clip creation
   * @param state ProjectState reference
   */
  void setProjectState(ProjectState *state) { projectState_ = state; }

  /**
   * @brief Set the audio device manager for input channel info
   * @param manager AudioDeviceManager reference
   */
  void setDeviceManager(const juce::AudioDeviceManager *manager) {
    deviceManager_ = manager;
  }

  /**
   * @brief Prepare for playback
   * @param sampleRate Current sample rate
   */
  void prepare(double sampleRate);

  /**
   * @brief Pre-prepare recording resources for a track when it gets armed.
   */
  void prepareRecordingForTrack(const Track &track, int trackIndex,
                                const juce::File &recordingsDir);

  //==========================================================================
  // Recording Control
  //==========================================================================

  /**
   * @brief Set the directory for storing recordings
   * @param recordDir Directory for recordings
   * @note MESSAGE THREAD ONLY
   */
  void setRecordingDirectory(const juce::File &recordDir);

  /**
   * @brief Start recording on all armed tracks
   * @param startPosition Starting sample position
   * @param tracks Vector of tracks
   * @note MESSAGE THREAD ONLY
   */
  void startRecording(juce::int64 startPosition,
                      const std::vector<std::shared_ptr<Track>> &tracks);

  /**
   * @brief Stop all recording and finalize clips
   * @param tracks Vector of tracks
   * @param tempoMap Tempo map for calculating clip positions 
   * @note MESSAGE THREAD ONLY
   */
  void stopRecording(const std::vector<std::shared_ptr<Track>> &tracks,
                     const TempoMap& tempoMap);

  /**
   * @brief Stop recording and discard all data (delete files)
   * @note MESSAGE THREAD ONLY
   */
  void discardCurrentRecording();

  /**
   * @brief Check if recording is active
   */
  bool isRecording() const { return isRecording_.load(); }

  //==========================================================================
  // Pre-roll / Count-in
  //==========================================================================

  /**
   * @brief Set pre-roll duration in bars (0 = disabled)
   */
  void setPreRollBars(int bars) { preRollBars_ = juce::jlimit(0, 4, bars); }
  int getPreRollBars() const { return preRollBars_.load(); }

  /**
   * @brief Check if currently in pre-roll (count-in) phase
   * @note AUDIO THREAD SAFE
   */
  bool isInPreRoll() const { return isInPreRoll_.load(); }

  /**
   * @brief Get pre-roll countdown in beats remaining
   * @note AUDIO THREAD SAFE
   */
  double getPreRollBeatsRemaining() const { return preRollBeatsRemaining_.load(); }

  /**
   * @brief Start recording with pre-roll
   * @param recordStartPosition Final recording start position (after pre-roll)
   * @param tracks Vector of tracks
   * @param tempoMap Tempo map for calculating pre-roll timing
   * @note MESSAGE THREAD ONLY
   */
  void startRecordingWithPreRoll(juce::int64 recordStartPosition,
                                 const std::vector<std::shared_ptr<Track>> &tracks,
                                 const TempoMap& tempoMap);

  //==========================================================================
  // Punch In/Out
  //==========================================================================

  /**
   * @brief Enable punch in/out recording
   */
  void setPunchEnabled(bool enabled) { punchEnabled_.store(enabled); }
  bool isPunchEnabled() const { return punchEnabled_.load(); }

  /**
   * @brief Set punch in position (samples)
   */
  void setPunchInPosition(juce::int64 position) { punchInPosition_ = position; }
  juce::int64 getPunchInPosition() const { return punchInPosition_.load(); }

  /**
   * @brief Set punch out position (samples, -1 = no punch out)
   */
  void setPunchOutPosition(juce::int64 position) { punchOutPosition_ = position; }
  juce::int64 getPunchOutPosition() const { return punchOutPosition_.load(); }

  /**
   * @brief Check if currently inside punch range
   * @note AUDIO THREAD SAFE
   */
  bool isInsidePunchRange(juce::int64 playheadPosition) const;

  /**
   * @brief Check if punch recording is currently active (between in and out points)
   * @note AUDIO THREAD SAFE
   */
  bool isPunchRecordingActive() const { return isPunchRecordingActive_.load(); }

  //==========================================================================
  // Audio Capture (RT-Safe)
  //==========================================================================

  /**
   * @brief Capture audio input for recording
   * @param inputData Input channel data
   * @param numInputChannels Number of input channels
   * @param numSamples Number of samples
   * @param tracks Vector of tracks
   * @note AUDIO THREAD ONLY - RT-safe
   */
  void captureAudio(const float *const *inputData, int numInputChannels,
                    int numSamples,
                    const std::vector<std::shared_ptr<Track>> &tracks);

  //==========================================================================
  // MIDI Recording (RT-Safe)
  //==========================================================================

  /**
   * @brief Add MIDI message to recording buffer
   * @param message MIDI message
   * @param samplePosition Sample position
   * @param trackIndex Target track index
   * @note AUDIO THREAD ONLY - RT-safe
   */
  void captureMidi(const juce::MidiMessage &message, juce::int64 samplePosition,
                   int trackIndex);

  /**
   * @brief Drain MIDI fifo to message thread
   * @note MESSAGE THREAD ONLY
   */
  void drainMidiFifo();


  //==========================================================================
  // Internal Methods (Public for testing)
  //==========================================================================

  /**
   * @brief Create clips from completed recordings
   */
  void finalizeRecordings(const std::vector<RecordingResult>& results,
                          const std::vector<std::shared_ptr<Track>> &tracks,
                          const TempoMap& tempoMap);

  /**
   * @brief Create an audio clip in ProjectState
   */
  void createAudioClip(const juce::File &audioFile, const juce::String &trackId,
                       juce::int64 startSamplePosition,
                       juce::int64 lengthSamples, const TempoMap& tempoMap);

  /**
   * @brief Create a MIDI clip in ProjectState
   */
  void createMidiClip(const juce::MidiMessageSequence &sequence,
                      const juce::String &trackId,
                      juce::int64 startSamplePosition, const TempoMap& tempoMap);

private:


private:

  //==========================================================================
  // State
  //==========================================================================

  double sampleRate_ = constants::kDefaultSampleRate;
  std::atomic<bool> isRecording_{false};

  // Audio recorder (owns TimeSliceThread and ring buffers)
  std::unique_ptr<AudioRecorder> audioRecorder_;

  // Audio device manager (for input routing)
  const juce::AudioDeviceManager *deviceManager_ = nullptr;

  // Project state for clip creation (owned by Engine)
  ProjectState *projectState_ = nullptr;

  // MIDI recording sessions (message thread only access)
  std::vector<MidiRecordingSession> midiSessions_;

  // Recording start position for clip placement
  juce::int64 recordingStartPosition_ = 0;

  // Lock-free MIDI recording fifo
  struct MidiFifoEntry {
    juce::MidiMessage message;
    juce::int64 samplePosition;
    int trackIndex;
  };
  juce::AbstractFifo midiFifoIndex_{constants::kMidiRecordFifoSize};
  std::vector<MidiFifoEntry> midiFifoData_;

  // Recording directory (cached for session)
  juce::File recordingDirectory_;

  // Thread safety for session modification
  juce::CriticalSection sessionLock_;

  // Bug 17: Track dropped MIDI messages
  std::atomic<uint64_t> droppedMidiMessages_{0};

  //==========================================================================
  // Pre-roll / Count-in State
  //==========================================================================
  std::atomic<int> preRollBars_{0};           // 0, 1, 2, or 4 bars
  std::atomic<bool> isInPreRoll_{false};      // Currently counting in
  std::atomic<double> preRollBeatsRemaining_{0.0};
  juce::int64 preRollEndPosition_ = 0;        // Sample position where recording starts

  //==========================================================================
  // Punch In/Out State
  //==========================================================================
  std::atomic<bool> punchEnabled_{false};
  std::atomic<juce::int64> punchInPosition_{0};
  std::atomic<juce::int64> punchOutPosition_{-1};  // -1 = disabled
  std::atomic<bool> isPunchRecordingActive_{false};  // Currently inside punch zone

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingManager)
};

} // namespace zenith
