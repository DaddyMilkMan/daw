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
            
            // Test property list request (Async, so we must simulate the response)
            peManager.requestPropertyList(mockMuid);
            
            // Simulate Response
            juce::DynamicObject::Ptr json = new juce::DynamicObject();
            juce::Array<juce::var> propsArray;
            juce::DynamicObject::Ptr p1 = new juce::DynamicObject();
            p1->setProperty("id", "filter_cutoff");
            p1->setProperty("name", "Cutoff");
            p1->setProperty("min", 0.0);
            p1->setProperty("max", 1.0);
            p1->setProperty("value", 0.5);
            propsArray.add(juce::var(p1));
            json->setProperty("properties", propsArray);
            
            peManager.handlePropertyResponse(mockMuid, juce::var(json));

            auto props = peManager.getProperties(mockMuid);
            
            expect(!props.empty(), "Properties should not be empty after request");
            if (!props.empty())
                expectEquals(props[0].id, juce::String("filter_cutoff"), "First property ID should be filter_cutoff");
            
            // Test property setting
            peManager.setProperty(mockMuid, "filter_cutoff", 0.8f);
            
            // Update local cache (setProperty updates cache immediately in our implementation)
            props = peManager.getProperties(mockMuid);
            if (!props.empty())
                expectWithinAbsoluteError(props[0].value, 0.8f, 0.001f, "Property value should be updated to 0.8");
            else
                expect(false, "Properties should not be empty after setProperty");
        }

        beginTest("Discovery Response Parsing");
        {
            Engine engine;
            Midi2DiscoveryService discovery(engine);
            
            // 1. Send Discovery Response (0x71)
            // MUID 393: 0x00 0x00 0x03 0x09
            uint8_t discMsg[] = { 0x7E, 0x7F, 0x0D, 0x71, 0x00, 0x00, 0x03, 0x09 };
            auto m1 = juce::MidiMessage::createSysExMessage(discMsg, sizeof(discMsg));
            discovery.handleIncomingMidiMessage(nullptr, m1);
            
            // 2. Send Capabilities Response (0x73) with PE bit (0x04) set
            uint8_t capMsg[] = { 0x7E, 0x7F, 0x0D, 0x73, 
                                 0x01, // Version
                                 0x00, 0x00, 0x03, 0x09, // MUID
                                 0x04 // PE Supported
                               };
            auto m2 = juce::MidiMessage::createSysExMessage(capMsg, sizeof(capMsg));
            discovery.handleIncomingMidiMessage(nullptr, m2);
            
            // 3. Send PE Get Property Data Response (0x35)
            // JSON: {"properties": [{"id": "discovered_prop", ...}]}
            juce::String jsonStr = R"({"properties": [{"id": "discovered_prop", "name": "Disc", "min": 0, "max": 1, "value": 0}]})";
            juce::MemoryBlock sysex;
            uint8_t peHeader[] = { 
                0x7E, 0x7F, 0x0D, 0x35, 0x02, // SubID2=0x35 (PE), 0x02=GetReply? No, 0x35 is Reply category.
                0x00, 0x00, 0x03, 0x09, // MUID
                0x01, // PE Version
                0x00, 0x00 // Header Size (0)
            };
            sysex.append(peHeader, sizeof(peHeader));
            sysex.append(jsonStr.toRawUTF8(), jsonStr.length());
            
            auto m3 = juce::MidiMessage::createSysExMessage(sysex.getData(), sysex.getSize());
            discovery.handleIncomingMidiMessage(nullptr, m3);
            
            // Check
            auto props = discovery.getPEManager().getProperties(393);
            expect(!props.empty(), "Properties should be discovered for MUID 393");
            if (!props.empty()) {
                expectEquals(props[0].id, juce::String("discovered_prop"));
            }
        }
    }
};

static Midi2Tests midi2Tests;

} // namespace zenith
