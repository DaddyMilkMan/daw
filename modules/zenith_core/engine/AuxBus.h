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
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include "PluginChain.h"



namespace zenith {

/**
 * @class AuxBus
 * @brief Auxiliary send/return bus for effects processing
 *
 * An AuxBus receives audio from multiple tracks via sends, processes it
 * through an effect chain, and returns it to the master mix.
 *
 * Typical uses:
 * - Reverb send (multiple tracks -> reverb -> master)
 * - Delay send (drum tracks -> delay -> master)
 * - Parallel compression (all tracks -> compressor -> master)
 */
class AuxBus : public juce::AudioSource {
public:
  AuxBus(const juce::String &name);
  ~AuxBus() override;

  //==============================================================================
  // AudioSource interface
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void releaseResources() override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;

  //==============================================================================
  // Properties
  [[nodiscard]] const juce::String &getName() const { return name_; }
  void setName(const juce::String &newName) { name_ = newName; }
  
  [[nodiscard]] const juce::String &getId() const { return id_; }
  void setId(const juce::String &newId) { id_ = newId; }

  int getBusIndex() const { return busIndex_; }
  void setBusIndex(int index) { busIndex_ = index; }

  //==============================================================================
  // Mixer controls (delegated to MixerChannel)
  //==============================================================================
  // Mixer controls (delegated to MixerChannel)
  void setVolume(float volume) { mixerChannel.setVolume(volume); }
  [[nodiscard]] float getVolume() const { return mixerChannel.getVolume(); }

  void setPan(float pan) { mixerChannel.setPan(pan); }
  [[nodiscard]] float getPan() const { return mixerChannel.getPan(); }

  void setMuted(bool muted) { mixerChannel.setMuted(muted); }
  [[nodiscard]] bool isMuted() const { return mixerChannel.isMuted(); }

  //==============================================================================
  // Plugin chain management
  void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
  void removePlugin(int pluginIndex);
  void clearPlugins();
  int getNumPlugins() const;
  juce::AudioPluginInstance *getPlugin(int index) const;

  //==============================================================================
  // Metering
  [[nodiscard]] float getCurrentLevel() const { return mixerChannel.getOutputLevel(); }
  [[nodiscard]] float getPeakLevel() const { return mixerChannel.getOutputPeak(); }
  void resetPeakMeters() { mixerChannel.resetPeaks(); }

  //==============================================================================
  // Direct buffer access for send accumulation
  juce::AudioBuffer<float> &getInputBuffer() { return inputBuffer_; }

private:
  juce::String name_;
  juce::String id_;
  int busIndex_ = -1;
  MixerChannel mixerChannel;

  // Input buffer for accumulating sends from tracks
  juce::AudioBuffer<float> inputBuffer_;

  // Plugin chain (effect processors)
  PluginChain pluginChain;
  juce::MidiBuffer emptyMidi_;
  // pluginLock_ is handled inside PluginChain

  // Processing state
  double currentSampleRate_ = 44100.0;
  int currentBlockSize_ = 512;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxBus)
};

} // namespace zenith
