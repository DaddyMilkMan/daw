/*
    Zenith DAW - Professional Synthesizer
    Copyright (C)2025 Micah Cooley <micahcooley@protonmail.com>
    AGPL-3.0
*/

#include "ZenithSynthProcessor.h"

namespace zenith {

//==============================================================================
// PARAMETERS
//==============================================================================

static const char* PARAM_GAIN = "master_gain";
static const char* PARAM_POLY = "poly_mode";
static const char* PARAM_OSC1_WAVE = "osc1_waveform";
static const char* PARAM_FILTER_TYPE = "filter1_type";
static const char* PARAM_FILTER_CUTOFF = "filter1_cutoff";
static const char* PARAM_FILTER_RES = "filter1_resonance";

//==============================================================================
// CONSTRUCTOR
//==============================================================================

ZenithSynthProcessor::ZenithSynthProcessor() {
    // Initialize voices
    for (auto& voice : voices_) {
        voice.setSampleRate(44100.0);
    }

    setupParameters();
    setupModulation();
}

//==============================================================================
// PARAMETERS
//==============================================================================

void ZenithSynthProcessor::setupParameters() {
    // Master gain: 0-100%
    gainParam_ = parameters_.createAndAddParameter(
        std::make_unique<juce::AudioParameterFloat>(
            PARAM_GAIN, "Master Gain", "",
            0.0f, 1.0f, 0.8f));

    // Poly/mono mode
    polyModeParam_ = parameters_.createAndAddParameter(
        std::make_unique<juce::AudioParameterBool>(
            PARAM_POLY, "Poly Mode", "", true));

    // Osc 1 waveform
    auto osc1Wave = std::make_unique<juce::AudioParameterChoice>(
        PARAM_OSC1_WAVE, "OSC1 Waveform", "",
        juce::StringArray("Saw", "Square", "Triangle", "Sine", "Wavetable"), 0);

    parameters_.addParameter(osc1Wave.release());

    // Filter type
    auto filterType = std::make_unique<juce::AudioParameterChoice>(
        PARAM_FILTER_TYPE, "Filter Type", "",
        juce::StringArray("SVF", "Moog", "MS-20", "SEM", "TB-303"), 0);

    parameters_.addParameter(filterType.release());

    // Filter cutoff
    parameters_.createAndAddParameter(
        std::make_unique<juce::AudioParameterFloat>(
            PARAM_FILTER_CUTOFF, "Filter Cutoff", "Hz",
            20.0f, 20000.0f, 1000.0f));

    // Filter resonance
    parameters_.createAndAddParameter(
        std::make_unique<juce::AudioParameterFloat>(
            PARAM_FILTER_RES, "Filter Resonance", "",
            0.0f, 1.0f, 0.5f));

    // State
    parameters_.state = juce::ValueTree(
        juce::Identifier("ZenithSynth"));
}

void ZenithSynthProcessor::setupModulation() {
    // LFO1 -> Filter Cutoff (default mod routing)
    for (auto& voice : voices_) {
        voice.setModulationSlot(0,
            ModulationSource::LFO1,
            ModulationDestination::FilterCutoff,
            0.3f);
    }
}

//==============================================================================
// PREPARE
//==============================================================================

void ZenithSynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Prepare voices
    for (auto& voice : voices_) {
        voice.setSampleRate(sampleRate);
    }

    // Prepare effects
    effects_.setSampleRate(sampleRate);
}

void ZenithSynthProcessor::releaseResources() {
    effects_.reset();
}

//==============================================================================
// PROCESS
//==============================================================================

void ZenithSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midi) {
    buffer.clear();

    // Process MIDI
    juce::MPESynthesiserBase::processMidiBuffer(midi,
        getSampleRate(), getBlockSize());

    // Render voices
    for (auto& voice : voices_) {
        if (voice.isActive()) {
            voice.renderNextBlock(buffer, 0, buffer.getNumSamples());
        }
    }

    // Apply master gain
    float gain = gainParam_->load(std::memory_order_relaxed);
    buffer.applyGain(gain);

    // Process effects chain
    effects_.process(buffer);
}

//==============================================================================
// EDITOR
//==============================================================================

juce::AudioProcessorEditor* ZenithSynthProcessor::createEditor() {
    // Editor will be created separately
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
// STATE
//==============================================================================

void ZenithSynthProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters_.copyState();
    destData.reset();
    destData.append(state.getData(), state.getDataSize());
}

void ZenithSynthProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::ValueTree tree = juce::ValueTree::readFromData(
        data, sizeInBytes);

    if (tree.isValid()) {
        parameters_.replaceState(tree);
    }
}

} // namespace zenith
