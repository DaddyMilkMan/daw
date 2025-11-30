/**
 * @file PianoRollComponent.cpp
 * @brief Piano Roll implementation (Phase 8.2 - Enhanced Ergonomics)
 */

#include "../include/PianoRollComponent.h"

//==============================================================================
PianoRollComponent::PianoRollComponent(ProjectState& state)
    : projectState(state)
{
    setWantsKeyboardFocus(true);
    startTimer(100);  // Periodic refresh check (10 Hz)
}

PianoRollComponent::~PianoRollComponent()
{
    stopTimer();

    // Detach ValueTree listener if attached
    if (currentClip.isValid())
    {
        auto [track, clip] = projectState.findClip(currentClip.clipId);
        if (clip.isValid())
        {
            auto midiNotesNode = clip.getChildWithName(ProjectState::ID_NOTES);
            if (midiNotesNode.isValid())
                midiNotesNode.removeListener(this);
        }
    }
}

//==============================================================================
void PianoRollComponent::setClipContext(const MidiClipContext& context)
{
    // Detach from old clip's ValueTree
    if (currentClip.isValid())
    {
        auto [oldTrack, oldClip] = projectState.findClip(currentClip.clipId);
        if (oldClip.isValid())
        {
            auto midiNotesNode = oldClip.getChildWithName(ProjectState::ID_NOTES);
            if (midiNotesNode.isValid())
                midiNotesNode.removeListener(this);
        }
    }

    // Set new clip
    currentClip = context;

    // Attach to new clip's ValueTree for auto-refresh
    if (currentClip.isValid())
    {
        auto [track, clip] = projectState.findClip(currentClip.clipId);
        if (clip.isValid())
        {
            // Get or create NOTES node
            auto midiNotesNode = clip.getChildWithName(ProjectState::ID_NOTES);
            if (!midiNotesNode.isValid())
            {
                // Create empty NOTES container if it doesn't exist
                midiNotesNode = juce::ValueTree(ProjectState::ID_NOTES);
                clip.appendChild(midiNotesNode, nullptr);
            }

            midiNotesNode.addListener(this);
        }

        // Load notes
        refreshNotesFromProjectState();
    }

    repaint();
}

//==============================================================================
// Data Management
//==============================================================================

void PianoRollComponent::refreshNotesFromProjectState()
{
    if (!currentClip.isValid())
    {
        noteRects.clear();
        return;
    }

    // Get notes from ProjectState (canonical source)
    auto notes = projectState.getMidiNotesForClip(currentClip.clipId);

    // Rebuild UI note cache
    noteRects.clear();
    for (const auto& note : notes)
    {
        NoteRect rect;
        rect.id = note.id;
        rect.pitch = note.pitch;
        rect.startBeats = note.startBeats;
        rect.lengthBeats = note.lengthBeats;
        rect.velocity = note.velocity;
        rect.muted = note.muted;
        rect.selected = false;  // Reset selection

        noteRects.push_back(rect);
    }

    updateNoteRectangles();
    needsRefresh = false;
    repaint();
}

void PianoRollComponent::updateNoteRectangles()
{
    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - velocityLaneHeight;

    for (auto& note : noteRects)
    {
        // Note grid area (main piano roll)
        float x = beatsToPixels(note.startBeats);
        float y = pitchToPixels(note.pitch);
        float width = beatsToPixels(note.lengthBeats);
        float height = static_cast<float>(pixelsPerPitch);

        note.bounds = juce::Rectangle<float>(x, y, width, height);

        // Phase 8.2: Velocity lane bar
        float velocityBarY = noteGridHeight + velocityToPixels(note.velocity);
        float velocityBarHeight = noteGridHeight + velocityLaneHeight - velocityBarY;
        note.velocityBounds = juce::Rectangle<float>(x, velocityBarY, width, velocityBarHeight);
    }
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

int PianoRollComponent::pixelsToPitch(float y) const
{
    int pitch = 127 - static_cast<int>((y + scrollOffsetY) / pixelsPerPitch);
    return juce::jlimit(0, 127, pitch);
}

float PianoRollComponent::pitchToPixels(int pitch) const
{
    return (127 - pitch) * pixelsPerPitch - scrollOffsetY;
}

double PianoRollComponent::pixelsToBeats(float x) const
{
    return (x + scrollOffsetX) / pixelsPerBeat;
}

float PianoRollComponent::beatsToPixels(double beats) const
{
    return static_cast<float>(beats * pixelsPerBeat - scrollOffsetX);
}

double PianoRollComponent::snapToGrid(double beats) const
{
    if (!snapEnabled || gridBeats <= 0.0)
        return beats;

    return std::round(beats / gridBeats) * gridBeats;
}

//==============================================================================
// Phase 8.2: Velocity Conversion
//==============================================================================

int PianoRollComponent::pixelsToVelocity(float y) const
{
    // y is relative to top of velocity lane
    // 0 at top = velocity 127, bottom = velocity 0
    float normalizedY = y / velocityLaneHeight;
    int velocity = static_cast<int>((1.0f - normalizedY) * 127.0f);
    return juce::jlimit(1, 127, velocity);
}

float PianoRollComponent::velocityToPixels(int velocity) const
{
    // Invert: velocity 127 at top (y=0), velocity 0 at bottom
    float normalized = velocity / 127.0f;
    return (1.0f - normalized) * velocityLaneHeight;
}

//==============================================================================
// Mouse Interaction
//==============================================================================

PianoRollComponent::NoteRect* PianoRollComponent::findNoteAtPosition(float x, float y)
{
    for (auto& note : noteRects)
    {
        if (note.bounds.contains(x, y))
            return &note;
    }
    return nullptr;
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!currentClip.isValid())
        return;

    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - velocityLaneHeight;

    // Phase 8.2: Check if click is in velocity lane
    if (e.y >= noteGridHeight)
    {
        auto* note = findNoteInVelocityLane(static_cast<float>(e.x));
        if (note != nullptr)
        {
            startEditingVelocity(note, e);
            return;
        }
    }

    // Check for note hit in main grid
    auto* note = findNoteAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));

    if (note != nullptr)
    {
        // Phase 8.2: Detect hit region (resize edges vs move)
        DragMode mode = detectNoteHitRegion(*note, static_cast<float>(e.x), static_cast<float>(e.y));

        if (mode == DragMode::ResizeLeft || mode == DragMode::ResizeRight)
        {
            startResizingNote(note, mode, e);
        }
        else  // MoveNote
        {
            // Phase 8.2: Multi-select with Ctrl/Cmd
            bool isMultiSelectModifier = e.mods.isCommandDown();
            if (isMultiSelectModifier)
            {
                selectNote(note, true);  // Add to selection
            }
            else if (!note->selected)
            {
                // Clear selection and select this note
                clearSelection();
                selectNote(note, false);
            }

            startMovingSelection(e);
        }
    }
    else
    {
        // No note hit
        // Phase 8.2: Start marquee select if Shift is held, otherwise create note
        if (e.mods.isShiftDown())
        {
            startMarqueeSelect(e);
        }
        else
        {
            createNoteAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));
        }
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    // Phase 8.2: Dispatch based on drag mode
    switch (currentDragMode)
    {
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
            // Legacy: fallback to old drag behavior
            if (draggingNote != nullptr)
                updateNoteDrag(e);
            break;
    }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent& e)
{
    // Phase 8.2: Dispatch based on drag mode
    switch (currentDragMode)
    {
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
            // Legacy: fallback to old drag behavior
            if (draggingNote != nullptr)
                finishNoteDrag();
            break;
    }

    currentDragMode = DragMode::None;
    activeNote = nullptr;
}

void PianoRollComponent::startDraggingNote(NoteRect* note, const juce::MouseEvent& e)
{
    draggingNote = note;
    dragStartPos = e.position;
    dragStartPitch = note->pitch;
    dragStartBeats = note->startBeats;

    // Select the note
    for (auto& n : noteRects)
        n.selected = (&n == draggingNote);

    repaint();
}

void PianoRollComponent::updateNoteDrag(const juce::MouseEvent& e)
{
    if (draggingNote == nullptr)
        return;

    // Calculate new position
    float deltaX = e.position.x - dragStartPos.x;
    float deltaY = e.position.y - dragStartPos.y;

    double newStartBeats = dragStartBeats + pixelsToBeats(deltaX) - pixelsToBeats(0);
    int newPitch = dragStartPitch - static_cast<int>(deltaY / pixelsPerPitch);

    if (snapEnabled)
        newStartBeats = snapToGrid(newStartBeats);

    newStartBeats = juce::jmax(0.0, newStartBeats);
    newPitch = juce::jlimit(0, 127, newPitch);

    // Update visual position (don't commit to ProjectState yet)
    draggingNote->startBeats = newStartBeats;
    draggingNote->pitch = newPitch;

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::finishNoteDrag()
{
    if (draggingNote == nullptr)
        return;

    // Check if note actually moved
    if (draggingNote->pitch != dragStartPitch ||
        std::abs(draggingNote->startBeats - dragStartBeats) > 0.001)
    {
        // Commit to ProjectState (undoable!)
        projectState.moveMidiNote(currentClip.clipId,
                                   draggingNote->id,
                                   draggingNote->startBeats,
                                   draggingNote->pitch,
                                   "Move MIDI note");

        DBG("Moved note " + draggingNote->id +
            " to pitch=" + juce::String(draggingNote->pitch) +
            ", start=" + juce::String(draggingNote->startBeats));

        // Note: ValueTree listener will trigger refresh automatically
    }
    else
    {
        // No change - just deselect
        draggingNote = nullptr;
        repaint();
    }

    draggingNote = nullptr;
}

void PianoRollComponent::createNoteAtPosition(float x, float y)
{
    int pitch = pixelsToPitch(y);
    double startBeats = pixelsToBeats(x);

    if (snapEnabled)
        startBeats = snapToGrid(startBeats);

    startBeats = juce::jmax(0.0, startBeats);
    pitch = juce::jlimit(0, 127, pitch);

    // Default note length (1 grid unit)
    double lengthBeats = gridBeats;

    // Create note spec
    ProjectState::MidiNoteSpec note;
    note.pitch = pitch;
    note.startBeats = startBeats;
    note.lengthBeats = lengthBeats;
    note.velocity = 100;
    note.muted = false;

    // Add via ProjectState (undoable!)
    juce::String noteId = projectState.addMidiNote(currentClip.clipId, note, "Create MIDI note");

    DBG("Created note " + noteId + " at pitch=" + juce::String(pitch) +
        ", start=" + juce::String(startBeats));

    // Note: ValueTree listener will trigger refresh automatically
}

void PianoRollComponent::deleteSelectedNotes()
{
    if (!currentClip.isValid())
        return;

    // Collect selected note IDs
    std::vector<juce::String> selectedIds;
    for (const auto& note : noteRects)
    {
        if (note.selected)
            selectedIds.push_back(note.id);
    }

    if (selectedIds.empty())
        return;

    // Delete via ProjectState (undoable!)
    // Note: Each delete is a separate undo action for now
    // TODO Phase 9: Batch operations
    for (const auto& noteId : selectedIds)
    {
        projectState.removeMidiNote(currentClip.clipId, noteId, "Delete MIDI note");
        DBG("Deleted note " + noteId);
    }

    // Note: ValueTree listener will trigger refresh automatically
}

//==============================================================================
// Phase 8.2: Mouse Move (for cursor feedback)
//==============================================================================

void PianoRollComponent::mouseMove(const juce::MouseEvent& e)
{
    // TODO Phase 8.3: Change cursor based on hit region (resize cursors, etc.)
    // For now, just a placeholder
}

//==============================================================================
// Phase 8.2: Mouse Wheel (zoom & scroll)
//==============================================================================

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        // Cmd/Ctrl + wheel = horizontal zoom
        float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
        zoomHorizontal(zoomFactor, static_cast<float>(e.x));
    }
    else if (e.mods.isAltDown())
    {
        // Alt + wheel = vertical zoom
        float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
        zoomVertical(zoomFactor, static_cast<float>(e.y));
    }
    else if (e.mods.isShiftDown())
    {
        // Shift + wheel = horizontal scroll
        scrollHorizontal(-wheel.deltaY * 50.0f);
    }
    else
    {
        // Default wheel = vertical scroll
        scrollVertical(-wheel.deltaY * 50.0f);
    }
}

//==============================================================================
// Phase 8.2: Edge Detection & Resize
//==============================================================================

PianoRollComponent::DragMode PianoRollComponent::detectNoteHitRegion(const NoteRect& note, float x, float y) const
{
    if (!note.bounds.contains(x, y))
        return DragMode::None;

    // Check left edge
    if (x < note.bounds.getX() + resizeHandleWidth)
        return DragMode::ResizeLeft;

    // Check right edge
    if (x > note.bounds.getRight() - resizeHandleWidth)
        return DragMode::ResizeRight;

    // Otherwise, move note
    return DragMode::MoveNote;
}

void PianoRollComponent::startResizingNote(NoteRect* note, DragMode mode, const juce::MouseEvent& e)
{
    currentDragMode = mode;
    activeNote = note;
    dragStartPos = e.position;

    // Cache original state
    dragStates.clear();
    NoteDragState state;
    state.id = note->id;
    state.originalStartBeats = note->startBeats;
    state.originalLengthBeats = note->lengthBeats;
    dragStates.push_back(state);

    // Select this note
    clearSelection();
    note->selected = true;
    repaint();
}

void PianoRollComponent::updateNoteResize(const juce::MouseEvent& e)
{
    if (activeNote == nullptr || dragStates.empty())
        return;

    float deltaX = e.position.x - dragStartPos.x;
    double deltaBeats = pixelsToBeats(deltaX) - pixelsToBeats(0);

    const auto& originalState = dragStates[0];

    if (currentDragMode == DragMode::ResizeLeft)
    {
        // Resize from left: change start + length
        double newStartBeats = originalState.originalStartBeats + deltaBeats;
        if (snapEnabled)
            newStartBeats = snapToGrid(newStartBeats);

        newStartBeats = juce::jmax(0.0, newStartBeats);
        double newLengthBeats = originalState.originalLengthBeats - (newStartBeats - originalState.originalStartBeats);
        newLengthBeats = juce::jmax(0.01, newLengthBeats);  // Minimum length

        activeNote->startBeats = newStartBeats;
        activeNote->lengthBeats = newLengthBeats;
    }
    else if (currentDragMode == DragMode::ResizeRight)
    {
        // Resize from right: change length only
        double newLengthBeats = originalState.originalLengthBeats + deltaBeats;
        if (snapEnabled)
        {
            double endBeats = snapToGrid(originalState.originalStartBeats + newLengthBeats);
            newLengthBeats = endBeats - originalState.originalStartBeats;
        }

        newLengthBeats = juce::jmax(0.01, newLengthBeats);  // Minimum length
        activeNote->lengthBeats = newLengthBeats;
    }

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::finishNoteResize()
{
    if (activeNote == nullptr || dragStates.empty())
        return;

    const auto& originalState = dragStates[0];

    // Check if note actually changed
    bool startChanged = std::abs(activeNote->startBeats - originalState.originalStartBeats) > 0.001;
    bool lengthChanged = std::abs(activeNote->lengthBeats - originalState.originalLengthBeats) > 0.001;

    if (startChanged || lengthChanged)
    {
        if (currentDragMode == DragMode::ResizeLeft)
        {
            // Resize from left: use moveMidiNote to change start, then setMidiNoteLength
            projectState.moveMidiNote(currentClip.clipId,
                                       activeNote->id,
                                       activeNote->startBeats,
                                       activeNote->pitch,
                                       "Resize MIDI note (left edge)");

            projectState.setMidiNoteLength(currentClip.clipId,
                                            activeNote->id,
                                            activeNote->lengthBeats,
                                            "Resize MIDI note (left edge)");
        }
        else if (currentDragMode == DragMode::ResizeRight)
        {
            // Resize from right: just change length
            projectState.setMidiNoteLength(currentClip.clipId,
                                            activeNote->id,
                                            activeNote->lengthBeats,
                                            "Resize MIDI note (right edge)");
        }

        DBG("Resized note " + activeNote->id +
            " to start=" + juce::String(activeNote->startBeats) +
            ", length=" + juce::String(activeNote->lengthBeats));
    }

    dragStates.clear();
}

//==============================================================================
// Phase 8.2: Multi-Selection
//==============================================================================

void PianoRollComponent::clearSelection()
{
    for (auto& note : noteRects)
        note.selected = false;
}

void PianoRollComponent::selectNote(NoteRect* note, bool addToSelection)
{
    if (!addToSelection)
        clearSelection();

    note->selected = !note->selected;  // Toggle if adding to selection
    repaint();
}

void PianoRollComponent::selectNotesInRectangle(const juce::Rectangle<float>& rect)
{
    for (auto& note : noteRects)
    {
        if (rect.intersects(note.bounds))
            note.selected = true;
    }
    repaint();
}

void PianoRollComponent::startMarqueeSelect(const juce::MouseEvent& e)
{
    currentDragMode = DragMode::MarqueeSelect;
    dragStartPos = e.position;
    marqueeRect = juce::Rectangle<float>(dragStartPos.x, dragStartPos.y, 0.0f, 0.0f);

    if (!e.mods.isCommandDown())
        clearSelection();

    repaint();
}

void PianoRollComponent::updateMarqueeSelect(const juce::MouseEvent& e)
{
    marqueeRect = juce::Rectangle<float>::leftTopRightBottom(
        juce::jmin(dragStartPos.x, e.position.x),
        juce::jmin(dragStartPos.y, e.position.y),
        juce::jmax(dragStartPos.x, e.position.x),
        juce::jmax(dragStartPos.y, e.position.y));

    repaint();
}

void PianoRollComponent::finishMarqueeSelect()
{
    selectNotesInRectangle(marqueeRect);
    marqueeRect = juce::Rectangle<float>();
    repaint();
}

void PianoRollComponent::startMovingSelection(const juce::MouseEvent& e)
{
    currentDragMode = DragMode::MoveNote;
    dragStartPos = e.position;

    // Cache original positions of all selected notes
    dragStates.clear();
    for (const auto& note : noteRects)
    {
        if (note.selected)
        {
            NoteDragState state;
            state.id = note.id;
            state.originalPitch = note.pitch;
            state.originalStartBeats = note.startBeats;
            dragStates.push_back(state);
        }
    }
}

void PianoRollComponent::updateSelectionMove(const juce::MouseEvent& e)
{
    if (dragStates.empty())
        return;

    float deltaX = e.position.x - dragStartPos.x;
    float deltaY = e.position.y - dragStartPos.y;

    double deltaBeats = pixelsToBeats(deltaX) - pixelsToBeats(0);
    int deltaPitch = -static_cast<int>(deltaY / pixelsPerPitch);

    // Update visual positions of all selected notes
    size_t stateIndex = 0;
    for (auto& note : noteRects)
    {
        if (note.selected && stateIndex < dragStates.size())
        {
            const auto& originalState = dragStates[stateIndex];

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

void PianoRollComponent::finishSelectionMove()
{
    if (dragStates.empty())
        return;

    // Commit all selected notes to ProjectState
    // TODO(zenith-core#1): Batch into single undo transaction
    size_t stateIndex = 0;
    for (auto& note : noteRects)
    {
        if (note.selected && stateIndex < dragStates.size())
        {
            const auto& originalState = dragStates[stateIndex];

            bool pitchChanged = note.pitch != originalState.originalPitch;
            bool startChanged = std::abs(note.startBeats - originalState.originalStartBeats) > 0.001;

            if (pitchChanged || startChanged)
            {
                projectState.moveMidiNote(currentClip.clipId,
                                           note.id,
                                           note.startBeats,
                                           note.pitch,
                                           "Move MIDI notes");

                DBG("Moved note " + note.id +
                    " to pitch=" + juce::String(note.pitch) +
                    ", start=" + juce::String(note.startBeats));
            }

            ++stateIndex;
        }
    }

    dragStates.clear();
}

//==============================================================================
// Phase 8.2: Velocity Editing
//==============================================================================

PianoRollComponent::NoteRect* PianoRollComponent::findNoteInVelocityLane(float x)
{
    for (auto& note : noteRects)
    {
        if (note.velocityBounds.contains(x, note.velocityBounds.getCentreY()))
            return &note;
    }
    return nullptr;
}

void PianoRollComponent::startEditingVelocity(NoteRect* note, const juce::MouseEvent& e)
{
    currentDragMode = DragMode::VelocityEdit;
    activeNote = note;
    dragStartPos = e.position;

    // Cache original velocity
    dragStates.clear();
    NoteDragState state;
    state.id = note->id;
    state.originalVelocity = note->velocity;
    dragStates.push_back(state);

    // Select this note
    clearSelection();
    note->selected = true;
    repaint();
}

void PianoRollComponent::updateVelocityEdit(const juce::MouseEvent& e)
{
    if (activeNote == nullptr || dragStates.empty())
        return;

    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - velocityLaneHeight;

    // Calculate velocity based on y position in velocity lane
    float yInLane = e.position.y - noteGridHeight;
    int newVelocity = pixelsToVelocity(yInLane);

    activeNote->velocity = newVelocity;

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::finishVelocityEdit()
{
    if (activeNote == nullptr || dragStates.empty())
        return;

    const auto& originalState = dragStates[0];

    // Check if velocity actually changed
    if (activeNote->velocity != originalState.originalVelocity)
    {
        projectState.setMidiNoteVelocity(currentClip.clipId,
                                          activeNote->id,
                                          activeNote->velocity,
                                          "Edit MIDI velocity");

        DBG("Changed velocity of note " + activeNote->id +
            " to " + juce::String(activeNote->velocity));
    }

    dragStates.clear();
}

//==============================================================================
// Phase 8.2: Zoom & Scroll
//==============================================================================

void PianoRollComponent::zoomHorizontal(float factor, float centerX)
{
    // Zoom around centerX
    double centerBeats = pixelsToBeats(centerX);

    pixelsPerBeat *= factor;
    pixelsPerBeat = juce::jlimit(20.0, 400.0, pixelsPerBeat);  // Clamp zoom

    // Adjust view to keep centerBeats at centerX
    viewStartBeats = centerBeats - (centerX / pixelsPerBeat);
    viewStartBeats = juce::jmax(0.0, viewStartBeats);

    // Update scrollOffsetX for compatibility
    scrollOffsetX = static_cast<int>(viewStartBeats * pixelsPerBeat);

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::zoomVertical(float factor, float centerY)
{
    // Zoom around centerY
    // int centerPitch = pixelsToPitch(centerY);  // Unused variable

    pixelsPerPitch *= factor;
    pixelsPerPitch = juce::jlimit(6.0, 48.0, pixelsPerPitch);  // Clamp zoom

    // Adjust view to keep centerPitch at centerY
    viewLowestPitch = 127 - static_cast<int>((centerY + viewLowestPitch * pixelsPerPitch) / pixelsPerPitch);
    viewLowestPitch = juce::jlimit(0, 127, viewLowestPitch);

    // Update scrollOffsetY for compatibility
    scrollOffsetY = static_cast<int>(viewLowestPitch * pixelsPerPitch);

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::scrollHorizontal(float delta)
{
    scrollOffsetX += static_cast<int>(delta);
    scrollOffsetX = juce::jmax(0, scrollOffsetX);

    viewStartBeats = scrollOffsetX / pixelsPerBeat;

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::scrollVertical(float delta)
{
    scrollOffsetY += static_cast<int>(delta);
    scrollOffsetY = juce::jlimit(0, 127 * static_cast<int>(pixelsPerPitch), scrollOffsetY);

    viewLowestPitch = scrollOffsetY / static_cast<int>(pixelsPerPitch);

    updateNoteRectangles();
    repaint();
}

//==============================================================================
// Keyboard Input
//==============================================================================

bool PianoRollComponent::keyPressed(const juce::KeyPress& key)
{
    // Delete key
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelectedNotes();
        return true;
    }

    // Undo (Cmd/Ctrl + Z)
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
    {
        projectState.undo();
        // ValueTree listener will refresh
        return true;
    }

    // Redo (Cmd/Ctrl + Shift + Z)
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        projectState.redo();
        // ValueTree listener will refresh
        return true;
    }

    // Phase 8.2: Zoom shortcuts
    if (key == juce::KeyPress('+') || key == juce::KeyPress('='))
    {
        // Zoom in horizontal (center of view)
        zoomHorizontal(1.2f, getWidth() / 2.0f);
        return true;
    }

    if (key == juce::KeyPress('-') || key == juce::KeyPress('_'))
    {
        // Zoom out horizontal (center of view)
        zoomHorizontal(0.8f, getWidth() / 2.0f);
        return true;
    }

    return false;
}

//==============================================================================
// ValueTree Listener (auto-refresh on changes)
//==============================================================================

void PianoRollComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.hasType(ProjectState::ID_NOTES))
    {
        DBG("PianoRoll: Note added via external change (undo/redo/Wingman)");
        needsRefresh = true;
    }
}

void PianoRollComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (parent.hasType(ProjectState::ID_NOTES))
    {
        DBG("PianoRoll: Note removed via external change");
        needsRefresh = true;
    }
}

void PianoRollComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType(ProjectState::ID_NOTE))
    {
        DBG("PianoRoll: Note property changed: " + property.toString());
        needsRefresh = true;
    }
}

//==============================================================================
// Timer Callback
//==============================================================================

void PianoRollComponent::timerCallback()
{
    if (needsRefresh)
    {
        refreshNotesFromProjectState();
    }
}

//==============================================================================
// Rendering
//==============================================================================

void PianoRollComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    if (!currentClip.isValid())
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(16.0f));
        g.drawText("No clip loaded", getLocalBounds(), juce::Justification::centred);
        return;
    }

    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - velocityLaneHeight;

    // Phase 8.2: Draw note grid and velocity lane separator
    auto noteGrid = bounds.removeFromTop(static_cast<int>(noteGridHeight));
    auto velocityLane = bounds;

    // Draw piano keys background (alternating white/black keys)
    g.setColour(juce::Colour(0xff3a3a3a));
    for (int pitch = 0; pitch <= 127; ++pitch)
    {
        int note = pitch % 12;
        bool isBlackKey = (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);

        if (isBlackKey)
        {
            float y = pitchToPixels(pitch);
            g.fillRect(0.0f, y, static_cast<float>(noteGrid.getWidth()), static_cast<float>(pixelsPerPitch));
        }
    }

    // Draw grid lines (vertical beat lines)
    g.setColour(juce::Colour(0xff404040));
    for (double beat = 0.0; beat < currentClip.clipLengthBeats; beat += gridBeats)
    {
        float x = beatsToPixels(beat);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(noteGrid.getHeight()));
    }

    // Draw horizontal pitch lines (every octave)
    for (int pitch = 0; pitch <= 127; pitch += 12)
    {
        float y = pitchToPixels(pitch);
        g.setColour(juce::Colour(0xff505050));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(noteGrid.getWidth()));
    }

    // Draw notes (in note grid)
    for (const auto& note : noteRects)
    {
        if (note.muted)
        {
            g.setColour(juce::Colours::darkgrey);
        }
        else if (note.selected)
        {
            g.setColour(juce::Colours::orange);
        }
        else
        {
            g.setColour(juce::Colours::lightblue);
        }

        g.fillRect(note.bounds.reduced(1.0f));

        // Draw note border
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRect(note.bounds, 1.0f);
    }

    // Phase 8.2: Draw velocity lane
    g.setColour(juce::Colour(0xff202020));
    g.fillRect(velocityLane);

    // Velocity lane border
    g.setColour(juce::Colour(0xff505050));
    g.drawHorizontalLine(static_cast<int>(noteGridHeight), 0.0f, static_cast<float>(getWidth()));

    // Draw velocity bars
    for (const auto& note : noteRects)
    {
        if (note.selected)
        {
            g.setColour(juce::Colours::orange.withAlpha(0.7f));
        }
        else
        {
            g.setColour(juce::Colours::lightblue.withAlpha(0.5f));
        }

        g.fillRect(note.velocityBounds.reduced(1.0f, 0.0f));
    }

    // Velocity lane grid lines
    g.setColour(juce::Colour(0xff303030));
    for (int vel = 0; vel <= 127; vel += 32)  // Draw lines at 0, 32, 64, 96, 127
    {
        float y = noteGridHeight + velocityToPixels(vel);
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(getWidth()));
    }

    // Phase 8.2: Draw marquee selection rectangle
    if (currentDragMode == DragMode::MarqueeSelect && !marqueeRect.isEmpty())
    {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillRect(marqueeRect);

        g.setColour(juce::Colours::white);
        g.drawRect(marqueeRect, 1.0f);
    }

    // Draw clip name in corner
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(14.0f));
    g.drawText(currentClip.clipName + " - " + currentClip.clipId,
               noteGrid.removeFromTop(30).reduced(10, 5),
               juce::Justification::centredLeft);
}

void PianoRollComponent::resized()
{
    updateNoteRectangles();
}


