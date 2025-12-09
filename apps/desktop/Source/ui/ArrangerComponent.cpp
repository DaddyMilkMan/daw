/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation
 */

#include "../../include/ui/ArrangerComponent.h"

#ifdef ZENITH_USE_SKIA
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkRect.h>
#include <skia/include/core/SkTypeface.h>
#endif

#include "../browser/BrowserDragSource.h"

//==============================================================================
namespace zenith {
//==============================================================================
ArrangerComponent::ArrangerComponent(zenith::ProjectState& ps, zenith::Engine& eng)
    : projectState(ps), engine(eng)
{
    projectState.getState().addListener(this);
    setWantsKeyboardFocus(true);
}

ArrangerComponent::~ArrangerComponent() {
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
    auto tracksNode =
        projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
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
    float height = static_cast<float>(trackHeight - 4); // 2px margin top/bottom

    clipView.bounds = juce::Rectangle<float>(x, y + 2.0f, width, height);
  }
}

ArrangerComponent::ClipView *ArrangerComponent::findClipView(const juce::String &clipId) {
  for (auto &clipView : clipViews) {
    if (clipView.clipId == clipId)
      return &clipView;
  }
  return nullptr;
}

ArrangerComponent::ClipView *ArrangerComponent::findClipAtPoint(juce::Point<float> point) {
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
  return static_cast<float>((beats - viewStartBeats) * pixelsPerBeat);
}

double ArrangerComponent::xToBeats(float x) const {
  return viewStartBeats + (x / pixelsPerBeat);
}

float ArrangerComponent::trackIndexToY(int trackIndex) const {
  return rulerHeight + (trackIndex - firstVisibleTrackIndex) * trackHeight;
}

int ArrangerComponent::yToTrackIndex(float y) const {
  if (y < rulerHeight)
    return -1;

  return firstVisibleTrackIndex +
         static_cast<int>((y - rulerHeight) / trackHeight);
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
// Component interface - Painting
//==============================================================================

void ArrangerComponent::paint(juce::Graphics &g) {
    // Pure Skia rendering; just fill background to avoid garbage.
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void ArrangerComponent::paintBackground(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff1e1e1e));
}

void ArrangerComponent::paintTimeRuler(juce::Graphics &g) {
  // Color definitions
  const juce::Colour gridLineColour = juce::Colour(0xff505050);

  g.setColour(juce::Colour(0xff2a2a2a));
  g.fillRect((float)0, (float)0, (float)getWidth(), (float)rulerHeight);

  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(12.0f));

  // Draw beat markers
  double startBeat = std::floor(viewStartBeats);
  double endBeat = viewStartBeats + (getWidth() / pixelsPerBeat);

  for (double beat = startBeat; beat <= endBeat; beat += 1.0) {
    float x = beatsToX(beat);
    if (x < 0 || x > getWidth())
      continue;

    // Draw tick
    g.setColour(gridLineColour);
    g.drawLine(x, rulerHeight - 8.0f, x, static_cast<float>(rulerHeight), 1.0f);

    // Draw beat number
    g.setColour(juce::Colours::lightgrey);
    g.drawText(juce::String(static_cast<int>(beat + 1)),
               static_cast<int>(x - 15), 2, 30, rulerHeight - 10,
               juce::Justification::centred, false);
  }
}

void ArrangerComponent::paintTracks(juce::Graphics &g) {
  // Color definitions
  const juce::Colour trackLaneColour = juce::Colour(0xff2a2a2a);
  const juce::Colour trackDividerColour = juce::Colour(0xff3c3c3c);
  const juce::Colour gridLineColour = juce::Colour(0xff505050);

  auto tracksNode =
      projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (!tracksNode.isValid())
    return;

  int numTracks = tracksNode.getNumChildren();

  for (int i = firstVisibleTrackIndex; i < numTracks; ++i) {
    float y = trackIndexToY(i);
    if (y > getHeight())
      break;

    // Alternate track colors
    g.setColour(i % 2 == 0 ? trackLaneColour : trackLaneColour.darker(0.1f));
    g.fillRect(0.0f, y, static_cast<float>(getWidth()),
               static_cast<float>(trackHeight));

    // Track divider
    g.setColour(trackDividerColour);
    g.drawLine(0.0f, y, static_cast<float>(getWidth()), y, 1.0f);

    // Draw vertical grid lines
    double startBeat = std::floor(viewStartBeats);
    double endBeat = viewStartBeats + (getWidth() / pixelsPerBeat);

    g.setColour(gridLineColour.withAlpha(0.3f));
    for (double beat = startBeat; beat <= endBeat; beat += 1.0) {
      float x = beatsToX(beat);
      if (x >= 0 && x <= getWidth())
        g.drawLine(x, y, x, y + trackHeight, 1.0f);
    }

    // Track name and status indicators
    auto track = tracksNode.getChild(i);
    auto trackName = track[zenith::ProjectState::PROP_NAME].toString();
    bool isMuted = track[zenith::ProjectState::PROP_MUTE];
    bool isSoloed = track[zenith::ProjectState::PROP_SOLO];
    bool isArmed = track[zenith::ProjectState::PROP_ARMED];

    int textX = 10;

    // Draw indicators
    if (isArmed)
    {
        g.setColour(juce::Colours::red);
        g.fillEllipse(textX, y + 6, 8, 8);
        textX += 12;
    }
    if (isSoloed)
    {
        g.setColour(juce::Colours::orange);
        g.fillEllipse(textX, y + 6, 8, 8);
        textX += 12;
    }
    if (isMuted)
    {
        g.setColour(juce::Colours::grey);
        g.fillEllipse(textX, y + 6, 8, 8);
        textX += 12;
    }

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    if (isMuted) g.setColour(juce::Colours::grey); // Dim text if muted
    
    g.setFont(juce::FontOptions(14.0f));
    g.drawText(trackName, textX, static_cast<int>(y + 5), 200, 20,
               juce::Justification::centredLeft, false);
  }
}

void ArrangerComponent::paintClips(juce::Graphics &g) {
  // Color definitions
  const juce::Colour midiClipColour = juce::Colour(0xff4a90e2);
  const juce::Colour audioClipColour = juce::Colour(0xffe27a4a);
  const juce::Colour selectedClipColour = juce::Colour(0xffffffff);

  for (const auto &clipView : clipViews) {
    // Clip color
    juce::Colour clipColour =
        clipView.isMidi ? midiClipColour : audioClipColour;

    if (clipView.isSelected) {
      // Draw selection border
      g.setColour(selectedClipColour);
      g.drawRect(clipView.bounds, 2.0f);
      clipColour = clipColour.brighter(0.2f);
    }

    // Draw clip fill
    g.setColour(clipColour);
    g.fillRect(clipView.bounds.reduced(1.0f));

    // Draw clip name
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.setFont(juce::FontOptions(12.0f));

    auto textBounds = clipView.bounds.reduced(4.0f, 2.0f);
    if (textBounds.getWidth() > 20.0f) {
      // Get clip name from ProjectState
      auto [track, clip] = projectState.findClip(clipView.clipId);
      if (clip.isValid()) {
        auto clipName = clip[zenith::ProjectState::PROP_NAME].toString();
        g.drawText(clipName, textBounds.toNearestInt(),
                   juce::Justification::centredLeft, true);
      }
    }
  }
}

void ArrangerComponent::paintMarquee(juce::Graphics &g) {
  if (currentDragMode == DragMode::Marquee && !marqueeRect.isEmpty()) {
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.fillRect(marqueeRect);

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawRect(marqueeRect, 1.0f);
  }
}

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
    // Check for resize zones
    if (clip->isInLeftResizeZone(e.position)) {
      currentDragMode = DragMode::ResizeClipLeft;
      resizingClipId = clip->clipId;
      resizeOriginalStart = clip->startBeats;
      resizeOriginalLength = clip->lengthBeats;
      DBG("ArrangerComponent: Start resize left");
    } else if (clip->isInRightResizeZone(e.position)) {
      currentDragMode = DragMode::ResizeClipRight;
      resizingClipId = clip->clipId;
      resizeOriginalStart = clip->startBeats;
      resizeOriginalLength = clip->lengthBeats;
      DBG("ArrangerComponent: Start resize right");
    } else {
      // Move mode
      currentDragMode = DragMode::MoveClips;

      bool isCtrlOrCmd = e.mods.isCommandDown();

      // Handle selection
      if (!clip->isSelected) {
        selectClip(clip->clipId, isCtrlOrCmd);
      } else if (isCtrlOrCmd) {
        // Ctrl-click on selected clip = deselect
        selectClip(clip->clipId, true);
      }

      // Cache original positions for all selected clips
      clipDragStates.clear();
      for (const auto &clipId : selectedClipIds) {
        if (auto *view = findClipView(clipId)) {
          // Find track index
          int trackIndex = 0;
          auto tracksNode =
              projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
          if (tracksNode.isValid()) {
            for (const auto &track : tracksNode) {
              if (track[zenith::ProjectState::PROP_ID].toString() == view->trackId)
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

      DBG("ArrangerComponent: Start move " +
          juce::String(clipDragStates.size()) + " clips");
    }
  } else {
    // Clicked empty area
    bool isShift = e.mods.isShiftDown();

    if (isShift) {
      // Start marquee selection
      currentDragMode = DragMode::Marquee;
      marqueeRect = juce::Rectangle<float>(e.position, e.position);
    } else {
      // Clear selection
      clearSelection();
    }
  }
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent &e) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

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
        auto tracksNode =
            projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
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
      auto startBeats = clipNode[zenith::ProjectState::PROP_START_BEATS].toString();
      auto lengthBeats = clipNode[zenith::ProjectState::PROP_LENGTH_BEATS].toString();
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

    auto tracksNode =
        projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    int maxTrackIndex =
        tracksNode.isValid() ? tracksNode.getNumChildren() - 1 : 0;

    firstVisibleTrackIndex =
        juce::jlimit(0, maxTrackIndex, firstVisibleTrackIndex);

    repaint();
  }

  recomputeClipBounds();
  repaint();
}

#ifdef ZENITH_USE_SKIA
void ArrangerComponent::drawSkia(SkCanvas* canvas) {
  auto bounds = getLocalBounds();
  // Background
  canvas->clear(SkColorSetRGB(30, 30, 30)); // 0xff1e1e1e

  // Tracks
  auto tracksNode =
      projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (tracksNode.isValid()) {
    int numTracks = tracksNode.getNumChildren();
    float width = (float)bounds.getWidth();

    SkPaint trackPaint;
    SkPaint dividerPaint;
    dividerPaint.setColor(SkColorSetRGB(60, 60, 60)); // trackDividerColour
    dividerPaint.setStrokeWidth(1.0f);

    SkPaint gridPaint;
    gridPaint.setColor(
        SkColorSetARGB(76, 100, 100, 100)); // gridLineColour alpha 0.3
    gridPaint.setStrokeWidth(1.0f);

    SkFont trackFont;
    trackFont.setSize(14.0f);
    trackFont.setEdging(SkFont::Edging::kAntiAlias);
    SkPaint trackTextPaint;
    trackTextPaint.setColor(
        SkColorSetARGB(178, 255, 255, 255)); // white alpha 0.7
    trackTextPaint.setAntiAlias(true);

    double startBeat = std::floor(viewStartBeats);
    double endBeat = viewStartBeats + (width / pixelsPerBeat);

    for (int i = firstVisibleTrackIndex; i < numTracks; ++i) {
      float y = trackIndexToY(i);
      if (y > bounds.getHeight())
        break;

      // Track lane background
      SkColor laneColor =
          (i % 2 == 0) ? SkColorSetRGB(40, 40, 40) : SkColorSetRGB(35, 35, 35);
      trackPaint.setColor(laneColor);
      canvas->drawRect(SkRect::MakeXYWH(0, y, width, (float)trackHeight), trackPaint);

      // Divider
      canvas->drawLine(0, y, width, y, dividerPaint);

      // Vertical grid lines
      for (double beat = startBeat; beat <= endBeat; beat += 1.0) {
        float x = beatsToX(beat);
        if (x >= 0 && x <= width)
          canvas->drawLine(x, y, x, y + (float)trackHeight, gridPaint);
      }

      // Track name and status indicators
      auto track = tracksNode.getChild(i);
      juce::String name = track[zenith::ProjectState::PROP_NAME].toString();
      bool isMuted = track[zenith::ProjectState::PROP_MUTE];
      bool isSoloed = track[zenith::ProjectState::PROP_SOLO];
      bool isArmed = track[zenith::ProjectState::PROP_ARMED];

      float textX = 10.0f;
      float indicatorY = y + 14.0f; // Center vertically relative to text roughly

      SkPaint indicatorPaint;
      indicatorPaint.setAntiAlias(true);

      if (isArmed) {
          indicatorPaint.setColor(SkColorSetRGB(255, 50, 50)); // Red
          canvas->drawCircle(textX + 4, indicatorY - 4, 4, indicatorPaint);
          textX += 14.0f;
      }
      if (isSoloed) {
          indicatorPaint.setColor(SkColorSetRGB(255, 165, 0)); // Orange
          canvas->drawCircle(textX + 4, indicatorY - 4, 4, indicatorPaint);
          textX += 14.0f;
      }
      if (isMuted) {
          indicatorPaint.setColor(SkColorSetRGB(100, 100, 100)); // Grey
          canvas->drawCircle(textX + 4, indicatorY - 4, 4, indicatorPaint);
          textX += 14.0f;
      }

      if (isMuted) {
          trackTextPaint.setColor(SkColorSetARGB(128, 150, 150, 150));
      } else {
          trackTextPaint.setColor(SkColorSetARGB(178, 255, 255, 255));
      }

      canvas->drawString(name.toRawUTF8(), textX, y + 20, trackFont,
                        trackTextPaint);
    }
  }

    // Clips
    for (const auto &clipView : clipViews) {
        SkRect clipRect = SkRect::MakeXYWH(
            (float)clipView.bounds.getX(), (float)clipView.bounds.getY(),
            (float)clipView.bounds.getWidth(), (float)clipView.bounds.getHeight());

        SkPaint clipPaint;

        // Selection border
        if (clipView.isSelected) {
            SkPaint borderPaint;
            borderPaint.setColor(SkColorSetRGB(255, 255, 255));
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(2.0f);
            canvas->drawRect(clipRect, borderPaint);
        }

        // Clip name
        if (clipRect.width() > 20.0f) {
            auto [track, clip] = projectState.findClip(clipView.clipId);
            if (clip.isValid()) {
                juce::String name = clip[zenith::ProjectState::PROP_NAME].toString();
                SkFont font;
                font.setSize(12.0f);
                font.setEdging(SkFont::Edging::kAntiAlias);
                SkPaint textPaint;
                textPaint.setColor(SkColorSetARGB(204, 0, 0, 0)); // black alpha 0.8
                textPaint.setAntiAlias(true);
                canvas->drawString(name.toRawUTF8(), clipRect.fLeft + 4.0f,
                                   clipRect.fTop + 14.0f, font, textPaint);
            }
        }
    }

    // Time Ruler
    {
        SkPaint rulerBgPaint;
        rulerBgPaint.setColor(SkColorSetRGB(42, 42, 42));
        canvas->drawRect(
            SkRect::MakeXYWH(0.0f, 0.0f, (float)bounds.getWidth(), (float)rulerHeight),
            rulerBgPaint);

        SkPaint tickPaint;
        tickPaint.setColor(SkColorSetRGB(100, 100, 100));
        tickPaint.setStrokeWidth(1.0f);

        SkFont font;
        font.setSize(12.0f);
        font.setEdging(SkFont::Edging::kAntiAlias);
        SkPaint textPaint;
        textPaint.setColor(SkColorSetRGB(200, 200, 200));
        textPaint.setAntiAlias(true);

        double startBeat = std::floor(viewStartBeats);
        double endBeat = viewStartBeats + (bounds.getWidth() / pixelsPerBeat);

        for (double beat = startBeat; beat <= endBeat; beat += 1.0) {
            float x = beatsToX(beat);
            if (x < 0 || x > bounds.getWidth())
                continue;

            canvas->drawLine(x, (float)rulerHeight - 8.0f, x, (float)rulerHeight, tickPaint);

            juce::String numStr = juce::String(static_cast<int>(beat + 1));
            float textWidth = font.measureText(numStr.toRawUTF8(), numStr.length(),
                                               SkTextEncoding::kUTF8);
            canvas->drawString(numStr.toRawUTF8(), x - textWidth / 2.0f, 15.0f, font,
                               textPaint);
        }
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
namespace zenith {

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

bool ArrangerComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& details)
{
    // Check if this is a browser drag
    juce::String description = details.description.toString();
    
    if (zenith::BrowserDragSource::isBrowserDrag(description))
    {
        auto type = zenith::BrowserDragSource::getTypeFromDescription(description);
        
        // Accept audio files, MIDI files, instruments, and plugins
        return type == zenith::BrowserItemType::AudioFile ||
               type == zenith::BrowserItemType::MidiFile ||
               type == zenith::BrowserItemType::Instrument ||
               type == zenith::BrowserItemType::Plugin;
    }
    
    return false;
}

void ArrangerComponent::itemDragEnter(const juce::DragAndDropTarget::SourceDetails& details)
{
    juce::ignoreUnused(details);
    isDropTargetActive_ = true;
    repaint();
}

void ArrangerComponent::itemDragExit(const juce::DragAndDropTarget::SourceDetails& details)
{
    juce::ignoreUnused(details);
    isDropTargetActive_ = false;
    dropTargetTrackIndex_ = -1;
    repaint();
}

void ArrangerComponent::itemDragMove(const juce::DragAndDropTarget::SourceDetails& details)
{
    // Calculate drop position
    auto localPos = getLocalPoint(details.sourceComponent, details.localPosition);
    
    dropTargetTrackIndex_ = yToTrackIndex(localPos.y);
    dropTargetBeats_ = snapToGrid(xToBeats(localPos.x));
    
    repaint();
}

void ArrangerComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails& details)
{
    isDropTargetActive_ = false;
    
    // Parse drag description to get item info
    juce::String description = details.description.toString();
    
    zenith::BrowserItemType itemType;
    juce::String itemId;
    juce::String itemName;
    
    if (!zenith::BrowserDragSource::parseDragDescription(description, itemType, itemId, itemName))
    {
        DBG("ArrangerComponent: Drop failed - could not parse drag description");
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
    auto tracksNode = projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    juce::String targetTrackId;
    
    if (tracksNode.isValid() && trackIndex >= 0 && trackIndex < tracksNode.getNumChildren())
    {
        // Use existing track
        auto track = tracksNode.getChild(trackIndex);
        targetTrackId = track[zenith::ProjectState::PROP_ID].toString();
    }
    else
    {
        // Create new track for the dropped item
        bool isMidiItem = itemType == zenith::BrowserItemType::MidiFile ||
                          itemType == zenith::BrowserItemType::Instrument;
        
        targetTrackId = projectState.createTrack(
            isMidiItem ? "midi" : "audio",
            itemName,
            "Drop new track"
        );
        
        DBG("ArrangerComponent: Created new track: " + targetTrackId);
    }
    
    if (targetTrackId.isEmpty())
    {
        DBG("ArrangerComponent: Drop failed - no target track");
        dropTargetTrackIndex_ = -1;
        repaint();
        return;
    }
    
    // Handle different item types
    switch (itemType)
    {
        case zenith::BrowserItemType::AudioFile:
        {
            // Create audio clip with the file
            juce::File audioFile(itemId);
            double clipLength = 4.0; // Default, will be updated when file loads
            
            juce::String clipId = projectState.createEmptyClip(
                targetTrackId, dropBeats, clipLength, false, 
                audioFile.getFileNameWithoutExtension(),
                "Drop audio file"
            );
            
            // Set the audio file path on the clip
            auto [track, clip] = projectState.findClip(clipId);
            if (clip.isValid())
            {
                clip.setProperty(zenith::ProjectState::PROP_AUDIO_FILE, audioFile.getFullPathName(), 
                                 &projectState.getUndoManager());
            }
            
            DBG("ArrangerComponent: Created audio clip from " + audioFile.getFileName());
            break;
        }
        
        case zenith::BrowserItemType::MidiFile:
        {
            // Create MIDI clip
            juce::File midiFile(itemId);
            
            juce::String clipId = projectState.createEmptyClip(
                targetTrackId, dropBeats, 4.0, true,
                midiFile.getFileNameWithoutExtension(),
                "Drop MIDI file"
            );
            
            DBG("ArrangerComponent: Created MIDI clip from " + midiFile.getFileName());
            break;
        }
        
        case zenith::BrowserItemType::Instrument:
        {
            // Create MIDI clip and load instrument
            juce::String clipId = projectState.createEmptyClip(
                targetTrackId, dropBeats, 4.0, true,
                itemName,
                "Drop instrument"
            );
            
            DBG("ArrangerComponent: Created clip for instrument " + itemName);
            break;
        }
        
        case zenith::BrowserItemType::Plugin:
        {
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

} // namespace zenith