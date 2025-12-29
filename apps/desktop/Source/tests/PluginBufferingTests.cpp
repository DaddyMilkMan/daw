/*
  ==============================================================================

    PluginBufferingTests.cpp
    Created: 2025-12-27
    Author:  Zenith DAW

    Tests for PluginChain sidechain buffering and High Watermark logic.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "TestUtils.h"
#include "../../engine/PluginChain.h"

namespace zenith {
namespace tests {

// Stub plugin with configurable channel counts
class TestPlugin : public StubAudioPlugin {
public:
    TestPlugin(int inChannels, int outChannels) 
        : inCh(inChannels), outCh(outChannels) {}
    
    // StubAudioPlugin might default to 2/2, override if needed/possible
    // Since StubAudioPlugin is simple, we might need to mock isBusesLayoutSupported
    
    // For this test, PluginChain uses getTotalNumInputChannels() which usually calls getBusCount etc.
    // StubAudioPlugin implementation details matter here.
    // Assuming StubAudioPlugin allows setting bus layouts or reports what we want.
    // If not, we will need a better mock.
    
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override {
        return true; // Accept anything
    }
    
    // Hack override to report specific channels if StubBase doesn't flexible
    // But usually default is Stereo.
    // Let's rely on standard JUCE mechanism or StubAudioPlugin if it's good.
    // Assuming StubAudioPlugin is basic.
    
private:
    int inCh, outCh;
};

// A Mock that we can control better
class MockChannelPlugin : public StubAudioPlugin {
public:
    MockChannelPlugin(int channels) {
        // StubAudioPlugin initializes to stereo. We need to override buses.
        // But StubAudioPlugin base constructor is fixed. 
        // We can't change buses easily after construction in legacy JUCE, 
        // but StubAudioPlugin uses BusesProperties.
        // Let's rely on setPlayConfigDetails or just assume the host uses getTotalNumInputChannels which we can mock?
        // PluginChain logic calls: plugin->getTotalNumInputChannels().
        // AudioProcessor::getTotalNumInputChannels() sums bus channels.
        
        // We need to force channel count.
        // Best way: override getTotalNumInputChannels() if it was virtual? It's not.
        // It relies on getBusesLayout().
        
        // Let's just override isBusesLayoutSupported and ensure we report correct channels.
        // Actually StubAudioPlugin is simple.
        // Let's just make a dedicated simple AudioPluginInstance mock without StubAudioPlugin if needed, 
        // or just accept defaults and hack it.
        
    MockChannelPlugin(int channels) 
    {
        setPlayConfigDetails(channels, channels, 44100.0, 512);
    }

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    const juce::String getName() const override { return "Mock"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
    void fillInPluginDescription(juce::PluginDescription &desc) const override {
        desc.name = getName();
        desc.uniqueId = 999;
    }
};

class MockBufferingPlugin : public juce::AudioPluginInstance {
public:
    MockBufferingPlugin(int channels) 
        : juce::AudioPluginInstance(BusesProperties()
             .withInput("Input", juce::AudioChannelSet::canonicalChannelSet(channels), true)
             .withOutput("Output", juce::AudioChannelSet::canonicalChannelSet(channels), true)) 
    {
    }

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    const juce::String getName() const override { return "MockBuffering"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    
    // AudioPluginInstance overrides
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
    void fillInPluginDescription(juce::PluginDescription &desc) const override {
        desc.name = getName();
        desc.uniqueId = 999;
    }
};


class PluginBufferingTests : public juce::UnitTest {
public:
    PluginBufferingTests() : juce::UnitTest("PluginBufferingTests", "Engine") {}

    void runTest() override {
        beginTest("High Watermark Growth");
        {
            PluginChain chain;
            
            // Initial state: empty chain
            // Prepare to play should init buffer to default min (e.g. 2 channels, blocksize)
            chain.prepareToPlay(44100.0, 512);

            // Access private activeSnapshot_
            auto snapshot = chain.activeSnapshot_.load();
            expect(snapshot != nullptr);
            expectEquals(snapshot->sidechainBuffer->getNumChannels(), 2);
            expectEquals(snapshot->sidechainBuffer->getNumSamples(), 512);

            // Add Plugin with 4 channels
            // For test simplicity, we don't need a real plugin if we just want to trigger the sizing logic.
            // But PluginChain uses plugin->getTotalNumInputChannels().
            // We use MockChannelPlugin.
            auto plugin4 = std::make_unique<MockBufferingPlugin>(4);
            chain.addPlugin(std::move(plugin4), 44100.0, 512);
            
            // Snapshot should be updated synchronously by addPlugin->updateSnapshot
            snapshot = chain.activeSnapshot_.load();
            expectEquals(snapshot->sidechainBuffer->getNumChannels(), 4);
            
            // Add Plugin with 2 channels
            auto plugin2 = std::make_unique<MockBufferingPlugin>(2);
            chain.addPlugin(std::move(plugin2), 44100.0, 512);
            
            snapshot = chain.activeSnapshot_.load();
            expectEquals(snapshot->sidechainBuffer->getNumChannels(), 4); // Should stay 4 (High Watermark)
            
            // Remove plugins
            chain.removePlugin(0); // Remove 4-ch plugin
            chain.removePlugin(0); // Remove 2-ch plugin
            
            snapshot = chain.activeSnapshot_.load();
            expectEquals(snapshot->sidechainBuffer->getNumChannels(), 4); // Should REMAIN 4
        }
    }
};

static PluginBufferingTests pluginBufferingTests;

} // namespace tests
} // namespace zenith
