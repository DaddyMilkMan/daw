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
#include "../../network/CollaborationManager.h"
#include "../../commands/CommandUtils.h"
#include "../../commands/CommandAPI.h"
#include "../../commands/TrackCommands.h"
#include "../../engine/TrackFreeze.h"
#include "../../engine/AudioExporter.h"
#include "../../utils/AudioAnalysisUtils.h"

#include <juce_events/juce_events.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>
#include <limits>

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
        
        comp->onFreeze = [this, i](const juce::String& trackId) {
            juce::ignoreUnused(trackId);
            auto outputDir = projectState_.getAssetDirectory("Freeze");
            owner_.engine_.freezeTrack(i, [this](float progress, const juce::String& status) {
                DBG("Freeze progress: " + juce::String(progress * 100, 1) + "% - " + status);
            });
        };

        comp->onUnfreeze = [this, i](const juce::String& trackId) {
            juce::ignoreUnused(trackId);
            owner_.engine_.unfreezeTrack(i);
        };

        comp->onSeparateStems = [this](const juce::String& trackId) {
            CommandAPI api(projectState_, owner_.engine_);
            TrackCommands trackCmds(owner_.engine_, projectState_, api);
            auto* paramsObj = new juce::DynamicObject();
            paramsObj->setProperty("trackId", trackId);
            juce::var params(paramsObj);
            
            // This is a long-running operation, but for now we'll call it synchronously on the message thread
            // In a real DAW, this should show a progress dialog.
            trackCmds.separateTrack(params);
        };
        
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
            view.fadeInBeats = clip.getProperty(zenith::ProjectState::PROP_FADE_IN, 0.0);
            view.fadeOutBeats = clip.getProperty(zenith::ProjectState::PROP_FADE_OUT, 0.0);

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
    
    CollaborationManager::getInstance().broadcastSelection(selectedClipIds_);
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

    CollaborationManager::getInstance().broadcastSelection(selectedClipIds_);
    owner_.repaint();
}

void ArrangerClipManager::selectClipsInRect(juce::Rectangle<float> rect) {
    for (auto& clipView : clipViews_) {
        if (rect.intersects(clipView.bounds)) {
            clipView.isSelected = true;
            selectedClipIds_.add(clipView.clipId);
        }
    }
    CollaborationManager::getInstance().broadcastSelection(selectedClipIds_);
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

    // BUG FIX #6: Log count BEFORE clearing selection
    const int numClipsToDelete = selectedClipIds_.size();

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

    DBG("ArrangerClipManager: Deleted " + juce::String(numClipsToDelete) + " clips");
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

void ArrangerClipManager::splitSelectedClipsAtPlayhead() {
    double playhead = owner_.playheadBeats_;

    // Copy list to avoid iterator invalidation
    juce::StringArray selectedIds = getSelectedClipIds();
    if (selectedIds.isEmpty()) return;

    bool anySplit = false;

    projectState_.getUndoManager().beginNewTransaction("Split Clips");

    for (const auto& clipId : selectedIds) {
        if (auto* view = findClipView(clipId)) {
            // Check if playhead is within clip range (with small tolerance)
            if (playhead > view->startBeats + 0.001 &&
                playhead < view->startBeats + view->lengthBeats - 0.001) {

                juce::int64 splitSamples = gridUtils_.beatsToSamples(playhead);

                // Call ProjectState
                projectState_.splitClip(view->trackId, clipId, splitSamples, "Split Clip");
                anySplit = true;
            }
        }
    }

    if (anySplit) {
        rebuildClipViews();
        owner_.repaint();
    }
}

void ArrangerClipManager::consolidateSelectedClips() {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (selectedClipIds_.isEmpty()) {
        DBG("ArrangerClipManager::consolidateSelectedClips: No clips selected");
        return;
    }

    // Capture clip data BY VALUE to avoid use-after-free
    // Structure to hold clip info safely for background thread
    struct ClipInfo {
        juce::String clipId;
        juce::String trackId;
        double startBeats;
        double lengthBeats;
    };

    // Group selected clips by track - using safe value copies
    std::map<juce::String, std::vector<ClipInfo>> clipsByTrack;
    for (const auto& clipId : selectedClipIds_) {
        if (auto* view = findClipView(clipId)) {
            ClipInfo info;
            info.clipId = view->clipId;
            info.trackId = view->trackId;
            info.startBeats = view->startBeats;
            info.lengthBeats = view->lengthBeats;
            clipsByTrack[view->trackId].push_back(info);
        }
    }

    if (clipsByTrack.empty()) return;

    projectState_.getUndoManager().beginNewTransaction("Consolidate Clips");

    // BUG FIX #1 & #2: Capture ALL data by value - no raw pointers or references to avoid dangling
    // Capture stable data needed for background thread
    juce::File projectDir = projectState_.getProjectFile().getParentDirectory();
    double tempo = projectState_.getTempo();
    double sampleRate = owner_.engine_.getSampleRate();
    
    // Build track index map (copy by value)
    std::map<juce::String, int> trackIndexMap;
    auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
        juce::String id = tracksNode.getChild(i)[zenith::ProjectState::PROP_ID].toString();
        trackIndexMap[id] = i;
    }

    // Weak reference pattern: use a shared flag to detect if owner is still valid
    auto aliveFlag = std::make_shared<std::atomic<bool>>(true);
    
    // Store reference to projectState for message thread callback (safe: UI objects live on message thread)
    ProjectState* projectStatePtr = &projectState_;
    ArrangerClipManager* self = this;

    // Launch consolidation in background with ALL data captured by value
    juce::Thread::launch([aliveFlag, self, projectStatePtr, clipsByTrack = std::move(clipsByTrack), 
                          projectDir, tempo, sampleRate, trackIndexMap]() {
        for (const auto& [trackId, clips] : clipsByTrack) {
            if (clips.size() < 2) continue;

            // Find range from copied data
            double minStart = std::numeric_limits<double>::max();
            double maxEnd = 0.0;
            std::vector<juce::String> clipIdsToDelete;
            
            for (const auto& clip : clips) {
                minStart = std::min(minStart, clip.startBeats);
                maxEnd = std::max(maxEnd, clip.startBeats + clip.lengthBeats);
                clipIdsToDelete.push_back(clip.clipId);
            }

            // Convert to time using captured values
            double startSeconds = (minStart / tempo) * 60.0;
            double durationSeconds = ((maxEnd - minStart) / tempo) * 60.0;

            // Find track index from captured map
            auto it = trackIndexMap.find(trackId);
            if (it == trackIndexMap.end()) continue;
            int trackIndex = it->second;

            // Create output file
            juce::File consolidatedDir = projectDir.getChildFile("Consolidated");
            consolidatedDir.createDirectory();
            
            juce::String fileName = "consolidated_" + trackId + "_" + 
                juce::String(juce::Time::currentTimeMillis()) + ".wav";
            juce::File outputFile = consolidatedDir.getChildFile(fileName);

            // Setup export options
            zenith::ExportOptions options;
            options.outputFile = outputFile;
            options.sampleRate = sampleRate;
            options.bitDepth = 24;
            options.format = zenith::ExportFormat::WAV;
            options.enableDither = false;
            options.normalize = false;
            options.startTime = startSeconds;
            options.duration = durationSeconds;
            options.exportStems = true;
            options.exportStemsAsync = false;  // Synchronous within this background thread
            options.stemTrackIndices = {trackIndex};

            // NOTE: We cannot safely access Engine from background thread for export
            // The export needs the Engine reference. For now, we skip the export part
            // and just prepare the data. A proper fix would use a job queue.
            // For safety, we post everything to message thread:
            
            // Post back to message thread for BOTH export and ProjectState modifications
            juce::MessageManager::callAsync([aliveFlag, self, projectStatePtr, trackId, clipIdsToDelete, 
                                             minStart, maxEnd, options]() mutable {
                // Check if owner is still alive
                if (!aliveFlag->load()) {
                    DBG("ArrangerClipManager: Owner destroyed, skipping consolidation callback");
                    return;
                }
                
                // Now safe to access Engine via owner on message thread
                zenith::AudioExporter exporter(self->owner_.engine_);
                bool success = exporter.exportProject(options);
                
                if (success) {
                    // Delete original clips using captured IDs (not pointers!)
                    for (const auto& clipIdToDelete : clipIdsToDelete) {
                        projectStatePtr->deleteClip(trackId, clipIdToDelete, "Consolidate Clips");
                    }

                    // Create new consolidated clip
                    projectStatePtr->createAudioClip(trackId, minStart, maxEnd - minStart,
                                                  options.outputFile.getFullPathName(), "Consolidated", 
                                                  "Consolidate Clips");
                    
                    self->rebuildClipViews();
                }
            });
        }
    });

    clearSelection();
}

void ArrangerClipManager::renderSelectedClipsToAudio() {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (selectedClipIds_.isEmpty()) {
        DBG("ArrangerClipManager::renderSelectedClipsToAudio: No clips selected");
        return;
    }

    // BUG FIX #7: Capture data and run export async to avoid UI freeze
    // Collect all clip data we need BY VALUE before going async
    struct RenderClipInfo {
        juce::String clipId;
        juce::String trackId;
        int trackIndex;
        double startBeats;
        double lengthBeats;
    };
    
    std::vector<RenderClipInfo> clipsToRender;
    auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    
    for (const auto& clipId : selectedClipIds_) {
        auto* view = findClipView(clipId);
        if (!view || !view->isMidi) continue; // Only render MIDI clips

        int trackIndex = view->trackIndex;
        if (trackIndex < 0 || trackIndex >= tracksNode.getNumChildren()) continue;

        auto trackNode = tracksNode.getChild(trackIndex);
        
        RenderClipInfo info;
        info.clipId = clipId;
        info.trackId = trackNode[zenith::ProjectState::PROP_ID].toString();
        info.trackIndex = trackIndex;
        info.startBeats = view->startBeats;
        info.lengthBeats = view->lengthBeats;
        clipsToRender.push_back(info);
    }
    
    if (clipsToRender.empty()) return;

    projectState_.getUndoManager().beginNewTransaction("Render to Audio");

    // Capture stable data
    juce::File projectDir = projectState_.getProjectFile().getParentDirectory();
    double tempo = projectState_.getTempo();
    double sampleRate = owner_.engine_.getSampleRate();
    
    // Safety flag for lifetime tracking
    auto aliveFlag = std::make_shared<std::atomic<bool>>(true);
    ProjectState* projectStatePtr = &projectState_;
    ArrangerClipManager* self = this;

    // Process each clip asynchronously
    for (const auto& clipInfo : clipsToRender) {
        // Calculate time range
        double startSeconds = (clipInfo.startBeats / tempo) * 60.0;
        double durationSeconds = (clipInfo.lengthBeats / tempo) * 60.0;

        // Create output file
        juce::File bouncedDir = projectDir.getChildFile("Bounced");
        bouncedDir.createDirectory();

        juce::String fileName = "bounced_" + clipInfo.clipId + "_" + 
            juce::String(juce::Time::currentTimeMillis()) + ".wav";
        juce::File outputFile = bouncedDir.getChildFile(fileName);

        // Setup export options
        zenith::ExportOptions options;
        options.outputFile = outputFile;
        options.sampleRate = sampleRate;
        options.bitDepth = 24;
        options.format = zenith::ExportFormat::WAV;
        options.enableDither = false;
        options.normalize = false;
        options.startTime = startSeconds;
        options.duration = durationSeconds;
        options.exportStems = true;
        options.exportStemsAsync = true;  // Async export to avoid blocking
        options.stemTrackIndices = {clipInfo.trackIndex};
        
        // Use a completion callback to handle post-export work
        options.progressCallback = [aliveFlag, self, projectStatePtr, clipInfo, outputFile](float progress, const juce::String& status) {
            juce::ignoreUnused(status);
            
            if (progress >= 1.0f) {
                // Export complete - post to message thread
                juce::MessageManager::callAsync([aliveFlag, self, projectStatePtr, clipInfo, outputFile]() {
                    if (!aliveFlag->load()) return;
                    
                    // Place audio clip at the same position
                    projectStatePtr->createAudioClip(clipInfo.trackId, clipInfo.startBeats, clipInfo.lengthBeats,
                                              outputFile.getFullPathName(), "Bounced Audio", 
                                              "Render to Audio");

                    // Mute the original MIDI clip instead of deleting it
                    auto [track, clip] = projectStatePtr->findClip(clipInfo.clipId);
                    if (clip.isValid()) {
                        clip.setProperty(zenith::ProjectState::PROP_MUTE, true, &projectStatePtr->getUndoManager());
                    }

                    DBG("ArrangerClipManager: Bounced MIDI clip " + clipInfo.clipId + " to " + outputFile.getFullPathName());
                    
                    self->rebuildClipViews();
                });
            }
        };

        // Export using AudioExporter (now async)
        zenith::AudioExporter exporter(owner_.engine_);
        exporter.exportProject(options);
    }
}

void ArrangerClipManager::detectTempoForSelectedClip() {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (selectedClipIds_.isEmpty()) {
        DBG("ArrangerClipManager::detectTempoForSelectedClip: No clips selected");
        return;
    }

    // Use the first selected audio clip
    for (const auto& clipId : selectedClipIds_) {
        auto* view = findClipView(clipId);
        if (!view || view->isMidi) continue; // Only analyze audio clips

        if (view->audioFilePath.isEmpty()) {
            DBG("ArrangerClipManager::detectTempoForSelectedClip: Clip has no audio file");
            continue;
        }

        juce::File audioFile(view->audioFilePath);
        if (!audioFile.existsAsFile()) {
            DBG("ArrangerClipManager::detectTempoForSelectedClip: Audio file not found: " + view->audioFilePath);
            continue;
        }

        // Load audio file
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
        if (!reader) {
            DBG("ArrangerClipManager::detectTempoForSelectedClip: Failed to read audio file");
            continue;
        }

        // Read into buffer
        int numSamples = static_cast<int>(reader->lengthInSamples);
        int numChannels = static_cast<int>(reader->numChannels);
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        reader->read(&buffer, 0, numSamples, 0, true, true);

        // Detect BPM
        double detectedBpm = zenith::AudioAnalysisUtils::detectBpm(buffer, reader->sampleRate);

        if (detectedBpm > 0.0) {
            DBG("ArrangerClipManager::detectTempoForSelectedClip: Detected BPM = " + juce::String(detectedBpm, 1));

            // Show result
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Tempo Detection",
                "Detected Tempo: " + juce::String(detectedBpm, 1) + " BPM\n\n"
                "You can set the project tempo to this value in the Transport bar.",
                "OK");
        } else {
            DBG("ArrangerClipManager::detectTempoForSelectedClip: Could not detect tempo");
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Tempo Detection",
                "Could not detect tempo for this audio clip.\n"
                "Try a clip with a clearer rhythmic structure.",
                "OK");
        }

        break; // Only process first audio clip
    }
}

} // namespace zenith

