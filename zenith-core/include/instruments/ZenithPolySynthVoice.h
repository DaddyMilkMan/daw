#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {
namespace instruments {

/**
 * @brief Individual voice for ZenithPolySynth
 *
 * Each voice contains:
 * - Two oscillators (saw, square, triangle, sine)
 * - State Variable Filter (LP/BP/HP)
 * - ADSR amplitude envelope
 * - Detune and unison support
 *
 * This class is RT-safe and does no allocations in renderNextBlock.
 */
class ZenithPolySynthVoice : public juce::SynthesiserVoice
{
public:
    ZenithPolySynthVoice();

    //==============================================================================
    // SynthesiserVoice interface

    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void startNote(int midiNoteNumber,
                   float velocity,
                   juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;

    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample,
                        int numSamples) override;

    //==============================================================================
    // Voice configuration

    void prepareToPlay(double sampleRate, int samplesPerBlock);

    //==============================================================================
    // Parameter setters (called from audio thread via atomic parameters)

    enum class OscWaveType
    {
        Sine = 0,
        Saw,
        Square,
        Triangle
    };

    enum class FilterType
    {
        LowPass = 0,
        BandPass,
        HighPass
    };

    void setOsc1Wave(OscWaveType type) { osc1WaveType_ = type; }
    void setOsc2Wave(OscWaveType type) { osc2WaveType_ = type; }
    void setOsc2Detune(float cents) { osc2Detune_ = cents; }
    void setOscMix(float mix) { oscMix_ = juce::jlimit(0.0f, 1.0f, mix); }
    void setNoiseLevel(float level) { noiseLevel_ = juce::jlimit(0.0f, 1.0f, level); }

    void setFilterType(FilterType type) { filterType_ = type; }
    void setFilterCutoff(float cutoffHz) { filterCutoff_ = cutoffHz; }
    void setFilterResonance(float resonance) { filterResonance_ = juce::jlimit(0.0f, 1.0f, resonance); }

    void setAmpEnvAttack(float timeSeconds) { ampEnv_.setParameters({timeSeconds, ampEnv_.getParameters().decay, ampEnv_.getParameters().sustain, ampEnv_.getParameters().release}); }
    void setAmpEnvDecay(float timeSeconds) { ampEnv_.setParameters({ampEnv_.getParameters().attack, timeSeconds, ampEnv_.getParameters().sustain, ampEnv_.getParameters().release}); }
    void setAmpEnvSustain(float level) { ampEnv_.setParameters({ampEnv_.getParameters().attack, ampEnv_.getParameters().decay, level, ampEnv_.getParameters().release}); }
    void setAmpEnvRelease(float timeSeconds) { ampEnv_.setParameters({ampEnv_.getParameters().attack, ampEnv_.getParameters().decay, ampEnv_.getParameters().sustain, timeSeconds}); }

    void setGain(float gainLinear) { gain_ = gainLinear; }

private:
    //==============================================================================
    // Oscillator generation

    float generateOsc1Sample();
    float generateOsc2Sample();
    float generateNoiseSample();

    //==============================================================================
    // Voice state

    double sampleRate_ = 44100.0;
    int currentMidiNote_ = -1;
    float currentVelocity_ = 0.0f;
    double currentFrequency_ = 440.0;

    // Oscillator state
    OscWaveType osc1WaveType_ = OscWaveType::Saw;
    OscWaveType osc2WaveType_ = OscWaveType::Saw;
    float osc2Detune_ = 0.0f; // cents
    float oscMix_ = 0.5f; // 0 = all osc1, 1 = all osc2
    float noiseLevel_ = 0.0f;

    double osc1Phase_ = 0.0;
    double osc2Phase_ = 0.0;
    double osc1PhaseIncrement_ = 0.0;
    double osc2PhaseIncrement_ = 0.0;

    // Filter
    FilterType filterType_ = FilterType::LowPass;
    float filterCutoff_ = 20000.0f;
    float filterResonance_ = 0.0f;

    using SVFilter = juce::dsp::StateVariableTPTFilter<float>;
    SVFilter filter_;

    // Envelope
    juce::ADSR ampEnv_;
    juce::ADSR::Parameters ampEnvParams_;

    // Noise generator
    juce::Random random_;

    // Master gain
    float gain_ = 0.8f;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthVoice)
};

/**
 * @brief Simple sound class for ZenithPolySynth
 *
 * This is required by JUCE's Synthesiser architecture but doesn't
 * hold any data for our use case.
 */
class ZenithPolySynthSound : public juce::SynthesiserSound
{
public:
    ZenithPolySynthSound() = default;

    bool appliesToNote(int midiNoteNumber) override
    {
        juce::ignoreUnused(midiNoteNumber);
        return true;
    }

    bool appliesToChannel(int midiChannel) override
    {
        juce::ignoreUnused(midiChannel);
        return true;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthSound)
};

} // namespace instruments
} // namespace zenith
