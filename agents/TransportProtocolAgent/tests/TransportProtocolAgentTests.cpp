/*
  ==============================================================================
    agents/TransportProtocolAgent/tests/TransportProtocolAgentTests.cpp
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "../TransportProtocolAgent.h"

// Mock AudioIODevice
class MockAudioIODevice : public juce::AudioIODevice
{
public:
    MockAudioIODevice(const juce::String& name, const juce::String& typeName)
        : AudioIODevice(name, typeName) {}

    juce::String open(const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start(juce::AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return 256; }
    double getCurrentSampleRate() override { return 44100.0; }
    int getCurrentBitDepth() override { return 16; }
    juce::BigInteger getActiveOutputChannels() const override { return {}; }
    juce::BigInteger getActiveInputChannels() const override { return {}; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }

    juce::StringArray getOutputChannelNames() override { return {"Out1", "Out2"}; }
    juce::StringArray getInputChannelNames() override { return {"In1", "In2"}; }
    juce::Array<double> getAvailableSampleRates() override { return {44100.0, 48000.0}; }
    juce::Array<int> getAvailableBufferSizes() override { return {128, 256, 512}; }
    int getDefaultBufferSize() override { return 256; }
};

// Mock AudioIODeviceType
class MockAudioIODeviceType : public juce::AudioIODeviceType
{
public:
    MockAudioIODeviceType(const juce::String& typeName)
        : AudioIODeviceType(typeName) {}

    void scanForDevices() override {}

    juce::StringArray getDeviceNames(bool wantInputNames = false) const override
    {
        if (wantInputNames) return inputDevices;
        return outputDevices;
    }

    int getDefaultDeviceIndex(bool forInput) const override
    {
        if (forInput) return defaultInputIndex;
        return defaultOutputIndex;
    }

    int getIndexOfDevice(juce::AudioIODevice* device, bool asInput) const override { return -1; }
    bool hasSeparateInputsAndOutputs() const override { return true; }

    juce::AudioIODevice* createDevice(const juce::String& outputDeviceName,
                                      const juce::String& inputDeviceName) override
    {
        return new MockAudioIODevice(inputDeviceName.isNotEmpty() ? inputDeviceName : outputDeviceName, getTypeName());
    }

    // Test helpers
    void setInputDevices(const juce::StringArray& names) { inputDevices = names; }
    void setDefaultInputIndex(int index) { defaultInputIndex = index; }
    void setOutputDevices(const juce::StringArray& names) { outputDevices = names; }
    void setDefaultOutputIndex(int index) { defaultOutputIndex = index; }

private:
    juce::StringArray inputDevices;
    juce::StringArray outputDevices;
    int defaultInputIndex = -1;
    int defaultOutputIndex = -1;
};

class TransportProtocolAgentTests : public juce::UnitTest
{
public:
    TransportProtocolAgentTests() : juce::UnitTest("TransportProtocolAgentTests", "Audio") {}

    void runTest() override
    {
        beginTest("Default Input Device - Active Type Priority");
        {
            auto manager = std::make_unique<juce::AudioDeviceManager>();

            // Register a mock type
            auto mockType = std::make_unique<MockAudioIODeviceType>("MockActiveType");
            mockType->setInputDevices({"MockInput1", "MockInput2"});
            mockType->setDefaultInputIndex(1); // "MockInput2"

            // We need to set this as active.
            // AudioDeviceManager::setAudioDeviceSetup uses a type name.
            // But to use it, the type must be added first.
            MockAudioIODeviceType* rawMock = mockType.get();
            manager->addAudioDeviceType(std::move(mockType));

            // Force manager to use this type
            manager->setCurrentAudioDeviceType("MockActiveType", true);

            zenith::agents::TransportProtocolAgent agent(std::move(manager));
            auto info = agent.getDefaultInputDevice();

            expectEquals(info.name, juce::String("MockInput2"));
            expectEquals(info.apiType, juce::String("MockActiveType"));
            expect(info.isDefault);
        }

        beginTest("Default Input Device - Platform Priority");
        {
            // Test that high priority type is chosen over low priority
            auto manager = std::make_unique<juce::AudioDeviceManager>();

            // Determine what strings are used on this platform
            juce::String highPriority;
            juce::String lowPriority;

            #if JUCE_LINUX
            highPriority = "JACK";
            lowPriority = "ALSA";
            #elif JUCE_WINDOWS
            highPriority = "ASIO";
            lowPriority = "DirectSound";
            #elif JUCE_MAC
            highPriority = "CoreAudio";
            lowPriority = "MockLow";
            #else
            highPriority = "MockHigh";
            lowPriority = "MockLow";
            #endif

            auto mockHigh = std::make_unique<MockAudioIODeviceType>(highPriority);
            mockHigh->setInputDevices({"HighDev"});
            mockHigh->setDefaultInputIndex(0);

            auto mockLow = std::make_unique<MockAudioIODeviceType>(lowPriority);
            mockLow->setInputDevices({"LowDev"});
            mockLow->setDefaultInputIndex(0);

            manager->addAudioDeviceType(std::move(mockLow)); // Add low first
            manager->addAudioDeviceType(std::move(mockHigh));

            zenith::agents::TransportProtocolAgent agent(std::move(manager));
            auto info = agent.getDefaultInputDevice();

            expectEquals(info.name, juce::String("HighDev"));
            expectEquals(info.apiType, highPriority);
        }

        beginTest("Default Output Device - Active Type Priority");
        {
            auto manager = std::make_unique<juce::AudioDeviceManager>();

            auto mockType = std::make_unique<MockAudioIODeviceType>("MockActiveType");
            mockType->setOutputDevices({"MockOut1", "MockOut2"});
            mockType->setDefaultOutputIndex(1); // "MockOut2"

            manager->addAudioDeviceType(std::move(mockType));
            manager->setCurrentAudioDeviceType("MockActiveType", true);

            zenith::agents::TransportProtocolAgent agent(std::move(manager));
            auto info = agent.getDefaultOutputDevice();

            expectEquals(info.name, juce::String("MockOut2"));
            expectEquals(info.apiType, juce::String("MockActiveType"));
            expect(info.isDefault);
        }

        beginTest("Enumerate Devices - Active Only Details");
        {
            auto manager = std::make_unique<juce::AudioDeviceManager>();
            auto mockType = std::make_unique<MockAudioIODeviceType>("MockType");
            mockType->setInputDevices({"In1"});
            mockType->setOutputDevices({"Out1"});

            manager->addAudioDeviceType(std::move(mockType));
            // Ensure no device is open/active by default or at least check inactive behavior
            // By default AudioDeviceManager might try to open something if we don't stop it.
            // But we didn't call initialiseWithDefaultDevices, so it should be clean.

            zenith::agents::TransportProtocolAgent agent(std::move(manager));
            auto devices = agent.enumerateDevices();

            bool foundIn = false;
            for (const auto& dev : devices) {
                if (dev.name == "In1") {
                    foundIn = true;
                    // Should be 0 because it's not active
                    expectEquals(dev.numInputChannels, 0);
                    expectEquals(dev.supportedSampleRates.size(), 0);
                    expect(dev.apiType == "MockType");
                }
            }
            expect(foundIn);
        }
    }
};

static TransportProtocolAgentTests transportProtocolAgentTests;

// Main entry point for the test app
int main(int argc, char* argv[]) {
  juce::UnitTestRunner runner;
  runner.runAllTests();
  return 0;
}
