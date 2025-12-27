
/**
 * @file PianoRollComponent.cpp
 * @brief Professional-grade MIDI Piano Roll Editor Implementation (Core)
 */

#include "PianoRollComponent.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/GlassmorphicPanel.h"
#include "../../engine/Engine.h"
#include <algorithm>
#include <cmath>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <effects/SkDashPathEffect.h>
#include <effects/SkGradientShader.h>
#include <limits>
#include "../controls/SkiaPopupMenu.h"
#include "../controls/ContextMenuManager.h"
#include "../../network/CollaborationManager.h"

#include <unordered_set>
#include <vector>

using namespace zenith;

// Magic numbers moved to constants/theme
static constexpr float NOTE_CORNER_RADIUS = design::dimensions::RADIUS_SM; // 8px for modern look
static constexpr float SELECTION_STROKE_WIDTH = 2.0f;
static constexpr float HOVER_STROKE_WIDTH = 2.0f;
constexpr float RULER_HEIGHT = 30.0f;
constexpr float TOOLBAR_HEIGHT = 40.0f;
constexpr float PIANO_WIDTH = 80.0f;

// Toolbar Layout (Agent 1 Constants)
constexpr float TOOLBAR_BUTTON_START_X = 10.0f;
constexpr float TOOLBAR_BUTTON_WIDTH = 60.0f;
constexpr float TOOLBAR_BUTTON_HEIGHT = 30.0f;
constexpr float TOOLBAR_BUTTON_MARGIN = 5.0f;
constexpr int DEFAULT_PIANO_KEY_VELOCITY = 100;

//==============================================================================
// Constructor / Destructor
//==============================================================================

PianoRollComponent::PianoRollComponent(zenith::ProjectState &state)
    : projectState(state) {
  // Thread Safety: Constructor must be called from message thread
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  setWantsKeyboardFocus(true);
  setMouseCursor(juce::MouseCursor::NormalCursor);

  // Initialize visual resources
  using namespace zenith::design;
  rulerBarFont_ = typography::getMonoFont(11.0f, FontWeight::Bold);
  rulerBeatFont_ = typography::getMonoFont(9.0f, FontWeight::Regular);
  clipNameFont_ = typography::getSkFont(10.0f, FontWeight::Medium);

  textPaint_.setAntiAlias(true);
  textPaint_.setColor(colors::TEXT_PRIMARY);

  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setAntiAlias(true);

  generalPaint_.setAntiAlias(true);
}

PianoRollComponent::~PianoRollComponent() {
  if (currentClip.isValid()) {
    auto [track, clip] = projectState.findClip(currentClip.clipId);
    if (clip.isValid()) {
      auto midiNotesNode =
          clip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (midiNotesNode.isValid())
        midiNotesNode.removeListener(this);
    }
  }
}

//==============================================================================
// Clip Management
//==============================================================================

void PianoRollComponent::setClipContext(const MidiClipContext &context) {
  // Thread Safety: Clip context changes must happen on message thread
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (currentClip.isValid()) {
    auto [oldTrack, oldClip] = projectState.findClip(currentClip.clipId);
    if (oldClip.isValid()) {
      auto midiNotesNode =
          oldClip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (midiNotesNode.isValid())
        midiNotesNode.removeListener(this);
    }
  }

  currentClip = context;

  if (currentClip.isValid()) {
    auto [track, clip] = projectState.findClip(currentClip.clipId);
    if (clip.isValid()) {
      auto midiNotesNode =
          clip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (!midiNotesNode.isValid()) {
        midiNotesNode = juce::ValueTree(zenith::ProjectState::ID_NOTES);
        clip.appendChild(midiNotesNode, nullptr);
      }
      midiNotesNode.addListener(this);
    }
    refreshNotesFromProjectState();
  }
  repaint();
}

void PianoRollComponent::refreshNotesFromProjectState() {
  // Thread Safety: Note refresh must happen on message thread
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (!currentClip.isValid()) {
    noteRects.clear();
    repaint();
    return;
  }

  // OPTIMIZATION: Use unordered_set for O(1) lookups instead of O(N)
  std::unordered_set<juce::String> selectedIds;
  for (const auto &n : noteRects) {
    if (n.selected)
      selectedIds.insert(n.id);
  }

  auto notes = projectState.getMidiNotesForClip(currentClip.clipId);
  noteRects.clear();
  noteRects.reserve(notes.size());

  for (const auto &n : notes) {
    NoteRect nr;
    nr.id = n.id;
    nr.pitch = n.pitch;
    nr.startBeats = n.startBeats;
    nr.lengthBeats = n.lengthBeats;
    nr.velocity = n.velocity;
    nr.muted = n.muted;
    nr.probability = n.probability;
    nr.condition = n.condition;
    nr.recurrence = n.recurrence;
    nr.articulationId = n.articulationId;

    nr.selected = selectedIds.contains(n.id);
    noteRects.push_back(nr);
  }

  // Multi-Clip
  for (const auto &ctx : multiClipContexts) {
    if (!ctx.isValid() || ctx.clipId == currentClip.clipId)
      continue;
    auto notes = projectState.getMidiNotesForClip(ctx.clipId);
    for (const auto &n : notes) {
      NoteRect nr;
      nr.id = n.id;
      nr.ownerClipId = ctx.clipId;
      nr.pitch = n.pitch;
      nr.startBeats = n.startBeats;
      nr.lengthBeats = n.lengthBeats;
      nr.velocity = n.velocity;
      nr.muted = n.muted;
      nr.selected = false;
      noteRects.push_back(nr);
    }
  }

  std::sort(noteRects.begin(), noteRects.end(),
            [](const NoteRect &a, const NoteRect &b) {
              if (std::abs(a.startBeats - b.startBeats) > 0.001)
                return a.startBeats < b.startBeats;
              return a.pitch < b.pitch;
            });

  updateVisiblePitches();
  updateNoteRectangles();
  detectNoteCollisions();
  repaint();
}

void PianoRollComponent::updateNoteRectangles() {
  float contentTop = RULER_HEIGHT + TOOLBAR_HEIGHT;
  // noteGridHeight is cached

  for (auto &note : noteRects) {
    float x = PIANO_WIDTH + beatsToPixels(note.startBeats);
    float y = RULER_HEIGHT + pitchToPixels(note.pitch);
    float width = beatsToPixels(note.lengthBeats);
    float height = pixelsPerPitch;

    note.bounds = juce::Rectangle<float>(x, y, width, height);

    float velocityBarY =
        RULER_HEIGHT + noteGridHeight + velocityToPixels(note.velocity);
    float velocityBarHeight =
        (RULER_HEIGHT + noteGridHeight + velocityLaneHeight) - velocityBarY;
    note.velocityBounds =
        juce::Rectangle<float>(x, velocityBarY, width, velocityBarHeight);
  }
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

int PianoRollComponent::pixelsToPitch(float y) const {
  float adjustedY = y - RULER_HEIGHT;
  int row = static_cast<int>((adjustedY + scrollOffsetY) / pixelsPerPitch);

  if (foldMode) {
    if (visiblePitches.empty())
      return 60;
    int maxRow = (int)visiblePitches.size() - 1;
    int resultRow = maxRow - row;
    resultRow = juce::jlimit(0, maxRow, resultRow);
    return visiblePitches[resultRow];
  } else {
    int pitch = 127 - row;
    return juce::jlimit(0, 127, pitch);
  }
}

float PianoRollComponent::pitchToPixels(int pitch) const {
  int row;
  if (foldMode) {
    row = mapPitchToRow(pitch);
    if (!visiblePitches.empty())
      row = ((int)visiblePitches.size() - 1) - row;
  } else {
    row = 127 - pitch;
  }
  return row * pixelsPerPitch - scrollOffsetY;
}

double PianoRollComponent::pixelsToBeats(float x) const {
  float adjustedX = x - PIANO_WIDTH;
  return (adjustedX + scrollOffsetX) / pixelsPerBeat;
}

float PianoRollComponent::beatsToPixels(double beats) const {
  return static_cast<float>(beats * pixelsPerBeat - scrollOffsetX);
}

double PianoRollComponent::snapToGrid(double beats) const {
  if (!snapEnabled || gridBeats <= 0.0)
    return beats;
  return std::round(beats / gridBeats) * gridBeats;
}

int PianoRollComponent::pixelsToVelocity(float y) const {

  // noteGridHeight is cached

  float yInLane = y - (TOOLBAR_HEIGHT + RULER_HEIGHT + noteGridHeight);
  float normalizedY = yInLane / velocityLaneHeight;
  int velocity = static_cast<int>((1.0f - normalizedY) * 127.0f);
  return juce::jlimit(1, 127, velocity);
}

float PianoRollComponent::velocityToPixels(int velocity) const {
  float normalized = velocity / 127.0f;
  return (1.0f - normalized) * velocityLaneHeight;
}

//==============================================================================
// Mouse Interaction Helpers
//==============================================================================

PianoRollComponent::NoteRect *PianoRollComponent::findNoteAtPosition(float x,
                                                                     float y) {
  for (auto it = noteRects.rbegin(); it != noteRects.rend(); ++it) {
    if (it->bounds.contains(x, y))
      return &(*it);
  }
  return nullptr;
}

PianoRollComponent::NoteRect *
PianoRollComponent::findNoteInVelocityLane(float x, float y) {

  float contentTop = RULER_HEIGHT + TOOLBAR_HEIGHT;
  // noteGridHeight is cached
  float velocityLaneTop = contentTop + noteGridHeight;

  if (y < velocityLaneTop || y > velocityLaneTop + velocityLaneHeight)
    return nullptr;

  for (auto it = noteRects.rbegin(); it != noteRects.rend(); ++it) {
    if (it->velocityBounds.contains(x, y))
      return &(*it);
  }
  return nullptr;
}

PianoRollComponent::DragMode
PianoRollComponent::detectNoteHitRegion(const NoteRect &note, float x,
                                        float y) const {
  if (!note.bounds.contains(x, y))
    return DragMode::None;
  if (x < note.bounds.getX() + resizeHandleWidth)
    return DragMode::ResizeLeft;
  if (x > note.bounds.getRight() - resizeHandleWidth)
    return DragMode::ResizeRight;
  return DragMode::MoveNote;
}

PianoRollComponent::CursorType
PianoRollComponent::getCursorForPosition(float x, float y) const {
  if (x < PIANO_WIDTH || y < RULER_HEIGHT)
    return CursorType::Normal;

  // Tool-aware cursor logic
  if (currentTool == Tool::Draw || currentTool == Tool::Erase ||
      currentTool == Tool::Slice) {
    return CursorType::Crosshair;
  }

  float contentTop = RULER_HEIGHT + TOOLBAR_HEIGHT;
  // noteGridHeight is cached
  if (y >= contentTop + noteGridHeight)
    return CursorType::Crosshair;

  for (const auto &note : noteRects) {
    if (note.bounds.contains(x, y)) {
      if (x < note.bounds.getX() + resizeHandleWidth)
        return CursorType::ResizeLeft;
      if (x > note.bounds.getRight() - resizeHandleWidth)
        return CursorType::ResizeRight;
      return CursorType::Hand;
    }
  }
  return CursorType::Crosshair;
}

juce::MouseCursor PianoRollComponent::getMouseCursor() {
  switch (currentCursorType) {
  case CursorType::Hand:
    return juce::MouseCursor::DraggingHandCursor;
  case CursorType::ResizeHorizontal:
  case CursorType::ResizeLeft:
  case CursorType::ResizeRight:
    return juce::MouseCursor::LeftRightResizeCursor;
  case CursorType::Crosshair:
    return juce::MouseCursor::CrosshairCursor;
  default:
    return juce::MouseCursor::NormalCursor;
  }
}

//==============================================================================
// ValueTree Listeners
//==============================================================================

void PianoRollComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                             juce::ValueTree &child) {
  if (parent.hasType(zenith::ProjectState::ID_NOTES)) {
    refreshNotesFromProjectState();
  }
}

void PianoRollComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                               juce::ValueTree &child,
                                               int index) {
  if (parent.hasType(zenith::ProjectState::ID_NOTES)) {
    refreshNotesFromProjectState();
  }
}

void PianoRollComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  refreshNotesFromProjectState();
}

void PianoRollComponent::mouseMove(const juce::MouseEvent &e) {
  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);

  float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;
  // noteGridHeight is cached

  // Track hovered piano key
  int newHoveredKey = -1;
  if (x < PIANO_WIDTH && y >= RULER_HEIGHT &&
      y < RULER_HEIGHT + noteGridHeight) {
    newHoveredKey = pixelsToPitch(y);
    newHoveredKey = juce::jlimit(0, 127, newHoveredKey);
  }

  if (newHoveredKey != hoveredPianoKey) {
    hoveredPianoKey = newHoveredKey;
    repaint(0, 0, (int)PIANO_WIDTH, getHeight());
  }

  // Update cursor based on position
  auto newCursorType = getCursorForPosition(x, y);
  if (newCursorType != currentCursorType) {
    currentCursorType = newCursorType;
    repaint(); // Cursor changes might affect tooltips or global state, keep
               // simple for cursor
  }

  // Track hovered note
  auto *newHoveredNote = findNoteAtPosition(x, y);
  if (newHoveredNote != hoveredNote) {
    juce::Rectangle<float> dirtyRect;

    if (hoveredNote) {
      hoveredNote->isHovered = false;
      dirtyRect = dirtyRect.getUnion(hoveredNote->bounds);
    }
    hoveredNote = newHoveredNote;
    if (hoveredNote) {
      hoveredNote->isHovered = true;
      dirtyRect = dirtyRect.getUnion(hoveredNote->bounds);
    }

    if (!dirtyRect.isEmpty()) {
      // Expand slightly for strokes/shadows
      repaint(dirtyRect.expanded(2.0f).toNearestInt());
    }
  }
}

void PianoRollComponent::mouseDown(const juce::MouseEvent &e) {
  if (!currentClip.isValid())
    return;

  if (stepSequencerMode) {
    // Handle Step Sequencer clicks
    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);

    // Check if within grid area
    if (x >= PIANO_WIDTH && y >= TOOLBAR_HEIGHT + RULER_HEIGHT) {
      float noteAreaY = y - (TOOLBAR_HEIGHT + RULER_HEIGHT);
      float noteAreaX = x - PIANO_WIDTH;

      // Calculate step and row
      // Assuming grid fills the view for now, or use pixelsPerBeat
      double beat = pixelsToBeats(x);
      double stepSize = gridBeats;
      int step = static_cast<int>(beat / stepSize);

      int pitch = pixelsToPitch(y);
      int row = mapPitchToRow(pitch); // Use logic compatible with visual rows

      // Toggle step
      if (step >= 0 && pitch >= 0 && pitch < 128) {
        toggleStep(
            pitch,
            step); // Using pitch directly for now as row might depend on fold
      }
    }
    return;
  }

  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);
  float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;
  float velocityLaneTop = contentTop + noteGridHeight;

  // 1. Check Toolbar Clicks
  if (y < TOOLBAR_HEIGHT) {
    handleToolbarClick(e, x, y);
    return;
  }

  // 2. Check Piano Key Clicks
  if (x < PIANO_WIDTH && y >= contentTop) {
    if (y < contentTop + noteGridHeight) {
      handlePianoKeyClick(e, x, y);
      return;
    }
  }

  // Ruler area - not interactive for now
  if (y < RULER_HEIGHT)
    return;

  // Velocity lane interaction
  if (y >= velocityLaneTop) {
    handleVelocityLaneClick(e, x, y);
    return;
  }

  // Main note area - behavior depends on current tool
  if (x < PIANO_WIDTH)
    return;

  if (e.mods.isRightButtonDown()) {
    auto *noteUnderMouse = findNoteAtPosition(x, y);
    auto menu = ContextMenuManager::createMenu();
    
    if (noteUnderMouse) {
      if (!noteUnderMouse->selected) {
        clearSelection();
        noteUnderMouse->selected = true;
      }
      
      menu->addSectionHeader("Note Options");
      menu->addItem(1, "Mute / Unmute", true, noteUnderMouse->muted, [this, noteUnderMouse]() {
          bool newMuteState = !noteUnderMouse->muted;
          // Apply to all selected notes if noteUnderMouse is part of selection
          if (noteUnderMouse->selected) {
              for (const auto& note : noteRects) {
                  if (note.selected) {
                      projectState.setMidiNoteMuted(currentClip.clipId, note.id, newMuteState, "Mute Notes");
                  }
              }
          } else {
              // Otherwise just the one note
              projectState.setMidiNoteMuted(currentClip.clipId, noteUnderMouse->id, newMuteState, "Mute Note");
          }
      });
      menu->addSeparator();
      menu->addItem(2, "Quantize", true, false, [this]() {
          projectState.getUndoManager().beginNewTransaction("Quantize Selected");
          for (const auto& note : noteRects) {
              if (note.selected) {
                  double start = note.startBeats;
                  double quantized = std::round(start / gridBeats) * gridBeats;
                  quantized = std::max(0.0, quantized);
                  if (std::abs(quantized - start) > 0.001) {
                      projectState.moveMidiNote(currentClip.clipId, note.id, quantized, note.pitch, "Quantize Note");
                  }
              }
          }
      });
      menu->addItem(3, "Legato", true, false, [this]() { applyLegato(); });
      menu->addItem(4, "Humanize...", true, false, [this]() { 
          auto* w = new juce::AlertWindow("Humanize", "Adjust randomization parameters:", juce::AlertWindow::QuestionIcon);
          w->addTextEditor("velocity", "10", "Velocity Range (+/-):");
          w->addTextEditor("timing", "0.05", "Timing Range (beats):");
          w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
          w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
          
          w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int result) {
              if (result != 0) {
                  double velRange = w->getTextEditorContents("velocity").getDoubleValue();
                  double timeRange = w->getTextEditorContents("timing").getDoubleValue();
                  projectState.humanizeClip(currentClip.clipId, velRange, timeRange, "Humanize Selected");
              }
              delete w;
          }), true);
      });
      menu->addSeparator();
      menu->addItem(5, "Duplicate", true, false, [this]() {
            // Check if ANY selected, if not return
            if (getSelectedNoteCount() == 0) return;
            
            projectState.getUndoManager().beginNewTransaction("Duplicate Notes");
            // Find end of selection to shift
            // Simple duplication: shift by grid or selection length?
            // Standard behavior: shift by length of selection or grid.
            // Let's shift by grid for immediate feedback or just offset.
            // Actually, best might be to use clipboard logic, but for now simple offset:
            for (const auto& note : noteRects) {
                if (note.selected) {
                     zenith::ProjectState::MidiNoteSpec newNote;
                     newNote.id = juce::Uuid().toString();
                     newNote.pitch = note.pitch;
                     newNote.startBeats = note.startBeats + gridBeats; // Offset by grid
                     newNote.lengthBeats = note.lengthBeats;
                     newNote.velocity = 100; // note.velocity isn't in NoteRect, fetch from state? NoteRect has basic info.
                     // We need to fetch full spec or store velocity in NoteRect.
                     // NoteRect doesn't seem to have velocity.
                     // Let's fetch original note spec.
                     auto noteSpec = projectState.getMidiNotesForClip(currentClip.clipId);
                     // Find spec
                     for (const auto& spec : noteSpec) {
                         if (spec.id == note.id) {
                             newNote.velocity = spec.velocity;
                             newNote.muted = spec.muted;
                             break;
                         }
                     }
                     projectState.addMidiNote(currentClip.clipId, newNote, "Duplicate Note");
                }
            }
      });
      menu->addItemComplete(6, "Delete", SkPath{}, "Del", true, false, true, [this]() {
          deleteSelectedNotes();
      });
    } else {
      menu->addSectionHeader("Piano Roll");
      menu->addItem(1, "Select All", true, false, [this]() { selectAll(); });
      menu->addItem(2, "Deselect All", true, false, [this]() { clearSelection(); repaint(); });
      menu->addSeparator();
      menu->addItem(3, "Quantize All", true, false, [this]() {
           projectState.quantizeClip(currentClip.clipId, gridBeats, "Quantize All");
      });
      menu->addItem(4, "Humanize All", true, false, [this]() {
           projectState.humanizeClip(currentClip.clipId, 20.0, 0.05, "Humanize All");
      });
      menu->addSeparator();
      
      auto gridMenu = ContextMenuManager::createMenu();
      gridMenu->addItem(10, "1/4", true, gridBeats == 1.0, nullptr);
      gridMenu->addItem(11, "1/8", true, gridBeats == 0.5, nullptr);
      gridMenu->addItem(12, "1/16", true, gridBeats == 0.25, nullptr);
      gridMenu->addItem(13, "1/32", true, gridBeats == 0.125, nullptr);
      menu->addSubMenu("Grid Resolution", std::move(gridMenu));
    }
    
    ContextMenuManager::getInstance().showMenuAt(std::move(menu), this, e.x, e.y);
    return;
  }

  handleNoteMainAreaClick(e, x, y);
}

void PianoRollComponent::handleToolbarClick(const juce::MouseEvent &e, float x,
                                            float y) {

  float btnX = TOOLBAR_BUTTON_START_X;
  float btnY = (TOOLBAR_HEIGHT - TOOLBAR_BUTTON_HEIGHT) / 2.0f;

  Tool tools[] = {Tool::Select, Tool::Draw, Tool::Erase, Tool::Slice};
  for (const auto tool : tools) {
    if (x >= btnX && x < btnX + TOOLBAR_BUTTON_WIDTH && y >= btnY &&
        y < btnY + TOOLBAR_BUTTON_HEIGHT) {
      setCurrentTool(tool);
      return;
    }
    btnX += TOOLBAR_BUTTON_WIDTH + TOOLBAR_BUTTON_MARGIN;
  }
}

void PianoRollComponent::handlePianoKeyClick(const juce::MouseEvent &e, float x,
                                             float y) {
  int pitch = pixelsToPitch(y);
  pitch = juce::jlimit(0, 127, pitch);
  playPianoKey(pitch, DEFAULT_PIANO_KEY_VELOCITY);
}

void PianoRollComponent::handleVelocityLaneClick(const juce::MouseEvent &e,
                                                 float x, float y) {

  auto *note = findNoteInVelocityLane(x, y);
  if (note) {
    startEditingVelocity(note, e);
  }
}

void PianoRollComponent::handleNoteMainAreaClick(const juce::MouseEvent &e,
                                                 float x, float y) {

  auto *note = findNoteAtPosition(x, y);

  switch (currentTool) {
  case Tool::Select:
    if (note) {
      DragMode mode = detectNoteHitRegion(*note, x, y);
      if (mode == DragMode::ResizeLeft || mode == DragMode::ResizeRight) {
        startResizingNote(note, mode, e);
      } else {
        bool isMultiSelectModifier = e.mods.isCommandDown();
        if (isMultiSelectModifier) {
          note->selected = !note->selected;
          broadcastSelection();
          repaint();
        } else if (!note->selected) {
          clearSelection();
          note->selected = true;
          broadcastSelection();
          repaint();
        }
        startMovingSelection(e);
      }
    } else {
      if (e.mods.isShiftDown()) {
        startMarqueeSelect(e);
      } else {
        // In select mode, clicking empty space clears selection
        clearSelection();
        repaint();
      }
    }
    break;

  case Tool::Draw:
    if (!note) {
      createNoteAtPosition(x, y);
    } else {
      // Clicking existing note in Draw mode selects it
      clearSelection();
      note->selected = true;
      startMovingSelection(e);
    }
    break;

  case Tool::Erase:
    if (note) {
      projectState.removeMidiNote(currentClip.clipId, note->id,
                                  "Erase MIDI note");
    }
    break;

  case Tool::Slice:
    if (note) {
      // Slice note at cursor position
      // Robust Implementation
      juce::String noteId = note->id;
      juce::String clipId = currentClip.clipId;
      double originalStart = note->startBeats;
      double originalLength = note->lengthBeats;
      int notePitch = note->pitch;
      int noteVelocity = note->velocity;
      bool noteMuted = note->muted;

      double sliceBeat = pixelsToBeats(x - PIANO_WIDTH);
      if (snapEnabled)
        sliceBeat = snapToGrid(sliceBeat);

      // Only slice if position is within note bounds
      if (sliceBeat > originalStart &&
          sliceBeat < originalStart + originalLength) {
        double leftLength = sliceBeat - originalStart;
        double rightLength = originalLength - leftLength;

        // Create right part first
        zenith::ProjectState::MidiNoteSpec rightNote;
        rightNote.pitch = notePitch;
        rightNote.startBeats = sliceBeat;
        rightNote.lengthBeats = rightLength;
        rightNote.velocity = noteVelocity;
        rightNote.muted = noteMuted;

        projectState.getUndoManager().beginNewTransaction("Slice MIDI note");

        // Shorten original
        projectState.setMidiNoteLength(clipId, noteId, leftLength, "");
        // Add new
        projectState.addMidiNote(clipId, rightNote, "");
      }
    }
    break;
  }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent &e) {
  if (sprayCanMode && currentDragMode == DragMode::None) {
    handleSprayPaint(e.position.x, e.position.y);
    return;
  }

  switch (currentDragMode) {
  case DragMode::MoveNote:
    updateSelectionMove(e);
    break;
  case DragMode::ResizeLeft:
  case DragMode::ResizeRight:
    updateNoteResize(e);
    break;
  case DragMode::VelocityEdit:
    updateVelocityEdit(e);
    break;
  case DragMode::MarqueeSelect:
    updateMarqueeSelect(e);
    break;
  default:
    break;
  }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent &e) {
  switch (currentDragMode) {
  case DragMode::MoveNote:
    finishSelectionMove();
    break;
  case DragMode::ResizeLeft:
  case DragMode::ResizeRight:
    finishNoteResize();
    break;
  case DragMode::VelocityEdit:
    finishVelocityEdit();
    break;
  case DragMode::MarqueeSelect:
    finishMarqueeSelect();
    break;
  default:
    break;
  }
  currentDragMode = DragMode::None;
  activeNote = nullptr;
}

void PianoRollComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  if (!currentClip.isValid())
    return;
  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);
  auto *note = findNoteAtPosition(x, y);
  if (note) {
    projectState.removeMidiNote(currentClip.clipId, note->id,
                                "Delete MIDI note");
  }
}

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent &e,
                                        const juce::MouseWheelDetails &wheel) {
  if (e.mods.isCommandDown()) {
    float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
    zoomHorizontal(zoomFactor, static_cast<float>(e.x));
  } else if (e.mods.isAltDown()) {
    float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
    zoomVertical(zoomFactor, static_cast<float>(e.y));
  } else if (e.mods.isShiftDown()) {
    scrollHorizontal(-wheel.deltaY * 50.0f);
  } else {
    scrollVertical(-wheel.deltaY * 50.0f);
  }
}

//==============================================================================
// Selection Management
//==============================================================================

void PianoRollComponent::clearSelection() {
  for (auto &note : noteRects)
    note.selected = false;
  
  broadcastSelection();
}

void PianoRollComponent::selectNote(NoteRect *note, bool addToSelection) {
  if (!addToSelection)
    clearSelection();
  if (note)
    note->selected = true;
  
  broadcastSelection();
  repaint();
}

void PianoRollComponent::selectNotesInRectangle(
    const juce::Rectangle<float> &rect) {
  for (auto &note : noteRects) {
    if (rect.intersects(note.bounds))
      note.selected = true;
  }
  broadcastSelection();
  repaint();
}

void PianoRollComponent::selectAll() {
  for (auto &note : noteRects)
    note.selected = true;
  broadcastSelection();
  repaint();
}

void PianoRollComponent::invertSelection() {
  for (auto &note : noteRects)
    note.selected = !note.selected;
  broadcastSelection();
  repaint();
}

void PianoRollComponent::broadcastSelection() {
    juce::StringArray selectedIds;
    for (const auto& note : noteRects) {
        if (note.selected)
            selectedIds.add(note.id);
    }
    CollaborationManager::getInstance().broadcastSelection(selectedIds);
}

int PianoRollComponent::getSelectedNoteCount() const {
  return static_cast<int>(
      std::count_if(noteRects.begin(), noteRects.end(),
                    [](const NoteRect &n) { return n.selected; }));
}

//==============================================================================
// Note Editing Operations
//==============================================================================

void PianoRollComponent::createNoteAtPosition(float x, float y) {
  int pitch = pixelsToPitch(y);
  double startBeats = pixelsToBeats(x - PIANO_WIDTH);
  if (snapEnabled)
    startBeats = snapToGrid(startBeats);
  startBeats = juce::jmax(0.0, startBeats);
  pitch = juce::jlimit(0, 127, pitch);
  double lengthBeats = gridBeats;

  zenith::ProjectState::MidiNoteSpec note;
  note.id = juce::Uuid().toString();
  note.pitch = pitch;
  note.startBeats = startBeats;
  note.lengthBeats = lengthBeats;
  note.velocity = 100;
  note.muted = false;

  projectState.addMidiNote(currentClip.clipId, note, "Create MIDI note");

  // Interaction Polish: Immediately select the new note
  // addMidiNote triggers listeners synchronously, so noteRects should be
  // updated.
  for (auto &n : noteRects) {
    if (n.id == note.id) {
      selectNote(&n, false);
      break;
    }
  }
}

void PianoRollComponent::deleteSelectedNotes() {
  if (!currentClip.isValid() || getSelectedNoteCount() == 0)
    return;
  std::vector<juce::String> selectedIds;
  for (const auto &note : noteRects) {
    if (note.selected)
      selectedIds.push_back(note.id);
  }
  projectState.getUndoManager().beginNewTransaction("Delete MIDI notes");
  for (const auto &noteId : selectedIds) {
    projectState.removeMidiNote(currentClip.clipId, noteId, "");
  }
}

//==============================================================================
// Copy/Paste/Cut
//==============================================================================

void PianoRollComponent::copySelectedNotes() {
  clipboard.clear();
  if (getSelectedNoteCount() == 0)
    return;
  double earliestTime = std::numeric_limits<double>::max();
  for (const auto &note : noteRects) {
    if (note.selected)
      earliestTime = std::min(earliestTime, note.startBeats);
  }
  clipboardReferenceTime = earliestTime;
  for (const auto &note : noteRects) {
    if (note.selected) {
      ClipboardNote clipNote;
      clipNote.pitch = note.pitch;
      clipNote.startBeats = note.startBeats - earliestTime;
      clipNote.lengthBeats = note.lengthBeats;
      clipNote.velocity = note.velocity;
      clipNote.muted = note.muted;
      clipboard.push_back(clipNote);
    }
  }
}

void PianoRollComponent::pasteNotes() {
  if (!currentClip.isValid() || clipboard.empty())
    return;
  double pasteTime = viewStartBeats;
  projectState.getUndoManager().beginNewTransaction("Paste MIDI notes");
  clearSelection();
  for (const auto &clipNote : clipboard) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = clipNote.pitch;
    note.startBeats = pasteTime + clipNote.startBeats;
    note.lengthBeats = clipNote.lengthBeats;
    note.velocity = clipNote.velocity;
    note.muted = clipNote.muted;
    projectState.addMidiNote(currentClip.clipId, note, "");
  }
}

void PianoRollComponent::cutSelectedNotes() {
  copySelectedNotes();
  deleteSelectedNotes();
}

//==============================================================================
// Drag Operations
//==============================================================================

void PianoRollComponent::startMovingSelection(const juce::MouseEvent &e) {
  currentDragMode = DragMode::MoveNote;
  dragStartPos = e.position;
  dragStates.clear();
  for (const auto &note : noteRects) {
    if (note.selected) {
      NoteDragState state;
      state.id = note.id;
      state.originalPitch = note.pitch;
      state.originalStartBeats = note.startBeats;
      dragStates.push_back(state);
    }
  }
}

void PianoRollComponent::updateSelectionMove(const juce::MouseEvent &e) {
  if (dragStates.empty())
    return;
  float deltaX = e.position.x - dragStartPos.x;
  float deltaY = e.position.y - dragStartPos.y;
  double deltaBeats =
      pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);
  int deltaPitch = -static_cast<int>(deltaY / pixelsPerPitch);

  size_t stateIndex = 0;
  for (auto &note : noteRects) {
    if (note.selected && stateIndex < dragStates.size()) {
      const auto &originalState = dragStates[stateIndex];
      double newStartBeats = originalState.originalStartBeats + deltaBeats;
      int newPitch = originalState.originalPitch + deltaPitch;
      if (snapEnabled)
        newStartBeats = snapToGrid(newStartBeats);
      newStartBeats = juce::jmax(0.0, newStartBeats);
      newPitch = juce::jlimit(0, 127, newPitch);
      note.startBeats = newStartBeats;
      note.pitch = newPitch;
      ++stateIndex;
    }
  }
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishSelectionMove() {
  if (dragStates.empty())
    return;
  projectState.getUndoManager().beginNewTransaction("Move MIDI notes");
  size_t stateIndex = 0;
  for (auto &note : noteRects) {
    if (note.selected && stateIndex < dragStates.size()) {
      const auto &originalState = dragStates[stateIndex];
      bool pitchChanged = note.pitch != originalState.originalPitch;
      bool startChanged =
          std::abs(note.startBeats - originalState.originalStartBeats) > 0.001;
      if (pitchChanged || startChanged) {
        projectState.moveMidiNote(currentClip.clipId, note.id, note.startBeats,
                                  note.pitch, "");
      }
      ++stateIndex;
    }
  }
  dragStates.clear();
}

void PianoRollComponent::startResizingNote(NoteRect *note, DragMode mode,
                                           const juce::MouseEvent &e) {
  currentDragMode = mode;
  activeNote = note;
  dragStartPos = e.position;
  dragStates.clear();
  NoteDragState state;
  state.id = note->id;
  state.originalStartBeats = note->startBeats;
  state.originalLengthBeats = note->lengthBeats;
  dragStates.push_back(state);
  clearSelection();
  note->selected = true;
  repaint();
}

void PianoRollComponent::updateNoteResize(const juce::MouseEvent &e) {
  if (!activeNote || dragStates.empty())
    return;
  float deltaX = e.position.x - dragStartPos.x;
  double deltaBeats =
      pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);
  const auto &originalState = dragStates[0];

  if (currentDragMode == DragMode::ResizeLeft) {
    double newStartBeats = originalState.originalStartBeats + deltaBeats;
    if (snapEnabled)
      newStartBeats = snapToGrid(newStartBeats);
    newStartBeats = juce::jmax(0.0, newStartBeats);
    double newLengthBeats = originalState.originalLengthBeats -
                            (newStartBeats - originalState.originalStartBeats);
    newLengthBeats = juce::jmax(0.01, newLengthBeats);
    activeNote->startBeats = newStartBeats;
    activeNote->lengthBeats = newLengthBeats;
  } else if (currentDragMode == DragMode::ResizeRight) {
    double newLengthBeats = originalState.originalLengthBeats + deltaBeats;
    if (snapEnabled) {
      double endBeats =
          snapToGrid(originalState.originalStartBeats + newLengthBeats);
      newLengthBeats = endBeats - originalState.originalStartBeats;
    }
    newLengthBeats = juce::jmax(0.01, newLengthBeats);
    activeNote->lengthBeats = newLengthBeats;
  }
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishNoteResize() {
  if (!activeNote || dragStates.empty())
    return;
  const auto &originalState = dragStates[0];
  bool startChanged = std::abs(activeNote->startBeats -
                               originalState.originalStartBeats) > 0.001;
  bool lengthChanged = std::abs(activeNote->lengthBeats -
                                originalState.originalLengthBeats) > 0.001;
  if (startChanged || lengthChanged) {
    if (currentDragMode == DragMode::ResizeLeft) {
      projectState.getUndoManager().beginNewTransaction("Resize MIDI note");
      projectState.moveMidiNote(currentClip.clipId, activeNote->id,
                                activeNote->startBeats, activeNote->pitch, "");
      projectState.setMidiNoteLength(currentClip.clipId, activeNote->id,
                                     activeNote->lengthBeats, "");
    } else {
      projectState.setMidiNoteLength(currentClip.clipId, activeNote->id,
                                     activeNote->lengthBeats,
                                     "Resize MIDI note");
    }
  }
  dragStates.clear();
}

void PianoRollComponent::startEditingVelocity(NoteRect *note,
                                              const juce::MouseEvent &e) {
  currentDragMode = DragMode::VelocityEdit;
  activeNote = note;
  dragStartPos = e.position;
  dragStates.clear();
  NoteDragState state;
  state.id = note->id;
  state.originalVelocity = note->velocity;
  dragStates.push_back(state);
  clearSelection();
  note->selected = true;
  repaint();
}

void PianoRollComponent::updateVelocityEdit(const juce::MouseEvent &e) {
  if (!activeNote || dragStates.empty())
    return;
  int newVelocity = pixelsToVelocity(static_cast<float>(e.y));
  activeNote->velocity = newVelocity;
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishVelocityEdit() {
  if (!activeNote || dragStates.empty())
    return;
  const auto &originalState = dragStates[0];
  if (activeNote->velocity != originalState.originalVelocity) {
    projectState.setMidiNoteVelocity(currentClip.clipId, activeNote->id,
                                     activeNote->velocity,
                                     "Edit MIDI velocity");
  }
  dragStates.clear();
}

void PianoRollComponent::startMarqueeSelect(const juce::MouseEvent &e) {
  currentDragMode = DragMode::MarqueeSelect;
  dragStartPos = e.position;
  marqueeRect =
      juce::Rectangle<float>(dragStartPos.x, dragStartPos.y, 0.0f, 0.0f);
  if (!e.mods.isCommandDown())
    clearSelection();
  repaint();
}

void PianoRollComponent::updateMarqueeSelect(const juce::MouseEvent &e) {
  marqueeRect = juce::Rectangle<float>::leftTopRightBottom(
      juce::jmin(dragStartPos.x, e.position.x),
      juce::jmin(dragStartPos.y, e.position.y),
      juce::jmax(dragStartPos.x, e.position.x),
      juce::jmax(dragStartPos.y, e.position.y));
  repaint();
}

void PianoRollComponent::finishMarqueeSelect() {
  selectNotesInRectangle(marqueeRect);
  marqueeRect = juce::Rectangle<float>();
  repaint();
}

//==============================================================================
// Zoom & Scroll
//==============================================================================

void PianoRollComponent::zoomHorizontal(float factor, float centerX) {
  double centerBeats = pixelsToBeats(centerX);
  pixelsPerBeat *= factor;
  pixelsPerBeat = juce::jlimit(10.0, 800.0, pixelsPerBeat);
  viewStartBeats = centerBeats - ((centerX - PIANO_WIDTH) / pixelsPerBeat);
  viewStartBeats = juce::jmax(0.0, viewStartBeats);
  scrollOffsetX = static_cast<int>(viewStartBeats * pixelsPerBeat);
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::zoomVertical(float factor, float centerY) {
  pixelsPerPitch *= factor;
  pixelsPerPitch = juce::jlimit(4.0, 96.0, pixelsPerPitch);
  scrollOffsetY = static_cast<int>(viewLowestPitch * pixelsPerPitch);
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::scrollHorizontal(float delta) {
  scrollOffsetX += static_cast<int>(delta);
  scrollOffsetX = juce::jmax(0, scrollOffsetX);
  viewStartBeats = scrollOffsetX / pixelsPerBeat;
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::scrollVertical(float delta) {
  scrollOffsetY += static_cast<int>(delta);
  scrollOffsetY =
      juce::jlimit(0, 127 * static_cast<int>(pixelsPerPitch), scrollOffsetY);
  viewLowestPitch = scrollOffsetY / static_cast<int>(pixelsPerPitch);
  updateNoteRectangles();
  repaint();
}

//==============================================================================
// Keyboard Shortcuts
//==============================================================================

bool PianoRollComponent::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
    deleteSelectedNotes();
    return true;
  }
  if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
    projectState.undo();
    return true;
  }
  if (key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0)) {
    projectState.redo();
    return true;
  }
  if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0)) {
    copySelectedNotes();
    return true;
  }
  if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0)) {
    pasteNotes();
    return true;
  }
  if (key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0)) {
    cutSelectedNotes();
    return true;
  }
  if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)) {
    selectAll();
    return true;
  }
  if (key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0)) {
    smartDuplicate();
    return true;
  }
  if (key == juce::KeyPress('q', 0, 0)) {
    quantizeSelected(gridBeats, 1.0f, 0.0f);
    return true;
  }

  // ... (keep simplified, delegates handle complex stuff if needed)

  return false;
}

void PianoRollComponent::resized() {
  auto bounds = getLocalBounds();
  float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;
  noteGridHeight = bounds.getHeight() - contentTop - velocityLaneHeight;
  updateNoteRectangles();
}

void PianoRollComponent::quantizeSelected(double grid, float strength,
                                          float swing) {
  if (grid <= 0.0)
    return;

  // Start undo transaction
  projectState.getUndoManager().beginNewTransaction("Quantize");

  bool anyChanged = false;

  for (auto &note : noteRects) {
    if (!note.selected)
      continue;

    double originalStart = note.startBeats;

    // Simple quantization
    double quantizedStart = std::round(originalStart / grid) * grid;

    // Apply strength (0.0 to 1.0)
    double newStart =
        originalStart + (quantizedStart - originalStart) * strength;

    // Note: Swing logic omitted for brevity

    if (std::abs(newStart - originalStart) > 0.0001) {
      projectState.moveMidiNote(currentClip.clipId, note.id, newStart,
                                note.pitch, "");
      anyChanged = true;
    }
  }

  if (anyChanged)
    repaint();
}

void PianoRollComponent::quantizeSelected(const QuantizeOptions& options) {
  double grid = options.gridSize;
  if (grid <= 0.0) {
    grid = gridBeats; // Use current grid if not specified
  }
  
  // Apply triplet adjustment if enabled
  if (options.useTriplets) {
    grid = grid * 2.0 / 3.0; // Convert to triplet grid
  }
  
  // Delegate to the simpler version
  quantizeSelected(grid, options.strength, options.swingAmount);
}

void PianoRollComponent::drawSkia(SkCanvas *canvas) {
  if (!canvas)
    return;
  using namespace zenith::design;

  auto localBounds = getLocalBounds();
  float width = (float)localBounds.getWidth();
  float height = (float)localBounds.getHeight();

  // Background (Glassmorphic)
  SkRect bgBounds = SkRect::MakeWH(width, height);
  GlassmorphicPanel::draw(canvas, bgBounds, GlassmorphicPanel::Style::Subtle);

  // generalPaint_ is used later
  generalPaint_.setColor(colors::BG_DARKER);
  generalPaint_.setStyle(SkPaint::kFill_Style);

  // Alias for legacy code
  SkPaint &paint = generalPaint_;
  float notesHeight = height - RULER_HEIGHT - velocityLaneHeight;

  // 1. Piano Keys Area Background
  SkRect pianoRect =
      SkRect::MakeXYWH(0, RULER_HEIGHT, PIANO_WIDTH, notesHeight);
  canvas->drawRect(pianoRect, generalPaint_);

  //==========================================================================
  // PROFESSIONAL TIMELINE RULER (Ableton/Logic style)
  //==========================================================================
  {
    SkRect rulerRect = SkRect::MakeXYWH(0, 0, width, RULER_HEIGHT);

    // Ruler background with gradient
    SkPoint gradPts[] = {{0, 0}, {0, RULER_HEIGHT}};
    SkColor gradColors[] = {colors::BG_DARK, colors::BG_DARKER};
    generalPaint_.setShader(SkGradientShader::MakeLinear(
        gradPts, gradColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(rulerRect, generalPaint_);
    generalPaint_.setShader(nullptr);

    // Bottom border
    borderPaint_.setColor(colors::BORDER_DEFAULT);
    borderPaint_.setStrokeWidth(1.0f);
    canvas->drawLine(0, RULER_HEIGHT - 1, width, RULER_HEIGHT - 1,
                     borderPaint_);

    // Calculate visible beat range
    double visibleStartBeat = viewStartBeats;
    double visibleEndBeat = pixelsToBeats(width);

    // Determine grid density based on zoom
    double beatsPerBar = 4.0; // Assume 4/4 time
    double barStep = 1.0;     // Every bar
    double beatSubdiv = 1.0;  // Beat subdivision

    if (pixelsPerBeat < 10.0) {
      barStep = 4.0; // Every 4 bars
      beatSubdiv = 4.0;
    } else if (pixelsPerBeat < 30.0) {
      barStep = 1.0; // Every bar
      beatSubdiv = 1.0;
    } else if (pixelsPerBeat < 80.0) {
      beatSubdiv = 0.5; // Half beats
    } else {
      beatSubdiv = 0.25; // Sixteenth notes
    }

    // Draw bar numbers and markers
    // Fonts are now members: rulerBarFont_, rulerBeatFont_

    double startBar = std::floor(visibleStartBeat / beatsPerBar) * beatsPerBar;

    for (double beat = startBar; beat <= visibleEndBeat; beat += beatSubdiv) {
      float x = PIANO_WIDTH + beatsToPixels(beat);
      if (x < PIANO_WIDTH)
        continue;

      int barNum = static_cast<int>(beat / beatsPerBar) + 1;
      double beatInBar = std::fmod(beat, beatsPerBar);
      bool isBarStart = std::abs(beatInBar) < 0.001;
      bool isDownbeat = std::fmod(beat, 1.0) < 0.001;

      if (isBarStart) {
        // Bar marker - tall line + number
        generalPaint_.setColor(colors::TEXT_SECONDARY);
        generalPaint_.setStrokeWidth(1.5f);
        canvas->drawLine(x, 4, x, RULER_HEIGHT - 4, generalPaint_);

        // Bar number with subtle glow
        textPaint_.setColor(colors::TEXT_PRIMARY);
        juce::String barStr = juce::String(barNum);
        canvas->drawString(barStr.toStdString().c_str(), x + 4, 18,
                           rulerBarFont_, textPaint_);

      } else if (isDownbeat && pixelsPerBeat >= 30.0) {
        // Beat marker - medium line
        generalPaint_.setColor(colors::BORDER_SUBTLE);
        generalPaint_.setStrokeWidth(1.0f);
        canvas->drawLine(x, RULER_HEIGHT - 12, x, RULER_HEIGHT - 4,
                         generalPaint_);

        // Beat number (1.2, 1.3, etc)
        if (pixelsPerBeat >= 50.0) {
          textPaint_.setColor(colors::TEXT_TERTIARY);
          int beatInBarNum = static_cast<int>(beatInBar) + 1;
          juce::String label =
              juce::String(barNum) + "." + juce::String(beatInBarNum);
          canvas->drawString(label.toStdString().c_str(), x + 2,
                             RULER_HEIGHT - 6, rulerBeatFont_, textPaint_);
        }
      } else if (pixelsPerBeat >= 80.0) {
        // Subdivision tick - short line
        generalPaint_.setColor(SkColorSetARGB(60, 255, 255, 255));
        generalPaint_.setStrokeWidth(0.5f);
        canvas->drawLine(x, RULER_HEIGHT - 6, x, RULER_HEIGHT - 2,
                         generalPaint_);
      }
    }

    // Clip name badge (top-left)
    if (currentClip.isValid()) {
      SkRect badge = SkRect::MakeXYWH(
          4, 4, juce::jmin(150.0f, static_cast<float>(PIANO_WIDTH - 8)), 22);
      generalPaint_.setColor(withAlpha(colors::VIOLET, 0.3f));
      canvas->drawRoundRect(badge, 4, 4, generalPaint_);

      // Border glow
      borderPaint_.setColor(withAlpha(colors::VIOLET, 0.6f));
      borderPaint_.setStrokeWidth(1.0f);
      canvas->drawRoundRect(badge, 4, 4, borderPaint_);

      // Clip name
      textPaint_.setColor(colors::TEXT_PRIMARY);
      // font is clipNameFont_

      juce::String clipName =
          currentClip.clipName.isEmpty() ? "MIDI Clip" : currentClip.clipName;
      if (clipName.length() > 18)
        clipName = clipName.substring(0, 17) + "...";
      canvas->drawString(clipName.toStdString().c_str(), 10, 19, clipNameFont_,
                         textPaint_);
    }
  }

  // 2. Grid Lines (Vertical) - Added for "Real" feel
  canvas->save();
  SkRect noteAreaRect = SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT,
                                         width - PIANO_WIDTH, notesHeight);
  canvas->clipRect(noteAreaRect);

  paint.setColor(colors::BORDER_SUBTLE);
  paint.setStrokeWidth(1.0f);
  // Draw vertical lines for beats
  // We iterate visible beat range
  double startBeat = std::floor(pixelsToBeats(PIANO_WIDTH));
  double endBeat = pixelsToBeats(width);

  // Optimization: Don't draw too many lines if zoomed out
  double beatStep = (pixelsPerBeat < 15.0) ? 4.0 : 1.0;

  for (double b = startBeat; b <= endBeat; b += beatStep) {
    float x = PIANO_WIDTH + beatsToPixels(b);
    if (x >= PIANO_WIDTH) {
      canvas->drawLine(x, RULER_HEIGHT, x, RULER_HEIGHT + notesHeight, paint);
    }
  }
  canvas->restore();

  // 3. Draw Keys and Horizontal Grid Lines
  int topPitch = pixelsToPitch(RULER_HEIGHT);
  int bottomPitch = pixelsToPitch(RULER_HEIGHT + notesHeight);

  topPitch = juce::jlimit(0, 127, topPitch);
  bottomPitch = juce::jlimit(0, 127, bottomPitch);

  for (int p = bottomPitch; p <= topPitch; ++p) {
    float y = pitchToPixels(p) + RULER_HEIGHT;
    float h = pixelsPerPitch;

    if (y < RULER_HEIGHT - h || y >= RULER_HEIGHT + notesHeight)
      continue;

    int noteInOctave = p % 12;
    bool black = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                  noteInOctave == 8 || noteInOctave == 10);

    // Draw Key
    static constexpr float kKeyLabelMinZoom = 12.0f;
    static constexpr float kKeyLabelDetailZoom = 18.0f;
    static constexpr float kKeyLabelMaxFontSize = 11.0f;
    static constexpr float kKeyLabelDetailMaxFontSize = 9.0f;
    static constexpr float kKeyLabelOffset = -24.0f;
    static constexpr float kKeyLabelDetailOffset = -18.0f;

    SkRect keyRect = SkRect::MakeXYWH(0, y, PIANO_WIDTH, h);
    paint.setStyle(SkPaint::kFill_Style);

    // Check if this key is hovered or playing
    bool isHovered = (p == hoveredPianoKey);
    bool isPlaying = (p == playingPianoKey);

    if (black) {
      // Black key
      if (isPlaying) {
        paint.setColor(colors::MAGENTA);
      } else if (isHovered) {
        paint.setColor(colors::BG_MEDIUM);
      } else {
        paint.setColor(colors::BG_DARKEST);
      }
      canvas->drawRect(keyRect, paint);
    } else {
      // White key (actually light grey)
      if (isPlaying) {
        paint.setColor(colors::CYAN);
      } else if (isHovered) {
        paint.setColor(colors::TEXT_PRIMARY);
      } else {
        paint.setColor(colors::TEXT_SECONDARY); // #A1A1AA
      }
      canvas->drawRect(keyRect, paint);

      // Shadow for depth
      SkPaint shadow;
      shadow.setColor(design::withAlpha(colors::BG_DARKEST, 0.25f));
      canvas->drawRect(SkRect::MakeXYWH(0, y + h - 1, PIANO_WIDTH, 1), shadow);
    }

    // Key Label
    if (pixelsPerPitch > kKeyLabelMinZoom) {
      static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                        "F#", "G",  "G#", "A",  "A#", "B"};
      SkPaint textPaint;
      textPaint.setAntiAlias(true);

      if (noteInOctave == 0) {
        // C notes get octave number
        textPaint.setColor(black ? colors::TEXT_SECONDARY : colors::BG_DARKEST);
        SkFont font = getMonoFont(
            juce::jmin(kKeyLabelMaxFontSize, (float)(pixelsPerPitch * 0.7f)),
            FontWeight::Bold);
        juce::String label = "C" + juce::String(p / 12 - 2);
        canvas->drawString(label.toStdString().c_str(),
                           PIANO_WIDTH + kKeyLabelOffset, y + h * 0.7f, font,
                           textPaint);
      } else if (pixelsPerPitch > kKeyLabelDetailZoom) {
        textPaint.setColor(colors::TEXT_TERTIARY);
        SkFont font = getMonoFont(juce::jmin(kKeyLabelDetailMaxFontSize,
                                             (float)(pixelsPerPitch * 0.5f)),
                                  FontWeight::Regular);
        canvas->drawString(noteNames[noteInOctave],
                           PIANO_WIDTH + kKeyLabelDetailOffset, y + h * 0.7f,
                           font, textPaint);
      }
    }

    // Horizontal Grid Line
    paint.setColor(colors::BORDER_SUBTLE);
    paint.setStrokeWidth(1.0f);
    canvas->drawLine(PIANO_WIDTH, y + h, width, y + h, paint);
  }

  // 4. Draw Step Sequencer or Notes
  SkRect notesRect = SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT,
                                      width - PIANO_WIDTH, notesHeight);

  if (stepSequencerMode) {
    drawStepSequencer(canvas, notesRect);
  } else {
    drawNotes(canvas, notesRect);
  }

  // 5. Ghost Notes
  if (ghostNotesEnabled && !stepSequencerMode) {
    drawGhostNotes(canvas, notesRect);
  }

  // 5.5 Arpeggiator Preview
  if (arpPreviewEnabled && !stepSequencerMode) {
    drawArpPreview(canvas, notesRect);
  }

  // 6. Playhead
  drawPlayhead(canvas, notesRect);

  // 5. Velocity Lane Background
  SkRect velocityRect =
      SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT + notesHeight,
                       width - PIANO_WIDTH, velocityLaneHeight);
  paint.setColor(colors::BG_DARK);
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRect(velocityRect, paint);

  // Velocity Bars - Draw vertical bars for each note
  canvas->save();
  canvas->clipRect(velocityRect);

  // Separator line
  paint.setColor(colors::BORDER_DEFAULT);
  canvas->drawLine(0, RULER_HEIGHT + notesHeight, width,
                   RULER_HEIGHT + notesHeight, paint);

  float velocityLaneTop = RULER_HEIGHT + notesHeight;
  float velocityLaneBottom = velocityLaneTop + velocityLaneHeight;

  for (const auto &note : noteRects) {
    // Skip notes outside visible horizontal range
    if (note.bounds.getX() > width || note.bounds.getRight() < PIANO_WIDTH)
      continue;

    float barWidth = std::max(2.0f, std::min(8.0f, note.bounds.getWidth()));
    float barX =
        note.bounds.getX() + (note.bounds.getWidth() - barWidth) * 0.5f;

    // Height based on velocity (0-127 maps to 0 to velocityLaneHeight)
    float normalizedVelocity = note.velocity / 127.0f;
    float barHeight = normalizedVelocity * (velocityLaneHeight - 4.0f);
    float barY = velocityLaneBottom - barHeight - 2.0f;

    // Color - selected uses CYAN, unselected uses VIOLET with velocity
    // intensity
    SkColor barColor;
    if (note.selected) {
      barColor = colors::CYAN;
    } else {
      barColor = interpolateColor(darken(colors::VIOLET, 0.2f), colors::VIOLET,
                                  normalizedVelocity);
    }

    // Draw velocity bar
    SkRect barRect = SkRect::MakeXYWH(barX, barY, barWidth, barHeight);
    SkRRect barRR = SkRRect::MakeRectXY(barRect, 1.0f, 1.0f);

    paint.setColor(barColor);
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRRect(barRR, paint);

    // Glow for selected
    if (note.selected) {
      SkPaint glowPaint;
      glowPaint.setColor(colors::CYAN);
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 2.0f));
      canvas->drawRRect(barRR, glowPaint);
    }
  }
  canvas->restore();

  // 6. Playhead - follows transport
  {
    float playheadX = PIANO_WIDTH + beatsToPixels(currentPlayheadBeats);
    if (playheadX >= PIANO_WIDTH && playheadX <= width) {
      // Playhead line
      SkPaint playheadPaint;
      playheadPaint.setColor(colors::TEXT_PRIMARY);
      playheadPaint.setStrokeWidth(2.0f);
      playheadPaint.setAntiAlias(true);
      canvas->drawLine(playheadX, RULER_HEIGHT, playheadX, height,
                       playheadPaint);

      // Triangle marker in ruler/toolbar area
      static constexpr float kPlayheadMarkerHalfWidth = 5.0f;
      static constexpr float kPlayheadMarkerHeight = 8.0f;

      const float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;
      SkPath trianglePath;
      trianglePath.moveTo(playheadX, contentTop);
      trianglePath.lineTo(playheadX - kPlayheadMarkerHalfWidth,
                          contentTop - kPlayheadMarkerHeight);
      trianglePath.lineTo(playheadX + kPlayheadMarkerHalfWidth,
                          contentTop - kPlayheadMarkerHeight);
      trianglePath.close();
      canvas->drawPath(trianglePath, playheadPaint);
    }
  }

  // 7. Marquee Selection
  if (!marqueeRect.isEmpty()) {
    SkRect m =
        SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                         marqueeRect.getWidth(), marqueeRect.getHeight());
    paint.setColor(withAlpha(colors::CYAN, 0.2f));
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(m, paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(colors::CYAN);
    canvas->drawRect(m, paint);
  }
}

void PianoRollComponent::smartDuplicate() {
  copySelectedNotes();
  if (clipboard.empty())
    return;

  double start = std::numeric_limits<double>::max();
  double end = std::numeric_limits<double>::lowest();
  bool hasSelection = false;

  for (const auto &note : noteRects) {
    if (note.selected) {
      hasSelection = true;
      start = std::min(start, note.startBeats);
      end = std::max(end, note.startBeats + note.lengthBeats);
    }
  }

  if (!hasSelection)
    return;

  double length = end - start;
  if (length < 0.001)
    length = gridBeats;

  projectState.getUndoManager().beginNewTransaction("Smart Duplicate");
  clearSelection();

  for (const auto &clipNote : clipboard) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = clipNote.pitch;
    note.startBeats = start + length + clipNote.startBeats;
    note.lengthBeats = clipNote.lengthBeats;
    note.velocity = clipNote.velocity;
    note.muted = clipNote.muted;

    projectState.addMidiNote(currentClip.clipId, note, "");
  }
}

void PianoRollComponent::timerCallback() { SkiaComponent::timerCallback(); }

void PianoRollComponent::handleSprayPaint(float x, float y) {
  createNoteAtPosition(x, y);
}

void PianoRollComponent::detectNoteCollisions() {
  // Reset collision flags
  for (auto &note : noteRects) {
    note.hasCollision = false;
  }

  // O(n²) collision detection - acceptable for typical note counts
  for (size_t i = 0; i < noteRects.size(); ++i) {
    for (size_t j = i + 1; j < noteRects.size(); ++j) {
      auto &a = noteRects[i];
      auto &b = noteRects[j];

      // Same pitch and overlapping time?
      if (a.pitch == b.pitch) {
        double aEnd = a.startBeats + a.lengthBeats;
        double bEnd = b.startBeats + b.lengthBeats;

        // Check for time overlap
        if (a.startBeats < bEnd && aEnd > b.startBeats) {
          a.hasCollision = true;
          b.hasCollision = true;
        }
      }
    }
  }
}

void PianoRollComponent::updateVisiblePitches() {
  visiblePitches.clear();

  if (!foldMode) {
    // Normal mode: show all 128 pitches
    for (int i = 0; i < 128; ++i)
      visiblePitches.push_back(i);
  } else {
    // Fold mode: only show pitches that have notes
    std::set<int> usedPitches;
    for (const auto &note : noteRects) {
      usedPitches.insert(note.pitch);
    }

    // Also include ghost notes if enabled
    for (const auto &ghost : ghostNotes) {
      usedPitches.insert(ghost.pitch);
    }

    // Convert to sorted vector (high to low for display)
    for (int p : usedPitches) {
      visiblePitches.push_back(p);
    }

    // Sort descending (highest pitch at top)
    std::sort(visiblePitches.begin(), visiblePitches.end(),
              std::greater<int>());

    // If no notes, show default range (C3-C5)
    if (visiblePitches.empty()) {
      for (int i = 72; i >= 48; --i)
        visiblePitches.push_back(i);
    }
  }
}

int PianoRollComponent::mapPitchToRow(int pitch) const {
  if (!foldMode)
    return 127 - pitch; // Normal: just invert for top-to-bottom

  // Fold mode: find pitch in visible list
  for (size_t i = 0; i < visiblePitches.size(); ++i) {
    if (visiblePitches[i] == pitch)
      return static_cast<int>(i);
  }
  return -1; // Not visible
}

int PianoRollComponent::mapRowToPitch(int row) const {
  if (!foldMode)
    return 127 - row; // Normal: invert back

  // Fold mode: lookup pitch from row
  if (row >= 0 && row < static_cast<int>(visiblePitches.size()))
    return visiblePitches[row];
  return -1;
}

//==============================================================================
// Tool System
//==============================================================================

void PianoRollComponent::setCurrentTool(Tool tool) {
  currentTool = tool;

  // Update cursor based on tool
  switch (tool) {
  case Tool::Select:
    setMouseCursor(juce::MouseCursor::NormalCursor);
    break;
  case Tool::Draw:
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    break;
  case Tool::Erase:
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    break;
  case Tool::Slice:
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    break;
  }
  repaint();
}

//==============================================================================
// Piano Key Interaction
//==============================================================================

void PianoRollComponent::playPianoKey(int pitch, int velocity) {
  pitch = juce::jlimit(0, 127, pitch);
  velocity = juce::jlimit(1, 127, velocity);

  playingPianoKey = pitch;

  // Trigger note preview callback if set
  if (notePreviewCallback) {
    notePreviewCallback(pitch, velocity, true);
  }

  repaint();
}

void PianoRollComponent::stopPianoKey(int pitch) {
  if (playingPianoKey == pitch) {
    if (notePreviewCallback) {
      notePreviewCallback(pitch, 0, false);
    }
    playingPianoKey = -1;
    repaint();
  }
}

//==============================================================================
// Skia Helpers
//==============================================================================

juce::Colour PianoRollComponent::getColorForVelocity(int velocity) const {
  // Clamp velocity to valid MIDI range
  velocity = juce::jlimit(0, 127, velocity);

  // Generate a color gradient from blue (soft) to red (loud)
  // Low velocity: cooler colors (blue/purple)
  // High velocity: warmer colors (orange/red)
  float normalized = velocity / 127.0f;

  // Use HSL color space for smooth gradient
  // Hue: 240 (blue) -> 0 (red) as velocity increases
  float hue = (1.0f - normalized) * 0.66f; // From blue to red
  float saturation =
      0.7f + normalized * 0.3f; // More saturated at high velocity
  float brightness = 0.6f + normalized * 0.4f; // Brighter at high velocity

  return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
}

SkColor PianoRollComponent::getSkiaColorForVelocity(int velocity) const {
  juce::Colour c = getColorForVelocity(velocity);
  return SkColorSetARGB(c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue());
}

//==============================================================================
// Rendering Modules
//==============================================================================

void PianoRollComponent::drawModernToolbar(SkCanvas *canvas,
                                           const SkRect &fullRect) {
  using namespace zenith::design;

  SkRect toolbarRect = SkRect::MakeXYWH(0, 0, fullRect.width(), TOOLBAR_HEIGHT);

  // Glassmorphism Background
  SkPaint bgPaint;
  bgPaint.setColor(colors::BG_DARK);
  canvas->drawRect(toolbarRect, bgPaint);

  SkPaint border;
  border.setColor(colors::BORDER_SUBTLE);
  canvas->drawLine(0, TOOLBAR_HEIGHT - 1, fullRect.width(), TOOLBAR_HEIGHT - 1,
                   border);

  // Tools
  float x = 10;
  const char *toolNames[] = {"SEL", "DRW", "ERS", "CUT"};
  Tool tools[] = {Tool::Select, Tool::Draw, Tool::Erase, Tool::Slice};

  for (int i = 0; i < 4; ++i) {
    SkRect btnRect = SkRect::MakeXYWH(x, 6, 40, 28);
    SkPaint btnPaint;
    bool isActive = (currentTool == tools[i]);

    if (isActive)
      btnPaint.setColor(colors::CYAN);
    else
      btnPaint.setColor(colors::BG_LIGHT);

    canvas->drawRoundRect(btnRect, 4, 4, btnPaint);

    SkPaint textPaint;
    textPaint.setColor(isActive ? colors::BG_DARKEST : colors::TEXT_PRIMARY);
    SkFont font = typography::getMonoFont(10, FontWeight::Bold);
    canvas->drawString(toolNames[i], x + 8, 24, font, textPaint);

    x += 46;
  }

// In drawModernToolbar:

  // Scale Lock
  x += 20;
  SkRect lockBtn = SkRect::MakeXYWH(x, 6, 80, 28);
  SkPaint lockPaint;
  lockPaint.setColor(scaleHighlight.enabled ? colors::VIOLET
                                            : colors::BG_LIGHT);
  canvas->drawRoundRect(lockBtn, 4, 4, lockPaint);

  SkPaint lockText;
  lockText.setColor(colors::TEXT_PRIMARY);
  SkFont font = typography::getSkFont(10, FontWeight::Bold);
  juce::String label = scaleHighlight.enabled ? "SCALE ON" : "SCALE OFF";
  canvas->drawString(label.toStdString().c_str(), x + 8, 24, font, lockText);
  
  // Ghost Notes
  x += 90;
  SkRect ghostBtn = SkRect::MakeXYWH(x, 6, 60, 28);
  SkPaint ghostPaint;
  ghostPaint.setColor(ghostNotesEnabled ? colors::ORANGE : colors::BG_LIGHT);
  canvas->drawRoundRect(ghostBtn, 4, 4, ghostPaint);
  
  juce::String ghostLabel = "GHOST";
  canvas->drawString(ghostLabel.toStdString().c_str(), x + 8, 24, font, lockText);
  
  // Humanize
  x += 70;
  SkRect humBtn = SkRect::MakeXYWH(x, 6, 40, 28);
  SkPaint humPaint;
  humPaint.setColor(colors::BG_LIGHT); // Action button, usually simple
  canvas->drawRoundRect(humBtn, 4, 4, humPaint);
  canvas->drawString("HUM", x + 8, 24, font, lockText);
  
  // Expression
  x += 50;
  SkRect exprBtn = SkRect::MakeXYWH(x, 6, 40, 28);
  // Highlight if any lane visible?
  bool exprVisible = false; 
  for(bool v : expressionLaneVisible) if(v) exprVisible = true;
  
  SkPaint exprPaint;
  exprPaint.setColor(exprVisible ? colors::CYAN_DARK : colors::BG_LIGHT);
  canvas->drawRoundRect(exprBtn, 4, 4, exprPaint);
  canvas->drawString("EXP", x + 8, 24, font, lockText);

}
// ... (rest of file)

void PianoRollComponent::humanizeTiming(float amount) {
     projectState.humanizeClip(currentClip.clipId, 0.0, (double)amount, "Humanize Timing");
}

void PianoRollComponent::applyLegato() {
    // Default to adjusting overlap
    projectState.legatoClip(currentClip.clipId, true, "Legato");
}

void PianoRollComponent::humanizeVelocity(float amount) {
    // Map 0.0-1.0 to 0-40 velocity range
    double range = amount * 40.0;
    projectState.humanizeClip(currentClip.clipId, range, 0.0, "Humanize Velocity");
}

void PianoRollComponent::setNoteTension(const juce::String& noteId, float tension) {
    projectState.setMidiNoteTension(currentClip.clipId, noteId, tension, "Set Note Tension");
}

void PianoRollComponent::drawPianoKeys(SkCanvas *canvas, const SkRect &area) {
  using namespace zenith::design;

  SkPaint bg;
  bg.setColor(colors::BG_DARKER);
  canvas->drawRect(area, bg);

  for (int p = 0; p < 128; ++p) {
    float y = area.top() + pitchToPixels(p);
    float h = pixelsPerPitch;

    if (y > area.bottom() || y + h < area.top())
      continue;

    int noteInOctave = p % 12;
    bool black = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                  noteInOctave == 8 || noteInOctave == 10);

    SkRect keyRect = SkRect::MakeXYWH(area.left(), y, area.width(), h);
    SkPaint keyPaint;
    keyPaint.setStyle(SkPaint::kFill_Style);

    bool isRoot =
        scaleHighlight.enabled && (noteInOctave == scaleHighlight.rootNote);
    bool inScale = isNoteInScale(p);
    bool isHovered = (p == hoveredPianoKey);
    bool isPlaying = (p == playingPianoKey);

    if (black) {
      if (isPlaying)
        keyPaint.setColor(colors::MAGENTA);
      else if (isHovered)
        keyPaint.setColor(colors::BG_MEDIUM);
      else if (scaleHighlight.enabled && !inScale)
        keyPaint.setColor(darken(colors::BG_DARKEST, 0.5f));
      else if (isRoot)
        keyPaint.setColor(withAlpha(colors::VIOLET, 0.3f));
      else
        keyPaint.setColor(colors::BG_DARKEST);
    } else {
      if (isPlaying)
        keyPaint.setColor(colors::CYAN);
      else if (isHovered)
        keyPaint.setColor(colors::TEXT_PRIMARY);
      else if (scaleHighlight.enabled && !inScale)
        keyPaint.setColor(darken(colors::TEXT_SECONDARY, 0.3f));
      else if (isRoot)
        keyPaint.setColor(withAlpha(colors::VIOLET, 0.2f));
      else
        keyPaint.setColor(colors::TEXT_SECONDARY);
    }

    canvas->drawRect(keyRect, keyPaint);

    SkPaint linePaint;
    linePaint.setColor(colors::BG_DARKEST);
    canvas->drawLine(area.left(), y + h, area.right(), y + h, linePaint);
  }
}

void PianoRollComponent::drawGrid(SkCanvas *canvas, const SkRect &area) {
  using namespace zenith::design;
  SkPaint vLine;
  vLine.setColor(colors::BORDER_SUBTLE);
  vLine.setStrokeWidth(1.0f);

  double minBeat = pixelsToBeats(0);
  double maxBeat = pixelsToBeats(area.width());

  for (double b = std::floor(minBeat); b <= maxBeat; b += 1.0) {
    float x = area.left() + beatsToPixels(b);
    canvas->drawLine(x, area.top(), x, area.bottom(), vLine);
  }
}

void PianoRollComponent::drawNotes(SkCanvas *canvas, const SkRect &area) {
  using namespace zenith::design;

  SkPaint selectedGlowPaint;
  selectedGlowPaint.setColor(colors::CYAN);
  selectedGlowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(SkBlurStyle::kSolid_SkBlurStyle, 4.0f));

  SkPaint collisionGlowPaint;
  collisionGlowPaint.setColor(colors::AMBER);
  collisionGlowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 3.0f));

  for (const auto &note : noteRects) {
    float x = area.left() + beatsToPixels(note.startBeats);
    float w = beatsToPixels(note.lengthBeats);
    float y = area.top() + pitchToPixels(note.pitch);
    float h = pixelsPerPitch;

    if (x > area.right() || x + w < area.left() || y > area.bottom() ||
        y + h < area.top())
      continue;

    SkRect r = SkRect::MakeXYWH(x, y, w, h);
    SkRect inner = r.makeInset(1.0f, 1.0f);
    SkRRect rr = SkRRect::MakeRectXY(inner, 3.0f, 3.0f);

    // COLLISION WARNING: Amber glow for overlapping notes
    if (note.hasCollision && !note.selected) {
      SkRect collisionRect = rr.rect().makeOutset(3.0f, 3.0f);
      canvas->drawRect(collisionRect, collisionGlowPaint);
    }

    // Color based on selection, mute, and velocity
    SkColor noteColor;
    float alpha = note.muted ? 0.4f : 1.0f; // Dim muted notes

    if (note.selected) {
      // Glow for selected notes
      SkRect outsetRect = rr.rect().makeOutset(2.0f, 2.0f);
      canvas->drawRect(outsetRect, selectedGlowPaint);
      noteColor = colors::CYAN;
    } else if (note.hasCollision) {
      // Collision: tinted amber
      noteColor = interpolateColor(colors::AMBER, colors::VIOLET, 0.4f);
    } else {
      // Normal: VIOLET with velocity intensity
      float velocityFactor = note.velocity / 127.0f;
      noteColor =
          interpolateColor(darken(colors::VIOLET, 0.3f),
                           lighten(colors::VIOLET, 0.15f), velocityFactor);
    }

    // Apply mute dimming
    if (note.muted) {
      noteColor = withAlpha(noteColor, 0.35f);
    }

    SkPaint p;
    p.setColor(noteColor);

    // Gradient for note depth
    SkPoint pts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
    SkColor nColors[2] = {lighten(noteColor, 0.12f), darken(noteColor, 0.08f)};
    p.setShader(SkGradientShader::MakeLinear(pts, nColors, nullptr, 2,
                                             SkTileMode::kClamp));

    canvas->drawRRect(rr, p);
    p.setShader(nullptr);

    // Velocity indicator stripe at top (like Ableton)
    if (r.height() > 6.0f && r.width() > 10.0f) {
      float stripeHeight = 2.0f;
      SkRect stripe = SkRect::MakeXYWH(inner.left() + 1, inner.top() + 1,
                                       inner.width() - 2, stripeHeight);
      SkPaint stripePaint;
      stripePaint.setColor(withAlpha(colors::TEXT_PRIMARY,
                                     0.3f + (note.velocity / 127.0f) * 0.4f));
      stripePaint.setAntiAlias(true);
      canvas->drawRect(stripe, stripePaint);
    }

    // Border
    SkPaint border;
    border.setStyle(SkPaint::kStroke_Style);
    border.setAntiAlias(true);
    if (note.hasCollision) {
      border.setColor(design::withAlpha(colors::AMBER, 0.8f));
      border.setStrokeWidth(1.5f);
    } else {
      border.setColor(colors::BORDER_DEFAULT);
      border.setStrokeWidth(1.0f);
    }
    canvas->drawRRect(rr, border);

    // Hover highlight
    if (note.isHovered && !note.selected) {
      SkPaint hoverPaint;
      hoverPaint.setStyle(SkPaint::kStroke_Style);
      hoverPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.6f));
      hoverPaint.setStrokeWidth(1.5f);
      hoverPaint.setAntiAlias(true);
      canvas->drawRRect(rr, hoverPaint);
    }

    // PROBABILITY INDICATOR (dice icon position - bottom right)
    if (note.probability < 0.99f && r.width() > 20.0f && r.height() > 12.0f) {
      float probSize = juce::jmin(r.height() - 4, 10.0f);
      float probWidth = probSize * note.probability;
      SkRect probBg = SkRect::MakeXYWH(r.right() - probSize - 3, r.bottom() - 6,
                                       probSize, 3);
      SkRect probFill = SkRect::MakeXYWH(r.right() - probSize - 3,
                                         r.bottom() - 6, probWidth, 3);

      SkPaint probBgPaint;
      probBgPaint.setColor(design::withAlpha(colors::BG_DARKEST, 0.4f));
      canvas->drawRoundRect(probBg, 1, 1, probBgPaint);

      SkPaint probFillPaint;
      probFillPaint.setColor(colors::AMBER);
      canvas->drawRoundRect(probFill, 1, 1, probFillPaint);
    }

    // MUTED INDICATOR (diagonal stripes or "M")
    if (note.muted && r.width() > 14.0f) {
      SkPaint mutePaint;
      mutePaint.setColor(withAlpha(colors::TEXT_TERTIARY, 0.5f));
      mutePaint.setAntiAlias(true);
      SkFont muteFont = typography::getMonoFont(8.0f, FontWeight::Bold);
      canvas->drawString("M", r.left() + 3, r.bottom() - 3, muteFont,
                         mutePaint);
    }
  }
}

void PianoRollComponent::drawVelocityLane(SkCanvas *canvas,
                                          const SkRect &area) {
  using namespace zenith::design;

  SkPaint bg;
  bg.setColor(colors::BG_DARK);
  canvas->drawRect(area, bg);

  SkPaint line;
  line.setColor(colors::BORDER_DEFAULT);
  canvas->drawLine(area.left(), area.top(), area.right(), area.top(), line);

  // Label
  SkFont labelFont = typography::getMonoFont(10.0f);
  SkPaint labelPaint;
  labelPaint.setColor(withAlpha(colors::TEXT_SECONDARY, 0.7f));
  canvas->drawString("VELOCITY", area.left() + 5, area.top() + 12, labelFont,
                     labelPaint);

  for (const auto &note : noteRects) {
    float x = area.left() + beatsToPixels(note.startBeats);
    if (x > area.right())
      continue;

    float h = (note.velocity / 127.0f) * area.height();
    SkRect bar = SkRect::MakeXYWH(x, area.bottom() - h, 4, h);

    SkPaint p;
    p.setColor(note.selected ? colors::CYAN : withAlpha(colors::VIOLET, 0.7f));
    canvas->drawRect(bar, p);

    // Draw value if selected
    if (note.selected) {
      SkPaint textP;
      textP.setColor(colors::TEXT_PRIMARY);
      canvas->drawString(std::to_string(note.velocity).c_str(), x,
                         area.bottom() - h - 2, labelFont, textP);
    }
  }
}

//==============================================================================
// Expression Lanes Implementation
//==============================================================================

void PianoRollComponent::setExpressionLaneVisible(ExpressionType type,
                                                  bool visible) {
  int index = static_cast<int>(type);
  if (index >= 0 && index < 4) {
    expressionLaneVisible[index] = visible;
    resized(); // Re-layout
    repaint();
  }
}

bool PianoRollComponent::getExpressionLaneVisible(ExpressionType type) const {
  int index = static_cast<int>(type);
  if (index >= 0 && index < 4) {
    return expressionLaneVisible[index];
  }
  return false;
}

void PianoRollComponent::setNoteExpression(
    const juce::String &noteId, ExpressionType type,
    const std::vector<ExpressionPoint> &points) {
  noteExpressions[noteId][type] = points;
  repaint();
}

std::vector<PianoRollComponent::ExpressionPoint>
PianoRollComponent::getNoteExpression(const juce::String &noteId,
                                      ExpressionType type) const {
  auto noteIt = noteExpressions.find(noteId);
  if (noteIt != noteExpressions.end()) {
    auto typeIt = noteIt->second.find(type);
    if (typeIt != noteIt->second.end()) {
      return typeIt->second;
    }
  }
  return {};
}

void PianoRollComponent::drawExpressionLanes(SkCanvas *canvas,
                                             const SkRect &area) {
  using namespace zenith::design;

  // Background
  SkPaint bg;
  bg.setColor(colors::BG_DARK);
  canvas->drawRect(area, bg);

  float currentY = area.top();
  int visibleLanes = 0;

  for (int i = 0; i < 4; ++i) {
    if (expressionLaneVisible[i]) {
      ExpressionType type = static_cast<ExpressionType>(i);
      SkRect laneRect =
          SkRect::MakeXYWH(area.left(), currentY, area.width(),
                           (float)expressionLaneHeight);

      // Separator
      SkPaint line;
      line.setColor(colors::BORDER_DEFAULT);
      canvas->drawLine(laneRect.left(), laneRect.top(), laneRect.right(),
                       laneRect.top(), line);

      // Label
      juce::String label;
      switch (type) {
      case ExpressionType::PitchBend: label = "PITCH"; break;
      case ExpressionType::Pressure: label = "PRESSURE"; break;
      case ExpressionType::Slide: label = "SLIDE (MPE)"; break;
      case ExpressionType::Expression: label = "EXPRESSION"; break;
      }

      SkFont labelFont = typography::getMonoFont(10.0f);
      SkPaint labelPaint;
      labelPaint.setColor(withAlpha(colors::TEXT_SECONDARY, 0.7f));
      canvas->drawString(label.toStdString().c_str(), laneRect.left() + 5,
                         laneRect.top() + 12, labelFont, labelPaint);

      // Draw Curves for Selected Notes
      bool hasSelection = false;
      for (const auto &note : noteRects) {
        if (note.selected) {
          hasSelection = true;
          auto points = getNoteExpression(note.id, type);
          if (points.empty())
            continue;

          SkPath path;
          bool first = true;
          float prevX = 0.0f, prevY = 0.0f;
          
          float noteStartPixel = beatsToPixels(note.startBeats);
          
          for (size_t idx = 0; idx < points.size(); ++idx) {
            const auto &pt = points[idx];
            float x = area.left() + noteStartPixel + beatsToPixels(pt.timeOffset);
            // laneRect.bottom() is 0.0 value, laneRect.top() is 1.0 value
            float y = laneRect.bottom() - (pt.value * laneRect.height());

            if (first) {
              path.moveTo(x, y);
              first = false;
            } else {
              // Bezier curve based on tension
              // tension: +1 = curve up/right, -1 = curve down/left, 0 = linear
              float tension = points[idx - 1].tension;
              if (std::abs(tension) < 0.01f) {
                // Linear segment
                path.lineTo(x, y);
              } else {
                // Quadratic Bezier with control point influenced by tension
                float midX = (prevX + x) * 0.5f;
                float midY = (prevY + y) * 0.5f;
                // Tension pulls the control point up (positive) or down (negative)
                float controlY = midY - (tension * laneRect.height() * 0.25f);
                path.quadTo(midX, controlY, x, y);
              }
            }
            prevX = x;
            prevY = y;
            
            // Draw point
            SkPaint pointPaint;
            pointPaint.setColor(colors::ACCENT_SECONDARY);
            canvas->drawCircle(x, y, 3.0f, pointPaint);
          }
          
          SkPaint curvePaint;
          curvePaint.setStyle(SkPaint::kStroke_Style);
          curvePaint.setColor(colors::ACCENT_PRIMARY);
          curvePaint.setStrokeWidth(1.5f);
          curvePaint.setAntiAlias(true);
          canvas->drawPath(path, curvePaint);
        }
      }
      
      if (!hasSelection) {
          SkPaint hintPaint;
          hintPaint.setColor(withAlpha(colors::TEXT_TERTIARY, 0.4f));
          canvas->drawString("(Select notes to view expression)", laneRect.left() + 100,
                             laneRect.centerY() + 4, labelFont, hintPaint);
      }

      currentY += expressionLaneHeight;
      visibleLanes++;
    }
  }
}

//==============================================================================
// Probability & Humanization Implementation
//==============================================================================

void PianoRollComponent::setNoteProbability(float probability) {
  probability = juce::jlimit(0.0f, 1.0f, probability);
  bool changed = false;
  
  for (auto &note : noteRects) {
    if (note.selected) {
      note.probability = probability;
      noteProbabilities[note.id] = probability;
      projectState.setMidiNoteProbability(currentClip.clipId, note.id, probability, "Set Note Probability");
      changed = true;
    }
  }
  if (changed) repaint();
}

float PianoRollComponent::getNoteProbability(const juce::String &noteId) const {
    auto it = noteProbabilities.find(noteId);
    if (it != noteProbabilities.end()) return it->second;
    return 1.0f;
}

void PianoRollComponent::randomizeProbabilities(float minProb, float maxProb) {
    juce::Random random;
    bool changed = false;
    for (auto &note : noteRects) {
        if (note.selected) {
            float p = minProb + random.nextFloat() * (maxProb - minProb);
            setNoteProbability(p); // This calls projectState + repaint inefficiently, but works
            changed = true;
        }
    }
    if (changed) repaint();
}

// Redundant definition removed (already defined around line 2030)

void PianoRollComponent::drawChordName(SkCanvas *canvas) {
  using namespace zenith::design;
  juce::String chord = getCurrentChordName();
  if (chord.isEmpty())
    return;

  float cx = getLocalBounds().getWidth() / 2.0f;
  float cy = TOOLBAR_HEIGHT + 20.0f;

  SkFont font = typography::getDisplayFont(16.0f);
  SkPaint textP;
  textP.setColor(colors::TEXT_PRIMARY);
  textP.setAntiAlias(true);

  float textW = font.measureText(chord.toStdString().c_str(), chord.length(),
                                 SkTextEncoding::kUTF8);

  SkRect pill = SkRect::MakeXYWH(cx - textW / 2 - 10, cy - 12, textW + 20, 24);
  SkPaint pillBg;
  pillBg.setColor(withAlpha(colors::BG_DARKEST, 0.8f));
  canvas->drawRoundRect(pill, 12, 12, pillBg);

  SkPaint border;
  border.setStyle(SkPaint::kStroke_Style);
  border.setColor(withAlpha(colors::CYAN, 0.5f));
  canvas->drawRoundRect(pill, 12, 12, border);

  canvas->drawString(chord.toStdString().c_str(), cx - textW / 2, cy + 6, font,
                     textP);
}

//==============================================================================
// Step Sequencer Implementation
//==============================================================================

void PianoRollComponent::setStepSequencerMode(bool enabled) {
  stepSequencerMode = enabled;
  if (enabled) {
    currentTool = Tool::Select; // Force select tool or a simpler pointer
    // Maybe adjust zoom to fit steps?
    syncStepSequencerToNotes();
  }
  repaint();
}

void PianoRollComponent::setStepSequencerRows(const std::vector<int> &pitches) {
  stepSequencerRows = pitches;
  repaint();
}

void PianoRollComponent::toggleStep(int pitch, int step) {
  // Calculate precise beat time
  double stepSize = gridBeats;
  double startBeat = step * stepSize;

  // Check if note exists at this step/pitch
  bool exists = false;
  for (const auto &note : noteRects) {
    if (note.pitch == pitch && std::abs(note.startBeats - startBeat) < 0.01) {
      // Found - remove it
      projectState.removeMidiNote(currentClip.clipId, note.id,
                                  "Toggle Step (Remove)");
      exists = true;
      break;
    }
  }

  if (!exists) {
    // Add note
    zenith::ProjectState::MidiNoteSpec note;
    note.id = juce::Uuid().toString();
    note.pitch = pitch;
    note.startBeats = startBeat;
    note.lengthBeats = stepSize; // Step length
    note.velocity = 100;
    note.muted = false;

    projectState.addMidiNote(currentClip.clipId, note, "Toggle Step (Add)");
  }
  // syncStepSequencerToNotes called via listener callback
}

bool PianoRollComponent::getStep(int pitch, int step) const {
  double stepSize = gridBeats;
  double startBeat = step * stepSize;

  for (const auto &note : noteRects) {
    if (note.pitch == pitch && std::abs(note.startBeats - startBeat) < 0.01) {
      return true;
    }
  }
  return false;
}

void PianoRollComponent::syncStepSequencerToNotes() {
  // In this unified implementation, we query notes directly.
  // If we had a separate grid cache, we'd update it here.
  // For now, getStep() dynamic query is fine for reasonable clip sizes.
}

void PianoRollComponent::syncNotesToStepSequencer() {
  // Not needed as we update ProjectState directly in toggleStep
}

void PianoRollComponent::drawStepSequencer(SkCanvas *canvas,
                                           const SkRect &area) {
  using namespace zenith::design;

  // Clip to area
  canvas->save();
  canvas->clipRect(area);

  float stepWidth = beatsToPixels(gridBeats);

  double startBeat = std::floor(pixelsToBeats(PIANO_WIDTH));
  double endBeat = pixelsToBeats(area.right() + PIANO_WIDTH); // Approx

  int topPitch = pixelsToPitch(RULER_HEIGHT);
  int bottomPitch = pixelsToPitch(RULER_HEIGHT + area.height());

  // Clamp
  topPitch = juce::jlimit(0, 127, topPitch);
  bottomPitch = juce::jlimit(0, 127, bottomPitch);

  SkPaint stepPaint;
  stepPaint.setAntiAlias(true);

  // Draw Grid Cells
  for (int p = bottomPitch; p <= topPitch; ++p) {
    float y = pitchToPixels(p) + RULER_HEIGHT;
    float h = pixelsPerPitch;

    // Row background (alternating?)
    // Already drawn by main grid loop in drawSkia

    // Iterate steps in view
    int startStep = static_cast<int>(startBeat / gridBeats);
    int endStep = static_cast<int>(endBeat / gridBeats) + 1;

    for (int s = startStep; s < endStep; ++s) {
      double beat = s * gridBeats;
      float x = PIANO_WIDTH + beatsToPixels(beat);

      SkRect cell = SkRect::MakeXYWH(x + 1, y + 1, stepWidth - 2, h - 2);

      bool active = getStep(p, s);

      if (active) {
        stepPaint.setColor(colors::ACCENT_PRIMARY);
        stepPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawRoundRect(cell, 2.0f, 2.0f, stepPaint);

        // Inner gloss/detail
        stepPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
        canvas->drawRoundRect(cell.makeInset(2, 2), 1.0f, 1.0f, stepPaint);
      } else {
        // Empty step visualization (subtle)
        stepPaint.setColor(SkColorSetARGB(10, 255, 255, 255));
        stepPaint.setStyle(SkPaint::kFill_Style);
        canvas->drawRoundRect(cell, 2.0f, 2.0f, stepPaint);
      }
    }
  }

  canvas->restore();
}

void PianoRollComponent::updateScaleHighlight() {
  // Update internal toggle state or parameters if needed
}

//==============================================================================
// Chord Intervals Check
//==============================================================================

std::vector<int> PianoRollComponent::getChordIntervals(ChordType type) {
  switch (type) {
  case ChordType::Major: return {0, 4, 7};
  case ChordType::Minor: return {0, 3, 7};
  case ChordType::Diminished: return {0, 3, 6};
  case ChordType::Augmented: return {0, 4, 8};
  case ChordType::Major7: return {0, 4, 7, 11};
  case ChordType::Minor7: return {0, 3, 7, 10};
  case ChordType::Dominant7: return {0, 4, 7, 10};
  case ChordType::Diminished7: return {0, 3, 6, 9};
  case ChordType::Sus2: return {0, 2, 7};
  case ChordType::Sus4: return {0, 5, 7};
  case ChordType::Add9: return {0, 4, 7, 14};
  case ChordType::Minor9: return {0, 3, 7, 10, 14};
  case ChordType::Power: return {0, 7};
  case ChordType::Sixth: return {0, 4, 7, 9};
  case ChordType::Minor6: return {0, 3, 7, 9};
  default: return {0, 4, 7};
  }
}

void PianoRollComponent::insertChord(int rootPitch, ChordType type,
                                     double startBeat, double lengthBeats,
                                     int velocity) {
  if (!currentClip.isValid())
    return;

  projectState.getUndoManager().beginNewTransaction("Insert Chord");

  auto intervals = getChordIntervals(type);
  for (int interval : intervals) {
    int pitch = rootPitch + interval;
    if (pitch > 127) continue;

    zenith::ProjectState::MidiNoteSpec note;
    note.id = juce::Uuid().toString();
    note.pitch = pitch;
    note.startBeats = startBeat;
    note.lengthBeats = lengthBeats;
    note.velocity = velocity;
    note.muted = false;

    projectState.addMidiNote(currentClip.clipId, note, "");
  }
  repaint();
}

bool PianoRollComponent::isNoteInScale(int pitch) const {
  if (!scaleHighlight.enabled)
    return true;
  
  int note = pitch % 12;
  int root = scaleHighlight.rootNote % 12;

  // Normalized note relative to root
  int degree = (note - root + 12) % 12;

  // Scale Intervals Table
  // (0 = root, always in scale)
  static const std::map<ScaleType, std::vector<int>> SCALE_INTERVALS = {
    {ScaleType::Chromatic,      {0,1,2,3,4,5,6,7,8,9,10,11}},
    {ScaleType::Major,          {0, 2, 4, 5, 7, 9, 11}},
    {ScaleType::Minor,          {0, 2, 3, 5, 7, 8, 10}}, // Natural Minor
    {ScaleType::HarmonicMinor,  {0, 2, 3, 5, 7, 8, 11}},
    {ScaleType::MelodicMinor,   {0, 2, 3, 5, 7, 9, 11}},
    {ScaleType::Dorian,         {0, 2, 3, 5, 7, 9, 10}},
    {ScaleType::Phrygian,       {0, 1, 3, 5, 7, 8, 10}},
    {ScaleType::Lydian,         {0, 2, 4, 6, 7, 9, 11}},
    {ScaleType::Mixolydian,     {0, 2, 4, 5, 7, 9, 10}},
    {ScaleType::Aeolian,        {0, 2, 3, 5, 7, 8, 10}}, // Same as Minor
    {ScaleType::Locrian,        {0, 1, 3, 5, 6, 8, 10}},
    {ScaleType::MajorPentatonic,{0, 2, 4, 7, 9}},
    {ScaleType::MinorPentatonic,{0, 3, 5, 7, 10}},
    {ScaleType::MajorBlues,     {0, 2, 3, 4, 7, 9}},
    {ScaleType::MinorBlues,     {0, 3, 5, 6, 7, 10}},
    {ScaleType::WholeTone,      {0, 2, 4, 6, 8, 10}},
    {ScaleType::Augmented,      {0, 3, 4, 7, 8, 11}},
    {ScaleType::DiminishedHalfWhole, {0, 1, 3, 4, 6, 7, 9, 10}},
    {ScaleType::DiminishedWholeHalf, {0, 2, 3, 5, 6, 8, 9, 11}},
    {ScaleType::HungarianMinor, {0, 2, 3, 6, 7, 8, 11}},
    {ScaleType::HungarianMajor, {0, 3, 4, 6, 7, 9, 10}},
    {ScaleType::Bhairav,        {0, 1, 4, 5, 7, 8, 11}},
    {ScaleType::Byzantine,      {0, 1, 4, 5, 7, 8, 11}}, // Same as Bhairav
    {ScaleType::Persian,        {0, 1, 4, 5, 6, 8, 11}},
    {ScaleType::Arabian,        {0, 2, 4, 5, 6, 8, 10}}, // Locrian Major (approx)
    {ScaleType::Japanese,       {0, 1, 5, 7, 8}}, // Insen
    {ScaleType::Chinese,        {0, 4, 6, 7, 9}}, // Approx Pentatonic
    {ScaleType::Balinese,       {0, 1, 3, 7, 8}}, // Pelog
    {ScaleType::NeapolitanMajor,{0, 1, 3, 5, 7, 9, 11}},
    {ScaleType::NeapolitanMinor,{0, 1, 3, 5, 7, 8, 11}},
    {ScaleType::Enigmatic,      {0, 1, 4, 6, 8, 10, 11}},
    {ScaleType::DoubleHarmonic, {0, 1, 4, 5, 7, 8, 11}},
    {ScaleType::SpanishGypsy,   {0, 1, 4, 5, 7, 8, 10}},
    {ScaleType::Prometheus,     {0, 2, 4, 6, 9, 10}}
  };

  auto it = SCALE_INTERVALS.find(scaleHighlight.scale);
  if (it == SCALE_INTERVALS.end()) return true;

  for (int interval : it->second) {
    if (degree == interval) return true;
  }

  return false;
}

juce::String PianoRollComponent::getCurrentChordName() const {
  // Analyze selected notes
  std::vector<int> pitches;
  for (const auto &note : noteRects) {
    if (note.selected) {
      pitches.push_back(note.pitch);
    }
  }
  return detectChord(pitches);
}

juce::String
PianoRollComponent::detectChord(const std::vector<int> &pitches) const {
  if (pitches.empty())
    return "";
  
  if (pitches.size() == 1) {
    // Return note name
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return noteNames[pitches[0] % 12];
  }
  
  if (pitches.size() == 2) {
    // Interval name? or just return Dyad
    int interval = std::abs(pitches[1] - pitches[0]);
    if (interval == 7) return "Power Chord (5)";
    if (interval == 4) return "Major 3rd";
    if (interval == 3) return "Minor 3rd";
    // ...
    return ""; // Too simple
  }

  // Normalize pitches to pitch classes (0-11) sorted unique
  std::vector<int> sortedPitches = pitches;
  std::sort(sortedPitches.begin(), sortedPitches.end());
  
  // Try to find root by checking all rotations
  // This is a naive but effective O(N*types) approach
  
  struct ChordDef {
    juce::String name;
    std::vector<int> intervals;
  };
  
  static const std::vector<ChordDef> CHORD_DEFS = {
    {"Major", {0, 4, 7}},
    {"Minor", {0, 3, 7}},
    {"Dim", {0, 3, 6}},
    {"Aug", {0, 4, 8}},
    {"Sus2", {0, 2, 7}},
    {"Sus4", {0, 5, 7}},
    {"Maj7", {0, 4, 7, 11}},
    {"Min7", {0, 3, 7, 10}},
    {"Dom7", {0, 4, 7, 10}},
    {"Dim7", {0, 3, 6, 9}},
    {"m7b5", {0, 3, 6, 10}},
    {"6", {0, 4, 7, 9}},
    {"m6", {0, 3, 7, 9}},
    {"9", {0, 4, 7, 10, 14}},
    {"Maj9", {0, 4, 7, 11, 14}},
    {"Min9", {0, 3, 7, 10, 14}}
  };
  
  // Try each note as root
  static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

  for (size_t i = 0; i < sortedPitches.size(); ++i) {
    int root = sortedPitches[i];
    int rootClass = root % 12;
    
    // Check relative intervals from this root
    std::set<int> presentIntervals;
    bool allFit = true;
    for (int p : sortedPitches) {
        // Calculate interval modulo 12, but handle multi-octave (9ths/11ths need care)
        // For basic detection we usually fold to 0-11, but 9ths are 14 semitones...
        // Let's stick to set classes (0-11) for simplicity first
        int interval = (p - root);
        if (interval < 0) continue; // Should not happen if sorted
        
        // We need to keep extended intervals if we want to detect 9ths specifically
        // But pure set class matching is more robust for inversions.
        
        int intervalClass = interval % 12; 
        presentIntervals.insert(intervalClass);
        
        // Also add extended? 
        if (interval == 14) presentIntervals.insert(14); 
    }
    
    // Check against defs
    for (const auto& def : CHORD_DEFS) {
      bool match = true;
      // Check if all def intervals are present
      for (int req : def.intervals) {
        if (presentIntervals.find(req % 12) == presentIntervals.end()) {
          match = false;
          break;
        }
      }
      // Check if we have extra notes not in def? 
      // Strict matching: size must match
      if (match && presentIntervals.size() == def.intervals.size()) {
          // Found it!
          juce::String chordName = noteNames[rootClass];
          chordName += " " + def.name;
          
          // Detect Inversion if root is not the lowest note
          if (sortedPitches[0] != root) {
              chordName += "/" + juce::String(noteNames[sortedPitches[0]%12]);
          }
          return chordName;
      }
    }
  }

  return "Unknown";
}

//==============================================================================
// Scale Lock
//==============================================================================

bool PianoRollComponent::getScaleLock() const { return scaleLockEnabled; }

void PianoRollComponent::setScaleLock(bool enabled) {
  scaleLockEnabled = enabled;
  repaint();
}

void PianoRollComponent::updateScaleLockNotes() {
  scaleLockNotes.resize(12, false);
  // Mark all notes in the current scale
  for (int i = 0; i < 12; ++i) {
    scaleLockNotes[i] = isNoteInScale(i);
  }
}

//==============================================================================
// Arpeggiator Implementation
//==============================================================================

void PianoRollComponent::setArpeggiatorPreview(bool enabled, ArpPattern pattern,
                                               double rate, int octaves) {
  arpPreviewEnabled = enabled;
  arpPattern = pattern;
  arpRate = rate;
  arpOctaves = octaves;

  if (enabled) {
    generateArpPreview();
  } else {
    arpPreviewNotes.clear();
  }
  repaint();
}

void PianoRollComponent::generateArpPreview() {
  arpPreviewNotes.clear();

  // 1. Get source notes (selected or all)
  std::vector<NoteRect> sourceNotes;
  for (const auto &n : noteRects) {
    if (n.selected)
      sourceNotes.push_back(n);
  }

  // If no notes selected, maybe use all notes overlapping playhead?
  // For now, require selection.
  if (sourceNotes.empty())
    return;

  // Sort by start time then pitch
  std::sort(sourceNotes.begin(), sourceNotes.end(),
            [](const NoteRect &a, const NoteRect &b) {
              if (std::abs(a.startBeats - b.startBeats) > 0.001)
                return a.startBeats < b.startBeats;
              return a.pitch < b.pitch; // Lowest pitch first
            });

  // Find Chord Chunks (notes starting at approx same time)
  struct ChordChunk {
    double start;
    double length;
    std::vector<int> pitches;
  };

  std::vector<ChordChunk> chunks;

  for (const auto &note : sourceNotes) {
    bool added = false;
    for (auto &chunk : chunks) {
      if (std::abs(note.startBeats - chunk.start) < 0.1) { // 0.1 beat tolerance
        chunk.pitches.push_back(note.pitch);
        chunk.length = std::max(chunk.length, note.lengthBeats);
        added = true;
        break;
      }
    }
    if (!added) {
      chunks.push_back({note.startBeats, note.lengthBeats, {note.pitch}});
    }
  }

  // Generate Arp for each chunk
  for (auto &chunk : chunks) {
    std::sort(chunk.pitches.begin(), chunk.pitches.end());

    // Expand Octaves
    std::vector<int> extendedPitches = chunk.pitches;
    for (int oct = 1; oct < arpOctaves; ++oct) {
      for (int p : chunk.pitches) {
        int newPitch = p + (12 * oct);
        if (newPitch < 128)
          extendedPitches.push_back(newPitch);
      }
    }

    if (extendedPitches.empty())
      continue;

    // Sort extended
    std::sort(extendedPitches.begin(), extendedPitches.end());

    // Apply Pattern ordering
    std::vector<int> patternPitches;
    switch (arpPattern) {
    case ArpPattern::Up:
      patternPitches = extendedPitches;
      break;
    case ArpPattern::Down:
      patternPitches = extendedPitches;
      std::reverse(patternPitches.begin(), patternPitches.end());
      break;
    case ArpPattern::UpDown:
      patternPitches = extendedPitches;
      for (int i = (int)extendedPitches.size() - 2; i >= 1; --i)
        patternPitches.push_back(extendedPitches[i]);
      break;
    case ArpPattern::DownUp:
      patternPitches = extendedPitches;
      std::reverse(patternPitches.begin(), patternPitches.end());
      for (size_t i = 1; i < extendedPitches.size() - 1; ++i)
        patternPitches.push_back(extendedPitches[i]);
      break;
    case ArpPattern::Random:
      patternPitches = extendedPitches;
      // poor man's shuffle for preview stability (seeded by size?)
      break;
    case ArpPattern::Order:
      patternPitches = extendedPitches;
      break;
    }

    // Fill time
    double t = chunk.start;
    double updateRate = (arpRate > 0) ? arpRate : 0.25;
    int idx = 0;

    while (t < chunk.start + chunk.length) {
      NoteRect newNote;
      newNote.pitch = patternPitches[idx % patternPitches.size()];
      newNote.startBeats = t;
      newNote.lengthBeats = updateRate;
      newNote.velocity = 100;
      newNote.id = "PREVIEW_" + juce::String(t);

      // Adjust length
      if (newNote.startBeats + newNote.lengthBeats >
          chunk.start + chunk.length) {
        newNote.lengthBeats = (chunk.start + chunk.length) - newNote.startBeats;
      }

      if (newNote.lengthBeats > 0.01) {
        arpPreviewNotes.push_back(newNote);
      }

      t += updateRate;
      idx++;
    }
  }

  // Recalculate preview bounds
  for (auto &note : arpPreviewNotes) {
    float x = PIANO_WIDTH + beatsToPixels(note.startBeats);
    float y = RULER_HEIGHT + pitchToPixels(note.pitch);
    float width = beatsToPixels(note.lengthBeats);
    float height = pixelsPerPitch;
    note.bounds = juce::Rectangle<float>(x, y, width, height);
  }
}

void PianoRollComponent::commitArpeggiator() {
  if (!arpPreviewEnabled || arpPreviewNotes.empty())
    return;

  if (!currentClip.isValid())
    return;

  projectState.getUndoManager().beginNewTransaction("Apply Arpeggiator");

  // Find selected notes to remove
  std::vector<juce::String> idsToRemove;
  for (const auto &n : noteRects) {
    if (n.selected)
      idsToRemove.push_back(n.id);
  }
  for (const auto &id : idsToRemove) {
    projectState.removeMidiNote(currentClip.clipId, id, "");
  }

  // Add new notes
  for (const auto &p : arpPreviewNotes) {
    zenith::ProjectState::MidiNoteSpec note;
    note.id = juce::Uuid().toString();
    note.pitch = p.pitch;
    note.startBeats = p.startBeats;
    note.lengthBeats = p.lengthBeats;
    note.velocity = p.velocity;
    note.muted = false;

    projectState.addMidiNote(currentClip.clipId, note, "");
  }

  setArpeggiatorPreview(false);
}

void PianoRollComponent::drawArpPreview(SkCanvas *canvas, const SkRect &area) {
  using namespace zenith::design;

  canvas->save();
  canvas->clipRect(area);

  SkPaint previewPaint;
  previewPaint.setColor(withAlpha(colors::ACCENT_PRIMARY, 0.5f));
  previewPaint.setStyle(SkPaint::kFill_Style);
  previewPaint.setAntiAlias(true);

  SkPaint previewBorder;
  previewBorder.setColor(withAlpha(colors::ACCENT_PRIMARY, 0.8f));
  previewBorder.setStyle(SkPaint::kStroke_Style);
  previewBorder.setStrokeWidth(1.0f);
  previewBorder.setAntiAlias(true);
  float intervals[] = {4.0f, 2.0f};
  previewBorder.setPathEffect(
      SkDashPathEffect::Make(SkSpan(intervals, 2), 0.0f));

  for (const auto &note : arpPreviewNotes) {
    if (note.bounds.getY() > area.bottom() ||
        note.bounds.getBottom() < area.top())
      continue;
    if (note.bounds.getX() > area.right() ||
        note.bounds.getRight() < area.left())
      continue;

    SkRect r =
        SkRect::MakeXYWH(note.bounds.getX(), note.bounds.getY(),
                         note.bounds.getWidth(), note.bounds.getHeight());

    SkRRect rr = SkRRect::MakeRectXY(r.makeInset(2, 2), 3.0f, 3.0f);

    canvas->drawRRect(rr, previewPaint);
    canvas->drawRRect(rr, previewBorder);
  }

  canvas->restore();
}

//==============================================================================
// MidiEditorContainer Implementation
//==============================================================================

#include "../../engine/Track.h"

MidiEditorContainer::MidiEditorContainer(zenith::ProjectState &state,
                                         zenith::Engine &engine)
    : projectState(state), engine_(engine) {
  pianoRoll = std::make_unique<PianoRollComponent>(state);
  // Setup Preview for Piano Roll
  pianoRoll->setNotePreviewCallback(
      [this](int pitch, int velocity, bool noteOn) {
        juce::MidiMessage msg;
        if (noteOn)
          msg = juce::MidiMessage::noteOn(1, pitch, (juce::uint8)velocity);
        else
          msg = juce::MidiMessage::noteOff(1, pitch);
        injectMidiMessage(msg);
      });
  addAndMakeVisible(pianoRoll.get());

  drumPad = std::make_unique<DrumPadComponent>(engine, state);
  // Setup Preview for Drum Pad
  drumPad->setNotePreviewCallback([this](int pitch, int velocity, bool noteOn) {
    juce::MidiMessage msg;
    if (noteOn)
      msg = juce::MidiMessage::noteOn(1, pitch, (juce::uint8)velocity);
    else
      msg = juce::MidiMessage::noteOff(1, pitch);
    injectMidiMessage(msg);
  });
  addChildComponent(drumPad.get()); // Hidden by default

  // Toggle Button
  toggleButton.setButtonText("Switch to Drum View");
  toggleButton.onClick = [this] { toggleView(); };
  addAndMakeVisible(toggleButton);
}

MidiEditorContainer::~MidiEditorContainer() {}

void MidiEditorContainer::setClipContext(const MidiClipContext &context) {
  currentContext = context;
  pianoRoll->setClipContext(context);
  drumPad->setClipContext(context.clipId);

  // Auto-detect mode based on track name
  if (context.clipName.containsIgnoreCase("drum") ||
      context.trackId.containsIgnoreCase("drum")) {
    if (activeView == View::PianoRoll)
      toggleView();
  }
}

void PianoRollComponent::setGhostNotesEnabled(bool enabled) {
  ghostNotesEnabled = enabled;
  if (enabled) {
    refreshGhostNotes();
  }
  repaint();
}

void PianoRollComponent::setGhostNoteOpacity(float opacity) {
  ghostNoteOpacity = juce::jlimit(0.1f, 0.9f, opacity);
  repaint();
}

void PianoRollComponent::addGhostClip(const juce::String &clipId) {
  if (std::find(ghostClipIds.begin(), ghostClipIds.end(), clipId) ==
      ghostClipIds.end()) {
    ghostClipIds.push_back(clipId);
    if (ghostNotesEnabled) refreshGhostNotes();
  }
}

void PianoRollComponent::removeGhostClip(const juce::String &clipId) {
  auto it = std::remove(ghostClipIds.begin(), ghostClipIds.end(), clipId);
  if (it != ghostClipIds.end()) {
    ghostClipIds.erase(it, ghostClipIds.end());
    if (ghostNotesEnabled) refreshGhostNotes();
  }
}

void PianoRollComponent::clearGhostClips() {
  ghostClipIds.clear();
  refreshGhostNotes();
}

void PianoRollComponent::refreshGhostNotes() {
    ghostNotes.clear();
    
    // Safety check
    if (!currentClip.isValid()) return;

    for (const auto& clipId : ghostClipIds) {
        // Skip self
        if (clipId == currentClip.clipId) continue;
        
        // Find clip to get position
        auto [track, clipTree] = projectState.findClip(clipId);
        if (!clipTree.isValid()) continue;
        
        // Assume "start" property for position in beats
        // Fallback to "pos" or 0 if not found
        double clipStart = (double)clipTree.getProperty("start", 
                              (double)clipTree.getProperty("pos", 0.0));
        
        // Calculate offset relative to current clip
        double offset = clipStart - currentClip.clipStartBeats;
        
        // Fetch notes
        auto notes = projectState.getMidiNotesForClip(clipId);
        
        for (const auto& spec : notes) {
             GhostNote gn;
             gn.pitch = spec.pitch;
             gn.startBeats = spec.startBeats + offset;
             gn.lengthBeats = spec.lengthBeats;
             
             // Setup bounds later or compute now if needed for simple drawing
             // We compute bounds in drawGhostNotes usually or here?
             // Helper struct just has data, draw call computes pixels
             
             ghostNotes.push_back(gn);
        }
    }
    
    repaint();
}

void PianoRollComponent::drawGhostNotes(SkCanvas *canvas, const SkRect &rect) {
    if (!canvas || !ghostNotesEnabled || ghostNotes.empty()) return;
    
    using namespace zenith::design;
    
    SkPaint ghostPaint;
    ghostPaint.setColor(withAlpha(colors::TEXT_SECONDARY, ghostNoteOpacity));
    ghostPaint.setAntiAlias(true);
    
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setColor(withAlpha(colors::TEXT_SECONDARY, ghostNoteOpacity * 1.5f));
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    
    for (const auto& ghost : ghostNotes) {
       float x = rect.left() + beatsToPixels(ghost.startBeats);
       float w = beatsToPixels(ghost.lengthBeats);
       float y = rect.top() + pitchToPixels(ghost.pitch);
       float h = pixelsPerPitch;
       
       // Culling
       if (x > rect.right() || x + w < rect.left() || 
           y > rect.bottom() || y + h < rect.top()) 
           continue;
       
       SkRect r = SkRect::MakeXYWH(x, y, w, h);
       SkRRect rr = SkRRect::MakeRectXY(r.makeInset(1, 1), 2.0f, 2.0f);
       
       canvas->drawRRect(rr, ghostPaint);
       canvas->drawRRect(rr, borderPaint);
    }
}

void PianoRollComponent::drawPlayhead(SkCanvas *canvas, const SkRect &rect) {
   if (!canvas) return;
   
   using namespace zenith::design;
   float width = (float)getLocalBounds().getWidth();
   float height = (float)getLocalBounds().getHeight();

    float playheadX = PIANO_WIDTH + beatsToPixels(currentPlayheadBeats);
    if (playheadX >= PIANO_WIDTH && playheadX <= width) {
      // Playhead line
      SkPaint playheadPaint;
      playheadPaint.setColor(colors::TEXT_PRIMARY);
      playheadPaint.setStrokeWidth(2.0f);
      playheadPaint.setAntiAlias(true);
      canvas->drawLine(playheadX, RULER_HEIGHT, playheadX, height,
                       playheadPaint);

      // Triangle marker in ruler/toolbar area
      static constexpr float kPlayheadMarkerHalfWidth = 5.0f;
      static constexpr float kPlayheadMarkerHeight = 8.0f;

      const float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;
      SkPath trianglePath;
      trianglePath.moveTo(playheadX, contentTop);
      trianglePath.lineTo(playheadX - kPlayheadMarkerHalfWidth,
                          contentTop - kPlayheadMarkerHeight);
      trianglePath.lineTo(playheadX + kPlayheadMarkerHalfWidth,
                          contentTop - kPlayheadMarkerHeight);
      trianglePath.close();
      canvas->drawPath(trianglePath, playheadPaint);
    }
}

void MidiEditorContainer::resized() {
  auto area = getLocalBounds();
  auto topBar = area.removeFromTop(30);

  toggleButton.setBounds(topBar.removeFromRight(150).reduced(2));

  if (activeView == View::PianoRoll) {
    pianoRoll->setBounds(area);
  } else {
    drumPad->setBounds(area);
  }
}

void MidiEditorContainer::toggleView() {
  if (activeView == View::PianoRoll) {
    activeView = View::DrumPad;
    pianoRoll->setVisible(false);
    drumPad->setVisible(true);
    toggleButton.setButtonText("Switch to Piano Roll");
  } else {
    activeView = View::PianoRoll;
    pianoRoll->setVisible(true);
    drumPad->setVisible(false);
    toggleButton.setButtonText("Switch to Drum View");
  }
  resized();
}

void MidiEditorContainer::injectMidiMessage(const juce::MidiMessage &msg) {
  // Find track by ID and inject message
  // Tracks can be iterated nicely on message thread
  for (const auto &track : engine_.tracks()) {
    if (track->getTrackId() == currentContext.trackId) {
      track->injectLiveMidiMessage(msg);
      break;
    }
  }
}
