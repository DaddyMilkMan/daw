#include "instruments/ZenithPolySynthVoice.h"

namespace zenith {
namespace instruments {

ZenithPolySynthVoice::ZenithPolySynthVoice()
{
    // Initialize envelope with sensible defaults
    ampEnvParams_.attack = 0.01f;
    ampEnvParams_.decay = 0.1f;
    ampEnvParams_.sustain = 0.7f;
    ampEnvParams_.release = 0.3f;
    ampEnv_.setParameters(ampEnvParams_);
}

bool ZenithPolySynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<ZenithPolySynthSound*>(sound) != nullptr;
}

void ZenithPolySynthVoice::startNote(int midiNoteNumber,
                                     float velocity,
                                     juce::SynthesiserSound* /*sound*/,
                                     int /*currentPitchWheelPosition*/)
{
    currentMidiNote_ = midiNoteNumber;
    currentVelocity_ = velocity;
    currentFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    // Calculate phase increments for both oscillators
    osc1PhaseIncrement_ = currentFrequency_ / sampleRate_;

    // Apply detune to osc2 (detune is in cents: 100 cents = 1 semitone)
    double detuneRatio = std::pow(2.0, osc2Detune_ / 1200.0);
    double osc2Frequency = currentFrequency_ * detuneRatio;
    osc2PhaseIncrement_ = osc2Frequency / sampleRate_;

    // Reset phases (or don't, for free-running oscillators - we'll reset for cleaner sound)
    osc1Phase_ = 0.0;
    osc2Phase_ = 0.0;

    // Start envelope
    ampEnv_.noteOn();
}

void ZenithPolySynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnv_.noteOff();
    }
    else
    {
        // Hard stop - kill the voice immediately
        ampEnv_.reset();
        clearCurrentNote();
    }
}

void ZenithPolySynthVoice::pitchWheelMoved(int /*newPitchWheelValue*/)
{
    // TODO: Implement pitch bend if desired
}

void ZenithPolySynthVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
    // TODO: Implement MIDI CC handling if desired
}

void ZenithPolySynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                          int startSample,
                                          int numSamples)
{
    // If envelope is not active, we can stop rendering
    if (!ampEnv_.isActive())
    {
        clearCurrentNote();
        return;
    }

    // Process each sample
    for (int i = 0; i < numSamples; ++i)
    {
        // Generate oscillator outputs
        float osc1Out = generateOsc1Sample();
        float osc2Out = generateOsc2Sample();

        // Mix oscillators
        float oscMixed = osc1Out * (1.0f - oscMix_) + osc2Out * oscMix_;

        // Add noise
        float noiseSample = generateNoiseSample();
        float mixed = oscMixed * (1.0f - noiseLevel_) + noiseSample * noiseLevel_;

        // Apply filter
        filter_.setCutoffFrequency(filterCutoff_);
        filter_.setResonance(filterResonance_);

        switch (filterType_)
        {
            case FilterType::LowPass:
                filter_.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                break;
            case FilterType::BandPass:
                filter_.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                break;
            case FilterType::HighPass:
                filter_.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                break;
        }

        float filtered = filter_.processSample(0, mixed);

        // Apply envelope and velocity
        float envValue = ampEnv_.getNextSample();
        float output = filtered * envValue * currentVelocity_ * gain_;

        // Write to output buffer (mix into existing content)
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample(channel, startSample + i, output);
        }
    }
}

void ZenithPolySynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    sampleRate_ = sampleRate;

    // Prepare filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1; // Mono processing per voice

    filter_.prepare(spec);
    filter_.reset();

    // Prepare envelope
    ampEnv_.setSampleRate(sampleRate);
    ampEnv_.reset();
}

//==============================================================================
// Oscillator generation

float ZenithPolySynthVoice::generateOsc1Sample()
{
    float sample = 0.0f;

    switch (osc1WaveType_)
    {
        case OscWaveType::Sine:
            sample = std::sin(osc1Phase_ * juce::MathConstants<double>::twoPi);
            break;

        case OscWaveType::Saw:
            sample = static_cast<float>(2.0 * osc1Phase_ - 1.0);
            break;

        case OscWaveType::Square:
            sample = (osc1Phase_ < 0.5) ? 1.0f : -1.0f;
            break;

        case OscWaveType::Triangle:
            sample = static_cast<float>(4.0 * std::abs(osc1Phase_ - 0.5) - 1.0);
            break;
    }

    // Advance phase
    osc1Phase_ += osc1PhaseIncrement_;
    if (osc1Phase_ >= 1.0)
        osc1Phase_ -= 1.0;

    return sample;
}

float ZenithPolySynthVoice::generateOsc2Sample()
{
    float sample = 0.0f;

    switch (osc2WaveType_)
    {
        case OscWaveType::Sine:
            sample = std::sin(osc2Phase_ * juce::MathConstants<double>::twoPi);
            break;

        case OscWaveType::Saw:
            sample = static_cast<float>(2.0 * osc2Phase_ - 1.0);
            break;

        case OscWaveType::Square:
            sample = (osc2Phase_ < 0.5) ? 1.0f : -1.0f;
            break;

        case OscWaveType::Triangle:
            sample = static_cast<float>(4.0 * std::abs(osc2Phase_ - 0.5) - 1.0);
            break;
    }

    // Advance phase
    osc2Phase_ += osc2PhaseIncrement_;
    if (osc2Phase_ >= 1.0)
        osc2Phase_ -= 1.0;

    return sample;
}

float ZenithPolySynthVoice::generateNoiseSample()
{
    return random_.nextFloat() * 2.0f - 1.0f; // White noise [-1, 1]
}

} // namespace instruments
} // namespace zenith
