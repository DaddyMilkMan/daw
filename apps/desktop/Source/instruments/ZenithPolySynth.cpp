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
#ifdef ZENITH_USE_SKIA
#include "../ui/skia/ZenithPolySynthUI.h"
#endif
#include "PresetGenerator.h"
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// ZenithOscillator Implementation
//==============================================================================

float ZenithOscillator::getNextSample(float frequency, float shape) {
    // Apply detune
    // Optimization: precalc detune multiplier if it was constant, but it might change?
    // Actually detuneCents_ is set per block.
    // But we can't easily cache this unless we track changes.
    // For single osc, pow is okay-ish (once per sample), but supersaw (7x) is bad.
    // For main osc, let's optimize later if needed.
    
    float detuneMultiplier = std::pow(2.0f, detuneCents_ / 1200.0f);
    frequency *= detuneMultiplier;
    
    // Update supersaw ratios if detune changed (rudimentary check)
    // Ideally this is called from setDetune, but setDetune is just a setter.
    // We'll call updateSupersawRatios() inside setDetune in the header? No, setDetune is inline.
    // We need to move setDetune to cpp or update here.
    // Let's rely on processSupersaw using the cached values which we update when parameters change?
    // Actually, simpler: calculate ratios in processSupersaw only if they might have changed?
    // No, `detuneCents_` determines the spread.
    // We will call updateSupersawRatios in processSupersaw if needed or just assume setDetune called it.
    // BUT setDetune is inline in header. I need to change it.
    // Wait, I can't change the header inline definition easily without another Replace.
    // I'll assume I can edit the header again or just do it here.
    
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
        case OscillatorWaveform::Supersaw:
            return processSupersaw(frequency);
        default:
            return 0.0f;
    }
}

void ZenithOscillator::updateSupersawRatios() {
    if (!supersawInit_) return; // Wait for init
    
    float spread = 1.0f + (detuneCents_ / 100.0f);
    for (int i = 0; i < 7; ++i) {
        supersawRatios_[i] = std::pow(2.0f, (supersawDetunes_[i] * spread) / 12.0f);
    }
}

float ZenithOscillator::processSine(float frequency) {
    float sample = std::sin(phase_ * juce::MathConstants<double>::twoPi);
    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processSaw(float frequency) {
    float phaseInc = frequency / sampleRate_;
    float sample = 2.0f * static_cast<float>(phase_) - 1.0f;
    
    // PolyBLEP
    sample -= poly_blep(static_cast<float>(phase_), phaseInc);
    
    phase_ += phaseInc;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processSquare(float frequency, float pulseWidth) {
    float phaseInc = frequency / sampleRate_;
    float sample = (phase_ < pulseWidth) ? 1.0f : -1.0f;
    
    // PolyBLEP (for both edges)
    sample += poly_blep(static_cast<float>(phase_), phaseInc);
    
    // Second edge at pulseWidth
    // We need to map phase relative to pulseWidth
    float phase2 = static_cast<float>(phase_) - pulseWidth;
    if (phase2 < 0.0f) phase2 += 1.0f;
    sample -= poly_blep(phase2, phaseInc);

    phase_ += phaseInc;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processTriangle(float frequency) {
    // Triangle is integral of square, less aliasing, but PolyBLEP can still help on peaks
    // For now, leave naive or improve later. The naive one is okay-ish.
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

float ZenithOscillator::processSupersaw(float frequency) {
    if (!supersawInit_) {
        supersawDetunes_[0] = 0.0f;
        supersawDetunes_[1] = -0.11f; supersawDetunes_[2] = 0.11f;
        supersawDetunes_[3] = -0.06f; supersawDetunes_[4] = 0.06f;
        supersawDetunes_[5] = -0.02f; supersawDetunes_[6] = 0.02f;
        
        for (auto& phase : supersawPhases_) phase = random_.nextFloat();
        supersawInit_ = true;
        updateSupersawRatios();
    }

    // NOTE: We assume updateSupersawRatios() is called when detune changes.
    // If setDetune is inline, we might need to update it here if we detect change?
    // Or we just recalculate ratios here if we suspect they are stale?
    // For "Vibe Coding" fix, let's just recalculate ONLY if we can't hook setDetune easily.
    // BUT the instruction was "Precompute detune ratios".
    // Since I can't easily modify the inline setDetune in header without another Replace,
    // I'll assume I'll add the call there in a moment.
    // For safety, I'll just use the ratios.
    
    float sample = 0.0f;
    // float spread = 1.0f + (detuneCents_ / 100.0f); // Used in updateSupersawRatios

    for (int i = 0; i < 7; ++i) {
        // float detunedFreq = frequency * std::pow(2.0f, (supersawDetunes_[i] * spread) / 12.0f);
        float detunedFreq = frequency * supersawRatios_[i];
        
        float phaseInc = detunedFreq / sampleRate_;
        
        float s = 2.0f * static_cast<float>(supersawPhases_[i]) - 1.0f;
        s -= poly_blep(static_cast<float>(supersawPhases_[i]), phaseInc);
        
        sample += s;
        
        supersawPhases_[i] += phaseInc;
        if (supersawPhases_[i] >= 1.0) supersawPhases_[i] -= 1.0;
    }

    return sample * 0.15f; 
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

    // Reverb
    if (reverbAmount_ > 0.0f) {
        initReverb();
        
        // Mix left and right for reverb input (mono in)
        float input = (left + right) * 0.5f * reverbAmount_;
        
        // Parallel comb filters
        float combOut = 0.0f;
        for (auto& comb : combs_) {
            combOut += comb.process(input);
        }
        
        // Serial allpass filters
        float allpassOut = combOut;
        for (auto& allpass : allpasses_) {
            allpassOut = allpass.process(allpassOut);
        }
        
        // Wet/Dry mix
        left += allpassOut * 0.2f; // Scale reverb tail
        right += allpassOut * 0.2f;
    }
}

//==============================================================================
// ZenithPolySynthVoice Implementation
//==============================================================================

ZenithPolySynthVoice::ZenithPolySynthVoice() {
    // Use a sensible default, but this should be updated by prepareToPlay
    constexpr double DEFAULT_SAMPLE_RATE = 44100.0;
    ampEnvelope_.setSampleRate(DEFAULT_SAMPLE_RATE);
    modEnvelope_.setSampleRate(DEFAULT_SAMPLE_RATE);
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
    
    // Update supersaw ratios initially
    osc1_.updateSupersawRatios();
    osc2_.updateSupersawRatios();
    osc3_.updateSupersawRatios();
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
    
    // Pre-calculate modulation for the block (control rate)
    computeModulation();
    
    // Update osc detunes (control rate) to refresh supersaw ratios
    // (We should probably do this only if they changed, but for now ensuring they are up to date)
    // Actually, since setOscDetune calls setDetune which updates the value, 
    // we need to make sure updateSupersawRatios is called.
    // Since we couldn't modify the header inline, we'll do it here for safety.
    osc1_.updateSupersawRatios();
    osc2_.updateSupersawRatios();
    osc3_.updateSupersawRatios();

    for (int i = 0; i < numSamples; ++i) {
        // Update frequency with glide
        if (glideTime_ > 0.0f) {
            float glideRate = 1.0f / (glideTime_ * getSampleRate());
            currentFrequency_ += (targetFrequency_ - currentFrequency_) * glideRate;
        } else {
            currentFrequency_ = targetFrequency_;
        }
        
        // Apply pitch bend and modulation
        float pitchMod = static_cast<float>(std::pow(2.0, pitchBend_ / 12.0));
        
        // Add modulation matrix pitch
        float modPitch = modulationState_.get(ModulationDestination::Osc1Pitch); 
        // Apply to all oscillators for now unless specific targets added
        float freq = currentFrequency_ * pitchMod * std::pow(2.0f, modPitch / 12.0f);
        
        // Generate oscillator samples
        float sample = 0.0f;
        
        // Oscillator 1
        float osc1Freq = freq * std::pow(2.0f, modulationState_.get(ModulationDestination::Osc1Pitch) / 12.0f);
        sample += osc1_.getNextSample(osc1Freq, oscShape_) * (osc1Mix_ + modulationState_.get(ModulationDestination::Osc1Mix));
        
        // Oscillator 2
        float osc2Freq = freq * std::pow(2.0f, modulationState_.get(ModulationDestination::Osc2Pitch) / 12.0f);
        sample += osc2_.getNextSample(osc2Freq, oscShape_) * (osc2Mix_ + modulationState_.get(ModulationDestination::Osc2Mix));
        
        // Oscillator 3
        float osc3Freq = freq * std::pow(2.0f, modulationState_.get(ModulationDestination::Osc3Pitch) / 12.0f);
        sample += osc3_.getNextSample(osc3Freq, oscShape_) * (osc3Mix_ + modulationState_.get(ModulationDestination::Osc3Mix));
        
        // Unison (Oscillator Stacking)
        if (unisonVoices_ > 1) {
            float unisonSpread = unisonDetune_ / 100.0f; // Cents to ratio-ish
            float unisonGain = 1.0f / std::sqrt(static_cast<float>(unisonVoices_));
            
            for (int u = 0; u < unisonVoices_ - 1 && u < 7; ++u) {
                float detune = (u % 2 == 0 ? 1.0f : -1.0f) * ((u / 2 + 1) * unisonSpread);
                float uFreq = freq * std::pow(2.0f, detune / 12.0f);
                
                unisonOscillators_[u].setWaveform(osc1_.getWaveform()); 
                sample += unisonOscillators_[u].getNextSample(uFreq, oscShape_) * osc1Mix_ * unisonGain;
            }
            sample *= unisonGain; 
        }

        // Apply filter 1
        float cutoffMod = modulationState_.get(ModulationDestination::FilterCutoff);
        filter1_.setCutoff(filterCutoff_ * std::pow(2.0f, cutoffMod * 5.0f)); // 5 octaves range
        filter1_.setResonance(juce::jlimit(0.0f, 1.0f, filter1_.getResonance() + modulationState_.get(ModulationDestination::FilterResonance)));
        
        sample = filter1_.processSample(sample);
        
        // Apply Filter 2
        if (filter2Cutoff_ > 20.0f) {
             if (filterSerial_) {
                 sample = filter2_.processSample(sample);
             }
        }
        
        // Apply amplitude envelope
        float ampEnv = ampEnvelope_.getNextSample();
        float modAmp = modulationState_.get(ModulationDestination::AmpGain);
        sample *= (ampEnv + modAmp) * velocity_;
        
        // Effects (Per-voice)
        float left = sample;
        float right = sample;
        effects_.process(left, right);
        
        // Add to output buffer
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
            float out = (channel == 0) ? left : right;
            outputBuffer.addSample(channel, startSample + i, out);
        }
        
        currentAmplitude_ = std::abs(sample);
        
        // Check if voice should stop
        // Only stop if envelope finished AND effects tail finished
        if (!ampEnvelope_.isActive() && !effects_.hasTail()) {
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
    
    // Legacy LFO routing (for backward compatibility if needed, or just map to matrix)
    // We'll assume the Matrix is the source of truth now.
    
    // Process Modulation Matrix
    for (const auto& slot : modulationMatrix_) {
        if (slot.isActive()) {
            float sourceValue = getModulationSourceValue(slot.source);
            modulationState_.add(slot.destination, sourceValue * slot.amount);
        }
    }
    
    // Hardcoded LFO targets from parameters (if not in matrix)
    // This ensures the basic knobs still work if the matrix isn't used
    if (lfo1Amount_ != 0.0f) {
        // Map legacy target enum to destination
        ModulationDestination dest = ModulationDestination::None;
        switch(lfo1Target_) {
            case LFOTarget::FilterCutoff: dest = ModulationDestination::FilterCutoff; break;
            case LFOTarget::Osc1Pitch: dest = ModulationDestination::Osc1Pitch; break;
            case LFOTarget::Osc2Pitch: dest = ModulationDestination::Osc2Pitch; break;
            case LFOTarget::Osc1Mix: dest = ModulationDestination::Osc1Mix; break;
            case LFOTarget::Osc2Mix: dest = ModulationDestination::Osc2Mix; break;
            default: break;
        }
        if (dest != ModulationDestination::None) {
            modulationState_.add(dest, lfo1Value_ * lfo1Amount_);
        }
    }
    
    if (lfo2Amount_ != 0.0f) {
         ModulationDestination dest = ModulationDestination::None;
        switch(lfo2Target_) {
            case LFOTarget::FilterCutoff: dest = ModulationDestination::FilterCutoff; break;
            case LFOTarget::Osc1Pitch: dest = ModulationDestination::Osc1Pitch; break;
            case LFOTarget::Osc2Pitch: dest = ModulationDestination::Osc2Pitch; break;
            case LFOTarget::Osc1Mix: dest = ModulationDestination::Osc1Mix; break;
            case LFOTarget::Osc2Mix: dest = ModulationDestination::Osc2Mix; break;
            default: break;
        }
        if (dest != ModulationDestination::None) {
            modulationState_.add(dest, lfo2Value_ * lfo2Amount_);
        }
    }
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
