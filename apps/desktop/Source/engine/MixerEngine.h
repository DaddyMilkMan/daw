/**
 * @file MixerEngine.h
 * @brief Audio mixing and routing system
 * 
 * This component handles:
 * - Audio mixing and routing
 * - Effects processing
 * - Level metering
 * - Output management
 */

#pragma once

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
 * @brief Audio mixing and processing engine
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
     * @brief Initialize the mixer
     * @param sampleRate Sample rate
     * @param bufferSize Buffer size
     * @param numOutputChannels Number of output channels
     */
    bool initialize(double sampleRate, int bufferSize, int numOutputChannels = 2);

    /**
     * @brief Prepare for processing
     */
    void prepareToPlay(double sampleRate, int bufferSize);

    /**
     * @brief Shutdown the mixer
     */
    void shutdown();

    //==========================================================================
    // Channel Management
    //==========================================================================

    /**
     * @brief Add a track to the mix
     */
    void addTrack(std::shared_ptr<Track> track);

    /**
     * @brief Remove a track from the mix
     */
    void removeTrack(const juce::String& trackId);

    /**
     * @brief Get mixer channel for a track
     */
    std::shared_ptr<MixerChannel> getMixerChannel(const juce::String& trackId) const;

    //==========================================================================
    // Aux Bus Management
    //==========================================================================

    /**
     * @brief Create an aux bus
     */
    std::shared_ptr<AuxBus> createAuxBus(const juce::String& name, int numChannels = 2);

    /**
     * @brief Remove an aux bus
     */
    void removeAuxBus(const juce::String& busId);

    /**
     * @brief Get aux bus by ID
     */
    std::shared_ptr<AuxBus> getAuxBus(const juce::String& busId) const;

    //==========================================================================
    // Master Processing
    //==========================================================================

    /**
     * @brief Set master output level
     */
    void setMasterLevel(float level);

    /**
     * @brief Get master output level
     */
    float getMasterLevel() const { return masterLevel_.load(); }

    /**
     * @brief Enable/disable master limiter
     */
    void setMasterLimiterEnabled(bool enabled);

    /**
     * @brief Check if master limiter is enabled
     */
    bool isMasterLimiterEnabled() const { return masterLimiterEnabled_.load(); }

    //==========================================================================
    // Audio Processing
    //==========================================================================

    /**
     * @brief Process audio through the mixer (RT-safe)
     */
    void processAudio(const float** inputChannels,
                      float** outputChannels,
                      int numInputChannels,
                      int numOutputChannels,
                      int numSamples);

    /**
     * @brief Process a single track's audio
     */
    void processTrack(const std::shared_ptr<Track>& track,
                     float** outputChannels,
                     int numOutputChannels,
                     int numSamples);

    //==========================================================================
    // Metering and Monitoring
    //==========================================================================

    /**
     * @brief Get current CPU usage
     */
    float getCpuUsage() const { return cpuUsage_.load(); }

    /**
     * @brief Get master output levels
     */
    std::pair<float, float> getMasterLevels() const;

    /**
     * @brief Get peak levels for a track
     */
    std::pair<float, float> getTrackLevels(const juce::String& trackId) const;

    //==========================================================================
    // Routing
    //==========================================================================

    /**
     * @brief Route track to aux bus
     */
    void routeToAux(const juce::String& trackId, const juce::String& auxBusId, float level = 1.0f);

    /**
     * @brief Remove aux routing
     */
    void removeAuxRouting(const juce::String& trackId, const juce::String& auxBusId);

    /**
     * @brief Set track output routing
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
