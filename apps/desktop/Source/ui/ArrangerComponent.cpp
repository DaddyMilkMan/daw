/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation
 */

#include "../../include/ui/ArrangerComponent.h"

// Skia Includes
#ifdef ZENITH_USE_SKIA
#include "skia/GlassmorphicPanel.h"
#include "skia/NeonGlow.h"
#include <core/SkBlurTypes.h> // For SkBlurStyle enum
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkFontMgr.h>
#include <core/SkFontStyle.h>
#include <core/SkMaskFilter.h> // For SkMaskFilter::MakeBlur
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <core/SkSpan.h> // For SkSpan used by SkDashPathEffect
#include <core/SkTypeface.h>
#include <effects/SkDashPathEffect.h> // For SkDashPathEffect::Make
#include <effects/SkGradientShader.h>

#endif

// Zenith Includes
#include "../../Source/engine/AudioFilePool.h"
#include "../browser/BrowserDragSource.h"
#include "../engine/AudioFilePool.h"
#include "skia/ZenithDesignSystem.h" // Explicitly include to make typography visible
#include "skia/ZenithUtils.h"

// JUCE Includes
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

// Standard Library
#include <algorithm>
#include <cmath>
#include <vector>

//==============================================================================
namespace zenith {

// Constants
// Constants
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float SECTION_HEIGHT = 24.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT =
    80.0f; // Taller tracks for better visibility
static constexpr float TOP_MARGIN =
    SECTION_HEIGHT + RULER_HEIGHT; // Offset for tracks
static constexpr float SCROLLBAR_HEIGHT = 14.0f;

//==============================================================================

ArrangerComponent::ArrangerComponent(Engine &eng, ProjectState &ps)
    : engine_(eng), projectState(ps) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  setWantsKeyboardFocus(true);

  // Listen to ProjectState changes
  projectState.getState().addListener(this);

  // Initial clip view build
  rebuildClipViews();

  // Start timer for playhead position updates (30Hz is plenty for visual
  // feedback)
  startTimerHz(30);

  // Initialize Macro Toolbar
  macroToolbar = std::make_unique<MacroToolbar>(engine_, projectState);
  addChildComponent(macroToolbar.get());

  // Initialize MiniMap
  addAndMakeVisible(&miniMap);
  miniMap.setAlwaysOnTop(true);

  macroToolbar->getSelectedClipIds = [this]() { return selectedClipIds; };
  macroToolbar->getSelectedTrackId = [this]() {
    if (selectedClipIds.isEmpty())
      return juce::String();
    auto *view = findClipView(selectedClipIds[0]);
    return view ? view->trackId : juce::String();
  };

  DBG("ArrangerComponent: Created");
}

ArrangerComponent::~ArrangerComponent() {
  stopTimer();
  projectState.getState().removeListener(this);
  DBG("ArrangerComponent: Destroyed");
}

//==============================================================================
// ValueTree::Listener interface
//==============================================================================

void ArrangerComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  juce::ignoreUnused(tree, property);
  // Clip property changed (start, length, etc.)
  rebuildClipViews();
  repaint();
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                            juce::ValueTree &child) {
  juce::ignoreUnused(parent, child);
  // Track or clip added
  rebuildClipViews();
  repaint();
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                              juce::ValueTree &child,
                                              int index) {
  juce::ignoreUnused(parent, child, index);
  // Track or clip removed
  rebuildClipViews();
  repaint();
}

void ArrangerComponent::valueTreeChildOrderChanged(juce::ValueTree &parent,
                                                   int oldIndex, int newIndex) {
  juce::ignoreUnused(parent, oldIndex, newIndex);
  rebuildClipViews();
  repaint();
}

//==============================================================================
// Clip view management
//==============================================================================

void ArrangerComponent::rebuildClipViews() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  clipViews.clear();

  auto tracksNode =
      projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (!tracksNode.isValid())
    return;

  int trackIndex = 0;
  for (const auto &track : tracksNode) {
    auto trackId = track[zenith::ProjectState::PROP_ID].toString();
    auto clipsNode = track.getChildWithName(zenith::ProjectState::ID_CLIPS);

    if (clipsNode.isValid()) {
      for (const auto &clip : clipsNode) {
        ClipView view;
        view.clipId = clip[zenith::ProjectState::PROP_ID].toString();
        view.trackId = trackId;
        view.startBeats = clip[zenith::ProjectState::PROP_START];
        view.lengthBeats = clip[zenith::ProjectState::PROP_LENGTH];

        auto clipType = clip[zenith::ProjectState::PROP_TYPE].toString();
        view.isMidi = (clipType == "midi");

        view.isSelected = selectedClipIds.contains(view.clipId);

        // Populate clip content for thumbnail rendering
        if (view.isMidi) {
          // Get MIDI notes for blob preview
          auto notes = projectState.getMidiNotesForClip(view.clipId);
          for (const auto &note : notes) {
            MidiNoteBlob blob;
            blob.pitch = note.pitch;
            blob.startBeats = note.startBeats;
            blob.lengthBeats = note.lengthBeats;
            view.noteBlobs.push_back(blob);
          }
        } else {
          // Get audio file path for waveform preview
          view.audioFilePath =
              clip[zenith::ProjectState::PROP_AUDIO_FILE].toString();

          // Trigger waveform cache build if needed
          if (view.audioFilePath.isNotEmpty()) {
            buildWaveformCache(view.audioFilePath);
          }
        }

        clipViews.add(view);
      }
    }

    trackIndex++;
  }

  recomputeClipBounds();

  // Update MiniMap Data
  std::vector<MiniMapComponent::MiniMapClip> mapClips;
  double maxBeat = 1.0;
  int maxTrack = 1;

  for (const auto &view : clipViews) {
    MiniMapComponent::MiniMapClip mc;
    mc.startBeats = view.startBeats;
    mc.lengthBeats = view.lengthBeats;
    // We need track index for Y pos
    // We can find it by iterating tracks or storing it in ClipView
    // (optimization for later) For now, re-find it (performance warning, but
    // fast enough for small projects)
    int tIdx = 0;
    auto tracks = projectState.getState().getChildWithName(
        zenith::ProjectState::ID_TRACKS);
    for (const auto &t : tracks) {
      if (t[zenith::ProjectState::PROP_ID].toString() == view.trackId)
        break;
      tIdx++;
    }
    mc.trackIndex = tIdx;
    mc.isMidi = view.isMidi;
    mc.isSelected = view.isSelected;

    mapClips.push_back(mc);

    if (view.startBeats + view.lengthBeats > maxBeat)
      maxBeat = view.startBeats + view.lengthBeats;
    if (tIdx + 1 > maxTrack)
      maxTrack = tIdx + 1;
  }

  // Update MiniMap
  miniMap.setArrangementData(maxBeat + 8.0 /* padding */, maxTrack, mapClips);

  // Also update visible range immediately
  double visibleBeats = (getWidth() - HEADER_WIDTH) / pixelsPerBeat;
  int visibleTracks = (int)(getHeight() - RULER_HEIGHT) / (int)TRACK_HEIGHT;
  miniMap.setVisibleRange(viewStartBeats, visibleBeats, firstVisibleTrackIndex,
                          visibleTracks);
}

void ArrangerComponent::recomputeClipBounds() {
  // Update section track view state
  if (sectionTrack) {
    sectionTrack->setVisibleRange(viewStartBeats, pixelsPerBeat);
  }

  for (auto &clipView : clipViews) {
    // Find track index for this clip
    int trackIndex = 0;
    auto tracksNode = projectState.getState().getChildWithName(
        zenith::ProjectState::ID_TRACKS);
    if (tracksNode.isValid()) {
      for (const auto &track : tracksNode) {
        if (track[zenith::ProjectState::PROP_ID].toString() == clipView.trackId)
          break;
        trackIndex++;
      }
    }

    float x = beatsToX(clipView.startBeats);
    float y = trackIndexToY(trackIndex);
    float width = static_cast<float>(clipView.lengthBeats * pixelsPerBeat);
    float height =
        static_cast<float>(TRACK_HEIGHT - 4); // 2px margin top/bottom

    clipView.bounds = juce::Rectangle<float>(x, y + 2.0f, width, height);
  }
}

ArrangerComponent::ClipView *
ArrangerComponent::findClipView(const juce::String &clipId) {
  for (auto &clipView : clipViews) {
    if (clipView.clipId == clipId)
      return &clipView;
  }
  return nullptr;
}

ArrangerComponent::ClipView *
ArrangerComponent::findClipAtPoint(juce::Point<float> point) {
  // Search in reverse order so topmost clips are hit first
  for (int i = clipViews.size() - 1; i >= 0; --i) {
    if (clipViews.getReference(i).bounds.contains(point))
      return &clipViews.getReference(i);
  }
  return nullptr;
}

//==============================================================================
// Coordinate conversion
//==============================================================================

float ArrangerComponent::beatsToX(double beats) const {
  return HEADER_WIDTH +
         static_cast<float>((beats - viewStartBeats) * pixelsPerBeat);
}

double ArrangerComponent::xToBeats(float x) const {
  return viewStartBeats + ((x - HEADER_WIDTH) / pixelsPerBeat);
}

float ArrangerComponent::trackIndexToY(int trackIndex) const {
  return TOP_MARGIN + (trackIndex - firstVisibleTrackIndex) * TRACK_HEIGHT;
}

int ArrangerComponent::yToTrackIndex(float y) const {
  if (y < TOP_MARGIN)
    return -1;

  return firstVisibleTrackIndex +
         static_cast<int>((y - TOP_MARGIN) / TRACK_HEIGHT);
}

double ArrangerComponent::snapToGrid(double beats) const {
  return std::round(beats / gridSnapBeats) * gridSnapBeats;
}

//==============================================================================
// Selection management
//==============================================================================

void ArrangerComponent::clearSelection() {
  selectedClipIds.clear();
  for (auto &clipView : clipViews)
    clipView.isSelected = false;
  repaint();
}

void ArrangerComponent::selectClip(const juce::String &clipId,
                                   bool addToSelection) {
  if (!addToSelection)
    clearSelection();

  if (selectedClipIds.contains(clipId)) {
    // Toggle off if adding to selection
    if (addToSelection) {
      selectedClipIds.removeString(clipId);
      if (auto *view = findClipView(clipId))
        view->isSelected = false;
    }
  } else {
    selectedClipIds.add(clipId);
    if (auto *view = findClipView(clipId))
      view->isSelected = true;
  }

  repaint();
}

void ArrangerComponent::selectClipsInRect(juce::Rectangle<float> rect) {
  for (auto &clipView : clipViews) {
    if (rect.intersects(clipView.bounds)) {
      clipView.isSelected = true;
      selectedClipIds.add(clipView.clipId);
    }
  }
  repaint();
}

bool ArrangerComponent::isClipSelected(const juce::String &clipId) const {
  return selectedClipIds.contains(clipId);
}

//==============================================================================
// Clip operations
//==============================================================================

void ArrangerComponent::createClipAtPoint(juce::Point<float> point) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (point.x < HEADER_WIDTH)
    return; // Don't create clips in header

  int trackIndex = yToTrackIndex(point.y);
  if (trackIndex < 0)
    return;

  // Get track at index
  auto tracksNode =
      projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (!tracksNode.isValid() || trackIndex >= tracksNode.getNumChildren())
    return;

  auto track = tracksNode.getChild(trackIndex);
  auto trackId = track[zenith::ProjectState::PROP_ID].toString();

  // Calculate clip position
  double startBeats = snapToGrid(xToBeats(point.x));
  double lengthBeats = 4.0; // Default 4 beats (1 bar in 4/4)

  // Create clip via ProjectState
  projectState.createEmptyClip(trackId, startBeats, lengthBeats, true, "Clip",
                               "Create clip");

  DBG("ArrangerComponent: Created clip at " + juce::String(startBeats) +
      " beats on track " + trackId);
}

void ArrangerComponent::deleteSelectedClips() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (selectedClipIds.isEmpty())
    return;

  // Begin single undo transaction for all deletes
  projectState.getUndoManager().beginNewTransaction("Delete clips");

  // Delete all selected clips
  for (const auto &clipId : selectedClipIds) {
    auto [track, clip] = projectState.findClip(clipId);
    if (track.isValid() && clip.isValid()) {
      juce::String trackId =
          track.getProperty(zenith::ProjectState::PROP_ID).toString();
      projectState.deleteClip(trackId, clipId, "Delete clips");
    }
  }

  clearSelection();

  DBG("ArrangerComponent: Deleted " + juce::String(selectedClipIds.size()) +
      " clips");
}

void ArrangerComponent::duplicateSelectedClips() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (selectedClipIds.isEmpty())
    return;

  // Begin single undo transaction
  projectState.getUndoManager().beginNewTransaction("Duplicate clips");

  juce::Array<juce::String> newClipIds;

  // Duplicate each selected clip
  for (const auto &clipId : selectedClipIds) {
    auto [track, clip] = projectState.findClip(clipId);
    if (!clip.isValid())
      continue;

    auto trackId = track[ProjectState::PROP_ID].toString();
    double startBeats = clip[ProjectState::PROP_START_BEATS];
    double lengthBeats = clip[ProjectState::PROP_LENGTH_BEATS];
    bool isMidi = (clip[ProjectState::PROP_TYPE].toString() == "midi");
    auto name = clip[ProjectState::PROP_NAME].toString();

    // Place duplicate after original
    double newStart = startBeats + lengthBeats;

    auto newClipId =
        projectState.createEmptyClip(trackId, newStart, lengthBeats, isMidi,
                                     name + " copy", "Duplicate clips");
    newClipIds.add(newClipId);
  }

  // Select the new clips
  clearSelection();
  for (const auto &newId : newClipIds)
    selectedClipIds.add(newId);

  rebuildClipViews();

  DBG("ArrangerComponent: Duplicated " + juce::String(newClipIds.size()) +
      " clips");
}

//==============================================================================
// Component interface - Painting (Pure Skia - All rendering in drawSkia())
//==============================================================================

void ArrangerComponent::resized() {
  recomputeClipBounds();

  // Position MiniMap at the top right, fixed height
  // It acts as a global navigation bar
  int mapHeight = 60;
  int mapWidth = 300; // Fixed width or proportional? Prompt implies reduction
                      // of entire arrangement.
  // If it replaces scrollbars, maybe it spans the width?
  // "MiniMapComponent to replace the scrollbars" often implies a strip.
  // Let's make it a strip at the top, spanning relative to arrangement length?
  // Usually mini-maps are fixed width or fill available width.
  // I will make it fixed width at top right for now, essentially a "Navigator".

  miniMap.setBounds(getWidth() - mapWidth - 10, 5, mapWidth, mapHeight);

  if (macroToolbar) {
    // Center horizontally, float near top (offset by Ruler + padding)
    float w = 420.0f;
    float h = 60.0f;
    float x = (getWidth() - w) * 0.5f;
    float y = RULER_HEIGHT + 20.0f;
    macroToolbar->setBounds((int)x, (int)y, (int)w, (int)h);
  }
}

//==============================================================================
// Component interface - Mouse handling
//==============================================================================
// NOSONAR - Complexity acceptable for rendering logic

void ArrangerComponent::mouseDown(const juce::MouseEvent &e) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  grabKeyboardFocus();

  dragStartPoint = e.position;
  currentDragMode = DragMode::None;

  auto *clip = findClipAtPoint(e.position);

  if (clip != nullptr) {
    // Check for resize zones (standard logic)
    if (clip->isInLeftResizeZone(e.position)) {
      currentDragMode = DragMode::ResizeClipLeft;
      resizingClipId = clip->clipId;
      resizeOriginalStart = clip->startBeats;
      resizeOriginalLength = clip->lengthBeats;
    } else if (clip->isInRightResizeZone(e.position)) {
      currentDragMode = DragMode::ResizeClipRight;
      resizingClipId = clip->clipId;
      resizeOriginalStart = clip->startBeats;
      resizeOriginalLength = clip->lengthBeats;
    } else {
      // Move mode
      currentDragMode = DragMode::MoveClips;

      // Determine Edit Mode based on modifiers
      if (e.mods.isAltDown() && e.mods.isShiftDown()) {
        currentEditMode = EditMode::Insert;
      } else if (e.mods.isAltDown()) {
        currentEditMode = EditMode::Ripple;
      } else {
        currentEditMode = EditMode::Overwrite;
      }

      // Populate initial starts for robust Ripple/Insert calculations
      initialClipStarts.clear();
      for (const auto &view : clipViews) {
        initialClipStarts[view.clipId] = view.startBeats;
      }

      bool isCtrlOrCmd = e.mods.isCommandDown();

      if (!clip->isSelected) {
        selectClip(clip->clipId, isCtrlOrCmd);
      } else if (isCtrlOrCmd) {
        selectClip(clip->clipId, true);
      }

      // Cache original positions
      clipDragStates.clear();
      for (const auto &clipId : selectedClipIds) {
        if (auto *view = findClipView(clipId)) {
          int trackIndex = 0;
          auto tracksNode = projectState.getState().getChildWithName(
              zenith::ProjectState::ID_TRACKS);
          if (tracksNode.isValid()) {
            for (const auto &track : tracksNode) {
              if (track[zenith::ProjectState::PROP_ID].toString() ==
                  view->trackId)
                break;
              trackIndex++;
            }
          }
          ClipDragState state;
          state.clipId = clipId;
          state.originalStartBeats = view->startBeats;
          state.originalTrackIndex = trackIndex;
          clipDragStates.add(state);
        }
      }
    }
  } else {
    // Clicked empty area
    if (e.position.x < HEADER_WIDTH && e.position.y > RULER_HEIGHT) {
      // Track Header Interaction
      int trackIndex = yToTrackIndex(e.position.y);
      auto tracksNode = projectState.getState().getChildWithName(
          zenith::ProjectState::ID_TRACKS);
      if (tracksNode.isValid() && trackIndex >= 0 &&
          trackIndex < tracksNode.getNumChildren()) {
        auto track = tracksNode.getChild(trackIndex);
        // Basic hit testing for M/S/R buttons
        // Assuming buttons are at x=120 (M), 150 (S), 180 (R) approx
        float relativeX = e.position.x;
        float rowY = trackIndexToY(trackIndex);
        float relY = e.position.y - rowY;

        // Layout: Name (0-110), M(120), S(150), R(180)
        if (relY >= 45 && relY <= 70) { // Button row
          if (relativeX >= 120 && relativeX <= 145) {
            bool m = track[zenith::ProjectState::PROP_MUTE];
            track.setProperty(zenith::ProjectState::PROP_MUTE, !m,
                              &projectState.getUndoManager());
          } else if (relativeX >= 150 && relativeX <= 175) {
            bool s = track[zenith::ProjectState::PROP_SOLO];
            track.setProperty(zenith::ProjectState::PROP_SOLO, !s,
                              &projectState.getUndoManager());
          } else if (relativeX >= 180 && relativeX <= 205) {
            bool r = track[zenith::ProjectState::PROP_ARMED];
            track.setProperty(zenith::ProjectState::PROP_ARMED, !r,
                              &projectState.getUndoManager());
          }
        }
      }
    } else {
      // Timeline Interaction
      bool isShift = e.mods.isShiftDown();
      if (isShift) {
        currentDragMode = DragMode::Marquee;
        marqueeRect = juce::Rectangle<float>(e.position, e.position);
      } else {
        clearSelection();
      }
    }
  }
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent &e) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (currentDragMode == DragMode::None) {
    if (e.getDistanceFromDragStart() > 5) {
      currentDragMode = DragMode::Marquee;
      marqueeRect = juce::Rectangle<float>(dragStartPoint, e.position);
      repaint();
    }
    return;
  }

  if (currentDragMode == DragMode::MoveClips) {
    // Calculate delta
    double deltaBeats = xToBeats(e.position.x) - xToBeats(dragStartPoint.x);
    int deltaTrackIndex =
        yToTrackIndex(e.position.y) - yToTrackIndex(dragStartPoint.y);

    // OPTIMIZATION: Early exit if visual delta is negligible
    // This prevents expensive ripple recalculations on every single pixel of
    // mouse jitter

    // Only recalc if moved more than micro-amount or track changed
    if (std::abs(deltaBeats - lastDragDeltaBeats_) < 0.001 &&
        deltaTrackIndex == lastDragDeltaTrack_) {
      return;
    }
    lastDragDeltaBeats_ = deltaBeats;
    lastDragDeltaTrack_ = deltaTrackIndex;

    // Reset Insertion Guide
    insertionGuideX = -1.0f;
    double minNewStartForGuide = 10000000.0; // Large value

    // 1. Find earliest original start of SELECTED clips on each track
    // This defines the "Wavefront" of the ripple
    std::map<juce::String, double> trackEarliestSelectedStart;

    // Also track which clips are selected for fast lookup
    std::map<juce::String, bool> isClipSelectedMap;

    for (const auto &clipId : selectedClipIds) {
      if (auto *view = findClipView(clipId)) {
        isClipSelectedMap[clipId] = true;
        double start = initialClipStarts[clipId];
        if (trackEarliestSelectedStart.find(view->trackId) ==
            trackEarliestSelectedStart.end()) {
          trackEarliestSelectedStart[view->trackId] = start;
        } else {
          trackEarliestSelectedStart[view->trackId] =
              std::min(trackEarliestSelectedStart[view->trackId], start);
        }
      }
    }

    // 2. Update Clip Positions
    for (auto &view : clipViews) {
      if (isClipSelectedMap[view.clipId]) {
        // --- Selected Clip: Follow Mouse ---
        double initial = initialClipStarts[view.clipId];
        double newStart = initial + deltaBeats;
        newStart = juce::jmax(0.0, newStart);
        view.startBeats = newStart;

        minNewStartForGuide = std::min(minNewStartForGuide, newStart);

        // Update Track ID (only for selected clips)
        // Find original track index from drag states
        for (const auto &ds : clipDragStates) {
          if (ds.clipId == view.clipId) {
            int newTrackIndex = ds.originalTrackIndex + deltaTrackIndex;
            newTrackIndex = juce::jmax(0, newTrackIndex);

            auto tracksNode = projectState.getState().getChildWithName(
                zenith::ProjectState::ID_TRACKS);
            if (tracksNode.isValid() &&
                newTrackIndex < tracksNode.getNumChildren()) {
              auto newTrack = tracksNode.getChild(newTrackIndex);
              view.trackId = newTrack[zenith::ProjectState::PROP_ID].toString();
            }
            break;
          }
        }
      } else if (currentEditMode == EditMode::Ripple ||
                 currentEditMode == EditMode::Insert) {
        // --- Unselected Clip: Apply Ripple/Insert ---
        // Check if this clip belongs to a track affected by the drag
        auto trackIt = trackEarliestSelectedStart.find(view.trackId);
        if (trackIt != trackEarliestSelectedStart.end()) {
          double earliestSel = trackIt->second;
          double initial = initialClipStarts[view.clipId];

          // If this clip starts AT or AFTER the ripple wavefront
          if (initial >= earliestSel) {
            // Apply delta
            double newStart = initial + deltaBeats;
            newStart = juce::jmax(0.0, newStart);
            view.startBeats = newStart;
          }
        }
      }
    }

    // 3. Set Insertion Guide Visibility
    if ((currentEditMode == EditMode::Ripple ||
         currentEditMode == EditMode::Insert) &&
        minNewStartForGuide < 10000000.0) {
      insertionGuideX = beatsToX(minNewStartForGuide);
    } // Overwrite mode doesn't show guide

    recomputeClipBounds();
    repaint();

  } else if (currentDragMode == DragMode::ResizeClipLeft) {
    if (auto *view = findClipView(resizingClipId)) {
      double newStart = xToBeats(e.position.x);
      double originalEnd = resizeOriginalStart + resizeOriginalLength;
      double newLength = originalEnd - newStart;

      // Enforce minimum length
      if (newLength < 0.25) {
        newStart = originalEnd - 0.25;
        newLength = 0.25;
      }

      view->startBeats = newStart;
      view->lengthBeats = newLength;

      recomputeClipBounds();
      repaint();
    }
  } else if (currentDragMode == DragMode::ResizeClipRight) {
    if (auto *view = findClipView(resizingClipId)) {
      double newEnd = xToBeats(e.position.x);
      double newLength = newEnd - resizeOriginalStart;

      // Enforce minimum length
      newLength = juce::jmax(0.25, newLength);

      view->lengthBeats = newLength;

      recomputeClipBounds();
      repaint();
    }
  } else if (currentDragMode == DragMode::Marquee) {
    marqueeRect = juce::Rectangle<float>(dragStartPoint, e.position);
    repaint();
  }
}

void ArrangerComponent::mouseUp(const juce::MouseEvent &e) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  juce::ignoreUnused(e);

  if (currentDragMode == DragMode::MoveClips) {
    // Commit move to ProjectState
    if (!selectedClipIds.isEmpty()) {
      projectState.getUndoManager().beginNewTransaction("Move clips");

      // Iterate over ALL clips to see which ones moved (handles Ripple/Insert
      // automatically)
      for (const auto &view : clipViews) {
        // Check if this clip started somewhere else
        auto it = initialClipStarts.find(view.clipId);
        if (it != initialClipStarts.end()) {
          double initialStart = it->second;
          double currentStart = view.startBeats;

          // Also check if track changed
          auto [track, clip] = projectState.findClip(view.clipId);
          if (clip.isValid()) {
            juce::String currentTrackIdInState =
                track[zenith::ProjectState::PROP_ID].toString();

            double snappedStart = snapToGrid(currentStart);

            // If moved significantly or track changed
            if (std::abs(snappedStart - snapToGrid(initialStart)) > 0.001 ||
                view.trackId != currentTrackIdInState) {
              projectState.moveClip(view.clipId, view.trackId, snappedStart,
                                    "Move clips");
            }
          }
        }
      }

      DBG("ArrangerComponent: Committed move for clips (EditMode: " +
          juce::String((int)currentEditMode) + ")");
    }

    clipDragStates.clear();
    initialClipStarts.clear();
    insertionGuideX = -1.0f; // Clear guide
  } else if (currentDragMode == DragMode::ResizeClipLeft ||
             currentDragMode == DragMode::ResizeClipRight) {
    // Commit resize to ProjectState
    if (auto *view = findClipView(resizingClipId)) {
      double snappedStart = snapToGrid(view->startBeats);
      double snappedLength = snapToGrid(view->lengthBeats);

      projectState.setClipRange(resizingClipId, snappedStart, snappedLength,
                                "Resize clip");

      DBG("ArrangerComponent: Committed resize for clip " + resizingClipId);
    }

    resizingClipId.clear();
  } else if (currentDragMode == DragMode::Marquee) {
    selectClipsInRect(marqueeRect);
    marqueeRect = juce::Rectangle<float>();
  }

  currentDragMode = DragMode::None;
  repaint();
}

void ArrangerComponent::mouseMove(const juce::MouseEvent &e) {
  // Update cursor based on hover position
  auto *clip = findClipAtPoint(e.position);

  if (macroToolbar) {
    macroToolbar->checkProximity(e.position);
  }

  if (clip != nullptr) {
    if (clip->isInLeftResizeZone(e.position) ||
        clip->isInRightResizeZone(e.position))
      setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else
      setMouseCursor(juce::MouseCursor::DraggingHandCursor);
  } else {
    setMouseCursor(juce::MouseCursor::NormalCursor);
  }
}

juce::String ArrangerComponent::getTooltip() {
  // Return tooltip text for hovered clip
  auto mousePos = getMouseXYRelative();
  auto *clip = findClipAtPoint(mousePos.toFloat());

  if (clip != nullptr) {
    auto [track, clipNode] = projectState.findClip(clip->clipId);
    if (clipNode.isValid()) {
      auto clipName = clipNode[zenith::ProjectState::PROP_NAME].toString();
      auto startBeats =
          clipNode[zenith::ProjectState::PROP_START_BEATS].toString();
      auto lengthBeats =
          clipNode[zenith::ProjectState::PROP_LENGTH_BEATS].toString();
      return clipName + " (" + startBeats + " beats, " + lengthBeats +
             " beats)";
    }
  }

  return {};
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto *clip = findClipAtPoint(e.position);

  if (clip == nullptr) {
    // Double-clicked empty area - create clip
    createClipAtPoint(e.position);
  } else {
    // Double-clicked existing clip - trigger callback
    if (onClipDoubleClicked) {
      onClipDoubleClicked(clip->trackId, clip->clipId);
    }
  }
}

void ArrangerComponent::mouseWheelMove(const juce::MouseEvent &e,
                                       const juce::MouseWheelDetails &wheel) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  bool isShift = e.mods.isShiftDown();
  bool isCtrlOrCmd = e.mods.isCommandDown();

  if (isCtrlOrCmd) {
    // Zoom horizontal
    double zoomFactor = 1.0 + (wheel.deltaY * 0.5);
    // double oldPixelsPerBeat = pixelsPerBeat;  // Unused variable
    pixelsPerBeat *= zoomFactor;
    pixelsPerBeat = juce::jlimit(10.0, 200.0, pixelsPerBeat);

    // Zoom around mouse position
    double beatsAtMouse = xToBeats(e.position.x);
    double pixelsAtMouse = e.position.x;
    viewStartBeats = beatsAtMouse - (pixelsAtMouse / pixelsPerBeat);
    viewStartBeats = juce::jmax(0.0, viewStartBeats);

    recomputeClipBounds();
    recomputeClipBounds();

    // Update MiniMap
    double visibleBeats = (double)(getWidth() - HEADER_WIDTH) / pixelsPerBeat;
    int visibleTracks = (int)((getHeight() - RULER_HEIGHT) / TRACK_HEIGHT);
    miniMap.setVisibleRange(viewStartBeats, visibleBeats,
                            firstVisibleTrackIndex, visibleTracks);

    repaint();
  } else if (isShift) {
    // Scroll horizontal
    viewStartBeats -= wheel.deltaY * 2.0;
    viewStartBeats = juce::jmax(0.0, viewStartBeats);

    recomputeClipBounds();
    recomputeClipBounds();

    // Update MiniMap
    double visibleBeats = (double)(getWidth() - HEADER_WIDTH) / pixelsPerBeat;
    int visibleTracks = (int)((getHeight() - RULER_HEIGHT) / TRACK_HEIGHT);
    miniMap.setVisibleRange(viewStartBeats, visibleBeats,
                            firstVisibleTrackIndex, visibleTracks);

    repaint();
  } else {
    // Scroll vertical
    firstVisibleTrackIndex -= static_cast<int>(wheel.deltaY * 2.0);

    auto tracksNode = projectState.getState().getChildWithName(
        zenith::ProjectState::ID_TRACKS);
    int maxTrackIndex =
        tracksNode.isValid() ? tracksNode.getNumChildren() - 1 : 0;

    firstVisibleTrackIndex =
        juce::jlimit(0, maxTrackIndex, firstVisibleTrackIndex);

    repaint();
  }
}

#ifdef ZENITH_USE_SKIA
void ArrangerComponent::drawSkia(SkCanvas *canvas) {
  using namespace zenith::design;
  auto bounds = getLocalBounds();
  float width = (float)bounds.getWidth();
  float height = (float)bounds.getHeight();

  // ============================================================================
  // 1. GLOBAL BACKGROUND (Deep Slate)
  // ============================================================================
  canvas->clear(colors::BG_DARKEST);

  // ============================================================================
  // 2. GRID & TIMELINE
  // ============================================================================
  SkPaint gridPaint;
  gridPaint.setColor(colors::BORDER_SUBTLE);
  gridPaint.setStrokeWidth(1.0f);
  gridPaint.setAntiAlias(true);
  // Dotted line effect - using SkSpan for modern Skia API
  SkScalar intervals[] = {2.0f, 4.0f};
  gridPaint.setPathEffect(
      SkDashPathEffect::Make(SkSpan<const SkScalar>(intervals, 2), 0.0f));

  double startBeat = std::floor(viewStartBeats);
  double endBeat = viewStartBeats + ((width - HEADER_WIDTH) / pixelsPerBeat);

  // Draw Vertical Grid Lines (Time)
  // Optimization: Don't draw every beat if zoomed out too far
  double beatStep = (pixelsPerBeat < 20.0) ? 4.0 : 1.0;

  int trackCount = 0;
  auto tracksNode =
      projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (tracksNode.isValid()) {
    trackCount = tracksNode.getNumChildren();
  }

  // Draw grid only within the timeline area
  canvas->save();
  // Clip to area below sections and ruler? Or just below sections?
  // Grid usually goes through ruler? Or starts below?
  // Original code: canvas->drawLine(x, 0, x, height, gridPaint);
  // We should start below sections (SECTION_HEIGHT).
  canvas->clipRect(SkRect::MakeXYWH(HEADER_WIDTH, SECTION_HEIGHT,
                                    width - HEADER_WIDTH,
                                    height - SECTION_HEIGHT));

  for (double beat = startBeat; beat <= endBeat; beat += beatStep) {
    float x = beatsToX(beat);
    canvas->drawLine(x, SECTION_HEIGHT, x, height, gridPaint);
  }
  canvas->restore();

  // Highlighting for Section Hover/Drag
  if (sectionTrack) {
    const auto *section = sectionTrack->getHoveredSection();
    if (!section)
      section = sectionTrack->getDraggingSection();

    if (section) {
      float sx = beatsToX(section->startBeats);
      float sl = (float)(section->lengthBeats * pixelsPerBeat);

      if (sl > 0) {
        SkPaint highlightPaint;
        // Parse section color or use accent
        juce::Colour c = juce::Colour::fromString(section->color);
        SkColor sc = SkColorSetARGB(40, c.getRed(), c.getGreen(),
                                    c.getBlue()); // Transparent

        highlightPaint.setColor(sc);
        highlightPaint.setStyle(SkPaint::kFill_Style);

        // Draw highlight strip (below ruler or full height?)
        // "highlight the background of the arrangement view for that time
        // range"
        canvas->drawRect(
            SkRect::MakeXYWH(sx, SECTION_HEIGHT, sl, height - SECTION_HEIGHT),
            highlightPaint);
      }
    }
  }

  // ============================================================================
  // 3. TRACKS RENDER LOOP
  // ============================================================================
  if (tracksNode.isValid()) {
    SkPaint trackBgPaint;
    trackBgPaint.setStyle(SkPaint::kFill_Style);

    SkPaint dividerPaint;
    dividerPaint.setColor(colors::BORDER_DEFAULT);
    dividerPaint.setStrokeWidth(1.0f);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors::TEXT_PRIMARY);

    // Use design system fonts for consistent typography
    SkFont nameFont =
        typography::getSkFont(typography::FONT_MD, FontWeight::Medium);
    SkFont smallFont =
        typography::getSkFont(typography::FONT_XS, FontWeight::Regular);

    for (int i = firstVisibleTrackIndex; i < trackCount; ++i) {
      float y = trackIndexToY(i);
      if (y > height)
        break;

      float trackHeight = TRACK_HEIGHT;
      SkRect trackRect = SkRect::MakeXYWH(0, y, width, trackHeight);
      SkRect headerRect = SkRect::MakeXYWH(0, y, HEADER_WIDTH, trackHeight);

      // A. Track Header Background (Slightly lighter than timeline)
      trackBgPaint.setColor(colors::BG_DARKER);
      canvas->drawRect(headerRect, trackBgPaint);

      // B. Track Timeline Background (Transparent/Darkest)
      // (Already cleared to BG_DARKEST, effectively)

      // C. Separator
      canvas->drawLine(0, y + trackHeight, width, y + trackHeight,
                       dividerPaint);

      // D. Header Content
      auto track = tracksNode.getChild(i);
      juce::String name = track[zenith::ProjectState::PROP_NAME].toString();

      // Track Name
      textPaint.setColor(colors::TEXT_PRIMARY);
      canvas->drawString(name.toStdString().c_str(), spacing::MD, y + 25.0f,
                         nameFont, textPaint);

      // Controls (Mute/Solo/Rec) - Modern "Pills"
      float btnY = y + 40.0f;
      float btnSize = 18.0f;
      float btnGap = 24.0f;
      float startX = spacing::MD;

      // Helper for buttons
      auto drawTrackButton = [&](float bx, const char *label, bool active,
                                 SkColor activeColor) {
        SkRect btnRect = SkRect::MakeXYWH(bx, btnY, btnSize, btnSize);
        SkPaint btnPaint;
        btnPaint.setAntiAlias(true);

        if (active) {
          btnPaint.setColor(activeColor);
          btnPaint.setStyle(SkPaint::kFill_Style);

          // Glow effect for active state
          SkPaint glowPaint;
          glowPaint.setAntiAlias(true);
          glowPaint.setColor(withAlpha(activeColor, 0.4f));
          glowPaint.setMaskFilter(
              SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
          canvas->drawCircle(bx + btnSize / 2, btnY + btnSize / 2,
                             btnSize / 2 + 2, glowPaint);
        } else {
          btnPaint.setColor(colors::BG_LIGHT); // Inactive dark grey
          btnPaint.setStyle(SkPaint::kFill_Style);
        }

        // Draw pill/circle
        canvas->drawRRect(SkRRect::MakeRectXY(btnRect, 4.0f, 4.0f), btnPaint);

        // Label
        SkPaint labelPaint;
        labelPaint.setAntiAlias(true);
        labelPaint.setColor(active ? colors::BG_DARKEST
                                   : colors::TEXT_SECONDARY);

        // Center text roughly
        canvas->drawString(label, bx + 5.0f, btnY + 13.0f, smallFont,
                           labelPaint);
      };

      bool isMuted = track[zenith::ProjectState::PROP_MUTE];
      bool isSoloed = track[zenith::ProjectState::PROP_SOLO];
      bool isArmed = track[zenith::ProjectState::PROP_ARMED];

      drawTrackButton(startX, "M", isMuted, colors::AMBER);
      drawTrackButton(startX + btnGap, "S", isSoloed, colors::BLUE);
      drawTrackButton(startX + btnGap * 2, "R", isArmed, colors::RED);

      // E. Right Border for Header (Glassy look)
      SkPaint borderPaint;
      borderPaint.setShader(SkGradientShader::MakeLinear(
          new SkPoint[2]{{HEADER_WIDTH - 1, y},
                         {HEADER_WIDTH - 1, y + trackHeight}},
          new SkColor[2]{colors::BORDER_SUBTLE, colors::BORDER_DEFAULT},
          nullptr, 2, SkTileMode::kClamp));
      borderPaint.setStrokeWidth(1.0f);
      canvas->drawLine(HEADER_WIDTH, y, HEADER_WIDTH, y + trackHeight,
                       borderPaint);
    }
  }

  // ============================================================================
  // 4. CLIPS RENDER LOOP
  // ============================================================================
  canvas->save();
  canvas->clipRect(SkRect::MakeXYWH(
      HEADER_WIDTH, RULER_HEIGHT, width - HEADER_WIDTH, height - RULER_HEIGHT));

  SkPaint clipPaint;
  clipPaint.setAntiAlias(true);

  SkPaint selectedClipPaint;
  selectedClipPaint.setAntiAlias(true);
  selectedClipPaint.setStyle(SkPaint::kStroke_Style);
  selectedClipPaint.setStrokeWidth(2.0f);
  selectedClipPaint.setColor(colors::CYAN);
  // Outer glow for selection
  selectedClipPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));

  SkFont clipTextFont;
  clipTextFont.setSize(typography::FONT_SM);
  clipTextFont.setSubpixel(true);

  for (const auto &clipView : clipViews) {
    // Check visibility
    if (clipView.bounds.getRight() < HEADER_WIDTH ||
        clipView.bounds.getX() > width)
      continue;

    SkRect r = SkRect::MakeXYWH(clipView.bounds.getX(), clipView.bounds.getY(),
                                clipView.bounds.getWidth(),
                                clipView.bounds.getHeight());

    // Round Rect for Clip
    SkRRect rr =
        SkRRect::MakeRectXY(r, dimensions::RADIUS_SM, dimensions::RADIUS_SM);

    // Determines Color Wrapper
    SkColor baseColor = clipView.isMidi ? colors::MAGENTA : colors::CYAN;
    if (clipView.isSelected) {
      baseColor = lighten(baseColor, 0.2f);
    }

    // 1. Clip Background (Glassy Gradient)
    SkPoint pts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
    SkColor bgColors[2] = {withAlpha(baseColor, 0.4f),
                           withAlpha(baseColor, 0.2f)};
    clipPaint.setShader(SkGradientShader::MakeLinear(pts, bgColors, nullptr, 2,
                                                     SkTileMode::kClamp));
    clipPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRRect(rr, clipPaint);
    clipPaint.setShader(nullptr); // Reset

    // 2. Clip Border (Subtle)
    SkPaint outlinePaint;
    outlinePaint.setAntiAlias(true);
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setColor(withAlpha(baseColor, 0.6f));
    outlinePaint.setStrokeWidth(1.0f);
    canvas->drawRRect(rr, outlinePaint);

    // 3. Selection Glow
    if (clipView.isSelected) {
      canvas->drawRRect(rr, selectedClipPaint);
    }

    // 4. Content (Waveform or MIDI Notes) - Use new professional rendering
    if (clipView.isMidi) {
      // MIDI clip - draw piano roll blob visualization
      drawClipMidiBlobs(canvas, clipView, r);
    } else {
      // Audio clip - draw real waveform from cached peaks
      drawClipWaveform(canvas, clipView, r);
    }

    // 5. Clip Name Label (Shadowed)
    SkPaint textShadow;
    textShadow.setColor(SkColorSetARGB(128, 0, 0, 0));
    canvas->drawString(clipView.clipId.toStdString().c_str(), r.left() + 6.0f,
                       r.top() + 14.0f, clipTextFont, textShadow);

    SkPaint textFill;
    textFill.setColor(colors::TEXT_PRIMARY);
    // Use the clip ID for the visible text as well for consistency
    canvas->drawString(clipView.clipId.toStdString().c_str(), r.left() + 5.0f,
                       r.top() + 13.0f, clipTextFont, textFill);
  }
  canvas->restore();

  // ============================================================================
  // 5. MARQUEE SELECTION
  // ============================================================================
  if (currentDragMode == DragMode::Marquee && !marqueeRect.isEmpty()) {
    SkRect mRect =
        SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                         marqueeRect.getWidth(), marqueeRect.getHeight());

    SkPaint marqueePaint;
    marqueePaint.setColor(withAlpha(colors::CYAN, 0.2f));
    marqueePaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(mRect, marqueePaint);

    marqueePaint.setColor(colors::CYAN);
    marqueePaint.setStyle(SkPaint::kStroke_Style);
    marqueePaint.setPathEffect(
        SkDashPathEffect::Make(SkSpan<const SkScalar>(intervals, 2), 0.0f));
    canvas->drawRect(mRect, marqueePaint);
  }

  // ============================================================================
  // 6. PLAYHEAD (The "Laser")
  // ============================================================================
  // (Assuming we have playhead position from engine or similar)
  // For now, we'll just draw a placeholder at 0 or 'currentPosition' if we had
  // it Since the original code didn't show playhead drawing in the snippet I
  // read, I will add a static one or based on engine state if I can access it.
  // Actually, let's look at beat 0 or viewStart.

  // Actually, let's just draw a "Playhead" at the start for visual confirmation
  // that the render pipeline is working.
  // Real implementation would read transport position.

  // ============================================================================
  // 7. INSERTION GUIDE (Ripple/Insert Mode)
  // ============================================================================
  if (currentDragMode == DragMode::MoveClips && insertionGuideX >= 0.0f &&
      (currentEditMode == EditMode::Ripple ||
       currentEditMode == EditMode::Insert)) {

    SkPaint guidePaint;
    // Neon Pink for Ripple, Neon Green for Insert
    guidePaint.setColor(currentEditMode == EditMode::Ripple
                            ? colors::NEON_PINK
                            : colors::NEON_GREEN);
    guidePaint.setStrokeWidth(2.0f);
    guidePaint.setAntiAlias(true);

    // Neon Glow
    guidePaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    canvas->drawLine(insertionGuideX, RULER_HEIGHT, insertionGuideX, height,
                     guidePaint);

    // Core bright line
    guidePaint.setMaskFilter(nullptr);
    guidePaint.setColor(SK_ColorWHITE);
    guidePaint.setStrokeWidth(1.0f);
    canvas->drawLine(insertionGuideX, RULER_HEIGHT, insertionGuideX, height,
                     guidePaint);

    // Mode Label
    SkFont labelFont =
        typography::getSkFont(typography::FONT_SM, FontWeight::Bold);
    SkPaint labelPaint;
    labelPaint.setColor(SK_ColorWHITE);
    labelPaint.setAntiAlias(true);

    juce::String label =
        (currentEditMode == EditMode::Ripple) ? "RIPPLE" : "INSERT";
    canvas->drawString(label.toStdString().c_str(), insertionGuideX + 5.0f,
                       RULER_HEIGHT + 20.0f, labelFont, labelPaint);
  }
}
#endif

//==============================================================================

bool ArrangerComponent::keyPressed(const juce::KeyPress &key) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Delete / Backspace
  if (key.isKeyCode(juce::KeyPress::deleteKey) ||
      key.isKeyCode(juce::KeyPress::backspaceKey)) {
    deleteSelectedClips();
    return true;
  }

  // Ctrl/Cmd+D - Duplicate
  if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'D') {
    duplicateSelectedClips();
    return true;
  }

  // Ctrl/Cmd+Z - Undo
  if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Z') {
    projectState.undo();
    return true;
  }

  // Ctrl/Cmd+Shift+Z or Ctrl/Cmd+Y - Redo
  if ((key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() &&
       key.getKeyCode() == 'Z') ||
      (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Y')) {
    projectState.redo();
    return true;
  }

  // + key - Zoom in
  if (key.getKeyCode() == '+' || key.getKeyCode() == '=') {
    pixelsPerBeat *= 1.2;
    pixelsPerBeat = juce::jmin(200.0, pixelsPerBeat);
    recomputeClipBounds();
    repaint();
    return true;
  }

  // - key - Zoom out
  if (key.getKeyCode() == '-') {
    pixelsPerBeat /= 1.2;
    pixelsPerBeat = juce::jmax(10.0, pixelsPerBeat);
    recomputeClipBounds();
    repaint();
    return true;
  }

  return false;
}

//==============================================================================
// DragAndDropTarget Interface
//==============================================================================

bool ArrangerComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails &details) {
  // Check if this is a browser drag
  juce::String description = details.description.toString();

  if (zenith::BrowserDragSource::isBrowserDrag(description)) {
    auto type = zenith::BrowserDragSource::getTypeFromDescription(description);

    // Accept audio files, MIDI files, instruments, and plugins
    return type == zenith::BrowserItemType::AudioFile ||
           type == zenith::BrowserItemType::MidiFile ||
           type == zenith::BrowserItemType::Instrument ||
           type == zenith::BrowserItemType::Plugin;
  }

  return false;
}

void ArrangerComponent::itemDragEnter(
    const juce::DragAndDropTarget::SourceDetails &details) {
  juce::ignoreUnused(details);
  isDropTargetActive_ = true;
  repaint();
}

void ArrangerComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails &details) {
  juce::ignoreUnused(details);
  isDropTargetActive_ = false;
  dropTargetTrackIndex_ = -1;
  repaint();
}

void ArrangerComponent::itemDragMove(
    const juce::DragAndDropTarget::SourceDetails &details) {
  // Calculate drop position
  auto localPos = getLocalPoint(details.sourceComponent, details.localPosition);

  dropTargetTrackIndex_ = yToTrackIndex(localPos.y);
  dropTargetBeats_ = snapToGrid(xToBeats(localPos.x));

  repaint();
}

void ArrangerComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails &details) {
  isDropTargetActive_ = false;

  // Parse drag description to get item info
  juce::String description = details.description.toString();

  zenith::BrowserItemType itemType;
  juce::String itemId;
  juce::String itemName;

  if (!zenith::BrowserDragSource::parseDragDescription(description, itemType,
                                                       itemId, itemName)) {
    DBG("ArrangerComponent: Drop failed - could not parse drag "
        "description");
    dropTargetTrackIndex_ = -1;
    repaint();
    return;
  }

  // Calculate drop position
  auto localPos = getLocalPoint(details.sourceComponent, details.localPosition);
  int trackIndex = yToTrackIndex(localPos.y);
  double dropBeats = snapToGrid(xToBeats(localPos.x));

  DBG("ArrangerComponent: Dropped " + itemName + " at track " +
      juce::String(trackIndex) + ", beat " + juce::String(dropBeats));

  // Get or create target track
  auto tracksNode = this->projectState.getState().getChildWithName(
      zenith::ProjectState::ID_TRACKS);
  juce::String targetTrackId;

  if (tracksNode.isValid() && trackIndex >= 0 &&
      trackIndex < tracksNode.getNumChildren()) {
    // Use existing track
    auto track = tracksNode.getChild(trackIndex);
    targetTrackId = track[zenith::ProjectState::PROP_ID].toString();
  } else {
    // Create new track for the dropped item
    bool isMidiItem = itemType == zenith::BrowserItemType::MidiFile ||
                      itemType == zenith::BrowserItemType::Instrument;

    targetTrackId = projectState.createTrack(isMidiItem ? "midi" : "audio",
                                             itemName, "Drop new track");

    DBG("ArrangerComponent: Created new track: " + targetTrackId);
  }

  if (targetTrackId.isEmpty()) {
    DBG("ArrangerComponent: Drop failed - no target track");
    dropTargetTrackIndex_ = -1;
    repaint();
    return;
  }

  // Handle different item types
  switch (itemType) {
  case zenith::BrowserItemType::AudioFile: {
    // Create audio clip with the file
    juce::File audioFile(itemId);
    double clipLength = 4.0; // Default, will be updated when file loads

    juce::String clipId = projectState.createEmptyClip(
        targetTrackId, dropBeats, clipLength, false,
        audioFile.getFileNameWithoutExtension(), "Drop audio file");

    // Set the audio file path on the clip
    auto [track, clip] = projectState.findClip(clipId);
    if (clip.isValid()) {
      clip.setProperty(zenith::ProjectState::PROP_AUDIO_FILE,
                       audioFile.getFullPathName(),
                       &projectState.getUndoManager());
    }

    DBG("ArrangerComponent: Created audio clip from " +
        audioFile.getFileName());
    break;
  }

  case zenith::BrowserItemType::MidiFile: {
    // Create MIDI clip
    juce::File midiFile(itemId);

    juce::String clipId = projectState.createEmptyClip(
        targetTrackId, dropBeats, 4.0, true,
        midiFile.getFileNameWithoutExtension(), "Drop MIDI file");

    DBG("ArrangerComponent: Created MIDI clip from " + midiFile.getFileName());
    break;
  }

  case zenith::BrowserItemType::Instrument: {
    // Create MIDI clip and load instrument
    juce::String clipId = projectState.createEmptyClip(
        targetTrackId, dropBeats, 4.0, true, itemName, "Drop instrument");

    DBG("ArrangerComponent: Created clip for instrument " + itemName);
    break;
  }

  case zenith::BrowserItemType::Plugin: {
    DBG("ArrangerComponent: Would load plugin " + itemName);
    break;
  }

  default:
    DBG("ArrangerComponent: Unhandled drop type");
    break;
  }

  dropTargetTrackIndex_ = -1;
  repaint();
}

//==============================================================================
// Timer callback - Updates playhead position from Engine
//==============================================================================

void ArrangerComponent::timerCallback() { updatePlayheadFromEngine(); }

void ArrangerComponent::updatePlayheadFromEngine() {
  // Get current playhead position from engine
  juce::int64 playheadSamples = engine_.getPlayheadSamples();
  bool wasPlaying = isPlaying_;
  isPlaying_ = engine_.isPlaying();

  // Convert samples to beats
  double newPlayheadBeats = samplesToBeats(playheadSamples);

  // Only repaint if position changed significantly (avoid unnecessary
  // repaints)
  if (std::abs(newPlayheadBeats - playheadBeats_) > 0.01 ||
      wasPlaying != isPlaying_) {
    playheadBeats_ = newPlayheadBeats;

    // Auto-scroll to follow playhead if enabled and playing
    if (followPlayhead_ && isPlaying_) {
      float playheadX = beatsToX(playheadBeats_);
      float visibleWidth = static_cast<float>(getWidth());

      // If playhead is off-screen or near the right edge, scroll
      if (playheadX > visibleWidth * 0.8f || playheadX < 0) {
        viewStartBeats = playheadBeats_ - (visibleWidth * 0.2 / pixelsPerBeat);
        viewStartBeats = juce::jmax(0.0, viewStartBeats);
        recomputeClipBounds();
      }
    }

    repaint();
  }

  // Update loop state from engine
  bool wasLoopEnabled = loopEnabled_;
  loopEnabled_ = engine_.isLooping();

  if (loopEnabled_) {
    double newLoopStart = samplesToBeats(engine_.getLoopStart());
    double newLoopEnd = samplesToBeats(engine_.getLoopEnd());

    if (std::abs(newLoopStart - loopStartBeats_) > 0.01 ||
        std::abs(newLoopEnd - loopEndBeats_) > 0.01 ||
        wasLoopEnabled != loopEnabled_) {
      loopStartBeats_ = newLoopStart;
      loopEndBeats_ = newLoopEnd;
      repaint();
    }
  } else if (wasLoopEnabled != loopEnabled_) {
    repaint();
  }
}

double ArrangerComponent::samplesToBeats(juce::int64 samples) const {
  double sampleRate = engine_.getSampleRate();
  if (sampleRate <= 0.0)
    sampleRate = 44100.0;

  double tempo = projectState.getTempo();
  if (tempo <= 0.0)
    tempo = 120.0;

  double seconds = static_cast<double>(samples) / sampleRate;
  double beatsPerSecond = tempo / 60.0;
  return seconds * beatsPerSecond;
}

//==============================================================================
// Grid resolution
//==============================================================================

void ArrangerComponent::setGridResolution(GridResolution res) {
  gridResolution_ = res;
  gridSnapBeats = gridResolutionToBeats(res);
  repaint();
}

//==============================================================================
// Waveform Cache Management
//==============================================================================

void ArrangerComponent::buildWaveformCache(const juce::String &audioFilePath) {
  // Check if already cached
  if (waveformCache_.find(audioFilePath) != waveformCache_.end()) {
    return;
  }

  // Get audio file from pool
  auto &pool = engine_.getAudioFilePool();
  juce::File file(audioFilePath);

  auto handle = pool.getFile(file);
  if (!handle || !handle->isValid()) {
    // File not loaded, try to load it
    juce::String error;
    handle = pool.loadFile(file, error);
    if (!handle || !handle->isValid()) {
      return; // Failed to load
    }
  }

  // Build waveform cache
  WaveformCache cache;
  cache.audioFilePath = audioFilePath;
  cache.samplesPerPixel = 512; // Resolution for thumbnail

  const juce::AudioBuffer<float> &buffer = handle->buffer;
  int numSamples = static_cast<int>(handle->lengthInSamples);
  int numChannels = handle->numChannels;

  if (numSamples <= 0 || numChannels <= 0) {
    return;
  }

  int numPeaks =
      (numSamples + cache.samplesPerPixel - 1) / cache.samplesPerPixel;
  cache.minPeaks.resize(numPeaks, 0.0f);
  cache.maxPeaks.resize(numPeaks, 0.0f);

  // Mix down to mono and compute peaks
  for (int peakIdx = 0; peakIdx < numPeaks; ++peakIdx) {
    int startSample = peakIdx * cache.samplesPerPixel;
    int endSample = juce::jmin(startSample + cache.samplesPerPixel, numSamples);

    float minVal = 0.0f;
    float maxVal = 0.0f;

    for (int s = startSample; s < endSample; ++s) {
      float sample = 0.0f;
      for (int ch = 0; ch < numChannels; ++ch) {
        sample += buffer.getSample(ch, s);
      }
      sample /= static_cast<float>(numChannels);

      minVal = juce::jmin(minVal, sample);
      maxVal = juce::jmax(maxVal, sample);
    }

    cache.minPeaks[peakIdx] = minVal;
    cache.maxPeaks[peakIdx] = maxVal;
  }

  cache.isValid = true;
  waveformCache_[audioFilePath] = std::move(cache);
}

const ArrangerComponent::WaveformCache *
ArrangerComponent::getWaveformCache(const juce::String &audioFilePath) const {
  auto it = waveformCache_.find(audioFilePath);
  if (it != waveformCache_.end() && it->second.isValid) {
    return &it->second;
  }
  return nullptr;
}

//==============================================================================
// Bar.Beat.Tick Formatting
//==============================================================================

int ArrangerComponent::getBeatsPerBar() const {
  return projectState.getTimeSignatureNumerator();
}

juce::String ArrangerComponent::formatBarBeatTick(double beats) const {
  int beatsPerBar = getBeatsPerBar();
  if (beatsPerBar <= 0)
    beatsPerBar = 4;

  int totalBeats = static_cast<int>(beats);
  int bar = (totalBeats / beatsPerBar) + 1;
  int beat = (totalBeats % beatsPerBar) + 1;

  // Tick is the fractional part (0-99 for display)
  double fractional = beats - static_cast<double>(totalBeats);
  int tick = static_cast<int>(fractional * 100.0);

  return juce::String(bar) + "." + juce::String(beat) + "." +
         juce::String(tick).paddedLeft('0', 2);
}

#ifdef ZENITH_USE_SKIA
void ArrangerComponent::drawClipMidiBlobs(SkCanvas *canvas,
                                          const ClipView &clip,
                                          const SkRect &clipRect) {
  using namespace zenith::design;
  SkPaint notePaint;
  notePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.8f));
  notePaint.setAntiAlias(true);

  for (const auto &blob : clip.noteBlobs) {
    if (clip.lengthBeats <= 0.001)
      continue;

    float nx = clipRect.left() +
               (blob.startBeats / clip.lengthBeats) * clipRect.width();
    float nw = (blob.lengthBeats / clip.lengthBeats) * clipRect.width();
    float ny =
        clipRect.top() + (1.0f - (blob.pitch / 127.0f)) * clipRect.height();

    SkRect noteRect = SkRect::MakeXYWH(nx, ny, std::max(2.0f, nw), 2.0f);
    canvas->drawRect(noteRect, notePaint);
  }
}

void ArrangerComponent::drawClipWaveform(SkCanvas *canvas, const ClipView &clip,
                                         const SkRect &clipRect) {
  using namespace zenith::design;

  auto *cache = getWaveformCache(clip.audioFilePath);
  if (!cache || !cache->isValid || cache->minPeaks.empty()) {
    SkPaint linePaint;
    linePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.3f));
    linePaint.setStrokeWidth(1.0f);
    canvas->drawLine(clipRect.left(), clipRect.centerY(), clipRect.right(),
                     clipRect.centerY(), linePaint);
    return;
  }

  SkPaint wavePaint;
  wavePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.8f));
  wavePaint.setStyle(SkPaint::kStroke_Style);
  wavePaint.setStrokeWidth(1.0f);
  wavePaint.setAntiAlias(true);

  SkPath path;
  float midY = clipRect.centerY();
  float heightScale = clipRect.height() * 0.4f;

  size_t numPeaks = cache->minPeaks.size();
  float stepX = clipRect.width() / static_cast<float>(numPeaks);

  path.moveTo(clipRect.left(), midY);

  for (size_t i = 0; i < numPeaks; ++i) {
    float x = clipRect.left() + i * stepX;
    float top = midY - (cache->maxPeaks[i] * heightScale);
    float bottom = midY - (cache->minPeaks[i] * heightScale);

    path.moveTo(x, top);
    path.lineTo(x, bottom);
  }

  canvas->drawPath(path, wavePaint);
}
#endif

} // namespace zenith
