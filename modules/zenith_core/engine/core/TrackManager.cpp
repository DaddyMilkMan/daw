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

 * @file TrackManager.cpp
 * @brief Concrete track management implementation
 */



TrackManager::TrackManager() {
    DBG("TrackManager: Constructor");
}

TrackManager::~TrackManager() {
    DBG("TrackManager: Destructor");
    clearAllTracks();
}

juce::String TrackManager::createTrack(const juce::String& name, const juce::String& type) {
    auto trackId = generateTrackId();

    // Create track based on type
    std::shared_ptr<Track> track;
    if (type.equalsIgnoreCase("audio")) {
        // TODO: Create AudioTrack
        DBG("TrackManager: Creating audio track - " + name);
    } else if (type.equalsIgnoreCase("midi")) {
        // TODO: Create MIDITrack
        DBG("TrackManager: Creating MIDI track - " + name);
    } else {
        DBG("TrackManager: Unknown track type - " + type);
        return juce::String();
    }

    if (track) {
        track->setId(trackId);
        track->setName(name);
        addTrack(track);
    }

    return trackId;
}

void TrackManager::addTrack(std::shared_ptr<Track> track) {
    if (!track) {
        DBG("TrackManager: Cannot add null track");
        return;
    }

    const juce::ScopedWriteLock lock(tracksLock_);

    tracks_.push_back(track);
    trackIdMap_[track->getId()] = track.get();

    // Initialize track state
    trackStates_.resize(tracks_.size());

    DBG("TrackManager: Added track '" + track->getName() + "' (" + track->getId() + ")");

    notifyTracksChanged();
}

void TrackManager::removeTrack(int index) {
    if (index < 0 || index >= tracks_.size()) {
        DBG("TrackManager: Invalid track index - " + juce::String(index));
        return;
    }

    const juce::ScopedWriteLock lock(tracksLock_);

    auto track = tracks_[index];
    DBG("TrackManager: Removing track '" + track->getName() + "' (" + track->getId() + ")");

    // Remove from maps
    trackIdMap_.erase(track->getId());
    tracks_.erase(tracks_.begin() + index);
    trackStates_.erase(trackStates_.begin() + index);

    notifyTracksChanged();
}

void TrackManager::removeTrack(const juce::String& trackId) {
    const juce::ScopedWriteLock lock(tracksLock_);

    auto it = std::find_if(tracks_.begin(), tracks_.end(),
        [&trackId](const std::shared_ptr<Track>& track) {
            return track->getId() == trackId;
        });

    if (it != tracks_.end()) {
        int index = it - tracks_.begin();
        removeTrack(index);
    }
}

int TrackManager::getNumTracks() const noexcept {
    const juce::ScopedReadLock lock(tracksLock_);
    return tracks_.size();
}

Track* TrackManager::getTrack(int index) const {
    const juce::ScopedReadLock lock(tracksLock_);
    return (index >= 0 && index < tracks_.size()) ? tracks_[index].get() : nullptr;
}

Track* TrackManager::getTrackById(const juce::String& trackId) const {
    const juce::ScopedReadLock lock(tracksLock_);
    auto it = trackIdMap_.find(trackId);
    return (it != trackIdMap_.end()) ? it->second : nullptr;
}

const std::vector<std::shared_ptr<Track>>& TrackManager::tracks() const {
    const juce::ScopedReadLock lock(tracksLock_);
    return tracks_;
}

std::vector<std::shared_ptr<Track>> TrackManager::getTracksSnapshot() const {
    const juce::ScopedReadLock lock(tracksLock_);
    return tracks_;
}

std::vector<Track*> TrackManager::getTrackPointersSnapshot() const {
    const juce::ScopedReadLock lock(tracksLock_);

    std::vector<Track*> snapshot;
    snapshot.reserve(tracks_.size());

    for (const auto& track : tracks_) {
        snapshot.push_back(track.get());
    }

    return snapshot;
}

void TrackManager::setTrackVolume(int trackIndex, float volume) {
    if (trackIndex < 0 || trackIndex >= tracks_.size()) {
        return;
    }

    auto track = tracks_[trackIndex];
    track->setVolume(volume);
    updateTrackState(trackIndex);
    notifyTrackChanged(trackIndex);
}

void TrackManager::setTrackPan(int trackIndex, float pan) {
    if (trackIndex < 0 || trackIndex >= tracks_.size()) {
        return;
    }

    auto track = tracks_[trackIndex];
    track->setPan(pan);
    updateTrackState(trackIndex);
    notifyTrackChanged(trackIndex);
}

void TrackManager::setTrackMute(int trackIndex, bool muted) {
    if (trackIndex < 0 || trackIndex >= tracks_.size()) {
        return;
    }

    auto track = tracks_[trackIndex];
    track->setMuted(muted);
    updateTrackState(trackIndex);
    notifyTrackChanged(trackIndex);
}

void TrackManager::setTrackSolo(int trackIndex, bool solo) {
    if (trackIndex < 0 || trackIndex >= tracks_.size()) {
        return;
    }

    auto track = tracks_[trackIndex];
    track->setSolo(solo);
    updateTrackState(trackIndex);
    notifyTrackChanged(trackIndex);
}

void TrackManager::setTrackArmed(int trackIndex, bool armed) {
    if (trackIndex < 0 || trackIndex >= tracks_.size()) {
        return;
    }

    auto track = tracks_[trackIndex];
    track->setArmed(armed);
    updateTrackState(trackIndex);
    notifyTrackChanged(trackIndex);
}

void TrackManager::setTrackInputChannel(int trackIndex, int channelIndex) {
    if (trackIndex < 0 || trackIndex >= tracks_.size()) {
        return;
    }

    auto track = tracks_[trackIndex];
    track->setInputChannel(channelIndex);
    updateTrackState(trackIndex);
    notifyTrackChanged(trackIndex);
}

float TrackManager::getTrackLevel(int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= trackStates_.size()) {
        return 0.0f;
    }
    return trackStates_[trackIndex].level.load();
}

float TrackManager::getTrackPeakLevel(int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= trackStates_.size()) {
        return 0.0f;
    }
    return trackStates_[trackIndex].peakLevel.load();
}

bool TrackManager::isTrackFrozen(int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= trackStates_.size()) {
        return false;
    }
    return trackStates_[trackIndex].frozen.load();
}

void TrackManager::addTrackChangeListener(juce::ChangeListener* listener) {
    trackChangeListeners_.addListener(listener);
}

void TrackManager::removeTrackChangeListener(juce::ChangeListener* listener) {
    trackChangeListeners_.removeListener(listener);
}

void TrackManager::clearAllTracks() {
    const juce::ScopedWriteLock lock(tracksLock_);

    DBG("TrackManager: Clearing all tracks");
    tracks_.clear();
    trackIdMap_.clear();
    trackStates_.clear();

    notifyTracksChanged();
}

void TrackManager::prepareForProcessing(int samplesPerBlock, double sampleRate) {
    const juce::ScopedWriteLock lock(tracksLock_);

    for (auto& track : tracks_) {
        track->prepareToPlay(samplesPerBlock, sampleRate);
    }

    DBG("TrackManager: Prepared " + juce::String(tracks_.size()) + " tracks for processing");
}

void TrackManager::updateTrackState(int index) {
    if (index >= 0 && index < trackStates_.size()) {
        // TODO: Get actual track state from track
        // For now, just reset
        trackStates_[index].level.store(0.0f);
        trackStates_[index].peakLevel.store(0.0f);
    }
}

void TrackManager::notifyTrackChanged(int index) {
    juce::ChangeBroadcaster::sendChangeMessage();
    trackChangeListeners_.call(&juce::ChangeListener::changeListenerCallback, nullptr);
}

void TrackManager::notifyTracksChanged() {
    juce::ChangeBroadcaster::sendChangeMessage();
    trackChangeListeners_.call(&juce::ChangeListener::changeListenerCallback, nullptr);
}

juce::String TrackManager::generateTrackId() {
    return "track_" + juce::String(nextTrackId_.fetch_add(1));
}

} // namespace zenith