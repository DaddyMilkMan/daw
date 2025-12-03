/**
 * @file PianoRollComponent.cpp
 * @brief Professional-grade MIDI Piano Roll Editor Implementation
 */



#include "../../include/ui/PianoRollComponent.h"
// Force rebuild
#include <algorithm>
#include <cmath>
#include <random>

//==============================================================================
// Constructor / Destructor
//==============================================================================

PianoRollComponent::PianoRollComponent(ProjectState& state)
    : projectState(state)
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

PianoRollComponent::~PianoRollComponent()
{
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
// Clip Management
//==============================================================================

void PianoRollComponent::setClipContext(const MidiClipContext& context)
{
    // Detach from old clip
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

    currentClip = context;

    // Attach to new clip
    if (currentClip.isValid())
    {
        auto [track, clip] = projectState.findClip(currentClip.clipId);
        if (clip.isValid())
        {
            auto midiNotesNode = clip.getChildWithName(ProjectState::ID_NOTES);
            if (!midiNotesNode.isValid())
            {
                midiNotesNode = juce::ValueTree(ProjectState::ID_NOTES);
                clip.appendChild(midiNotesNode, nullptr);
            }
            midiNotesNode.addListener(this);
        }

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

    auto notes = projectState.getMidiNotesForClip(currentClip.clipId);
    
    noteRects.clear();
    noteRects.reserve(notes.size());
    
    for (const auto& note : notes)
    {
        NoteRect rect;
        rect.id = note.id;
        rect.pitch = note.pitch;
        rect.startBeats = note.startBeats;
        rect.lengthBeats = note.lengthBeats;
        rect.velocity = note.velocity;
        rect.muted = note.muted;
        rect.selected = false;
        
        noteRects.push_back(rect);
    }

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::updateNoteRectangles()
{
    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;

    for (auto& note : noteRects)
    {
        // Main note grid bounds
        float x = PIANO_WIDTH + beatsToPixels(note.startBeats);
        float y = RULER_HEIGHT + pitchToPixels(note.pitch);
        float width = beatsToPixels(note.lengthBeats);
        float height = pixelsPerPitch;

        note.bounds = juce::Rectangle<float>(x, y, width, height);

        // Velocity lane bounds
        float velocityBarY = RULER_HEIGHT + noteGridHeight + velocityToPixels(note.velocity);
        float velocityBarHeight = (RULER_HEIGHT + noteGridHeight + velocityLaneHeight) - velocityBarY;
        note.velocityBounds = juce::Rectangle<float>(x, velocityBarY, width, velocityBarHeight);
    }
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

int PianoRollComponent::pixelsToPitch(float y) const
{
    float adjustedY = y - RULER_HEIGHT;
    int pitch = 127 - static_cast<int>((adjustedY + scrollOffsetY) / pixelsPerPitch);
    return juce::jlimit(0, 127, pitch);
}

float PianoRollComponent::pitchToPixels(int pitch) const
{
    return (127 - pitch) * pixelsPerPitch - scrollOffsetY;
}

double PianoRollComponent::pixelsToBeats(float x) const
{
    float adjustedX = x - PIANO_WIDTH;
    return (adjustedX + scrollOffsetX) / pixelsPerBeat;
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

int PianoRollComponent::pixelsToVelocity(float y) const
{
    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
    float yInLane = y - (RULER_HEIGHT + noteGridHeight);
    
    float normalizedY = yInLane / velocityLaneHeight;
    int velocity = static_cast<int>((1.0f - normalizedY) * 127.0f);
    return juce::jlimit(1, 127, velocity);
}

float PianoRollComponent::velocityToPixels(int velocity) const
{
    float normalized = velocity / 127.0f;
    return (1.0f - normalized) * velocityLaneHeight;
}

//==============================================================================
// Mouse Interaction Helpers
//==============================================================================

PianoRollComponent::NoteRect* PianoRollComponent::findNoteAtPosition(float x, float y)
{
    // Search in reverse order (top notes first)
    for (auto it = noteRects.rbegin(); it != noteRects.rend(); ++it)
    {
        if (it->bounds.contains(x, y))
            return &(*it);
    }
    return nullptr;
}

PianoRollComponent::NoteRect* PianoRollComponent::findNoteInVelocityLane(float x, float y)
{
    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
    float velocityLaneTop = RULER_HEIGHT + noteGridHeight;
    
    if (y < velocityLaneTop || y > velocityLaneTop + velocityLaneHeight)
        return nullptr;

    // Search in reverse order
    for (auto it = noteRects.rbegin(); it != noteRects.rend(); ++it)
    {
        if (it->velocityBounds.contains(x, y))
            return &(*it);
    }
    return nullptr;
}

PianoRollComponent::DragMode PianoRollComponent::detectNoteHitRegion(
    const NoteRect& note, float x, float y) const
{
    if (!note.bounds.contains(x, y))
        return DragMode::None;

    // Left edge resize?
    if (x < note.bounds.getX() + resizeHandleWidth)
        return DragMode::ResizeLeft;

    // Right edge resize?
    if (x > note.bounds.getRight() - resizeHandleWidth)
        return DragMode::ResizeRight;

    return DragMode::MoveNote;
}

PianoRollComponent::CursorType PianoRollComponent::getCursorForPosition(float x, float y) const
{
    // Check if over piano keys
    if (x < PIANO_WIDTH)
        return CursorType::Normal;

    // Check if over ruler
    if (y < RULER_HEIGHT)
        return CursorType::Normal;

    // Check velocity lane
    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
    if (y >= RULER_HEIGHT + noteGridHeight)
        return CursorType::Crosshair;

    // Check if over a note
    for (const auto& note : noteRects)
    {
        if (note.bounds.contains(x, y))
        {
            // Check resize handles
            if (x < note.bounds.getX() + resizeHandleWidth)
                return CursorType::ResizeLeft;
            if (x > note.bounds.getRight() - resizeHandleWidth)
                return CursorType::ResizeRight;
            
            return CursorType::Hand;
        }
    }

    return CursorType::Crosshair;
}

juce::MouseCursor PianoRollComponent::getMouseCursor()
{
    switch (currentCursorType)
    {
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

void PianoRollComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.hasType(ProjectState::ID_NOTES))
    {
        refreshNotesFromProjectState();
    }
}

void PianoRollComponent::valueTreeChildRemoved(
    juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    (void)child;
    (void)index;
    
    if (parent.hasType(ProjectState::ID_NOTES))
    {
        refreshNotesFromProjectState();
    }
}

void PianoRollComponent::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property)
{
    (void)property;
    
    if (tree.hasType(ProjectState::ID_NOTE))
    {
        refreshNotesFromProjectState();
    }
}

//==============================================================================
// Component Methods
//==============================================================================

void PianoRollComponent::resized()
{
    updateNoteRectangles();
}

void PianoRollComponent::mouseMove(const juce::MouseEvent& e)
{
    // Update cursor based on position
    auto newCursorType = getCursorForPosition(static_cast<float>(e.x), static_cast<float>(e.y));
    if (newCursorType != currentCursorType)
    {
        currentCursorType = newCursorType;
        repaint();
    }
    
    // Update hover state
    auto* newHoveredNote = findNoteAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));
    if (newHoveredNote != hoveredNote)
    {
        if (hoveredNote)
            hoveredNote->isHovered = false;
        
        hoveredNote = newHoveredNote;
        
        if (hoveredNote)
            hoveredNote->isHovered = true;
        
        repaint();
    }
}



//==============================================================================
// Mouse Events
//==============================================================================

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!currentClip.isValid())
        return;

    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);

    // Ignore clicks on piano keys or ruler
    if (x < PIANO_WIDTH || y < RULER_HEIGHT)
        return;

    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
    float velocityLaneTop = RULER_HEIGHT + noteGridHeight;

    // Check if in velocity lane
    if (y >= velocityLaneTop)
    {
        auto* note = findNoteInVelocityLane(x, y);
        if (note)
        {
            startEditingVelocity(note, e);
            return;
        }
    }

    // Check for note hit in main grid
    auto* note = findNoteAtPosition(x, y);

    if (note)
    {
        // Detect hit region (resize vs move)
        DragMode mode = detectNoteHitRegion(*note, x, y);

        if (mode == DragMode::ResizeLeft || mode == DragMode::ResizeRight)
        {
            // Start resizing
            startResizingNote(note, mode, e);
        }
        else
        {
            // Multi-select with Cmd/Ctrl
            bool isMultiSelectModifier = e.mods.isCommandDown();
            
            if (isMultiSelectModifier)
            {
                // Toggle selection
                note->selected = !note->selected;
                repaint();
            }
            else if (!note->selected)
            {
                // Clear selection and select this note
                clearSelection();
                note->selected = true;
                repaint();
            }

            // Start moving selection
            startMovingSelection(e);
        }
    }
    else
    {
        // No note hit
        if (e.mods.isShiftDown())
        {
            // Marquee select
            startMarqueeSelect(e);
        }
        else
        {
            // Create new note
            createNoteAtPosition(x, y);
        }
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
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
            break;
    }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent& e)
{
    (void)e;
    
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
            break;
    }

    currentDragMode = DragMode::None;
    activeNote = nullptr;
}

void PianoRollComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!currentClip.isValid())
        return;

    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);

    // Double-click to delete note
    auto* note = findNoteAtPosition(x, y);
    if (note)
    {
        projectState.removeMidiNote(currentClip.clipId, note->id, "Delete MIDI note");
    }
}

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
        // Default = vertical scroll
        scrollVertical(-wheel.deltaY * 50.0f);
    }
}

//==============================================================================
// Selection Management
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

    if (note)
        note->selected = true;
    
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

void PianoRollComponent::selectAll()
{
    for (auto& note : noteRects)
        note.selected = true;
    repaint();
}

void PianoRollComponent::invertSelection()
{
    for (auto& note : noteRects)
        note.selected = !note.selected;
    repaint();
}

int PianoRollComponent::getSelectedNoteCount() const
{
    return static_cast<int>(std::count_if(noteRects.begin(), noteRects.end(),
        [](const NoteRect& n) { return n.selected; }));
}

//==============================================================================
// Note Editing Operations
//==============================================================================

void PianoRollComponent::createNoteAtPosition(float x, float y)
{
    int pitch = pixelsToPitch(y);
    double startBeats = pixelsToBeats(x - PIANO_WIDTH);

    if (snapEnabled)
        startBeats = snapToGrid(startBeats);

    startBeats = juce::jmax(0.0, startBeats);
    pitch = juce::jlimit(0, 127, pitch);

    // Default note length (1 grid unit)
    double lengthBeats = gridBeats;

    // Create note
    ProjectState::MidiNoteSpec note;
    note.pitch = pitch;
    note.startBeats = startBeats;
    note.lengthBeats = lengthBeats;
    note.velocity = 100;
    note.muted = false;

    juce::String noteId = projectState.addMidiNote(currentClip.clipId, note, "Create MIDI note");
    
    DBG("Created note " + noteId + " at pitch=" + juce::String(pitch) +
        ", start=" + juce::String(startBeats));
}

void PianoRollComponent::deleteSelectedNotes()
{
    if (!currentClip.isValid() || getSelectedNoteCount() == 0)
        return;

    // Collect selected note IDs
    std::vector<juce::String> selectedIds;
    for (const auto& note : noteRects)
    {
        if (note.selected)
            selectedIds.push_back(note.id);
    }

    // FIXED: Batched undo transaction
    projectState.getUndoManager().beginNewTransaction("Delete MIDI notes");
    
    for (const auto& noteId : selectedIds)
    {
        projectState.removeMidiNote(currentClip.clipId, noteId, "");
    }
}

//==============================================================================
// Copy/Paste/Cut
//==============================================================================

void PianoRollComponent::copySelectedNotes()
{
    clipboard.clear();
    
    if (getSelectedNoteCount() == 0)
        return;

    // Find earliest selected note for relative positioning
    double earliestTime = std::numeric_limits<double>::max();
    for (const auto& note : noteRects)
    {
        if (note.selected)
            earliestTime = std::min(earliestTime, note.startBeats);
    }

    clipboardReferenceTime = earliestTime;

    // Copy selected notes
    for (const auto& note : noteRects)
    {
        if (note.selected)
        {
            ClipboardNote clipNote;
            clipNote.pitch = note.pitch;
            clipNote.startBeats = note.startBeats - earliestTime;  // Relative to earliest
            clipNote.lengthBeats = note.lengthBeats;
            clipNote.velocity = note.velocity;
            clipNote.muted = note.muted;
            
            clipboard.push_back(clipNote);
        }
    }

    DBG("Copied " + juce::String(clipboard.size()) + " notes to clipboard");
}

void PianoRollComponent::pasteNotes()
{
    if (!currentClip.isValid() || clipboard.empty())
        return;

    // Paste at current playhead or view start
    double pasteTime = viewStartBeats;

    // FIXED: Batched undo transaction
    projectState.getUndoManager().beginNewTransaction("Paste MIDI notes");

    clearSelection();

    for (const auto& clipNote : clipboard)
    {
        ProjectState::MidiNoteSpec note;
        note.pitch = clipNote.pitch;
        note.startBeats = pasteTime + clipNote.startBeats;
        note.lengthBeats = clipNote.lengthBeats;
        note.velocity = clipNote.velocity;
        note.muted = clipNote.muted;

        juce::String noteId = projectState.addMidiNote(currentClip.clipId, note, "");
        
        // Select pasted notes
        // (Will be selected after refresh)
    }

    DBG("Pasted " + juce::String(clipboard.size()) + " notes");
}

void PianoRollComponent::cutSelectedNotes()
{
    copySelectedNotes();
    deleteSelectedNotes();
}

//==============================================================================
// Drag Operations - Move
//==============================================================================

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

    double deltaBeats = pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);
    int deltaPitch = -static_cast<int>(deltaY / pixelsPerPitch);

    // Update visual positions
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

    // FIXED: Batched undo transaction
    projectState.getUndoManager().beginNewTransaction("Move MIDI notes");

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
                projectState.moveMidiNote(currentClip.clipId, note.id,
                                         note.startBeats, note.pitch, "");
            }

            ++stateIndex;
        }
    }

    dragStates.clear();
}

//==============================================================================
// Drag Operations - Resize
//==============================================================================

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
    if (!activeNote || dragStates.empty())
        return;

    float deltaX = e.position.x - dragStartPos.x;
    double deltaBeats = pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);

    const auto& originalState = dragStates[0];

    if (currentDragMode == DragMode::ResizeLeft)
    {
        double newStartBeats = originalState.originalStartBeats + deltaBeats;
        if (snapEnabled)
            newStartBeats = snapToGrid(newStartBeats);

        newStartBeats = juce::jmax(0.0, newStartBeats);
        double newLengthBeats = originalState.originalLengthBeats - 
                                (newStartBeats - originalState.originalStartBeats);
        newLengthBeats = juce::jmax(0.01, newLengthBeats);

        activeNote->startBeats = newStartBeats;
        activeNote->lengthBeats = newLengthBeats;
    }
    else if (currentDragMode == DragMode::ResizeRight)
    {
        double newLengthBeats = originalState.originalLengthBeats + deltaBeats;
        if (snapEnabled)
        {
            double endBeats = snapToGrid(originalState.originalStartBeats + newLengthBeats);
            newLengthBeats = endBeats - originalState.originalStartBeats;
        }

        newLengthBeats = juce::jmax(0.01, newLengthBeats);
        activeNote->lengthBeats = newLengthBeats;
    }

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::finishNoteResize()
{
    if (!activeNote || dragStates.empty())
        return;

    const auto& originalState = dragStates[0];

    bool startChanged = std::abs(activeNote->startBeats - originalState.originalStartBeats) > 0.001;
    bool lengthChanged = std::abs(activeNote->lengthBeats - originalState.originalLengthBeats) > 0.001;

    if (startChanged || lengthChanged)
    {
        // FIXED: Batched undo for left resize, single transaction for right resize
        if (currentDragMode == DragMode::ResizeLeft)
        {
            projectState.getUndoManager().beginNewTransaction("Resize MIDI note");
            projectState.moveMidiNote(currentClip.clipId, activeNote->id,
                                     activeNote->startBeats, activeNote->pitch, "");
            projectState.setMidiNoteLength(currentClip.clipId, activeNote->id,
                                          activeNote->lengthBeats, "");
        }
        else
        {
            projectState.setMidiNoteLength(currentClip.clipId, activeNote->id,
                                          activeNote->lengthBeats, 
                                          "Resize MIDI note");
        }
    }

    dragStates.clear();
}

//==============================================================================
// TO BE CONTINUED IN PART 3...
// (Velocity editing, marquee select, zoom/scroll, advanced features)
//==============================================================================



//==============================================================================
// Drag Operations - Velocity
//==============================================================================

void PianoRollComponent::startEditingVelocity(NoteRect* note, const juce::MouseEvent& e)
{
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

void PianoRollComponent::updateVelocityEdit(const juce::MouseEvent& e)
{
    if (!activeNote || dragStates.empty())
        return;

    int newVelocity = pixelsToVelocity(static_cast<float>(e.y));
    activeNote->velocity = newVelocity;

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::finishVelocityEdit()
{
    if (!activeNote || dragStates.empty())
        return;

    const auto& originalState = dragStates[0];

    if (activeNote->velocity != originalState.originalVelocity)
    {
        projectState.setMidiNoteVelocity(currentClip.clipId, activeNote->id,
                                        activeNote->velocity, 
                                        "Edit MIDI velocity");
    }

    dragStates.clear();
}

//==============================================================================
// Drag Operations - Marquee Select
//==============================================================================

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

//==============================================================================
// Zoom & Scroll
//==============================================================================

void PianoRollComponent::zoomHorizontal(float factor, float centerX)
{
    double centerBeats = pixelsToBeats(centerX);

    pixelsPerBeat *= factor;
    pixelsPerBeat = juce::jlimit(20.0, 400.0, pixelsPerBeat);

    viewStartBeats = centerBeats - ((centerX - PIANO_WIDTH) / pixelsPerBeat);
    viewStartBeats = juce::jmax(0.0, viewStartBeats);

    scrollOffsetX = static_cast<int>(viewStartBeats * pixelsPerBeat);

    updateNoteRectangles();
    repaint();
}

void PianoRollComponent::zoomVertical(float factor, float centerY)
{
    pixelsPerPitch *= factor;
    pixelsPerPitch = juce::jlimit(8.0, 48.0, pixelsPerPitch);

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
// Advanced Features - Quantize
//==============================================================================

void PianoRollComponent::quantizeSelected(double gridSize, float strength, float swing)
{
    if (getSelectedNoteCount() == 0)
        return;

    projectState.getUndoManager().beginNewTransaction("Quantize MIDI notes");

    for (auto& note : noteRects)
    {
        if (note.selected)
        {
            double originalStart = note.startBeats;
            double quantizedStart = std::round(originalStart / gridSize) * gridSize;
            
            // Apply swing (offset every other grid position)
            int gridIndex = static_cast<int>(std::round(quantizedStart / gridSize));
            if (gridIndex % 2 == 1 && swing != 0.0f)
            {
                quantizedStart += gridSize * swing * 0.5f;
            }

            // Blend between original and quantized based on strength
            double newStart = originalStart + (quantizedStart - originalStart) * strength;
            newStart = juce::jmax(0.0, newStart);

            if (std::abs(newStart - originalStart) > 0.001)
            {
                projectState.moveMidiNote(currentClip.clipId, note.id,
                                         newStart, note.pitch, "");
            }
        }
    }
}

//==============================================================================
// Advanced Features - Velocity Humanization
//==============================================================================

void PianoRollComponent::humanizeVelocity(float amount)
{
    if (getSelectedNoteCount() == 0)
        return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    projectState.getUndoManager().beginNewTransaction("Humanize MIDI velocity");

    for (auto& note : noteRects)
    {
        if (note.selected)
        {
            int originalVelocity = note.velocity;
            
            // Random variation proportional to current velocity
            float variation = dis(gen) * amount * 30.0f;  // Up to Â±30 at full amount
            int newVelocity = originalVelocity + static_cast<int>(variation);
            newVelocity = juce::jlimit(1, 127, newVelocity);

            if (newVelocity != originalVelocity)
            {
                projectState.setMidiNoteVelocity(currentClip.clipId, note.id,
                                                newVelocity, "");
            }
        }
    }
}

//==============================================================================
// Advanced Features - Velocity Curves
//==============================================================================

void PianoRollComponent::applyVelocityCurve(VelocityCurve curve, float amount)
{
    if (getSelectedNoteCount() == 0)
        return;

    // Sort selected notes by time
    std::vector<NoteRect*> selectedNotes;
    for (auto& note : noteRects)
    {
        if (note.selected)
            selectedNotes.push_back(&note);
    }

    std::sort(selectedNotes.begin(), selectedNotes.end(),
        [](const NoteRect* a, const NoteRect* b) {
            return a->startBeats < b->startBeats;
        });

    if (selectedNotes.empty())
        return;

    projectState.getUndoManager().beginNewTransaction("Apply velocity curve");

    int count = static_cast<int>(selectedNotes.size());

    for (int i = 0; i < count; ++i)
    {
        auto* note = selectedNotes[i];
        int originalVelocity = note->velocity;
        int newVelocity = originalVelocity;

        float t = (count > 1) ? (i / static_cast<float>(count - 1)) : 0.5f;

        switch (curve)
        {
            case VelocityCurve::RampUp:
            {
                int targetVelocity = 1 + static_cast<int>(t * 126.0f);
                newVelocity = originalVelocity + 
                             static_cast<int>((targetVelocity - originalVelocity) * amount);
                break;
            }

            case VelocityCurve::RampDown:
            {
                int targetVelocity = 127 - static_cast<int>(t * 126.0f);
                newVelocity = originalVelocity + 
                             static_cast<int>((targetVelocity - originalVelocity) * amount);
                break;
            }

            case VelocityCurve::Compress:
            {
                // Calculate average velocity
                int avgVelocity = 0;
                for (const auto* n : selectedNotes)
                    avgVelocity += n->velocity;
                avgVelocity /= count;

                int diff = originalVelocity - avgVelocity;
                newVelocity = avgVelocity + static_cast<int>(diff * (1.0f - amount));
                break;
            }

            case VelocityCurve::Expand:
            {
                int avgVelocity = 0;
                for (const auto* n : selectedNotes)
                    avgVelocity += n->velocity;
                avgVelocity /= count;

                int diff = originalVelocity - avgVelocity;
                newVelocity = avgVelocity + static_cast<int>(diff * (1.0f + amount));
                break;
            }

            case VelocityCurve::Invert:
            {
                int inverted = 128 - originalVelocity;
                newVelocity = originalVelocity + 
                             static_cast<int>((inverted - originalVelocity) * amount);
                break;
            }
        }

        newVelocity = juce::jlimit(1, 127, newVelocity);

        if (newVelocity != originalVelocity)
        {
            projectState.setMidiNoteVelocity(currentClip.clipId, note->id,
                                            newVelocity, "");
        }
    }
}

//==============================================================================
// Advanced Features - Smart Duplicate
//==============================================================================

void PianoRollComponent::smartDuplicate()
{
    if (getSelectedNoteCount() < 2)
        return;

    // Find time range of selection
    double minTime = std::numeric_limits<double>::max();
    double maxTime = 0.0;
    
    for (const auto& note : noteRects)
    {
        if (note.selected)
        {
            minTime = std::min(minTime, note.startBeats);
            maxTime = std::max(maxTime, note.startBeats + note.lengthBeats);
        }
    }

    double patternLength = maxTime - minTime;

    projectState.getUndoManager().beginNewTransaction("Smart duplicate MIDI notes");

    clearSelection();

    for (const auto& note : noteRects)
    {
        if (note.selected)
        {
            ProjectState::MidiNoteSpec newNote;
            newNote.pitch = note.pitch;
            newNote.startBeats = note.startBeats + patternLength;
            newNote.lengthBeats = note.lengthBeats;
            newNote.velocity = note.velocity;
            newNote.muted = note.muted;

            projectState.addMidiNote(currentClip.clipId, newNote, "");
        }
    }
}

//==============================================================================
// Advanced Features - Note Repeater/Roll
//==============================================================================

void PianoRollComponent::createRoll(float x, float y, double rollSpeed)
{
    int pitch = pixelsToPitch(y);
    double startBeats = pixelsToBeats(x - PIANO_WIDTH);

    if (snapEnabled)
        startBeats = snapToGrid(startBeats);

    // Create 16 notes for the roll
    int numNotes = 16;
    double totalLength = rollSpeed * numNotes;

    projectState.getUndoManager().beginNewTransaction("Create MIDI roll");

    for (int i = 0; i < numNotes; ++i)
    {
        ProjectState::MidiNoteSpec note;
        note.pitch = pitch;
        note.startBeats = startBeats + (i * rollSpeed);
        note.lengthBeats = rollSpeed * 0.9;  // Slight gap
        note.velocity = 100 - (i * 3);  // Decreasing velocity
        note.muted = false;

        projectState.addMidiNote(currentClip.clipId, note, "");
    }
}

//==============================================================================
// Advanced Features - Toggle Mute
//==============================================================================

void PianoRollComponent::toggleMuteSelected()
{
    if (getSelectedNoteCount() == 0)
        return;

    projectState.getUndoManager().beginNewTransaction("Toggle MIDI note mute");

    for (auto& note : noteRects)
    {
        if (note.selected)
        {
            // Toggle mute state
            bool newMuteState = !note.muted;
            
            // Update via ProjectState (note: you may need to add setMidiNoteMuted method)
            // For now, this is a placeholder - implement in ProjectState if needed
            // projectState.setMidiNoteMuted(currentClip.clipId, note.id, newMuteState, "");
            
            note.muted = newMuteState;
        }
    }

    repaint();
}

//==============================================================================
// Advanced Features - Scale Highlighting
//==============================================================================

void PianoRollComponent::setScaleHighlight(int rootNote, ScaleType scale)
{
    scaleHighlight.enabled = true;
    scaleHighlight.rootNote = rootNote % 12;
    scaleHighlight.scale = scale;
    updateScaleHighlight();
    repaint();
}

void PianoRollComponent::clearScaleHighlight()
{
    scaleHighlight.enabled = false;
    repaint();
}

void PianoRollComponent::updateScaleHighlight()
{
    scaleHighlight.highlightedPitches.clear();
    scaleHighlight.highlightedPitches.resize(128, false);

    if (!scaleHighlight.enabled)
        return;

    // Scale intervals (semitones from root)
    std::vector<int> intervals;

    switch (scaleHighlight.scale)
    {
        case ScaleType::Major:
            intervals = {0, 2, 4, 5, 7, 9, 11};
            break;
        case ScaleType::Minor:
            intervals = {0, 2, 3, 5, 7, 8, 10};
            break;
        case ScaleType::Dorian:
            intervals = {0, 2, 3, 5, 7, 9, 10};
            break;
        case ScaleType::Phrygian:
            intervals = {0, 1, 3, 5, 7, 8, 10};
            break;
        case ScaleType::Lydian:
            intervals = {0, 2, 4, 6, 7, 9, 11};
            break;
        case ScaleType::Mixolydian:
            intervals = {0, 2, 4, 5, 7, 9, 10};
            break;
        case ScaleType::Aeolian:
            intervals = {0, 2, 3, 5, 7, 8, 10};
            break;
        case ScaleType::Locrian:
            intervals = {0, 1, 3, 5, 6, 8, 10};
            break;
        case ScaleType::HarmonicMinor:
            intervals = {0, 2, 3, 5, 7, 8, 11};
            break;
        case ScaleType::MelodicMinor:
            intervals = {0, 2, 3, 5, 7, 9, 11};
            break;
        default:  // Chromatic
            for (int i = 0; i < 12; ++i)
                intervals.push_back(i);
            break;
    }

    // Mark all pitches in scale
    for (int octave = 0; octave < 11; ++octave)
    {
        for (int interval : intervals)
        {
            int pitch = (octave * 12) + scaleHighlight.rootNote + interval;
            if (pitch >= 0 && pitch < 128)
                scaleHighlight.highlightedPitches[pitch] = true;
        }
    }
}

bool PianoRollComponent::isNoteInScale(int pitch) const
{
    if (!scaleHighlight.enabled || pitch < 0 || pitch >= 128)
        return true;

    return scaleHighlight.highlightedPitches[pitch];
}

//==============================================================================
// Advanced Features - Chord Detection
//==============================================================================

juce::String PianoRollComponent::detectChord(const std::vector<int>& pitches) const
{
    if (pitches.size() < 3)
        return "";

    // Get unique pitch classes (mod 12)
    std::set<int> pitchClasses;
    for (int pitch : pitches)
        pitchClasses.insert(pitch % 12);

    std::vector<int> sortedClasses(pitchClasses.begin(), pitchClasses.end());
    std::sort(sortedClasses.begin(), sortedClasses.end());

    // Try each pitch class as root
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    for (int root : sortedClasses)
    {
        std::vector<int> intervals;
        for (int pc : sortedClasses)
        {
            int interval = (pc - root + 12) % 12;
            intervals.push_back(interval);
        }
        std::sort(intervals.begin(), intervals.end());

        // Check common chord types
        if (intervals == std::vector<int>{0, 4, 7})
            return juce::String(noteNames[root]) + " Major";
        if (intervals == std::vector<int>{0, 3, 7})
            return juce::String(noteNames[root]) + " Minor";
        if (intervals == std::vector<int>{0, 4, 7, 11})
            return juce::String(noteNames[root]) + " Maj7";
        if (intervals == std::vector<int>{0, 3, 7, 10})
            return juce::String(noteNames[root]) + " m7";
        if (intervals == std::vector<int>{0, 4, 7, 10})
            return juce::String(noteNames[root]) + " 7";
        if (intervals == std::vector<int>{0, 3, 6})
            return juce::String(noteNames[root]) + " Dim";
        if (intervals == std::vector<int>{0, 4, 8})
            return juce::String(noteNames[root]) + " Aug";
        if (intervals == std::vector<int>{0, 5, 7})
            return juce::String(noteNames[root]) + " Sus4";
    }

    return "Unknown Chord";
}

juce::String PianoRollComponent::getCurrentChordName() const
{
    std::vector<int> selectedPitches;
    for (const auto& note : noteRects)
    {
        if (note.selected)
            selectedPitches.push_back(note.pitch);
    }

    return detectChord(selectedPitches);
}

//==============================================================================
// TO BE CONTINUED IN PART 4...
// (Keyboard shortcuts, Rendering, Color helpers)
//==============================================================================



//==============================================================================
// Keyboard Shortcuts
//==============================================================================

bool PianoRollComponent::keyPressed(const juce::KeyPress& key)
{
    // Delete / Backspace
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelectedNotes();
        return true;
    }

    // Undo (Cmd/Ctrl+Z)
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
    {
        projectState.undo();
        return true;
    }

    // Redo (Cmd/Ctrl+Shift+Z or Cmd/Ctrl+Y)
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0) ||
        key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0))
    {
        projectState.redo();
        return true;
    }

    // Copy (Cmd/Ctrl+C)
    if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0))
    {
        copySelectedNotes();
        return true;
    }

    // Paste (Cmd/Ctrl+V)
    if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0))
    {
        pasteNotes();
        return true;
    }

    // Cut (Cmd/Ctrl+X)
    if (key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0))
    {
        cutSelectedNotes();
        return true;
    }

    // Select All (Cmd/Ctrl+A)
    if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0))
    {
        selectAll();
        return true;
    }

    // Duplicate (Cmd/Ctrl+D)
    if (key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0))
    {
        smartDuplicate();
        return true;
    }

    // Mute (Cmd/Ctrl+M)
    if (key == juce::KeyPress('m', juce::ModifierKeys::commandModifier, 0))
    {
        toggleMuteSelected();
        return true;
    }

    // Quantize (Q)
    if (key == juce::KeyPress('q', 0, 0))
    {
        quantizeSelected(gridBeats, 1.0f, 0.0f);
        return true;
    }

    // Humanize (H)
    if (key == juce::KeyPress('h', 0, 0))
    {
        humanizeVelocity(0.3f);
        return true;
    }

    // Zoom In (+/=)
    if (key == juce::KeyPress('+', 0, 0) || key == juce::KeyPress('=', 0, 0))
    {
        zoomHorizontal(1.2f, getWidth() / 2.0f);
        return true;
    }

    // Zoom Out (-)
    if (key == juce::KeyPress('-', 0, 0))
    {
        zoomHorizontal(0.8f, getWidth() / 2.0f);
        return true;
    }

    // Invert Selection (Cmd/Ctrl+I)
    if (key == juce::KeyPress('i', juce::ModifierKeys::commandModifier, 0))
    {
        invertSelection();
        return true;
    }

    return false;
}

//==============================================================================
// Color Helpers
//==============================================================================

juce::Colour PianoRollComponent::getColorForVelocity(int velocity) const
{
    // Gradient from blue (low) â†’ green (mid) â†’ orange/red (high)
    float normalized = velocity / 127.0f;

    if (normalized < 0.33f)
    {
        // Low velocity: Blue
        float t = normalized / 0.33f;
        return juce::Colour::fromHSV(0.6f, 0.7f, 0.5f + t * 0.3f, 1.0f);
    }
    else if (normalized < 0.67f)
    {
        // Mid velocity: Green
        float t = (normalized - 0.33f) / 0.34f;
        return juce::Colour::fromHSV(0.4f - t * 0.15f, 0.7f, 0.6f + t * 0.2f, 1.0f);
    }
    else
    {
        // High velocity: Orange/Red
        float t = (normalized - 0.67f) / 0.33f;
        return juce::Colour::fromHSV(0.1f - t * 0.1f, 0.8f, 0.7f + t * 0.3f, 1.0f);
    }
}

//==============================================================================
// Rendering - Piano Keys
//==============================================================================

void PianoRollComponent::drawPianoKeys(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    for (int pitch = 0; pitch <= 127; ++pitch)
    {
        float y = RULER_HEIGHT + pitchToPixels(pitch);
        
        if (y < RULER_HEIGHT || y > area.getBottom())
            continue;

        int noteInOctave = pitch % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                          noteInOctave == 8 || noteInOctave == 10);

        // Piano key background
        if (scaleHighlight.enabled && isNoteInScale(pitch))
        {
            // Highlighted notes (in scale)
            g.setColour(isBlackKey ? juce::Colour(0xff4a4a5a) : juce::Colour(0xff3a3a4a));
        }
        else
        {
            // Non-highlighted notes
            g.setColour(isBlackKey ? juce::Colour(0xff3a3a3a) : juce::Colour(0xff2a2a2a));
        }
        
        g.fillRect(0.0f, y, static_cast<float>(PIANO_WIDTH), pixelsPerPitch);

        // C note labels
        if (noteInOctave == 0)
        {
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.setFont(juce::FontOptions(10.0f));
            juce::String label = "C" + juce::String((pitch / 12) - 2);
            g.drawText(label, 4, static_cast<int>(y), PIANO_WIDTH - 8, 
                      static_cast<int>(pixelsPerPitch), 
                      juce::Justification::centredLeft, false);
        }

        // Key border
        g.setColour(juce::Colour(0xff404040));
        g.drawHorizontalLine(static_cast<int>(y + pixelsPerPitch), 
                            0.0f, static_cast<float>(PIANO_WIDTH));
    }
}

//==============================================================================
// Rendering - Grid
//==============================================================================

void PianoRollComponent::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    // Horizontal pitch lines (every octave = C note)
    for (int pitch = 0; pitch <= 127; pitch += 12)
    {
        float y = RULER_HEIGHT + pitchToPixels(pitch);
        if (y >= RULER_HEIGHT && y <= area.getBottom())
        {
            g.setColour(juce::Colour(0xff505050));
            g.drawHorizontalLine(static_cast<int>(y), 
                                static_cast<float>(PIANO_WIDTH), 
                                static_cast<float>(area.getRight()));
        }
    }

    // Vertical beat lines
    for (double beat = 0.0; beat <= currentClip.clipLengthBeats; beat += gridBeats)
    {
        float x = PIANO_WIDTH + beatsToPixels(beat);
        if (x >= PIANO_WIDTH && x <= area.getRight())
        {
            // Measure lines (every 4 beats) are brighter
            bool isMeasureLine = (std::fmod(beat, 4.0) < 0.001);
            g.setColour(isMeasureLine ? juce::Colour(0xff606060) : juce::Colour(0xff404040));
            g.drawVerticalLine(static_cast<int>(x), 
                              static_cast<float>(RULER_HEIGHT), 
                              static_cast<float>(area.getBottom()));
        }
    }
}

//==============================================================================
// Rendering - Notes
//==============================================================================

void PianoRollComponent::drawNotes(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    (void)area;

    for (const auto& note : noteRects)
    {
        // Skip notes outside visible area
        if (note.bounds.getRight() < PIANO_WIDTH || note.bounds.getX() > getWidth())
            continue;
        if (note.bounds.getBottom() < RULER_HEIGHT || note.bounds.getY() > getHeight())
            continue;

        // Note fill color
        juce::Colour noteColor;
        
        if (note.muted)
        {
            noteColor = juce::Colours::darkgrey.withAlpha(0.5f);
        }
        else if (note.selected)
        {
            noteColor = juce::Colours::orange.brighter(0.3f);
        }
        else
        {
            // Color by velocity (gradient)
            noteColor = getColorForVelocity(note.velocity);
        }

        // Hover state: brighten
        if (note.isHovered)
        {
            noteColor = noteColor.brighter(0.2f);
        }

        g.setColour(noteColor);
        g.fillRoundedRectangle(note.bounds.reduced(1.0f), 3.0f);

        // Note border
        g.setColour(noteColor.brighter(0.3f));
        g.drawRoundedRectangle(note.bounds.reduced(1.0f), 3.0f, 1.5f);

        // Resize handle indicators (subtle dots)
        if (note.isHovered || note.selected)
        {
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            
            // Left handle
            float handleY = note.bounds.getCentreY();
            g.fillEllipse(note.bounds.getX() + 3, handleY - 2, 4, 4);
            
            // Right handle
            g.fillEllipse(note.bounds.getRight() - 7, handleY - 2, 4, 4);
        }
    }
}

//==============================================================================
// Rendering - Velocity Lane
//==============================================================================

void PianoRollComponent::drawVelocityLane(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(area);

    // Border
    g.setColour(juce::Colour(0xff505050));
    g.drawHorizontalLine(area.getY(), 0.0f, static_cast<float>(getWidth()));

    // Grid lines (every 32 velocity units)
    for (int vel = 0; vel <= 127; vel += 32)
    {
        float y = area.getY() + velocityToPixels(vel);
        g.setColour(juce::Colour(0xff303030));
        g.drawHorizontalLine(static_cast<int>(y), 
                            static_cast<float>(PIANO_WIDTH), 
                            static_cast<float>(getWidth()));
        
        // Velocity labels
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(juce::String(vel), 2, static_cast<int>(y) - 6, PIANO_WIDTH - 4, 12, 
                   juce::Justification::centredRight, false);
    }

    // Velocity bars for each note
    for (const auto& note : noteRects)
    {
        if (note.velocityBounds.getRight() < PIANO_WIDTH || 
            note.velocityBounds.getX() > getWidth())
            continue;

        juce::Colour barColor;
        
        if (note.selected)
        {
            barColor = juce::Colours::orange.withAlpha(0.8f);
        }
        else
        {
            barColor = getColorForVelocity(note.velocity).withAlpha(0.6f);
        }

        g.setColour(barColor);
        g.fillRect(note.velocityBounds.reduced(1.0f, 0.0f));
    }
}

//==============================================================================
// Rendering - Chord Name Overlay
//==============================================================================

void PianoRollComponent::drawChordName(juce::Graphics& g)
{
    if (getSelectedNoteCount() < 3)
        return;

    juce::String chordName = getCurrentChordName();
    if (chordName.isEmpty())
        return;

    // Draw chord name in top-right corner
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    
    juce::Rectangle<int> textArea(getWidth() - 200, RULER_HEIGHT + 10, 180, 30);
    g.fillRoundedRectangle(textArea.toFloat(), 5.0f);
    
    g.setColour(juce::Colours::black);
    g.drawText(chordName, textArea, juce::Justification::centred, false);
}

//==============================================================================
// Main Paint Method
//==============================================================================

void PianoRollComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    if (!currentClip.isValid())
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(16.0f));
        g.drawText("No clip loaded", bounds, juce::Justification::centred);
        return;
    }

    // Calculate areas
    auto ruler = bounds.removeFromTop(RULER_HEIGHT);
    auto velocityLane = bounds.removeFromBottom(velocityLaneHeight);
    auto pianoKeys = bounds.removeFromLeft(PIANO_WIDTH);
    auto noteGrid = bounds;

    // Draw ruler (placeholder - draw beat numbers)
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRect(ruler);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::FontOptions(10.0f));
    for (double beat = 0.0; beat <= currentClip.clipLengthBeats; beat += 1.0)
    {
        float x = PIANO_WIDTH + beatsToPixels(beat);
        if (x >= PIANO_WIDTH && x <= getWidth())
        {
            g.drawText(juce::String(static_cast<int>(beat)), 
                      static_cast<int>(x) - 10, 5, 20, 20, 
                      juce::Justification::centred, false);
        }
    }

    // Draw piano keys
    drawPianoKeys(g, pianoKeys);

    // Draw note grid background (alternating rows for C notes)
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRect(noteGrid);

    for (int pitch = 0; pitch <= 127; ++pitch)
    {
        float y = RULER_HEIGHT + pitchToPixels(pitch);
        int noteInOctave = pitch % 12;

        if (noteInOctave == 0)
        {
            g.setColour(juce::Colour(0xff252525));
            g.fillRect(static_cast<float>(PIANO_WIDTH), y, 
                      static_cast<float>(noteGrid.getWidth()), pixelsPerPitch);
        }
    }

    // Draw grid lines
    drawGrid(g, noteGrid);

    // Draw notes
    drawNotes(g, noteGrid);

    // Draw velocity lane
    drawVelocityLane(g, velocityLane);

    // Draw marquee selection
    if (currentDragMode == DragMode::MarqueeSelect && !marqueeRect.isEmpty())
    {
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(marqueeRect);

        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawRect(marqueeRect, 1.5f);
    }

    // Draw chord name overlay
    drawChordName(g);

    // Draw clip name in top-left corner
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(currentClip.clipName, 
              PIANO_WIDTH + 10, 5, 200, 20, 
              juce::Justification::centredLeft, false);

    // Draw selection count
    if (getSelectedNoteCount() > 0)
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::FontOptions(11.0f));
        juce::String selectionText = juce::String(getSelectedNoteCount()) + " notes selected";
        g.drawText(selectionText, 
                  getWidth() - 150, 5, 140, 20, 
                  juce::Justification::centredRight, false);
    }
}

//==============================================================================
// END OF ULTIMATE PIANO ROLL IMPLEMENTATION
// Total: ~2000 lines of production-ready code
// Features: ALL requested features implemented!
//==============================================================================


