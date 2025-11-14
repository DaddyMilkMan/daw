/**
 * @file PianoRollComponent.cpp
 * @brief Piano Roll implementation
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
        auto clip = projectState.findClip(currentClip.clipId);
        if (clip.isValid())
        {
            auto midiNotesNode = clip.getChildWithName(ProjectState::ID_MIDI_NOTES);
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
        auto oldClip = projectState.findClip(currentClip.clipId);
        if (oldClip.isValid())
        {
            auto midiNotesNode = oldClip.getChildWithName(ProjectState::ID_MIDI_NOTES);
            if (midiNotesNode.isValid())
                midiNotesNode.removeListener(this);
        }
    }

    // Set new clip
    currentClip = context;

    // Attach to new clip's ValueTree for auto-refresh
    if (currentClip.isValid())
    {
        auto clip = projectState.findClip(currentClip.clipId);
        if (clip.isValid())
        {
            // Get or create MIDI_NOTES node
            auto midiNotesNode = clip.getChildWithName(ProjectState::ID_MIDI_NOTES);
            if (!midiNotesNode.isValid())
            {
                // Create empty MIDI_NOTES container if it doesn't exist
                midiNotesNode = juce::ValueTree(ProjectState::ID_MIDI_NOTES);
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
    for (auto& note : noteRects)
    {
        float x = beatsToPixels(note.startBeats);
        float y = pitchToPixels(note.pitch);
        float width = beatsToPixels(note.lengthBeats);
        float height = static_cast<float>(pixelsPerPitch);

        note.bounds = juce::Rectangle<float>(x, y, width, height);
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

    auto* note = findNoteAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));

    if (note != nullptr)
    {
        // Start dragging existing note
        startDraggingNote(note, e);
    }
    else
    {
        // Create new note
        createNoteAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingNote != nullptr)
    {
        updateNoteDrag(e);
    }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent& e)
{
    if (draggingNote != nullptr)
    {
        finishNoteDrag();
    }
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

    return false;
}

//==============================================================================
// ValueTree Listener (auto-refresh on changes)
//==============================================================================

void PianoRollComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.hasType(ProjectState::ID_MIDI_NOTES))
    {
        DBG("PianoRoll: Note added via external change (undo/redo/Wingman)");
        needsRefresh = true;
    }
}

void PianoRollComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (parent.hasType(ProjectState::ID_MIDI_NOTES))
    {
        DBG("PianoRoll: Note removed via external change");
        needsRefresh = true;
    }
}

void PianoRollComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType(ProjectState::ID_MIDI_NOTE))
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
        g.setFont(juce::Font(16.0f));
        g.drawText("No clip loaded", getLocalBounds(), juce::Justification::centred);
        return;
    }

    auto bounds = getLocalBounds();

    // Draw piano keys background (alternating white/black keys)
    g.setColour(juce::Colour(0xff3a3a3a));
    for (int pitch = 0; pitch <= 127; ++pitch)
    {
        int note = pitch % 12;
        bool isBlackKey = (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);

        if (isBlackKey)
        {
            float y = pitchToPixels(pitch);
            g.fillRect(0.0f, y, static_cast<float>(bounds.getWidth()), static_cast<float>(pixelsPerPitch));
        }
    }

    // Draw grid lines (vertical beat lines)
    g.setColour(juce::Colour(0xff404040));
    for (double beat = 0.0; beat < currentClip.clipLengthBeats; beat += gridBeats)
    {
        float x = beatsToPixels(beat);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(bounds.getHeight()));
    }

    // Draw horizontal pitch lines (every octave)
    for (int pitch = 0; pitch <= 127; pitch += 12)
    {
        float y = pitchToPixels(pitch);
        g.setColour(juce::Colour(0xff505050));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(bounds.getWidth()));
    }

    // Draw notes
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

    // Draw clip name in corner
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f));
    g.drawText(currentClip.clipName + " - " + currentClip.clipId,
               bounds.removeFromTop(30).reduced(10, 5),
               juce::Justification::centredLeft);
}

void PianoRollComponent::resized()
{
    updateNoteRectangles();
}
