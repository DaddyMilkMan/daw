/**
 * @file ZenithPolySynth.h
 * @brief Simple polyphonic synthesizer with macro controls
 *
 * Features:
 * - Simple oscillator with multiple waveforms
 * - Basic filter (low-pass)
 * - ADSR envelope
 * - 4 smart macros (Warmth, Space, Bite, Movement)
 */

#pragma once

#include <JuceHeader.h>
#include "InstrumentRegistry.h"
#include "InstrumentMetadata.h"
#include "InstrumentPreset.h"

namespace zenith {

//==============================================================================
/**
 * @brief Simple synthesizer voice
 */
class PolySynthVoice : public juce::SynthesiserVoice
{
public:
    PolySynthVoice() = default;

    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int) override
    {
        currentAngle = 0.0;
        level = velocity * 0.5;
        tailOff = 0.0;

        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;

        envelope.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            envelope.noteOff();
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override
    {
        if (angleDelta == 0.0)
            return;

        while (--numSamples >= 0)
        {
            // Generate waveform
            auto currentSample = generateWaveform() * level;

            // Apply envelope
            currentSample *= envelope.getNextSample();

            // Output to both channels
            for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                outputBuffer.addSample(i, startSample, currentSample);

            // Advance phase
            currentAngle += angleDelta;
            if (currentAngle > juce::MathConstants<double>::pi * 2.0)
                currentAngle -= juce::MathConstants<double>::pi * 2.0;

            ++startSample;

            // Stop if envelope finished
            if (!envelope.isActive())
            {
                clearCurrentNote();
                angleDelta = 0.0;
                break;
            }
        }
    }

    void setWaveform(int type) { waveformType = type; }
    void setEnvelopeParameters(float attack, float decay, float sustain, float release)
    {
        juce::ADSR::Parameters params;
        params.attack = attack;
        params.decay = decay;
        params.sustain = sustain;
        params.release = release;
        envelope.setParameters(params);
    }

    void prepareToPlay(double sampleRate)
    {
        envelope.setSampleRate(sampleRate);
    }

private:
    float generateWaveform()
    {
        switch (waveformType)
        {
            case 0: // Sine
                return static_cast<float>(std::sin(currentAngle));

            case 1: // Saw
                return static_cast<float>(currentAngle / juce::MathConstants<double>::pi - 1.0);

            case 2: // Square
                return currentAngle < juce::MathConstants<double>::pi ? 1.0f : -1.0f;

            case 3: // Triangle
                {
                    auto phase = currentAngle / (juce::MathConstants<double>::pi * 2.0);
                    return static_cast<float>(phase < 0.5 ? (phase * 4.0 - 1.0) : (3.0 - phase * 4.0));
                }

            default:
                return 0.0f;
        }
    }

    double currentAngle = 0.0;
    double angleDelta = 0.0;
    double level = 0.0;
    double tailOff = 0.0;
    int waveformType = 0;  // 0=Sine, 1=Saw, 2=Square, 3=Triangle
    juce::ADSR envelope;
};

//==============================================================================
/**
 * @brief Simple synth sound (accepts all notes)
 */
class PolySynthSound : public juce::SynthesiserSound
{
public:
    PolySynthSound() = default;

    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

//==============================================================================
/**
 * @brief Zenith PolySynth instrument processor
 */
class ZenithPolySynth : public ZenithInstrumentProcessor
{
public:
    ZenithPolySynth();
    ~ZenithPolySynth() override = default;

    //==========================================================================
    // Instrument interface
    //==========================================================================

    const InstrumentMetadata& getInstrumentMetadata() const override
    {
        return metadata_;
    }

    MacroEngine& getMacroEngine() override { return macroEngine_; }
    const MacroEngine& getMacroEngine() const override { return macroEngine_; }

    bool loadPreset(const ZenithInstrumentPreset& preset) override;
    ZenithInstrumentPreset getCurrentPreset() const override;

    //==========================================================================
    // AudioProcessor interface
    //==========================================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    //==========================================================================
    // Parameter access
    //==========================================================================

    juce::AudioProcessorValueTreeState& getParameters() { return parameters_; }

private:
    //==========================================================================
    // Metadata creation
    //==========================================================================

    static InstrumentMetadata createMetadata();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Parameter update
    //==========================================================================

    void updateParametersFromMacros();

    //==========================================================================
    // Member variables
    //==========================================================================

    InstrumentMetadata metadata_;
    MacroEngine macroEngine_;
    juce::AudioProcessorValueTreeState parameters_;
    juce::Synthesiser synth_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynth)
};

} // namespace zenith
