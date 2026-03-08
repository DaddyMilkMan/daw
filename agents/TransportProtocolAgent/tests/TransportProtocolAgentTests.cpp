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
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }

    // Capabilities
    juce::Array<double> getAvailableSampleRates() override { return { 44100.0, 48000.0 }; }
    juce::Array<int> getAvailableBufferSizes() override { return { 128, 256, 512 }; }

    juce::BigInteger getActiveInputChannels() const override { return inputChannels; }
    juce::BigInteger getActiveOutputChannels() const override { return outputChannels; }

    // Missing pure virtuals needed for instantiation
    juce::StringArray getOutputChannelNames() override { return {"Out L", "Out R"}; }
    juce::StringArray getInputChannelNames() override { return {"In L", "In R"}; }
    int getDefaultBufferSize() override { return 256; }

    // Test helpers
    void setInputChannels(int num) { inputChannels.setRange(0, num, true); }
    void setOutputChannels(int num) { outputChannels.setRange(0, num, true); }

private:
    juce::BigInteger inputChannels;
    juce::BigInteger outputChannels;
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
        auto* dev = new MockAudioIODevice(inputDeviceName.isNotEmpty() ? inputDeviceName : outputDeviceName, getTypeName());
        dev->setInputChannels(2);
        dev->setOutputChannels(2);
        return dev;
    }

    // Test helpers
    void setInputDevices(const juce::StringArray& names) { inputDevices = names; }
    void setDefaultInputIndex(int index) { defaultInputIndex = index; }

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

            manager->addAudioDeviceType(std::move(mockType));
            manager->setCurrentAudioDeviceType("MockActiveType", true);

            zenith::agents::TransportProtocolAgent agent(std::move(manager));
            auto info = agent.getDefaultInputDevice();

            expectEquals(info.name, juce::String("MockInput2"));
            expectEquals(info.apiType, juce::String("MockActiveType"));
            expect(info.isDefault);
        }

        beginTest("Default Input Device - Platform Priority");
        {
            auto manager = std::make_unique<juce::AudioDeviceManager>();

            juce::String highPriority = "ASIO";
            juce::String lowPriority = "DirectSound";
            #if JUCE_LINUX
            highPriority = "JACK"; lowPriority = "ALSA";
            #elif JUCE_MAC
            highPriority = "CoreAudio"; lowPriority = "MockLow";
            #endif

            auto mockHigh = std::make_unique<MockAudioIODeviceType>(highPriority);
            mockHigh->setInputDevices({"HighDev"});
            mockHigh->setDefaultInputIndex(0);

            auto mockLow = std::make_unique<MockAudioIODeviceType>(lowPriority);
            mockLow->setInputDevices({"LowDev"});
            mockLow->setDefaultInputIndex(0);

            manager->addAudioDeviceType(std::move(mockLow));
            manager->addAudioDeviceType(std::move(mockHigh));

            zenith::agents::TransportProtocolAgent agent(std::move(manager));
            auto info = agent.getDefaultInputDevice();

            expectEquals(info.name, juce::String("HighDev"));
            expectEquals(info.apiType, highPriority);
        }

        beginTest("Enumerate Devices - Active Only Detail");
        {
            auto manager = std::make_unique<juce::AudioDeviceManager>();

            auto mockType = std::make_unique<MockAudioIODeviceType>("MockType");
            mockType->setInputDevices({"Input1", "Input2"});
            mockType->setDefaultInputIndex(0); // "Input1"

            manager->addAudioDeviceType(std::move(mockType));
            manager->setCurrentAudioDeviceType("MockType", true);

            zenith::agents::TransportProtocolAgent agent(std::move(manager));
            auto devices = agent.enumerateDevices();

            expectEquals(devices.size(), (size_t)2);

            bool foundActive = false;
            bool foundInactive = false;

            for (const auto& dev : devices)
            {
                if (dev.name == "Input1")
                {
                    foundActive = true;
                    expectEquals(dev.numInputChannels, 2);
                    expect(dev.supportedSampleRates.size() > 0);
                    expect(dev.supportedBufferSizes.size() > 0);
                }
                else if (dev.name == "Input2")
                {
                    foundInactive = true;
                    expectEquals(dev.numInputChannels, 0);
                    expectEquals(dev.numOutputChannels, 0);
                    expect(dev.supportedSampleRates.isEmpty());
                    expect(dev.supportedBufferSizes.isEmpty());
                }
            }

            expect(foundActive);
            expect(foundInactive);
        }
    }
};

static TransportProtocolAgentTests transportProtocolAgentTests;

int main(int argc, char* argv[]) {
  juce::UnitTestRunner runner;
  runner.runAllTests();
  return 0;
}
