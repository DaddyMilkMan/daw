/*
  ==============================================================================

    ZenithUltraSynthVoice.cpp
    Created: [Date] Author: Claude AI
    Implementation of the Zenith Ultra Synth voice processor

  ==============================================================================
*/

#include "ZenithUltraSynthVoice.h"
#include "../ZenithPolySynth.h"
#include "../../ZenithOscillator.h"
#include "../../ZenithFilter.h"

namespace Zenith
{

ZenithUltraSynthVoice::ZenithUltraSynthVoice(ZenithUltraSynthProcessor* processor)
    : synthesisMode_(ZenithUltraSynthProcessor::SynthesisMode::Subtractive)
    , voiceIndex_(0)
    , unisonDetune_(0.0f)
    , isActive_(false)
    , isSustaining_(false)
    , isReleased_(false)
    , midiNote_(0)
    , velocity_(0.0f)
    , pitchBend_(0.0f)
    , notePressure_(0.0f)
    , timbre_(0.0f)
    , voiceStartTime_(0)
    , voiceStopTime_(0)
    , internalBufferSize_(2048)
{
    // Initialize audio buffer
    internalBuffer_.setSize(2, internalBufferSize_);

    // Initialize per-engine voices
    initializeVoiceEngines();

    // Initialize modulation
    modulationDepths_.fill(0.0f);

    // Initialize smoothed parameters
    smoothedFrequency_.reset(sampleRate_, 0.1);
    smoothedAmplitude_.reset(sampleRate_, 0.1);
}

ZenithUltraSynthVoice::~ZenithUltraSynthVoice()
{
    clearCurrentNote();
}

void ZenithUltraSynthVoice::initializeVoiceEngines()
{
    try {
        // Initialize subtractive voice (existing ZenithPolySynth)
        subtractiveVoice_ = std::make_unique<SubtractiveVoice>();

        // Initialize physical modeling voice
        physicalModelingVoice_ = std::make_unique<PhysicalModelingVoice>();

        // Initialize neural synthesis voice
        neuralSynthVoice_ = std::make_unique<NeuralSynthVoice>();

        // Initialize wavetable voice
        wavetableVoice_ = std::make_unique<AdvancedWavetableVoice>();
    }
    catch (const std::exception& e) {
        DBG("ZenithUltraSynthVoice: Error initializing voice engines - " << e.what());
    }
}

void ZenithUltraSynthVoice::clearCurrentNote()
{
    // Stop all engines
    if (physicalModelingVoice_) physicalModelingVoice_->noteOff();
    if (neuralSynthVoice_) neuralSynthVoice_->noteOff();
    if (wavetableVoice_) wavetableVoice_->noteOff();
    if (subtractiveVoice_) subtractiveVoice_->clearCurrentNote();

    // Reset state
    isActive_ = false;
    isSustaining_ = false;
    isReleased_ = false;
    midiNote_ = 0;
    velocity_ = 0.0f;

    voiceStopTime_ = juce::Time::getMillisecondCounter();
}

bool ZenithUltraSynthVoice::canPlaySound(const juce::MPESound* sound)
{
    return dynamic_cast<const ZenithUltraSynthSound*>(sound) != nullptr;
}

void ZenithUltraSynthVoice::noteStarted()
{
    if (!isActive_)
    {
        isActive_ = true;
        isSustaining_ = true;
        voiceStartTime_ = juce::Time::getMillisecondCounter();

        // Get current note parameters
        auto mpeNote = getCurrentlyPlayingNote();
        midiNote_ = mpeNote.initialNote;
        velocity_ = mpeNote.noteOnVelocity.asUnsignedFloat();
        pitchBend_ = mpeNote.pitchbend.asUnsignedFloat() * 2.0f - 1.0f;
        notePressure_ = mpeNote.pressure.asUnsignedFloat();
        timbre_ = mpeNote.timbre.asUnsignedFloat();

        // Calculate frequency
        float frequency = mpeNote.getFrequencyInHertz();

        // Start appropriate engine based on synthesis mode
        switch (synthesisMode_)
        {
        case ZenithUltraSynthProcessor::SynthesisMode::PhysicalModeling:
            if (physicalModelingVoice_)
            {
                physicalModelingVoice_->noteOn(frequency, velocity_);
            }
            break;

        case ZenithUltraSynthProcessor::SynthesisMode::Neural:
            if (neuralSynthVoice_)
            {
                neuralSynthVoice_->noteOn(frequency, velocity_);
            }
            break;

        case ZenithUltraSynthProcessor::SynthesisMode::Wavetable:
            if (wavetableVoice_)
            {
                wavetableVoice_->noteOn(frequency, velocity_);
            }
            break;

        case ZenithUltraSynthProcessor::SynthesisMode::Subtractive:
        case ZenithUltraSynthProcessor::SynthesisMode::Hybrid:
        default:
            if (subtractiveVoice_)
            {
                subtractiveVoice_->noteOn(midiNote_, velocity_);
            }
            break;
        }

        // Update MPE parameters
        updateMPEParameters();
    }
}

void ZenithUltraSynthVoice::noteStopped(bool allowTailOff)
{
    if (isActive_)
    {
        isSustaining_ = false;
        isReleased_ = true;

        // Stop appropriate engine based on synthesis mode
        switch (synthesisMode_)
        {
        case ZenithUltraSynthProcessor::SynthesisMode::PhysicalModeling:
            if (physicalModelingVoice_)
            {
                physicalModelingVoice_->noteOff();
            }
            break;

        case ZenithUltraSynthProcessor::SynthesisMode::Neural:
            if (neuralSynthVoice_)
            {
                neuralSynthVoice_->noteOff();
            }
            break;

        case ZenithUltraSynthProcessor::SynthesisMode::Wavetable:
            if (wavetableVoice_)
            {
                wavetableVoice_->noteOff();
            }
            break;

        case ZenithUltraSynthProcessor::SynthesisMode::Subtractive:
        case ZenithUltraSynthProcessor::SynthesisMode::Hybrid:
        default:
            if (subtractiveVoice_)
            {
                subtractiveVoice_->noteStopped(allowTailOff);
            }
            break;
        }

        // Check if voice is actually stopped
        if (!allowTailOff)
        {
            clearCurrentNote();
        }
    }
}

void ZenithUltraSynthVoice::notePressureChanged()
{
    if (isActive_)
    {
        notePressure_ = getCurrentlyPlayingNote().pressure.asUnsignedFloat();
        updateMPEParameters();
    }
}

void ZenithUltraSynthVoice::notePitchbendChanged()
{
    if (isActive_)
    {
        pitchBend_ = getCurrentlyPlayingNote().pitchbend.asUnsignedFloat() * 2.0f - 1.0f;
        updateMPEParameters();
    }
}

void ZenithUltraSynthVoice::noteTimbreChanged()
{
    if (isActive_)
    {
        timbre_ = getCurrentlyPlayingNote().timbre.asUnsignedFloat();
        updateMPEParameters();
    }
}

void ZenithUltraSynthVoice::noteKeyStateChanged()
{
    // Handle key state changes (legato, staccato, etc.)
    // Implementation depends on specific requirements
}

void ZenithUltraSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                           int startSample, int numSamples)
{
    if (!isActive_ || numSamples <= 0)
    {
        return;
    }

    // Clear internal buffer
    internalBuffer_.clear();
    internalBuffer_.setNumSamples(numSamples);

    // Render based on synthesis mode
    switch (synthesisMode_)
    {
    case ZenithUltraSynthProcessor::SynthesisMode::PhysicalModeling:
        renderPhysicalModeling(internalBuffer_, numSamples);
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Neural:
        renderNeuralSynthesis(internalBuffer_, numSamples);
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Wavetable:
        renderWavetable(internalBuffer_, numSamples);
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Subtractive:
    case ZenithUltraSynthProcessor::SynthesisMode::Hybrid:
    default:
        renderSubtractive(internalBuffer_, numSamples);
        break;
    }

    // Process voice modulation
    processVoiceModulation(internalBuffer_, numSamples);

    // Apply voice effects
    applyVoiceEffects(internalBuffer_, numSamples);

    // Copy to output buffer
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        int startChan = channel % internalBuffer_.getNumChannels();
        outputBuffer.addFrom(channel, startSample, internalBuffer_, startChan, 0, numSamples);
    }

    // Check if voice has finished
    if (isReleased_ && !isSustaining_)
    {
        if (checkVoiceFinished())
        {
            clearCurrentNote();
        }
    }
}

void ZenithUltraSynthVoice::renderPhysicalModeling(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (physicalModelingVoice_)
    {
        physicalModelingVoice_->process(buffer, numSamples);
    }
}

void ZenithUltraSynthVoice::renderNeuralSynthesis(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (neuralSynthVoice_)
    {
        neuralSynthVoice_->process(buffer, numSamples);
    }
}

void ZenithUltraSynthVoice::renderWavetable(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (wavetableVoice_)
    {
        wavetableVoice_->process(buffer, numSamples);
    }
}

void ZenithUltraSynthVoice::renderSubtractive(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (subtractiveVoice_)
    {
        // Handle unison detuning
        if (unisonDetune_ > 0.0f && voiceIndex_ == 0)
        {
            // For unison voices, render with detune
            auto originalBuffer = buffer;
            subtractiveVoice_->process(buffer, numSamples);

            // Add detuned copy
            detunedBuffer_.clear();
            detunedBuffer_.setSize(buffer.getNumChannels(), numSamples);

            // Apply detuning by modifying pitch bend
            auto originalPitchBend = pitchBend_;
            pitchBend_ += unisonDetune_;
            subtractiveVoice_->process(detunedBuffer_, numSamples);
            pitchBend_ = originalPitchBend;

            // Mix original and detuned
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                buffer.add(channel, 0, detunedBuffer_, channel, 0, numSamples, 0.5f);
            }
        }
        else
        {
            subtractiveVoice_->process(buffer, numSamples);
        }
    }
}

void ZenithUltraSynthVoice::processVoiceModulation(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Apply voice-specific modulation (LFOs, envelopes, etc.)
    // Implementation depends on specific modulation requirements

    // Placeholder for modulation processing
    // This would handle:
    // - Pitch modulation
    // - Amplitude modulation
    // - Filter modulation
    // - Modulation routing
}

void ZenithUltraSynthVoice::applyVoiceEffects(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Apply voice-specific effects (reverb, delay, etc.)
    // Implementation depends on specific effect requirements

    // Placeholder for effect processing
    // This would handle:
    // - Reverb
    // - Delay
    // - Chorus
    // - Flanger
    // etc.
}

void ZenithUltraSynthVoice::updateMPEParameters()
{
    // Apply MPE parameters to active synthesis engine
    switch (synthesisMode_)
    {
    case ZenithUltraSynthProcessor::SynthesisMode::PhysicalModeling:
        if (physicalModelingVoice_)
        {
            physicalModelingVoice_->setPressure(notePressure_);
            physicalModelingVoice_->setPitchBend(pitchBend_);
            physicalModelingVoice_->setTimbre(timbre_);
        }
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Neural:
        if (neuralSynthVoice_)
        {
            neuralSynthVoice_->setPressure(notePressure_);
            neuralSynthVoice_->setPitchBend(pitchBend_);
            neuralSynthVoice_->setTimbre(timbre_);
        }
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Wavetable:
        if (wavetableVoice_)
        {
            wavetableVoice_->setPressure(notePressure_);
            wavetableVoice_->setPitchBend(pitchBend_);
            wavetableVoice_->setTimbre(timbre_);
        }
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Subtractive:
    case ZenithUltraSynthProcessor::SynthesisMode::Hybrid:
    default:
        if (subtractiveVoice_)
        {
            // Apply MPE parameters to subtractive engine
            // Implementation depends on MPE support in existing engine
        }
        break;
    }
}

void ZenithUltraSynthVoice::setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode mode)
{
    synthesisMode_ = mode;
}

ZenithUltraSynthProcessor::SynthesisMode ZenithUltraSynthVoice::getSynthesisMode() const
{
    return synthesisMode_;
}

void ZenithUltraSynthVoice::setVoiceIndex(int index)
{
    voiceIndex_ = index;
}

int ZenithUltraSynthVoice::getVoiceIndex() const
{
    return voiceIndex_;
}

void ZenithUltraSynthVoice::setUnisonDetune(float detuneAmount)
{
    unisonDetune_ = detuneAmount;
}

float ZenithUltraSynthVoice::getUnisonDetune() const
{
    return unisonDetune_;
}

void ZenithUltraSynthVoice::handleMidiEvent(const juce::MidiMessage& m)
{
    juce::MPESynthesiserVoice::handleMidiEvent(m);

    // Handle specific MIDI events
    if (m.isPitchWheel())
    {
        pitchBend_ = m.getPitchWheelValue() / 8192.0f - 1.0f;
    }
    else if (m.isAftertouch())
    {
        notePressure_ = m.getAfterTouchValue() / 127.0f;
    }
}

bool ZenithUltraSynthVoice::isVoiceActive() const
{
    return isActive_;
}

float ZenithUltraSynthVoice::getVelocity() const
{
    return velocity_;
}

float ZenithUltraSynthVoice::getFrequency() const
{
    return getCurrentlyPlayingNote().getFrequencyInHertz();
}

int ZenithUltraSynthVoice::getMidiNote() const
{
    return midiNote_;
}

void ZenithUltraSynthVoice::setModulationDepth(int modulatorIndex, float depth)
{
    if (modulatorIndex >= 0 && modulatorIndex < modulationDepths_.size())
    {
        modulationDepths_[modulatorIndex] = depth;
    }
}

float ZenithUltraSynthVoice::getModulationDepth(int modulatorIndex) const
{
    if (modulatorIndex >= 0 && modulatorIndex < modulationDepths_.size())
    {
        return modulationDepths_[modulatorIndex];
    }
    return 0.0f;
}

float ZenithUltraSynthVoice::getVoiceCpuUsage() const
{
    // Estimate CPU usage for this voice
    // This would be implemented based on actual processing load
    return 0.0f;
}

juce::uint64 ZenithUltraSynthVoice::getVoiceAge() const
{
    if (isActive_)
    {
        return juce::Time::getMillisecondCounter() - voiceStartTime_;
    }
    return 0;
}

bool ZenithUltraSynthVoice::isSustaining() const
{
    return isSustaining_;
}

bool ZenithUltraSynthVoice::isReleased() const
{
    return isReleased_;
}

float ZenithUltraSynthVoice::getReleaseProgress() const
{
    if (isReleased_ && voiceStopTime_ > 0)
    {
        auto elapsed = juce::Time::getMillisecondCounter() - voiceStopTime_;
        return juce::jlimit(0.0f, 1.0f, elapsed / 1000.0f); // 1 second release
    }
    return 0.0f;
}

bool ZenithUltraSynthVoice::checkVoiceFinished()
{
    // Check if voice has finished playing
    // This depends on the specific synthesis engine and release behavior

    switch (synthesisMode_)
    {
    case ZenithUltraSynthProcessor::SynthesisMode::PhysicalModeling:
        if (physicalModelingVoice_)
        {
            return !physicalModelingVoice_->isActive();
        }
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Neural:
        if (neuralSynthVoice_)
        {
            return !neuralSynthVoice_->isActive();
        }
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Wavetable:
        if (wavetableVoice_)
        {
            return !wavetableVoice_->isActive();
        }
        break;

    case ZenithUltraSynthProcessor::SynthesisMode::Subtractive:
    case ZenithUltraSynthProcessor::SynthesisMode::Hybrid:
    default:
        if (subtractiveVoice_)
        {
            return !subtractiveVoice_->isVoiceActive();
        }
        break;
    }

    return false;
}

} // namespace Zenith