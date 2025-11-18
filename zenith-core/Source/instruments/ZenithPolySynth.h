/*
  ==============================================================================

    ZenithPolySynth.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Built-in polyphonic synthesizer with basic parameters and macros.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Instrument.h"

namespace zenith {

//==============================================================================
/**
    Simple polyphonic synthesizer processor

    Provides:
    - Sine/Saw/Square oscillator
    - ADSR envelope
    - Low-pass filter
    - Basic parameters for CommandAPI control
*/
class ZenithPolySynthProcessor : public juce::Synthesiser,
                                  public juce::AudioProcessor
{
public:
    //==========================================================================
    // Parameter indices
    //==========================================================================
    enum Parameters
    {
        OscType = 0,
        FilterCutoff,
        FilterResonance,
        Attack,
        Decay,
        Sustain,
        Release,
        NumParameters
    };

    //==========================================================================
    ZenithPolySynthProcessor();
    ~ZenithPolySynthProcessor() override = default;

    //==========================================================================
    // AudioProcessor interface
    //==========================================================================
    const juce::String getName() const override { return "Zenith Poly Synth"; }
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
    // Simple sine wave voice
    struct SineVoice : public juce::SynthesiserVoice
    {
        bool canPlaySound(juce::SynthesiserSound*) override { return true; }
        void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void pitchWheelMoved(int) override {}
        void controllerMoved(int, int) override {}
        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    private:
        double currentAngle = 0.0;
        double angleDelta = 0.0;
        double level = 0.0;
        double tailOff = 0.0;
    };

    struct SineSound : public juce::SynthesiserSound
    {
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthProcessor)
};

//==============================================================================
/**
    Zenith Poly Synth instrument wrapper

    Exposes the synthesizer with metadata and presets for CommandAPI.
*/
class ZenithPolySynth : public InstrumentBase
{
public:
    ZenithPolySynth();
    ~ZenithPolySynth() override = default;

    /**
     * @brief Create metadata for this instrument
     */
    static InstrumentMetadata createMetadata();

private:
    void registerPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynth)
};

} // namespace zenith
