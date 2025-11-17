/*
  ==============================================================================

    Track.h
    Ported from: VexelDAW-Native/Source/Audio/Track.h (2025-11-11)
    Author:  Vexel DAW → Zenith DAW

    Audio/MIDI track with clip playback, plugin chain, and mixer controls

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - OwnedArray<Clip> → std::vector<std::unique_ptr<Clip>>
    - Plugin hosting stubbed for Phase 2

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace zenith {

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

    const juce::String& getTrackId() const { return trackId; }
    void setTrackId(const juce::String& id) { trackId = id; }

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
    void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
    void removePlugin(int pluginIndex);
    void clearPlugins();
    int getNumPlugins() const;
    juce::AudioPluginInstance* getPlugin(int index) const;

    //==============================================================================
    // MIDI Scheduling
    /**
     * @brief Generate MIDI events for the current audio block from NOTES in clips
     * @param trackState ValueTree for this track from ProjectState
     * @param tempo Current tempo in BPM
     * @param sampleRate Current sample rate
     * @param blockStartSample Transport position at start of block
     * @param blockSize Number of samples in block
     * @param midiOut MIDI buffer to fill with events
     */
    void generateMidiForBlock(const juce::ValueTree& trackState,
                               double tempo,
                               double sampleRate,
                               juce::int64 blockStartSample,
                               int blockSize,
                               juce::MidiBuffer& midiOut);

    //==============================================================================
    // Clip management
    class Clip;  // Forward declaration

    void addClip(std::unique_ptr<Clip> clip);
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
    juce::String trackId;
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
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> plugins;
    juce::CriticalSection pluginLock;
    juce::AudioBuffer<float> pluginBuffer;
    juce::MidiBuffer midiBuffer;  // For MIDI events to plugins

    //==============================================================================
    // Clips (JUCE 8 adaptation: OwnedArray → std::vector<std::unique_ptr<>>)
    std::vector<std::unique_ptr<Clip>> clips;
    juce::CriticalSection clipsLock;

    //==============================================================================
    // Helper methods
    void processPluginChain(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples);
    void applyGainAndPan(juce::AudioBuffer<float>& buffer, int numSamples);
    void updateLevelMeters(const juce::AudioBuffer<float>& buffer, int numSamples);

    // MIDI Scheduler state
    struct ActiveNote
    {
        int pitch;
        int channel;
        juce::String noteId;  // For tracking which ValueTree note this came from
    };
    std::vector<ActiveNote> activeNotes;
    juce::int64 lastProcessedSample = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
