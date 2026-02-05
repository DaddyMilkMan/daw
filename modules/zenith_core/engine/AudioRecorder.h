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

    AudioRecorder.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Handles audio file recording streams and threading with RT-safe ring buffer.

    Thread Safety:

    - write() is AUDIO THREAD ONLY (RT-safe, lock-free)
    - startRecording()/stopRecording() are MESSAGE THREAD ONLY
    - Uses RCU (Read-Copy-Update) pattern for lock-free session list access
    - Uses juce::AbstractFifo for lock-free ring buffer

    Architecture:
    - Audio thread writes to ring buffer via write()
    - Background thread (TimeSlice) flushes ring buffer to disk
    - No locks on audio thread path
    - Sessions managed via shared_ptr and atomic snapshot for true RT safety

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

#include "EngineConstants.h"

namespace zenith {

// Forward declarations
class Track;

//==============================================================================
/**
    Ring buffer for RT-safe audio recording.

    Uses juce::AbstractFifo for lock-free producer-consumer pattern.
    Audio thread writes, background thread reads and flushes to disk.
*/
class AudioRingBuffer {
public:
  static constexpr int kDefaultBufferSizeFrames =
      65536; // ~1.5 seconds at 44.1kHz

  AudioRingBuffer(int numChannels = 2,
                  int bufferSizeFrames = kDefaultBufferSizeFrames);
  ~AudioRingBuffer() = default;

  //==========================================================================
  // RT-Safe Write (Audio Thread)
  //==========================================================================

  /**
   * @brief Write audio samples to the ring buffer (RT-safe, lock-free)
   */
  int write(const float *const *data, int numChannels, int numSamples);

  //==========================================================================
  // Non-RT Read (Background Thread)
  //==========================================================================

  /**
   * @brief Read and remove samples from the ring buffer (NOT RT-safe)
   */
  int read(juce::AudioBuffer<float> &output, int numSamples);

  int getNumReady() const { return fifo_.getNumReady(); }
  int getCapacity() const { return bufferSizeFrames_; }
  bool hasOverflowed(bool reset = false);
  void reset();

private:
  juce::AbstractFifo fifo_;
  juce::AudioBuffer<float> buffer_;
  int numChannels_;
  int bufferSizeFrames_;
  std::atomic<bool> overflowed_{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRingBuffer)
};

//==============================================================================
/**
    Per-track recording session.

    Contains ring buffer, file writer, and metadata for one track's recording.
    Now managed via shared_ptr for RCU safety.
*/
struct RecordingSession {
  std::unique_ptr<AudioRingBuffer> ringBuffer;
  std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
  juce::File file;
  juce::String trackId; // Added for robust linking
  int trackIndex = -1;
  int inputChannelStart = 0;
  int numChannels = 2;
  juce::int64 startSamplePosition = 0;
  std::atomic<juce::int64> samplesRecorded{0};
  double sampleRate = 44100.0;
  std::atomic<bool> isActive{false};

  RecordingSession() = default;
  ~RecordingSession() = default;

  // Custom move operations needed because of unique_ptr
  RecordingSession(RecordingSession &&other) noexcept;
  RecordingSession &operator=(RecordingSession &&other) noexcept;

  // Non-copyable
  RecordingSession(const RecordingSession &) = delete;
  RecordingSession &operator=(const RecordingSession &) = delete;
};

//==============================================================================
/**
    Recording result returned when stopping recording.
*/
struct RecordingResult {
  juce::File file;
  int trackIndex;
  juce::String trackId;
  juce::int64 startSamplePosition;
  juce::int64 samplesRecorded;
  double sampleRate;
};

//==============================================================================
/**
    Handles multi-track audio recording with RT-safe ring buffers.

    Features:
    - RT-safe audio capture via lock-free ring buffers
    - Background disk writing via TimeSliceThread
    - Lock-free RCU pattern for session list management (High Effort)
*/
class AudioRecorder : public juce::TimeSliceClient {
public:
  enum class RecordingState {
    Idle,
    Recording,
    Finalizing
  };

  AudioRecorder();
  ~AudioRecorder() override;

  //==========================================================================
  // Configuration
  //==========================================================================

  void prepare(double sampleRate);

  //==========================================================================
  // Recording Control (MESSAGE THREAD ONLY)
  //==========================================================================

  void startRecording(const std::vector<std::shared_ptr<Track>> &tracks,
                      const juce::AudioDeviceManager &deviceManager,
                      juce::int64 startSample, const juce::File &recordingsDir);

  void stopRecording(std::function<void(std::vector<RecordingResult>)> completionCallback);

  RecordingState getState() const { return state_.load(); }
  bool isRecording() const { return state_.load() == RecordingState::Recording; }
  bool isFinalizing() const { return state_.load() == RecordingState::Finalizing; }

  //==========================================================================
  // Loop Recording Mode (for Take Folders)
  //==========================================================================

  /**
   * @brief Enable loop recording mode.
   * When enabled, recording will automatically create new takes when the
   * transport loops back to the loop start position.
   */
  void setLoopRecordingEnabled(bool enabled) {
    loopRecordingEnabled_.store(enabled);
  }
  bool isLoopRecordingEnabled() const { return loopRecordingEnabled_.load(); }

  /**
   * @brief Set the loop boundaries for loop recording.
   * @param loopStart Loop start position in samples.
   * @param loopEnd Loop end position in samples.
   */
  void setLoopBoundaries(juce::int64 loopStart, juce::int64 loopEnd) {
    loopStart_.store(loopStart);
    loopEnd_.store(loopEnd);
  }

  /**
   * @brief Get the current take number (increments on each loop cycle).
   */
  int getCurrentTakeNumber() const { return currentTakeNumber_.load(); }

  /**
   * @brief Notify recorder that a loop cycle occurred.
   * Called from transport when looping back.
   * This triggers creation of new takes for loop recording mode.
   * @note MESSAGE THREAD ONLY.
   */
  void onLoopCycle();


  //==========================================================================
  // Audio Capture (AUDIO THREAD ONLY - RT-SAFE)
  //==========================================================================

  void write(const float *const *inputChannelData, int numInputChannels,
             int numSamples, const std::vector<std::shared_ptr<Track>> &tracks);

  //==========================================================================
  // TimeSliceClient Interface (Background Thread)
  //==========================================================================

  int useTimeSlice() override;

private:
  //==========================================================================
  // RCU (Read-Copy-Update) Snapshot Pattern
  // Ensures audio thread never sees invalid memory or locks
  //==========================================================================

  struct SessionSnapshot {
    std::vector<std::shared_ptr<RecordingSession>> sessions;
    SessionSnapshot() = default;
    SessionSnapshot(const std::vector<std::shared_ptr<RecordingSession>> &s)
        : sessions(s) {}
  };

  // Atomic pointer for RT threads (Audio & Background)
  std::atomic<SessionSnapshot *> activeSessionSnapshot_{nullptr};

  // Message thread ownership
  std::shared_ptr<SessionSnapshot> currentSessionSnapshot_;

  std::vector<std::shared_ptr<RecordingSession>> sessions_; // Shared ownership

  void updateSessionSnapshot();

  //==========================================================================
  // Internal Methods and State
  //==========================================================================

  juce::File createRecordingFile(const juce::File &dir,
                                 const juce::String &trackName);

  std::unique_ptr<juce::TimeSliceThread> writerThread_;
  std::atomic<RecordingState> state_{RecordingState::Idle};
  std::function<void(std::vector<RecordingResult>)> completionCallback_;
  double sampleRate_ = constants::kDefaultSampleRate;

  juce::AudioBuffer<float> tempReadBuffer_;
  static constexpr int kFlushBlockSize = 4096;

  //==========================================================================
  // Loop Recording State
  //==========================================================================

  std::atomic<bool> loopRecordingEnabled_{false};
  std::atomic<juce::int64> loopStart_{0};
  std::atomic<juce::int64> loopEnd_{0};
  std::atomic<int> currentTakeNumber_{1};

  // Stores completed take results from previous loop cycles
  std::vector<RecordingResult> completedTakes_;

  // Recording directory for creating new take files
  juce::File recordingsDir_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};

} // namespace zenith
