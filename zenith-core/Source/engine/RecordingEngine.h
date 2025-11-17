/*
  ==============================================================================

    RecordingEngine.h
    Created: 2025-11-17
    Author:  Zenith DAW

    Real-time safe audio recording engine

    Responsibilities:
    - Capture audio input from armed tracks
    - Write to temporary WAV files using lock-free FIFO
    - Create Clips on ProjectState when recording stops

    Thread Safety:
    - Audio thread calls pushAudioInput() (RT-safe, lock-free)
    - Background thread writes to disk via ThreadedWriter
    - Message thread controls start/stop

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

// Forward declarations
class ProjectState;
class Engine;

namespace zenith {

//==============================================================================
/**
    Real-time safe audio recording engine.

    Uses JUCE's ThreadedWriter for lock-free audio capture from the audio
    thread to disk via a background thread.
*/
class RecordingEngine
{
public:
    //==========================================================================
    RecordingEngine(ProjectState& projectState, Engine& engine);
    ~RecordingEngine();

    //==========================================================================
    // Recording Control (Message Thread)
    //==========================================================================

    /**
     * @brief Start recording audio input
     * @param sampleRate Current audio device sample rate
     * @param numChannels Number of input channels to record
     * @note Call from message thread only
     */
    void startRecording(double sampleRate, int numChannels);

    /**
     * @brief Stop recording and create clips on armed tracks
     * @note Call from message thread only
     * @note Creates Clip objects in ProjectState for each armed track
     */
    void stopRecording();

    /**
     * @brief Check if currently recording
     * @return true if recording is active
     */
    bool isRecording() const { return isRecording_.load(); }

    /**
     * @brief Get the sample position when recording started
     * @return Start sample position (for timeline alignment)
     */
    int64_t getRecordingStartSample() const { return recordingStartSample_; }

    //==========================================================================
    // Audio Input (Audio Thread - RT-safe!)
    //==========================================================================

    /**
     * @brief Push audio input samples to recording buffer
     * @param inputData Input audio channels
     * @param numChannels Number of input channels
     * @param numSamples Number of samples per channel
     * @note AUDIO THREAD ONLY - Real-time safe!
     * @note Uses lock-free FIFO via ThreadedWriter
     */
    void pushAudioInput(const float* const* inputData, int numChannels, int numSamples);

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    // References to core systems
    ProjectState& projectState_;
    Engine& engine_;

    // Recording state (atomic for thread-safe access)
    std::atomic<bool> isRecording_{false};

    // Recording parameters
    double recordingSampleRate_ = 44100.0;
    int recordingNumChannels_ = 2;
    int64_t recordingStartSample_ = 0;  // Timeline position where recording started
    std::atomic<int64_t> recordedSampleCount_{0};  // Total samples recorded

    // Temporary recording file
    juce::File tempRecordingFile_;
    std::unique_ptr<juce::WavAudioFormat> wavFormat_;
    std::unique_ptr<juce::AudioFormatWriter> formatWriter_;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter_;

    // Background thread for writing audio to disk
    juce::TimeSliceThread recordingThread_{"Recording Thread"};

    // Helper methods
    void createClipsFromRecording();
    juce::File getTempRecordingFile();

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingEngine)
};

} // namespace zenith
