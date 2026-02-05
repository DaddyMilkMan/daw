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

 * @file ITrackManager.h
 * @brief Track management interface
 *
 * Manages audio and MIDI tracks, providing thread-safe access and snapshots.
 */


#include <vector>
#include <juce_core/juce_core.h>
#include "Track.h"

namespace zenith {

class ITrackManager {
public:
    virtual ~ITrackManager() = default;

    //==========================================================================
    // Track Lifecycle
    //==========================================================================

    virtual juce::String createTrack(const juce::String& name, const juce::String& type) = 0;
    virtual void addTrack(std::shared_ptr<Track> track) = 0;
    virtual void removeTrack(int index) = 0;
    virtual void removeTrack(const juce::String& trackId) = 0;

    //==========================================================================
    // Track Access
    //==========================================================================

    virtual int getNumTracks() const noexcept = 0;
    virtual Track* getTrack(int index) const = 0;
    virtual Track* getTrackById(const juce::String& trackId) const = 0;
    virtual const std::vector<std::shared_ptr<Track>>& tracks() const = 0;

    //==========================================================================
    // Thread-Safe Snapshots
    //==========================================================================

    virtual std::vector<std::shared_ptr<Track>> getTracksSnapshot() const = 0;
    virtual std::vector<Track*> getTrackPointersSnapshot() const = 0;

    //==========================================================================
    // Track State
    //==========================================================================

    virtual void setTrackVolume(int trackIndex, float volume) = 0;
    virtual void setTrackPan(int trackIndex, float pan) = 0;
    virtual void setTrackMute(int trackIndex, bool muted) = 0;
    virtual void setTrackSolo(int trackIndex, bool solo) = 0;
    virtual void setTrackArmed(int trackIndex, bool armed) = 0;
    virtual void setTrackInputChannel(int trackIndex, int channelIndex) = 0;

    //==========================================================================
    // Track State Queries
    //==========================================================================

    virtual float getTrackLevel(int trackIndex) const = 0;
    virtual float getTrackPeakLevel(int trackIndex) const = 0;
    virtual bool isTrackFrozen(int trackIndex) const = 0;

    //==========================================================================
    // Event Listeners
    //==========================================================================

    virtual void addTrackChangeListener(juce::ChangeListener* listener) = 0;
    virtual void removeTrackChangeListener(juce::ChangeListener* listener) = 0;

    //==========================================================================
    // Management
    //==========================================================================

    virtual void clearAllTracks() = 0;
    virtual void prepareForProcessing(int samplesPerBlock, double sampleRate) = 0;
};

} // namespace zenith