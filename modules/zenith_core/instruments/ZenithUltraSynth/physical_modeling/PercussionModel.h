/*
  ==============================================================================

    PercussionModel.h
    Created: [Date] Author: Claude AI
    Modal synthesis for drums and percussion instruments

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class PercussionModelVoice
{
public:
    // Drum types
    enum class DrumType
    {
        Kick,           // Kick drum
        Snare,          // Snare drum
        Tom,            // Tom drum
        Hihat,          // Hi-hat (closed/open)
        Cymbal,         // Crash/ride cymbal
        Percussion,     // Various percussion (congas, bongos, etc.)
        Triangle,       // Triangle bell
        Chimes,         // Tubular chimes
        Taiko           // Taiko drum
    };

    PercussionModelVoice();
    ~PercussionModelVoice();

    // Voice lifecycle
    void noteOn(float velocity, DrumType type);
    void noteOff();
    void reset();

    // Audio processing
    void process(juce::AudioBuffer<float>& buffer, int numSamples);
    bool isActive() const;

    // Physical parameters
    void setDecay(float decay);                   // Envelope decay time
    void setTone(float tone);                     // Pitch/timbre brightness
    void setSnares(float snares);                 // Snare wire rattle amount
    void setCymbalComplexity(float complexity);   // Cymbal mode density
    void setHeadType(float type);                 // Drum head stiffness
    void setShellSize(float size);                 // Drum shell size
    void setDrumPosition(float position);         // 3D position (for stereo imaging)
    void setStickHardness(float hardness);        // Drumstick/beater hardness
    void setStickPosition(float position);         // Strike position on drum
    void setResonance(float resonance);           // Drum shell resonance

    // Advanced parameters
    void setNumberOfModes(int modes);            // Number of resonant modes
    void setModeFrequency(int modeIndex, float frequency); // Frequency of specific mode
    void setModeDamping(int modeIndex, float damping);   // Damping of specific mode
    void setModeAmplitude(int modeIndex, float amplitude); // Amplitude of specific mode
    void setModeTuning(int modeIndex, float tuning);      // Fine tuning of mode frequency

    // Envelope control
    void setAttack(float attack);                 // Attack time
    void setDecayShape(float shape);              // Decay curve shape
    void setSustain(float sustain);              // Sustain level
    void setRelease(float release);               // Release time

    // Noise and texture
    void setNoiseAmount(float amount);            // Noise component amount
    void setNoiseColor(float color);              // Noise brightness
    void setMetallicResonance(float amount);      // Metallic resonance amount
    void setAirResonance(float amount);          // Air cavity resonance

    // MPE support
    void setPitchBend(float bendAmount);          // Pitch bend amount
    void setPressure(float pressure);              // Pressure sensitivity
    void setTimbre(float timbre);                // Timbre modulation

    // Parameter smoothing
    void setSmoothingTime(float timeMs);          // Parameter smoothing time
    float getSmoothingTime() const;

    // Analysis and monitoring
    float getRmsLevel() const;
    float getPeakLevel() const;
    float getFundamentalFrequency() const;
    juce::Array<float> getModeAmplitudes() const;

    // Voice age
    float getVoiceAge() const;
    void updateAge();

private:
    // Core parameters
    float velocity_;
    DrumType drumType_;
    bool isActive_;

    // Physical parameters
    float decay_;
    float tone_;
    float snares_;
    float cymbalComplexity_;
    float headType_;
    float shellSize_;
    float drumPosition_;
    float stickHardness_;
    float stickPosition_;
    float resonance_;
    float noiseAmount_;
    float noiseColor_;
    float metallicResonance_;
    float airResonance_;

    // Envelope parameters
    float attack_;
    float decayShape_;
    float sustain_;
    float release_;

    // MPE parameters
    float pitchBend_;
    float pressure_;
    float timbre_;

    // Audio processing
    double sampleRate_;
    int bufferSize_;

    // Modal synthesis
    struct Mode
    {
        float frequency;      // Modal frequency
        float amplitude;      // Modal amplitude
        float damping;        // Modal decay rate
        float phase;          // Current phase
        float phaseIncrement; // Phase increment
        float targetAmplitude;
        float targetFrequency;
        float targetDamping;
    };
    std::vector<Mode> modes_;

    // Cymbal-specific processing
    struct MetalMode
    {
        float frequency;
        float amplitude;
        float decay;
        float phase;
    };
    std::vector<MetalMode> metalModes_;

    // Envelope generator
    struct Envelope
    {
        float value;
        float attackTime;
        float decayTime;
        float sustainLevel;
        float releaseTime;
        float state;  // 0: attack, 1: decay, 2: sustain, 3: release
        float phase;
    } envelope_;

    // Noise generators
    juce::Random noiseGenerator_;
    juce::AudioBuffer<float> noiseBuffer_;

    // Excitation signal
    juce::AudioBuffer<float> excitationBuffer_;

    // Filters
    std::unique_ptr<juce::dsp::IIR::Filter<float>> toneFilter_;
    std::unique_ptr<juce::dsp::IIR::Filter<float>> noiseFilter_;
    std::unique_ptr<juce::dsp::IIR::Filter<float>> metallicFilter_;

    // Smoothed parameters
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmplitude_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFrequency_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDecay_;

    // Voice age tracking
    juce::uint64 voiceStartTime_;
    float voiceAge_;

    // Analysis data
    float rmsLevel_;
    float peakLevel_;
    float fundamentalFrequency_;

    // Private helper methods
    void initializeModes();
    void updateModeParameters();
    void processModalSynthesis(juce::AudioBuffer<float>& buffer, int numSamples);
    void processMetalModes(juce::AudioBuffer<float>& buffer, int numSamples);
    void processExcitation(juce::AudioBuffer<float>& buffer, int numSamples);
    void processNoise(juce::AudioBuffer<float>& buffer, int numSamples);
    void processSnares(juce::AudioBuffer<float>& buffer, int numSamples);
    void updateEnvelope();
    void applyFilters(juce::AudioBuffer<float>& buffer, int numSamples);

    // Utility methods
    float calculateStrikeNoise() const;
    float calculateModalResponse(float frequency, float amplitude) const;
    float getDrumEnvelopeValue() const;
    void updateFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PercussionModelVoice)
};

} // namespace Zenith