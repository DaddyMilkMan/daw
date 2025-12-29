/**
 * @file MidiEditorContainer.cpp
 * @brief Implementation of MidiEditorContainer
 */

#include "PianoRollComponent.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

//==============================================================================
// MidiEditorContainer
//==============================================================================

MidiEditorContainer::MidiEditorContainer(ProjectState& state, Engine& engine)
    : projectState(state), engine_(engine)
{
    // Initialize Piano Roll
    pianoRoll = std::make_unique<PianoRollComponent>(projectState);
    addAndMakeVisible(pianoRoll.get());

    // Initialize Drum Pad
    drumPad = std::make_unique<DrumPadComponent>(engine_, projectState);
    addChildComponent(drumPad.get());

    // Initialize Toggle Button
    toggleButton.setButtonText("Toggle View");
    toggleButton.onClick = [this] { toggleView(); };
    addAndMakeVisible(toggleButton);

    // Initial State
    activeView = View::PianoRoll;
    drumPad->setVisible(false);
}

MidiEditorContainer::~MidiEditorContainer()
{
}

void MidiEditorContainer::setClipContext(const MidiClipContext& context)
{
    currentContext = context;
    if (pianoRoll)
        pianoRoll->setClipContext(context);
    
    // In future: update drum pad context if needed
}

void MidiEditorContainer::resized()
{
    auto bounds = getLocalBounds();
    
    // Toggle button at top right (or relevant position)
    // For now, let's put it in a header area or just overlay
    int headerHeight = 30;
    
    auto header = bounds.removeFromTop(headerHeight);
    toggleButton.setBounds(header.removeFromRight(100).reduced(2));

    // Content area
    if (pianoRoll)
        pianoRoll->setBounds(bounds);
    
    if (drumPad)
        drumPad->setBounds(bounds);
}

void MidiEditorContainer::toggleView()
{
    if (activeView == View::PianoRoll)
    {
        activeView = View::DrumPad;
        pianoRoll->setVisible(false);
        drumPad->setVisible(true);
    }
    else
    {
        activeView = View::PianoRoll;
        pianoRoll->setVisible(true);
        drumPad->setVisible(false);
    }
}

void MidiEditorContainer::injectMidiMessage(const juce::MidiMessage& msg)
{
    // Forward to active view if appropriate
    // Ideally this would be handled by the engine or input system
}

} // namespace zenith
