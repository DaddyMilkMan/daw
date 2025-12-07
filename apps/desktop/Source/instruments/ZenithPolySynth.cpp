/*
  ==============================================================================

    ZenithPolySynth.cpp
    Created: 2025-11-18
    Refactored: 2025-12-06
    Author:  Zenith DAW

    Implementation of ZenithPolySynth processor and instrument.

  ==============================================================================
*/

#define NOMINMAX
#include "ZenithPolySynth.h"
#include "ZenithPolySynthVoice.h"
#ifdef ZENITH_USE_SKIA
#include "../ui/skia/ZenithPolySynthUI.h"
#endif
#include "PresetGenerator.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// Parameter ID Strings
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

const juce::String ZenithPolySynthProcessor::FilterType = "filter_type";
const juce::String ZenithPolySynthProcessor::FilterCutoff = "filter_cutoff";
const juce::String ZenithPolySynthProcessor::FilterResonance = "filter_res";
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
const juce::String ZenithPolySynthProcessor::QualitySetting = "quality";

const juce::String ZenithPolySynthProcessor::DistortionAmount = "dist_amount";
const juce::String ZenithPolySynthProcessor::ChorusAmount = "chorus_amount";
const juce::String ZenithPolySynthProcessor::ReverbAmount = "reverb_amount";

// Unused but kept for compatibility/placeholders if needed
const juce::String ZenithPolySynthProcessor::NoiseLevel = "noise_level";
const juce::String ZenithPolySynthProcessor::SubOscLevel = "sub_level";
const juce::String ZenithPolySynthProcessor::FilterEnvAmount = "filter_env_amt";
const juce::String ZenithPolySynthProcessor::UnisonVoices = "unison_voices";
const juce::String ZenithPolySynthProcessor::UnisonDetune = "unison_detune";

//==============================================================================
// ZenithPolySynthProcessor Implementation
//==============================================================================

ZenithPolySynthProcessor::ZenithPolySynthProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "Parameters", createParameterLayout()) {
    // Add sound
    synthesiser_.addSound(new ZenithPolySynthSound());
    
    // Add voices
    for (int i = 0; i < 16; ++i) {
        synthesiser_.addVoice(new ZenithPolySynthVoice());
    }
    
    updateVoiceCount();
    
    // Initialize effects defaults
    effects_.setDistortion(0.0f);
    effects_.setChorus(0.0f);
    effects_.setReverb(0.0f);
}

ZenithPolySynthProcessor::~ZenithPolySynthProcessor() {}

void ZenithPolySynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    synthesiser_.setCurrentPlaybackSampleRate(sampleRate);
    
    for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
        if (auto *voice = static_cast<ZenithPolySynthVoice *>(synthesiser_.getVoice(i))) {
            voice->setSampleRate(sampleRate);
        }
    }
    
    effects_.setSampleRate(sampleRate);
    effects_.reset();
}

void ZenithPolySynthProcessor::releaseResources() {
    synthesiser_.allNotesOff(0, false);
}

void ZenithPolySynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear();
    
    // 1. Generate Synth Audio
    synthesiser_.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    // 2. Update Parameters (Control Rate)
    // Optimization: Don't update every block if not needed? 
    // For now, keep it to ensure responsiveness.
    updateVoiceParameters();
    
    // 3. Apply Global Effects
    if (buffer.getNumChannels() > 0) {
        float* leftCh = buffer.getWritePointer(0);
        float* rightCh = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : nullptr;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float l = leftCh[i];
            float r = (rightCh != nullptr) ? rightCh[i] : l; // Mono fallback
            
            effects_.process(l, r);
            
            leftCh[i] = l;
            if (rightCh) rightCh[i] = r;
        }
    }
    
    // 4. Apply Master Gain
    float masterGain = *parameters_.getRawParameterValue(MasterGain);
    buffer.applyGain(masterGain);
    
    // 5. Push to visualizer
    if (buffer.getNumChannels() > 0) {
        // Collect mono mix for visualizer if needed, or just L
        pushToVisualizer(buffer.getReadPointer(0), buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* ZenithPolySynthProcessor::createEditor() { 
#ifdef ZENITH_USE_SKIA
    return new ZenithPolySynthUI(*this); 
#else
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

void ZenithPolySynthProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ZenithPolySynthProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(parameters_.state.getType())) {
            parameters_.replaceState(juce::ValueTree::fromXml(*xmlState));
            // Force update voice parameters after state load
            updateVoiceParameters();
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ZenithPolySynthProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Oscillator parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc1Wave, "Osc1 Wave", 0.0f, 5.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc1Detune, "Osc1 Detune", -100.0f, 100.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc1Mix, "Osc1 Mix", 0.0f, 1.0f, 1.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc2Wave, "Osc2 Wave", 0.0f, 5.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc2Detune, "Osc2 Detune", -100.0f, 100.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc2Mix, "Osc2 Mix", 0.0f, 1.0f, 0.5f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc3Wave, "Osc3 Wave", 0.0f, 5.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc3Detune, "Osc3 Detune", -100.0f, 100.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(Osc3Mix, "Osc3 Mix", 0.0f, 1.0f, 0.0f));
    
    // Filter parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(FilterCutoff, "Filter Cutoff", 20.0f, 20000.0f, 1000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(FilterResonance, "Filter Resonance", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(FilterDrive, "Filter Drive", 1.0f, 10.0f, 1.0f));
    
    // Envelope parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(AmpAttack, "Amp Attack", 0.001f, 5.0f, 0.01f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(AmpDecay, "Amp Decay", 0.001f, 5.0f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(AmpSustain, "Amp Sustain", 0.0f, 1.0f, 0.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(AmpRelease, "Amp Release", 0.001f, 5.0f, 0.1f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(ModAttack, "Mod Attack", 0.001f, 5.0f, 0.01f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ModDecay, "Mod Decay", 0.001f, 5.0f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ModSustain, "Mod Sustain", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ModRelease, "Mod Release", 0.001f, 5.0f, 0.1f));
    
    // LFO parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(LFO1Rate, "LFO1 Rate", 0.1f, 20.0f, 2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(LFO1Amount, "LFO1 Amount", 0.0f, 1.0f, 0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(LFO2Rate, "LFO2 Rate", 0.1f, 20.0f, 4.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(LFO2Amount, "LFO2 Amount", 0.0f, 1.0f, 0.0f));
    
    // Master parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(MasterGain, "Master Gain", 0.0f, 2.0f, 0.8f));
    layout.add(std::make_unique<juce::AudioParameterInt>(MaxVoices, "Max Voices", 1, 32, 16));
    
    // Effects
    layout.add(std::make_unique<juce::AudioParameterFloat>(DistortionAmount, "Distortion", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ChorusAmount, "Chorus", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ReverbAmount, "Reverb", 0.0f, 1.0f, 0.0f));
    
    return layout;
}

void ZenithPolySynthProcessor::updateVoiceParameters() {
    // Update global effects parameters
    effects_.setDistortion(*parameters_.getRawParameterValue(DistortionAmount));
    effects_.setChorus(*parameters_.getRawParameterValue(ChorusAmount));
    effects_.setReverb(*parameters_.getRawParameterValue(ReverbAmount));

    // Update all voices with current parameter values
    for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
        if (auto *voice = static_cast<ZenithPolySynthVoice *>(synthesiser_.getVoice(i))) {
            // Update oscillators
            voice->setOsc1Waveform(static_cast<OscillatorWaveform>(static_cast<int>(*parameters_.getRawParameterValue(Osc1Wave))));
            voice->setOsc1Detune(*parameters_.getRawParameterValue(Osc1Detune));
            voice->setOsc1Mix(*parameters_.getRawParameterValue(Osc1Mix));
            
            voice->setOsc2Waveform(static_cast<OscillatorWaveform>(static_cast<int>(*parameters_.getRawParameterValue(Osc2Wave))));
            voice->setOsc2Detune(*parameters_.getRawParameterValue(Osc2Detune));
            voice->setOsc2Mix(*parameters_.getRawParameterValue(Osc2Mix));
            
            voice->setOsc3Waveform(static_cast<OscillatorWaveform>(static_cast<int>(*parameters_.getRawParameterValue(Osc3Wave))));
            voice->setOsc3Detune(*parameters_.getRawParameterValue(Osc3Detune));
            voice->setOsc3Mix(*parameters_.getRawParameterValue(Osc3Mix));
            
            // Update filter
            voice->setFilterCutoff(*parameters_.getRawParameterValue(FilterCutoff));
            voice->setFilterResonance(*parameters_.getRawParameterValue(FilterResonance));
            voice->setFilterDrive(*parameters_.getRawParameterValue(FilterDrive));
            
            // Update envelopes
            voice->setAmpEnvelope(
                *parameters_.getRawParameterValue(AmpAttack),
                *parameters_.getRawParameterValue(AmpDecay),
                *parameters_.getRawParameterValue(AmpSustain),
                *parameters_.getRawParameterValue(AmpRelease)
            );
            
            voice->setModEnvelope(
                *parameters_.getRawParameterValue(ModAttack),
                *parameters_.getRawParameterValue(ModDecay),
                *parameters_.getRawParameterValue(ModSustain),
                *parameters_.getRawParameterValue(ModRelease)
            );
            
            // Update LFOs
            voice->setLFO1(*parameters_.getRawParameterValue(LFO1Rate), 
                          *parameters_.getRawParameterValue(LFO1Amount),
                          LFOTarget::FilterCutoff);
                          
            voice->setLFO2(*parameters_.getRawParameterValue(LFO2Rate),
                          *parameters_.getRawParameterValue(LFO2Amount),
                          LFOTarget::FilterCutoff);
                          
            // We should also pass specific modulation matrix slots if we had parameters for them.
            // Since we have a globalModMatrix_, we should update the voice's internal matrix.
            // Ideally ZenithPolySynthVoice should point to a shared matrix, but for now we copy or update slots.
            
            for (int s=0; s < 64; ++s) {
                 if (globalModMatrix_[s].isActive()) {
                     voice->setModulationSlot(s, globalModMatrix_[s].source, globalModMatrix_[s].destination, globalModMatrix_[s].amount);
                 } else {
                     // Clear slot in voice if inactive
                     voice->setModulationSlot(s, ModulationSource::None, ModulationDestination::None, 0.0f);
                 }
            }
        }
    }
}

void ZenithPolySynthProcessor::updateVoiceCount() {
    int targetVoices = static_cast<int>(*parameters_.getRawParameterValue(MaxVoices));
    int currentVoices = synthesiser_.getNumVoices();
    
    if (targetVoices > currentVoices) {
        for (int i = currentVoices; i < targetVoices; ++i) {
            synthesiser_.addVoice(new ZenithPolySynthVoice());
        }
    } else if (targetVoices < currentVoices) {
        for (int i = currentVoices - 1; i >= targetVoices; --i) {
            synthesiser_.removeVoice(i);
        }
    }
}

float ZenithPolySynthProcessor::getModulationMatrix(ModulationSource src, ModulationDestination dst) const {
    for (const auto& slot : globalModMatrix_) {
        if (slot.source == src && slot.destination == dst) {
            return slot.amount;
        }
    }
    return 0.0f;
}

void ZenithPolySynthProcessor::setModulationMatrix(ModulationSource src, ModulationDestination dst, float amount) {
    // 1. Update existing
    for (auto& slot : globalModMatrix_) {
        if (slot.source == src && slot.destination == dst) {
            slot.amount = amount;
            if (std::abs(amount) < 0.001f) {
                slot.source = ModulationSource::None;
                slot.destination = ModulationDestination::None;
                slot.amount = 0.0f;
            }
            return;
        }
    }
    
    // 2. Find empty slot
    if (std::abs(amount) > 0.001f) {
        for (auto& slot : globalModMatrix_) {
            if (!slot.isActive()) {
                slot.source = src;
                slot.destination = dst;
                slot.amount = amount;
                return;
            }
        }
    }
}

int ZenithPolySynthProcessor::readFromVisualizer(float* bufferData, int numSamples) {
    int s1, s2;
    int numReady = visualizerFifo_.getNumReady();
    if (numReady > 0) {
        int toRead = std::min(numReady, numSamples);
        visualizerFifo_.prepareToRead(toRead, s1, s2);
        
        // Circular buffer read
        // visualizerBuffer_.size() is 4096.
        for (int i = 0; i < s1; ++i) bufferData[i] = visualizerBuffer_[i]; // This logic is wrong, visualizerBuffer_ indices are not 0..s1 directly if FIFO...
        
        // Wait, prepareToRead returns START INDICES.
        // But AbstractFifo usage usually involves getReadPointer logic on the buffer itself using the indices.
        // My visualizerBuffer_ is just a vector std::vector<float>.
        
        // Correct usage:
        // visualizerFifo_ manages indices into visualizerBuffer_.
        // BUT visualizerFifo_.prepareToRead gives us start1, block1, start2, block2.
        
        // Oh, prepareToRead returns void but sets start1, len1, start2, len2 references.
        // My previous code: visualizerFifo_.prepareToRead(toRead, s1, s2);
        // This signature doesn't match standard JUCE AbstractFifo::prepareToRead(int numWanted, int& startIndex1, int& blockSize1, int& startIndex2, int& blockSize2) ?
        
        // Actually JUCE AbstractFifo::prepareToRead(int numWanted, int& start1, int& size1, int& start2, int& size2)
        
        int start1, size1, start2, size2;
        visualizerFifo_.prepareToRead(toRead, start1, size1, start2, size2);
        
        if (size1 > 0) {
            for(int i=0; i<size1; ++i) bufferData[i] = visualizerBuffer_[start1 + i];
        }
        if (size2 > 0) {
            for(int i=0; i<size2; ++i) bufferData[size1 + i] = visualizerBuffer_[start2 + i];
        }
        
        visualizerFifo_.finishedRead(toRead);
        return toRead;
    }
    return 0;
}

void ZenithPolySynthProcessor::pushToVisualizer(const float* bufferData, int numSamples) {
    int start1, size1, start2, size2;
    visualizerFifo_.prepareToWrite(numSamples, start1, size1, start2, size2);
    
    if (size1 > 0) {
        for(int i=0; i<size1; ++i) visualizerBuffer_[start1 + i] = bufferData[i];
    }
    if (size2 > 0) {
         for(int i=0; i<size2; ++i) visualizerBuffer_[start2 + i] = bufferData[size1 + i];
    }
    
    visualizerFifo_.finishedWrite(size1 + size2);
}

//==============================================================================
// ZenithPolySynth Instrument Wrapper
//==============================================================================

ZenithPolySynth::ZenithPolySynth() 
    : InstrumentBase("Zenith Poly Synth")
{
}

InstrumentMetadata ZenithPolySynth::createMetadata() {
    InstrumentMetadata meta;
    meta.name = "Zenith Poly Synth";
    meta.category = "Synth";
    meta.description = "Advanced Polyphonic Synthesizer";
    meta.createProcessor = []() -> std::unique_ptr<juce::AudioProcessor> {
        return std::make_unique<ZenithPolySynthProcessor>();
    };
    return meta;
}

void ZenithPolySynth::registerPresets() {
    // TODO: move presets here from PresetGenerator or call it
}

} // namespace zenith
