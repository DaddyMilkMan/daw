/**
 * @file BasicAudioTest.cpp
 * @brief Basic unit tests for the Zenith DAW audio engine
 * 
 * Verifies that the engine can process audio without generating NaNs or Infs.
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include <cmath>

namespace zenith {
namespace tests {

class MockAudioDevice : public juce::AudioIODevice
{
public:
    MockAudioDevice() : juce::AudioIODevice("MockDevice", "Mock") {}
    juce::String open(const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start(juce::AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return 512; }
    double getCurrentSampleRate() override { return 48000.0; }
    int getCurrentBitDepth() override { return 24; }
    juce::StringArray getOutputChannelNames() override { return {"Out1", "Out2"}; }
    juce::StringArray getInputChannelNames() override { return {}; }
    juce::BigInteger getActiveOutputChannels() const override { juce::BigInteger b; b.setBit(0); b.setBit(1); return b; }
    juce::BigInteger getActiveInputChannels() const override { return {}; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }
    juce::Array<double> getAvailableSampleRates() override { return {44100.0, 48000.0, 96000.0}; }
    juce::Array<int> getAvailableBufferSizes() override { return {128, 256, 512, 1024}; }
    int getDefaultBufferSize() override { return 512; }
};

class BasicAudioTest : public juce::UnitTest
{
public:
    BasicAudioTest() : juce::UnitTest("Basic Audio Processing") {}
    
    void runTest() override
    {
        beginTest("Track processes audio without NaN/Inf");
        
        // Setup
        zenith::Engine engine;
        zenith::ProjectState state;
        MockAudioDevice mockDevice;
        
        engine.setProjectState(&state);
        
        // Initialize engine state manually via callback
        engine.audioDeviceAboutToStart(&mockDevice);
        
        // Create a track
        engine.createTrack("Test Track", "audio");
        
        // Process one block
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        
        juce::AudioIODeviceCallbackContext context;
        // The context struct only contains hostTimeNs, no other fields to set
        
        const float* inputData[2] = { nullptr, nullptr };
        float* outputData[2] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
        
        engine.audioDeviceIOCallbackWithContext(
            inputData, 2,
            outputData, 2,
            buffer.getNumSamples(),
            context
        );
        
        // ACTUAL ASSERTION - check output is valid
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* samples = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                expect(!std::isnan(samples[i]), "Output contains NaN");
                expect(!std::isinf(samples[i]), "Output contains Inf");
            }
        }
        
        engine.audioDeviceStopped();
    }
};

static BasicAudioTest basicAudioTest;

} // namespace tests
} // namespace zenith

