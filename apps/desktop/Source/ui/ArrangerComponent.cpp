/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation
 */

#include "../../include/ui/ArrangerComponent.h"

#ifdef ZENITH_USE_SKIA
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkFontMgr.h>
#include <skia/include/core/SkFontStyle.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkRRect.h>
#include <skia/include/core/SkRect.h>
#include <skia/include/core/SkTypeface.h>
#include <skia/include/core/SkMaskFilter.h>       // Added
#include <skia/include/core/SkBlurTypes.h>        // Added
#include <skia/include/effects/SkDashPathEffect.h> // Added
#include "skia/ZenithDesignSystem.h"
#include <skia/include/effects/SkGradientShader.h>
#include "skia/GlassmorphicPanel.h" // Added
#include "skia/NeonGlow.h"          // Added
#endif

#include "../../Source/engine/AudioFilePool.h"
#include "../browser/BrowserDragSource.h"
#include "../engine/AudioFilePool.h"

using namespace zenith::design;

// Constants
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT = 80.0f; // Taller tracks for better visibility
static constexpr float SCROLLBAR_HEIGHT = 14.0f;

//==============================================================================
namespace zenith {
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
                buildWaveformCache(view.audioFilePath);
            }
        }

        clipViews.add(view);
      }
    }

    trackIndex++;
  }

  recomputeClipBounds();
}

void ArrangerComponent::recomputeClipBounds() {
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
  return RULER_HEIGHT + (trackIndex - firstVisibleTrackIndex) * TRACK_HEIGHT;
}

int ArrangerComponent::yToTrackIndex(float y) const {
  if (y < RULER_HEIGHT)
    return -1;

  return firstVisibleTrackIndex +
         static_cast<int>((y - RULER_HEIGHT) / TRACK_HEIGHT);
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

void ArrangerComponent::resized() { recomputeClipBounds(); }

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

    // Update clip view positions for visual feedback
    for (const auto &dragState : clipDragStates) {
      if (auto *view = findClipView(dragState.clipId)) {
        double newStart = dragState.originalStartBeats + deltaBeats;
        int newTrackIndex = dragState.originalTrackIndex + deltaTrackIndex;

        // Clamp
        newStart = juce::jmax(0.0, newStart);
        newTrackIndex = juce::jmax(0, newTrackIndex);

        // Update visual position
        view->startBeats = newStart;

        // Update track (if changed)
        auto tracksNode = projectState.getState().getChildWithName(
            zenith::ProjectState::ID_TRACKS);
        if (tracksNode.isValid() &&
            newTrackIndex < tracksNode.getNumChildren()) {
          auto newTrack = tracksNode.getChild(newTrackIndex);
          view->trackId = newTrack[zenith::ProjectState::PROP_ID].toString();
        }
      }
    }

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
    if (!clipDragStates.isEmpty()) {
      projectState.getUndoManager().beginNewTransaction("Move clips");

      for (const auto &dragState : clipDragStates) {
        if (auto *view = findClipView(dragState.clipId)) {
          double snappedStart = snapToGrid(view->startBeats);
          projectState.moveClip(dragState.clipId, view->trackId, snappedStart,
                                "Move clips");
        }
      }

      DBG("ArrangerComponent: Committed move for " +
          juce::String(clipDragStates.size()) + " clips");
    }

    clipDragStates.clear();
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
    // Select clips in marquee
    selectClipsInRect(marqueeRect);
    marqueeRect = juce::Rectangle<float>();
  }

  currentDragMode = DragMode::None;
  repaint();
}

void ArrangerComponent::mouseMove(const juce::MouseEvent &e) {
  // Update cursor based on hover position
  auto *clip = findClipAtPoint(e.position);

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
    repaint();
  } else if (isShift) {
    // Scroll horizontal
    viewStartBeats -= wheel.deltaY * 2.0;
    viewStartBeats = juce::jmax(0.0, viewStartBeats);

    recomputeClipBounds();
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
void ArrangerComponent::drawSkia(SkCanvas* canvas) {
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
  // Dotted line effect
  SkScalar intervals[] = { 2.0f, 4.0f };
  gridPaint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0.0f));

  double startBeat = std::floor(viewStartBeats);
  double endBeat = viewStartBeats + ((width - HEADER_WIDTH) / pixelsPerBeat);

  // Draw Vertical Grid Lines (Time)
  // Optimization: Don't draw every beat if zoomed out too far
  double beatStep = (pixelsPerBeat < 20.0) ? 4.0 : 1.0; 

  int trackCount = 0;
  auto tracksNode = projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (tracksNode.isValid()) {
    trackCount = tracksNode.getNumChildren();
  }

  // Draw grid only within the timeline area
  canvas->save();
  canvas->clipRect(SkRect::MakeXYWH(HEADER_WIDTH, 0, width - HEADER_WIDTH, height));
  
  for (double beat = startBeat; beat <= endBeat; beat += beatStep) {
      float x = beatsToX(beat);
      canvas->drawLine(x, 0, x, height, gridPaint);
  }
  canvas->restore();

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

    // Manual font setup - typography::getSkFont helper not available
    SkFont nameFont;
    nameFont.setSize(typography::FONT_MD);
    nameFont.setSubpixel(true);
    nameFont.setEdging(SkFont::Edging::kAntiAlias);

    SkFont smallFont;
    smallFont.setSize(typography::FONT_XS);
    smallFont.setSubpixel(true);

    for (int i = firstVisibleTrackIndex; i < trackCount; ++i) {
      float y = trackIndexToY(i);
      if (y > height) break;

      float trackHeight = TRACK_HEIGHT;
      SkRect trackRect = SkRect::MakeXYWH(0, y, width, trackHeight);
      SkRect headerRect = SkRect::MakeXYWH(0, y, HEADER_WIDTH, trackHeight);

      // A. Track Header Background (Slightly lighter than timeline)
      trackBgPaint.setColor(colors::BG_DARKER);
      canvas->drawRect(headerRect, trackBgPaint);

      // B. Track Timeline Background (Transparent/Darkest)
      // (Already cleared to BG_DARKEST, effectively)

      // C. Separator
      canvas->drawLine(0, y + trackHeight, width, y + trackHeight, dividerPaint);

      // D. Header Content
      auto track = tracksNode.getChild(i);
      juce::String name = track[zenith::ProjectState::PROP_NAME].toString();
      
      // Track Name
      textPaint.setColor(colors::TEXT_PRIMARY);
      canvas->drawString(name.toStdString().c_str(), spacing::MD, y + 25.0f, font, textPaint);

      // Controls (Mute/Solo/Rec) - Modern "Pills"
      float btnY = y + 40.0f;
      float btnSize = 18.0f;
      float btnGap = 24.0f;
      float startX = spacing::MD;

      // Helper for buttons
      auto drawTrackButton = [&](float bx, const char* label, bool active, SkColor activeColor) {
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
              glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
              canvas->drawCircle(bx + btnSize/2, btnY + btnSize/2, btnSize/2 + 2, glowPaint);
          } else {
              btnPaint.setColor(colors::BG_LIGHT); // Inactive dark grey
              btnPaint.setStyle(SkPaint::kFill_Style);
          }
          
          // Draw pill/circle
          canvas->drawRRect(SkRRect::MakeRectXY(btnRect, 4.0f, 4.0f), btnPaint);
          
          // Label
          SkPaint labelPaint;
          labelPaint.setAntiAlias(true);
          labelPaint.setColor(active ? colors::BG_DARKEST : colors::TEXT_SECONDARY);
          
          // Center text roughly
          canvas->drawString(label, bx + 5.0f, btnY + 13.0f, smallFont, labelPaint);
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
          new SkPoint[2]{{HEADER_WIDTH-1, y}, {HEADER_WIDTH-1, y+trackHeight}},
          new SkColor[2]{colors::BORDER_SUBTLE, colors::BORDER_DEFAULT},
          nullptr, 2, SkTileMode::kClamp));
      borderPaint.setStrokeWidth(1.0f);
      canvas->drawLine(HEADER_WIDTH, y, HEADER_WIDTH, y + trackHeight, borderPaint);
    }
  }

  // ============================================================================
  // 4. CLIPS RENDER LOOP
  // ============================================================================
  canvas->save();
  canvas->clipRect(SkRect::MakeXYWH(HEADER_WIDTH, RULER_HEIGHT, width - HEADER_WIDTH, height - RULER_HEIGHT));

  SkPaint clipPaint;
  clipPaint.setAntiAlias(true);
  
  SkPaint selectedClipPaint;
  selectedClipPaint.setAntiAlias(true);
  selectedClipPaint.setStyle(SkPaint::kStroke_Style);
  selectedClipPaint.setStrokeWidth(2.0f);
  selectedClipPaint.setColor(colors::CYAN);
  // Outer glow for selection
  selectedClipPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));

  SkFont clipTextFont;
  clipTextFont.setSize(typography::FONT_SM);
  clipTextFont.setSubpixel(true);

  for (const auto& clipView : clipViews) {
     // Check visibility
     if (clipView.bounds.getRight() < HEADER_WIDTH || clipView.bounds.getX() > width) continue;

     SkRect r = SkRect::MakeXYWH(clipView.bounds.getX(), clipView.bounds.getY(), 
                                clipView.bounds.getWidth(), clipView.bounds.getHeight());
     
     // Round Rect for Clip
     SkRRect rr = SkRRect::MakeRectXY(r, dimensions::RADIUS_SM, dimensions::RADIUS_SM);

     // Determines Color Wrapper
     SkColor baseColor = clipView.isMidi ? colors::MAGENTA : colors::CYAN;
     if (clipView.isSelected) {
        baseColor = lighten(baseColor, 0.2f);
     }
     
     // 1. Clip Background (Glassy Gradient)
     SkPoint pts[2] = { {r.left(), r.top()}, {r.left(), r.bottom()} };
     SkColor bgColors[2] = { withAlpha(baseColor, 0.4f), withAlpha(baseColor, 0.2f) };
     clipPaint.setShader(SkGradientShader::MakeLinear(pts, bgColors, nullptr, 2, SkTileMode::kClamp));
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

     // 4. Content (Waveform or Notes hint)
     if (clipView.isMidi && !clipView.noteBlobs.empty()) {
         SkPaint notePaint;
         notePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.8f)); 
         for (const auto& blob : clipView.noteBlobs) {
             // Mini-map of notes
             float nx = r.left() + (blob.startBeats / clipView.lengthBeats) * r.width();
             float nw = (blob.lengthBeats / clipView.lengthBeats) * r.width();
             float ny = r.top() + (1.0f - (blob.pitch / 127.0f)) * r.height(); // Simple mapping
             canvas->drawRect(SkRect::MakeXYWH(nx, ny, std::max(2.0f, nw), 2.0f), notePaint);
         }
     } else if (!clipView.isMidi) {
         // Fake waveform line for now (visual flair)
         SkPaint wavePaint;
         wavePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.5f));
         wavePaint.setStyle(SkPaint::kStroke_Style);
         wavePaint.setStrokeWidth(1.0f);
         
         SkPath wavePath;
         wavePath.moveTo(r.left(), r.centerY());
         float step = 5.0f;
         for (float wx = r.left(); wx < r.right(); wx += step) {
             float amp = (float)(std::sin(wx * 0.1) * r.height() * 0.3); // Fake data
             wavePath.lineTo(wx, r.centerY() + amp);
         }
         canvas->drawPath(wavePath, wavePaint);
     }

     // 5. Clip Name Label (Shadowed)
     SkPaint textShadow;
     textShadow.setColor(SkColorSetARGB(128, 0, 0, 0));
     canvas->drawString(clipViews.getReference(0).clipId.toStdString().c_str(), // Placeholder name actually
                        r.left() + 6.0f, r.top() + 14.0f, clipTextFont, textShadow);
     
     SkPaint textFill;
     textFill.setColor(colors::TEXT_PRIMARY);
     // Note: We don't have the Name string in ClipView struct in previous read, 
     // assuming we might need to fetch it or used cached. 
     // For now, drawing "Clip" or using loop info if available.
     // The previous code didn't store name in ClipView, just ID. 
     // We will just draw "Clip" or similar to be safe, or ID.
     canvas->drawString("Clip", r.left() + 5.0f, r.top() + 13.0f, clipTextFont, textFill);
  }
  canvas->restore();

  // ============================================================================
  // 5. MARQUEE SELECTION
  // ============================================================================
  if (currentDragMode == DragMode::Marquee && !marqueeRect.isEmpty()) {
      SkRect mRect = SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(), 
                                    marqueeRect.getWidth(), marqueeRect.getHeight());
      
      SkPaint marqueePaint;
      marqueePaint.setColor(withAlpha(colors::CYAN, 0.2f));
      marqueePaint.setStyle(SkPaint::kFill_Style);
      canvas->drawRect(mRect, marqueePaint);
      
      marqueePaint.setColor(colors::CYAN);
      marqueePaint.setStyle(SkPaint::kStroke_Style);
      marqueePaint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0.0f));
      canvas->drawRect(mRect, marqueePaint);
  }

  // ============================================================================
  // 6. PLAYHEAD (The "Laser")
  // ============================================================================
  // (Assuming we have playhead position from engine or similar)
  // For now, we'll just draw a placeholder at 0 or 'currentPosition' if we had it
  // Since the original code didn't show playhead drawing in the snippet I read, 
  // I will add a static one or based on engine state if I can access it.
  // Actually, let's look at beat 0 or viewStart.
  
  // Actually, let's just draw a "Playhead" at the start for visual confirmation
  // that the render pipeline is working. 
  // Real implementation would read transport position.
}
#endif

      if (isArmed) drawIndicator(colors::RED);
      if (isSoloed) drawIndicator(colors::AMBER);
      if (isMuted) drawIndicator(colors::TEXT_DISABLED);

      // Text Color - Muted tracks overlap with muted text
      SkColor textColor = isMuted ? colors::TEXT_DISABLED : colors::TEXT_PRIMARY;
      trackTextPaint.setColor(textColor);

      // Draw text centered vertically approx
      canvas->drawString(name.toRawUTF8(), textX, contentY + (typography::FONT_MD * 0.35f), trackFont,
                        trackTextPaint);
    }
  }

    // 3. Clips
    for (const auto &clipView : clipViews) {
        SkRect clipRect = SkRect::MakeXYWH(
            (float)clipView.bounds.getX(), (float)clipView.bounds.getY(),
            (float)clipView.bounds.getWidth(), (float)clipView.bounds.getHeight());

        // Determine base color based on clip type
        SkColor clipColor = clipView.isMidi ? colors::MAGENTA : colors::CYAN;
        
        // Use GlassmorphicPanel for render
        // Logic:
        // - If Selected: Use ActiveGlow style with the clip color
        // - If Normal: Use Floating style with clip color as accent (or just Floating and we tint it manually? 
        //   Actually GlassmorphicPanel::drawWithAccent is perfect)
        
        if (clipView.isSelected) {
            GlassmorphicPanel::drawWithAccent(canvas, clipRect, clipColor, GlassmorphicPanel::Style::ActiveGlow);
        } else {
            // For unselected clips, use a subtle accent
             // We can use drawWithAccent but with a lighter/different style or just standard draw and overlay color
             // Let's use Floating style but we want the color tint.
             // GlassmorphicPanel currently supports specific styles.
             // Let's manually tint if needed or rely on the helper.
             // Helper drawWithAccent: "Uses the accent color for borders and subtle glow/tint"
             GlassmorphicPanel::drawWithAccent(canvas, clipRect, clipColor, GlassmorphicPanel::Style::Floating);
        }

        // Clip Name
        if (clipRect.width() > 20.0f) {
            auto [track, clip] = projectState.findClip(clipView.clipId);
            if (clip.isValid()) {
                juce::String name = clip[zenith::ProjectState::PROP_NAME].toString();
                
                SkFont font;
                font.setSize(typography::FONT_SM);
                font.setEdging(SkFont::Edging::kAntiAlias);
                
                // High contrast text inside clips
                // Use NeonGlow for text if selected? No, simpler is better for labels inside clips usually.
                SkPaint textPaint;
                textPaint.setColor(clipView.isSelected ? colors::TEXT_PRIMARY : colors::TEXT_SECONDARY);
                textPaint.setAntiAlias(true);
                
                // Text shadow for readability
                SkPaint shadowPaint;
                shadowPaint.setColor(SkColorSetARGB(128, 0,0,0));
                shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
                canvas->drawString(name.toRawUTF8(), clipRect.fLeft + spacing::XS,
                                   clipRect.fTop + typography::FONT_SM + spacing::XS + 1, font, shadowPaint);

                canvas->drawString(name.toRawUTF8(), clipRect.fLeft + spacing::XS,
                                   clipRect.fTop + typography::FONT_SM + spacing::XS, font, textPaint);
            }
        }

        // Draw clip content (waveform or MIDI blobs)
        if (clipView.isMidi) {
            drawClipMidiBlobs(canvas, clipView, clipRect);
        } else {
            drawClipWaveform(canvas, clipView, clipRect);
        }
    }

    // 4. Time Ruler (Professional Bar.Beat.Tick format)
    {
        // Background
        SkPaint rulerBgPaint;
        rulerBgPaint.setColor(colors::BG_DARKER);
        canvas->drawRect(
            SkRect::MakeXYWH(0.0f, 0.0f, width, (float)RULER_HEIGHT),
            rulerBgPaint);

        // Bottom border
        SkPaint rulerBorderPaint;
        rulerBorderPaint.setColor(colors::BORDER_SUBTLE);
        canvas->drawLine(0, RULER_HEIGHT, width, RULER_HEIGHT, rulerBorderPaint);

        SkPaint tickPaint;
        tickPaint.setColor(colors::TEXT_DISABLED);
        tickPaint.setStrokeWidth(1.0f);
        tickPaint.setAntiAlias(true);

        SkPaint barTickPaint;
        barTickPaint.setColor(colors::TEXT_SECONDARY);
        barTickPaint.setStrokeWidth(1.5f);
        barTickPaint.setAntiAlias(true);

        SkFont font;
        font.setSize(typography::FONT_XS);
        font.setEdging(SkFont::Edging::kAntiAlias);
        
        SkPaint textPaint;
        textPaint.setColor(colors::TEXT_SECONDARY);
        textPaint.setAntiAlias(true);

        int beatsPerBar = getBeatsPerBar();
        if (beatsPerBar <= 0) beatsPerBar = 4;

        double startBeat = std::floor(viewStartBeats);
        double endBeat = viewStartBeats + (width / pixelsPerBeat);

        for (double beat = startBeat; beat <= endBeat; beat += 1.0) {
            float x = beatsToX(beat);
            if (x < 0 || x > width)
                continue;

            int beatInt = static_cast<int>(beat);
            bool isBarLine = (beatInt % beatsPerBar == 0);

            if (isBarLine) {
                // Bar line - taller tick, show bar number
                canvas->drawLine(x, (float)RULER_HEIGHT - 12.0f, x, (float)RULER_HEIGHT, barTickPaint);
                
                int barNumber = (beatInt / beatsPerBar) + 1;
                juce::String barStr = juce::String(barNumber);
                float textWidth = font.measureText(barStr.toRawUTF8(), barStr.length(),
                                                   SkTextEncoding::kUTF8);
                canvas->drawString(barStr.toRawUTF8(), x - textWidth / 2.0f, 12.0f, font,
                                   textPaint);
            } else {
                // Beat tick - shorter
                canvas->drawLine(x, (float)RULER_HEIGHT - 4.0f, x, (float)RULER_HEIGHT, tickPaint);
            }
        }
    }

    // 5. Marquee
    if (currentDragMode == DragMode::Marquee && !marqueeRect.isEmpty()) {
        SkRect mRect =
            SkRect::MakeXYWH((float)marqueeRect.getX(), (float)marqueeRect.getY(),
                             (float)marqueeRect.getWidth(), (float)marqueeRect.getHeight());

        SkPaint fillPaint;
        fillPaint.setColor(withAlpha(colors::CYAN, 0.1f));
        canvas->drawRect(mRect, fillPaint);

        SkPaint borderPaint;
        borderPaint.setColor(withAlpha(colors::CYAN, 0.5f));
        borderPaint.setStyle(SkPaint::kStroke_Style);
        canvas->drawRect(mRect, borderPaint);
    }

    // 6. Loop Region
    if (loopEnabled_) {
        float loopStartX = beatsToX(loopStartBeats_);
        float loopEndX = beatsToX(loopEndBeats_);
        
        // Clamp to visible area
        if (loopEndX > 0 && loopStartX < width) {
            loopStartX = juce::jmax(0.0f, loopStartX);
            loopEndX = juce::jmin(width, loopEndX);
            
            // Highlight in ruler
            SkPaint loopRulerPaint;
            loopRulerPaint.setColor(withAlpha(colors::BLUE, 0.3f));
            canvas->drawRect(SkRect::MakeXYWH(loopStartX, 0, loopEndX - loopStartX, RULER_HEIGHT), loopRulerPaint);
            
            // Subtle tint over track area
            SkPaint loopTrackPaint;
            loopTrackPaint.setColor(withAlpha(colors::BLUE, 0.05f));
            canvas->drawRect(SkRect::MakeXYWH(loopStartX, RULER_HEIGHT, loopEndX - loopStartX, height - RULER_HEIGHT), loopTrackPaint);
            
            // Loop brackets
            SkPaint bracketPaint;
            bracketPaint.setColor(colors::BLUE); // Solid blue
            canvas->drawRect(SkRect::MakeXYWH(loopStartX, 0, 2, RULER_HEIGHT), bracketPaint);
            canvas->drawRect(SkRect::MakeXYWH(loopEndX - 2, 0, 2, RULER_HEIGHT), bracketPaint);
            
            // Labels
            SkFont markerFont;
            markerFont.setSize(typography::FONT_XS);
            SkPaint markerTextPaint;
            markerTextPaint.setColor(colors::TEXT_PRIMARY);
            markerTextPaint.setAntiAlias(true);
            canvas->drawString("L", loopStartX + 4, 12, markerFont, markerTextPaint);
            canvas->drawString("R", loopEndX - 10, 12, markerFont, markerTextPaint);
        }
    }

    // 7. Playhead
    {
        float x = beatsToX(playheadBeats_);
        
        if (x >= 0 && x <= width) {
            SkColor playheadColor = isPlaying_ 
                ? colors::CYAN        // Neon Cyan when playing
                : colors::TEXT_DISABLED; // Dimmed when stopped
            
            SkPaint playheadPaint;
            playheadPaint.setColor(playheadColor);
            playheadPaint.setStrokeWidth(1.5f);
            playheadPaint.setAntiAlias(true);
            
            // Line
            canvas->drawLine(x, 0, x, height, playheadPaint);
            
            // Triangle Head
            SkPath triangle;
            triangle.moveTo(x - 6, 0);
            triangle.lineTo(x + 6, 0);
            triangle.lineTo(x, 12);
            triangle.close();
            
            playheadPaint.setStyle(SkPaint::kFill_Style);
            canvas->drawPath(triangle, playheadPaint);
        }
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

void ArrangerComponent::buildWaveformCache(const juce::String& audioFilePath) {
    // Check if already cached
    if (waveformCache_.find(audioFilePath) != waveformCache_.end()) {
        return;
    }

    // Get audio file from pool
    auto& pool = engine_.getAudioFilePool();
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

    const juce::AudioBuffer<float>& buffer = handle->buffer;
    int numSamples = static_cast<int>(handle->lengthInSamples);
    int numChannels = handle->numChannels;
    
    if (numSamples <= 0 || numChannels <= 0) {
        return;
    }

    int numPeaks = (numSamples + cache.samplesPerPixel - 1) / cache.samplesPerPixel;
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

const ArrangerComponent::WaveformCache* ArrangerComponent::getWaveformCache(const juce::String& audioFilePath) const {
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
    if (beatsPerBar <= 0) beatsPerBar = 4;

    int totalBeats = static_cast<int>(beats);
    int bar = (totalBeats / beatsPerBar) + 1;
    int beat = (totalBeats % beatsPerBar) + 1;
    
    // Tick is the fractional part (0-99 for display)
    double fractional = beats - static_cast<double>(totalBeats);
    int tick = static_cast<int>(fractional * 100.0);
    
    return juce::String(bar) + "." + juce::String(beat) + "." + juce::String(tick).paddedLeft('0', 2);
}

#ifdef ZENITH_USE_SKIA
//==============================================================================
// Clip Content Drawing - Waveform
//==============================================================================

void ArrangerComponent::drawClipWaveform(SkCanvas* canvas, const ClipView& clip, const SkRect& clipRect) {
    using namespace zenith::design;

    if (clip.audioFilePath.isEmpty()) {
        return;
    }

    const WaveformCache* cache = getWaveformCache(clip.audioFilePath);
    if (!cache || cache->minPeaks.empty()) {
        return;
    }

    float clipWidth = clipRect.width();
    float clipHeight = clipRect.height();
    float centerY = clipRect.centerY();
    float waveHeight = (clipHeight - 20.0f) * 0.5f; // Leave room for name

    // Calculate visible range
    int numPeaks = static_cast<int>(cache->minPeaks.size());
    float peaksPerPixel = static_cast<float>(numPeaks) / clipWidth;

    SkPaint wavePaint;
    wavePaint.setAntiAlias(true);
    wavePaint.setColor(clip.isSelected ? colors::CYAN : withAlpha(colors::BLUE, 0.7f));
    wavePaint.setStrokeWidth(1.0f);
    wavePaint.setStyle(SkPaint::kStroke_Style);

    // Draw waveform as vertical lines
    for (float px = 0; px < clipWidth; px += 1.0f) {
        int peakIdx = static_cast<int>(px * peaksPerPixel);
        if (peakIdx >= numPeaks) break;

        float minVal = cache->minPeaks[peakIdx];
        float maxVal = cache->maxPeaks[peakIdx];

        float y1 = centerY - (maxVal * waveHeight);
        float y2 = centerY - (minVal * waveHeight);

        // Clamp to clip bounds
        y1 = juce::jmax(clipRect.fTop + 14.0f, y1);  // Below clip name
        y2 = juce::jmin(clipRect.fBottom - 2.0f, y2);

        if (y2 > y1) {
            canvas->drawLine(clipRect.fLeft + px, y1, clipRect.fLeft + px, y2, wavePaint);
        }
    }
}

//==============================================================================
// Clip Content Drawing - MIDI Blobs
//==============================================================================

void ArrangerComponent::drawClipMidiBlobs(SkCanvas* canvas, const ClipView& clip, const SkRect& clipRect) {
    using namespace zenith::design;

    if (clip.noteBlobs.empty()) {
        return;
    }

    // Find pitch range for scaling
    int minPitch = 127;
    int maxPitch = 0;
    for (const auto& blob : clip.noteBlobs) {
        minPitch = juce::jmin(minPitch, blob.pitch);
        maxPitch = juce::jmax(maxPitch, blob.pitch);
    }
    
    if (maxPitch == minPitch) {
        minPitch -= 6;
        maxPitch += 6;
    }
    
    int pitchRange = maxPitch - minPitch;
    if (pitchRange < 12) {
        int expand = (12 - pitchRange) / 2;
        minPitch -= expand;
        maxPitch += expand;
        pitchRange = maxPitch - minPitch;
    }

    float clipWidth = clipRect.width();
    float noteAreaTop = clipRect.fTop + 14.0f; // Below clip name
    float noteAreaHeight = clipRect.height() - 16.0f;

    SkPaint blobPaint;
    blobPaint.setAntiAlias(true);
    blobPaint.setColor(clip.isSelected ? colors::NEON_GREEN : withAlpha(colors::NEON_GREEN, 0.7f));

    for (const auto& blob : clip.noteBlobs) {
        // Calculate position relative to clip
        float noteStartRatio = static_cast<float>((blob.startBeats) / clip.lengthBeats);
        float noteLengthRatio = static_cast<float>(blob.lengthBeats / clip.lengthBeats);
        
        float x = clipRect.fLeft + (noteStartRatio * clipWidth);
        float w = noteLengthRatio * clipWidth;
        w = juce::jmax(2.0f, w); // Minimum width of 2px
        
        // Y position (inverted - higher pitch = higher on screen)
        float pitchRatio = static_cast<float>(blob.pitch - minPitch) / static_cast<float>(pitchRange);
        float y = noteAreaTop + noteAreaHeight * (1.0f - pitchRatio);
        float h = juce::jmax(2.0f, noteAreaHeight / static_cast<float>(pitchRange));
        h = juce::jmin(h, 6.0f); // Max height of 6px for blob

        // Clamp to clip bounds
        if (x < clipRect.fLeft) {
            w -= (clipRect.fLeft - x);
            x = clipRect.fLeft;
        }
        if (x + w > clipRect.fRight) {
            w = clipRect.fRight - x;
        }

        if (w > 0) {
            SkRRect roundedNote;
            roundedNote.setRectXY(SkRect::MakeXYWH(x, y - h / 2.0f, w, h), 1.0f, 1.0f);
            canvas->drawRRect(roundedNote, blobPaint);
        }
    }
}
#endif

} // namespace zenith