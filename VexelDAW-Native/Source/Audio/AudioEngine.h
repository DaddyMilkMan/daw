/*
  ==============================================================================

    AudioEngine.h
    Core audio processing engine - replaces Web Audio API

    This is the real-time audio processor that:
    - Manages all tracks and their routing
    - Performs mixing and master bus processing
    - Handles transport control (play/stop/record)
    - Provides sample-accurate audio processing

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include "MixerChannel.h"

//==============================================================================
/**
 * Main audio engine - handles all real-time audio processing
 *
 * Converts from Web Audio API to native JUCE audio processing:
 * - Web Audio nodes → JUCE AudioSource chain
 * - ScriptProcessorNode → Native C++ processing
 * - AudioContext → AudioDeviceManager + AudioSourcePlayer
 */
class AudioEngine : public juce::AudioSource,
                    public juce::ChangeListener
{
public:
    //==============================================================================
    AudioEngine();
    ~AudioEngine() override;

    //==============================================================================
    // AudioSource interface - called by audio thread
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    //==============================================================================
    // Transport control
    void play();
    void pause();
    void stop();
    void setLooping (bool shouldLoop);

    bool isPlaying() const { return playing.load(); }
    bool isRecording() const { return recording.load(); }
    bool isLooping() const { return looping.load(); }

    //==============================================================================
    // Tempo and timing
    void setTempo (double newTempo);
    double getTempo() const { return tempo.load(); }

    void setTimeSignature (int numerator, int denominator);
    int getTimeSignatureNumerator() const { return timeSignatureNumerator.load(); }
    int getTimeSignatureDenominator() const { return timeSignatureDenominator.load(); }

    //==============================================================================
    // Track management
    Track* addTrack (const juce::String& name, Track::Type type);
    void removeTrack (int trackIndex);
    Track* getTrack (int index) const { return tracks[index]; }
    int getNumTracks() const { return tracks.size(); }
    void clearAllTracks();

    //==============================================================================
    // Master controls
    void setMasterVolume (float volume);
    float getMasterVolume() const { return masterVolume.load(); }

    void setMasterPan (float pan);
    float getMasterPan() const { return masterPan.load(); }

    //==============================================================================
    // Playback position
    double getPlayheadPosition() const { return playheadPosition.load(); }
    void setPlayheadPosition (double newPosition);

    //==============================================================================
    // CPU and performance monitoring
    double getCpuUsage() const { return cpuUsage.load(); }
    int getExpectedBlockSize() const { return blockSize; }
    double getSampleRate() const { return currentSampleRate; }

    //==============================================================================
    // ChangeListener override
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

private:
    //==============================================================================
    // Track storage
    juce::OwnedArray<Track> tracks;
    juce::CriticalSection tracksLock;  // Protects track array modifications

    //==============================================================================
    // Mixing
    juce::MixerAudioSource mixer;
    std::unique_ptr<MixerChannel> masterChannel;

    //==============================================================================
    // Transport state (atomic for lock-free access)
    std::atomic<bool> playing { false };
    std::atomic<bool> recording { false };
    std::atomic<bool> looping { false };
    std::atomic<double> playheadPosition { 0.0 };

    //==============================================================================
    // Timing (atomic for lock-free access)
    std::atomic<double> tempo { 120.0 };
    std::atomic<int> timeSignatureNumerator { 4 };
    std::atomic<int> timeSignatureDenominator { 4 };

    //==============================================================================
    // Master bus controls (atomic)
    std::atomic<float> masterVolume { 0.8f };
    std::atomic<float> masterPan { 0.0f };

    //==============================================================================
    // Audio settings
    double currentSampleRate = 44100.0;
    int blockSize = 512;

    //==============================================================================
    // Performance monitoring
    std::atomic<double> cpuUsage { 0.0 };
    juce::int64 totalSamplesProcessed = 0;

    //==============================================================================
    // Internal helper methods
    void advancePlayhead (int numSamples);
    void applyMasterProcessing (juce::AudioBuffer<float>& buffer);
    void applyMasterPanning (juce::AudioBuffer<float>& buffer);
    void updateCpuUsage (double processingTime);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
