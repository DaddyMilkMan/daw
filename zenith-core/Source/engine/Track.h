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

// Forward declarations
namespace zenith {
    class Instrument;
}

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
    // Instrument management (for Instrument tracks)
    /**
     * @brief Set the instrument for this track
     * @param instrument Instrument instance (must be non-null)
     * @note Message thread only
     */
    void setInstrument(std::unique_ptr<Instrument> instrument);

    /**
     * @brief Get the current instrument (if any)
     * @return Pointer to instrument, or nullptr if no instrument set
     * @note Message thread only
     */
    Instrument* getInstrument() const { return instrument_.get(); }

    /**
     * @brief Check if track has an instrument
     */
    bool hasInstrument() const { return instrument_ != nullptr; }

    //==============================================================================
    // Plugin chain management - TODO(Phase 2: plugin hosting)
    // Stubbed for now; will implement in Phase 2 with VST3/AU support
    void addPlugin(void* plugin) { (void)plugin; /* stub */ }
    void removePlugin(int pluginIndex) { (void)pluginIndex; /* stub */ }
    void clearPlugins() { /* stub */ }
    int getNumPlugins() const { return 0; }
    void* getPlugin(int index) const { (void)index; return nullptr; }

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
    // Instrument (for Instrument tracks)
    std::unique_ptr<Instrument> instrument_;
    juce::AudioBuffer<float> instrumentBuffer_;
    juce::MidiBuffer midiBuffer_;

    //==============================================================================
    // Plugin chain - TODO(Phase 2: plugin hosting)
    // Placeholder for future VST3/AU hosting
    juce::CriticalSection pluginLock;
    juce::AudioBuffer<float> pluginBuffer;

    //==============================================================================
    // Preallocated buffer for clip processing (RT-safe, no allocation on audio thread)
    juce::AudioBuffer<float> clipBuffer_;

    //==============================================================================
    // Clips (JUCE 8 adaptation: OwnedArray → std::vector<std::unique_ptr<>>)
    std::vector<std::unique_ptr<Clip>> clips;
    juce::CriticalSection clipsLock;

    //==============================================================================
    // Helper methods
    void processPluginChain(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyGainAndPan(juce::AudioBuffer<float>& buffer, int numSamples);
    void updateLevelMeters(const juce::AudioBuffer<float>& buffer, int numSamples);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
