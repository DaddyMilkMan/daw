/*
  ==============================================================================

    ZenithPolySynth.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of ZenithPolySynth - multi-oscillator subtractive synthesizer.

  ==============================================================================
*/

#include "ZenithPolySynth.h"
#include <cmath>

namespace zenith {

//==============================================================================
// ZenithOscillator Implementation
//==============================================================================

float ZenithOscillator::getNextSample(float frequency)
{
    // Apply detune
    float detuneMultiplier = std::pow(2.0f, detuneCents_ / 1200.0f);
    float detunedFrequency = frequency * detuneMultiplier;

    switch (waveform_)
    {
        case OscillatorWaveform::Sine:     return processSine(detunedFrequency);
        case OscillatorWaveform::Saw:      return processSaw(detunedFrequency);
        case OscillatorWaveform::Square:   return processSquare(detunedFrequency);
        case OscillatorWaveform::Triangle: return processTriangle(detunedFrequency);
        case OscillatorWaveform::Noise:    return processNoise();
        case OscillatorWaveform::Supersaw: return processSaw(detunedFrequency); // Same as saw for individual osc
        default:                           return 0.0f;
    }
}

float ZenithOscillator::processSine(float frequency)
{
    float sample = std::sin(phase_ * juce::MathConstants<double>::twoPi);
    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0)
        phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processSaw(float frequency)
{
    // PolyBLEP antialiasing for saw wave
    float naiveSaw = static_cast<float>(2.0 * phase_ - 1.0);

    // Simple PolyBLEP implementation
    float t = phase_;
    float dt = frequency / sampleRate_;

    // Correct discontinuity at phase wrap
    if (t < dt)
    {
        t = t / dt;
        naiveSaw -= (t + t - t * t - 1.0f);
    }
    else if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        naiveSaw -= (t + t + t * t + 1.0f);
    }

    phase_ += dt;
    if (phase_ >= 1.0)
        phase_ -= 1.0;

    return naiveSaw;
}

float ZenithOscillator::processSquare(float frequency)
{
    // PolyBLEP antialiasing for square wave
    float naiveSquare = (phase_ < 0.5) ? 1.0f : -1.0f;

    float t = phase_;
    float dt = frequency / sampleRate_;

    // Correct discontinuity at rising edge (phase = 0)
    if (t < dt)
    {
        t = t / dt;
        naiveSquare += (t + t - t * t - 1.0f);
    }
    else if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        naiveSquare += (t + t + t * t + 1.0f);
    }

    // Correct discontinuity at falling edge (phase = 0.5)
    t = phase_ - 0.5;
    if (t > 0.0 && t < dt)
    {
        t = t / dt;
        naiveSquare -= (t + t - t * t - 1.0f);
    }
    else if (t > -dt && t < 0.0)
    {
        t = (t + dt) / dt;
        naiveSquare -= (t + t + t * t + 1.0f);
    }

    phase_ += dt;
    if (phase_ >= 1.0)
        phase_ -= 1.0;

    return naiveSquare;
}

float ZenithOscillator::processTriangle(float frequency)
{
    // Triangle wave from phase
    float triangle;
    if (phase_ < 0.25)
        triangle = 4.0f * static_cast<float>(phase_);
    else if (phase_ < 0.75)
        triangle = 2.0f - 4.0f * static_cast<float>(phase_);
    else
        triangle = -4.0f + 4.0f * static_cast<float>(phase_);

    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0)
        phase_ -= 1.0;

    return triangle;
}

float ZenithOscillator::processNoise()
{
    return random_.nextFloat() * 2.0f - 1.0f;
}

//==============================================================================
// ZenithFilter Implementation
//==============================================================================

void ZenithFilter::setSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
    cutoffSmoothed_.reset(sampleRate, 0.02); // 20ms smoothing
    resonanceSmoothed_.reset(sampleRate, 0.02);
}

void ZenithFilter::setCutoff(float cutoffHz)
{
    cutoffSmoothed_.setTargetValue(juce::jlimit(20.0f, 20000.0f, cutoffHz));
}

void ZenithFilter::setResonance(float resonance)
{
    resonanceSmoothed_.setTargetValue(juce::jlimit(0.0f, 1.0f, resonance));
}

void ZenithFilter::reset()
{
    v0_ = v1_ = v2_ = 0.0f;
    ic1eq_ = ic2eq_ = 0.0f;
    cutoffSmoothed_.setCurrentAndTargetValue(1000.0f);
    resonanceSmoothed_.setCurrentAndTargetValue(0.5f);
}

float ZenithFilter::processSample(float input)
{
    // Get smoothed parameter values
    float cutoff = cutoffSmoothed_.getNextValue();
    float resonance = resonanceSmoothed_.getNextValue();

    // Apply drive/saturation
    input *= drive_;
    input = std::tanh(input);

    // State variable filter (Chamberlin/Hal formulation)
    // Reference: Zavalishin's "The Art of VA Filter Design"

    float fc = cutoff / static_cast<float>(sampleRate_);
    fc = juce::jlimit(0.0001f, 0.45f, fc); // Prevent instability

    float g = std::tan(juce::MathConstants<float>::pi * fc);
    float k = 2.0f - 2.0f * resonance; // Resonance (damping)

    float a1 = 1.0f / (1.0f + g * (g + k));
    float a2 = g * a1;
    float a3 = g * a2;

    v0_ = input;
    v1_ = a1 * ic1eq_ + a2 * (v0_ - ic2eq_);
    v2_ = ic2eq_ + a2 * ic1eq_ + a3 * (v0_ - ic2eq_);

    ic1eq_ = 2.0f * v1_ - ic1eq_;
    ic2eq_ = 2.0f * v2_ - ic2eq_;

    // Select output based on filter type
    switch (type_)
    {
        case FilterType::Lowpass:  return v2_;
        case FilterType::Bandpass: return v1_;
        case FilterType::Highpass: return v0_ - k * v1_ - v2_;
        default:                   return v2_;
    }
}

//==============================================================================
// ZenithPolySynthVoice Implementation
//==============================================================================

ZenithPolySynthVoice::ZenithPolySynthVoice()
{
    // Initialize envelopes with default parameters
    ampEnvParams_.attack = 0.01f;
    ampEnvParams_.decay = 0.1f;
    ampEnvParams_.sustain = 0.8f;
    ampEnvParams_.release = 0.3f;
    ampEnvelope_.setParameters(ampEnvParams_);

    modEnvParams_.attack = 0.01f;
    modEnvParams_.decay = 0.5f;
    modEnvParams_.sustain = 0.0f;
    modEnvParams_.release = 0.1f;
    modEnvelope_.setParameters(modEnvParams_);
}

bool ZenithPolySynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<ZenithPolySynthSound*>(sound) != nullptr;
}

void ZenithPolySynthVoice::startNote(int midiNoteNumber, float velocity,
                                      juce::SynthesiserSound*, int)
{
    currentMidiNote_ = midiNoteNumber;
    velocity_ = velocity;

    targetFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    // Initialize frequency (with or without glide)
    if (glideTime_ <= 0.0f || !monoMode_)
    {
        currentFrequency_ = targetFrequency_;
    }

    // Reset oscillators
    osc1_.reset();
    osc2_.reset();
    osc3_.reset();
    for (auto& osc : unisonOscillators_)
        osc.reset();

    // Reset filter
    filter_.reset();

    // Reset LFOs
    lfo1_.reset();
    lfo2_.reset();

    // Trigger envelopes
    ampEnvelope_.noteOn();
    modEnvelope_.noteOn();
}

void ZenithPolySynthVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnvelope_.noteOff();
        modEnvelope_.noteOff();
    }
    else
    {
        clearCurrentNote();
        ampEnvelope_.reset();
        modEnvelope_.reset();
    }
}

void ZenithPolySynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // Convert MIDI pitch wheel to semitones (+/- 2 semitones)
    float pitchBend = (newPitchWheelValue - 8192) / 8192.0f * 2.0f;
    targetFrequency_ = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote_ + pitchBend);
}

void ZenithPolySynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                            int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    // Compute modulation matrix once per buffer (RT-safe)
    computeModulation();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Update glide/portamento
        updateFrequency();

        // Apply pitch modulation from modulation matrix
        float pitchMod1 = modulationState_.get(ModulationDestination::Osc1Pitch) * 12.0f; // +/- 12 semitones
        float pitchMod2 = modulationState_.get(ModulationDestination::Osc2Pitch) * 12.0f;
        float pitchMod3 = modulationState_.get(ModulationDestination::Osc3Pitch) * 12.0f;

        float osc1Freq = currentFrequency_ * std::pow(2.0f, pitchMod1 / 12.0f);
        float osc2Freq = currentFrequency_ * std::pow(2.0f, pitchMod2 / 12.0f);
        float osc3Freq = currentFrequency_ * std::pow(2.0f, pitchMod3 / 12.0f);

        // Apply mix modulation from modulation matrix
        float osc1MixMod = juce::jlimit(0.0f, 1.0f, osc1Mix_ + modulationState_.get(ModulationDestination::Osc1Mix));
        float osc2MixMod = juce::jlimit(0.0f, 1.0f, osc2Mix_ + modulationState_.get(ModulationDestination::Osc2Mix));
        float osc3MixMod = juce::jlimit(0.0f, 1.0f, osc3Mix_ + modulationState_.get(ModulationDestination::Osc3Mix));

        // Generate oscillator output
        float oscOutput = 0.0f;

        // Oscillator 1
        if (osc1MixMod > 0.0f)
        {
            oscOutput += osc1_.getNextSample(osc1Freq) * osc1MixMod;
        }

        // Oscillator 2
        if (osc2MixMod > 0.0f)
        {
            oscOutput += osc2_.getNextSample(osc2Freq) * osc2MixMod;
        }

        // Oscillator 3
        if (osc3MixMod > 0.0f)
        {
            oscOutput += osc3_.getNextSample(osc3Freq) * osc3MixMod;
        }

        // Unison (supersaw) voices
        if (unisonVoices_ > 1)
        {
            float unisonOutput = 0.0f;
            for (int v = 0; v < unisonVoices_; ++v)
            {
                float detune = (v - unisonVoices_ / 2.0f) * unisonDetune_;
                unisonOscillators_[v].setDetune(detune);
                unisonOutput += unisonOscillators_[v].getNextSample(currentFrequency_);
            }
            oscOutput += unisonOutput / static_cast<float>(unisonVoices_) * 0.5f;
        }

        // Normalize oscillator mix
        float totalMix = osc1MixMod + osc2MixMod + osc3MixMod;
        if (totalMix > 0.0f)
            oscOutput /= totalMix;

        // Apply LFO modulations (legacy support)
        float lfo1Value = lfo1_.getNextValue(sampleRate_);
        float lfo2Value = lfo2_.getNextValue(sampleRate_);

        // Apply mod envelope to filter cutoff
        float modEnvValue = modEnvelope_.getNextSample();
        float filterCutoffMod = filterCutoff_ + (modEnvValue * 5000.0f); // Add up to 5kHz

        // Apply LFO to filter if targeted (legacy)
        if (lfo1_.target == LFOTarget::FilterCutoff)
            filterCutoffMod += lfo1Value * 2000.0f;
        if (lfo2_.target == LFOTarget::FilterCutoff)
            filterCutoffMod += lfo2Value * 2000.0f;

        // Apply filter cutoff modulation from modulation matrix
        filterCutoffMod += modulationState_.get(ModulationDestination::FilterCutoff) * 10000.0f; // +/- 10kHz

        filter_.setCutoff(filterCutoffMod);

        // Apply filter resonance modulation from modulation matrix
        float baseResonance = 0.3f; // Default resonance
        float resonanceMod = juce::jlimit(0.0f, 1.0f,
                                          baseResonance + modulationState_.get(ModulationDestination::FilterResonance));
        filter_.setResonance(resonanceMod);

        // Filter the oscillator output
        float filteredOutput = filter_.processSample(oscOutput);

        // Apply amplitude envelope and velocity
        float ampEnvValue = ampEnvelope_.getNextSample();

        // Apply volume modulation from modulation matrix
        float volumeMod = juce::jlimit(0.0f, 2.0f, 1.0f + modulationState_.get(ModulationDestination::Volume));

        float finalOutput = filteredOutput * ampEnvValue * velocity_ * volumeMod;

        // Apply pan modulation from modulation matrix
        float panValue = juce::jlimit(-1.0f, 1.0f, pan_ + modulationState_.get(ModulationDestination::Pan));

        // Mix into output buffer (stereo with pan)
        if (outputBuffer.getNumChannels() >= 2)
        {
            // Simple constant-power panning
            float leftGain = std::cos((panValue + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            float rightGain = std::sin((panValue + 1.0f) * juce::MathConstants<float>::pi * 0.25f);

            outputBuffer.addSample(0, startSample + sample, finalOutput * leftGain);
            outputBuffer.addSample(1, startSample + sample, finalOutput * rightGain);
        }
        else
        {
            // Mono output
            outputBuffer.addSample(0, startSample + sample, finalOutput);
        }

        // Check if voice should be stopped
        if (!ampEnvelope_.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

void ZenithPolySynthVoice::setAmpEnvelope(float attack, float decay, float sustain, float release)
{
    ampEnvParams_.attack = attack;
    ampEnvParams_.decay = decay;
    ampEnvParams_.sustain = sustain;
    ampEnvParams_.release = release;
    ampEnvelope_.setParameters(ampEnvParams_);
}

void ZenithPolySynthVoice::setModEnvelope(float attack, float decay, float sustain, float release)
{
    modEnvParams_.attack = attack;
    modEnvParams_.decay = decay;
    modEnvParams_.sustain = sustain;
    modEnvParams_.release = release;
    modEnvelope_.setParameters(modEnvParams_);
}

void ZenithPolySynthVoice::setLFO1(float rate, float amount, LFOTarget target)
{
    lfo1_.rate = rate;
    lfo1_.amount = amount;
    lfo1_.target = target;
}

void ZenithPolySynthVoice::setLFO2(float rate, float amount, LFOTarget target)
{
    lfo2_.rate = rate;
    lfo2_.amount = amount;
    lfo2_.target = target;
}

void ZenithPolySynthVoice::setSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
    osc1_.setSampleRate(sampleRate);
    osc2_.setSampleRate(sampleRate);
    osc3_.setSampleRate(sampleRate);
    for (auto& osc : unisonOscillators_)
        osc.setSampleRate(sampleRate);

    filter_.setSampleRate(sampleRate);
    ampEnvelope_.setSampleRate(sampleRate);
    modEnvelope_.setSampleRate(sampleRate);
}

void ZenithPolySynthVoice::updateFrequency()
{
    if (glideTime_ > 0.0f && currentFrequency_ != targetFrequency_)
    {
        // Exponential glide
        float glideRate = 1.0f - std::exp(-1.0f / (glideTime_ * static_cast<float>(sampleRate_)));
        currentFrequency_ += (targetFrequency_ - currentFrequency_) * glideRate;

        // Snap to target if very close
        if (std::abs(currentFrequency_ - targetFrequency_) < 0.01f)
            currentFrequency_ = targetFrequency_;
    }
    else
    {
        currentFrequency_ = targetFrequency_;
    }
}

float ZenithPolySynthVoice::applyLFOs()
{
    // TODO: Apply LFOs to various targets based on routing
    // This will be expanded when tempo sync is added via ProjectState
    return 0.0f;
}

//==============================================================================
// Modulation Matrix Implementation
//==============================================================================

void ZenithPolySynthVoice::setModulationSlot(int slotIndex, ModulationSource source,
                                              ModulationDestination destination, float amount)
{
    if (slotIndex >= 0 && slotIndex < kNumModSlots)
    {
        modulationSlots_[slotIndex].source = source;
        modulationSlots_[slotIndex].destination = destination;
        modulationSlots_[slotIndex].amount = juce::jlimit(-1.0f, 1.0f, amount);
    }
}

void ZenithPolySynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    // Convert MIDI 0-127 to 0-1
    float normalizedValue = newControllerValue / 127.0f;

    // Handle mod wheel (CC#1)
    if (controllerNumber == 1)
    {
        setModWheel(normalizedValue);
    }
}

void ZenithPolySynthVoice::channelPressureChanged(int newChannelPressureValue)
{
    // Convert MIDI 0-127 to 0-1
    setAftertouch(newChannelPressureValue / 127.0f);
}

float ZenithPolySynthVoice::getModulationSourceValue(ModulationSource source) const
{
    switch (source)
    {
        case ModulationSource::None:
            return 0.0f;

        case ModulationSource::LFO1:
            // LFO returns -1 to +1 (sine wave)
            return std::sin(lfo1_.phase * juce::MathConstants<float>::twoPi);

        case ModulationSource::LFO2:
            // LFO returns -1 to +1 (sine wave)
            return std::sin(lfo2_.phase * juce::MathConstants<float>::twoPi);

        case ModulationSource::Env1:
            // Amp envelope is 0 to 1
            return ampEnvelope_.isActive() ? ampEnvelope_.getNextSample() : 0.0f;

        case ModulationSource::Env2:
            // Mod envelope is 0 to 1
            return modEnvelope_.isActive() ? modEnvelope_.getNextSample() : 0.0f;

        case ModulationSource::Velocity:
            // Note velocity 0 to 1
            return velocity_;

        case ModulationSource::ModWheel:
            // MIDI mod wheel 0 to 1
            return modWheel_;

        case ModulationSource::Aftertouch:
            // MIDI aftertouch 0 to 1
            return aftertouch_;

        default:
            return 0.0f;
    }
}

void ZenithPolySynthVoice::computeModulation()
{
    // Reset modulation state
    modulationState_.reset();

    // Process each modulation slot
    for (const auto& slot : modulationSlots_)
    {
        if (!slot.isActive())
            continue;

        // Get source value
        float sourceValue = getModulationSourceValue(slot.source);

        // Apply amount and accumulate to destination
        float contribution = sourceValue * slot.amount;
        modulationState_.add(slot.destination, contribution);
    }
}

//==============================================================================
// ZenithPolySynthProcessor Implementation
//==============================================================================

ZenithPolySynthProcessor::ZenithPolySynthProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Add 16 voices for polyphony
    for (int i = 0; i < 16; ++i)
        addVoice(new ZenithPolySynthVoice());

    addSound(new ZenithPolySynthSound());

    // Create parameters - ORDER MUST MATCH enum Parameters!

    // Oscillator 1
    addParameter(new juce::AudioParameterChoice("osc1_wave", "Osc 1 Wave",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"}, 1)); // Default: Saw
    addParameter(new juce::AudioParameterFloat("osc1_detune", "Osc 1 Detune", -50.0f, 50.0f, 0.0f));
    addParameter(new juce::AudioParameterFloat("osc1_mix", "Osc 1 Mix", 0.0f, 1.0f, 1.0f));

    // Oscillator 2
    addParameter(new juce::AudioParameterChoice("osc2_wave", "Osc 2 Wave",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"}, 1)); // Default: Saw
    addParameter(new juce::AudioParameterFloat("osc2_detune", "Osc 2 Detune", -50.0f, 50.0f, 7.0f));
    addParameter(new juce::AudioParameterFloat("osc2_mix", "Osc 2 Mix", 0.0f, 1.0f, 0.5f));

    // Oscillator 3
    addParameter(new juce::AudioParameterChoice("osc3_wave", "Osc 3 Wave",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"}, 0)); // Default: Sine
    addParameter(new juce::AudioParameterFloat("osc3_detune", "Osc 3 Detune", -50.0f, 50.0f, -7.0f));
    addParameter(new juce::AudioParameterFloat("osc3_mix", "Osc 3 Mix", 0.0f, 1.0f, 0.0f));

    // Unison
    addParameter(new juce::AudioParameterInt("unison_voices", "Unison Voices", 1, 7, 1));
    addParameter(new juce::AudioParameterFloat("unison_detune", "Unison Detune", 0.0f, 50.0f, 10.0f));

    // Filter
    addParameter(new juce::AudioParameterChoice("filter_type", "Filter Type",
        juce::StringArray{"Lowpass", "Bandpass", "Highpass"}, 0)); // Default: Lowpass
    addParameter(new juce::AudioParameterFloat("filter_cutoff", "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));
    addParameter(new juce::AudioParameterFloat("filter_resonance", "Filter Resonance", 0.0f, 1.0f, 0.3f));
    addParameter(new juce::AudioParameterFloat("filter_drive", "Filter Drive", 1.0f, 5.0f, 1.0f));

    // Amp Envelope
    addParameter(new juce::AudioParameterFloat("amp_attack", "Amp Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
    addParameter(new juce::AudioParameterFloat("amp_decay", "Amp Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));
    addParameter(new juce::AudioParameterFloat("amp_sustain", "Amp Sustain", 0.0f, 1.0f, 0.8f));
    addParameter(new juce::AudioParameterFloat("amp_release", "Amp Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.3f));

    // Mod Envelope
    addParameter(new juce::AudioParameterFloat("mod_attack", "Mod Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
    addParameter(new juce::AudioParameterFloat("mod_decay", "Mod Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.5f));
    addParameter(new juce::AudioParameterFloat("mod_sustain", "Mod Sustain", 0.0f, 1.0f, 0.0f));
    addParameter(new juce::AudioParameterFloat("mod_release", "Mod Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.1f));

    // LFO 1
    addParameter(new juce::AudioParameterFloat("lfo1_rate", "LFO 1 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 5.0f));
    addParameter(new juce::AudioParameterFloat("lfo1_amount", "LFO 1 Amount", 0.0f, 1.0f, 0.0f));
    addParameter(new juce::AudioParameterChoice("lfo1_target", "LFO 1 Target",
        juce::StringArray{"Filter Cutoff", "Osc1 Pitch", "Osc2 Pitch", "Osc1 Mix", "Osc2 Mix"}, 0));

    // LFO 2
    addParameter(new juce::AudioParameterFloat("lfo2_rate", "LFO 2 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 2.0f));
    addParameter(new juce::AudioParameterFloat("lfo2_amount", "LFO 2 Amount", 0.0f, 1.0f, 0.0f));
    addParameter(new juce::AudioParameterChoice("lfo2_target", "LFO 2 Target",
        juce::StringArray{"Filter Cutoff", "Osc1 Pitch", "Osc2 Pitch", "Osc1 Mix", "Osc2 Mix"}, 0));

    // Global
    addParameter(new juce::AudioParameterFloat("glide_time", "Glide Time", 0.0f, 2.0f, 0.0f));
    addParameter(new juce::AudioParameterBool("mono_mode", "Mono Mode", false));
    addParameter(new juce::AudioParameterFloat("master_gain", "Master Gain",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 0.7f));
}

void ZenithPolySynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    setCurrentPlaybackSampleRate(sampleRate);

    // Initialize all voices
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ZenithPolySynthVoice*>(getVoice(i)))
        {
            voice->setSampleRate(sampleRate);
        }
    }

    // Initialize master gain smoothing
    masterGainSmoothed_.reset(sampleRate, 0.05); // 50ms smoothing
    masterGainSmoothed_.setCurrentAndTargetValue(0.7f);
}

void ZenithPolySynthProcessor::releaseResources()
{
    // No dynamic resources to release
}

void ZenithPolySynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    // Update voice parameters before rendering
    updateVoiceParameters();

    // Render synthesizer
    juce::Synthesiser::renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master gain
    float masterGain = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[MasterGain])->get();
    masterGainSmoothed_.setTargetValue(masterGain);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] *= masterGainSmoothed_.getNextValue();
        }
    }
}

void ZenithPolySynthProcessor::updateVoiceParameters()
{
    // Get parameter values
    auto osc1Wave = static_cast<OscillatorWaveform>(dynamic_cast<juce::AudioParameterChoice*>(getParameters()[Osc1Wave])->getIndex());
    auto osc1Detune = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[Osc1Detune])->get();
    auto osc1Mix = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[Osc1Mix])->get();

    auto osc2Wave = static_cast<OscillatorWaveform>(dynamic_cast<juce::AudioParameterChoice*>(getParameters()[Osc2Wave])->getIndex());
    auto osc2Detune = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[Osc2Detune])->get();
    auto osc2Mix = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[Osc2Mix])->get();

    auto osc3Wave = static_cast<OscillatorWaveform>(dynamic_cast<juce::AudioParameterChoice*>(getParameters()[Osc3Wave])->getIndex());
    auto osc3Detune = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[Osc3Detune])->get();
    auto osc3Mix = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[Osc3Mix])->get();

    auto unisonVoices = dynamic_cast<juce::AudioParameterInt*>(getParameters()[UnisonVoices])->get();
    auto unisonDetune = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[UnisonDetune])->get();

    auto filterType = static_cast<FilterType>(dynamic_cast<juce::AudioParameterChoice*>(getParameters()[FilterType])->getIndex());
    auto filterCutoff = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[FilterCutoff])->get();
    auto filterResonance = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[FilterResonance])->get();
    auto filterDrive = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[FilterDrive])->get();

    auto ampAttack = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[AmpAttack])->get();
    auto ampDecay = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[AmpDecay])->get();
    auto ampSustain = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[AmpSustain])->get();
    auto ampRelease = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[AmpRelease])->get();

    auto modAttack = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[ModAttack])->get();
    auto modDecay = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[ModDecay])->get();
    auto modSustain = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[ModSustain])->get();
    auto modRelease = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[ModRelease])->get();

    auto lfo1Rate = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[LFO1Rate])->get();
    auto lfo1Amount = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[LFO1Amount])->get();
    auto lfo1Target = static_cast<LFOTarget>(dynamic_cast<juce::AudioParameterChoice*>(getParameters()[LFO1Target])->getIndex());

    auto lfo2Rate = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[LFO2Rate])->get();
    auto lfo2Amount = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[LFO2Amount])->get();
    auto lfo2Target = static_cast<LFOTarget>(dynamic_cast<juce::AudioParameterChoice*>(getParameters()[LFO2Target])->getIndex());

    auto glideTime = dynamic_cast<juce::AudioParameterFloat*>(getParameters()[GlideTime])->get();
    auto monoMode = dynamic_cast<juce::AudioParameterBool*>(getParameters()[MonoMode])->get();

    // Update all voices
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ZenithPolySynthVoice*>(getVoice(i)))
        {
            // Oscillators
            voice->setOsc1Waveform(osc1Wave);
            voice->setOsc1Detune(osc1Detune);
            voice->setOsc1Mix(osc1Mix);

            voice->setOsc2Waveform(osc2Wave);
            voice->setOsc2Detune(osc2Detune);
            voice->setOsc2Mix(osc2Mix);

            voice->setOsc3Waveform(osc3Wave);
            voice->setOsc3Detune(osc3Detune);
            voice->setOsc3Mix(osc3Mix);

            // Unison
            voice->setUnisonVoices(unisonVoices);
            voice->setUnisonDetune(unisonDetune);

            // Filter
            voice->setFilterType(filterType);
            voice->setFilterCutoff(filterCutoff);
            voice->setFilterResonance(filterResonance);
            voice->setFilterDrive(filterDrive);

            // Envelopes
            voice->setAmpEnvelope(ampAttack, ampDecay, ampSustain, ampRelease);
            voice->setModEnvelope(modAttack, modDecay, modSustain, modRelease);

            // LFOs
            voice->setLFO1(lfo1Rate, lfo1Amount, lfo1Target);
            voice->setLFO2(lfo2Rate, lfo2Amount, lfo2Target);

            // Global
            voice->setGlideTime(glideTime);
            voice->setMonoMode(monoMode);
        }
    }
}

void ZenithPolySynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameter state
    juce::MemoryOutputStream stream(destData, false);

    for (auto* param : getParameters())
    {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
            stream.writeFloat(floatParam->get());
        else if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
            stream.writeInt(choiceParam->getIndex());
        else if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param))
            stream.writeBool(boolParam->get());
        else if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param))
            stream.writeInt(intParam->get());
    }
}

void ZenithPolySynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter state
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);

    for (auto* param : getParameters())
    {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
            floatParam->setValueNotifyingHost(floatParam->convertTo0to1(stream.readFloat()));
        else if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
            choiceParam->setValueNotifyingHost(choiceParam->convertTo0to1(static_cast<float>(stream.readInt())));
        else if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param))
            boolParam->setValueNotifyingHost(stream.readBool() ? 1.0f : 0.0f);
        else if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param))
            intParam->setValueNotifyingHost(intParam->convertTo0to1(stream.readInt()));
    }
}

//==============================================================================
// ZenithPolySynth Implementation
//==============================================================================

ZenithPolySynth::ZenithPolySynth()
    : InstrumentBase(std::make_unique<ZenithPolySynthProcessor>(), createMetadata())
{
    // Map parameter IDs to JUCE indices - ORDER MUST MATCH ZenithPolySynthProcessor::Parameters
    mapParameter("osc1_wave", ZenithPolySynthProcessor::Osc1Wave);
    mapParameter("osc1_detune", ZenithPolySynthProcessor::Osc1Detune);
    mapParameter("osc1_mix", ZenithPolySynthProcessor::Osc1Mix);
    mapParameter("osc2_wave", ZenithPolySynthProcessor::Osc2Wave);
    mapParameter("osc2_detune", ZenithPolySynthProcessor::Osc2Detune);
    mapParameter("osc2_mix", ZenithPolySynthProcessor::Osc2Mix);
    mapParameter("osc3_wave", ZenithPolySynthProcessor::Osc3Wave);
    mapParameter("osc3_detune", ZenithPolySynthProcessor::Osc3Detune);
    mapParameter("osc3_mix", ZenithPolySynthProcessor::Osc3Mix);

    mapParameter("unison_voices", ZenithPolySynthProcessor::UnisonVoices);
    mapParameter("unison_detune", ZenithPolySynthProcessor::UnisonDetune);

    mapParameter("filter_type", ZenithPolySynthProcessor::FilterType);
    mapParameter("filter_cutoff", ZenithPolySynthProcessor::FilterCutoff);
    mapParameter("filter_resonance", ZenithPolySynthProcessor::FilterResonance);
    mapParameter("filter_drive", ZenithPolySynthProcessor::FilterDrive);

    mapParameter("amp_attack", ZenithPolySynthProcessor::AmpAttack);
    mapParameter("amp_decay", ZenithPolySynthProcessor::AmpDecay);
    mapParameter("amp_sustain", ZenithPolySynthProcessor::AmpSustain);
    mapParameter("amp_release", ZenithPolySynthProcessor::AmpRelease);

    mapParameter("mod_attack", ZenithPolySynthProcessor::ModAttack);
    mapParameter("mod_decay", ZenithPolySynthProcessor::ModDecay);
    mapParameter("mod_sustain", ZenithPolySynthProcessor::ModSustain);
    mapParameter("mod_release", ZenithPolySynthProcessor::ModRelease);

    mapParameter("lfo1_rate", ZenithPolySynthProcessor::LFO1Rate);
    mapParameter("lfo1_amount", ZenithPolySynthProcessor::LFO1Amount);
    mapParameter("lfo1_target", ZenithPolySynthProcessor::LFO1Target);

    mapParameter("lfo2_rate", ZenithPolySynthProcessor::LFO2Rate);
    mapParameter("lfo2_amount", ZenithPolySynthProcessor::LFO2Amount);
    mapParameter("lfo2_target", ZenithPolySynthProcessor::LFO2Target);

    mapParameter("glide_time", ZenithPolySynthProcessor::GlideTime);
    mapParameter("mono_mode", ZenithPolySynthProcessor::MonoMode);
    mapParameter("master_gain", ZenithPolySynthProcessor::MasterGain);

    // Register factory presets
    registerPresets();
}

InstrumentMetadata ZenithPolySynth::createMetadata()
{
    InstrumentMetadata metadata;
    metadata.instrumentId = "zenith.poly_synth";
    metadata.name = "Zenith Poly Synth";
    metadata.category = "synth";
    metadata.description = "Multi-oscillator subtractive synthesizer optimized for EDM/trap/future-bass";

    // Helper lambda to add parameters
    auto addParam = [&](const juce::String& id, const juce::String& name, const juce::String& category,
                        ParameterMetadata::Type type, float defVal, float minVal, float maxVal,
                        const juce::String& units = "", const juce::StringArray& choices = {})
    {
        ParameterMetadata param;
        param.id = id;
        param.name = name;
        param.category = category;
        param.type = type;
        param.defaultValue = defVal;
        param.minValue = minVal;
        param.maxValue = maxVal;
        param.units = units;
        param.choices = choices;
        metadata.parameters.push_back(param);
    };

    // Oscillator parameters
    addParam("osc1_wave", "Osc 1 Wave", "Oscillator", ParameterMetadata::Type::Choice, 0.5f, 0.0f, 1.0f, "",
             juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"});
    addParam("osc1_detune", "Osc 1 Detune", "Oscillator", ParameterMetadata::Type::Float, 0.5f, 0.0f, 1.0f, "cents");
    addParam("osc1_mix", "Osc 1 Mix", "Oscillator", ParameterMetadata::Type::Float, 1.0f, 0.0f, 1.0f, "%");

    addParam("osc2_wave", "Osc 2 Wave", "Oscillator", ParameterMetadata::Type::Choice, 0.5f, 0.0f, 1.0f, "",
             juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"});
    addParam("osc2_detune", "Osc 2 Detune", "Oscillator", ParameterMetadata::Type::Float, 0.57f, 0.0f, 1.0f, "cents");
    addParam("osc2_mix", "Osc 2 Mix", "Oscillator", ParameterMetadata::Type::Float, 0.5f, 0.0f, 1.0f, "%");

    addParam("osc3_wave", "Osc 3 Wave", "Oscillator", ParameterMetadata::Type::Choice, 0.0f, 0.0f, 1.0f, "",
             juce::StringArray{"Sine", "Saw", "Square", "Triangle", "Noise", "Supersaw"});
    addParam("osc3_detune", "Osc 3 Detune", "Oscillator", ParameterMetadata::Type::Float, 0.43f, 0.0f, 1.0f, "cents");
    addParam("osc3_mix", "Osc 3 Mix", "Oscillator", ParameterMetadata::Type::Float, 0.0f, 0.0f, 1.0f, "%");

    addParam("unison_voices", "Unison Voices", "Oscillator", ParameterMetadata::Type::Float, 0.0f, 0.0f, 1.0f, "");
    addParam("unison_detune", "Unison Detune", "Oscillator", ParameterMetadata::Type::Float, 0.2f, 0.0f, 1.0f, "cents");

    // Filter parameters
    addParam("filter_type", "Filter Type", "Filter", ParameterMetadata::Type::Choice, 0.0f, 0.0f, 1.0f, "",
             juce::StringArray{"Lowpass", "Bandpass", "Highpass"});
    addParam("filter_cutoff", "Filter Cutoff", "Filter", ParameterMetadata::Type::Float, 0.4f, 0.0f, 1.0f, "Hz");
    addParam("filter_resonance", "Filter Resonance", "Filter", ParameterMetadata::Type::Float, 0.3f, 0.0f, 1.0f, "%");
    addParam("filter_drive", "Filter Drive", "Filter", ParameterMetadata::Type::Float, 0.0f, 0.0f, 1.0f, "dB");

    // Amp Envelope
    addParam("amp_attack", "Amp Attack", "Envelope", ParameterMetadata::Type::Float, 0.01f, 0.0f, 1.0f, "s");
    addParam("amp_decay", "Amp Decay", "Envelope", ParameterMetadata::Type::Float, 0.1f, 0.0f, 1.0f, "s");
    addParam("amp_sustain", "Amp Sustain", "Envelope", ParameterMetadata::Type::Float, 0.8f, 0.0f, 1.0f, "%");
    addParam("amp_release", "Amp Release", "Envelope", ParameterMetadata::Type::Float, 0.3f, 0.0f, 1.0f, "s");

    // Mod Envelope
    addParam("mod_attack", "Mod Attack", "Modulation", ParameterMetadata::Type::Float, 0.01f, 0.0f, 1.0f, "s");
    addParam("mod_decay", "Mod Decay", "Modulation", ParameterMetadata::Type::Float, 0.5f, 0.0f, 1.0f, "s");
    addParam("mod_sustain", "Mod Sustain", "Modulation", ParameterMetadata::Type::Float, 0.0f, 0.0f, 1.0f, "%");
    addParam("mod_release", "Mod Release", "Modulation", ParameterMetadata::Type::Float, 0.1f, 0.0f, 1.0f, "s");

    // LFO 1
    addParam("lfo1_rate", "LFO 1 Rate", "Modulation", ParameterMetadata::Type::Float, 0.5f, 0.0f, 1.0f, "Hz");
    addParam("lfo1_amount", "LFO 1 Amount", "Modulation", ParameterMetadata::Type::Float, 0.0f, 0.0f, 1.0f, "%");
    addParam("lfo1_target", "LFO 1 Target", "Modulation", ParameterMetadata::Type::Choice, 0.0f, 0.0f, 1.0f, "",
             juce::StringArray{"Filter Cutoff", "Osc1 Pitch", "Osc2 Pitch", "Osc1 Mix", "Osc2 Mix"});

    // LFO 2
    addParam("lfo2_rate", "LFO 2 Rate", "Modulation", ParameterMetadata::Type::Float, 0.2f, 0.0f, 1.0f, "Hz");
    addParam("lfo2_amount", "LFO 2 Amount", "Modulation", ParameterMetadata::Type::Float, 0.0f, 0.0f, 1.0f, "%");
    addParam("lfo2_target", "LFO 2 Target", "Modulation", ParameterMetadata::Type::Choice, 0.0f, 0.0f, 1.0f, "",
             juce::StringArray{"Filter Cutoff", "Osc1 Pitch", "Osc2 Pitch", "Osc1 Mix", "Osc2 Mix"});

    // Global
    addParam("glide_time", "Glide Time", "Global", ParameterMetadata::Type::Float, 0.0f, 0.0f, 1.0f, "s");
    addParam("mono_mode", "Mono Mode", "Global", ParameterMetadata::Type::Bool, 0.0f, 0.0f, 1.0f, "");
    addParam("master_gain", "Master Gain", "Global", ParameterMetadata::Type::Float, 0.7f, 0.0f, 1.0f, "dB");

    //==========================================================================
    // AI-FRIENDLY MACRO CONTROLS
    //==========================================================================
    //
    // Macros provide high-level semantic controls that are easy for AI to reason about.
    // Instead of tweaking dozens of low-level parameters, AI can use macros like:
    //   - "Increase brightness" -> adjusts filter cutoff and resonance
    //   - "Add more movement" -> adjusts LFO amounts
    //   - "Make it thicker" -> adjusts unison and oscillator mix
    //
    // Each macro affects multiple parameters with pre-defined relationships.
    // This makes it much easier for AI to create expressive, musical results.
    //

    // Macro 1: Brightness
    {
        MacroMetadata macro;
        macro.id = "macro_brightness";
        macro.name = "Brightness";
        macro.description = "Controls overall tonal brightness. Increases filter cutoff and adds subtle resonance for more sparkle.";

        macro.targets.push_back({"filter_cutoff", 0.8f});
        macro.targets.push_back({"filter_resonance", 0.4f});
        metadata.macros.push_back(macro);
    }

    // Macro 2: Thickness
    {
        MacroMetadata macro;
        macro.id = "macro_thickness";
        macro.name = "Thickness";
        macro.description = "Controls sound thickness and richness. Adds unison voices and increases oscillator layering for a fuller sound.";

        macro.targets.push_back({"unison_voices", 0.7f});
        macro.targets.push_back({"osc2_mix", 0.6f});
        macro.targets.push_back({"unison_detune", 0.5f});
        metadata.macros.push_back(macro);
    }

    // Macro 3: Movement
    {
        MacroMetadata macro;
        macro.id = "macro_movement";
        macro.name = "Movement";
        macro.description = "Controls modulation movement and motion. Increases LFO amounts for more dynamic, evolving sounds.";

        macro.targets.push_back({"lfo1_amount", 0.8f});
        macro.targets.push_back({"lfo2_amount", 0.6f});
        metadata.macros.push_back(macro);
    }

    // Macro 4: Attack
    {
        MacroMetadata macro;
        macro.id = "macro_attack";
        macro.name = "Attack";
        macro.description = "Controls how quickly the sound starts. Lower values create slow, gradual fades; higher values create instant, punchy attacks.";

        macro.targets.push_back({"amp_attack", -0.9f});  // Negative: macro up = faster attack
        macro.targets.push_back({"mod_attack", -0.7f});
        metadata.macros.push_back(macro);
    }

    // Macro 5: Release
    {
        MacroMetadata macro;
        macro.id = "macro_release";
        macro.name = "Release";
        macro.description = "Controls how long the sound takes to fade after note release. Higher values create longer, more sustaining tails.";

        macro.targets.push_back({"amp_release", 0.8f});
        macro.targets.push_back({"mod_release", 0.6f});
        metadata.macros.push_back(macro);
    }

    // Macro 6: Warmth
    {
        MacroMetadata macro;
        macro.id = "macro_warmth";
        macro.name = "Warmth";
        macro.description = "Controls tonal warmth and saturation. Reduces filter cutoff and adds subtle drive for analog-style warmth.";

        macro.targets.push_back({"filter_cutoff", -0.6f});  // Negative: macro up = darker sound
        macro.targets.push_back({"filter_drive", 0.5f});
        metadata.macros.push_back(macro);
    }

    // Macro 7: Detune
    {
        MacroMetadata macro;
        macro.id = "macro_detune";
        macro.name = "Detune";
        macro.description = "Controls oscillator detuning amount. Adds chorus-like effects and width by detuning oscillators against each other.";

        macro.targets.push_back({"osc2_detune", 0.7f});
        macro.targets.push_back({"osc3_detune", -0.7f});  // Negative: detune in opposite direction
        macro.targets.push_back({"unison_detune", 0.6f});
        metadata.macros.push_back(macro);
    }

    // Macro 8: Depth
    {
        MacroMetadata macro;
        macro.id = "macro_depth";
        macro.name = "Depth";
        macro.description = "Controls modulation envelope depth and sustain. Increases how much the modulation envelope affects the sound over time.";

        macro.targets.push_back({"mod_sustain", 0.7f});
        macro.targets.push_back({"mod_decay", 0.6f});
        metadata.macros.push_back(macro);
    }

    return metadata;
}

void ZenithPolySynth::registerPresets()
{
    // Preset 1: EDM Supersaw Lead
    {
        std::map<juce::String, float> params;
        params["osc1_wave"] = 5.0f / 5.0f;    // Supersaw
        params["osc1_mix"] = 1.0f;
        params["osc2_wave"] = 1.0f / 5.0f;    // Saw
        params["osc2_detune"] = 0.6f;
        params["osc2_mix"] = 0.7f;
        params["unison_voices"] = 6.0f / 6.0f;
        params["unison_detune"] = 0.4f;
        params["filter_cutoff"] = 0.7f;
        params["filter_resonance"] = 0.4f;
        params["amp_attack"] = 0.02f;
        params["amp_decay"] = 0.3f;
        params["amp_sustain"] = 0.7f;
        params["amp_release"] = 0.5f;
        InstrumentBase::registerPreset("edm_supersaw_lead", "EDM Supersaw Lead", params);
    }

    // Preset 2: Trap 808 Bass
    {
        std::map<juce::String, float> params;
        params["osc1_wave"] = 0.0f / 5.0f;    // Sine
        params["osc1_mix"] = 1.0f;
        params["osc2_wave"] = 2.0f / 5.0f;    // Square
        params["osc2_detune"] = 0.48f;        // -2 cents
        params["osc2_mix"] = 0.3f;
        params["filter_type"] = 0.0f;         // Lowpass
        params["filter_cutoff"] = 0.3f;
        params["filter_resonance"] = 0.5f;
        params["filter_drive"] = 1.5f / 5.0f;
        params["amp_attack"] = 0.001f;
        params["amp_decay"] = 0.6f;
        params["amp_sustain"] = 0.3f;
        params["amp_release"] = 0.8f;
        params["mod_attack"] = 0.001f;
        params["mod_decay"] = 0.4f;
        params["mod_sustain"] = 0.0f;
        InstrumentBase::registerPreset("trap_808_bass", "Trap 808 Bass", params);
    }

    // Preset 3: Future Bass Chord
    {
        std::map<juce::String, float> params;
        params["osc1_wave"] = 1.0f / 5.0f;    // Saw
        params["osc1_mix"] = 0.8f;
        params["osc2_wave"] = 2.0f / 5.0f;    // Square
        params["osc2_detune"] = 0.55f;        // +5 cents
        params["osc2_mix"] = 0.6f;
        params["osc3_wave"] = 0.0f / 5.0f;    // Sine
        params["osc3_detune"] = 0.4f;         // -10 cents
        params["osc3_mix"] = 0.4f;
        params["filter_cutoff"] = 0.5f;
        params["filter_resonance"] = 0.35f;
        params["amp_attack"] = 0.05f;
        params["amp_decay"] = 0.4f;
        params["amp_sustain"] = 0.85f;
        params["amp_release"] = 1.0f;
        params["lfo1_rate"] = 0.25f;          // ~5 Hz
        params["lfo1_amount"] = 0.3f;
        params["lfo1_target"] = 0.0f;         // Filter Cutoff
        InstrumentBase::registerPreset("future_bass_chord", "Future Bass Chord", params);
    }

    // Preset 4: Wobble Bass
    {
        std::map<juce::String, float> params;
        params["osc1_wave"] = 1.0f / 5.0f;    // Saw
        params["osc1_mix"] = 1.0f;
        params["osc2_wave"] = 1.0f / 5.0f;    // Saw
        params["osc2_detune"] = 0.52f;        // +2 cents
        params["osc2_mix"] = 0.9f;
        params["filter_type"] = 0.0f;         // Lowpass
        params["filter_cutoff"] = 0.4f;
        params["filter_resonance"] = 0.75f;
        params["filter_drive"] = 2.0f / 5.0f;
        params["amp_attack"] = 0.001f;
        params["amp_sustain"] = 1.0f;
        params["amp_release"] = 0.1f;
        params["lfo1_rate"] = 0.15f;          // ~3 Hz
        params["lfo1_amount"] = 0.8f;
        params["lfo1_target"] = 0.0f;         // Filter Cutoff
        InstrumentBase::registerPreset("wobble_bass", "Wobble Bass", params);
    }

    // Preset 5: Pluck Lead
    {
        std::map<juce::String, float> params;
        params["osc1_wave"] = 3.0f / 5.0f;    // Triangle
        params["osc1_mix"] = 1.0f;
        params["filter_cutoff"] = 0.8f;
        params["filter_resonance"] = 0.2f;
        params["amp_attack"] = 0.001f;
        params["amp_decay"] = 0.15f;
        params["amp_sustain"] = 0.2f;
        params["amp_release"] = 0.1f;
        params["mod_attack"] = 0.001f;
        params["mod_decay"] = 0.3f;
        params["mod_sustain"] = 0.0f;
        InstrumentBase::registerPreset("pluck_lead", "Pluck Lead", params);
    }
}

//==============================================================================
// Debug Test Helper (JUCE_DEBUG only)
//==============================================================================

#if JUCE_DEBUG

/**
 * @brief Quick sanity check for ZenithPolySynth
 *
 * To test the synth, you can create a simple test in your DAW's debug mode:
 *
 * Example usage:
 *
 * @code
 * void testZenithPolySynth()
 * {
 *     // Create synth instance
 *     auto synth = std::make_unique<zenith::ZenithPolySynth>();
 *     auto* processor = synth->getAudioProcessor();
 *
 *     // Prepare for playback
 *     processor->prepareToPlay(44100.0, 512);
 *
 *     // Create a test chord: C major (MIDI notes 60, 64, 67)
 *     juce::MidiBuffer midiBuffer;
 *     midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);  // C
 *     midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 64, 0.8f), 0);  // E
 *     midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 67, 0.8f), 0);  // G
 *
 *     // Process audio for 2 seconds
 *     juce::AudioBuffer<float> audioBuffer(2, 512);
 *     for (int i = 0; i < (44100 * 2) / 512; ++i)
 *     {
 *         audioBuffer.clear();
 *         processor->processBlock(audioBuffer, midiBuffer);
 *
 *         // Check that we're generating audio
 *         if (i == 10)  // After a few blocks
 *         {
 *             float rms = audioBuffer.getRMSLevel(0, 0, audioBuffer.getNumSamples());
 *             DBG("RMS level: " << rms);
 *             jassert(rms > 0.001f);  // Should be generating sound
 *         }
 *
 *         midiBuffer.clear();
 *     }
 *
 *     // Note off
 *     midiBuffer.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
 *     midiBuffer.addEvent(juce::MidiMessage::noteOff(1, 64), 0);
 *     midiBuffer.addEvent(juce::MidiMessage::noteOff(1, 67), 0);
 *
 *     // Process release
 *     for (int i = 0; i < 100; ++i)
 *     {
 *         audioBuffer.clear();
 *         processor->processBlock(audioBuffer, midiBuffer);
 *         midiBuffer.clear();
 *     }
 *
 *     DBG("ZenithPolySynth test completed successfully!");
 * }
 * @endcode
 *
 * To test presets:
 *
 * @code
 * void testZenithPolySynthPresets()
 * {
 *     auto synth = std::make_unique<zenith::ZenithPolySynth>();
 *
 *     // List available presets
 *     auto presetIds = synth->getPresetIds();
 *     DBG("Available presets:");
 *     for (const auto& id : presetIds)
 *         DBG("  - " << id);
 *
 *     // Load and test each preset
 *     for (const auto& id : presetIds)
 *     {
 *         DBG("Testing preset: " << id);
 *         synth->loadPreset(id);
 *
 *         // Verify parameters were loaded
 *         float filterCutoff = synth->getParameter("filter_cutoff");
 *         DBG("  Filter cutoff: " << filterCutoff);
 *     }
 * }
 * @endcode
 *
 * To test parameter changes:
 *
 * @code
 * void testZenithPolySynthParameters()
 * {
 *     auto synth = std::make_unique<zenith::ZenithPolySynth>();
 *
 *     // Test setting parameters
 *     synth->setParameter("filter_cutoff", 0.8f);
 *     synth->setParameter("filter_resonance", 0.5f);
 *     synth->setParameter("amp_attack", 0.1f);
 *
 *     // Test macros
 *     synth->setMacro("macro_brightness", 0.9f);
 *     synth->setMacro("macro_thickness", 0.7f);
 *
 *     DBG("Parameter test completed!");
 * }
 * @endcode
 */

#endif // JUCE_DEBUG

} // namespace zenith
