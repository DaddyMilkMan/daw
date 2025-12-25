/**
 * @file Midi2Tests.cpp
 * @brief Unit tests for MIDI 2.0 Property Exchange (JUCE UnitTest version)
 */

#include <juce_core/juce_core.h>
#include "../engine/Engine.h"
#include "../engine/Midi2DiscoveryService.h"
#include "../engine/PropertyExchangeManager.h"

namespace zenith {

class Midi2Tests : public juce::UnitTest {
public:
    Midi2Tests() : juce::UnitTest("MIDI 2.0 Property Exchange", "MIDI") {}

    void runTest() override {
        beginTest("PropertyExchangeManager Logic");
        {
            PropertyExchangeManager peManager;
            uint32_t mockMuid = 12345;
            
            // Test property list request
            peManager.requestPropertyList(mockMuid);
            auto props = peManager.getProperties(mockMuid);
            
            expect(!props.empty(), "Properties should not be empty after request");
            if (!props.empty())
                expectEquals(props[0].id, juce::String("filter_cutoff"), "First property ID should be filter_cutoff");
            
            // Test property setting
            peManager.setProperty(mockMuid, "filter_cutoff", 0.5f);
            props = peManager.getProperties(mockMuid);
            expectWithinAbsoluteError(props[0].value, 0.5f, 0.001f, "Property value should be updated to 0.5");
        }

        beginTest("Discovery Response Parsing");
        {
            Engine engine;
            Midi2DiscoveryService discovery(engine);
            
            // Simulate a MIDI-CI Discovery Response SysEx
            // 7E, 7F, 0D, 70, 01, ...
            // Parsing: (data[4] << 21) | (data[5] << 14) | (data[6] << 7) | data[7]
            // For MUID 393: 393 = 3 * 128 + 9
            // Bytes: 0x00, 0x00, 0x03, 0x09
            uint8_t data[] = { 0x7E, 0x7F, 0x0D, 0x70, 0x00, 0x00, 0x03, 0x09 };
            
            auto msg = juce::MidiMessage::createSysExMessage(data, sizeof(data));
            
            discovery.handleIncomingMidiMessage(nullptr, msg);
            
            // Check if properties were requested (mock response would have populated PE manager)
            auto props = discovery.getPEManager().getProperties(393);
            expect(!props.empty(), "Properties should be discovered for MUID 393");
        }
    }
};

static Midi2Tests midi2Tests;

} // namespace zenith
