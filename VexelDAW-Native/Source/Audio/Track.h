/*
  ==============================================================================

    Track.h
    Created: 2025-11-11
    Author:  Zenith DAW

    Audio/MIDI track with clip playback, plugin chain, and mixer controls

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Represents an audio or MIDI track in the DAW.

    Each track can contain multiple clips, has its own plugin chain,
    and provides mixer controls (volume, pan, mute, solo).

    This class is designed to be used from both the audio thread and the
    message thread, so all controls use atomic operations for lock-free access.
*/
class Track : public juce::AudioSource,
              public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    enum class Type
    {
        Audio,
        MIDI,
        Instrument  // MIDI track with instrument plugin
    };

    //==============================================================================
    Track(const juce::String& name, Type type);
    ~Track() override;

    //==============================================================================
    // AudioSource interface
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    //==============================================================================
    // Track properties
    const juce::String& getName() const { return trackName; }
    void setName(const juce::String& newName);

    Type getType() const { return trackType; }
    juce::String getTypeString() const;

    int getTrackIndex() const { return trackIndex; }
    void setTrackIndex(int index) { trackIndex = index; }

    //==============================================================================
    // Mixer controls (thread-safe using atomics)
    void setVolume(float newVolume);           // 0.0 to 1.0
    float getVolume() const { return volume.load(); }

    void setPan(float newPan);                 // -1.0 (left) to 1.0 (right)
    float getPan() const { return pan.load(); }

    void setMuted(bool shouldBeMuted);
    bool isMuted() const { return muted.load(); }

    void setSolo(bool shouldBeSolo);
    bool isSolo() const { return solo.load(); }

    void setArmed(bool shouldBeArmed);         // For recording
    bool isArmed() const { return armed.load(); }

    void setEnabled(bool shouldBeEnabled);
    bool isEnabled() const { return enabled.load(); }

    //==============================================================================
    // Plugin chain management
    void addPlugin(juce::AudioPluginInstance* plugin);
    void removePlugin(int pluginIndex);
    void clearPlugins();
    int getNumPlugins() const;
    juce::AudioPluginInstance* getPlugin(int index) const;

    //==============================================================================
    // Clip management
    class Clip;  // Forward declaration

    void addClip(Clip* clip);
    void removeClip(int clipIndex);
    void removeClip(Clip* clip);
    void clearClips();
    int getNumClips() const;
    Clip* getClip(int index) const;

    //==============================================================================
    // Monitoring
    float getCurrentLevel() const { return currentLevel.load(); }
    float getPeakLevel() const { return peakLevel.load(); }
    void resetPeakLevel();

    //==============================================================================
    // State management
    juce::ValueTree getState() const;
    void loadState(const juce::ValueTree& state);

private:
    //==============================================================================
    // Track properties
    juce::String trackName;
    Type trackType;
    int trackIndex = -1;

    //==============================================================================
    // Audio processing state
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    //==============================================================================
    // Mixer controls (atomic for lock-free access)
    std::atomic<float> volume{0.8f};
    std::atomic<float> pan{0.0f};
    std::atomic<bool> muted{false};
    std::atomic<bool> solo{false};
    std::atomic<bool> armed{false};
    std::atomic<bool> enabled{true};

    //==============================================================================
    // Level monitoring (atomic for lock-free access)
    std::atomic<float> currentLevel{0.0f};
    std::atomic<float> peakLevel{0.0f};

    //==============================================================================
    // Plugin chain
    juce::OwnedArray<juce::AudioPluginInstance> plugins;
    juce::CriticalSection pluginLock;

    // Buffers for plugin processing
    juce::AudioBuffer<float> pluginBuffer;

    //==============================================================================
    // Clips
    juce::OwnedArray<Clip> clips;
    juce::CriticalSection clipsLock;

    //==============================================================================
    // Helper methods
    void processPluginChain(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyGainAndPan(juce::AudioBuffer<float>& buffer, int numSamples);
    void updateLevelMeters(const juce::AudioBuffer<float>& buffer, int numSamples);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
