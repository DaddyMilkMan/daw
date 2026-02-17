/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation - Core component and event dispatch
 * 
 * This file contains the ArrangerComponent core functionality. Most logic has
 * been delegated to specialized helper classes:
 * - ArrangerGridUtils: Coordinate conversion and waveform caching
 * - ArrangerClipManager: Clip lifecycle and selection
 * - ArrangerInputHandler: Mouse and keyboard input
 * - ArrangerRenderer: Skia drawing
 */

#include "ArrangerComponent.h"
#include "ArrangerGridUtils.h"
#include "ArrangerClipManager.h"
#include "ArrangerInputHandler.h"
#include "ArrangerTrackComponent.h"

#ifdef ZENITH_USE_SKIA
#include "ArrangerRenderer.h"
#endif

// Zenith Includes
#include "../../browser/BrowserDragSource.h"
#include "../../engine/Track.h"
#include "GridResolutionDropdown.h"
#include "ZenithDesignSystem.h"

// JUCE Includes
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace zenith {

namespace {
juce::PluginDescription resolvePluginDescription(Engine& engine,
                                                 const juce::String& keyOrName) {
    juce::PluginDescription resolved;
    const auto descriptions = engine.getPluginHost().getPluginDescriptions();
    for (const auto& desc : descriptions) {
        if (desc.fileOrIdentifier == keyOrName || desc.name == keyOrName) {
            resolved = desc;
            break;
        }
    }
    return resolved;
}

Track* findTrackById(Engine& engine, const juce::String& trackId) {
    for (const auto& track : engine.tracks()) {
        if (track != nullptr && track->getTrackId() == trackId)
            return track.get();
    }
    return nullptr;
}
} // namespace

//==============================================================================
// Layout Constants - USE DESIGN SYSTEM (Single Source of Truth)
//==============================================================================
static constexpr float HEADER_WIDTH = zenith::design::dimensions::ARRANGER_HEADER_WIDTH;
static constexpr float SECTION_HEIGHT = zenith::design::dimensions::ARRANGER_SECTION_HEIGHT;
static constexpr float RULER_HEIGHT = zenith::design::dimensions::ARRANGER_RULER_HEIGHT;
static constexpr float TRACK_HEIGHT = zenith::design::dimensions::ARRANGER_TRACK_HEIGHT;
static constexpr float TOP_MARGIN = zenith::design::dimensions::ARRANGER_TOP_MARGIN;

//==============================================================================
// Constructor & Destructor
//==============================================================================

ArrangerComponent::ArrangerComponent(Engine& eng, ProjectState& ps, CommandAPI& api)
    : engine_(eng), projectState(ps), commandAPI(api) {

    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    setWantsKeyboardFocus(true);

    // Create helper modules
    gridUtils_ = std::make_unique<ArrangerGridUtils>(*this, engine_, projectState);
    clipManager_ = std::make_unique<ArrangerClipManager>(*this, projectState, *gridUtils_);
    inputHandler_ = std::make_unique<ArrangerInputHandler>(*this, projectState, *clipManager_, *gridUtils_);
    
#ifdef ZENITH_USE_SKIA
    renderer_ = std::make_unique<ArrangerRenderer>(*this, engine_, projectState, *clipManager_, *gridUtils_);
#endif

    // Listen to ProjectState changes
    projectState.getState().addListener(this);

    // Initial clip view build
    clipManager_->rebuildClipViews();

    // Start timer for playhead position updates (60Hz for smooth visual feedback)
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60);

    // Initialize Macro Toolbar
    macroToolbar = std::make_unique<MacroToolbar>(engine_, projectState);
    addChildComponent(macroToolbar.get());

    // Initialize Grid Dropdown
    gridDropdown = std::make_unique<GridResolutionDropdown>();
    addAndMakeVisible(gridDropdown.get());
    gridDropdown->setResolution(gridResolution_);
    gridDropdown->onResolutionChanged = [this](GridResolution res) {
        setGridResolution(res);
    };

    // Initialize MiniMap
    addAndMakeVisible(&miniMap);
    miniMap.setAlwaysOnTop(true);

    // Initialize Timeline Ruler
    addAndMakeVisible(timelineRuler);
    timelineRuler.onSeek = [this](double beat) {
        engine_.setPlayheadSamples(gridUtils_->beatsToSamples(beat));
    };
    
    timelineRuler.onLoopChanged = [this](double start, double end) {
        if (end <= start) end = start + 0.25;
        juce::int64 startSamples = gridUtils_->beatsToSamples(start);
        juce::int64 endSamples = gridUtils_->beatsToSamples(end);
        
        engine_.setLoopRegion(startSamples, endSamples);
        if (!engine_.isLooping()) engine_.setLooping(true);
    };

    macroToolbar->getSelectedClipIds = [this]() { 
        return clipManager_->getSelectedClipIds(); 
    };
    macroToolbar->getSelectedTrackId = [this]() {
        const auto& selectedIds = clipManager_->getSelectedClipIds();
        if (selectedIds.isEmpty())
            return juce::String();
        auto* view = clipManager_->findClipView(selectedIds[0]);
        return view ? view->trackId : juce::String();
    };

    // Setup Freeze Progress Callback
    macroToolbar->onFreezeProgress = [this](float progress,
                                            const juce::String &status) {
      if (!freezeOverlay)
        return;

      if (!freezeOverlay->isVisible())
        freezeOverlay->setVisible(true);

      freezeOverlay->setProgress(progress);
      freezeOverlay->setStatus(status);

      // Hide when done
      if (progress >= 1.0f) {
        freezeOverlay->setVisible(false);
      }
    };

    // Initialize Freeze Overlay
    freezeOverlay = std::make_unique<FreezeProgressOverlay>();
    addAndMakeVisible(freezeOverlay.get());
    freezeOverlay->setVisible(false);
    freezeOverlay->onCancel = [this]() { engine_.cancelFreeze(); };

    // Initialize Section Track
    sectionTrack.reset(new ArrangerTrackComponent(projectState, *gridUtils_,
                                                  ArrangerTrackComponent::TrackType::Section));
    addAndMakeVisible(sectionTrack.get());

    DBG("ArrangerComponent: Created");
}

ArrangerComponent::~ArrangerComponent() {
    stopTimer();
    projectState.getState().removeListener(this);
    DBG("ArrangerComponent: Destroyed");
}

//==============================================================================
// ValueTree::Listener Interface
//==============================================================================

void ArrangerComponent::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property) {
    juce::ignoreUnused(tree, property);
    clipManager_->rebuildClipViews();
    repaint();
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree& parent,
                                            juce::ValueTree& child) {
    juce::ignoreUnused(parent, child);
    clipManager_->rebuildClipViews();
    repaint();
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree& parent,
                                              juce::ValueTree& child,
                                              int index) {
    juce::ignoreUnused(parent, child, index);
    clipManager_->rebuildClipViews();
    repaint();
}

void ArrangerComponent::valueTreeChildOrderChanged(juce::ValueTree& parent,
                                                   int oldIndex, int newIndex) {
    juce::ignoreUnused(parent, oldIndex, newIndex);
    clipManager_->rebuildClipViews();
    repaint();
}

//==============================================================================
// Component Interface - Layout
//==============================================================================

void ArrangerComponent::resized() {
    clipManager_->recomputeClipBounds();

    // Position MiniMap at the top right
    int mapHeight = 60;
    int mapWidth = 300;
    miniMap.setBounds(getWidth() - mapWidth - 10, 5, mapWidth, mapHeight);

    if (sectionTrack) {
        sectionTrack->setBounds(HEADER_WIDTH, 0, getWidth() - HEADER_WIDTH,
                                static_cast<int>(SECTION_HEIGHT));
    }
    
    if (gridDropdown) {
        gridDropdown->setBounds(HEADER_WIDTH - 80, SECTION_HEIGHT + 3, 70, RULER_HEIGHT - 6);
    }

    timelineRuler.setBounds(HEADER_WIDTH, SECTION_HEIGHT, getWidth() - HEADER_WIDTH, RULER_HEIGHT);
    if (getWidth() > HEADER_WIDTH) {
        timelineRuler.setVisibleRange(viewStartBeats, (getWidth() - HEADER_WIDTH) / pixelsPerBeat);
    }

    if (macroToolbar) {
        float w = 420.0f;
        float h = 60.0f;
        float x = (getWidth() - w) * 0.5f;
        float y = RULER_HEIGHT + 20.0f;
        macroToolbar->setBounds(static_cast<int>(x), static_cast<int>(y), 
                                static_cast<int>(w), static_cast<int>(h));
    }

    if (freezeOverlay) {
        freezeOverlay->setBounds(getLocalBounds());
    }

    // Layout Tracks
    for (size_t i = 0; i < trackComponents.size(); ++i) {
        float y = gridUtils_->trackIndexToY(static_cast<int>(i));
        if (y + TRACK_HEIGHT < TOP_MARGIN || y > getHeight()) {
            trackComponents[i]->setVisible(false);
        } else {
            trackComponents[i]->setVisible(true);
            trackComponents[i]->setBounds(0, static_cast<int>(y), getWidth(), 
                                          static_cast<int>(TRACK_HEIGHT));
            trackComponents[i]->setViewContext(pixelsPerBeat, viewStartBeats);
        }
    }
}

//==============================================================================
// Mouse Event Delegation
//==============================================================================

void ArrangerComponent::mouseDown(const juce::MouseEvent& e) {
    inputHandler_->mouseDown(e);
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent& e) {
    inputHandler_->mouseDrag(e);
}

void ArrangerComponent::mouseUp(const juce::MouseEvent& e) {
    inputHandler_->mouseUp(e);
}

void ArrangerComponent::mouseMove(const juce::MouseEvent& e) {
    inputHandler_->mouseMove(e);
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    inputHandler_->mouseDoubleClick(e);
}

void ArrangerComponent::mouseWheelMove(const juce::MouseEvent& e,
                                       const juce::MouseWheelDetails& wheel) {
    inputHandler_->mouseWheelMove(e, wheel);
}

bool ArrangerComponent::keyPressed(const juce::KeyPress& key) {
    return inputHandler_->keyPressed(key);
}

juce::String ArrangerComponent::getTooltip() {
    return inputHandler_->getTooltip();
}

//==============================================================================
// Skia Rendering
//=============================================================================

#ifdef ZENITH_USE_SKIA
void ArrangerComponent::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // 1. Draw Background
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_DARKEST);
    canvas->drawRect(skBounds, bgPaint);

    // 2. Draw Child SkiaComponents (Tracks, Ruler, MiniMap, etc.)
    // Note: Tracks should be behind clips (which are drawn in step 3)
    drawChildren(canvas);

    // 3. Draw ArrangerRenderer (Grid, Clips, Playhead, etc.)
    if (renderer_) {
        renderer_->drawSkia(canvas);
    }
}
#endif

//==============================================================================
// Grid Resolution
//==============================================================================

void ArrangerComponent::setGridResolution(GridResolution res) {
    gridResolution_ = res;
    gridSnapBeats = gridResolutionToBeats(res);
    repaint();
}

//==============================================================================
// Timer Callback - Playhead Updates
//==============================================================================

void ArrangerComponent::timerCallback() {
    updatePlayheadFromEngine();
}

void ArrangerComponent::updatePlayheadFromEngine() {
    // Get current playhead position from engine
    juce::int64 positionSamples = engine_.getPlayheadSamples();
    double newPlayheadBeats = gridUtils_->samplesToBeats(positionSamples);

    bool wasPlaying = isPlaying_;
    isPlaying_ = engine_.isPlaying();

    // Get loop state
    bool wasLoopEnabled = loopEnabled_;
    loopEnabled_ = engine_.isLooping();
    
    if (loopEnabled_) {
        juce::int64 loopStart = engine_.getLoopStart();
        juce::int64 loopEnd = engine_.getLoopEnd();
        loopStartBeats_ = gridUtils_->samplesToBeats(loopStart);
        loopEndBeats_ = gridUtils_->samplesToBeats(loopEnd);
    }

    // Check if playhead moved significantly
    if (std::abs(newPlayheadBeats - playheadBeats_) > 0.01 || wasPlaying != isPlaying_) {
        playheadBeats_ = newPlayheadBeats;

        // Auto-scroll if following and playing
        if (followPlayhead_ && isPlaying_) {
            float playheadX = gridUtils_->beatsToX(playheadBeats_);
            float visibleWidth = getWidth() - HEADER_WIDTH;

            // Scroll if playhead is near right edge
            if (playheadX > HEADER_WIDTH + visibleWidth * 0.85f) {
                viewStartBeats = playheadBeats_ - (visibleWidth * 0.15f / pixelsPerBeat);
                viewStartBeats = juce::jmax(0.0, viewStartBeats);
                clipManager_->recomputeClipBounds();
            }
        }
        
        timelineRuler.setVisibleRange(viewStartBeats, (getWidth() - HEADER_WIDTH) / pixelsPerBeat);

        repaint();
    }

    if (wasLoopEnabled != loopEnabled_) {
        repaint();
    }
    
    // Sync loop state to ruler
    timelineRuler.setLoopRange(loopStartBeats_, loopEndBeats_, loopEnabled_);
}

//==============================================================================
// DragAndDropTarget Interface
//==============================================================================

bool ArrangerComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails& details) {
    juce::String description = details.description.toString();
    
    // Check for browser drag
    if (description.startsWith("browser:") ||
        BrowserDragSource::isBrowserDrag(description) ||
        description.startsWith("zenith_plugin|")) {
        return true;
    }
    
    // Check for file drag
    if (auto* files = dynamic_cast<juce::StringArray*>(details.description.getDynamicObject())) {
        return !files->isEmpty();
    }
    
    return false;
}

void ArrangerComponent::itemDragEnter(
    const juce::DragAndDropTarget::SourceDetails& details) {
    juce::ignoreUnused(details);
    isDropTargetActive_ = true;
    repaint();
}

void ArrangerComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails& details) {
    juce::ignoreUnused(details);
    isDropTargetActive_ = false;
    dropTargetTrackIndex_ = -1;
    repaint();
}

void ArrangerComponent::itemDragMove(
    const juce::DragAndDropTarget::SourceDetails& details) {
    juce::Point<float> pt = details.localPosition.toFloat();
    
    dropTargetTrackIndex_ = gridUtils_->yToTrackIndex(pt.y);
    dropTargetBeats_ = gridUtils_->snapToGrid(gridUtils_->xToBeats(pt.x));
    
    repaint();
}

void ArrangerComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& details) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    isDropTargetActive_ = false;

    juce::String description = details.description.toString();
    juce::Point<float> pt = details.localPosition.toFloat();

    int trackIndex = gridUtils_->yToTrackIndex(pt.y);
    double beats = gridUtils_->snapToGrid(gridUtils_->xToBeats(pt.x));

    if (trackIndex < 0 || beats < 0) {
        dropTargetTrackIndex_ = -1;
        repaint();
        return;
    }

    // Get target track
    auto tracksNode = projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIndex >= tracksNode.getNumChildren()) {
        dropTargetTrackIndex_ = -1;
        repaint();
        return;
    }

    auto track = tracksNode.getChild(trackIndex);
    juce::String trackId = track[zenith::ProjectState::PROP_ID].toString();

    // Handle browser drag
    if (description.startsWith("browser:")) {
        juce::String path = description.fromFirstOccurrenceOf("browser:", false, true);
        juce::File file(path);

        if (file.existsAsFile()) {
            // Determine if audio or MIDI based on extension
            juce::String ext = file.getFileExtension().toLowerCase();
            bool isMidi = (ext == ".mid" || ext == ".midi");

            // Create clip
            double lengthBeats = 4.0; // Default, will be updated based on file
            
            juce::String clipName = file.getFileNameWithoutExtension();
            juce::String clipId = projectState.createEmptyClip(
                trackId, beats, lengthBeats, isMidi, clipName, "Drop file");

            // If audio, set the audio file path
            if (!isMidi && clipId.isNotEmpty()) {
                auto [foundTrack, clip] = projectState.findClip(clipId);
                if (clip.isValid()) {
                    clip.setProperty(zenith::ProjectState::PROP_AUDIO_FILE,
                                     file.getFullPathName(),
                                     &projectState.getUndoManager());
                }
            }

            DBG("ArrangerComponent: Dropped file at " + juce::String(beats) +
                " beats on track " + trackId);
        }
    } else if (BrowserDragSource::isBrowserDrag(description)) {
        BrowserItemType type = BrowserItemType::Unknown;
        juce::String itemId;
        juce::String itemName;
        if (BrowserDragSource::parseDragDescription(description, type, itemId, itemName)) {
            juce::ignoreUnused(itemName);
            if (type == BrowserItemType::AudioFile || type == BrowserItemType::MidiFile) {
                juce::File file(itemId);
                if (file.existsAsFile()) {
                    const bool isMidi = (type == BrowserItemType::MidiFile);
                    juce::String clipName = file.getFileNameWithoutExtension();
                    juce::String clipId = projectState.createEmptyClip(
                        trackId, beats, 4.0, isMidi, clipName, "Drop browser file");
                    if (!isMidi && clipId.isNotEmpty()) {
                        auto [foundTrack, clip] = projectState.findClip(clipId);
                        juce::ignoreUnused(foundTrack);
                        if (clip.isValid()) {
                            clip.setProperty(zenith::ProjectState::PROP_AUDIO_FILE,
                                             file.getFullPathName(),
                                             &projectState.getUndoManager());
                        }
                    }
                }
            } else if (type == BrowserItemType::Plugin ||
                       type == BrowserItemType::Instrument) {
                if (auto* targetTrack = findTrackById(engine_, trackId)) {
                    const auto resolved = resolvePluginDescription(engine_, itemId);
                    if (!resolved.name.isEmpty() || !resolved.fileOrIdentifier.isEmpty()) {
                        juce::String error;
                        const double sampleRate = engine_.getSampleRate() > 0.0 ? engine_.getSampleRate() : 44100.0;
                        const int blockSize = engine_.getBufferSize() > 0 ? engine_.getBufferSize() : 512;
                        if (auto plugin = engine_.getPluginHost().createInstance(
                                resolved, sampleRate, blockSize, error)) {
                            targetTrack->addPlugin(std::move(plugin));
                        }
                    }
                }
            }
        }
    } else if (description.startsWith("zenith_plugin|")) {
        auto pluginKey = description.fromFirstOccurrenceOf("zenith_plugin|", false, false)
                                     .upToFirstOccurrenceOf("|", false, false);
        if (auto* targetTrack = findTrackById(engine_, trackId)) {
            const auto resolved = resolvePluginDescription(engine_, pluginKey);
            if (!resolved.name.isEmpty() || !resolved.fileOrIdentifier.isEmpty()) {
                juce::String error;
                const double sampleRate = engine_.getSampleRate() > 0.0 ? engine_.getSampleRate() : 44100.0;
                const int blockSize = engine_.getBufferSize() > 0 ? engine_.getBufferSize() : 512;
                if (auto plugin = engine_.getPluginHost().createInstance(
                        resolved, sampleRate, blockSize, error)) {
                    targetTrack->addPlugin(std::move(plugin));
                }
            }
        }
    }

    dropTargetTrackIndex_ = -1;
    repaint();
}

} // namespace zenith
