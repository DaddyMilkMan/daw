/**
 * @file TrackManager.h
 * @brief Track and clip management system
 * 
 * This component handles:
 * - Track creation and management
 * - Clip management within tracks
 * - Track state synchronization
 * - Audio/MIDI/Instrument track types
 */

#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace zenith {

// Forward declarations
class Track;
class Clip;
class AudioTrack;
class MidiTrack;
class InstrumentTrack;
class BusTrack;
class ProjectState;

//==============================================================================
/**
 * @class TrackManager
 * @brief Manages all tracks in the project
 * 
 * This class provides a clean interface for managing tracks without
 * exposing the complexity of the underlying track system.
 */
class TrackManager {
public:
    //==========================================================================
    TrackManager();
    ~TrackManager();

    //==========================================================================
    // Track Creation
    //==========================================================================

    /**
     * @brief Create a new audio track
     * @param name Track name
     * @return Shared pointer to the new track
     */
    std::shared_ptr<AudioTrack> createAudioTrack(const juce::String& name = "Audio Track");

    /**
     * @brief Create a new MIDI track
     * @param name Track name
     * @return Shared pointer to the new track
     */
    std::shared_ptr<MidiTrack> createMidiTrack(const juce::String& name = "MIDI Track");

    /**
     * @brief Create a new instrument track
     * @param name Track name
     * @return Shared pointer to the new track
     */
    std::shared_ptr<InstrumentTrack> createInstrumentTrack(const juce::String& name = "Instrument Track");

    /**
     * @brief Create a new bus track
     * @param name Track name
     * @param isAux Whether this is an aux bus (true) or submix bus (false)
     * @return Shared pointer to the new track
     */
    std::shared_ptr<BusTrack> createBusTrack(const juce::String& name = "Bus Track", bool isAux = false);

    //==========================================================================
    // Track Management
    //==========================================================================

    /**
     * @brief Get all tracks
     */
    const std::vector<std::shared_ptr<Track>>& getAllTracks() const { return tracks_; }

    /**
     * @brief Get track by ID
     */
    std::shared_ptr<Track> getTrackById(const juce::String& trackId) const;

    /**
     * @brief Get track by index
     */
    std::shared_ptr<Track> getTrackByIndex(int index) const;

    /**
     * @brief Get number of tracks
     */
    size_t getTrackCount() const { return tracks_.size(); }

    /**
     * @brief Remove a track
     */
    void removeTrack(const juce::String& trackId);

    /**
     * @brief Remove all tracks
     */
    void removeAllTracks();

    //==========================================================================
    // Track Ordering
    //==========================================================================

    /**
     * @brief Move track to new position
     */
    void moveTrack(const juce::String& trackId, int newIndex);

    /**
     * @brief Get track index
     */
    int getTrackIndex(const juce::String& trackId) const;

    //==========================================================================
    // Track State Management
    //==========================================================================

    /**
     * @brief Update all track states (call from audio thread)
     */
    void updateTrackStates(int numSamples);

    /**
     * @brief Prepare tracks for playback
     */
    void prepareTracks(double sampleRate, int bufferSize);

    /**
     * @brief Process all tracks (call from audio thread)
     */
    void processTracks(const float** inputChannels,
                      float** outputChannels,
                      int numInputChannels,
                      int numOutputChannels,
                      int numSamples);

    //==========================================================================
    // Project Integration
    //==========================================================================

    /**
     * @brief Set project state
     */
    void setProjectState(ProjectState* state) { projectState_ = state; }

    /**
     * @brief Save track states to project
     */
    void saveToProject();

    /**
     * @brief Load track states from project
     */
    void loadFromProject();

private:
    //==========================================================================
    // Internal Methods
    //==========================================================================

    juce::String generateTrackId() const;
    void validateTrackOrder();
    void notifyTrackAdded(const std::shared_ptr<Track>& track);
    void notifyTrackRemoved(const juce::String& trackId);

    //==========================================================================
    // Member Variables
    //==========================================================================

    std::vector<std::shared_ptr<Track>> tracks_;
    std::atomic<int> nextTrackId_{1};
    ProjectState* projectState_ = nullptr;

    // RT-safe track snapshot for audio thread
    struct TrackSnapshot {
        std::vector<Track*> trackPointers;  // Raw pointers for RT access
        juce::int64 snapshotVersion = 0;
    };
    std::atomic<TrackSnapshot*> currentSnapshot_{nullptr};
    std::atomic<juce::int64> snapshotVersion_{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackManager)
};

} // namespace zenith
