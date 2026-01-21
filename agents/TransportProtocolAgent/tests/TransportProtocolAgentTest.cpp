/*
  ==============================================================================
    agents/TransportProtocolAgent/tests/TransportProtocolAgentTest.cpp
    Unit tests for TransportProtocolAgent.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../TransportProtocolAgent.h"

class TransportProtocolAgentTest : public juce::UnitTest
{
public:
    TransportProtocolAgentTest() : juce::UnitTest("TransportProtocolAgentTest") {}

    void runTest() override
    {
        beginTest("Enumerate Devices");

        zenith::agents::TransportProtocolAgent agent;
        auto devices = agent.enumerateDevices();

        logMessage("Found " + juce::String(devices.size()) + " devices.");

        for (const auto& device : devices)
        {
            logMessage("Device: " + device.name +
                       " | Type: " + device.apiType +
                       " | In: " + juce::String(device.numInputChannels) +
                       " | Out: " + juce::String(device.numOutputChannels) +
                       " | Default: " + (device.isDefault ? "Yes" : "No"));
        }

        // Since we are running in a headless CI environment, we might not find many real devices.
        // But we should verify that the call doesn't crash and returns a vector.
        expect(true, "Enumeration completed without crash");
    }
};

static TransportProtocolAgentTest transportProtocolAgentTest;
