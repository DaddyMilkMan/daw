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

// MixerEngine.h

#include <atomic>
#include <memory>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace zenith {

// Forward declarations
class Track;
class MixerChannel;
class AuxBus;
class MasterLimiter;
class MeteringSystem;

//==============================================================================
/**
 * @class MixerEngine
 // Brief: Audio mixing and processing engine
 * 
 * Handles all audio mixing, routing, and effects processing
 * with RT-safe operation.
 */
class MixerEngine {
public:
    //==========================================================================
    MixerEngine();
    ~MixerEngine();

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
     // Brief: Initialize the mixer
     * @param sampleRate Sample rate
     * @param bufferSize Buffer size
     * @param numOutputChannels Number of output channels
     */
    bool initialize(double sampleRate, int bufferSize, int numOutputChannels = 2);

    /**
     // Brief: Prepare for processing
     */
    void prepareToPlay(double sampleRate, int bufferSize);

    /**
     // Brief: Shutdown the mixer
     */
    void shutdown();

    //==========================================================================
    // Channel Management
    //==========================================================================

    /**
     // Brief: Add a track to the mix
     */
    void addTrack(std::shared_ptr<Track> track);

    /**
     // Brief: Remove a track from the mix
     */
    void removeTrack(const juce::String& trackId);

    /**
     // Brief: Get mixer channel for a track
     */
    std::shared_ptr<MixerChannel> getMixerChannel(const juce::String& trackId) const;

    //==========================================================================
    // Aux Bus Management
    //==========================================================================

    /**
     // Brief: Create an aux bus
     */
    std::shared_ptr<AuxBus> createAuxBus(const juce::String& name, int numChannels = 2);

    /**
     // Brief: Remove an aux bus
     */
    void removeAuxBus(const juce::String& busId);

    /**
     // Brief: Get aux bus by ID
     */
    std::shared_ptr<AuxBus> getAuxBus(const juce::String& busId) const;

    //==========================================================================
    // Master Processing
    //==========================================================================

    /**
     // Brief: Set master output level
     */
    void setMasterLevel(float level);

    /**
     // Brief: Get master output level
     */
    float getMasterLevel() const { return masterLevel_.load(); }

    /**
     // Brief: Enable/disable master limiter
     */
    void setMasterLimiterEnabled(bool enabled);

    /**
     // Brief: Check if master limiter is enabled
     */
    bool isMasterLimiterEnabled() const { return masterLimiterEnabled_.load(); }

    //==========================================================================
    // Audio Processing
    //==========================================================================

    /**
     // Brief: Process audio through the mixer (RT-safe)
     */
    void processAudio(const float** inputChannels,
                      float** outputChannels,
                      int numInputChannels,
                      int numOutputChannels,
                      int numSamples);

    /**
     // Brief: Process a single track's audio
     */
    void processTrack(const std::shared_ptr<Track>& track,
                     float** outputChannels,
                     int numOutputChannels,
                     int numSamples);

    //==========================================================================
    // Metering and Monitoring
    //==========================================================================

    /**
     // Brief: Get current CPU usage
     */
    float getCpuUsage() const { return cpuUsage_.load(); }

    /**
     // Brief: Get master output levels
     */
    std::pair<float, float> getMasterLevels() const;

    /**
     // Brief: Get peak levels for a track
     */
    std::pair<float, float> getTrackLevels(const juce::String& trackId) const;

    //==========================================================================
    // Routing
    //==========================================================================

    /**
     // Brief: Route track to aux bus
     */
    void routeToAux(const juce::String& trackId, const juce::String& auxBusId, float level = 1.0f);

    /**
     // Brief: Remove aux routing
     */
    void removeAuxRouting(const juce::String& trackId, const juce::String& auxBusId);

    /**
     // Brief: Set track output routing
     */
    void setTrackOutput(const juce::String& trackId, const std::vector<std::string>& outputDestinations);

private:
    //==========================================================================
    // Internal Processing
    //==========================================================================

    void processMasterSection(float** outputChannels, int numOutputChannels, int numSamples);
    void updateMetering(float** outputChannels, int numOutputChannels, int numSamples);
    void processAuxBuses(int numSamples);
    void clearBuffers(float** outputChannels, int numOutputChannels, int numSamples);

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Mixer channels (one per track)
    std::vector<std::shared_ptr<MixerChannel>> mixerChannels_;

    // Aux buses
    std::vector<std::shared_ptr<AuxBus>> auxBuses_;

    // Master section
    std::atomic<float> masterLevel_{1.0f};
    std::atomic<bool> masterLimiterEnabled_{true};
    std::unique_ptr<MasterLimiter> masterLimiter_;

    // Metering system
    std::unique_ptr<MeteringSystem> meteringSystem_;

    // Processing parameters
    double currentSampleRate_ = 48000.0;
    int currentBufferSize_ = 512;
    int numOutputChannels_ = 2;

    // Performance monitoring
    std::atomic<float> cpuUsage_{0.0f};
    juce::int64 lastProcessTime_ = 0;

    // Temporary buffers
    juce::AudioBuffer<float> mixBuffer_;
    juce::AudioBuffer<float> auxBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerEngine)
};

} // namespace zenith
