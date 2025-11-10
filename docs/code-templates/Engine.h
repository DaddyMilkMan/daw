/*
  ==============================================================================

    Engine.h
    Created: 2025-11-10

    Core audio engine for Zenith DAW.
    Manages the audio graph, device I/O, and audio thread processing.

    THREAD SAFETY:
    - Audio callbacks run on the audio thread (real-time priority)
    - UI/command methods run on the message thread
    - Never lock, allocate, or block on the audio thread!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
    The ZenithEngine class manages all audio processing and I/O.

    Architecture:
    - Owns the AudioDeviceManager (I/O setup)
    - Owns the AudioProcessorGraph (mixer, plugins, routing)
    - Reads from ProjectState (double-buffered, lock-free)
    - Writes audio/MIDI data to disk via background threads

    Thread Model:
    - Audio Thread: getNextAudioBlock(), processBlock() callbacks
    - Message Thread: All public methods (addTrack, loadPlugin, etc.)
*/
class ZenithEngine : public juce::AudioIODeviceCallback
{
public:
    //==========================================================================
    ZenithEngine();
    ~ZenithEngine() override;

    //==========================================================================
    // Audio Device Management (call from message thread)

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    /** Changes the audio device settings. Safe to call from message thread. */
    void setAudioDeviceSetup(const juce::AudioDeviceManager::AudioDeviceSetup& setup);

    //==========================================================================
    // Transport Control (thread-safe)

    void play();
    void stop();
    void togglePlayback();

    bool isPlaying() const { return playing.load(std::memory_order_relaxed); }

    void setPlayheadPosition(double timeInSeconds);
    double getPlayheadPosition() const { return playheadPosition.load(std::memory_order_relaxed); }

    //==========================================================================
    // Project State Access

    ProjectState& getProjectState() { return projectState; }
    const ProjectState& getProjectState() const { return projectState; }

    bool hasUnsavedChanges() const { return projectState.hasUnsavedChanges(); }

    //==========================================================================
    // Audio Graph Management (call from message thread)

    /** Adds a new audio track to the mixer. Returns track ID. */
    juce::String addTrack(const juce::String& trackName, int numChannels);

    /** Removes a track by ID. */
    void removeTrack(const juce::String& trackId);

    /** Loads a plugin and inserts it into the specified track. */
    bool loadPlugin(const juce::String& trackId, const juce::File& pluginFile);

    //==========================================================================
    // AudioIODeviceCallback Implementation (AUDIO THREAD - REAL-TIME SAFE!)

    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    //==========================================================================
    // REAL-TIME AUDIO THREAD METHODS (lock-free, no allocations!)

    /**
        Process the audio graph for one buffer.
        CALLED ON AUDIO THREAD - MUST BE REAL-TIME SAFE!
    */
    void processAudioGraph(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples
    );

    /**
        Update playhead position.
        CALLED ON AUDIO THREAD - MUST BE REAL-TIME SAFE!
    */
    void updatePlayhead(int numSamples);

    //==========================================================================
    // Audio infrastructure
    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<juce::AudioProcessorGraph> audioGraph;

    // Project state (message thread writes, audio thread reads snapshots)
    ProjectState projectState;

    // Transport state (lock-free atomics)
    std::atomic<bool> playing { false };
    std::atomic<double> playheadPosition { 0.0 };
    std::atomic<double> sampleRate { 44100.0 };
    std::atomic<int> blockSize { 512 };

    // Graph node references (for quick lookup)
    juce::AudioProcessorGraph::NodeID audioInputNode;
    juce::AudioProcessorGraph::NodeID audioOutputNode;
    juce::AudioProcessorGraph::NodeID midiInputNode;

    // Lock-free communication between threads
    // TODO: Add FIFO for parameter changes, meter data, etc.
    // juce::AbstractFifo parameterFifo;
    // juce::AbstractFifo meterDataFifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithEngine)
};
