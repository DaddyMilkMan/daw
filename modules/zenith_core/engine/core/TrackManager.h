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

/*
    ==============================================================================
    Original file header:
*/

 * @file TrackManager.h
 * @brief Concrete track management implementation
 *
 * Manages audio and MIDI tracks with thread-safe access and snapshots.
 */


#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>

namespace zenith {

class TrackManager : public ITrackManager {
public:
    TrackManager();
    ~TrackManager() override;

    //==========================================================================
    // ITrackManager Implementation
    //==========================================================================

    juce::String createTrack(const juce::String& name, const juce::String& type) override;
    void addTrack(std::shared_ptr<Track> track) override;
    void removeTrack(int index) override;
    void removeTrack(const juce::String& trackId) override;

    int getNumTracks() const noexcept override;
    Track* getTrack(int index) const override;
    Track* getTrackById(const juce::String& trackId) const override;
    const std::vector<std::shared_ptr<Track>>& tracks() const override;

    std::vector<std::shared_ptr<Track>> getTracksSnapshot() const override;
    std::vector<Track*> getTrackPointersSnapshot() const override;

    void setTrackVolume(int trackIndex, float volume) override;
    void setTrackPan(int trackIndex, float pan) override;
    void setTrackMute(int trackIndex, bool muted) override;
    void setTrackSolo(int trackIndex, bool solo) override;
    void setTrackArmed(int trackIndex, bool armed) override;
    void setTrackInputChannel(int trackIndex, int channelIndex) override;

    float getTrackLevel(int trackIndex) const override;
    float getTrackPeakLevel(int trackIndex) const override;
    bool isTrackFrozen(int trackIndex) const override;

    void addTrackChangeListener(juce::ChangeListener* listener) override;
    void removeTrackChangeListener(juce::ChangeListener* listener) override;

    void clearAllTracks() override;
    void prepareForProcessing(int samplesPerBlock, double sampleRate) override;

private:
    //==========================================================================
    // Track Management
    //==========================================================================

    mutable juce::ReadWriteLock tracksLock_;
    std::vector<std::shared_ptr<Track>> tracks_;
    std::unordered_map<juce::String, Track*> trackIdMap_;
    std::atomic<uint64_t> nextTrackId_{0};

    //==========================================================================
    // Track State (for fast UI updates)
    //==========================================================================

    struct TrackState {
        std::atomic<float> level{0.0f};
        std::atomic<float> peakLevel{0.0f};
        std::atomic<bool> frozen{false};
    };

    std::vector<TrackState> trackStates_;

    //==========================================================================
    // Event System
    //==========================================================================

    juce::ListenerList<juce::ChangeListener> trackChangeListeners_;

    //==========================================================================
    // Helpers
    //==========================================================================

    void updateTrackState(int index);
    void notifyTrackChanged(int index);
    void notifyTracksChanged();
    juce::String generateTrackId();
};

} // namespace zenith