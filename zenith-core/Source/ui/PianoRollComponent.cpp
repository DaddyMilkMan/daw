/*
  ==============================================================================

    PianoRollComponent.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 4: Piano Roll MIDI Editor

    Piano roll implementation

  ==============================================================================
*/

#include "PianoRollComponent.h"
#include "../engine/Track.h"
#include "../engine/Clip.h"
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"
#include "../../include/TempoMap.h"

//==============================================================================
// Piano RollComponent Implementation
//==============================================================================

PianoRollComponent::PianoRollComponent(zenith::Track::Clip* clipToEdit, Engine& engineRef)
    : clip(clipToEdit), engine(engineRef)
{
    if (clip == nullptr)
    {
        DBG("PianoRollComponent: ERROR - null clip");
        return;
    }

    // Set up key listener for delete key
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Get tempo and sample rate from Engine
    currentSampleRate = engine.getSampleRate();
    currentTempo = engine.getTempoMap().getTempoAt(0); // Start tempo for now

    // Build note cache from clip
    updateNoteCache();

    DBG("PianoRollComponent: Created for clip " + clip->getName());
}

PianoRollComponent::~PianoRollComponent()
{
    removeKeyListener(this);
}

//==============================================================================
// Component interface
//==============================================================================

void PianoRollComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    auto bounds = getLocalBounds();

    // Reserve left area for piano keys
    auto keysBounds = bounds.removeFromLeft(static_cast<int>(pianoKeysWidth));

    // Draw piano keys
    drawPianoKeys(g, keysBounds);

    // Draw grid in main area
    drawGrid(g, bounds);

    // Draw notes
    drawNotes(g, bounds);
}

void PianoRollComponent::resized()
{
    // Rebuild note cache when size changes
    updateNoteCache();
}

//==============================================================================
// Mouse interaction
//==============================================================================

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    // Hit-test for notes
    auto* hit = hitTestNote(e.position);

    if (hit != nullptr)
    {
        // Select this note
        selectedNote = hit;
        isDragging = false;
        dragStartPosition = e.position;
        noteDragStartNumber = hit->noteNumber;
        noteDragStartBeats = hit->startTime / (currentSampleRate * 60.0 / currentTempo);

        DBG("PianoRollComponent: Selected note " + juce::String(hit->noteNumber));

        repaint();
    }
    else
    {
        // Clicked empty space - create new note
        selectedNote = nullptr;

        // Calculate note number and start time from mouse position
        float relativeX = e.position.getX() - pianoKeysWidth;
        float relativeY = e.position.getY();

        int noteNumber = pixelsToNoteNumber(relativeY);
        double startBeats = snapToGrid(pixelsToBeats(relativeX));

        // Default note length (1/4 note)
        double lengthBeats = 0.25;

        // Create the note
        createNote(noteNumber, startBeats, lengthBeats, 100);

        DBG("PianoRollComponent: Created note " + juce::String(noteNumber) + " at " + juce::String(startBeats) + " beats");

        repaint();
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (selectedNote == nullptr)
        return;

    isDragging = true;

    // Calculate new position
    float relativeX = e.position.getX() - pianoKeysWidth;
    float relativeY = e.position.getY();

    int newNoteNumber = pixelsToNoteNumber(relativeY);
    double newStartBeats = snapToGrid(pixelsToBeats(relativeX));

    // Clamp to valid MIDI range
    newNoteNumber = juce::jlimit(0, 127, newNoteNumber);

    // Move the note
    moveNote(selectedNote, newNoteNumber, newStartBeats);

    repaint();
}

void PianoRollComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    isDragging = false;
}

void PianoRollComponent::mouseMove(const juce::MouseEvent& e)
{
    // Update hover state
    auto* hit = hitTestNote(e.position);
    
    if (hit != hoveredNote)
    {
        hoveredNote = hit;
        hoverAlpha = 0.0f;
        repaint();
    }
}

void PianoRollComponent::mouseExit(const juce::MouseEvent& /*e*/)
{
    hoveredNote = nullptr;
    hoverAlpha = 0.0f;
    repaint();
}

//==============================================================================
// Timer callback
//==============================================================================

void PianoRollComponent::timerCallback()
{
    bool needsRepaint = false;
    
    // Animate hover alpha
    if (hoveredNote != nullptr && hoverAlpha < 1.0f)
    {
        hoverAlpha = std::min(1.0f, hoverAlpha + 0.15f);
        needsRepaint = true;
    }
    
    if (needsRepaint)
    {
        repaint();
    }
}

//==============================================================================
// Key listener
//==============================================================================

bool PianoRollComponent::keyPressed(const juce::KeyPress& key, Component* originatingComponent)
{
    juce::ignoreUnused(originatingComponent);

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        if (selectedNote != nullptr)
        {
            DBG("PianoRollComponent: Deleting note " + juce::String(selectedNote->noteNumber));

            deleteNote(selectedNote->noteNumber, selectedNote->startTime);
            selectedNote = nullptr;

            repaint();
            return true;
        }
    }

    return false;
}

//==============================================================================
// View control
//==============================================================================

void PianoRollComponent::setPixelsPerBeat(float ppb)
{
    pixelsPerBeat = juce::jlimit(10.0f, 200.0f, ppb);
    updateNoteCache();
    repaint();
}

void PianoRollComponent::setNoteHeight(float height)
{
    noteHeight = juce::jlimit(8.0f, 30.0f, height);
    updateNoteCache();
    repaint();
}

//==============================================================================
// Rendering helpers
//==============================================================================

void PianoRollComponent::drawPianoKeys(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Background
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRect(bounds);

    // Draw keys
    for (int noteNumber = highestNote; noteNumber >= lowestNote; --noteNumber)
    {
        float y = noteNumberToPixels(noteNumber);

        // Determine if black or white key
        int noteInOctave = noteNumber % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10);

        // Draw key
        if (isBlackKey)
            g.setColour(juce::Colour(0xff1a1a1a));
        else
            g.setColour(juce::Colour(0xff4a4a4a));

        g.fillRect(bounds.getX(), static_cast<int>(y), bounds.getWidth(), static_cast<int>(noteHeight));

        // Draw border
        g.setColour(juce::Colours::black);
        g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

        // Draw note name for C notes
        if (noteInOctave == 0)  // C
        {
            int octave = noteNumber / 12 - 1;
            juce::String noteName = "C" + juce::String(octave);

            g.setColour(juce::Colours::lightgrey);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(noteName,
                      bounds.getX() + 2,
                      static_cast<int>(y),
                      bounds.getWidth() - 4,
                      static_cast<int>(noteHeight),
                      juce::Justification::centredLeft,
                      true);
        }
    }

    // Border
    g.setColour(juce::Colours::black);
    g.drawVerticalLine(bounds.getRight(), static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
}

void PianoRollComponent::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Vertical grid lines (beats)
    g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.3f));

    int maxBeats = static_cast<int>(pixelsToBeats(static_cast<float>(getWidth() - pianoKeysWidth)) + 1);

    for (int beat = 0; beat <= maxBeats; ++beat)
    {
        float x = pianoKeysWidth + beatsToPixels(beat);

        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            // Thicker line every 4 beats (bar line)
            if (beat % 4 == 0)
                g.setColour(juce::Colour(0xff5a5a5a).withAlpha(0.5f));
            else
                g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.3f));

            g.drawVerticalLine(static_cast<int>(x),
                             static_cast<float>(bounds.getY()),
                             static_cast<float>(bounds.getBottom()));
        }
    }

    // Horizontal grid lines (semitones)
    g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.3f));

    for (int noteNumber = lowestNote; noteNumber <= highestNote; ++noteNumber)
    {
        float y = noteNumberToPixels(noteNumber);

        g.drawHorizontalLine(static_cast<int>(y),
                           static_cast<float>(bounds.getX()),
                           static_cast<float>(bounds.getRight()));
    }
}

void PianoRollComponent::drawNotes(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    juce::ignoreUnused(bounds);

    for (const auto& note : noteCache)
    {
        // Note color
        if (&note == selectedNote)
            g.setColour(juce::Colours::yellow);
        else
            g.setColour(juce::Colours::green.brighter(0.2f));

        // Draw note rectangle
        g.fillRect(note.bounds.reduced(1.0f));

        // Border
        g.setColour(juce::Colours::black);
        g.drawRect(note.bounds, 1.0f);
    }
}

//==============================================================================
// Time/pitch mapping
//==============================================================================

float PianoRollComponent::beatsToPixels(double beats) const
{
    return static_cast<float>(beats * pixelsPerBeat);
}

double PianoRollComponent::pixelsToBeats(float pixels) const
{
    return pixels / pixelsPerBeat;
}

int PianoRollComponent::pixelsToNoteNumber(float y) const
{
    // Y=0 is highestNote, Y increases downward
    int noteNumber = highestNote - static_cast<int>(y / noteHeight);
    return juce::jlimit(lowestNote, highestNote, noteNumber);
}

float PianoRollComponent::noteNumberToPixels(int noteNumber) const
{
    return (highestNote - noteNumber) * noteHeight;
}

//==============================================================================
// Note management
//==============================================================================

void PianoRollComponent::updateNoteCache()
{
    noteCache.clear();

    if (clip == nullptr)
        return;

    const auto* midiSequence = clip->getMidiSequence();

    if (midiSequence == nullptr)
        return;

    // Iterate through MIDI events
    for (int i = 0; i < midiSequence->getNumEvents(); ++i)
    {
        const auto* event = midiSequence->getEventPointer(i);

        if (event == nullptr)
            continue;

        const auto& message = event->message;

        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            double startTime = event->message.getTimeStamp();
            int velocity = message.getVelocity();

            // Find corresponding note-off
            double endTime = startTime + 0.5;  // Default duration

            for (int j = i + 1; j < midiSequence->getNumEvents(); ++j)
            {
                const auto* offEvent = midiSequence->getEventPointer(j);

                if (offEvent != nullptr && offEvent->message.isNoteOff() && offEvent->message.getNoteNumber() == noteNumber)
                {
                    endTime = offEvent->message.getTimeStamp();
                    break;
                }
            }

            double duration = endTime - startTime;

            // Convert to screen coordinates
            double startBeats = startTime / (currentSampleRate * 60.0 / currentTempo);
            double durationBeats = duration / (currentSampleRate * 60.0 / currentTempo);

            float x = pianoKeysWidth + beatsToPixels(startBeats);
            float y = noteNumberToPixels(noteNumber);
            float width = beatsToPixels(durationBeats);
            float height = noteHeight;

            // Create NoteVisual
            NoteVisual visual;
            visual.noteNumber = noteNumber;
            visual.startTime = startTime;
            visual.duration = duration;
            visual.velocity = velocity;
            visual.bounds = juce::Rectangle<float>(x, y, width, height);

            noteCache.push_back(visual);
        }
    }

    DBG("PianoRollComponent: Note cache updated - " + juce::String(noteCache.size()) + " notes");
}

NoteVisual* PianoRollComponent::hitTestNote(juce::Point<float> position)
{
    for (auto it = noteCache.rbegin(); it != noteCache.rend(); ++it)
    {
        if (it->bounds.contains(position))
        {
            return std::to_address(it);
        }
    }

    return nullptr;
}

void PianoRollComponent::createNote(int noteNumber, double startBeats, double lengthBeats, int velocity)
{
    if (clip == nullptr)
        return;

    // Convert beats to seconds
    double samplesPerBeat = currentSampleRate * 60.0 / currentTempo;
    double startTime = startBeats * samplesPerBeat / currentSampleRate;
    double endTime = (startBeats + lengthBeats) * samplesPerBeat / currentSampleRate;

    // Create note-on and note-off messages
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, noteNumber, static_cast<juce::uint8>(velocity));
    noteOn.setTimeStamp(startTime);

    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, noteNumber);
    noteOff.setTimeStamp(endTime);

    // Add to clip's MIDI sequence (this modifies the clip)
    // For MVP, we directly modify the sequence
    // TODO(zenith-core#1): Use undo/redo system
    auto sequence = clip->getMidiSequence();
    if (sequence != nullptr)
    {
        auto newSequence = *sequence;  // Copy
        newSequence.addEvent(noteOn);
        newSequence.addEvent(noteOff);
        newSequence.updateMatchedPairs();
        newSequence.sort();

        clip->setMidiSequence(newSequence);
    }

    // Rebuild cache
    updateNoteCache();
}

void PianoRollComponent::deleteNote(int noteNumber, double startTime)
{
    if (clip == nullptr)
        return;

    auto sequence = clip->getMidiSequence();
    if (sequence == nullptr)
        return;

    auto newSequence = *sequence;  // Copy

    // Find and remove the note-on/off pair
    const double timeEpsilon = 0.001;  // Tolerance for time matching

    for (int i = newSequence.getNumEvents() - 1; i >= 0; --i)
    {
        const auto* event = newSequence.getEventPointer(i);

        if (event != nullptr)
        {
            const auto& msg = event->message;

            if ((msg.isNoteOn() || msg.isNoteOff()) &&
                msg.getNoteNumber() == noteNumber &&
                std::abs(msg.getTimeStamp() - startTime) < timeEpsilon)
            {
                newSequence.deleteEvent(i, false);
            }
        }
    }

    newSequence.updateMatchedPairs();
    clip->setMidiSequence(newSequence);

    // Rebuild cache
    updateNoteCache();
}

void PianoRollComponent::moveNote(NoteVisual* note, int newNoteNumber, double newStartBeats)
{
    if (clip == nullptr || note == nullptr)
        return;

    // Delete old note
    deleteNote(note->noteNumber, note->startTime);

    // Create new note at std::make_unique<position>(preserve duration and velocity)
    double durationBeats = note->duration / (currentSampleRate * 60.0 / currentTempo);
    createNote(newNoteNumber, newStartBeats, durationBeats, note->velocity);
}

//==============================================================================
// Grid snapping
//==============================================================================

double PianoRollComponent::snapToGrid(double beats) const
{
    return std::round(beats / gridResolution) * gridResolution;
}

//==============================================================================
// PianoRollWindow Implementation
//==============================================================================

PianoRollWindow::PianoRollWindow(zenith::Track::Clip* clipToEdit, Engine& engineRef)
    : DocumentWindow(clipToEdit != nullptr ? "Piano Roll - " + clipToEdit->getName() : "Piano Roll",
                     juce::Colours::darkgrey,
                     DocumentWindow::allButtons),
      clip(clipToEdit),
      engine(engineRef)
{
    if (clip == nullptr)
    {
        DBG("PianoRollWindow: ERROR - null clip");
        return;
    }

    // Create piano roll component
    pianoRoll = std::make_unique<PianoRollComponent>(clip, engine);

    // Set as content
    setContentNonOwned(pianoRoll.get(), true);

    // Set size
    setResizable(true, true);
    centreWithSize(1000, 600);

    // Show window
    setVisible(true);

    DBG("PianoRollWindow: Created for clip " + clip->getName());
}

PianoRollWindow::~PianoRollWindow()
{
    clearContentComponent();
}

void PianoRollWindow::closeButtonPressed()
{
    // Just delete this window
    delete this;
}

