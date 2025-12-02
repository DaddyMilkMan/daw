/*
  ==============================================================================

    ZenithPolySynth.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of ZenithPolySynth - multi-oscillator subtractive
    synthesizer.

  ==============================================================================
*/

#define NOMINMAX
#include "ZenithPolySynth.h"
#include "../ui/skia/ZenithPolySynthUI.h"
#include "../../src/utils/PresetGenerator.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// ZenithOscillator Implementation
//==============================================================================

float ZenithOscillator::getNextSample(float frequency, float shape) {
    // Apply detune
    float detuneMultiplier = std::pow(2.0f, detuneCents_ / 1200.0f);
    frequency *= detuneMultiplier;
    
    switch (waveform_) {
        case OscillatorWaveform::Sine:
            return processSine(frequency);
        case OscillatorWaveform::Saw:
            return processSaw(frequency);
        case OscillatorWaveform::Square:
            return processSquare(frequency, shape);
        case OscillatorWaveform::Triangle:
            return processTriangle(frequency);
        case OscillatorWaveform::Noise:
            return processNoise();
        default:
            return 0.0f;
    }
}

float ZenithOscillator::processSine(float frequency) {
    float sample = std::sin(phase_ * juce::MathConstants<double>::twoPi);
    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processSaw(float frequency) {
    float sample = 2.0f * static_cast<float>(phase_) - 1.0f;
    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processSquare(float frequency, float pulseWidth) {
    float sample = (phase_ < pulseWidth) ? 1.0f : -1.0f;
    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processTriangle(float frequency) {
    float sample;
    if (phase_ < 0.5) {
        sample = 4.0f * static_cast<float>(phase_) - 1.0f;
    } else {
        sample = 3.0f - 4.0f * static_cast<float>(phase_);
    }
    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processNoise() {
    return random_.nextFloat() * 2.0f - 1.0f;
}

//==============================================================================
// ZenithFilter Implementation
//==============================================================================

void ZenithFilter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    cutoffSmoothed_.reset(sampleRate, 0.05);
    resonanceSmoothed_.reset(sampleRate, 0.05);
}

void ZenithFilter::setCutoff(float cutoffHz) {
    cutoffSmoothed_.setTargetValue(juce::jlimit(20.0f, 20000.0f, cutoffHz));
}

void ZenithFilter::setResonance(float resonance) {
    resonanceSmoothed_.setTargetValue(juce::jlimit(0.0f, 1.0f, resonance));
}

void ZenithFilter::reset() {
    v0_ = v1_ = v2_ = 0.0f;
    ic1eq_ = ic2eq_ = 0.0f;
    cutoffSmoothed_.setCurrentAndTargetValue(1000.0f);
    resonanceSmoothed_.setCurrentAndTargetValue(0.0f);
}

float ZenithFilter::processSample(float input) {
    float cutoff = cutoffSmoothed_.getNextValue();
    float resonance = resonanceSmoothed_.getNextValue();
    
    // Apply drive
    input *= drive_;
    input = std::tanh(input);
    
    // State variable filter
    float g = std::tan(juce::MathConstants<float>::pi * cutoff / static_cast<float>(sampleRate_));
    float k = 2.0f - 2.0f * resonance;
    
    float a1 = 1.0f / (1.0f + g * (g + k));
    float a2 = g * a1;
    float a3 = g * a2;
    
    v0_ = input;
    v1_ = a1 * ic1eq_ + a2 * (v0_ - ic2eq_);
    v2_ = ic2eq_ + a2 * ic1eq_ + a3 * (v0_ - ic2eq_);
    
    ic1eq_ = 2.0f * v1_ - ic1eq_;
    ic2eq_ = 2.0f * v2_ - ic2eq_;
    
    switch (type_) {
        case FilterType::Lowpass:
            return v2_;
        case FilterType::Bandpass:
            return v1_;
        case FilterType::Highpass:
            return v0_ - k * v1_ - v2_;
        default:
            return v2_;
    }
}

//==============================================================================
// ZenithEffects Implementation
//==============================================================================

void ZenithEffects::process(float &left, float &right) {
    // Distortion
    if (distortionAmount_ > 0.0f) {
        float drive = 1.0f + distortionAmount_ * 9.0f;
        left = std::tanh(left * drive) / drive;
        right = std::tanh(right * drive) / drive;
    }
    
    // Chorus
    if (chorusAmount_ > 0.0f) {
        float lfoValue = std::sin(chorusPhase_ * juce::MathConstants<float>::twoPi);
        int delayTime = static_cast<int>(5.0f + lfoValue * 3.0f);
        
        int readPos = (delayPos_ - delayTime + delayBufferL_.size()) % delayBufferL_.size();
        float chorusL = delayBufferL_[readPos];
        float chorusR = delayBufferR_[readPos];
        
        left = left * (1.0f - chorusAmount_) + chorusL * chorusAmount_;
        right = right * (1.0f - chorusAmount_) + chorusR * chorusAmount_;
        
        delayBufferL_[delayPos_] = left;
        delayBufferR_[delayPos_] = right;
        delayPos_ = (delayPos_ + 1) % delayBufferL_.size();
        
        chorusPhase_ += 2.0f / static_cast<float>(sampleRate_);
        if (chorusPhase_ >= 1.0f) chorusPhase_ -= 1.0f;
    }
}

//==============================================================================
// ZenithPolySynthVoice Implementation
//==============================================================================

ZenithPolySynthVoice::ZenithPolySynthVoice() {
    ampEnvelope_.setSampleRate(44100.0);
    modEnvelope_.setSampleRate(44100.0);
}

bool ZenithPolySynthVoice::canPlaySound(juce::SynthesiserSound *sound) {
    return dynamic_cast<ZenithPolySynthSound *>(sound) != nullptr;
}

void ZenithPolySynthVoice::startNote(int midiNoteNumber, float velocity,
                                     juce::SynthesiserSound *sound,
                                     int currentPitchWheelPosition) {
    juce::ignoreUnused(sound, currentPitchWheelPosition);
    currentFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    targetFrequency_ = currentFrequency_;
    velocity_ = velocity;
    
    ampEnvelope_.noteOn();
    modEnvelope_.noteOn();
    
    osc1_.randomizePhase();
    osc2_.randomizePhase();
    osc3_.randomizePhase();
}

void ZenithPolySynthVoice::stopNote(float velocity, bool allowTailOff) {
    juce::ignoreUnused(velocity);
    ampEnvelope_.noteOff();
    modEnvelope_.noteOff();
    
    if (!allowTailOff) {
        clearCurrentNote();
    }
}

void ZenithPolySynthVoice::pitchWheelMoved(int newPitchWheelValue) {
    pitchBend_ = (newPitchWheelValue - 8192) / 8192.0f;
}

void ZenithPolySynthVoice::controllerMoved(int controllerNumber, int newControllerValue) {
    if (controllerNumber == 1) { // Mod wheel
        modWheel_ = newControllerValue / 127.0f;
    }
}

void ZenithPolySynthVoice::channelPressureChanged(int newChannelPressureValue) {
    aftertouch_ = newChannelPressureValue / 127.0f;
}

void ZenithPolySynthVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer, 
                                           int startSample, int numSamples) {
    if (!isVoiceActive()) return;
    
    for (int i = 0; i < numSamples; ++i) {
        // Update frequency with glide
        if (glideTime_ > 0.0f) {
            float glideRate = 1.0f / (glideTime_ * getSampleRate());
            currentFrequency_ += (targetFrequency_ - currentFrequency_) * glideRate;
        } else {
            currentFrequency_ = targetFrequency_;
        }
        
        // Apply pitch bend
        float pitchMod = static_cast<float>(std::pow(2.0, pitchBend_ / 12.0));
        float freq = currentFrequency_ * pitchMod;
        
        // Generate oscillator samples
        float sample = 0.0f;
        sample += osc1_.getNextSample(freq, oscShape_) * osc1Mix_;
        sample += osc2_.getNextSample(freq, oscShape_) * osc2Mix_;
        sample += osc3_.getNextSample(freq, oscShape_) * osc3Mix_;
        
        // Apply filter
        sample = filter1_.processSample(sample);
        
        // Apply amplitude envelope
        float ampEnv = ampEnvelope_.getNextSample();
        sample *= ampEnv * velocity_;
        
        // Add to output buffer
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
            outputBuffer.addSample(channel, startSample + i, sample);
        }
        
        currentAmplitude_ = std::abs(sample);
        
        // Check if voice should stop
        if (!ampEnvelope_.isActive()) {
            clearCurrentNote();
            break;
        }
    }
}

void ZenithPolySynthVoice::setSampleRate(double sampleRate) {
    osc1_.setSampleRate(sampleRate);
    osc2_.setSampleRate(sampleRate);
    osc3_.setSampleRate(sampleRate);
    filter1_.setSampleRate(sampleRate);
    filter2_.setSampleRate(sampleRate);
    effects_.setSampleRate(sampleRate);
    ampEnvelope_.setSampleRate(sampleRate);
    modEnvelope_.setSampleRate(sampleRate);
}

void ZenithPolySynthVoice::setAmpEnvelope(float attack, float decay, float sustain, float release) {
    juce::ADSR::Parameters params;
    params.attack = attack;
    params.decay = decay;
    params.sustain = sustain;
    params.release = release;
    ampEnvelope_.setParameters(params);
    ampEnvParams_ = params;
}

void ZenithPolySynthVoice::setModEnvelope(float attack, float decay, float sustain, float release) {
    juce::ADSR::Parameters params;
    params.attack = attack;
    params.decay = decay;
    params.sustain = sustain;
    params.release = release;
    modEnvelope_.setParameters(params);
    modEnvParams_ = params;
}

void ZenithPolySynthVoice::setLFO1(float rate, float amount, LFOTarget target) {
    lfo1Rate_ = rate;
    lfo1Amount_ = amount;
    lfo1Target_ = target;
}

void ZenithPolySynthVoice::setLFO2(float rate, float amount, LFOTarget target) {
    lfo2Rate_ = rate;
    lfo2Amount_ = amount;
    lfo2Target_ = target;
}

void ZenithPolySynthVoice::setModulationSlot(int slotIndex, ModulationSource source,
                                             ModulationDestination destination, float amount) {
    if (slotIndex >= 0 && slotIndex < static_cast<int>(modulationMatrix_.size())) {
        modulationMatrix_[slotIndex].source = source;
        modulationMatrix_[slotIndex].destination = destination;
        modulationMatrix_[slotIndex].amount = amount;
    }
}

void ZenithPolySynthVoice::updateFrequency() {
    // This is called internally during renderNextBlock
}

void ZenithPolySynthVoice::computeModulation() {
    // Reset modulation state
    modulationState_.reset();
    
    // Compute LFO values
    lfo1Value_ = std::sin(lfo1Phase_ * juce::MathConstants<double>::twoPi);
    lfo2Value_ = std::sin(lfo2Phase_ * juce::MathConstants<double>::twoPi);
    
    lfo1Phase_ += lfo1Rate_ / getSampleRate();
    lfo2Phase_ += lfo2Rate_ / getSampleRate();
    
    if (lfo1Phase_ >= 1.0) lfo1Phase_ -= 1.0;
    if (lfo2Phase_ >= 1.0) lfo2Phase_ -= 1.0;
}

float ZenithPolySynthVoice::getModulationSourceValue(ModulationSource source) {
    switch (source) {
        case ModulationSource::LFO1:
            return lfo1Value_;
        case ModulationSource::LFO2:
            return lfo2Value_;
        case ModulationSource::Env1:
            return ampEnvelope_.getNextSample();
        case ModulationSource::Env2:
            return modEnvelope_.getNextSample();
        case ModulationSource::Velocity:
            return velocity_;
        case ModulationSource::ModWheel:
            return modWheel_;
        case ModulationSource::Aftertouch:
            return aftertouch_;
        default:
            return 0.0f;
    }
}

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
}

ZenithPolySynthProcessor::~ZenithPolySynthProcessor() {}

void ZenithPolySynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    synthesiser_.setCurrentPlaybackSampleRate(sampleRate);
    
    for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
        if (auto *voice = dynamic_cast<ZenithPolySynthVoice *>(synthesiser_.getVoice(i))) {
            voice->setSampleRate(sampleRate);
        }
    }
}

void ZenithPolySynthProcessor::releaseResources() {
    synthesiser_.allNotesOff(0, false);
}

void ZenithPolySynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear();
    synthesiser_.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    // Update voice parameters if needed
    updateVoiceParameters();
    
    // Push to visualizer (use left channel for now)
    if (buffer.getNumChannels() > 0) {
        pushToVisualizer(buffer.getReadPointer(0), buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* ZenithPolySynthProcessor::createEditor() { 
    return new ZenithPolySynthUI(*this); 
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
    
    return layout;
}

void ZenithPolySynthProcessor::updateVoiceParameters() {
    // Update all voices with current parameter values
    for (int i = 0; i < synthesiser_.getNumVoices(); ++i) {
        if (auto *voice = dynamic_cast<ZenithPolySynthVoice *>(synthesiser_.getVoice(i))) {
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
            // Note: LFOTarget is no longer used directly in setLFO1/2 in the header signature, 
            // but the implementation still takes it. We should cast safely.
            // However, looking at the header, setLFO1 takes (rate, amount, LFOTarget).
            // The parameter value is float, so we cast to int then LFOTarget.
            
            // Actually, the parameters LFO1Target and LFO2Target exist in the layout but were not retrieved in the original code snippet.
            // The original code passed LFOTarget::FilterCutoff hardcoded or via a parameter that wasn't shown being retrieved.
            // Let's retrieve the target parameter if it exists, or default to FilterCutoff.
            
            // Checking createParameterLayout, LFO1Target is NOT added there! 
            // Wait, LFO1Target string constant exists, but it's not added to the layout in createParameterLayout().
            // So we can't get it from parameters_.
            
            // For now, let's just use the hardcoded FilterCutoff as it was in the original code, 
            // but ensure we are passing the correct enum type.
            
            voice->setLFO1(*parameters_.getRawParameterValue(LFO1Rate), 
                          *parameters_.getRawParameterValue(LFO1Amount),
                          LFOTarget::FilterCutoff);
                          
            voice->setLFO2(*parameters_.getRawParameterValue(LFO2Rate),
                          *parameters_.getRawParameterValue(LFO2Amount),
                          LFOTarget::FilterCutoff);
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
            // If amount is 0, we could clear the slot, but for now keep it
            if (std::abs(amount) < 0.001f) {
                slot.source = ModulationSource::None;
                slot.destination = ModulationDestination::None;
                slot.amount = 0.0f;
            }
            return;
        }
    }
    
    // 2. Add new if amount is significant
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

int ZenithPolySynthProcessor::readFromVisualizer(float* buffer, int numSamples) {
    int numReady = visualizerFifo_.getNumReady();
    int numToRead = std::min(numReady, numSamples);
    
    if (numToRead > 0) {
        int start1, size1, start2, size2;
        visualizerFifo_.prepareToRead(numToRead, start1, size1, start2, size2);
        
        if (size1 > 0) std::memcpy(buffer, visualizerBuffer_.data() + start1, size1 * sizeof(float));
        if (size2 > 0) std::memcpy(buffer + size1, visualizerBuffer_.data() + start2, size2 * sizeof(float));
        
        visualizerFifo_.finishedRead(numToRead);
    }
    
    return numToRead;
}

void ZenithPolySynthProcessor::pushToVisualizer(const float* buffer, int numSamples) {
    int numFree = visualizerFifo_.getFreeSpace();
    int numToWrite = std::min(numFree, numSamples);
    
    if (numToWrite > 0) {
        int start1, size1, start2, size2;
        visualizerFifo_.prepareToWrite(numToWrite, start1, size1, start2, size2);
        
        if (size1 > 0) std::memcpy(visualizerBuffer_.data() + start1, buffer, size1 * sizeof(float));
        if (size2 > 0) std::memcpy(visualizerBuffer_.data() + start2, buffer + size1, size2 * sizeof(float));
        
        visualizerFifo_.finishedWrite(numToWrite);
    }
}

// Static parameter IDs
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
const juce::String ZenithPolySynthProcessor::QualitySetting = "quality";
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

//==============================================================================
// ZenithPolySynth Implementation
//==============================================================================

ZenithPolySynth::ZenithPolySynth() 
    : InstrumentBase(std::make_unique<ZenithPolySynthProcessor>(), createMetadata()) {
    registerPresets();
}

InstrumentMetadata ZenithPolySynth::createMetadata() {
    InstrumentMetadata metadata;
    metadata.instrumentId = "zenith_poly_synth";
    metadata.name = "Zenith Poly Synth";
    metadata.description = "Multi-oscillator subtractive synthesizer";
    metadata.category = "Synthesizer";
    metadata.tags = {"synth", "poly", "subtractive", "analog"};
    return metadata;
}

void ZenithPolySynth::registerPresets() {
    // Register factory presets
    // Implementation pending preset system finalization
}

} // namespace zenith
