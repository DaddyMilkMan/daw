/*
  ==============================================================================

    ZenithPolySynth.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Implementation of ZenithPolySynth.

  ==============================================================================
*/

#include "ZenithPolySynth.h"
#include "ZenithPolySynthVoice.h"
#include "InstrumentMetadata.h"

namespace zenith {

//==============================================================================
// Parameter ID Definitions
//==============================================================================

const juce::String ZenithPolySynthProcessor::Osc1Wave = "osc1_wave";
const juce::String ZenithPolySynthProcessor::Osc1Detune = "osc1_detune";
const juce::String ZenithPolySynthProcessor::Osc1Mix = "osc1_mix";
const juce::String ZenithPolySynthProcessor::Osc2Wave = "osc2_wave";
const juce::String ZenithPolySynthProcessor::Osc2Detune = "osc2_detune";
const juce::String ZenithPolySynthProcessor::Osc2Mix = "osc2_mix";
const juce::String ZenithPolySynthProcessor::Osc3Wave = "osc3_wave";
const juce::String ZenithPolySynthProcessor::Osc3Detune = "osc3_detune";
const juce::String ZenithPolySynthProcessor::Osc3Mix = "osc3_mix";
const juce::String ZenithPolySynthProcessor::NoiseLevel = "noise_level";
const juce::String ZenithPolySynthProcessor::SubOscLevel = "sub_osc_level";
const juce::String ZenithPolySynthProcessor::FilterEnvAmount = "filter_env_amount";

const juce::String ZenithPolySynthProcessor::UnisonVoices = "unison_voices";
const juce::String ZenithPolySynthProcessor::UnisonDetune = "unison_detune";

const juce::String ZenithPolySynthProcessor::FilterType = "filter_type";
const juce::String ZenithPolySynthProcessor::FilterCutoff = "filter_cutoff";
const juce::String ZenithPolySynthProcessor::FilterResonance = "filter_resonance";
const juce::String ZenithPolySynthProcessor::FilterDrive = "filter_drive";

const juce::String ZenithPolySynthProcessor::AmpAttack = "amp_attack";
const juce::String ZenithPolySynthProcessor::AmpDecay = "amp_decay";
const juce::String ZenithPolySynthProcessor::AmpSustain = "amp_sustain";
const juce::String ZenithPolySynthProcessor::AmpRelease = "amp_release";

const juce::String ZenithPolySynthProcessor::ModAttack = "mod_attack";
const juce::String ZenithPolySynthProcessor::ModDecay = "mod_decay";
const juce::String ZenithPolySynthProcessor::ModSustain = "mod_sustain";
const juce::String ZenithPolySynthProcessor::ModRelease = "mod_release";

const juce::String ZenithPolySynthProcessor::LFO1Rate = "lfo1_rate";
const juce::String ZenithPolySynthProcessor::LFO1Amount = "lfo1_amount";
const juce::String ZenithPolySynthProcessor::LFO1Target = "lfo1_target";

const juce::String ZenithPolySynthProcessor::LFO2Rate = "lfo2_rate";
const juce::String ZenithPolySynthProcessor::LFO2Amount = "lfo2_amount";
const juce::String ZenithPolySynthProcessor::LFO2Target = "lfo2_target";

const juce::String ZenithPolySynthProcessor::GlideTime = "glide_time";
const juce::String ZenithPolySynthProcessor::MonoMode = "mono_mode";
const juce::String ZenithPolySynthProcessor::MasterGain = "master_gain";

const juce::String ZenithPolySynthProcessor::MaxVoices = "max_voices";
const juce::String ZenithPolySynthProcessor::QualitySetting = "quality_setting";

const juce::String ZenithPolySynthProcessor::DistortionAmount = "distortion_amount";
const juce::String ZenithPolySynthProcessor::ChorusAmount = "chorus_amount";
const juce::String ZenithPolySynthProcessor::ReverbAmount = "reverb_amount";

//==============================================================================
// ZenithPolySynthProcessor Implementation
//==============================================================================

ZenithPolySynthProcessor::ZenithPolySynthProcessor()
    : parameters_(*this, nullptr, "ZenithPolySynthParams", createParameterLayout())
{
    // Add voices
    for (int i = 0; i < 16; ++i) {
        synthesiser_.addVoice(new ZenithPolySynthVoice());
    }
    
    // Add sound
    synthesiser_.addSound(new ZenithPolySynthSound());
}

ZenithPolySynthProcessor::~ZenithPolySynthProcessor() {
}

void ZenithPolySynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    synthesiser_.setCurrentPlaybackSampleRate(sampleRate);
    effects_.setSampleRate(sampleRate);
    effects_.reset();
    
    for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<ZenithPolySynthVoice*>(synthesiser_.getVoice(i))) {
            voice->setSampleRate(sampleRate);
        }
    }
}

void ZenithPolySynthProcessor::releaseResources() {
}

void ZenithPolySynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    updateVoiceParameters();
    
    // Render synth
    synthesiser_.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    // Apply effects (iterate samples)
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float l = left[i];
        float r = right ? right[i] : l;
        
        effects_.process(l, r);
        
        left[i] = l;
        if (right) right[i] = r;
    }
    
    // Visualizer
    pushToVisualizer(buffer.getReadPointer(0), buffer.getNumSamples());
}

juce::AudioProcessorEditor* ZenithPolySynthProcessor::createEditor() {
    return nullptr; // TODO: Implement editor
}

void ZenithPolySynthProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ZenithPolySynthProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(parameters_.state.getType()))
            parameters_.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout ZenithPolySynthProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Oscillators
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc1Mix, "Osc 1 Mix", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(Osc1Wave, "Osc 1 Wave", juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"}, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc1Detune, "Osc 1 Detune", -100.0f, 100.0f, 0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc2Mix, "Osc 2 Mix", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(Osc2Wave, "Osc 2 Wave", juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"}, 2));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc2Detune, "Osc 2 Detune", -100.0f, 100.0f, 0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc3Mix, "Osc 3 Mix", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(Osc3Wave, "Osc 3 Wave", juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"}, 3));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc3Detune, "Osc 3 Detune", -100.0f, 100.0f, 0.0f));
    
    // Filter
    layout.add(std::make_unique<juce::AudioParameterChoice>(FilterType, "Filter Type", juce::StringArray{"Lowpass", "Bandpass", "Highpass"}, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(FilterCutoff, "Cutoff", 20.0f, 20000.0f, 1000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(FilterResonance, "Resonance", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(FilterDrive, "Drive", 0.0f, 10.0f, 0.0f));
    
    // Amp Env
    layout.add(std::make_unique<juce::AudioParameterFloat>(AmpAttack, "Amp Attack", 0.001f, 5.0f, 0.01f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(AmpDecay, "Amp Decay", 0.001f, 5.0f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(AmpSustain, "Amp Sustain", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(AmpRelease, "Amp Release", 0.001f, 10.0f, 0.1f));
    
    // Unison
    layout.add(std::make_unique<juce::AudioParameterInt>(UnisonVoices, "Unison Voices", 1, 7, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(UnisonDetune, "Unison Detune", 0.0f, 100.0f, 0.0f));
    
    // Global
    layout.add(std::make_unique<juce::AudioParameterFloat>(MasterGain, "Master Gain", 0.0f, 2.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(GlideTime, "Glide Time", 0.0f, 2.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(MonoMode, "Mono Mode", false));
    
    // Effects
    layout.add(std::make_unique<juce::AudioParameterFloat>(DistortionAmount, "Distortion", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ChorusAmount, "Chorus", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ReverbAmount, "Reverb", 0.0f, 1.0f, 0.0f));
    
    // Placeholders
    layout.add(std::make_unique<juce::AudioParameterFloat>(NoiseLevel, "Noise Level", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(SubOscLevel, "Sub Level", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(FilterEnvAmount, "Filter Env Amount", 0.0f, 1.0f, 0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(ModAttack, "Mod Attack", 0.001f, 5.0f, 0.01f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ModDecay, "Mod Decay", 0.001f, 5.0f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ModSustain, "Mod Sustain", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ModRelease, "Mod Release", 0.001f, 10.0f, 0.1f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(LFO1Rate, "LFO1 Rate", 0.1f, 20.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(LFO1Amount, "LFO1 Amount", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(LFO1Target, "LFO1 Target", juce::StringArray{"Cutoff", "Pitch", "Mix"}, 0));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(LFO2Rate, "LFO2 Rate", 0.1f, 20.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(LFO2Amount, "LFO2 Amount", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(LFO2Target, "LFO2 Target", juce::StringArray{"Cutoff", "Pitch", "Mix"}, 0));
    
    return layout;
}

void ZenithPolySynthProcessor::updateVoiceParameters() {
    for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<ZenithPolySynthVoice*>(synthesiser_.getVoice(i))) {
            voice->setOsc1Mix(*parameters_.getRawParameterValue(Osc1Mix));
            voice->setOsc1Waveform(static_cast<OscillatorWaveform>((int)*parameters_.getRawParameterValue(Osc1Wave)));
            voice->setOsc1Detune(*parameters_.getRawParameterValue(Osc1Detune));
            
            voice->setOsc2Mix(*parameters_.getRawParameterValue(Osc2Mix));
            voice->setOsc2Waveform(static_cast<OscillatorWaveform>((int)*parameters_.getRawParameterValue(Osc2Wave)));
            voice->setOsc2Detune(*parameters_.getRawParameterValue(Osc2Detune));
            
            voice->setOsc3Mix(*parameters_.getRawParameterValue(Osc3Mix));
            voice->setOsc3Waveform(static_cast<OscillatorWaveform>((int)*parameters_.getRawParameterValue(Osc3Wave)));
            voice->setOsc3Detune(*parameters_.getRawParameterValue(Osc3Detune));
            
            voice->setFilterType(static_cast<zenith::FilterType>((int)*parameters_.getRawParameterValue(FilterType)));
            voice->setFilterCutoff(*parameters_.getRawParameterValue(FilterCutoff));
            voice->setFilterResonance(*parameters_.getRawParameterValue(FilterResonance));
            voice->setFilterDrive(*parameters_.getRawParameterValue(FilterDrive));
            
            voice->setAmpEnvelope(*parameters_.getRawParameterValue(AmpAttack),
                                  *parameters_.getRawParameterValue(AmpDecay),
                                  *parameters_.getRawParameterValue(AmpSustain),
                                  *parameters_.getRawParameterValue(AmpRelease));
                                  
            voice->setUnisonVoices((int)*parameters_.getRawParameterValue(UnisonVoices));
            voice->setUnisonDetune(*parameters_.getRawParameterValue(UnisonDetune));
            
            voice->setGlideTime(*parameters_.getRawParameterValue(GlideTime));
            voice->setMonoMode(*parameters_.getRawParameterValue(MonoMode) > 0.5f);
        }
    }
    
    effects_.setDistortion(*parameters_.getRawParameterValue(DistortionAmount));
    effects_.setChorus(*parameters_.getRawParameterValue(ChorusAmount));
    effects_.setReverb(*parameters_.getRawParameterValue(ReverbAmount));
}

float ZenithPolySynthProcessor::getModulationMatrix(ModulationSource src, ModulationDestination dst) const {
    return 0.0f;
}

void ZenithPolySynthProcessor::setModulationMatrix(ModulationSource src, ModulationDestination dst, float amount) {
}

int ZenithPolySynthProcessor::readFromVisualizer(float* buffer, int numSamples) {
    return 0; // Stub
}

void ZenithPolySynthProcessor::pushToVisualizer(const float* buffer, int numSamples) {
}

//==============================================================================
// ZenithPolySynth Instrument Wrapper
//==============================================================================

ZenithPolySynth::ZenithPolySynth() 
    : InstrumentBase(std::make_unique<ZenithPolySynthProcessor>(), createMetadata())
{
}

InstrumentMetadata ZenithPolySynth::createMetadata() {
    InstrumentMetadata meta;
    meta.instrumentId = "zenith_poly_synth";
    meta.name = "Zenith Poly Synth";
    meta.category = "Synthesizer";
    meta.description = "Multi-oscillator subtractive synthesizer with dual filters and effects.";
    meta.version = "1.0.0";
    meta.author = "Zenith DAW";
    meta.isBuiltIn = true;
    return meta;
}

void ZenithPolySynth::registerPresets() {
    // TODO: Register factory presets
}

} // namespace zenith