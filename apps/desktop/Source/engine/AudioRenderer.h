/*
  ==============================================================================

    AudioRenderer.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Handles audio graph rendering and mixing for the engine.
    
    Extracted from Engine.cpp for better modularity.
    
    Thread Safety:
    - renderAudioGraph() is AUDIO THREAD ONLY
    - All methods are RT-safe (no allocations, no locks)

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>
#include <memory>

#include "EngineConstants.h"
#include "RoutingGraph.h"

namespace zenith {

// Forward declarations
class Track;
class AuxBus;
class MasterLimiter;
class TempoMap;

//==============================================================================
/**
    Audio graph renderer for the engine.
    
    Handles the core audio rendering pipeline:
    - Track rendering with plugin processing
    - Aux bus processing and mixing
    - Master bus processing with limiter
    - PDC (plugin delay compensation)
*/
class AudioRenderer {
public:
    //==========================================================================
    AudioRenderer() = default;
    ~AudioRenderer() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Prepare renderer for playback
     * @param sampleRate Current sample rate
     * @param blockSize Maximum block size
     * @param numTracks Number of tracks to prepare buffers for
     * @param numAuxBuses Number of aux buses
     */
    void prepare(double sampleRate, int blockSize, 
                 size_t numTracks, size_t numAuxBuses);

    /**
     * @brief Reset renderer state
     */
    void reset();

    //==========================================================================
    // Rendering
    //==========================================================================

    /**
     * @brief Render the audio graph to output buffer
     * @param outputBuffer Output buffer to fill
     * @param numSamples Number of samples to render
     * @param playheadPosition Current playhead position in samples
     * @param tracks Vector of tracks to render
     * @param auxBuses Vector of aux buses
     * @param routingGraph Routing graph for signal flow
     * @param masterLimiter Master bus limiter
     * @param masterPlugins Master bus plugin chain
     * @param tempoMap Tempo map for automation
     * @param incomingMidi Optional incoming MIDI buffer
     * @note AUDIO THREAD ONLY
     */
    void renderAudioGraph(
        juce::AudioBuffer<float>& outputBuffer,
        int numSamples,
        juce::int64 playheadPosition,
        const std::vector<std::shared_ptr<Track>>& tracks,
        const std::vector<std::shared_ptr<AuxBus>>& auxBuses,
        const RoutingGraph& routingGraph,
        MasterLimiter& masterLimiter,
        std::vector<std::unique_ptr<juce::AudioPluginInstance>>& masterPlugins,
        const TempoMap* tempoMap,
        const juce::MidiBuffer* incomingMidi = nullptr);

    //==========================================================================
    // PDC (Plugin Delay Compensation)
    //==========================================================================

    /**
     * @brief Calculate and update PDC for all tracks
     * @param tracks Vector of tracks
     * @return Maximum latency in samples
     */
    int calculatePDC(const std::vector<std::shared_ptr<Track>>& tracks);

    /**
     * @brief Enable/disable PDC
     */
    void setPDCEnabled(bool enabled) { pdcEnabled_.store(enabled); }
    bool isPDCEnabled() const { return pdcEnabled_.load(); }

    /**
     * @brief Get maximum track latency
     */
    int getMaxTrackLatency() const { return maxTrackLatency_.load(); }

    //==========================================================================
    // Metering
    //==========================================================================

    /**
     * @brief Get current master output level
     */
    float getMasterLevel() const { return masterLevel_.load(); }

    /**
     * @brief Get peak master output level
     */
    float getMasterPeakLevel() const { return masterPeakLevel_.load(); }

    /**
     * @brief Reset peak meters
     */
    void resetPeakMeters() { masterPeakLevel_.store(0.0f); }

    //==========================================================================
    // Latency Query
    //==========================================================================

    /**
     * @brief Get latency for a specific track in samples
     */
    int getTrackLatency(int trackIndex) const;

    /**
     * @brief Get master bus latency in samples
     */
    int getMasterLatency() const;

    /**
     * @brief Update master latency calculation
     * @param masterPlugins Reference to master plugins
     * @param limiterLatency Latency from the master limiter
     */
    void updateMasterLatency(
        const std::vector<std::unique_ptr<juce::AudioPluginInstance>>& masterPlugins,
        int limiterLatency);

private:
    //==========================================================================
    // Internal Methods
    //==========================================================================

    /**
     * @brief Apply PDC delay to a buffer
     */
    void applyPDCDelay(juce::AudioBuffer<float>& buffer, 
                       int trackIndex, int numSamples);

    /**
     * @brief Process master bus plugins
     */
    void processMasterPlugins(
        juce::AudioBuffer<float>& buffer,
        std::vector<std::unique_ptr<juce::AudioPluginInstance>>& plugins);

    /**
     * @brief Update output metering
     */
    void updateMasterMeters(const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    // State
    //==========================================================================

    double sampleRate_ = constants::kDefaultSampleRate;
    int blockSize_ = constants::kDefaultBufferSize;

    // Per-track buffers (pre-allocated)
    std::vector<juce::AudioBuffer<float>> trackBuffers_;
    
    // Aux bus buffers
    std::vector<juce::AudioBuffer<float>> auxBusBuffers_;

    // PDC state
    // Note: PDC is disabled by default to avoid unexpected latency when adding plugins.
    // Enable explicitly via setPDCEnabled(true) when latency compensation is needed.
    std::atomic<bool> pdcEnabled_{false};
    std::atomic<int> maxTrackLatency_{0};
    std::vector<int> trackLatencies_;
    std::vector<juce::AudioBuffer<float>> pdcDelayBuffers_;
    std::vector<int> pdcDelayWritePos_;

    // Metering (atomic for lock-free GUI access)
    std::atomic<float> masterLevel_{0.0f};
    std::atomic<float> masterPeakLevel_{0.0f};

    // Master latency cache
    std::atomic<int> masterLatency_{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRenderer)
};

} // namespace zenith
