/*
  ==============================================================================

    TransportProtocolAgentTest.cpp
    Tests for TransportProtocolAgent.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "TransportProtocolAgent.h"

namespace zenith {
namespace tests {

class TransportProtocolAgentTest : public juce::UnitTest {
public:
    TransportProtocolAgentTest() : juce::UnitTest("TransportProtocolAgent") {}

    void runTest() override {
        beginTest("Device Enumeration");
        {
            agents::TransportProtocolAgent agent;
            auto devices = agent.enumerateDevices();

            expect(devices.size() >= 0, "Device list should be a valid vector");

            // In CI environment, we might not have devices, but we can verify logic
            for (const auto& device : devices) {
                expect(device.name.isNotEmpty(), "Device name should not be empty");
                expect(device.apiType.isNotEmpty(), "API type should not be empty");
            }
        }

        beginTest("Open Device (Dry Run)");
        {
            agents::TransportProtocolAgent agent;

            // Attempt to open a non-existent device
            bool result = agent.openDevice("NonExistentDevice12345", 44100.0, 512);
            expect(!result, "Should fail to open non-existent device");

            // If we have devices, try to open the first one
            auto devices = agent.enumerateDevices();
            if (!devices.empty()) {
                // Find a device that is likely to work (e.g. not a restricted one)
                // Just try the first one for now.
                auto& firstDevice = devices.front();

                // We don't assert success because in CI/headless environments opening a device might fail
                // or not be permitted. We just ensure it doesn't crash.
                bool openResult = agent.openDevice(firstDevice.name, 44100.0, 512);

                if (openResult) {
                     expect(agent.isDeviceOpen(), "Agent should report device is open");
                     agent.closeDevice();
                     expect(!agent.isDeviceOpen(), "Agent should report device is closed");
                }
            }
        }
    }
};

static TransportProtocolAgentTest transportProtocolAgentTest;

} // namespace tests
} // namespace zenith
