/*
  ==============================================================================

    RecordingManager.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Manages audio and MIDI recording operations with RT-safe architecture.

    Thread Safety:
    - startRecording()/stopRecording() are MESSAGE THREAD ONLY
    - captureAudio() is AUDIO THREAD SAFE (RT-safe via AudioRecorder)
    - captureMidi() is AUDIO THREAD SAFE (RT-safe via lock-free fifo)
    - Uses lock-free fifos for RT-safe recording

    Architecture:
    - Delegates audio recording to AudioRecorder (ring buffer based)
    - Manages MIDI recording via lock-free fifo
    - Creates clips in ProjectState upon stopRecording()

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <map>
#include <memory>
#include <vector>

#include "EngineConstants.h"

namespace zenith {

// Forward declarations
class Track;
class ProjectState;
class AudioInputListener {
public:
  virtual ~AudioInputListener() = default;

  /**
   * @brief Called when audio input is received
   * @param inputData Input channel data (array of float*)
   * @param numInputChannels Number of input channels
   * @param numSamples Number of samples
   * @note AUDIO THREAD - Real-time safe!
   */
  virtual void onAudioInput(const float *const *inputData, int numInputChannels,
                            int numSamples) = 0;
};

class AudioRecorder;

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
   * @note MESSAGE THREAD ONLY
   */
  void stopRecording(const std::vector<std::shared_ptr<Track>> &tracks);

  /**
   * @brief Check if recording is active
   */
  bool isRecording() const { return isRecording_.load(); }

  //==========================================================================
  // Listeners
  //==========================================================================

  void addAudioInputListener(AudioInputListener *listener);
  void removeAudioInputListener(AudioInputListener *listener);
  bool hasActiveListeners() const { return hasListeners_.load(); }

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

private:
  //==========================================================================
  // Internal Methods
  //==========================================================================

  /**
   * @brief Create clips from completed recordings
   */
  void finalizeRecordings(const std::vector<std::shared_ptr<Track>> &tracks);

  /**
   * @brief Create an audio clip in ProjectState
   */
  void createAudioClip(const juce::File &audioFile, const juce::String &trackId,
                       juce::int64 startSamplePosition,
                       juce::int64 lengthSamples, double sampleRate);

  /**
   * @brief Create a MIDI clip in ProjectState
   */
  void createMidiClip(const juce::MidiMessageSequence &sequence,
                      const juce::String &trackId,
                      juce::int64 startSamplePosition, double sampleRate);

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
  // Thread safety for session modification
  juce::CriticalSection sessionLock_;

  // Listeners
  juce::CriticalSection listenerLock_;
  std::vector<AudioInputListener *> listeners_;
  std::atomic<bool> hasListeners_{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingManager)
};

} // namespace zenith
