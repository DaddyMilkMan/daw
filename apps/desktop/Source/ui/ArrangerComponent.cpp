/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/Arranger view implementation
 */

#include "../../include/ui/ArrangerComponent.h"
#include "../../include/ui/ArrangerTrackComponent.h"

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
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float SECTION_HEIGHT = 24.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT =
    80.0f; // Taller tracks for better visibility
static constexpr float TOP_MARGIN =
    SECTION_HEIGHT + RULER_HEIGHT; // Offset for tracks
static constexpr float SCROLLBAR_HEIGHT = 14.0f;

// Grid Visibility Constants
static constexpr uint8_t kBarHighlightAlphaTop = 15;    // Zebra stripe gradient top
static constexpr uint8_t kBarHighlightAlphaBottom = 8;  // Zebra stripe gradient bottom
static constexpr uint8_t kBarLineAlpha = 100;           // Bar line opacity
static constexpr float kBarLineWidth = 1.5f;            // Bar line stroke width
static constexpr uint8_t kBeatLineAlpha = 50;           // Beat line opacity

//==============================================================================

ArrangerComponent::ArrangerComponent(Engine &eng, ProjectState &ps)
    : engine_(eng), projectState(ps) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

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
  };

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

  // Indexing is handled by iteration order

  int trackIndex = 0;
  for (const auto &track : tracksNode) {
    auto trackId = track[zenith::ProjectState::PROP_ID].toString();
    auto clipsNode = track.getChildWithName(zenith::ProjectState::ID_CLIPS);

    if (clipsNode.isValid()) {
      for (const auto &clip : clipsNode) {
        ClipView view;
        view.clipId = clip[zenith::ProjectState::PROP_ID].toString();
        view.trackId = trackId;
        view.trackIndex = trackIndex; // Cache track index in ClipView
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
    // OPTIMIZATION: Use cached trackIndex from ClipView (O(1) instead of O(N)
    // per clip)
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

  // Cache invariant values for the loop
  const float trackHeight =
      static_cast<float>(TRACK_HEIGHT - 4); // 2px margin top/bottom

  for (auto &clipView : clipViews) {
    // OPTIMIZATION: Use cached trackIndex from ClipView (O(1) instead of O(N)
    // per clip)
    float x = beatsToX(clipView.startBeats);
    float y = trackIndexToY(clipView.trackIndex);
    float width = static_cast<float>(clipView.lengthBeats * pixelsPerBeat);

    clipView.bounds = juce::Rectangle<float>(x, y + 2.0f, width, trackHeight);
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
    macroToolbar->setBounds((int)x, (int)y, (int)w, (int)h);
  }
}

//==============================================================================
// Component interface - Painting (Main Render Loop)
//==============================================================================

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
  // 2. GRID & TIMELINE - PREMIUM RENDERING
  // ============================================================================
  int beatsPerBar = getBeatsPerBar();
  double startBeat = std::floor(viewStartBeats);
  double endBeat = viewStartBeats + ((width - HEADER_WIDTH) / pixelsPerBeat);

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
  canvas->clipRect(SkRect::MakeXYWH(HEADER_WIDTH, SECTION_HEIGHT,
                                    width - HEADER_WIDTH,
                                    height - SECTION_HEIGHT));

  // A. Alternating BAR HIGHLIGHTING (subtle zebra striping)
  SkPaint barHighlightPaint;
  barHighlightPaint.setStyle(SkPaint::kFill_Style);
  barHighlightPaint.setAntiAlias(false);

  int startBar = static_cast<int>(std::floor(startBeat / beatsPerBar));
  int endBar = static_cast<int>(std::ceil(endBeat / beatsPerBar));

  for (int bar = startBar; bar <= endBar; ++bar) {
    if (bar % 2 == 0) {
      // Every other bar gets subtle highlight
      float barStartX = beatsToX(bar * beatsPerBar);
      float barEndX = beatsToX((bar + 1) * beatsPerBar);

      // Subtle gradient highlight
      SkPoint pts[2] = {{barStartX, SECTION_HEIGHT}, {barStartX, height}};
      SkColor gradColors[2] = {
          SkColorSetARGB(kBarHighlightAlphaTop, 255, 255, 255),
          SkColorSetARGB(kBarHighlightAlphaBottom, 255, 255, 255)
      };
      barHighlightPaint.setShader(SkGradientShader::MakeLinear(
          pts, gradColors, nullptr, 2, SkTileMode::kClamp));

      canvas->drawRect(SkRect::MakeXYWH(barStartX, SECTION_HEIGHT,
                                        barEndX - barStartX,
                                        height - SECTION_HEIGHT),
                       barHighlightPaint);
      barHighlightPaint.setShader(nullptr);
    }
  }

  // B. GRID LINES with hierarchy
  for (double beat = startBeat; beat <= endBeat; beat += beatStep) {
    float x = beatsToX(beat);
    int beatNum = static_cast<int>(beat);
    bool isBarLine = (beatNum % beatsPerBar == 0);

    SkPaint gridPaint;
    gridPaint.setAntiAlias(true);

    if (isBarLine) {
      // BAR LINES - more visible, solid
      gridPaint.setColor(SkColorSetARGB(kBarLineAlpha, 255, 255, 255));
      gridPaint.setStrokeWidth(kBarLineWidth);
    } else {
      // BEAT LINES - subtle, dotted
      gridPaint.setColor(SkColorSetARGB(kBeatLineAlpha, 255, 255, 255));
      gridPaint.setStrokeWidth(1.0f);
      static const SkScalar intervals[] = {2.0f, 4.0f};
      static const auto dashEffect =
          SkDashPathEffect::Make(SkSpan<const SkScalar>(intervals, 2), 0.0f);
      gridPaint.setPathEffect(dashEffect);
    }

    canvas->drawLine(x, SECTION_HEIGHT, x, height, gridPaint);
  }
  canvas->restore();

  // C. HEADER/TIMELINE BOUNDARY GLOW
  SkPaint boundaryGlowPaint;
  SkPoint glowPts[2] = {{HEADER_WIDTH, 0}, {HEADER_WIDTH + 30, 0}};
  SkColor glowColors[2] = {
      SkColorSetARGB(40, 0, 200, 255), // Subtle cyan glow
      SkColorSetARGB(0, 0, 200, 255)   // Fade out
  };
  boundaryGlowPaint.setShader(SkGradientShader::MakeLinear(
      glowPts, glowColors, nullptr, 2, SkTileMode::kClamp));
  canvas->drawRect(SkRect::MakeXYWH(HEADER_WIDTH, SECTION_HEIGHT, 30,
                                    height - SECTION_HEIGHT),
                   boundaryGlowPaint);

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
        juce::Colour c = section->color;
        if (c.isTransparent())
          c = juce::Colours::cyan; // Fallback
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

      // A. Track Header Background - PREMIUM GLASSMORPHIC GRADIENT
      {
        // Gradient from slightly lighter top to darker bottom
        SkPoint hdrGradPts[2] = {{0, y}, {0, y + trackHeight}};
        SkColor hdrGradColors[3] = {
            SkColorSetRGB(35, 45,
                          55), // Top - Cyan tint (visible Neon Noir style)
            SkColorSetRGB(25, 25, 30), // Middle
            SkColorSetRGB(18, 18, 22)  // Bottom - darkest
        };
        float hdrPositions[3] = {0.0f, 0.3f, 1.0f};
        trackBgPaint.setShader(SkGradientShader::MakeLinear(
            hdrGradPts, hdrGradColors, hdrPositions, 3, SkTileMode::kClamp));
        canvas->drawRect(headerRect, trackBgPaint);
        trackBgPaint.setShader(nullptr);

        // Top edge highlight (glass effect)
        SkPaint topHighlight;
        topHighlight.setColor(SkColorSetARGB(20, 255, 255, 255));
        topHighlight.setStrokeWidth(1.0f);
        canvas->drawLine(0, y + 0.5f, HEADER_WIDTH, y + 0.5f, topHighlight);
      }

      // B. Track Timeline Background - Subtle alternating row tint
      if (i % 2 == 1) {
        SkPaint altRowPaint;
        altRowPaint.setColor(SkColorSetARGB(8, 255, 255, 255));
        canvas->drawRect(SkRect::MakeXYWH(HEADER_WIDTH, y, width - HEADER_WIDTH,
                                          trackHeight),
                         altRowPaint);
      }

      // C. Separator with gradient fade
      {
        SkPaint sepPaint;
        SkPoint sepPts[2] = {{0, 0}, {width, 0}};
        SkColor sepColors[3] = {
            SkColorSetARGB(60, 255, 255, 255), // Left - visible
            SkColorSetARGB(30, 255, 255, 255), // Middle
            SkColorSetARGB(10, 255, 255, 255)  // Right - faded
        };
        float sepPos[3] = {0.0f, 0.3f, 1.0f};
        sepPaint.setShader(SkGradientShader::MakeLinear(
            sepPts, sepColors, sepPos, 3, SkTileMode::kClamp));
        sepPaint.setStrokeWidth(1.0f);
        canvas->drawLine(0, y + trackHeight - 0.5f, width,
                         y + trackHeight - 0.5f, sepPaint);
      }

      // D. Header Content
      auto track = tracksNode.getChild(i);
      juce::String name = track[zenith::ProjectState::PROP_NAME].toString();

      // Track Number Badge
      {
        SkPaint badgePaint;
        badgePaint.setAntiAlias(true);
        badgePaint.setColor(SkColorSetARGB(40, 255, 255, 255));
        SkRect badgeRect = SkRect::MakeXYWH(spacing::SM, y + 8, 24, 18);
        canvas->drawRRect(SkRRect::MakeRectXY(badgeRect, 4, 4), badgePaint);

        SkPaint numPaint;
        numPaint.setAntiAlias(true);
        numPaint.setColor(colors::TEXT_SECONDARY);
        canvas->drawString(juce::String(i + 1).toStdString().c_str(),
                           spacing::SM + 6, y + 21, smallFont, numPaint);
      }

      // Track Name with text shadow
      {
        SkPaint shadowPaint;
        shadowPaint.setAntiAlias(true);
        shadowPaint.setColor(SkColorSetARGB(80, 0, 0, 0));
        canvas->drawString(name.toStdString().c_str(), spacing::MD + 24 + 1,
                           y + 23.0f + 1, nameFont, shadowPaint);

        textPaint.setColor(colors::TEXT_PRIMARY);
        canvas->drawString(name.toStdString().c_str(), spacing::MD + 24,
                           y + 23.0f, nameFont, textPaint);
      }

      // Controls (Mute/Solo/Rec) - PREMIUM PILL BUTTONS
      float btnY = y + 42.0f;
      float btnSize = 22.0f;
      float btnGap = 28.0f;
      float startX = spacing::MD;

      // Helper for premium buttons
      auto drawTrackButton = [&](float bx, const char *label, bool active,
                                 SkColor activeColor) {
        SkRect btnRect = SkRect::MakeXYWH(bx, btnY, btnSize, btnSize);
        SkRRect btnRRect = SkRRect::MakeRectXY(btnRect, 6.0f, 6.0f);

        // Drop shadow
        SkPaint shadowPaint;
        shadowPaint.setAntiAlias(true);
        shadowPaint.setColor(SkColorSetARGB(40, 0, 0, 0));
        shadowPaint.setMaskFilter(
            SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
        SkRect shadowRect = btnRect;
        shadowRect.offset(0, 1);
        canvas->drawRRect(SkRRect::MakeRectXY(shadowRect, 6.0f, 6.0f),
                          shadowPaint);

        if (active) {
          // OUTER GLOW first
          SkPaint glowPaint;
          glowPaint.setAntiAlias(true);
          glowPaint.setColor(withAlpha(activeColor, 0.5f));
          glowPaint.setMaskFilter(
              SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
          canvas->drawRRect(btnRRect, glowPaint);

          // Gradient fill
          SkPaint btnPaint;
          btnPaint.setAntiAlias(true);
          SkPoint gradPts[2] = {{bx, btnY}, {bx, btnY + btnSize}};
          SkColor gradColors[2] = {lighten(activeColor, 0.2f), activeColor};
          btnPaint.setShader(SkGradientShader::MakeLinear(
              gradPts, gradColors, nullptr, 2, SkTileMode::kClamp));
          canvas->drawRRect(btnRRect, btnPaint);

          // Inner highlight
          SkPaint innerHighlight;
          innerHighlight.setAntiAlias(true);
          innerHighlight.setStyle(SkPaint::kStroke_Style);
          innerHighlight.setStrokeWidth(1.0f);
          innerHighlight.setColor(SkColorSetARGB(80, 255, 255, 255));
          SkRRect innerRRect = btnRRect;
          innerRRect.inset(0.5f, 0.5f);
          canvas->drawRRect(innerRRect, innerHighlight);
        } else {
          // Inactive button with subtle gradient
          SkPaint btnPaint;
          btnPaint.setAntiAlias(true);
          SkPoint gradPts[2] = {{bx, btnY}, {bx, btnY + btnSize}};
          SkColor gradColors[2] = {SkColorSetRGB(55, 55, 65),
                                   SkColorSetRGB(40, 40, 48)};
          btnPaint.setShader(SkGradientShader::MakeLinear(
              gradPts, gradColors, nullptr, 2, SkTileMode::kClamp));
          canvas->drawRRect(btnRRect, btnPaint);

          // Border
          SkPaint borderPaint;
          borderPaint.setAntiAlias(true);
          borderPaint.setStyle(SkPaint::kStroke_Style);
          borderPaint.setStrokeWidth(1.0f);
          borderPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
          canvas->drawRRect(btnRRect, borderPaint);
        }

        // Label - centered
        SkPaint labelPaint;
        labelPaint.setAntiAlias(true);
        labelPaint.setColor(active ? SK_ColorWHITE : colors::TEXT_SECONDARY);

        // Better centering
        float textX = bx + (btnSize - 8.0f) / 2.0f;
        float textY = btnY + btnSize * 0.68f;
        canvas->drawString(label, textX, textY, smallFont, labelPaint);
      };

      bool isMuted = track[zenith::ProjectState::PROP_MUTE];
      bool isSoloed = track[zenith::ProjectState::PROP_SOLO];
      bool isArmed = track[zenith::ProjectState::PROP_ARMED];

      drawTrackButton(startX, "M", isMuted, colors::AMBER);
      drawTrackButton(startX + btnGap, "S", isSoloed, colors::NEON_CYAN);
      drawTrackButton(startX + btnGap * 2, "R", isArmed, colors::NEON_RED);

      // E. Right Border for Header - GLASSY DIVIDER
      {
        SkPaint dividerPaint;
        SkPoint divPts[2] = {{HEADER_WIDTH - 1, y},
                             {HEADER_WIDTH - 1, y + trackHeight}};
        SkColor divColors[3] = {
            SkColorSetARGB(60, 255, 255, 255), // Top highlight
            SkColorSetARGB(30, 255, 255, 255), // Middle
            SkColorSetARGB(10, 255, 255, 255)  // Bottom fade
        };
        float divPos[3] = {0.0f, 0.2f, 1.0f};
        dividerPaint.setShader(SkGradientShader::MakeLinear(
            divPts, divColors, divPos, 3, SkTileMode::kClamp));
        dividerPaint.setStrokeWidth(1.0f);
        canvas->drawLine(HEADER_WIDTH - 0.5f, y, HEADER_WIDTH - 0.5f,
                         y + trackHeight, dividerPaint);

        // Dark side shadow
        SkPaint shadowLine;
        shadowLine.setColor(SkColorSetARGB(40, 0, 0, 0));
        shadowLine.setStrokeWidth(1.0f);
        canvas->drawLine(HEADER_WIDTH + 0.5f, y, HEADER_WIDTH + 0.5f,
                         y + trackHeight, shadowLine);
      }
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

    // Use larger radius for premium feel
    float clipRadius = 6.0f;
    SkRRect rr = SkRRect::MakeRectXY(r, clipRadius, clipRadius);

    // Determine base color based on clip type
    SkColor baseColor = clipView.isMidi ? colors::MAGENTA : colors::CYAN;
    if (clipView.isSelected) {
      baseColor = lighten(baseColor, 0.15f);
    }

    // ========================================
    // 1. DROP SHADOW (underneath clip)
    // ========================================
    {
      SkPaint shadowPaint;
      shadowPaint.setAntiAlias(true);
      shadowPaint.setColor(SkColorSetARGB(60, 0, 0, 0));
      shadowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
      SkRect shadowRect = r;
      shadowRect.offset(0, 2);
      canvas->drawRRect(SkRRect::MakeRectXY(shadowRect, clipRadius, clipRadius),
                        shadowPaint);
    }

    // ========================================
    // 2. GLASSMORPHIC BACKGROUND
    // ========================================
    {
      // Multi-stop gradient for depth
      SkPaint bgPaint;
      bgPaint.setAntiAlias(true);

      SkPoint pts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
      SkColor bgColors[3] = {
          withAlpha(lighten(baseColor, 0.1f), 0.5f), // Top - lighter
          withAlpha(baseColor, 0.35f),               // Middle
          withAlpha(darken(baseColor, 0.2f), 0.25f)  // Bottom - darker
      };
      float bgPositions[3] = {0.0f, 0.4f, 1.0f};

      bgPaint.setShader(SkGradientShader::MakeLinear(pts, bgColors, bgPositions,
                                                     3, SkTileMode::kClamp));
      canvas->drawRRect(rr, bgPaint);
    }

    // ========================================
    // 3. INNER GLASSMORPHIC TEXTURE
    // ========================================
    {
      // Subtle noise/texture overlay for glass feel
      SkPaint texturePaint;
      texturePaint.setAntiAlias(true);
      texturePaint.setColor(SkColorSetARGB(8, 255, 255, 255));
      texturePaint.setBlendMode(SkBlendMode::kOverlay);
      canvas->drawRRect(rr, texturePaint);
    }

    // ========================================
    // 4. TOP RIM LIGHT (glass highlight)
    // ========================================
    {
      SkPaint rimPaint;
      rimPaint.setAntiAlias(true);
      rimPaint.setStyle(SkPaint::kStroke_Style);
      rimPaint.setStrokeWidth(1.0f);

      SkPoint rimPts[2] = {{r.left(), r.top()},
                           {r.right() * 0.6f, r.top() + r.height() * 0.3f}};
      SkColor rimColors[2] = {
          SkColorSetARGB(120, 255, 255, 255), // Bright highlight
          SkColorSetARGB(0, 255, 255, 255)    // Fade out
      };
      rimPaint.setShader(SkGradientShader::MakeLinear(
          rimPts, rimColors, nullptr, 2, SkTileMode::kClamp));

      SkRRect innerRR = rr;
      innerRR.inset(0.5f, 0.5f);
      canvas->drawRRect(innerRR, rimPaint);
    }

    // ========================================
    // 5. CLIP BORDER
    // ========================================
    {
      SkPaint borderPaint;
      borderPaint.setAntiAlias(true);
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(1.0f);

      // Gradient border - brighter at top
      SkPoint borderPts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
      SkColor borderColors[2] = {withAlpha(baseColor, 0.8f),
                                 withAlpha(baseColor, 0.4f)};
      borderPaint.setShader(SkGradientShader::MakeLinear(
          borderPts, borderColors, nullptr, 2, SkTileMode::kClamp));

      canvas->drawRRect(rr, borderPaint);
    }

    // ========================================
    // 6. SELECTION GLOW (outer neon effect)
    // ========================================
    if (clipView.isSelected) {
      // Outer glow
      SkPaint glowPaint;
      glowPaint.setAntiAlias(true);
      glowPaint.setStyle(SkPaint::kStroke_Style);
      glowPaint.setStrokeWidth(3.0f);
      glowPaint.setColor(withAlpha(colors::NEON_CYAN, 0.6f));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
      canvas->drawRRect(rr, glowPaint);

      // Core bright border
      SkPaint corePaint;
      corePaint.setAntiAlias(true);
      corePaint.setStyle(SkPaint::kStroke_Style);
      corePaint.setStrokeWidth(1.5f);
      corePaint.setColor(colors::NEON_CYAN);
      canvas->drawRRect(rr, corePaint);
    }

    // ========================================
    // 7. CONTENT (Waveform or MIDI)
    // ========================================
    {
      // Inset the content area for padding
      SkRect contentRect = r;
      contentRect.inset(4.0f, 16.0f); // Leave room for label at top
      contentRect.fTop += 4.0f;

      canvas->save();
      canvas->clipRRect(rr, true);

      if (clipView.isMidi) {
        // MIDI clip - enhanced piano roll visualization
        drawClipMidiBlobs(canvas, clipView, contentRect);
      } else {
        // Audio clip - enhanced waveform visualization
        drawClipWaveform(canvas, clipView, contentRect);
      }

      canvas->restore();
    }

    // ========================================
    // 8. CLIP LABEL (with pill background)
    // ========================================
    {
      // Label background pill
      SkPaint pillPaint;
      pillPaint.setAntiAlias(true);
      pillPaint.setColor(SkColorSetARGB(140, 0, 0, 0));

      SkRect pillRect = SkRect::MakeXYWH(r.left() + 4, r.top() + 4,
                                         std::min(r.width() - 8, 100.0f), 14);
      canvas->drawRRect(SkRRect::MakeRectXY(pillRect, 3, 3), pillPaint);

      // Clip name text
      SkFont labelFont =
          typography::getSkFont(typography::FONT_XS, FontWeight::Medium);
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      textPaint.setColor(SK_ColorWHITE);

      // Truncate text if too long
      juce::String displayName = clipView.clipId;
      if (displayName.length() > 12) {
        displayName = displayName.substring(0, 10) + "...";
      }

      canvas->drawString(displayName.toStdString().c_str(), r.left() + 8,
                         r.top() + 14, labelFont, textPaint);
    }

    // ========================================
    // 9. CLIP TYPE INDICATOR (icon badge)
    // ========================================
    {
      // Small badge in bottom-right corner
      float badgeSize = 14.0f;
      SkRect badgeRect =
          SkRect::MakeXYWH(r.right() - badgeSize - 4,
                           r.bottom() - badgeSize - 4, badgeSize, badgeSize);

      SkPaint badgePaint;
      badgePaint.setAntiAlias(true);
      badgePaint.setColor(withAlpha(baseColor, 0.8f));
      canvas->drawRRect(SkRRect::MakeRectXY(badgeRect, 3, 3), badgePaint);

      // Icon (simple shape)
      SkPaint iconPaint;
      iconPaint.setAntiAlias(true);
      iconPaint.setColor(SK_ColorWHITE);
      iconPaint.setStyle(SkPaint::kStroke_Style);
      iconPaint.setStrokeWidth(1.5f);

      float cx = badgeRect.centerX();
      float cy = badgeRect.centerY();

      if (clipView.isMidi) {
        // MIDI icon - musical note shape
        canvas->drawLine(cx - 2, cy + 3, cx - 2, cy - 2, iconPaint);
        canvas->drawCircle(cx - 3, cy + 2, 2, iconPaint);
      } else {
        // Audio icon - waveform shape
        canvas->drawLine(cx - 3, cy, cx - 1, cy - 2, iconPaint);
        canvas->drawLine(cx - 1, cy - 2, cx + 1, cy + 2, iconPaint);
        canvas->drawLine(cx + 1, cy + 2, cx + 3, cy, iconPaint);
      }
    }
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
    static const SkScalar dashIntervals[] = {2.0f, 4.0f};
    marqueePaint.setPathEffect(
        SkDashPathEffect::Make(SkSpan<const SkScalar>(dashIntervals, 2), 0.0f));
    canvas->drawRect(mRect, marqueePaint);
  }

  // ============================================================================
  // 6. SECTION TRACK (Manual render per instructions)
  // ============================================================================
  if (sectionTrack) {
    canvas->save();
    // Translate to section track position
    canvas->translate(sectionTrack->getX(), sectionTrack->getY());

    // We must ensure the section track has the correct view context
    sectionTrack->setViewContext(pixelsPerBeat, viewStartBeats);

    // Determine clip rect for the section track to ensure it doesn't draw over
    // the header
    SkRect sectionClip =
        SkRect::MakeWH(sectionTrack->getWidth(), sectionTrack->getHeight());
    canvas->clipRect(sectionClip);

    sectionTrack->drawSkia(canvas);
    canvas->restore();
  }

  // ============================================================================
  // 7. PLAYHEAD (The "Laser")
  // ============================================================================
  // Convert playhead beats to X
  float playheadX = beatsToX(playheadBeats_);

  // Only draw if visible
  if (playheadX >= HEADER_WIDTH && playheadX <= width) {
    SkPaint playheadPaint;
    playheadPaint.setColor(colors::NEON_RED); // Red/Neon as requested
    playheadPaint.setStrokeWidth(2.0f);
    playheadPaint.setAntiAlias(true);

    // Glow Effect
    playheadPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    canvas->drawLine(playheadX, 0, playheadX, height, playheadPaint);

    // Core Line
    playheadPaint.setMaskFilter(nullptr);
    playheadPaint.setColor(SK_ColorWHITE); // White hot core
    playheadPaint.setStrokeWidth(1.0f);
    canvas->drawLine(playheadX, 0, playheadX, height, playheadPaint);

    // Triangle Cap
    SkPath cap;
    cap.moveTo(playheadX - 6, RULER_HEIGHT);
    cap.lineTo(playheadX + 6, RULER_HEIGHT);
    cap.lineTo(playheadX, RULER_HEIGHT + 8);
    cap.close();

    SkPaint capPaint;
    capPaint.setColor(colors::NEON_RED);
    capPaint.setStyle(SkPaint::kFill_Style);
    capPaint.setAntiAlias(true);
    canvas->drawPath(cap, capPaint);
  }

  // ============================================================================
  // 8. INSERTION GUIDE (Ripple/Insert Mode)
  // ============================================================================
  if ((currentDragMode == DragMode::MoveClips) && insertionGuideX >= 0.0f &&
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

  if (clip.noteBlobs.empty() || clip.lengthBeats <= 0.001) {
    // No notes - draw a GENERATIVE placeholder pattern so it's visible
    juce::Random rng(clip.clipId.hashCode());

    SkPaint placeholderPaint;
    placeholderPaint.setColor(
        SkColorSetARGB(150, 255, 255, 255)); // Much brighter
    placeholderPaint.setAntiAlias(true);

    int numNotes = (int)(clipRect.width() / 15.0f) + 1;
    for (int i = 0; i < numNotes; ++i) {
      if (rng.nextFloat() > 0.6f)
        continue;

      float x = clipRect.left() + i * 15.0f + rng.nextFloat() * 5.0f;
      float y = clipRect.top() + 10.0f +
                rng.nextFloat() * (clipRect.height() - 20.0f);
      float w = 10.0f + rng.nextFloat() * 10.0f;
      float h = 4.0f;

      if (x + w > clipRect.right())
        w = clipRect.right() - x;

      SkRect r = SkRect::MakeXYWH(x, y, w, h);
      canvas->drawRRect(SkRRect::MakeRectXY(r, 2, 2), placeholderPaint);

      // Ghost tail
      SkPaint tailPaint;
      tailPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
      canvas->drawRect(SkRect::MakeXYWH(x + w, y, 5, h), tailPaint);
    }
    return;
  }

  // Calculate pitch range for better visualization
  int minPitch = 127, maxPitch = 0;
  for (const auto &blob : clip.noteBlobs) {
    minPitch = std::min(minPitch, blob.pitch);
    maxPitch = std::max(maxPitch, blob.pitch);
  }

  // Add padding to pitch range
  int pitchRange = std::max(12, maxPitch - minPitch + 4);
  int pitchMin = std::max(0, minPitch - 2);

  // Draw notes with gradient and rounded corners
  for (const auto &blob : clip.noteBlobs) {
    float nx = clipRect.left() +
               (blob.startBeats / clip.lengthBeats) * clipRect.width();
    float nw = (blob.lengthBeats / clip.lengthBeats) * clipRect.width();

    // Normalize pitch to visible range
    float normalizedPitch = (float)(blob.pitch - pitchMin) / (float)pitchRange;
    normalizedPitch = std::clamp(normalizedPitch, 0.0f, 1.0f);

    float ny =
        clipRect.bottom() - 4 - (normalizedPitch * (clipRect.height() - 8));
    float noteHeight = std::max(3.0f, clipRect.height() / (float)pitchRange);
    noteHeight = std::min(noteHeight, 8.0f);

    SkRect noteRect = SkRect::MakeXYWH(nx, ny - noteHeight / 2,
                                       std::max(4.0f, nw - 1.0f), noteHeight);

    // Note background with gradient
    SkPaint notePaint;
    notePaint.setAntiAlias(true);

    SkPoint notePts[2] = {{noteRect.left(), noteRect.top()},
                          {noteRect.left(), noteRect.bottom()}};
    SkColor noteColors[2] = {SkColorSetARGB(220, 255, 255, 255),
                             SkColorSetARGB(160, 200, 200, 220)};
    notePaint.setShader(SkGradientShader::MakeLinear(
        notePts, noteColors, nullptr, 2, SkTileMode::kClamp));

    canvas->drawRRect(SkRRect::MakeRectXY(noteRect, 2.0f, 2.0f), notePaint);

    // Subtle shadow under each note
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(SkColorSetARGB(40, 0, 0, 0));
    SkRect shadowRect = noteRect;
    shadowRect.offset(0, 1);
    canvas->drawRRect(SkRRect::MakeRectXY(shadowRect, 2.0f, 2.0f), shadowPaint);
  }
}

void ArrangerComponent::drawClipWaveform(SkCanvas *canvas, const ClipView &clip,
                                         const SkRect &clipRect) {
  using namespace zenith::design;

  auto *cache = getWaveformCache(clip.audioFilePath);
  if (!cache || !cache->isValid || cache->minPeaks.empty()) {
    // No waveform - draw a placeholder waveform shape
    SkPaint placeholderPaint;
    placeholderPaint.setColor(
        SkColorSetARGB(150, 255, 255, 255)); // Much brighter for visibility
    placeholderPaint.setStrokeWidth(1.5f);
    placeholderPaint.setStyle(SkPaint::kStroke_Style);
    placeholderPaint.setAntiAlias(true);

    SkPath placeholder;
    float cY = clipRect.centerY();
    float amp = clipRect.height() * 0.25f;
    placeholder.moveTo(clipRect.left(), cY);

    // Draw a sine-wave-like shape
    for (float x = clipRect.left(); x < clipRect.right(); x += 4) {
      float phase = (x - clipRect.left()) / 20.0f;
      float y =
          cY + std::sin(phase) * amp * (0.3f + 0.7f * std::sin(phase * 0.3f));
      placeholder.lineTo(x, y);
    }

    canvas->drawPath(placeholder, placeholderPaint);
    return;
  }

  // Enhanced filled waveform with gradient
  float midY = clipRect.centerY();
  float heightScale = clipRect.height() * 0.45f;

  size_t numPeaks = cache->minPeaks.size();
  float barWidth = clipRect.width() / static_cast<float>(numPeaks);
  barWidth = std::max(1.0f, barWidth);

  // Create fill gradient
  SkPoint gradPts[2] = {{0, clipRect.top()}, {0, clipRect.bottom()}};
  SkColor gradColors[3] = {
      SkColorSetARGB(180, 255, 255, 255), // Top - bright
      SkColorSetARGB(120, 200, 220, 255), // Middle - slightly blue
      SkColorSetARGB(80, 150, 180, 220)   // Bottom - faded
  };
  float gradPositions[3] = {0.0f, 0.5f, 1.0f};

  SkPaint wavePaint;
  wavePaint.setShader(SkGradientShader::MakeLinear(
      gradPts, gradColors, gradPositions, 3, SkTileMode::kClamp));
  wavePaint.setAntiAlias(true);

  // Draw as filled bars for better visibility
  for (size_t i = 0; i < numPeaks; ++i) {
    float x = clipRect.left() + i * barWidth;
    float top = midY - (cache->maxPeaks[i] * heightScale);
    float bottom = midY - (cache->minPeaks[i] * heightScale);

    // Ensure minimum height for visibility
    float barHeight = std::max(2.0f, bottom - top);

    SkRect barRect =
        SkRect::MakeXYWH(x, top, std::max(1.0f, barWidth - 0.5f), barHeight);
    canvas->drawRect(barRect, wavePaint);
  }

  // Center line for reference
  SkPaint centerLinePaint;
  centerLinePaint.setColor(SkColorSetARGB(40, 255, 255, 255));
  centerLinePaint.setStrokeWidth(0.5f);
  canvas->drawLine(clipRect.left(), midY, clipRect.right(), midY,
                   centerLinePaint);
}
#endif

} // namespace zenith
