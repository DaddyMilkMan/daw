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

  ==============================================================================

    SessionController.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of SessionController.

  ==============================================================================

*/

#include "SessionController.h"
#include "../../../engine/Track.h"
#include "../../../engine/EngineEvent.h"

namespace zenith::ui {

//==============================================================================
// Construction/Destruction
//==============================================================================

SessionController::SessionController(SkiaSessionView& view,
                                     zenith::Engine& engine,
                                     zenith::ProjectState& projectState)
    : view_(view)
    , engine_(engine)
    , projectState_(projectState)
{
    // Listen for project state changes
    projectState_.addListener(this);
    
    // Get initial values
    tempo_ = projectState_.getTempo();
    sampleRate_ = projectState_.getSampleRate();
    if (sampleRate_ <= 0.0) sampleRate_ = 44100.0;
    
    // Initial sync
    fullSync();
    
    // Start timer for meter updates (30fps is enough for meters)
    startTimerHz(30);
}

SessionController::~SessionController() {
    stopTimer();
    projectState_.removeListener(this);
}

//==============================================================================
// Clip Launching
//==============================================================================

void SessionController::launchClip(int trackIndex, int sceneIndex) {
    auto it = slotToClipId_.find({trackIndex, sceneIndex});
    if (it == slotToClipId_.end()) {
        return;  // No clip in this slot
    }
    
    const juce::String& clipId = it->second;
    
    // Queue clip launch event with quantization
    zenith::EngineEvent event;
    event.type = zenith::EngineEvent::Type::LaunchClip;
    event.trackIndex = trackIndex;
    event.clipId = clipId;
    event.sceneIndex = sceneIndex;
    event.value = static_cast<float>(getNextQuantizedPosition());  // Store launch position
    
    // Send to engine
    engine_.queueEvent(event);
    
    // If transport is not playing, start it at the quantized position
    if (!engine_.isPlaying()) {
        engine_.play();
    }
    
    // Update view state to show clip is playing (or queued if quantization is pending)
    ClipSlotData data;
    data.state = (launchQuantize_ == LaunchQuantize::None) ? 
                 ClipSlotState::Playing : ClipSlotState::Queued;
    data.name = clipNames_[clipId];
    data.color = clipColors_[clipId];
    data.playProgress = 0.0f;
    view_.setClipState(trackIndex, sceneIndex, data);
    
    // Store as active clip for this track
    activeClips_[trackIndex] = {trackIndex, sceneIndex};
}

void SessionController::stopClip(int trackIndex) {
    // Stop the active clip on this track
    auto it = activeClips_.find(trackIndex);
    if (it != activeClips_.end()) {
        auto [activeTrack, activeScene] = it->second;
        juce::ignoreUnused(activeTrack);
        
        // Get the clip ID
        auto clipIt = slotToClipId_.find(it->second);
        if (clipIt != slotToClipId_.end()) {
            // Queue stop clip event
            zenith::EngineEvent event;
            event.type = zenith::EngineEvent::Type::StopClip;
            event.trackIndex = trackIndex;
            event.clipId = clipIt->second;
            event.sceneIndex = activeScene;
            engine_.queueEvent(event);
            
            // Update view to stopped state
            ClipSlotData data;
            data.state = ClipSlotState::Stopped;
            data.name = clipNames_[clipIt->second];
            data.color = clipColors_[clipIt->second];
            view_.setClipState(trackIndex, activeScene, data);
        }
        
        activeClips_.erase(it);
    }
}

void SessionController::launchScene(int sceneIndex) {
    for (size_t trackIndex = 0; trackIndex < trackIds_.size(); ++trackIndex) {
        launchClip(static_cast<int>(trackIndex), sceneIndex);
    }
}

void SessionController::stopAllClips() {
    for (size_t trackIndex = 0; trackIndex < trackIds_.size(); ++trackIndex) {
        stopClip(static_cast<int>(trackIndex));
    }
}

//==============================================================================
// Recording
//==============================================================================

void SessionController::armSlot(int trackIndex, int sceneIndex) {
    // Arm track for recording
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackIds_.size())) {
        projectState_.setTrackArmed(trackIds_[trackIndex], true);
    }
    juce::ignoreUnused(sceneIndex);
}

void SessionController::startRecording() {
    engine_.record();
}

void SessionController::stopRecording() {
    engine_.stopRecording();
}

//==============================================================================
// Transport
//==============================================================================

void SessionController::play() {
    engine_.play();
}

void SessionController::stop() {
    engine_.stop();
}

void SessionController::togglePlayback() {
    if (engine_.isPlaying()) {
        stop();
    } else {
        play();
    }
}

//==============================================================================
// Track Operations
//==============================================================================

void SessionController::setTrackMute(int trackIndex, bool muted) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackIds_.size())) {
        projectState_.setTrackMute(trackIds_[trackIndex], muted);
    }
}

void SessionController::setTrackSolo(int trackIndex, bool soloed) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackIds_.size())) {
        projectState_.setTrackSolo(trackIds_[trackIndex], soloed);
    }
}

void SessionController::setTrackVolume(int trackIndex, float volume) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackIds_.size())) {
        projectState_.setTrackVolume(trackIds_[trackIndex], volume);
    }
}

void SessionController::setTrackPan(int trackIndex, float pan) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackIds_.size())) {
        projectState_.setTrackPan(trackIds_[trackIndex], pan);
    }
}

//==============================================================================
// Sync
//==============================================================================

void SessionController::fullSync() {
    tempo_ = projectState_.getTempo();
    rebuildTrackData();
    rebuildSceneData();
    rebuildClipSlots();
    updateViewData();
}

void SessionController::syncMeters() {
    // Get meter levels from engine tracks
    auto tracks = engine_.getTracksSnapshot();
    
    for (size_t i = 0; i < trackIds_.size() && i < tracks.size(); ++i) {
        if (tracks[i]) {
            // Get output level from track's mixer channel
            float level = tracks[i]->getMixerChannel().getOutputLevel();
            // Normalize to 0-1 range (level is likely in dB or linear)
            // Assuming linear output, clamp to 0-1
            level = std::clamp(level, 0.0f, 1.0f);
            view_.setTrackMeterLevel(static_cast<int>(i), level);
        }
    }
}

void SessionController::syncClipStates() {
    if (!engine_.isPlaying()) {
        return;
    }

    double playheadBeats = engine_.getPlaybackPositionBeats();

    // Update clip states based on active clips
    for (const auto& [slotKey, clipId] : slotToClipId_) {
        auto [trackIndex, sceneIndex] = slotKey;

        // Check if this clip is currently active (playing)
        bool isActive = false;
        auto activeIt = activeClips_.find(trackIndex);
        if (activeIt != activeClips_.end()) {
            auto [activeTrack, activeScene] = activeIt->second;
            juce::ignoreUnused(activeTrack);
            isActive = (activeScene == sceneIndex);
        }

        // Get clip length from project state
        juce::ValueTree trackTree = projectState_.getTrack(trackIds_[trackIndex]);
        if (!trackTree.isValid()) continue;

        juce::ValueTree clipsContainer = trackTree.getChildWithName(zenith::ProjectState::ID_CLIPS);
        if (!clipsContainer.isValid()) continue;

        if (sceneIndex >= clipsContainer.getNumChildren()) continue;

        juce::ValueTree clipTree = clipsContainer.getChild(sceneIndex);
        if (!clipTree.isValid() || !clipTree.hasType(zenith::ProjectState::ID_CLIP)) continue;

        // Get clip properties
        double startBeats = 0.0;
        double lengthBeats = 4.0;  // Default 4 beats

        if (clipTree.hasProperty(zenith::ProjectState::PROP_START_BEATS)) {
            startBeats = clipTree.getProperty(zenith::ProjectState::PROP_START_BEATS, 0.0);
            lengthBeats = clipTree.getProperty(zenith::ProjectState::PROP_LENGTH_BEATS, 4.0);
        } else {
            juce::int64 startSamples = clipTree.getProperty(zenith::ProjectState::PROP_START, 0);
            juce::int64 lengthSamples = clipTree.getProperty(zenith::ProjectState::PROP_LENGTH, 0);
            startBeats = samplesToBeats(startSamples);
            lengthBeats = samplesToBeats(lengthSamples);
        }

        // Calculate progress (0-1) based on playhead position within clip
        double clipEndBeats = startBeats + lengthBeats;
        float progress = 0.0f;

        if (isActive && lengthBeats > 0.0) {
            // Calculate progress within the loop
            double loopStart = 0.0;
            double loopLength = 8.0;  // Default 8 bar loop

            // Get loop region if available
            if (engine_.isLooping()) {
                juce::int64 loopStartSamples = engine_.getLoopStart();
                juce::int64 loopEndSamples = engine_.getLoopEnd();
                loopStart = samplesToBeats(loopStartSamples);
                loopLength = samplesToBeats(loopEndSamples - loopStartSamples);
            }

            // Normalize playhead to loop position
            double loopPosition = playheadBeats - loopStart;
            while (loopPosition < 0) loopPosition += loopLength;
            while (loopPosition >= loopLength) loopPosition -= loopLength;

            // Calculate clip progress within loop
            double clipStartInLoop = startBeats - loopStart;
            while (clipStartInLoop < 0) clipStartInLoop += loopLength;
            while (clipStartInLoop >= loopLength) clipStartInLoop -= loopLength;

            double positionInClip = loopPosition - clipStartInLoop;
            progress = static_cast<float>(std::clamp(positionInClip / lengthBeats, 0.0, 1.0));
        }

        // Update the view with new state
        ClipSlotData slotData;
        slotData.state = isActive ? ClipSlotState::Playing : ClipSlotState::Stopped;
        slotData.name = clipNames_[clipId];
        slotData.color = clipColors_[clipId];
        slotData.playProgress = progress;
        slotData.isSelected = (selectedSlot_.first == static_cast<int>(trackIndex) &&
                               selectedSlot_.second == sceneIndex);

        juce::String clipType = clipTree.getProperty(zenith::ProjectState::PROP_TYPE, "audio");
        slotData.isMidi = (clipType == "midi");

        view_.setClipState(trackIndex, sceneIndex, slotData);
    }
}

void SessionController::rebuildTrackData() {
    trackIds_.clear();
    
    int numTracks = projectState_.getNumTracks();
    trackIds_.reserve(numTracks);
    
    std::vector<SessionTrackData> tracks;
    tracks.reserve(numTracks);
    
    for (int i = 0; i < numTracks; ++i) {
        juce::ValueTree trackTree = projectState_.getTrackByIndex(i);
        if (trackTree.isValid()) {
            juce::String trackId = trackTree.getProperty(zenith::ProjectState::PROP_ID, "");
            trackIds_.push_back(trackId);
            tracks.push_back(extractTrackData(trackTree, i));
        }
    }
    
    view_.setTracks(tracks);
}

void SessionController::rebuildSceneData() {
    // Determine number of scenes based on max clips per track
    int maxScenes = 8;  // Default minimum
    
    for (const auto& trackId : trackIds_) {
        juce::ValueTree trackTree = projectState_.getTrack(trackId);
        if (!trackTree.isValid()) continue;
        
        juce::ValueTree clipsContainer = trackTree.getChildWithName(zenith::ProjectState::ID_CLIPS);
        if (clipsContainer.isValid()) {
            maxScenes = std::max(maxScenes, clipsContainer.getNumChildren());
        }
    }
    
    scenes_.clear();
    scenes_.resize(maxScenes);
    
    for (int i = 0; i < maxScenes; ++i) {
        scenes_[i].name = "Scene " + juce::String(i + 1);
        scenes_[i].color = juce::Colour::fromHSV(i * 0.1f, 0.5f, 0.7f, 1.0f);
    }
    
    view_.setScenes(scenes_);
}

void SessionController::rebuildClipSlots() {
    slotToClipId_.clear();
    clipNames_.clear();
    clipColors_.clear();
    
    for (size_t trackIndex = 0; trackIndex < trackIds_.size(); ++trackIndex) {
        juce::ValueTree trackTree = projectState_.getTrack(trackIds_[trackIndex]);
        if (!trackTree.isValid()) continue;
        
        juce::ValueTree clipsContainer = trackTree.getChildWithName(zenith::ProjectState::ID_CLIPS);
        if (!clipsContainer.isValid()) continue;
        
        for (int sceneIndex = 0; sceneIndex < clipsContainer.getNumChildren(); ++sceneIndex) {
            juce::ValueTree clipTree = clipsContainer.getChild(sceneIndex);
            if (clipTree.hasType(zenith::ProjectState::ID_CLIP)) {
                juce::String clipId = clipTree.getProperty(zenith::ProjectState::PROP_ID, "");
                slotToClipId_[{static_cast<int>(trackIndex), sceneIndex}] = clipId;
                
                // Store clip metadata for later use
                clipNames_[clipId] = clipTree.getProperty(zenith::ProjectState::PROP_NAME, "Clip");
                if (clipTree.hasProperty(zenith::ProjectState::PROP_COLOR)) {
                    juce::String colorStr = clipTree.getProperty(zenith::ProjectState::PROP_COLOR);
                    clipColors_[clipId] = juce::Colour::fromString(colorStr);
                } else {
                    // Inherit track color
                    juce::ValueTree parentTrack = clipTree.getParent().getParent();
                    if (parentTrack.isValid() && parentTrack.hasProperty(zenith::ProjectState::PROP_COLOR)) {
                        clipColors_[clipId] = juce::Colour::fromString(
                            parentTrack.getProperty(zenith::ProjectState::PROP_COLOR).toString());
                    } else {
                        clipColors_[clipId] = juce::Colours::grey;
                    }
                }
                
                ClipSlotData slotData = extractClipSlotData(clipTree);
                view_.setClipState(static_cast<int>(trackIndex), sceneIndex, slotData);
            }
        }
    }
}

void SessionController::updateViewData() {
    // Already done in rebuild methods via setTracks/setScenes/setClipState
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void SessionController::valueTreePropertyChanged(juce::ValueTree& tree,
                                                  const juce::Identifier& property) {
    // Guard against re-entrant calls
    if (isProcessingValueTreeChange_) {
        return;
    }

    juce::ignoreUnused(tree, property);
    // Could be smarter about incremental updates
    isProcessingValueTreeChange_ = true;
    fullSync();
    isProcessingValueTreeChange_ = false;
}

void SessionController::valueTreeChildAdded(juce::ValueTree& parent,
                                             juce::ValueTree& child) {
    // Guard against re-entrant calls
    if (isProcessingValueTreeChange_) {
        return;
    }

    juce::ignoreUnused(parent, child);
    isProcessingValueTreeChange_ = true;
    fullSync();
    isProcessingValueTreeChange_ = false;
}

void SessionController::valueTreeChildRemoved(juce::ValueTree& parent,
                                               juce::ValueTree& child,
                                               int index) {
    // Guard against re-entrant calls
    if (isProcessingValueTreeChange_) {
        return;
    }

    juce::ignoreUnused(parent, child, index);
    isProcessingValueTreeChange_ = true;
    fullSync();
    isProcessingValueTreeChange_ = false;
}

void SessionController::valueTreeChildOrderChanged(juce::ValueTree& parent,
                                                    int oldIndex,
                                                    int newIndex) {
    // Guard against re-entrant calls
    if (isProcessingValueTreeChange_) {
        return;
    }

    juce::ignoreUnused(parent, oldIndex, newIndex);
    isProcessingValueTreeChange_ = true;
    fullSync();
    isProcessingValueTreeChange_ = false;
}

void SessionController::valueTreeParentChanged(juce::ValueTree& tree) {
    juce::ignoreUnused(tree);
    // Usually don't need to handle this
}

//==============================================================================
// Timer
//==============================================================================

void SessionController::timerCallback() {
    syncMeters();
    
    if (engine_.isPlaying()) {
        syncClipStates();
    }
}

//==============================================================================
// Internal Helpers
//==============================================================================

SessionTrackData SessionController::extractTrackData(const juce::ValueTree& trackTree, 
                                                      int index) const {
    SessionTrackData data;
    
    data.name = trackTree.getProperty(zenith::ProjectState::PROP_NAME, 
                                       "Track " + juce::String(index + 1));
    
    // Get color
    if (trackTree.hasProperty(zenith::ProjectState::PROP_COLOR)) {
        juce::String colorStr = trackTree.getProperty(zenith::ProjectState::PROP_COLOR);
        data.color = juce::Colour::fromString(colorStr);
    } else {
        float hue = std::fmod(index * 0.13f, 1.0f);
        data.color = juce::Colour::fromHSV(hue, 0.6f, 0.8f, 1.0f);
    }
    
    data.isMuted = trackTree.getProperty(zenith::ProjectState::PROP_MUTE, false);
    data.isSolo = trackTree.getProperty(zenith::ProjectState::PROP_SOLO, false);
    data.isArmed = trackTree.getProperty(zenith::ProjectState::PROP_ARMED, false);
    data.volume = trackTree.getProperty(zenith::ProjectState::PROP_VOLUME, 0.8f);
    data.pan = trackTree.getProperty(zenith::ProjectState::PROP_PAN, 0.5f);
    data.meterLevel = 0.0f;  // Updated via timer
    
    return data;
}

ClipSlotData SessionController::extractClipSlotData(const juce::ValueTree& clipTree) const {
    ClipSlotData data;
    
    data.state = ClipSlotState::Stopped;
    data.name = clipTree.getProperty(zenith::ProjectState::PROP_NAME, "Clip");
    
    if (clipTree.hasProperty(zenith::ProjectState::PROP_COLOR)) {
        juce::String colorStr = clipTree.getProperty(zenith::ProjectState::PROP_COLOR);
        data.color = juce::Colour::fromString(colorStr);
    } else {
        data.color = juce::Colours::grey;
    }
    
    juce::String clipType = clipTree.getProperty(zenith::ProjectState::PROP_TYPE, "audio");
    data.isMidi = (clipType == "midi");
    
    data.playProgress = 0.0f;
    data.isSelected = false;
    
    return data;
}

juce::int64 SessionController::getNextQuantizedPosition() const {
    juce::int64 currentPos = engine_.getPlayheadSamples();
    
    if (launchQuantize_ == LaunchQuantize::None) {
        return currentPos;
    }
    
    // Calculate quantize grid in samples
    double beatsPerBar = static_cast<double>(projectState_.getTimeSignatureNumerator());
    double samplesPerBeat = (sampleRate_ * 60.0) / tempo_;
    
    double gridBeats = 1.0;
    switch (launchQuantize_) {
        case LaunchQuantize::Bar:       gridBeats = beatsPerBar; break;
        case LaunchQuantize::HalfBar:   gridBeats = beatsPerBar / 2.0; break;
        case LaunchQuantize::Beat:      gridBeats = 1.0; break;
        case LaunchQuantize::HalfBeat:  gridBeats = 0.5; break;
        case LaunchQuantize::Quarter:   gridBeats = 0.25; break;
        default: break;
    }
    
    juce::int64 gridSamples = static_cast<juce::int64>(gridBeats * samplesPerBeat);
    if (gridSamples <= 0) gridSamples = 1;
    
    // Round up to next grid position
    juce::int64 nextGrid = ((currentPos / gridSamples) + 1) * gridSamples;
    return nextGrid;
}

double SessionController::samplesToBeats(juce::int64 samples) const {
    if (tempo_ <= 0.0 || sampleRate_ <= 0.0) return 0.0;
    double seconds = static_cast<double>(samples) / sampleRate_;
    return seconds * (tempo_ / 60.0);
}

juce::int64 SessionController::beatsToSamples(double beats) const {
    if (tempo_ <= 0.0 || sampleRate_ <= 0.0) return 0;
    double seconds = beats / (tempo_ / 60.0);
    return static_cast<juce::int64>(seconds * sampleRate_);
}

} // namespace zenith::ui
