/*
  ==============================================================================

    TestUtils.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Shared utilities for unit testing, including mock objects.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {
namespace tests {

/**
 * @class StubAudioPlugin
 * @brief A base class for mock plugins that implements all pure virtual methods with no-ops.
 * 
 * Inherit from this and override only the methods you need to test.
 */
class StubAudioPlugin : public juce::AudioPluginInstance {
public:
    StubAudioPlugin() 
        : juce::AudioPluginInstance(juce::BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)) 
    {}

    //==============================================================================
    // Required AudioProcessor overrides (No-ops)
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    
    //==============================================================================
    // Metadata overrides (Defaults)
    const juce::String getName() const override { return "Stub Plugin"; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    
    //==============================================================================
    // Editor overrides (No editor)
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    //==============================================================================
    // Program/State overrides (No-ops)
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

} // namespace tests
} // namespace zenith
