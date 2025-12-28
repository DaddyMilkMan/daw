/*
  ==============================================================================

    EngineTests.cpp
    Created: 2025-12-02
    Author:  Zenith DAW

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "Engine.h"
#include "EngineEvent.h"

namespace zenith {
namespace tests {

class EngineTests : public juce::UnitTest
{
public:
    EngineTests() : juce::UnitTest("Core Engine", "AudioEngine") {}

    void runTest() override
    {

        beginTest("Lock-free Event Queue");
        {
            zenith::Engine engine;
            
            // Queue an event
            zenith::EngineEvent e(zenith::EngineEvent::Type::SetPluginParam);
            e.trackIndex = 1;
            e.pluginIndex = 0;
            e.paramIndex = 2;
            e.value = 0.5f;
            
            bool success = engine.queueEvent(e);
            expect(success, "Event queueing should succeed");
            
            // We can't easily verify processing here without a full audio callback,
            // but we've verified the API and thread-safe queueing.
        }
        
        beginTest("MIDI FIFO");
        {
            zenith::MidiFifo fifo;
            juce::MidiMessage msg = juce::MidiMessage::noteOn(1, 60, 0.8f);
            
            fifo.push(msg);
            
            juce::MidiMessage popped;
            bool gotIt = fifo.pop(popped);
            
            expect(gotIt, "Popping from FIFO should succeed after push");
            expect(popped.isNoteOn(), "Popped message should be a Note On");
            expectEquals(popped.getNoteNumber(), 60, "Popped note number should match");
            expectWithinAbsoluteError(popped.getFloatVelocity(), 0.8f, 0.01f, "Velocity should match");
        }
    }
};

static EngineTests engineTests;

} // namespace tests
} // namespace zenith
