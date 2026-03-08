/*
    Zenith DAW - Professional Synthesizer
    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
    AGPL-3.0
*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include "ZenithPolySynthVoice.h"
#include "ZenithModulationMatrix.h"
#include "ZenithEffects.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

//==============================================================================
// MAIN SYNTH PROCESSOR
//==============================================================================

/**
    ZenithSynthProcessor

    Shipping-quality MPE polysynth matching Serum/Vital.

    Features:
    - 16-voice polyphony with MPE
    - 3 oscillators per voice with sync/FM/ring
    - 5 filter models with oversampling
    - 32-slot modulation matrix
    - Full effects chain
    - Wavetable import/export
*/
class ZenithSynthProcessor : public juce::AudioProcessor,
                            public juce::AudioProcessorValueTreeState::Listener {
public:
    ZenithSynthProcessor();
    ~ZenithSynthProcessor() override = default;

    //==========================================================================
    // AudioProcessor Overrides
    //==========================================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Zenith"; }

    double getTailLengthSeconds() const override { return 1.0; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }

    //==========================================================================
    // Programs
    //==========================================================================

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }

    //==========================================================================
    // State
    //==========================================================================

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    //==========================================================================
    // Parameters
    //==========================================================================

    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters_; }

    ZenithPolySynthVoice& getVoice(int index) { return voices_[index]; }

private:
    //==========================================================================
    // Voices
    //==========================================================================

    static constexpr int MAX_VOICES = 16;
    std::array<ZenithPolySynthVoice, MAX_VOICES> voices_;

    //==========================================================================
    // Effects
    //==========================================================================

    ZenithEffectsChain effects_;

    //==========================================================================
    // Parameters
    //==========================================================================

    juce::AudioProcessorValueTreeState parameters_ {
        *this, nullptr, "ZenithParameters"
    };

    std::atomic<float>* gainParam_ = nullptr;
    std::atomic<float>* polyModeParam_ = nullptr;

    //==========================================================================
    // Initialization
    //==========================================================================

    void setupParameters();
    void setupModulation();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSynthProcessor)
};

} // namespace zenith
