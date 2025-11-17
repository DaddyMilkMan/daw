/*
  ==============================================================================

    RecordingManager.h
    Created: 2025-11-17
    Author:  Zenith DAW

    Real-time safe audio recording manager

    RT-safe recording pipeline:
    1. Audio callback pushes input to ring buffer (lock-free)
    2. Background thread pulls from ring buffer and writes to AudioBuffer
    3. On stop: creates Clip with recorded audio and adds to armed track

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>

namespace zenith {

// Forward declarations
class ProjectState;
class Track;

//==============================================================================
/**
 * Manages real-time audio recording with lock-free ring buffer
 */
class RecordingManager
{
public:
    //==============================================================================
    RecordingManager();
    ~RecordingManager();

    //==============================================================================
    // Recording Control (called from message thread)

    /**
     * Start recording on armed tracks
     * @param sampleRate Current audio sample rate
     * @param currentBeat Current playhead position in beats (for clip positioning)
     * @param samplePosition Current sample position in transport
     */
    void startRecording(double sampleRate, double currentBeat, int64_t samplePosition);

    /**
     * Stop recording and create clips
     * @param projectState ProjectState to add recorded clips to
     * @param armedTracks List of armed tracks to add clips to
     * @return Number of clips created
     */
    int stopRecording(ProjectState* projectState, const std::vector<Track*>& armedTracks);

    /**
     * Check if currently recording
     */
    bool isRecording() const { return isRecording_.load(); }

    //==============================================================================
    // Audio Thread Interface (RT-safe)

    /**
     * Push input audio from audio callback to ring buffer
     * ⚠️ MUST BE REAL-TIME SAFE - called from audio thread!
     *
     * @param inputChannelData Array of input channel pointers
     * @param numInputChannels Number of input channels
     * @param numSamples Number of samples to record
     */
    void pushInputFromAudioCallback(
        const float* const* inputChannelData,
        int numInputChannels,
        int numSamples);

    //==============================================================================
    // Background Processing (called from timer on message thread)

    /**
     * Process recorded audio from ring buffer
     * Drains the ring buffer and writes to in-memory buffer
     */
    void processRecordedAudio();

private:
    //==============================================================================
    // Ring Buffer for lock-free audio transfer
    static constexpr int RING_BUFFER_SIZE = 88200 * 10; // 10 seconds at 44.1kHz

    juce::AbstractFifo fifo_;
    juce::AudioBuffer<float> ringBuffer_;

    //==============================================================================
    // Recording State
    std::atomic<bool> isRecording_{false};
    std::atomic<double> recordingSampleRate_{44100.0};

    double recordingStartBeat_{0.0};
    int64_t recordingStartSample_{0};
    int recordedSampleCount_{0};

    // In-memory buffer for recorded audio
    juce::AudioBuffer<float> recordedAudioBuffer_;
    int maxRecordedChannels_{2};

    //==============================================================================
    // Thread Safety
    juce::CriticalSection recordingStateLock_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingManager)
};

} // namespace zenith
