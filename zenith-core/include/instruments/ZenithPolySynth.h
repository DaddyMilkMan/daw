#pragma once

#include "instruments/ZenithPolySynthVoice.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {
namespace instruments {

/**
 * @brief ZenithPolySynth - A polyphonic subtractive synthesizer
 *
 * This is a built-in instrument plugin for Zenith DAW featuring:
 * - Two oscillators with multiple waveforms (sine, saw, square, triangle)
 * - Detune and oscillator mixing
 * - White noise generator
 * - State Variable Filter (LP/BP/HP)
 * - ADSR amplitude envelope
 * - Up to 16 voices of polyphony
 *
 * All processing is RT-safe with no allocations in the audio thread.
 */
class ZenithPolySynth : public juce::AudioProcessor
{
public:
    ZenithPolySynth();
    ~ZenithPolySynth() override;

    //==============================================================================
    // AudioProcessor interface

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    //==============================================================================
    // Editor

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    // Program/preset management

    const juce::String getName() const override { return "Zenith Poly Synth"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 2.0; } // For release tail

    //==============================================================================
    // State save/load

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
    const juce::String getProgramName(int index) override { juce::ignoreUnused(index); return "Default"; }
    void changeProgramName(int index, const juce::String& newName) override { juce::ignoreUnused(index, newName); }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter access

    juce::AudioProcessorValueTreeState& getParameters() { return parameters_; }

private:
    //==============================================================================
    // Parameter creation

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateVoiceParameters();

    //==============================================================================
    // Synthesizer engine

    juce::Synthesiser synth_;
    static constexpr int kNumVoices = 16;

    //==============================================================================
    // Parameters

    juce::AudioProcessorValueTreeState parameters_;

    // Parameter IDs
    static inline const juce::String kParamOsc1Wave = "osc1_wave";
    static inline const juce::String kParamOsc2Wave = "osc2_wave";
    static inline const juce::String kParamOsc2Detune = "osc2_detune";
    static inline const juce::String kParamOscMix = "osc_mix";
    static inline const juce::String kParamNoiseLevel = "noise_level";

    static inline const juce::String kParamFilterType = "filter_type";
    static inline const juce::String kParamFilterCutoff = "filter_cutoff";
    static inline const juce::String kParamFilterResonance = "filter_resonance";

    static inline const juce::String kParamEnvAttack = "env_attack";
    static inline const juce::String kParamEnvDecay = "env_decay";
    static inline const juce::String kParamEnvSustain = "env_sustain";
    static inline const juce::String kParamEnvRelease = "env_release";

    static inline const juce::String kParamMasterGain = "master_gain";

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynth)
};

/**
 * @brief Factory function for creating ZenithPolySynth instances
 *
 * This is used by the InstrumentRegistry to instantiate the synth.
 *
 * @return A new ZenithPolySynth instance
 */
std::unique_ptr<juce::AudioProcessor> createZenithPolySynth();

} // namespace instruments
} // namespace zenith
