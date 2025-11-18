/*
  ==============================================================================

    ZenithSampler.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Built-in sampler with basic parameters and macros.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Instrument.h"

namespace zenith {

//==============================================================================
/**
    Simple sampler processor with envelope and filter controls
*/
class ZenithSamplerProcessor : public juce::Synthesiser,
                                public juce::AudioProcessor
{
public:
    //==========================================================================
    // Parameter indices
    //==========================================================================
    enum Parameters
    {
        Attack = 0,
        Decay,
        Sustain,
        Release,
        FilterCutoff,
        FilterResonance,
        NumParameters
    };

    //==========================================================================
    ZenithSamplerProcessor();
    ~ZenithSamplerProcessor() override = default;

    /**
     * @brief Get the AudioProcessorValueTreeState for parameter attachments
     */
    juce::AudioProcessorValueTreeState& getParameters() { return parameters_; }

    //==========================================================================
    // AudioProcessor interface
    //==========================================================================
    const juce::String getName() const override { return "Zenith Sampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::AudioProcessorValueTreeState parameters_;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerProcessor)
};

//==============================================================================
/**
    Zenith Sampler instrument wrapper
*/
class ZenithSampler : public InstrumentBase
{
public:
    ZenithSampler();
    ~ZenithSampler() override = default;

    /**
     * @brief Create metadata for this instrument
     */
    static InstrumentMetadata createMetadata();

    /**
     * @brief Get the AudioProcessorValueTreeState for parameter attachments in editor
     */
    juce::AudioProcessorValueTreeState* getParameterState();

private:
    void registerPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSampler)
};

} // namespace zenith
