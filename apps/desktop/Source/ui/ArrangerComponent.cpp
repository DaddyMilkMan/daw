/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation
 */

#include "../../include/ui/ArrangerComponent.h"

#ifdef ZENITH_USE_SKIA
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkFontMgr.h>   // Added
#include <skia/include/core/SkFontStyle.h> // Added
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkRect.h>
#include <skia/include/core/SkTypeface.h>
#include <skia/include/effects/SkGradientShader.h> // Added

#endif

#include "../../Source/engine/AudioFilePool.h"
#include "../browser/BrowserDragSource.h"
#include "skia/ZenithDesignSystem.h"

using namespace zenith::design;

// Constants
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT =
    80.0f; // Taller tracks for better visibility
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
void ArrangerComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  float w = (float)bounds.getWidth();
  float h = (float)bounds.getHeight();

  // 1. Background (Darkest)
  canvas->clear(design::colors::BG_DARKEST);

  // 2. Grid & Timeline Area (Clipped to right of Header)
  canvas->save();
  canvas->clipRect(SkRect::MakeXYWH(HEADER_WIDTH, 0, w - HEADER_WIDTH, h));

  auto tracksNode =
      projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (tracksNode.isValid()) {
    int numTracks = tracksNode.getNumChildren();

    // Grid Paints
    SkPaint gridPaintMain;
    gridPaintMain.setColor(
        design::withAlpha(design::colors::BORDER_SUBTLE, 0.1f));
    gridPaintMain.setStrokeWidth(1.0f);

    SkPaint gridPaintBeat;
    gridPaintBeat.setColor(
        design::withAlpha(design::colors::BORDER_SUBTLE, 0.05f));

    // Draw Tracks Backgrounds (Alternating)
    for (int i = firstVisibleTrackIndex; i < numTracks; ++i) {
      float y = trackIndexToY(i);
      if (y > h)
        break;

      SkPaint trackBgPaint;
      trackBgPaint.setColor((i % 2 == 0) ? design::colors::BG_DARKER
                                         : design::colors::BG_DARK);
      canvas->drawRect(
          SkRect::MakeXYWH(HEADER_WIDTH, y, w - HEADER_WIDTH, TRACK_HEIGHT),
          trackBgPaint);

      // Horizontal Divider
      SkPaint dividerPaint;
      dividerPaint.setColor(design::colors::BORDER_SUBTLE);
      canvas->drawLine(HEADER_WIDTH, y + TRACK_HEIGHT, w, y + TRACK_HEIGHT,
                       dividerPaint);
    }

    // Vertical Grid Lines
    double startBeat = std::floor(viewStartBeats);
    double endBeat = viewStartBeats + ((w - HEADER_WIDTH) / pixelsPerBeat);

    for (double beat = startBeat; beat <= endBeat; beat += 1.0) {
      float x = beatsToX(beat);
      // Draw Bar line (assuming 4/4)
      if (std::abs(std::fmod(beat, 4.0)) < 0.001) {
        canvas->drawLine(x, 0, x, h, gridPaintMain);
      } else {
        canvas->drawLine(x, 0, x, h, gridPaintBeat);
      }
    }

    // 3. Clips
    for (const auto &clipView : clipViews) {
      if (clipView.bounds.getRight() < HEADER_WIDTH ||
          clipView.bounds.getX() > w)
        continue;

      SkRect clipRect = SkRect::MakeXYWH((float)clipView.bounds.getX(),
                                         (float)clipView.bounds.getY(),
                                         (float)clipView.bounds.getWidth(),
                                         (float)clipView.bounds.getHeight());

      // Clip Body - Rounded Rect
      SkRRect rrect =
          SkRRect::MakeRectXY(clipRect, design::dimensions::RADIUS_MD,
                              design::dimensions::RADIUS_MD);

      SkPaint clipPaint;
      SkColor baseColor =
          clipView.isMidi ? design::colors::MAGENTA : design::colors::CYAN;

      if (clipView.isSelected) {
        baseColor = design::lighten(baseColor, 0.2f);
      }

      // Gradient
      SkPoint pts[] = {{clipRect.fLeft, clipRect.fTop},
                       {clipRect.fLeft, clipRect.fBottom}};
      SkColor colors[2] = {design::withAlpha(baseColor, 0.3f),
                           design::withAlpha(baseColor, 0.1f)};
      clipPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                       SkTileMode::kClamp));

      canvas->drawRRect(rrect, clipPaint);

      // Border
      SkPaint borderPaint;
      borderPaint.setColor(clipView.isSelected
                               ? design::colors::TEXT_PRIMARY
                               : design::withAlpha(baseColor, 0.5f));
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(clipView.isSelected ? 2.0f : 1.0f);
      borderPaint.setAntiAlias(true);
      canvas->drawRRect(rrect, borderPaint);

      // Clip Content (Waveform or MIDI)
      auto [track, clip] = projectState.findClip(clipView.clipId);
      if (clip.isValid()) {
        // Clip Name
        juce::String name = clip[zenith::ProjectState::PROP_NAME].toString();

        // Name Background Bar
        SkPaint nameBgPaint;
        nameBgPaint.setColor(design::withAlpha(baseColor, 0.8f));
        // Top bar for name
        SkPath headerPath;
        SkRect headerRect = SkRect::MakeXYWH(clipRect.fLeft, clipRect.fTop,
                                             clipRect.width(), 16.0f);
        float radii[] = {design::dimensions::RADIUS_MD,
                         design::dimensions::RADIUS_MD,
                         design::dimensions::RADIUS_MD,
                         design::dimensions::RADIUS_MD,
                         0,
                         0,
                         0,
                         0};
        SkRRect headerRRect;
        headerRRect.setRectRadii(headerRect, (const SkVector *)radii);
        // Actually standard RRect clipped? simpler: just fill top part
        canvas->drawRect(headerRect, nameBgPaint);

        SkFont font;
        font.setSize(10.0f);
        SkPaint textPaint;
        textPaint.setColor(design::colors::BG_DARKEST);
        textPaint.setAntiAlias(true);
        canvas->drawString(name.toRawUTF8(), clipRect.fLeft + 4.0f,
                           clipRect.fTop + 12.0f, font, textPaint);

        if (!clipView.isMidi) {
          // Draw Waveform
          juce::File audioFile(
              clip.getProperty(zenith::ProjectState::PROP_AUDIO_FILE)
                  .toString());
          if (audioFile.existsAsFile()) {
            auto audioHandle = engine_.getAudioFilePool().getFile(audioFile);
            if (audioHandle) {
              SkPaint wavePaint;
              wavePaint.setColor(design::withAlpha(baseColor, 0.8f));
              wavePaint.setStyle(SkPaint::kStroke_Style);
              wavePaint.setStrokeWidth(1.0f);

              SkPath path;
              float midY = clipRect.centerY() + 8; // Offset for header
              float height = clipRect.height() - 16;

              // Very simple decimated waveform
              // Map pixels to samples
              int numSamples = audioHandle->buffer.getNumSamples();
              float samplesPerPixel = (float)numSamples / clipRect.width();

              // Optimization: don't draw every pixel if zoomed out too much
              int step = 2;

              const float *samples =
                  audioHandle->buffer.getReadPointer(0); // Left channel
              bool first = true;

              for (float px = 0; px < clipRect.width(); px += step) {
                int sampleIdx = (int)(px * samplesPerPixel);
                if (sampleIdx < numSamples) {
                  float val = samples[sampleIdx];
                  float y = midY - (val * height * 0.5f);
                  if (first) {
                    path.moveTo(clipRect.fLeft + px, y);
                    first = false;
                  } else {
                    path.lineTo(clipRect.fLeft + px, y);
                  }
                }
              }
              canvas->drawPath(path, wavePaint);
            }
          }
        } else {
          // Draw Mini-Notes from ProjectState
          SkPaint notePaint;
          notePaint.setColor(
              design::withAlpha(design::colors::TEXT_PRIMARY, 0.6f));

          auto notesNode =
              clip.getChildWithName(zenith::ProjectState::ID_NOTES);

          if (notesNode.isValid()) {
            float pxPerBeat = clipRect.width() / (float)clipView.lengthBeats;

            for (int k = 0; k < notesNode.getNumChildren(); ++k) {
              auto note = notesNode.getChild(k);
              int pitch = note[zenith::ProjectState::PROP_PITCH];
              double start = note[zenith::ProjectState::PROP_START];
              double length = note[zenith::ProjectState::PROP_LENGTH];

              float normalizedPitch =
                  1.0f - ((pitch - 21) / 88.0f); // Piano range
              float nx = clipRect.fLeft + (float)(start * pxPerBeat);
              float nw = (float)(length * pxPerBeat);
              float ny = clipRect.fTop + 16 +
                         (normalizedPitch * (clipRect.height() - 20));

              if (nx < clipRect.fRight && nx + nw > clipRect.fLeft) {
                SkRect noteRect = SkRect::MakeXYWH(nx, ny, nw, 3.0f);
                canvas->drawRect(noteRect, notePaint);
              }
            }
          }
        }
      }
    }
  }
  // End Tracks Node check

  // Restore clip (so we can draw headers/ruler over)
  canvas->restore();

  // 4. Track Headers (Fixed Left Panel)
  {
    // Panel Background (Frosted Glass style)
    SkPaint headerBgPaint;
    headerBgPaint.setColor(design::colors::BG_DARK);
    canvas->drawRect(SkRect::MakeXYWH(0, 0, HEADER_WIDTH, h), headerBgPaint);

    // Divider
    SkPaint divPaint;
    divPaint.setColor(design::colors::BORDER_STRONG);
    canvas->drawLine(HEADER_WIDTH, 0, HEADER_WIDTH, h, divPaint);

    if (tracksNode.isValid()) {
      int numTracks = tracksNode.getNumChildren();
      for (int i = firstVisibleTrackIndex; i < numTracks; ++i) {
        float y = trackIndexToY(i);
        if (y > h)
          break;

        auto track = tracksNode.getChild(i);
        juce::String name = track[zenith::ProjectState::PROP_NAME].toString();
        bool isMuted = track[zenith::ProjectState::PROP_MUTE];
        bool isSoloed = track[zenith::ProjectState::PROP_SOLO];
        bool isArmed = track[zenith::ProjectState::PROP_ARMED];

        // Font
        SkFont nameFont;
        nameFont.setSize(14.0f);
        SkPaint textPaint;
        textPaint.setColor(design::colors::TEXT_PRIMARY);
        textPaint.setAntiAlias(true);
        canvas->drawString(name.toRawUTF8(), 10.0f, y + 25.0f, nameFont,
                           textPaint);

        // Controls Row
        float btnY = y + 50.0f;
        float btnSize = 25.0f;

        auto drawButton = [&](float bx, const char *label, bool active,
                              SkColor activeColor) {
          SkPaint btnP;
          btnP.setAntiAlias(true);
          btnP.setColor(active ? activeColor : design::colors::BG_LIGHT);
          canvas->drawCircle(bx, btnY, 12.0f, btnP);

          // Stroke
          SkPaint stroke;
          stroke.setAntiAlias(true);
          stroke.setStyle(SkPaint::kStroke_Style);
          stroke.setColor(active ? design::colors::TEXT_PRIMARY
                                 : design::colors::BORDER_DEFAULT);
          canvas->drawCircle(bx, btnY, 12.0f, stroke);

          // Label
          SkFont lblFont;
          lblFont.setSize(10.0f);
          SkPaint lblP;
          lblP.setColor(active ? design::colors::TEXT_PRIMARY
                               : design::colors::TEXT_SECONDARY);
          lblP.setAntiAlias(true);
          // Center text roughly
          canvas->drawString(label, bx - 4.0f, btnY + 4.0f, lblFont, lblP);
        };

        drawButton(132.0f, "M", isMuted, design::colors::BLUE);
        drawButton(162.0f, "S", isSoloed, design::colors::AMBER);
        drawButton(192.0f, "R", isArmed, design::colors::RED);
      }
    }
  }

  // 5. Ruler (Top)
  {
    SkPaint rulerBg;
    rulerBg.setColor(design::colors::BG_DARKEST);
    canvas->drawRect(
        SkRect::MakeXYWH(HEADER_WIDTH, 0, w - HEADER_WIDTH, RULER_HEIGHT),
        rulerBg);

    // Divider
    SkPaint div;
    div.setColor(design::colors::BORDER_DEFAULT);
    canvas->drawLine(0, RULER_HEIGHT, w, RULER_HEIGHT, div);

    SkFont font;
    font.setSize(10.0f);
    SkPaint textP;
    textP.setColor(design::colors::TEXT_SECONDARY);
    textP.setAntiAlias(true);

    double startBeat = std::floor(viewStartBeats);
    double endBeat = viewStartBeats + ((w - HEADER_WIDTH) / pixelsPerBeat);

    for (double beat = startBeat; beat <= endBeat; beat += 1.0) {
      float x = beatsToX(beat);
      if (x < HEADER_WIDTH || x > w)
        continue;

      // Big tick
      SkPaint tick;
      tick.setColor(design::colors::TEXT_SECONDARY);
      canvas->drawLine(x, 15, x, 30, tick);

      // Number
      juce::String numStr = juce::String(static_cast<int>(beat + 1));
      canvas->drawString(numStr.toRawUTF8(), x + 2, 12, font, textP);
    }

    // Header Top Label
    SkPaint headerTopBg;
    headerTopBg.setColor(design::colors::BG_DARKER);
    canvas->drawRect(SkRect::MakeXYWH(0, 0, HEADER_WIDTH, RULER_HEIGHT),
                     headerTopBg);

    SkFont hFont;
    hFont.setSize(12.0f);
    canvas->drawString("TRACKS", 10, 20, hFont, textP);
  }

  // 6. Playhead
  float phX = beatsToX(playheadBeats_);
  if (phX >= HEADER_WIDTH && phX <= w) {
    SkPaint phPaint;
    phPaint.setColor(design::colors::CYAN);
    phPaint.setStrokeWidth(1.0f);
    phPaint.setAntiAlias(true);

    // Glow
    SkPaint glowPaint = phPaint;
    glowPaint.setStrokeWidth(2.0f);
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur((SkBlurStyle)0, 4.0f));
    canvas->drawLine(phX, 0, phX, h, glowPaint);

    // Main line
    canvas->drawLine(phX, 0, phX, h, phPaint);

    // Head
    SkPath head;
    head.moveTo(phX - 6, 0);
    head.lineTo(phX + 6, 0);
    head.lineTo(phX, 10);
    head.close();

    // Draw head with glow
    canvas->drawPath(head, glowPaint);
    canvas->drawPath(head, phPaint);
  }

  // 7. Marquee
  if (currentDragMode == DragMode::Marquee && !marqueeRect.isEmpty()) {
    SkPaint fill;
    fill.setColor(design::withAlpha(design::colors::CYAN, 0.2f));
    SkRect mRect =
        SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                         marqueeRect.getWidth(), marqueeRect.getHeight());
    canvas->drawRect(mRect, fill);

    SkPaint stroke;
    stroke.setColor(design::colors::CYAN);
    stroke.setStyle(SkPaint::kStroke_Style);
    canvas->drawRect(mRect, stroke);
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

} // namespace zenith