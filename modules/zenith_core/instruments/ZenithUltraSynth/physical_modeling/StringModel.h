/*
  ==============================================================================

    StringModel.h
    Created: [Date] Author: Claude AI
    Physical string modeling with Karplus-Strong and modal synthesis

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class StringModelVoice
{
public:
    // Excitation types
    enum class Excitation
    {
        Pluck,  // Guitar string pluck
        Bow,    // Violin bowing
        Strike, // Piano hammer strike
        Blow    // Harp/plucked string with breath
    };

    StringModelVoice();
    ~StringModelVoice();

    // Voice lifecycle
    void noteOn(float frequency, float velocity, Excitation excitation);
    void noteOff();
    void reset();

    // Audio processing
    void process(juce::AudioBuffer<float>& buffer, int numSamples);
    bool isActive() const;

    // Physical parameters
    void setStringLength(float length);           // 0-1, string physical length
    void setStringDiameter(float diameter);       // String thickness (affects timbre)
    void setStringTension(float tension);         // Tension amount (affects pitch)
    void setDamping(float damping);               // Energy loss rate
    void setBrightness(float brightness);         // High-frequency content
    void setInharmonicity(float inharmonicity);   // Stiffness (piano-like behavior)
    void setBodyResonance(float resonance);       // Body feedback amount
    void setExcitationPosition(float position);  // 0-1, excitation position along string

    // Advanced parameters
    void setNumberOfModes(int modes);            // Number of resonant modes
    void setModeDecay(int modeIndex, float decay); // Decay time for specific mode
    void setModeFrequency(int modeIndex, float frequency); // Frequency offset for mode
    void setModeAmplitude(int modeIndex, float amplitude); // Amplitude of mode

    // Vibrato and modulation
    void setVibratoDepth(float depth);           // Vibrato amount
    void setVibratoRate(float rate);             // Vibrato frequency (Hz)
    void setVibratoPhase(float phase);           // Vibrato phase offset

    // MPE support
    void setPitchBend(float bendAmount);          // Pitch bend amount
    void setPressure(float pressure);            // Pressure sensitivity
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
    float frequency_;
    float velocity_;
    Excitation excitationType_;
    bool isActive_;

    // Physical parameters
    float stringLength_;
    float stringDiameter_;
    float stringTension_;
    float damping_;
    float brightness_;
    float inharmonicity_;
    float bodyResonance_;
    float excitationPosition_;

    // Vibrato parameters
    float vibratoDepth_;
    float vibratoRate_;
    float vibratoPhase_;
    float currentVibratoAmount_;

    // MPE parameters
    float pitchBend_;
    float pressure_;
    float timbre_;

    // Audio processing
    double sampleRate_;
    int bufferSize_;

    // Karplus-Strong delay line
    std::vector<float> delayLine_;
    int writeIndex_;
    int readIndex_;
    int delayLength_;

    // Modal synthesis
    struct Mode
    {
        float frequency;      // Modal frequency
        float amplitude;      // Modal amplitude
        float decay;          // Modal decay rate
        float phase;          // Current phase
        float targetAmplitude;
        float targetFrequency;
    };
    std::vector<Mode> modes_;

    // Filters for damping and brightness
    std::unique_ptr<juce::dsp::IIR::Filter<float>> dampingFilter_;
    std::unique_ptr<juce::dsp::IIR::Filter<float>> brightnessFilter_;

    // Buffers and processing
    juce::AudioBuffer<float> excitationBuffer_;
    juce::AudioBuffer<float> modalBuffer_;

    // Smoothed parameters
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFrequency_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmplitude_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDamping_;

    // Voice age tracking
    juce::uint64 voiceStartTime_;
    float voiceAge_;

    // Analysis data
    float rmsLevel_;
    float peakLevel_;

    // Private helper methods
    void updateDelayLength();
    void calculateModalFrequencies();
    void processKarplusStrong(juce::AudioBuffer<float>& buffer, int numSamples);
    void processModalSynthesis(juce::AudioBuffer<float>& buffer, int numSamples);
    void processExcitation(juce::AudioBuffer<float>& buffer, int numSamples);
    void updateVibrato();
    void applyMPEParameters();

    // Utility methods
    float calculateInharmonicFactor(int harmonic) const;
    float getExcitationSample(Excitation type, float position, float velocity);
    void updateFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringModelVoice)
};

} // namespace Zenith