/*
  ==============================================================================

    TrackProcessor.h
    Created: 2025
    Author:  Zenith DAW

    Audio processing logic extracted from Track class.
    Handles PluginChain, MixerChannel, and audio buffer processing.

  ==============================================================================
*/

#pragma once

#include "MixerChannel.h"
#include "PluginChain.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

class Clip; // Forward decl
class Track; // Forward decl for setSidechainSource

/**
 * @brief Handles audio processing for a Track.
 * 
 * Owned by Track. Access methods are thread-safe or intended for specific threads.
 */
class TrackProcessor {
public:
    TrackProcessor();
    ~TrackProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    juce::MidiBuffer& getEmptyMidiBuffer() noexcept;

    /**
     * @brief Process audio block. Replaces Track::getNextAudioBlock logic.
     */
    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                      juce::MidiBuffer& midiMessages,
                      const std::vector<juce::AudioBuffer<float>*>& auxBuffers,
                      const juce::AudioBuffer<float>* sidechainBuffer);

    // Convenience overload for frozen track processing
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // Accessors for Track (Model/UI) to interact with state
    MixerChannel& getMixerChannel() { return mixerChannel; }
    const MixerChannel& getMixerChannel() const { return mixerChannel; }

    PluginChain& getPluginChain() { return pluginChain; }
    const PluginChain& getPluginChain() const { return pluginChain; }
    
    // Internal buffers
    juce::AudioBuffer<float>& getPluginBuffer() { return pluginBuffer; }
    juce::AudioBuffer<float>& getSidechainBuffer() { return sidechainBuffer; }

    void setSidechainSource(int pluginIndex, Track* sourceTrack) {
        sidechainSources[pluginIndex] = sourceTrack;
    } 

    Track* getSidechainSource(int pluginIndex) const {
        auto it = sidechainSources.find(pluginIndex);
        return it != sidechainSources.end() ? it->second : nullptr;
    }

private:
    // Core components
    MixerChannel mixerChannel;
    PluginChain pluginChain;

    // Buffers
    juce::AudioBuffer<float> pluginBuffer;
    juce::AudioBuffer<float> sidechainBuffer;
    juce::MidiBuffer emptyMidi_;
    
    double currentSampleRate = 48000.0;
    int currentBlockSize = 512;

    std::unordered_map<int, Track*> sidechainSources;
    std::vector<juce::AudioBuffer<float>*> emptyAux_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackProcessor)
};

} // namespace zenith
