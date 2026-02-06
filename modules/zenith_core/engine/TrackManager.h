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

// TrackManager.h

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
 // Brief: Manages all tracks in the project
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
     // Brief: Create a new audio track
     * @param name Track name
     * @return Shared pointer to the new track
     */
    std::shared_ptr<AudioTrack> createAudioTrack(const juce::String& name = "Audio Track");

    /**
     // Brief: Create a new MIDI track
     * @param name Track name
     * @return Shared pointer to the new track
     */
    std::shared_ptr<MidiTrack> createMidiTrack(const juce::String& name = "MIDI Track");

    /**
     // Brief: Create a new instrument track
     * @param name Track name
     * @return Shared pointer to the new track
     */
    std::shared_ptr<InstrumentTrack> createInstrumentTrack(const juce::String& name = "Instrument Track");

    /**
     // Brief: Create a new bus track
     * @param name Track name
     * @param isAux Whether this is an aux bus (true) or submix bus (false)
     * @return Shared pointer to the new track
     */
    std::shared_ptr<BusTrack> createBusTrack(const juce::String& name = "Bus Track", bool isAux = false);

    //==========================================================================
    // Track Management
    //==========================================================================

    /**
     // Brief: Get all tracks
     */
    const std::vector<std::shared_ptr<Track>>& getAllTracks() const { return tracks_; }

    /**
     // Brief: Get track by ID
     */
    std::shared_ptr<Track> getTrackById(const juce::String& trackId) const;

    /**
     // Brief: Get track by index
     */
    std::shared_ptr<Track> getTrackByIndex(int index) const;

    /**
     // Brief: Get number of tracks
     */
    size_t getTrackCount() const { return tracks_.size(); }

    /**
     // Brief: Remove a track
     */
    void removeTrack(const juce::String& trackId);

    /**
     // Brief: Remove all tracks
     */
    void removeAllTracks();

    //==========================================================================
    // Track Ordering
    //==========================================================================

    /**
     // Brief: Move track to new position
     */
    void moveTrack(const juce::String& trackId, int newIndex);

    /**
     // Brief: Get track index
     */
    int getTrackIndex(const juce::String& trackId) const;

    //==========================================================================
    // Track State Management
    //==========================================================================

    /**
     // Brief: Update all track states (call from audio thread)
     */
    void updateTrackStates(int numSamples);

    /**
     // Brief: Prepare tracks for playback
     */
    void prepareTracks(double sampleRate, int bufferSize);

    /**
     // Brief: Process all tracks (call from audio thread)
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
     // Brief: Set project state
     */
    void setProjectState(ProjectState* state) { projectState_ = state; }

    /**
     // Brief: Save track states to project
     */
    void saveToProject();

    /**
     // Brief: Load track states from project
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
