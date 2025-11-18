/*
  ==============================================================================

    ZenithPolySynth.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Multi-oscillator subtractive polyphonic synthesizer optimized for EDM/trap/future-bass.

    Features:
    - 2-3 oscillators with sine, saw, square, triangle, noise, and supersaw modes
    - Multimode filter (lowpass, bandpass, highpass) with resonance and drive
    - 2 ADSR envelopes (amplitude and modulation)
    - 2 LFOs with multiple targets
    - Unison/detune for supersaw
    - Glide (portamento)
    - RT-safe: all buffers preallocated, no locks in audio thread

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Instrument.h"
#include <array>

namespace zenith {

//==============================================================================
/**
    Waveform types for oscillators
*/
enum class OscillatorWaveform
{
    Sine = 0,
    Saw,
    Square,
    Triangle,
    Noise,
    Supersaw,
    NumWaveforms
};

/**
    Filter types
*/
enum class FilterType
{
    Lowpass = 0,
    Bandpass,
    Highpass,
    NumTypes
};

/**
    LFO target parameters
*/
enum class LFOTarget
{
    FilterCutoff = 0,
    Osc1Pitch,
    Osc2Pitch,
    Osc1Mix,
    Osc2Mix,
    NumTargets
};

//==============================================================================
/**
    Single oscillator with multiple waveforms and detune
*/
class ZenithOscillator
{
public:
    ZenithOscillator() = default;

    void setWaveform(OscillatorWaveform waveform) { waveform_ = waveform; }
    void setDetune(float detuneCents) { detuneCents_ = detuneCents; }
    void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
    void reset() { phase_ = 0.0; }

    /**
     * @brief Generate next sample
     * @param frequency Base frequency in Hz
     * @return Sample value in range [-1, 1]
     */
    float getNextSample(float frequency);

private:
    OscillatorWaveform waveform_ = OscillatorWaveform::Saw;
    double phase_ = 0.0;
    double sampleRate_ = 44100.0;
    float detuneCents_ = 0.0f;
    juce::Random random_;

    float processSine(float frequency);
    float processSaw(float frequency);
    float processSquare(float frequency);
    float processTriangle(float frequency);
    float processNoise();
};

//==============================================================================
/**
    Multimode filter with smoothed parameters
*/
class ZenithFilter
{
public:
    ZenithFilter() = default;

    void setType(FilterType type) { type_ = type; }
    void setSampleRate(double sampleRate);
    void setCutoff(float cutoffHz);
    void setResonance(float resonance);
    void setDrive(float drive) { drive_ = drive; }
    void reset();

    /**
     * @brief Process one sample
     * @param input Input sample
     * @return Filtered sample
     */
    float processSample(float input);

private:
    FilterType type_ = FilterType::Lowpass;
    double sampleRate_ = 44100.0;

    // Smoothed parameters to avoid zipper noise
    juce::SmoothedValue<float> cutoffSmoothed_;
    juce::SmoothedValue<float> resonanceSmoothed_;

    float drive_ = 1.0f;

    // State variables filter implementation
    float v0_ = 0.0f, v1_ = 0.0f, v2_ = 0.0f;
    float ic1eq_ = 0.0f, ic2eq_ = 0.0f;
};

//==============================================================================
/**
    Voice for ZenithPolySynth - RT-safe polyphonic voice
*/
class ZenithPolySynthVoice : public juce::SynthesiserVoice
{
public:
    ZenithPolySynthVoice();
    ~ZenithPolySynthVoice() override = default;

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override {}
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    //==========================================================================
    // Parameter setters (called from message thread or via atomic parameters)
    //==========================================================================
    void setOsc1Waveform(OscillatorWaveform waveform) { osc1_.setWaveform(waveform); }
    void setOsc2Waveform(OscillatorWaveform waveform) { osc2_.setWaveform(waveform); }
    void setOsc3Waveform(OscillatorWaveform waveform) { osc3_.setWaveform(waveform); }

    void setOsc1Detune(float cents) { osc1_.setDetune(cents); }
    void setOsc2Detune(float cents) { osc2_.setDetune(cents); }
    void setOsc3Detune(float cents) { osc3_.setDetune(cents); }

    void setOsc1Mix(float mix) { osc1Mix_ = mix; }
    void setOsc2Mix(float mix) { osc2Mix_ = mix; }
    void setOsc3Mix(float mix) { osc3Mix_ = mix; }

    void setUnisonVoices(int voices) { unisonVoices_ = juce::jlimit(1, 7, voices); }
    void setUnisonDetune(float cents) { unisonDetune_ = cents; }

    void setFilterType(FilterType type) { filter_.setType(type); }
    void setFilterCutoff(float cutoff) { filterCutoff_ = cutoff; }
    void setFilterResonance(float resonance) { filter_.setResonance(resonance); }
    void setFilterDrive(float drive) { filter_.setDrive(drive); }

    void setAmpEnvelope(float attack, float decay, float sustain, float release);
    void setModEnvelope(float attack, float decay, float sustain, float release);

    void setLFO1(float rate, float amount, LFOTarget target);
    void setLFO2(float rate, float amount, LFOTarget target);

    void setGlideTime(float glideTimeSeconds) { glideTime_ = glideTimeSeconds; }
    void setMonoMode(bool mono) { monoMode_ = mono; }

    void setSampleRate(double sampleRate);

private:
    //==========================================================================
    // Oscillators
    //==========================================================================
    ZenithOscillator osc1_, osc2_, osc3_;
    std::array<ZenithOscillator, 7> unisonOscillators_; // For supersaw unison

    float osc1Mix_ = 1.0f;
    float osc2Mix_ = 0.5f;
    float osc3Mix_ = 0.0f;

    int unisonVoices_ = 1;
    float unisonDetune_ = 10.0f; // cents

    //==========================================================================
    // Filter
    //==========================================================================
    ZenithFilter filter_;
    float filterCutoff_ = 2000.0f;
    float filterEnvAmount_ = 0.0f;

    //==========================================================================
    // Envelopes
    //==========================================================================
    juce::ADSR ampEnvelope_;
    juce::ADSR::Parameters ampEnvParams_;

    juce::ADSR modEnvelope_;
    juce::ADSR::Parameters modEnvParams_;

    //==========================================================================
    // LFOs
    //==========================================================================
    struct LFO
    {
        float phase = 0.0f;
        float rate = 5.0f;      // Hz
        float amount = 0.0f;
        LFOTarget target = LFOTarget::FilterCutoff;

        float getNextValue(double sampleRate)
        {
            float value = std::sin(phase * juce::MathConstants<float>::twoPi);
            phase += rate / static_cast<float>(sampleRate);
            if (phase >= 1.0f)
                phase -= 1.0f;
            return value * amount;
        }

        void reset() { phase = 0.0f; }
    };

    LFO lfo1_, lfo2_;

    //==========================================================================
    // Voice state
    //==========================================================================
    double sampleRate_ = 44100.0;
    int currentMidiNote_ = 0;
    float currentFrequency_ = 440.0f;
    float targetFrequency_ = 440.0f;
    float glideTime_ = 0.0f;
    bool monoMode_ = false;
    float velocity_ = 1.0f;

    //==========================================================================
    // Helper methods
    //==========================================================================
    void updateFrequency();
    float applyLFOs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthVoice)
};

//==============================================================================
/**
    Sound for ZenithPolySynth
*/
class ZenithPolySynthSound : public juce::SynthesiserSound
{
public:
    ZenithPolySynthSound() = default;
    ~ZenithPolySynthSound() override = default;

    bool appliesToNote(int midiNoteNumber) override { return true; }
    bool appliesToChannel(int midiChannel) override { return true; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthSound)
};

//==============================================================================
/**
    ZenithPolySynth AudioProcessor

    Main synthesizer processor that manages voices and parameters.
*/
class ZenithPolySynthProcessor : public juce::Synthesiser,
                                  public juce::AudioProcessor
{
public:
    //==========================================================================
    // Parameter indices - must match order in constructor
    //==========================================================================
    enum Parameters
    {
        // Oscillators
        Osc1Wave = 0,
        Osc1Detune,
        Osc1Mix,
        Osc2Wave,
        Osc2Detune,
        Osc2Mix,
        Osc3Wave,
        Osc3Detune,
        Osc3Mix,

        // Unison
        UnisonVoices,
        UnisonDetune,

        // Filter
        FilterType,
        FilterCutoff,
        FilterResonance,
        FilterDrive,

        // Amp Envelope
        AmpAttack,
        AmpDecay,
        AmpSustain,
        AmpRelease,

        // Mod Envelope
        ModAttack,
        ModDecay,
        ModSustain,
        ModRelease,

        // LFO 1
        LFO1Rate,
        LFO1Amount,
        LFO1Target,

        // LFO 2
        LFO2Rate,
        LFO2Amount,
        LFO2Target,

        // Global
        GlideTime,
        MonoMode,
        MasterGain,

        NumParameters
    };

    //==========================================================================
    ZenithPolySynthProcessor();
    ~ZenithPolySynthProcessor() override = default;

    //==========================================================================
    // AudioProcessor interface
    //==========================================================================
    const juce::String getName() const override { return "Zenith Poly Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    //==========================================================================
    // Update voices with current parameters (called from audio thread)
    //==========================================================================
    void updateVoiceParameters();

    juce::SmoothedValue<float> masterGainSmoothed_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthProcessor)
};

//==============================================================================
/**
    Zenith Poly Synth instrument wrapper

    Exposes the synthesizer with comprehensive metadata and presets.
*/
class ZenithPolySynth : public InstrumentBase
{
public:
    ZenithPolySynth();
    ~ZenithPolySynth() override = default;

    /**
     * @brief Create metadata for this instrument
     */
    static InstrumentMetadata createMetadata();

private:
    void registerPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynth)
};

} // namespace zenith
