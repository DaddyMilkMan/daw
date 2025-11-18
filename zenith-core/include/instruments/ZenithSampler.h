/**
 * @file ZenithSampler.h
 * @brief Simple sample playback instrument with macro controls
 *
 * Features:
 * - Sample playback with ADSR envelope
 * - Basic filter
 * - Sample start/end controls
 * - 4 smart macros (Body, Snap, LoFi, Tone)
 */

#pragma once

#include <JuceHeader.h>
#include "InstrumentRegistry.h"
#include "InstrumentMetadata.h"
#include "InstrumentPreset.h"

namespace zenith {

//==============================================================================
/**
 * @brief Sampler voice with envelope and filter
 */
class ZenithSamplerVoice : public juce::SamplerVoice
{
public:
    ZenithSamplerVoice() = default;

    void setEnvelopeParameters(float attack, float decay, float sustain, float release)
    {
        juce::ADSR::Parameters params;
        params.attack = attack;
        params.decay = decay;
        params.sustain = sustain;
        params.release = release;
        envelope_.setParameters(params);
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override
    {
        if (auto* playingSound = static_cast<juce::SamplerSound*>(getCurrentlyPlayingSound().get()))
        {
            auto& data = *playingSound->getAudioData();
            const float* const inL = data.getReadPointer(0);
            const float* const inR = data.getNumChannels() > 1 ? data.getReadPointer(1) : nullptr;

            float* outL = outputBuffer.getWritePointer(0, startSample);
            float* outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;

            while (--numSamples >= 0)
            {
                auto pos = (int) sourceSamplePosition;
                auto alpha = (float) (sourceSamplePosition - pos);
                auto invAlpha = 1.0f - alpha;

                // Simple linear interpolation
                float l = (inL[pos] * invAlpha + inL[pos + 1] * alpha);
                float r = (inR != nullptr) ? (inR[pos] * invAlpha + inR[pos + 1] * alpha) : l;

                // Apply envelope
                auto envelopeValue = envelope_.getNextSample();
                l *= lgain * envelopeValue;
                r *= rgain * envelopeValue;

                if (outR != nullptr)
                {
                    *outL++ += l;
                    *outR++ += r;
                }
                else
                {
                    *outL++ += (l + r) * 0.5f;
                }

                sourceSamplePosition += pitchRatio;

                if (sourceSamplePosition > playingSound->getLength() || !envelope_.isActive())
                {
                    stopNote(0.0f, false);
                    break;
                }
            }
        }
    }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* s, int pitchWheel) override
    {
        juce::SamplerVoice::startNote(midiNoteNumber, velocity, s, pitchWheel);
        envelope_.noteOn();
    }

    void stopNote(float velocity, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            envelope_.noteOff();
        }
        else
        {
            clearCurrentNote();
            envelope_.reset();
        }
    }

    void prepareToPlay(double sampleRate)
    {
        envelope_.setSampleRate(sampleRate);
    }

private:
    juce::ADSR envelope_;
};

//==============================================================================
/**
 * @brief Zenith Sampler instrument processor
 */
class ZenithSampler : public ZenithInstrumentProcessor
{
public:
    ZenithSampler();
    ~ZenithSampler() override = default;

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

    /**
     * @brief Load a sample file
     */
    void loadSample(const juce::File& file);

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
    juce::Synthesiser sampler_;
    juce::AudioFormatManager formatManager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSampler)
};

} // namespace zenith
