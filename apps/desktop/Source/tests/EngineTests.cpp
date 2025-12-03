/*
  ==============================================================================

    EngineTests.cpp
    Created: 2025-12-02
    Author:  Zenith DAW

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../../include/Engine.h"
#include "../../include/EngineEvent.h"

// Simple test runner
struct EngineTests
{
    void run()
    {
        testLockFreeQueue();
        testMidiFifo();
    }
    
    void testLockFreeQueue()
    {
        zenith::Engine engine;
        
        // Queue an event
        zenith::EngineEvent e(zenith::EngineEvent::Type::SetPluginParam);
        e.trackIndex = 1;
        e.pluginIndex = 0;
        e.paramIndex = 2;
        e.value = 0.5f;
        
        bool success = engine.queueEvent(e);
        jassert(success);
        
        // In a real test, we would check if the event was processed, 
        // but processEvents() consumes it internally.
        // This just verifies compilation and basic API.
    }
    
    void testMidiFifo()
    {
        zenith::MidiFifo fifo;
        juce::MidiMessage msg = juce::MidiMessage::noteOn(1, 60, 0.8f);
        
        fifo.push(msg);
        
        juce::MidiMessage popped;
        bool gotIt = fifo.pop(popped);
        
        jassert(gotIt);
        jassert(popped.isNoteOn());
        jassert(popped.getNoteNumber() == 60);
    }
};
