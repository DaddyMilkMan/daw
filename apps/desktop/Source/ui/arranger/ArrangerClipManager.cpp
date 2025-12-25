/**
 * @file ArrangerClipManager.cpp
 * @brief Implementation of clip management for ArrangerComponent
 */

#include "ArrangerClipManager.h"
#include "ArrangerComponent.h"
#include "ArrangerGridUtils.h"
#include "ArrangerTrackComponent.h"
#include "MiniMapComponent.h"
#include "ProjectState.h"

#include <juce_events/juce_events.h>

namespace zenith {

//==============================================================================
// Layout Constants (must match ArrangerComponent.cpp)
//==============================================================================
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float SECTION_HEIGHT = 24.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT = 80.0f;
static constexpr float TOP_MARGIN = SECTION_HEIGHT + RULER_HEIGHT;

//==============================================================================
// Constructor
//==============================================================================

ArrangerClipManager::ArrangerClipManager(ArrangerComponent& owner, ProjectState& projectState, ArrangerGridUtils& gridUtils)
    : owner_(owner)
    , projectState_(projectState)
    , gridUtils_(gridUtils)
{
}

//==============================================================================
// Clip View Management
//==============================================================================

void ArrangerClipManager::rebuildClipViews() {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    clipViews_.clear();

    auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int trackIndex = 0;
    for (const auto& track : tracksNode) {
        processTrackClips(track, trackIndex);
        trackIndex++;
    }

    recomputeClipBounds();

    // Update MiniMap Data
    std::vector<MiniMapComponent::MiniMapClip> mapClips;
    double maxBeat = 1.0;
    int maxTrack = 1;

    for (const auto& view : clipViews_) {
        MiniMapComponent::MiniMapClip mc;
        mc.startBeats = view.startBeats;
        mc.lengthBeats = view.lengthBeats;
        mc.trackIndex = view.trackIndex;
        mc.isMidi = view.isMidi;
        mc.isSelected = view.isSelected;

        mapClips.push_back(mc);

        if (view.startBeats + view.lengthBeats > maxBeat)
            maxBeat = view.startBeats + view.lengthBeats;
        if (view.trackIndex + 1 > maxTrack)
            maxTrack = view.trackIndex + 1;
    }

    // Update MiniMap
    owner_.miniMap.setArrangementData(maxBeat + 8.0 /* padding */, maxTrack, mapClips);

    // Also update visible range immediately
    double visibleBeats = (owner_.getWidth() - HEADER_WIDTH) / owner_.pixelsPerBeat;
    int visibleTracks = (int)(owner_.getHeight() - RULER_HEIGHT) / (int)TRACK_HEIGHT;
    owner_.miniMap.setVisibleRange(owner_.viewStartBeats, visibleBeats, owner_.firstVisibleTrackIndex, visibleTracks);
    
    rebuildTrackComponents();
}

void ArrangerClipManager::rebuildTrackComponents() {
    auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    if (!tracksNode.isValid()) {
        owner_.trackComponents.clear();
        return;
    }
    
    int numTracks = tracksNode.getNumChildren();
    size_t required = static_cast<size_t>(numTracks);
    
    // Add if needed
    while (owner_.trackComponents.size() < required) {
        auto type = ArrangerTrackComponent::TrackType::Audio;
        auto newTrack = std::make_unique<ArrangerTrackComponent>(projectState_, gridUtils_, type);
        owner_.addChildComponent(newTrack.get());
        owner_.trackComponents.push_back(std::move(newTrack));
    }
    
    // Remove if needed
    while (owner_.trackComponents.size() > required) {
        owner_.trackComponents.pop_back();
    }
    
    // Update Data
    for (int i = 0; i < numTracks; ++i) {
        auto trackNode = tracksNode.getChild(i);
        auto* comp = owner_.trackComponents[i].get();
        
        comp->setTrackId(trackNode[zenith::ProjectState::PROP_ID].toString());
        comp->setTrackName(trackNode[zenith::ProjectState::PROP_NAME].toString());
        comp->setTrackIndex(i);
        comp->setViewContext(owner_.pixelsPerBeat, owner_.viewStartBeats);
        
        // Sync Mute/Solo/Rec state from ValueTree
        bool isMuted = trackNode.getProperty(zenith::ProjectState::PROP_MUTE, false);
        bool isSoloed = trackNode.getProperty(zenith::ProjectState::PROP_SOLO, false);
        bool isArmed = trackNode.getProperty(zenith::ProjectState::PROP_ARMED, false);
        comp->setMuted(isMuted);
        comp->setSoloed(isSoloed);
        comp->setRecordArmed(isArmed);
        
        comp->updateTakeFolders();
    }
    
    owner_.resized();
}

void ArrangerClipManager::recomputeClipBounds() {
    // Update section track view state
    if (owner_.sectionTrack) {
        owner_.sectionTrack->setVisibleRange(owner_.viewStartBeats, owner_.pixelsPerBeat);
    }

    const float trackHeight = static_cast<float>(TRACK_HEIGHT - 4); // 2px margin top/bottom

    for (auto& clipView : clipViews_) {
        float x = gridUtils_.beatsToX(clipView.startBeats);
        float y = gridUtils_.trackIndexToY(clipView.trackIndex);
        float width = static_cast<float>(clipView.lengthBeats * owner_.pixelsPerBeat);

        clipView.bounds = juce::Rectangle<float>(x, y + 2.0f, width, trackHeight);
    }
}

ClipView* ArrangerClipManager::findClipView(const juce::String& clipId) {
    for (auto& clipView : clipViews_) {
        if (clipView.clipId == clipId)
            return &clipView;
    }
    return nullptr;
}

ClipView* ArrangerClipManager::findClipAtPoint(juce::Point<float> point) {
    // Search in reverse order so topmost clips are hit first
    for (int i = clipViews_.size() - 1; i >= 0; --i) {
        if (clipViews_.getReference(i).bounds.contains(point))
            return &clipViews_.getReference(i);
    }
    return nullptr;
}

void ArrangerClipManager::processTrackClips(const juce::ValueTree& track, int trackIndex) {
    auto trackId = track[zenith::ProjectState::PROP_ID].toString();
    auto clipsNode = track.getChildWithName(zenith::ProjectState::ID_CLIPS);

    if (clipsNode.isValid()) {
        for (const auto& clip : clipsNode) {
            // Skip Take Folders (handled by ArrangerTrackComponent)
            if (clip.hasType(zenith::ProjectState::ID_TAKE_FOLDER))
                continue;

            ClipView view;
            view.clipId = clip[zenith::ProjectState::PROP_ID].toString();
            view.trackId = trackId;
            view.trackIndex = trackIndex;
            view.startBeats = clip.getProperty(zenith::ProjectState::PROP_START_BEATS);
            view.lengthBeats = clip.getProperty(zenith::ProjectState::PROP_LENGTH_BEATS);

            auto clipType = clip[zenith::ProjectState::PROP_TYPE].toString();
            view.isMidi = (clipType == "midi");

            view.isSelected = selectedClipIds_.contains(view.clipId);

            // Populate clip content for thumbnail rendering
            if (view.isMidi) {
                // Get MIDI notes for blob preview
                auto notes = projectState_.getMidiNotesForClip(view.clipId);
                for (const auto& note : notes) {
                    MidiNoteBlob blob;
                    blob.pitch = note.pitch;
                    blob.startBeats = note.startBeats;
                    blob.lengthBeats = note.lengthBeats;
                    view.noteBlobs.push_back(blob);
                }
            } else {
                // Get audio file path for waveform preview
                view.audioFilePath = clip[zenith::ProjectState::PROP_AUDIO_FILE].toString();

                // Trigger waveform cache build if needed
                if (view.audioFilePath.isNotEmpty()) {
                    gridUtils_.buildWaveformCache(view.audioFilePath);
                }
            }

            clipViews_.add(view);
        }
    }
}

//==============================================================================
// Selection Management
//==============================================================================

void ArrangerClipManager::clearSelection() {
    selectedClipIds_.clear();
    for (auto& clipView : clipViews_)
        clipView.isSelected = false;
    owner_.repaint();
}

void ArrangerClipManager::selectClip(const juce::String& clipId, bool addToSelection) {
    if (!addToSelection)
        clearSelection();

    if (selectedClipIds_.contains(clipId)) {
        // Toggle off if adding to selection
        if (addToSelection) {
            selectedClipIds_.removeString(clipId);
            if (auto* view = findClipView(clipId))
                view->isSelected = false;
        }
    } else {
        selectedClipIds_.add(clipId);
        if (auto* view = findClipView(clipId))
            view->isSelected = true;
    }

    owner_.repaint();
}

void ArrangerClipManager::selectClipsInRect(juce::Rectangle<float> rect) {
    for (auto& clipView : clipViews_) {
        if (rect.intersects(clipView.bounds)) {
            clipView.isSelected = true;
            selectedClipIds_.add(clipView.clipId);
        }
    }
    owner_.repaint();
}

bool ArrangerClipManager::isClipSelected(const juce::String& clipId) const {
    return selectedClipIds_.contains(clipId);
}

//==============================================================================
// Clip Operations
//==============================================================================

void ArrangerClipManager::createClipAtPoint(juce::Point<float> point) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (point.x < HEADER_WIDTH)
        return; // Don't create clips in header

    int trackIndex = gridUtils_.yToTrackIndex(point.y);
    if (trackIndex < 0)
        return;

    // Get track at index
    auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIndex >= tracksNode.getNumChildren())
        return;

    auto track = tracksNode.getChild(trackIndex);
    auto trackId = track[zenith::ProjectState::PROP_ID].toString();

    // Calculate clip position
    double startBeats = gridUtils_.snapToGrid(gridUtils_.xToBeats(point.x));
    double lengthBeats = 4.0; // Default 4 beats (1 bar in 4/4)

    // Create clip via ProjectState
    projectState_.createEmptyClip(trackId, startBeats, lengthBeats, true, "Clip", "Create clip");

    DBG("ArrangerClipManager: Created clip at " + juce::String(startBeats) +
        " beats on track " + trackId);
}

void ArrangerClipManager::deleteSelectedClips() {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (selectedClipIds_.isEmpty())
        return;

    // Begin single undo transaction for all deletes
    projectState_.getUndoManager().beginNewTransaction("Delete clips");

    // Delete all selected clips
    for (const auto& clipId : selectedClipIds_) {
        auto [track, clip] = projectState_.findClip(clipId);
        if (track.isValid() && clip.isValid()) {
            juce::String trackId = track.getProperty(zenith::ProjectState::PROP_ID).toString();
            projectState_.deleteClip(trackId, clipId, "Delete clips");
        }
    }

    clearSelection();

    DBG("ArrangerClipManager: Deleted " + juce::String(selectedClipIds_.size()) + " clips");
}

void ArrangerClipManager::duplicateSelectedClips() {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (selectedClipIds_.isEmpty())
        return;

    // Begin single undo transaction
    projectState_.getUndoManager().beginNewTransaction("Duplicate clips");

    juce::Array<juce::String> newClipIds;

    // Duplicate each selected clip
    for (const auto& clipId : selectedClipIds_) {
        auto [track, clip] = projectState_.findClip(clipId);
        if (!clip.isValid())
            continue;

        auto trackId = track[ProjectState::PROP_ID].toString();
        double startBeats = clip[ProjectState::PROP_START_BEATS];
        double lengthBeats = clip[ProjectState::PROP_LENGTH_BEATS];
        bool isMidi = (clip[ProjectState::PROP_TYPE].toString() == "midi");
        auto name = clip[ProjectState::PROP_NAME].toString();

        // Place duplicate after original
        double newStart = startBeats + lengthBeats;

        auto newClipId = projectState_.createEmptyClip(
            trackId, newStart, lengthBeats, isMidi, name + " copy", "Duplicate clips");
        newClipIds.add(newClipId);
    }

    // Select the new clips
    clearSelection();
    for (const auto& newId : newClipIds)
        selectedClipIds_.add(newId);

    rebuildClipViews();

    DBG("ArrangerClipManager: Duplicated " + juce::String(newClipIds.size()) + " clips");
}

} // namespace zenith
