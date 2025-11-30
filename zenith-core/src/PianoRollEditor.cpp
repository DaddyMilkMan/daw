/**
 * @file PianoRollEditor.cpp
 * @brief PianoRollEditor implementation (integration stub)
 */

#include "../include/PianoRollEditor.h"

//==============================================================================
// PianoRollEditor Implementation
//==============================================================================

PianoRollEditor::PianoRollEditor(ProjectState& ps,
                                 const juce::String& tid,
                                 const juce::String& cid)
    : DocumentWindow("Piano Roll - " + cid,
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons),
      projectState(ps),
      trackId(tid),
      clipId(cid)
{
    DBG("PianoRollEditor: Opening for track " + trackId + ", clip " + clipId);

    // Create content
    content = std::make_unique<ContentComponent>(projectState, trackId, clipId);
    setContentOwned(content.get(), true);

    // Set window properties
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(800, 400);

    setVisible(true);
}

PianoRollEditor::~PianoRollEditor()
{
    DBG("PianoRollEditor: Destructor");
}

void PianoRollEditor::closeButtonPressed()
{
    // Delete this window
    delete this;
}

//==============================================================================
// ContentComponent Implementation
//==============================================================================

PianoRollEditor::ContentComponent::ContentComponent(ProjectState& ps,
                                                     const juce::String& tid,
                                                     const juce::String& cid)
    : projectState(ps), trackId(tid), clipId(cid)
{
}

void PianoRollEditor::ContentComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("Piano Roll Editor (Integration Stub)",
               getLocalBounds().reduced(20),
               juce::Justification::topLeft);

    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Track: " + trackId + ", Clip: " + clipId,
               getLocalBounds().reduced(20).removeFromTop(40),
               juce::Justification::topLeft);

    // Draw piano keyboard (left side, 88 keys)
    int keyboardWidth = 80;
    juce::Rectangle<int> keyboardArea(0, 60, keyboardWidth, getHeight() - 60);

    for (int note = 0; note < 128; ++note)
    {
        int y = pitchToY(note);
        bool isBlackKey = false;
        int octaveNote = note % 12;
        if (octaveNote == 1 || octaveNote == 3 || octaveNote == 6 || octaveNote == 8 || octaveNote == 10)
            isBlackKey = true;

        g.setColour(isBlackKey ? juce::Colours::black : juce::Colours::white);
        g.fillRect(keyboardArea.getX(), y, keyboardWidth, noteHeight);

        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(keyboardArea.getX(), y, keyboardWidth, noteHeight);
    }

    // Draw grid (beats)
    int gridLeft = keyboardWidth;
    g.setColour(juce::Colour(0xff3a3a3a));

    for (int beat = 0; beat < 64; ++beat)
    {
        float x = gridLeft + static_cast<float>(beat) * pixelsPerBeat;
        g.drawVerticalLine(static_cast<int>(x), 60.0f, static_cast<float>(getHeight()));
    }

    // Draw horizontal lines for pitch
    for (int note = 0; note < 128; ++note)
    {
        int y = pitchToY(note);
        g.drawHorizontalLine(y, static_cast<float>(gridLeft), static_cast<float>(getWidth()));
    }

    // Draw MIDI notes (integration stub)
    // TODO(zenith-core#1): When U3 MIDI note model is merged, read from ProjectState clip's NOTES nodes
    g.setColour(juce::Colours::green);
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("MIDI notes will be displayed here when U3 MIDI model is merged.\n"
               "Click to add notes (writes to ProjectState).",
               getLocalBounds().reduced(100),
               juce::Justification::centred);
}

void PianoRollEditor::ContentComponent::resized()
{
}

void PianoRollEditor::ContentComponent::mouseDown(const juce::MouseEvent& event)
{
    // Integration stub: Add MIDI note
    int keyboardWidth = 80;

    if (event.x > keyboardWidth)
    {
        int pitch = yToPitch(event.y);
        double beat = static_cast<double>(event.x - keyboardWidth) / pixelsPerBeat;

        DBG("PianoRollEditor: Add note - pitch " + juce::String(pitch) +
            ", beat " + juce::String(beat));

        // TODO(zenith-core#1): When U3 MIDI note model is merged:
        // 1. Find clip in ProjectState
        // 2. Add NOTE child node to clip's NOTES container
        // 3. Set properties: pitch, start (beats), length (beats), velocity
        // 4. Engine will pick up changes and play notes
    }
}

void PianoRollEditor::ContentComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Integration stub: Drag to move note
    juce::ignoreUnused(event);
}

//==============================================================================
int PianoRollEditor::ContentComponent::pitchToY(int midiNote) const
{
    // MIDI note 127 at top, 0 at bottom
    return 60 + (127 - midiNote) * noteHeight;
}

int PianoRollEditor::ContentComponent::yToPitch(int y) const
{
    int note = 127 - ((y - 60) / noteHeight);
    return juce::jlimit(0, 127, note);
}

