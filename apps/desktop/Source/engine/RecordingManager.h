/*
  ==============================================================================

    RecordingManager.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Handles audio and MIDI recording operations.
    
    Extracted from Engine.cpp for better modularity.
    
    Thread Safety:
    - startRecording()/stopRecording() are MESSAGE THREAD ONLY
    - captureAudio() is AUDIO THREAD SAFE (RT-safe)
    - Uses lock-free fifos for RT-safe recording

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include <memory>
#include <map>

#include "EngineConstants.h"

namespace zenith {

// Forward declarations
class Track;
class ProjectState;

//==============================================================================
/**
    Audio recording session data.
*/
struct AudioRecordingSession {
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
    juce::File file;
    int numChannels = 2;
    double sampleRate = constants::kDefaultSampleRate;
    int trackIndex = -1;
    juce::int64 startSamplePosition = 0;
    juce::int64 samplesRecorded = 0;
    bool isActive = false;
};

//==============================================================================
/**
    MIDI recording session data.
*/
struct MidiRecordingSession {
    juce::MidiMessageSequence sequence;
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
    void setProjectState(ProjectState* state) { projectState_ = state; }

    /**
     * @brief Prepare for playback
     * @param sampleRate Current sample rate
     */
    void prepare(double sampleRate);

    //==========================================================================
    // Recording Control
    //==========================================================================

    /**
     * @brief Prepare recording for a specific track (async)
     * @param track Track to prepare
     * @param trackIndex Track index
     * @param recordDir Directory for recordings
     * @note MESSAGE THREAD ONLY
     */
    void prepareRecordingForTrack(Track& track, int trackIndex, 
                                  const juce::File& recordDir);

    /**
     * @brief Start recording on all armed tracks
     * @param startPosition Starting sample position
     * @param tracks Vector of tracks
     * @note MESSAGE THREAD ONLY
     */
    void startRecording(juce::int64 startPosition,
                        const std::vector<std::shared_ptr<Track>>& tracks);

    /**
     * @brief Stop all recording and finalize clips
     * @param tracks Vector of tracks
     * @note MESSAGE THREAD ONLY
     */
    void stopRecording(const std::vector<std::shared_ptr<Track>>& tracks);

    /**
     * @brief Check if recording is active
     */
    bool isRecording() const { return isRecording_.load(); }

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
    void captureAudio(const float* const* inputData, 
                      int numInputChannels,
                      int numSamples,
                      const std::vector<std::shared_ptr<Track>>& tracks);

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
    void captureMidi(const juce::MidiMessage& message, 
                     juce::int64 samplePosition,
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
    void finalizeRecordings(const std::vector<std::shared_ptr<Track>>& tracks);

    /**
     * @brief Create unique recording filename
     */
    juce::File createRecordingFile(const juce::File& dir, 
                                   const juce::String& trackName,
                                   const juce::String& extension);

    //==========================================================================
    // State
    //==========================================================================

    double sampleRate_ = constants::kDefaultSampleRate;
    std::atomic<bool> isRecording_{false};

    // Writer thread (owned by RecordingManager)
    std::unique_ptr<juce::TimeSliceThread> writerThread_;

    // Project state for clip creation (owned by Engine)
    ProjectState* projectState_ = nullptr;

    // Active recording sessions
    std::vector<AudioRecordingSession> audioSessions_;
    std::vector<MidiRecordingSession> midiSessions_;

    // Prepared (pre-punched) sessions
    std::vector<AudioRecordingSession> preppedSessions_;

    // Lock-free MIDI recording fifo
    struct MidiFifoEntry {
        juce::MidiMessage message;
        juce::int64 samplePosition;
        int trackIndex;
    };
    juce::AbstractFifo midiFifoIndex_{constants::kMidiRecordFifoSize};
    std::vector<MidiFifoEntry> midiFifoData_;

    // Recording directory
    juce::File recordingDirectory_;

    // Thread safety
    juce::CriticalSection sessionLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingManager)
};

} // namespace zenith
