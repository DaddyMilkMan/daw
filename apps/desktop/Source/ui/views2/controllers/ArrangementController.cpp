/*
  ==============================================================================

    ArrangementController.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of ArrangementController.

  ==============================================================================
*/

#include "ArrangementController.h"
#include "../../../engine/TempoMap.h"
#include <limits>
#include "../../../engine/Track.h"

namespace zenith::ui {

//==============================================================================
// Construction/Destruction
//==============================================================================

ArrangementController::ArrangementController(SkiaArrangementView& view,
                                             zenith::Engine& engine,
                                             zenith::ProjectState& projectState)
    : view_(view)
    , engine_(engine)
    , projectState_(projectState)
{
    // Listen for project state changes
    projectState_.addListener(this);
    
    // Get initial sample rate
    sampleRate_ = projectState_.getSampleRate();
    if (sampleRate_ <= 0.0) {
        sampleRate_ = 44100.0;
    }
    
    // Initial sync
    fullSync();
    
    // Setup view callbacks
    view_.onClipMoved = [this](const juce::String& clipId, int newTrackIndex, double newStartBeat) {
        this->onClipMoved(clipId, newTrackIndex, newStartBeat);
    };
    
    view_.onClipDoubleClicked = [this](const juce::String& clipId) {
        this->onClipDoubleClicked(clipId);
    };
    
    // Start timer for playhead updates (60fps)
    startTimerHz(60);
}

ArrangementController::~ArrangementController() {
    stopTimer();
    projectState_.removeListener(this);
}

//==============================================================================
// Data Access
//==============================================================================

std::vector<ClipViewData> ArrangementController::getClipsForTrack(int trackIndex) const {
    std::vector<ClipViewData> result;
    
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) {
        return result;
    }
    
    const auto& trackId = tracks_[trackIndex].id;
    for (const auto& clip : clips_) {
        if (clip.trackId == trackId) {
            result.push_back(clip);
        }
    }
    
    return result;
}

//==============================================================================
// Transport Sync
//==============================================================================

void ArrangementController::syncPlayhead() {
    double beats = engine_.getPlaybackPositionBeats();
    view_.setPlayheadPosition(beats);
}

void ArrangementController::syncLoopRegion() {
    juce::int64 loopStart = engine_.getLoopStart();
    juce::int64 loopEnd = engine_.getLoopEnd();
    
    double startBeats = samplesToBeats(loopStart);
    double endBeats = samplesToBeats(loopEnd);
    
    view_.setLoopRegion(startBeats, endBeats);
    view_.setLoopEnabled(engine_.isLooping());
}

void ArrangementController::syncTempo() {
    tempo_ = projectState_.getTempo();
    // Tempo doesn't need to be pushed to view currently, but could be used
    // for grid calculations
}

void ArrangementController::syncTimeSignature() {
    int numerator = projectState_.getTimeSignatureNumerator();
    int denominator = projectState_.getTimeSignatureDenominator();

    if (numerator <= 0) numerator = 4;
    if (denominator <= 0) denominator = 4;

    view_.setTimeSignature(numerator, denominator);
}

//==============================================================================
// User Actions
//==============================================================================

void ArrangementController::play() {
    engine_.play();
}

void ArrangementController::stop() {
    engine_.stop();
}

void ArrangementController::togglePlayback() {
    if (engine_.isPlaying()) {
        stop();
    } else {
        play();
    }
}

void ArrangementController::setPlayheadBeats(double beats) {
    juce::int64 samples = beatsToSamples(beats);
    engine_.setPlayheadSamples(samples);
    view_.setPlayheadPosition(beats);
}

void ArrangementController::setLoopRegion(double startBeats, double endBeats) {
    juce::int64 startSamples = beatsToSamples(startBeats);
    juce::int64 endSamples = beatsToSamples(endBeats);
    engine_.setLoopRegion(startSamples, endSamples);
    view_.setLoopRegion(startBeats, endBeats);
}

void ArrangementController::setLoopEnabled(bool enabled) {
    engine_.setLooping(enabled);
    view_.setLoopEnabled(enabled);
}

//==============================================================================
// Clip Operations
//==============================================================================

void ArrangementController::selectClip(const juce::String& clipId) {
    selectedClipIds_.clear();
    selectedClipIds_.push_back(clipId);
    
    // Update view selection
    ArrangementSelection sel;
    for (size_t i = 0; i < clips_.size(); ++i) {
        if (clips_[i].id == clipId) {
            // Find track index
            for (size_t t = 0; t < tracks_.size(); ++t) {
                if (tracks_[t].id == clips_[i].trackId) {
                    sel.selectedClips.push_back({static_cast<int>(t), static_cast<int>(i)});
                    break;
                }
            }
            break;
        }
    }
    view_.setSelection(sel);
}

void ArrangementController::selectClips(const std::vector<juce::String>& clipIds) {
    selectedClipIds_ = clipIds;
    
    ArrangementSelection sel;
    for (const auto& clipId : clipIds) {
        for (size_t i = 0; i < clips_.size(); ++i) {
            if (clips_[i].id == clipId) {
                for (size_t t = 0; t < tracks_.size(); ++t) {
                    if (tracks_[t].id == clips_[i].trackId) {
                        sel.selectedClips.push_back({static_cast<int>(t), static_cast<int>(i)});
                        break;
                    }
                }
                break;
            }
        }
    }
    view_.setSelection(sel);
}

void ArrangementController::clearSelection() {
    selectedClipIds_.clear();
    view_.clearSelection();
}

void ArrangementController::moveClip(const juce::String& clipId, 
                                      const juce::String& newTrackId, 
                                      double newStartBeats) {
    projectState_.moveClip(clipId, newTrackId, newStartBeats, "Move Clip");
}

void ArrangementController::resizeClip(const juce::String& clipId, double newLengthBeats) {
    auto [trackTree, clipTree] = projectState_.findClip(clipId);
    if (!clipTree.isValid()) return;
    
    double startBeats = clipTree.getProperty(zenith::ProjectState::PROP_START, 0.0);
    projectState_.setClipRange(clipId, startBeats, newLengthBeats, "Resize Clip");
}

void ArrangementController::splitClipAtPlayhead(const juce::String& clipId) {
    auto [trackTree, clipTree] = projectState_.findClip(clipId);
    if (!clipTree.isValid() || !trackTree.isValid()) return;
    
    juce::String trackId = trackTree.getProperty(zenith::ProjectState::PROP_ID);
    juce::int64 playheadSamples = engine_.getPlayheadSamples();
    
    projectState_.splitClip(trackId, clipId, playheadSamples, "Split Clip");
}

void ArrangementController::deleteSelectedClips() {
    for (const auto& clipId : selectedClipIds_) {
        // Use 3-param version to disambiguate (find track for this clip)
        auto [trackTree, clipTree] = projectState_.findClip(clipId);
        if (trackTree.isValid()) {
            juce::String trackId = trackTree.getProperty(zenith::ProjectState::PROP_ID);
            projectState_.deleteClip(trackId, clipId, "Delete Clip");
        }
    }
    selectedClipIds_.clear();
    view_.clearSelection();
}

void ArrangementController::duplicateSelectedClips() {
    if (selectedClipIds_.empty()) {
        return;
    }

    // Calculate offset for duplication (place after the last selected clip)
    double maxEndBeat = 0.0;
    double minStartBeat = std::numeric_limits<double>::max();
    bool hasSelection = false;
    std::vector<juce::String> newClipIds;

    for (const auto& clipId : selectedClipIds_) {
        // Find the clip
        for (const auto& clip : clips_) {
            if (clip.id == clipId) {
                double clipEnd = clip.startBeats + clip.lengthBeats;
                if (clipEnd > maxEndBeat) {
                    maxEndBeat = clipEnd;
                }
                if (clip.startBeats < minStartBeat) {
                    minStartBeat = clip.startBeats;
                }
                hasSelection = true;
                break;
            }
        }
    }
    
    if (!hasSelection) return;

    // Duplicated clips start after the last selected clip
    // Shift amount is the total duration of the selection (start to end)
    double shiftBeats = maxEndBeat - minStartBeat; 
    
    // Fallback if something is weird (single clip overlap?)
    if (shiftBeats <= 0.001) shiftBeats = 1.0;

    // Duplicate each selected clip
    for (const auto& clipId : selectedClipIds_) {
        // Find the clip and its track
        for (const auto& clip : clips_) {
            if (clip.id == clipId) {
                // Find the track ID
                juce::String trackId;
                for (const auto& track : tracks_) {
                    if (track.id == clip.trackId) {
                        trackId = track.id;
                        break;
                    }
                }

                if (trackId.isEmpty()) continue;

                // Create new clip with offset position
                juce::String newClipId = projectState_.createClip(
                    trackId,
                    clip.type,
                    beatsToSamples(clip.startBeats + shiftBeats),
                    beatsToSamples(clip.lengthBeats),
                    clip.name + " Copy",
                    clip.audioFilePath
                );

                if (!newClipId.isEmpty()) {
                    newClipIds.push_back(newClipId);

                    // Copy fade settings
                    projectState_.setClipFade(newClipId, clip.fadeInBeats, clip.fadeOutBeats, "Duplicate Clip Fade");

                    // Copy audio file reference if present
                    if (!clip.audioFilePath.isEmpty()) {
                        juce::ValueTree newClipTree = projectState_.findClip(newClipId).second;
                        if (newClipTree.isValid()) {
                            newClipTree.setProperty(zenith::ProjectState::PROP_AUDIO_FILE, clip.audioFilePath, nullptr);
                        }
                    }
                }
                break;
            }
        }
    }

    // Select the newly created clips
    if (!newClipIds.empty()) {
        selectedClipIds_ = newClipIds;
        fullSync();  // Refresh to show new clips
    }
}

//==============================================================================
// Track Operations
//==============================================================================

void ArrangementController::selectTrack(int trackIndex) {
    selectedTrackIndex_ = trackIndex;
    // Could update view to highlight track
}

void ArrangementController::setTrackMute(int trackIndex, bool muted) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) return;
    projectState_.setTrackMute(tracks_[trackIndex].id, muted);
}

void ArrangementController::setTrackSolo(int trackIndex, bool soloed) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) return;
    projectState_.setTrackSolo(tracks_[trackIndex].id, soloed);
}

void ArrangementController::setTrackArmed(int trackIndex, bool armed) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) return;
    projectState_.setTrackArmed(tracks_[trackIndex].id, armed);
}

//==============================================================================
// Full Sync
//==============================================================================

void ArrangementController::fullSync() {
    syncTempo();
    syncTimeSignature();
    rebuildTrackList();
    rebuildClipList();
    updateViewFromState();
    syncLoopRegion();
    syncPlayhead();
}

void ArrangementController::rebuildTrackList() {
    tracks_.clear();
    
    int numTracks = projectState_.getNumTracks();
    tracks_.reserve(numTracks);
    
    for (int i = 0; i < numTracks; ++i) {
        juce::ValueTree trackTree = projectState_.getTrackByIndex(i);
        if (trackTree.isValid()) {
            tracks_.push_back(extractTrackData(trackTree, i));
        }
    }
}

void ArrangementController::rebuildClipList() {
    clips_.clear();
    
    for (const auto& track : tracks_) {
        juce::ValueTree trackTree = projectState_.getTrack(track.id);
        if (!trackTree.isValid()) continue;
        
        juce::ValueTree clipsContainer = trackTree.getChildWithName(zenith::ProjectState::ID_CLIPS);
        if (!clipsContainer.isValid()) continue;
        
        for (int i = 0; i < clipsContainer.getNumChildren(); ++i) {
            juce::ValueTree clipTree = clipsContainer.getChild(i);
            if (clipTree.hasType(zenith::ProjectState::ID_CLIP)) {
                clips_.push_back(extractClipData(clipTree, track.id));
            }
        }
    }
}

void ArrangementController::updateViewFromState() {
    // Update track count
    view_.setTrackCount(static_cast<int>(tracks_.size()));
    
    // Set track display data for each track
    for (size_t i = 0; i < tracks_.size(); ++i) {
        view_.setTrackHeight(static_cast<int>(i), 80.0f);  // Default height
        
        const auto& track = tracks_[i];
        view_.setTrackData(static_cast<int>(i), 
                          track.name, 
                          track.color, 
                          track.isMuted, 
                          track.isSoloed, 
                          track.isArmed);
    }
    
    // Build track ID to index mapping
    std::map<juce::String, int> trackIdToIndex;
    for (size_t i = 0; i < tracks_.size(); ++i) {
        trackIdToIndex[tracks_[i].id] = static_cast<int>(i);
    }
    
    // Convert ClipViewData to ClipRenderData and push to view
    std::vector<SkiaArrangementView::ClipRenderData> renderClips;
    renderClips.reserve(clips_.size());
    
    for (const auto& clip : clips_) {
        SkiaArrangementView::ClipRenderData renderClip;
        renderClip.id = clip.id;
        renderClip.name = clip.name;
        renderClip.startBeats = clip.startBeats;
        renderClip.lengthBeats = clip.lengthBeats;
        renderClip.color = clip.color;
        renderClip.isMidi = (clip.type == "midi");
        renderClip.isSelected = clip.isSelected;
        renderClip.fadeInBeats = clip.fadeInBeats;
        renderClip.fadeOutBeats = clip.fadeOutBeats;
        renderClip.audioFilePath = clip.audioFilePath;
        
        // Map track ID to track index
        auto it = trackIdToIndex.find(clip.trackId);
        if (it != trackIdToIndex.end()) {
            renderClip.trackIndex = it->second;
            renderClips.push_back(renderClip);
        }
    }
    
    view_.setClips(renderClips);
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void ArrangementController::valueTreePropertyChanged(juce::ValueTree& tree,
                                                      const juce::Identifier& property) {
    // Guard against re-entrant calls
    if (isProcessingValueTreeChange_) {
        return;
    }

    juce::ignoreUnused(tree, property);
    // Incremental updates could be smarter here, but for now just rebuild
    isProcessingValueTreeChange_ = true;
    rebuildTrackList();
    rebuildClipList();
    updateViewFromState();
    isProcessingValueTreeChange_ = false;
}

void ArrangementController::valueTreeChildAdded(juce::ValueTree& parent,
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

void ArrangementController::valueTreeChildRemoved(juce::ValueTree& parent,
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

void ArrangementController::valueTreeChildOrderChanged(juce::ValueTree& parent,
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

void ArrangementController::valueTreeParentChanged(juce::ValueTree& tree) {
    juce::ignoreUnused(tree);
    // Usually don't need to handle this, and guard not needed here
    // as we don't trigger any state changes
}

//==============================================================================
// Timer
//==============================================================================

void ArrangementController::timerCallback() {
    if (engine_.isPlaying()) {
        syncPlayhead();
    }
    
    // Sync meter levels from engine
    syncMeters();
}

void ArrangementController::syncMeters() {
    auto engineTracks = engine_.getTracksSnapshot();
    
    for (size_t i = 0; i < tracks_.size() && i < engineTracks.size(); ++i) {
        if (engineTracks[i]) {
            float level = engineTracks[i]->getMixerChannel().getOutputLevel();
            level = std::clamp(level, 0.0f, 1.0f);
            view_.setTrackMeterLevel(static_cast<int>(i), level);
        }
    }
}

//==============================================================================
// Internal Helpers
//==============================================================================

TrackViewData ArrangementController::extractTrackData(const juce::ValueTree& trackTree, 
                                                       int index) const {
    TrackViewData data;
    
    data.id = trackTree.getProperty(zenith::ProjectState::PROP_ID, "");
    data.name = trackTree.getProperty(zenith::ProjectState::PROP_NAME, "Track " + juce::String(index + 1));
    data.type = trackTree.getProperty(zenith::ProjectState::PROP_TYPE, "audio");
    
    // Get color - try to read from state, fall back to theme color
    if (trackTree.hasProperty(zenith::ProjectState::PROP_COLOR)) {
        juce::String colorStr = trackTree.getProperty(zenith::ProjectState::PROP_COLOR);
        data.color = juce::Colour::fromString(colorStr);
    } else {
        // Use index-based color from theme
        float hue = std::fmod(index * 0.13f, 1.0f);
        data.color = juce::Colour::fromHSV(hue, 0.6f, 0.8f, 1.0f);
    }
    
    data.isMuted = trackTree.getProperty(zenith::ProjectState::PROP_MUTE, false);
    data.isSoloed = trackTree.getProperty(zenith::ProjectState::PROP_SOLO, false);
    data.isArmed = trackTree.getProperty(zenith::ProjectState::PROP_ARMED, false);
    data.volume = trackTree.getProperty(zenith::ProjectState::PROP_VOLUME, 1.0f);
    data.pan = trackTree.getProperty(zenith::ProjectState::PROP_PAN, 0.0f);
    
    return data;
}

ClipViewData ArrangementController::extractClipData(const juce::ValueTree& clipTree, 
                                                     const juce::String& trackId) const {
    ClipViewData data;
    
    data.id = clipTree.getProperty(zenith::ProjectState::PROP_ID, "");
    data.trackId = trackId;
    data.name = clipTree.getProperty(zenith::ProjectState::PROP_NAME, "Clip");
    data.type = clipTree.getProperty(zenith::ProjectState::PROP_TYPE, "audio");
    
    // Get color
    if (clipTree.hasProperty(zenith::ProjectState::PROP_COLOR)) {
        juce::String colorStr = clipTree.getProperty(zenith::ProjectState::PROP_COLOR);
        data.color = juce::Colour::fromString(colorStr);
    } else {
        // Inherit from track
        for (const auto& track : tracks_) {
            if (track.id == trackId) {
                data.color = track.color;
                break;
            }
        }
    }
    
    // Position and length - try beats first, fall back to samples
    if (clipTree.hasProperty(zenith::ProjectState::PROP_START_BEATS)) {
        data.startBeats = clipTree.getProperty(zenith::ProjectState::PROP_START_BEATS, 0.0);
        data.lengthBeats = clipTree.getProperty(zenith::ProjectState::PROP_LENGTH_BEATS, 4.0);
    } else {
        juce::int64 startSamples = clipTree.getProperty(zenith::ProjectState::PROP_START, 0);
        juce::int64 lengthSamples = clipTree.getProperty(zenith::ProjectState::PROP_LENGTH, 0);
        data.startBeats = samplesToBeats(startSamples);
        data.lengthBeats = samplesToBeats(lengthSamples);
    }
    
    data.offsetBeats = clipTree.getProperty(zenith::ProjectState::PROP_OFFSET, 0.0);
    data.fadeInBeats = clipTree.getProperty(zenith::ProjectState::PROP_FADE_IN, 0.0);
    data.fadeOutBeats = clipTree.getProperty(zenith::ProjectState::PROP_FADE_OUT, 0.0);
    data.audioFilePath = clipTree.getProperty(zenith::ProjectState::PROP_AUDIO_FILE, "");
    
    // Check if selected
    data.isSelected = std::find(selectedClipIds_.begin(), selectedClipIds_.end(), data.id) 
                      != selectedClipIds_.end();
    
    return data;
}

double ArrangementController::samplesToBeats(juce::int64 samples) const {
    if (tempo_ <= 0.0 || sampleRate_ <= 0.0) return 0.0;
    double seconds = static_cast<double>(samples) / sampleRate_;
    return seconds * (tempo_ / 60.0);
}

juce::int64 ArrangementController::beatsToSamples(double beats) const {
    if (tempo_ <= 0.0 || sampleRate_ <= 0.0) return 0;
    double seconds = beats / (tempo_ / 60.0);
    return static_cast<juce::int64>(seconds * sampleRate_);
}

void ArrangementController::onClipMoved(const juce::String& clipId, int newTrackIndex, double newStartBeat) {
    // Find the track ID for the new track index
    if (newTrackIndex < 0 || newTrackIndex >= static_cast<int>(tracks_.size())) {
        return;
    }
    
    const juce::String& newTrackId = tracks_[newTrackIndex].id;
    
    // Find the current track for this clip
    juce::String currentTrackId;
    for (const auto& clip : clips_) {
        if (clip.id == clipId) {
            currentTrackId = clip.trackId;
            break;
        }
    }
    
    if (currentTrackId.isEmpty()) {
        return;  // Clip not found
    }
    
    // Move the clip in ProjectState
    if (currentTrackId == newTrackId) {
        // Same track, just update position - get current length
        double currentLength = 4.0;  // Default
        for (const auto& clip : clips_) {
            if (clip.id == clipId) {
                currentLength = clip.lengthBeats;
                break;
            }
        }
        projectState_.setClipRange(clipId, newStartBeat, currentLength, "Move Clip");
    } else {
        // Different track - need to move clip between tracks
        projectState_.moveClip(clipId, newTrackId, newStartBeat, "Move Clip");
    }
}

void ArrangementController::onClipDoubleClicked(const juce::String& clipId) {
    // Find the clip
    for (const auto& clip : clips_) {
        if (clip.id == clipId) {
            // Select the clip
            selectClip(clipId);

            // Notify parent component to open clip editor
            if (onOpenClipEditor) {
                onOpenClipEditor(clipId, clip.trackId);
            }
            break;
        }
    }
}

} // namespace zenith::ui
