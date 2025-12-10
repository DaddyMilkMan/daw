/*
  ==============================================================================

    AudioRecorder.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Handles audio file recording streams and threading.
    Extracted from Engine to improve SRP.

  ==============================================================================
*/

#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <atomic>

namespace zenith {

// Forward declarations
class Track;

class AudioRecorder {
public:
    AudioRecorder();
    ~AudioRecorder();

    void prepare(double sampleRate);
    void startRecording(const std::vector<std::shared_ptr<Track>>& tracks, 
                       const juce::AudioDeviceManager& deviceManager,
                       juce::int64 startSample,
                       const juce::File& recordingsDir);
    
    // Returns list of created file paths and their track indices
    struct RecordingResult {
        juce::File file;
        int trackIndex;
        juce::int64 startSample;
        double sampleRate;
    };
    
    std::vector<RecordingResult> stopRecording();

    bool isRecording() const { return isRecording_.load(); }
    
    // Audio Thread: Process input block
    void processBlock(const float* const* inputChannelData, int numInputChannels, int numSamples);

private:
    std::unique_ptr<juce::TimeSliceThread> writerThread_;
    std::atomic<bool> isRecording_{ false };
    
    struct ActiveSession {
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
        juce::File file;
        int trackIndex;
        int inputChannelIndex;
        juce::int64 startSample;
        double sampleRate;
    };
    
    std::vector<ActiveSession> activeSessions_;
    
    // Temporary buffer for copying inputs
    juce::AudioBuffer<float> tempBuffer_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};

} // namespace zenith
