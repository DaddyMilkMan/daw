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

// Forward declaration
class PluginHost;

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

    // Phase 1.3: Version that takes explicit playhead position
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, int64_t playheadSamples);

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
    // Plugin chain management (Phase 3: VST3 hosting MVP)
    // MESSAGE THREAD ONLY for add/remove/clear
    // Audio thread can process existing plugins safely (no modifications during playback)
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

    /**
     * @brief Load plugin states from ValueTree
     *
     * This must be called AFTER loadState() and requires access to PluginHost
     * to recreate plugin instances.
     *
     * @param state The track state ValueTree
     * @param pluginHost Reference to PluginHost for plugin instantiation
     */
    void loadPluginStates(const juce::ValueTree& state, PluginHost& pluginHost);

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
    // Instrument (for Instrument tracks)
    std::unique_ptr<Instrument> instrument_;
    juce::AudioBuffer<float> instrumentBuffer_;
    juce::MidiBuffer midiBuffer_;

    //==============================================================================
    // Plugin chain (Phase 3: VST3 hosting MVP)
    // Plugins are modified on message thread, processed on audio thread
    // No lock needed during processing (plugins vector is only modified on message thread when stopped)
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> plugins;
    juce::CriticalSection pluginLock;  // Only for add/remove operations
    juce::AudioBuffer<float> pluginBuffer;
    juce::MidiBuffer midiBuffer;  // For MIDI events to plugins
    juce::MidiBuffer pluginMidiBuffer;  // Temp MIDI buffer for plugin processing

    //==============================================================================
    // Phase 2A: Lock-free clip list using RCU-style atomic snapshot
    //
    // Pattern:
    // - Track owns clips via std::vector<std::unique_ptr<Clip>> (message thread only)
    // - ClipSnapshot holds raw Clip* pointers for audio thread to iterate
    // - Audio thread loads snapshot atomically, iterates without locking
    // - Message thread creates new snapshot when modifying clips, swaps atomically
    //
    // This eliminates clipsLock from the audio thread (RT-safe).

    struct ClipSnapshot
    {
        std::vector<Clip*> clips;  // Raw pointers (non-owning)

        ClipSnapshot() = default;
        explicit ClipSnapshot(const std::vector<std::unique_ptr<Clip>>& ownedClips)
        {
            clips.reserve(ownedClips.size());
            for (const auto& clip : ownedClips)
                clips.push_back(clip.get());
        }
    };

    // Clip ownership (message thread only)
    std::vector<std::unique_ptr<Clip>> clipsOwned_;

    // Atomic snapshot for audio thread (RT-safe read)
    std::atomic<std::shared_ptr<const ClipSnapshot>> clipsSnapshot_;

    // Helper: Create new snapshot from current ownership
    void updateClipSnapshot();

    // Phase 1: Pre-allocated clip buffer to avoid RT allocations
    juce::AudioBuffer<float> clipBuffer_;

    // Phase 2A: Pre-allocated MIDI buffer for MIDI clip playback
    juce::MidiBuffer midiBuffer_;

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
