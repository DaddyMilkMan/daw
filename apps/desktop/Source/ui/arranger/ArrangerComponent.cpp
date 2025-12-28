/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation
 */

#include "ArrangerComponent.h"
#include "ArrangerTrackComponent.h"

// Skia Includes
#ifdef ZENITH_USE_SKIA
#include "../framework/GlassmorphicPanel.h"
#include "../framework/NeonGlow.h"
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
#include "../../utils/StemSeparationJob.h" // For AI Stem Separation
#include "../browser/BrowserDragSource.h"
#include "../engine/AudioFilePool.h"
#include "ZenithDesignSystem.h" // Explicitly include to make typography visible
#include "ZenithUtils.h"

// JUCE Includes
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

// Standard Library
#include <algorithm>
#include <cmath>
#include <vector>



  setWantsKeyboardFocus(true);

  // Listen to ProjectState changes
  projectState.getState().addListener(this);

  // Initial clip view build
  rebuildClipViews();

  // Start timer for playhead position updates (60Hz for smooth visual feedback)
  startTimerHz(60);

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
      // Delay hide slightly or handle via generic finish
      freezeOverlay->setVisible(false);
    }
  };

  // Initialize Freeze Overlay (after MacroToolbar so it sits on top if added
  // later, but z-order matters) Actually addAndMakeVisible brings to front.
  freezeOverlay = std::make_unique<FreezeProgressOverlay>();
  addAndMakeVisible(freezeOverlay.get());
  freezeOverlay->setVisible(false);

  freezeOverlay->onCancel = [this]() { engine_.cancelFreeze(); };

  // Initialize Section Track
  sectionTrack = std::make_unique<ArrangerTrackComponent>(projectState);
  addChildComponent(sectionTrack.get());

  DBG("ArrangerComponent: Created");
}

ArrangerComponent::~ArrangerComponent() {
  stopTimer();
  projectState.getState().removeListener(this);
  DBG("ArrangerComponent: Destroyed");
}


  if (sectionTrack) {
    // Section track sits at the top, above the ruler
    // Or between Ruler and Tracks?
    // Based on TOP_MARGIN = SECTION_HEIGHT + RULER_HEIGHT, it implies Section
    // is top 24, Ruler is next 30? Let's place it at Y=0
    sectionTrack->setBounds(HEADER_WIDTH, 0, getWidth() - HEADER_WIDTH,
                            (int)SECTION_HEIGHT);
  }

  if (macroToolbar) {
    // Center horizontally, float near top (offset by Ruler + padding)
    float w = 420.0f;
    float h = 60.0f;
    float x = (getWidth() - w) * 0.5f;
    float y = RULER_HEIGHT + 20.0f;
    float x = (getWidth() - w) * 0.5f;
    float y = RULER_HEIGHT + 20.0f;
    macroToolbar->setBounds((int)x, (int)y, (int)w, (int)h);
  }

  if (freezeOverlay) {
    freezeOverlay->setBounds(getLocalBounds());
  }

  // Layout Tracks
  const float trackHeight = TRACK_HEIGHT; // 80.0f
  // We need to account for scroll position (firstVisibleTrackIndex)
  // For now, simple vertical stack starting from TOP_MARGIN

  float yEntry = TOP_MARGIN; // + (0 - firstVisibleTrackIndex) * trackHeight?
  // Actually trackIndexToY handles the scroll math:
  // TOP_MARGIN + (trackIndex - firstVisibleTrackIndex) * TRACK_HEIGHT

  for (size_t i = 0; i < trackComponents.size(); ++i) {
    float y = trackIndexToY((int)i);
    if (y + trackHeight < TOP_MARGIN || y > getHeight()) {
      trackComponents[i]->setVisible(false);
    } else {
      trackComponents[i]->setVisible(true);
      trackComponents[i]->setBounds(0, (int)y, getWidth(), (int)trackHeight);
      trackComponents[i]->setViewContext(pixelsPerBeat, viewStartBeats);
    }
  }
}

}
#endif

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

  if (clip != nullptr) {
    // Double-clicked existing clip - trigger callback
    if (onClipDoubleClicked) {
      onClipDoubleClicked(clip->trackId, clip->clipId);
    }
    return;
  }

  // Double-clicked empty area - create clip
  if (e.position.x < HEADER_WIDTH || e.position.y < SECTION_HEIGHT)
    return;

  double beat = viewStartBeats + (e.position.x - HEADER_WIDTH) / pixelsPerBeat;
  int trackIndex = yToTrackIndex(e.position.y);

  // Snap to nearest integer beat
  beat = std::floor(beat);

  if (beat < 0)
    beat = 0;

  auto tracksNode =
      projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
  if (tracksNode.isValid() && trackIndex >= 0 &&
      trackIndex < tracksNode.getNumChildren()) {
    auto track = tracksNode.getChild(trackIndex);
    juce::String trackId = track[zenith::ProjectState::PROP_ID].toString();
    juce::String type = track[zenith::ProjectState::PROP_TYPE].toString();
    bool isMidi = (type != "audio");

    projectState.createEmptyClip(trackId, beat, 4.0, isMidi, "New Clip",
                                 "Double Click Create");
    repaint();
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

    recomputeClipBounds();
    repaint();
  }
}

}

} // namespace zenith
