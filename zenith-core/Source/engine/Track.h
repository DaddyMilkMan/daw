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
    // VST3 hosting with RT-safe processing
    //
    // NOTE: All plugin management methods MUST be called from MESSAGE THREAD only!
    // The audio thread only reads the plugin chain during processPluginChain().
    //
    // TODO: Future enhancements:
    // - AU support (macOS)
    // - Plugin preset/parameter persistence
    // - Plugin GUI management
    // - Plugin scanning UI

    /**
     * @brief Add a plugin to the track's plugin chain
     * @param pluginDescription Plugin description from format manager
     * @param formatManager Plugin format manager to create instance
     * @param sampleRate Current sample rate
     * @param blockSize Current block size
     * @param errorMessage Output error message if loading fails
     * @return true if plugin was added successfully
     * @note MESSAGE THREAD ONLY! Not RT-safe.
     */
    bool addPlugin(const juce::PluginDescription& pluginDescription,
                   juce::AudioPluginFormatManager& formatManager,
                   double sampleRate,
                   int blockSize,
                   juce::String& errorMessage);

    /**
     * @brief Remove a plugin from the chain
     * @param pluginIndex Index of plugin to remove (0-based)
     * @note MESSAGE THREAD ONLY! Not RT-safe.
     */
    void removePlugin(int pluginIndex);

    /**
     * @brief Clear all plugins from the chain
     * @note MESSAGE THREAD ONLY! Not RT-safe.
     */
    void clearPlugins();

    /**
     * @brief Get number of plugins in the chain
     * @return Plugin count
     * @note Thread-safe (uses lock)
     */
    int getNumPlugins() const;

    /**
     * @brief Set plugin bypass state
     * @param pluginIndex Index of plugin (0-based)
     * @param bypassed true to bypass, false to enable
     * @note MESSAGE THREAD ONLY for now (could be made RT-safe with atomic)
     */
    void setPluginBypassed(int pluginIndex, bool bypassed);

    /**
     * @brief Check if plugin is bypassed
     * @param pluginIndex Index of plugin (0-based)
     * @return true if bypassed
     * @note Thread-safe (uses lock)
     */
    bool isPluginBypassed(int pluginIndex) const;

    /**
     * @brief Get plugin description for persistence
     * @param pluginIndex Index of plugin (0-based)
     * @return Plugin description (empty if invalid index)
     * @note Thread-safe (uses lock)
     */
    juce::PluginDescription getPluginDescription(int pluginIndex) const;

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
     * @brief Load plugins from saved state
     * @param state ValueTree containing plugin state (from getState())
     * @param formatManager Plugin format manager to create instances
     * @param sampleRate Current sample rate
     * @param blockSize Current block size
     * @note MESSAGE THREAD ONLY! Call after loadState() to instantiate plugins.
     * @note This is a workaround because loadState() doesn't have access to Engine.
     */
    void loadPluginsFromState(const juce::ValueTree& state,
                              juce::AudioPluginFormatManager& formatManager,
                              double sampleRate,
                              int blockSize);

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
    // RT-safe design: plugins are created/destroyed on MESSAGE THREAD only,
    // and only read (processBlock) on AUDIO THREAD
    struct PluginSlot
    {
        std::unique_ptr<juce::AudioPluginInstance> instance;
        juce::PluginDescription description;
        bool bypassed = false;
    };

    std::vector<PluginSlot> pluginChain;
    juce::CriticalSection pluginLock;
    juce::AudioBuffer<float> pluginBuffer;

    // TODO: Future enhancement - store plugin state (presets/parameters)
    // For now, we only persist plugin identifiers for reloading

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
