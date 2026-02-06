/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


 * @file PianoRollInput.cpp
 * @brief Piano Roll Input Handling - Part of PianoRollComponent



//==============================================================================
// Mouse Interaction
//==============================================================================

void PianoRollComponent::mouseMove(const juce::MouseEvent &e) {
  juce::MouseEvent event = e.withNewPosition(e.position.translated(-contentOffsetX_, 0));
  if (e.x < contentOffsetX_) {
      currentCursorType = CursorType::Normal;
      setMouseCursor(getMouseCursor());
      return;
  }
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
    setMouseCursor(getMouseCursor());
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
      repaint(dirtyRect.expanded(design::stroke::THICK).toNearestInt());
    }
  }
}

void PianoRollComponent::mouseDown(const juce::MouseEvent &e) {
  // Adjust logic coordinates for the side panel offset
  juce::MouseEvent event = e.withNewPosition(e.position.translated(-contentOffsetX_, 0));
  
  // Check if click is actually in the piano roll area (rendering logic checks translated bounds, 
  // but we should ensure we don't handle clicks meant for the side panel if it wasn't a child component)
  // Since side panel is a child component, it should consume events first. 
  // But if it ignores them, they bubble up.
  if (e.x < contentOffsetX_) return; 

  if (event.mods.isRightButtonDown()) {
    return;
  }

  if (!currentClip.isValid())
    return;

  if (stepSequencerMode) {
    // Handle Step Sequencer clicks
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
          // BUG FIX: Use shared ownership to prevent memory leak
          auto window = std::make_shared<juce::AlertWindow>("Humanize", "Adjust randomization parameters:", juce::AlertWindow::QuestionIcon);
          window->addTextEditor("velocity", "10", "Velocity Range (+/-):");
          window->addTextEditor("timing", "0.05", "Timing Range (beats):");
          window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
          window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
          
          auto safeThis = juce::Component::SafePointer<PianoRollComponent>(this);
          auto clipId = currentClip.clipId;

          window->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, window, clipId](int result) {
              if (result != 0 && safeThis) {
                  double velRange = window->getTextEditorContents("velocity").getDoubleValue();
                  double timeRange = window->getTextEditorContents("timing").getDoubleValue();
                  safeThis->projectState.humanizeClip(clipId, velRange, timeRange, "Humanize Selected");
              }
              // shared_ptr handles deletion
          }), true);
      });
      menu->addSeparator();
      menu->addItem(5, "Duplicate", true, false, [this]() {
            // Check if ANY selected, if not return
            if (getSelectedNoteCount() == 0) return;
            
            projectState.getUndoManager().beginNewTransaction("Duplicate Notes");
            
            // BUG FIX: Optimization - Pre-fetch note specs 
            auto allNoteSpecs = projectState.getMidiNotesForClip(currentClip.clipId);
            std::unordered_map<juce::String, zenith::ProjectState::MidiNoteSpec> specMap;
            for (const auto& spec : allNoteSpecs) {
                specMap[spec.id] = spec;
            }

            // Find end of selection to shift
            for (const auto& note : noteRects) {
                if (note.selected) {
                     zenith::ProjectState::MidiNoteSpec newNote;
                     newNote.id = juce::Uuid().toString();
                     newNote.pitch = note.pitch;
                     newNote.startBeats = note.startBeats + gridBeats; // Offset by grid
                     newNote.lengthBeats = note.lengthBeats;
                     
                     auto it = specMap.find(note.id);
                     if (it != specMap.end()) {
                         newNote.velocity = it->second.velocity;
                         newNote.muted = it->second.muted;
                     } else {
                         newNote.velocity = 100;
                         newNote.muted = false;
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
  juce::MouseEvent event = e.withNewPosition(e.position.translated(-contentOffsetX_, 0));
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
  case DragMode::ExpressionTension:
    updateExpressionTension(e);
    break;
  default:
    break;
  }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent &e) {
  juce::MouseEvent event = e.withNewPosition(e.position.translated(-contentOffsetX_, 0));
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
  case DragMode::ExpressionTension:
    finishExpressionTension();
    break;
  default:
    break;
  }
  currentDragMode = DragMode::None;
  activeNote = nullptr;
}

void PianoRollComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  juce::MouseEvent event = e.withNewPosition(e.position.translated(-contentOffsetX_, 0));
  if (event.x < contentOffsetX_) return;
  if (!currentClip.isValid())
    return;
  float x = static_cast<float>(event.x);
  float y = static_cast<float>(event.y);
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

bool PianoRollComponent::keyPressed(const juce::KeyPress &key) {
    if (key == juce::KeyPress::backspaceKey || key == juce::KeyPress::deleteKey) {
        deleteSelectedNotes();
        return true;
    }
    if (key.isKeyCode('C') \u0026\u0026 key.getModifiers().isCommandDown()) {
        copySelectedNotes();
        return true;
    }
    if (key.isKeyCode('V') \u0026\u0026 key.getModifiers().isCommandDown()) {
        pasteNotes();
        return true;
    }
    if (key.isKeyCode('X') \u0026\u0026 key.getModifiers().isCommandDown()) {
        cutSelectedNotes();
        return true;
    }
    if (key.isKeyCode('A') \u0026\u0026 key.getModifiers().isCommandDown()) {
        selectAll();
        return true;
    }
    if (key.isKeyCode('D') \u0026\u0026 key.getModifiers().isCommandDown()) {
        smartDuplicate();
        return true;
    }
    if (key.isKeyCode('Q')) {
        quantizeSelected(gridBeats);
        return true;
    }
    return false;
}

//==============================================================================
// Drag Operations
//==============================================================================

void PianoRollComponent::startMovingSelection(const juce::MouseEvent \u0026e) {
  currentDragMode = DragMode::MoveNote;
  dragStartPos = e.position;
  dragStates.clear();
  for (const auto \u0026note : noteRects) {
    if (note.selected) {
      NoteDragState state;
      state.id = note.id;
      state.originalPitch = note.pitch;
      state.originalStartBeats = note.startBeats;
      dragStates.push_back(state);
    }
  }
}

void PianoRollComponent::updateSelectionMove(const juce::MouseEvent \u0026e) {
  if (dragStates.empty())
    return;
  float deltaX = e.position.x - dragStartPos.x;
  float deltaY = e.position.y - dragStartPos.y;
  double deltaBeats =
      pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);
  int deltaPitch = -static_cast\u003cint\u003e(deltaY / pixelsPerPitch);

  size_t stateIndex = 0;
  for (auto \u0026note : noteRects) {
    if (note.selected \u0026\u0026 stateIndex \u003c dragStates.size()) {
      const auto \u0026originalState = dragStates[stateIndex];
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
  for (auto \u0026note : noteRects) {
    if (note.selected \u0026\u0026 stateIndex \u003c dragStates.size()) {
      const auto \u0026originalState = dragStates[stateIndex];
      bool pitchChanged = note.pitch != originalState.originalPitch;
      bool startChanged =
          std::abs(note.startBeats - originalState.originalStartBeats) \u003e 0.001;
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
                                           const juce::MouseEvent \u0026e) {
  currentDragMode = mode;
  activeNote = note;
  dragStartPos = e.position;
  dragStates.clear();
  NoteDragState state;
  state.id = note-\u003eid;
  state.originalStartBeats = note-\u003estartBeats;
  state.originalLengthBeats = note-\u003elengthBeats;
  dragStates.push_back(state);
  clearSelection();
  note-\u003eselected = true;
  repaint();
}

void PianoRollComponent::updateNoteResize(const juce::MouseEvent \u0026e) {
  if (!activeNote || dragStates.empty())
    return;
  float deltaX = e.position.x - dragStartPos.x;
  double deltaBeats =
      pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);
  const auto \u0026originalState = dragStates[0];

  if (currentDragMode == DragMode::ResizeLeft) {
    double newStartBeats = originalState.originalStartBeats + deltaBeats;
    if (snapEnabled)
      newStartBeats = snapToGrid(newStartBeats);
    newStartBeats = juce::jmax(0.0, newStartBeats);
    double newLengthBeats = originalState.originalLengthBeats -
                            (newStartBeats - originalState.originalStartBeats);
    newLengthBeats = juce::jmax(0.01, newLengthBeats);
    activeNote-\u003estartBeats = newStartBeats;
    activeNote-\u003elengthBeats = newLengthBeats;
  } else if (currentDragMode == DragMode::ResizeRight) {
    double newLengthBeats = originalState.originalLengthBeats + deltaBeats;
    if (snapEnabled) {
      double endBeats =
          snapToGrid(originalState.originalStartBeats + newLengthBeats);
      newLengthBeats = endBeats - originalState.originalStartBeats;
    }
    newLengthBeats = juce::jmax(0.01, newLengthBeats);
    activeNote-\u003elengthBeats = newLengthBeats;
  }
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishNoteResize() {
  if (!activeNote || dragStates.empty())
    return;
  const auto \u0026originalState = dragStates[0];
  bool startChanged = std::abs(activeNote-\u003estartBeats -
                               originalState.originalStartBeats) \u003e 0.001;
  bool lengthChanged = std::abs(activeNote-\u003elengthBeats -
                                originalState.originalLengthBeats) \u003e 0.001;
  if (startChanged || lengthChanged) {
    if (currentDragMode == DragMode::ResizeLeft) {
      projectState.getUndoManager().beginNewTransaction("Resize MIDI note");
      projectState.moveMidiNote(currentClip.clipId, activeNote-\u003eid,
                                activeNote-\u003estartBeats, activeNote-\u003epitch, "");
      projectState.setMidiNoteLength(currentClip.clipId, activeNote-\u003eid,
                                     activeNote-\u003elengthBeats, "");
    } else {
      projectState.setMidiNoteLength(currentClip.clipId, activeNote-\u003eid,
                                     activeNote-\u003elengthBeats,
                                     "Resize MIDI note");
    }
  }
  dragStates.clear();
}

void PianoRollComponent::startEditingVelocity(NoteRect *note,
                                              const juce::MouseEvent \u0026e) {
  currentDragMode = DragMode::VelocityEdit;
  activeNote = note;
  dragStartPos = e.position;
  dragStates.clear();
  NoteDragState state;
  state.id = note-\u003eid;
  state.originalVelocity = note-\u003evelocity;
  dragStates.push_back(state);
  clearSelection();
  note-\u003eselected = true;
  repaint();
}

void PianoRollComponent::updateVelocityEdit(const juce::MouseEvent \u0026e) {
  if (!activeNote || dragStates.empty())
    return;
  int newVelocity = pixelsToVelocity(static_cast\u003cfloat\u003e(e.y));
  activeNote-\u003evelocity = newVelocity;
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishVelocityEdit() {
  if (!activeNote || dragStates.empty())
    return;
  const auto \u0026originalState = dragStates[0];
  if (activeNote-\u003evelocity != originalState.originalVelocity) {
    projectState.setMidiNoteVelocity(currentClip.clipId, activeNote-\u003eid,
                                     activeNote-\u003evelocity,
                                     "Edit MIDI velocity");
  }
  dragStates.clear();
}

void PianoRollComponent::startMarqueeSelect(const juce::MouseEvent \u0026e) {
  currentDragMode = DragMode::MarqueeSelect;
  dragStartPos = e.position;
  marqueeRect =
      juce::Rectangle\u003cfloat\u003e(dragStartPos.x, dragStartPos.y, 0.0f, 0.0f);
  if (!e.mods.isCommandDown())
    clearSelection();
  repaint();
}

void PianoRollComponent::updateMarqueeSelect(const juce::MouseEvent \u0026e) {
  marqueeRect = juce::Rectangle\u003cfloat\u003e::leftTopRightBottom(
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
// Expression Tension Editing
//==============================================================================

void PianoRollComponent::startEditingExpressionTension(const juce::MouseEvent &e) {
  currentDragMode = DragMode::ExpressionTension;
  dragStartPos = e.position;
  repaint();
}

void PianoRollComponent::updateExpressionTension(const juce::MouseEvent &e) {
  if (currentDragMode != DragMode::ExpressionTension) return;
  
  float deltaY = static_cast<float>(e.y - dragStartPos.y);
  float sensitivity = 0.01f;
  float tensionChange = -deltaY * sensitivity;
  tensionChange = juce::jlimit(-1.0f, 1.0f, tensionChange);
  
  for (auto &note : noteRects) {
    if (note.selected) {
      float currentTension = note.tension;
      float newTension = juce::jlimit(-1.0f, 1.0f, currentTension + tensionChange);
      note.tension = newTension;
      projectState.setMidiNoteTension(currentClip.clipId, note.id, newTension, "Edit expression tension");
    }
  }
  
  dragStartPos = e.position;
  repaint();
}

void PianoRollComponent::finishExpressionTension() {
}

} // namespace zenith
