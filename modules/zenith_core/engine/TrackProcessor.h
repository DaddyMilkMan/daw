/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "MixerChannel.h"
#include "PluginChain.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

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
                      std::span<juce::AudioBuffer<float>* const> auxBuffers,
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

    void setSidechainSource(int pluginIndex, std::shared_ptr<Track> sourceTrack) {
        sidechainSources[pluginIndex] = sourceTrack;
    } 

    std::shared_ptr<Track> getSidechainSource(int pluginIndex) const {
        auto it = sidechainSources.find(pluginIndex);
        return it != sidechainSources.end() ? it->second.lock() : nullptr;
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

    std::unordered_map<int, std::weak_ptr<Track>> sidechainSources;
    std::vector<juce::AudioBuffer<float>*> emptyAux_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackProcessor)
};

} // namespace zenith
