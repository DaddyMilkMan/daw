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

//==============================================================================

#include <vector>
#include <memory>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

class Track;
class AuxBus;
struct AudioRenderContext;

class IAudioRenderer {
public:
    virtual ~IAudioRenderer() = default;

    //==========================================================================
    // Initialization
    //==========================================================================

    virtual void initialize(int numInputChannels, int numOutputChannels, double sampleRate) = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    //==========================================================================
    // Audio Processing
    //==========================================================================

    virtual void processAudioBlock(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        juce::int64 playheadPosition,
        const std::vector<Track*>& tracks,
        const std::vector<AuxBus*>& auxBuses,
        const juce::MidiBuffer* incomingMidi = nullptr) = 0;

    //==========================================================================
    // Rendering Context
    //==========================================================================

    virtual void setRenderContext(const AudioRenderContext& context) = 0;
    virtual const AudioRenderContext& getRenderContext() const = 0;

    //==========================================================================
    // Master Bus
    //==========================================================================

    virtual void setMasterVolume(float volume) = 0;
    virtual float getMasterVolume() const = 0;
    virtual void setMasterMute(bool muted) = 0;
    virtual bool isMasterMuted() const = 0;

    //==========================================================================
    // Plugin Management
    //==========================================================================

    virtual void addMasterPlugin(std::shared_ptr<juce::AudioPluginInstance> plugin) = 0;
    virtual void removeMasterPlugin(int index) = 0;
    virtual int getNumMasterPlugins() const = 0;
    virtual std::shared_ptr<juce::AudioPluginInstance> getMasterPlugin(int index) const = 0;

    //==========================================================================
    // Master Limiter
    //==========================================================================

    virtual void setMasterLimiterEnabled(bool enabled) = 0;
    virtual bool isMasterLimiterEnabled() const = 0;
    virtual void setMasterLimiterCeiling(float ceilingDb) = 0;
    virtual float getMasterLimiterCeiling() const = 0;
    virtual float getMasterLimiterGainReduction() const = 0;
    virtual int getMasterLimiterLatency() const = 0;

    //==========================================================================
    // Metering
    //==========================================================================

    virtual float getMasterLevel() const = 0;
    virtual float getMasterPeakLevel() const = 0;
    virtual void resetPeakMeters() = 0;

    //==========================================================================
    // Plugin Delay Compensation
    //==========================================================================

    virtual void recalculatePDC(const std::vector<Track*>& tracks) = 0;
    virtual int getTrackLatency(int trackIndex) const = 0;
    virtual int getMasterLatency() const = 0;
    virtual int getMaxTrackLatency() const = 0;
    virtual bool isPDCEnabled() const = 0;
    virtual void setPDCEnabled(bool enabled) = 0;

    //==========================================================================
    // Configuration
    //==========================================================================

    virtual void setSampleRate(double sampleRate) = 0;
    virtual void setBufferSize(int bufferSize) = 0;
    virtual void enableTestTone(bool enabled) = 0;
    virtual bool isTestToneEnabled() const = 0;
};

} // namespace zenith