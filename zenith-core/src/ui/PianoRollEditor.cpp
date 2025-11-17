/**
 * @file PianoRollEditor.cpp
 * @brief Piano Roll editor implementation
 */

#include "../../include/ui/PianoRollEditor.h"

//==============================================================================
PianoRollEditor::PianoRollEditor(ProjectState& state,
                                 const juce::String& tid,
                                 const juce::String& cid)
    : projectState(state),
      trackId(tid),
      clipId(cid)
{
    DBG("PianoRollEditor: Opening editor for track " + trackId + ", clip " + clipId);

    // Get the notes node for this clip
    notesNode = projectState.getNotesForClip(trackId, clipId);

    // If notes node exists, listen to it
    if (notesNode.isValid())
    {
        notesNode.addListener(this);
        rebuildNotes();
    }
    else
    {
        DBG("PianoRollEditor: No notes node yet (will be created on first note add)");
    }
}

PianoRollEditor::~PianoRollEditor()
{
    if (notesNode.isValid())
        notesNode.removeListener(this);

    DBG("PianoRollEditor: Destroyed");
}

//==============================================================================
// Component interface
//==============================================================================

void PianoRollEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Piano keys area
    auto pianoKeysArea = bounds.removeFromLeft(pianoKeysWidth);
    drawPianoKeys(g, pianoKeysArea);

    // Grid and notes area
    auto gridArea = bounds;
    drawGrid(g, gridArea);
    drawNotes(g, gridArea);
}

void PianoRollEditor::resized()
{
    // No child components yet
}

void PianoRollEditor::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.position;

    // Skip if clicking on piano keys
    if (pos.x < pianoKeysWidth)
        return;

    // Adjust for piano keys offset
    pos.x -= pianoKeysWidth;

    // Check if clicking on an existing note
    int noteIndex = findNoteAt(pos);

    if (noteIndex >= 0)
    {
        // Start dragging the note
        interactionMode = InteractionMode::DraggingNote;
        draggedNoteIndex = noteIndex;
        dragStartPosition = pos;

        auto& note = noteVisuals[noteIndex];
        dragStartBeats = note.startBeats;
        dragStartPitch = note.pitch;

        DBG("PianoRollEditor: Started dragging note " + note.noteId);
    }
    else
    {
        // Create new note
        double startBeats = quantize(positionToBeats(pos.x));
        int pitch = positionToPitch(pos.y);
        double lengthBeats = gridDivision; // Default to one grid division
        int velocity = 100; // Default velocity

        auto noteId = projectState.addNote(trackId, clipId, startBeats, lengthBeats, pitch, velocity, "Add Note");

        if (!noteId.isEmpty())
        {
            DBG("PianoRollEditor: Created note " + noteId + " at " + juce::String(startBeats) + " beats, pitch " + juce::String(pitch));

            // Rebuild notes if needed (in case notes node was just created)
            if (!notesNode.isValid())
            {
                notesNode = projectState.getNotesForClip(trackId, clipId);
                if (notesNode.isValid())
                    notesNode.addListener(this);
            }

            repaint();
        }
    }
}

void PianoRollEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (interactionMode == InteractionMode::DraggingNote && draggedNoteIndex >= 0)
    {
        auto pos = e.position;
        pos.x -= pianoKeysWidth;

        // Calculate new position
        auto delta = pos - dragStartPosition;
        double deltaBeats = delta.x / pixelsPerBeat;
        int deltaPitch = -static_cast<int>(delta.y / pixelsPerNote);

        double newStartBeats = quantize(dragStartBeats + deltaBeats);
        int newPitch = juce::jlimit(0, 127, dragStartPitch + deltaPitch);

        // Keep the same length and velocity
        auto& note = noteVisuals[draggedNoteIndex];
        double lengthBeats = note.lengthBeats;
        int velocity = note.velocity;

        // Update the note
        projectState.moveNote(trackId, clipId, note.noteId,
                              newStartBeats, lengthBeats, newPitch, velocity,
                              "Move Note");
    }
}

void PianoRollEditor::mouseUp(const juce::MouseEvent& e)
{
    interactionMode = InteractionMode::None;
    draggedNoteIndex = -1;
}

void PianoRollEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto pos = e.position;

    // Skip if clicking on piano keys
    if (pos.x < pianoKeysWidth)
        return;

    // Adjust for piano keys offset
    pos.x -= pianoKeysWidth;

    // Find note at position
    int noteIndex = findNoteAt(pos);

    if (noteIndex >= 0)
    {
        // Delete the note
        auto& note = noteVisuals[noteIndex];
        projectState.removeNote(trackId, clipId, note.noteId, "Delete Note");

        DBG("PianoRollEditor: Deleted note " + note.noteId);
    }
}

//==============================================================================
// Zoom and scroll
//==============================================================================

void PianoRollEditor::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat = juce::jlimit(10.0, 200.0, ppb);
    rebuildNotes();
    repaint();
}

void PianoRollEditor::setPixelsPerNote(int ppn)
{
    pixelsPerNote = juce::jlimit(4, 32, ppn);
    rebuildNotes();
    repaint();
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void PianoRollEditor::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent == notesNode && child.hasType(ProjectState::ID_NOTE))
    {
        DBG("PianoRollEditor: Note added to clip");
        rebuildNotes();
        repaint();
    }
}

void PianoRollEditor::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (parent == notesNode)
    {
        DBG("PianoRollEditor: Note removed from clip");
        rebuildNotes();
        repaint();
    }
}

void PianoRollEditor::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    // Not used for notes
}

void PianoRollEditor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType(ProjectState::ID_NOTE))
    {
        DBG("PianoRollEditor: Note property changed");
        rebuildNotes();
        repaint();
    }
}

//==============================================================================
// Helper methods
//==============================================================================

void PianoRollEditor::rebuildNotes()
{
    noteVisuals.clear();

    if (!notesNode.isValid())
        return;

    for (auto noteTree : notesNode)
    {
        if (!noteTree.hasType(ProjectState::ID_NOTE))
            continue;

        NoteVisual nv;
        nv.noteId = noteTree[ProjectState::PROP_ID].toString();
        nv.startBeats = noteTree[ProjectState::PROP_START_BEATS];
        nv.lengthBeats = noteTree[ProjectState::PROP_LENGTH_BEATS];
        nv.pitch = noteTree[ProjectState::PROP_PITCH];
        nv.velocity = noteTree[ProjectState::PROP_VELOCITY];
        nv.bounds = noteToBounds(nv.startBeats, nv.lengthBeats, nv.pitch);

        noteVisuals.push_back(nv);
    }

    DBG("PianoRollEditor: Rebuilt " + juce::String(noteVisuals.size()) + " notes");
}

juce::Rectangle<float> PianoRollEditor::noteToBounds(double startBeats, double lengthBeats, int pitch) const
{
    float x = static_cast<float>(startBeats * pixelsPerBeat);
    float y = static_cast<float>((highestNote - pitch) * pixelsPerNote);
    float w = static_cast<float>(lengthBeats * pixelsPerBeat);
    float h = static_cast<float>(pixelsPerNote);

    return juce::Rectangle<float>(x, y, w, h);
}

double PianoRollEditor::positionToBeats(float x) const
{
    return x / pixelsPerBeat;
}

int PianoRollEditor::positionToPitch(float y) const
{
    int pitch = highestNote - static_cast<int>(y / pixelsPerNote);
    return juce::jlimit(0, 127, pitch);
}

double PianoRollEditor::quantize(double beats) const
{
    if (gridDivision <= 0.0)
        return beats;

    return std::round(beats / gridDivision) * gridDivision;
}

int PianoRollEditor::findNoteAt(juce::Point<float> position)
{
    for (int i = 0; i < static_cast<int>(noteVisuals.size()); ++i)
    {
        if (noteVisuals[i].bounds.contains(position))
            return i;
    }

    return -1;
}

void PianoRollEditor::drawPianoKeys(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(area);

    // Draw piano keys
    for (int pitch = lowestNote; pitch <= highestNote; ++pitch)
    {
        int noteInOctave = pitch % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10);

        float y = static_cast<float>((highestNote - pitch) * pixelsPerNote);
        auto keyRect = juce::Rectangle<float>(0.0f, y, static_cast<float>(area.getWidth()), static_cast<float>(pixelsPerNote));

        if (isBlackKey)
        {
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(keyRect);
        }
        else
        {
            g.setColour(juce::Colour(0xff3a3a3a));
            g.fillRect(keyRect);
        }

        // Draw divider
        g.setColour(juce::Colour(0xff000000));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(area.getWidth()));

        // Draw note name for C notes
        if (noteInOctave == 0 && pixelsPerNote >= 10)
        {
            g.setColour(juce::Colours::white);
            g.setFont(10.0f);
            int octave = (pitch / 12) - 1;
            g.drawText("C" + juce::String(octave), keyRect.reduced(2), juce::Justification::centredLeft, false);
        }
    }
}

void PianoRollEditor::drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Draw horizontal lines (pitch)
    for (int pitch = lowestNote; pitch <= highestNote; ++pitch)
    {
        int noteInOctave = pitch % 12;
        bool isC = (noteInOctave == 0);

        float y = static_cast<float>((highestNote - pitch) * pixelsPerNote);

        if (isC)
            g.setColour(juce::Colour(0xff404040));
        else
            g.setColour(juce::Colour(0xff2a2a2a));

        g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(area.getWidth()));
    }

    // Draw vertical lines (beats)
    int maxBeats = area.getWidth() / static_cast<int>(pixelsPerBeat) + 2;
    for (int beat = 0; beat < maxBeats; ++beat)
    {
        float x = static_cast<float>(beat * pixelsPerBeat);

        // Draw beat lines
        if (beat % 4 == 0)
            g.setColour(juce::Colour(0xff505050)); // Bar lines
        else
            g.setColour(juce::Colour(0xff303030)); // Beat lines

        g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(area.getHeight()));

        // Draw sub-divisions
        if (gridDivision < 1.0)
        {
            int subdivisions = static_cast<int>(1.0 / gridDivision);
            for (int sub = 1; sub < subdivisions; ++sub)
            {
                float subX = x + static_cast<float>(sub * gridDivision * pixelsPerBeat);
                g.setColour(juce::Colour(0xff252525));
                g.drawVerticalLine(static_cast<int>(subX), 0.0f, static_cast<float>(area.getHeight()));
            }
        }
    }
}

void PianoRollEditor::drawNotes(juce::Graphics& g, juce::Rectangle<int> area)
{
    for (const auto& note : noteVisuals)
    {
        // Map velocity to color brightness
        float brightness = juce::jmap(static_cast<float>(note.velocity), 0.0f, 127.0f, 0.3f, 0.9f);

        // Use a nice blue color
        g.setColour(juce::Colour::fromHSV(0.6f, 0.6f, brightness, 1.0f));
        g.fillRect(note.bounds.reduced(1.0f));

        // Border
        g.setColour(juce::Colour::fromHSV(0.6f, 0.4f, brightness + 0.1f, 1.0f));
        g.drawRect(note.bounds, 1.0f);
    }
}
