/*
  ==============================================================================

    WindModel.h
    Created: [Date] Author: Claude AI
    Wind instrument physical modeling (clarinet, saxophone, flute)

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class WindModelVoice
{
public:
    // Embouchure types
    enum class Embouchure
    {
        Clarinet,    // Single reed instrument
        Saxophone,  // Single reed with conical bore
        Flute,      // Edgeblown instrument
        Brass       // Lip-vibrated instrument
    };

    WindModelVoice();
    ~WindModelVoice();

    // Voice lifecycle
    void noteOn(float frequency, float velocity, float breathPressure);
    void noteOff();
    void reset();

    // Audio processing
    void process(juce::AudioBuffer<float>& buffer, int numSamples);
    bool isActive() const;

    // Physical parameters
    void setBreathPressure(float pressure);        // 0-1, excitation pressure
    void setEmbouchureType(Embouchure type);
    void setVibratoDepth(float depth);            // Breath vibrato amount
    void setVibratoRate(float rate);              // Vibrato frequency (Hz)
    void setToneColor(float color);               // Brightness/darkness
    void setGrowl(float growl);                   // Brass growl/undertone
    void setReedStiffness(float stiffness);       // Reed/lip stiffness
    void setReedOpening(float opening);           // Reed/lip opening amount
    void setBoreLength(float length);             // Instrument tube length
    void setBoreDiameter(float diameter);         // Instrument tube diameter

    // Breath and articulation
    void setBreathNoise(float amount);            // Breath noise component
    void setTonguingPosition(float position);     // Tongue position for articulation
    void setTonguingHardness(float hardness);     // Tongue hardness for articulation

    // Advanced acoustics
    void setResonanceBoost(float boost);          // Formant boost
    void setResonanceFrequency(float frequency);  // Formant frequency
    void setNoiseFloor(float floor);              // Background noise level
    void setNonlinearity(float amount);           // Signal nonlinearity

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
    float getBreathAmount() const;

    // Voice age
    float getVoiceAge() const;
    void updateAge();

private:
    // Core parameters
    float frequency_;
    float velocity_;
    float breathPressure_;
    Embouchure embouchureType_;
    bool isActive_;

    // Physical parameters
    float vibratoDepth_;
    float vibratoRate_;
    float toneColor_;
    float growl_;
    float reedStiffness_;
    float reedOpening_;
    float boreLength_;
    float boreDiameter_;
    float breathNoise_;
    float tonguingPosition_;
    float tonguingHardness_;
    float resonanceBoost_;
    float resonanceFrequency_;
    float noiseFloor_;
    float nonlinearity_;

    // MPE parameters
    float pitchBend_;
    float pressure_;
    float timbre_;

    // Audio processing
    double sampleRate_;
    int bufferSize_;

    // Breathing and excitation
    float currentBreathPressure_;
    float vibratoPhase_;
    float breathNoise_;

    // Jet/reed model
    struct ReedModel
    {
        float stiffness;       // Reed/lip stiffness
        float opening;         // Reed/lip opening
        float velocity;        // Reed/lip velocity
        float force;          // Force on reed
    } reedModel_;

    // Bore resonance model
    struct BoreModel
    {
        std::vector<float> delayLine;    // Bore delay line
        int writeIndex;
        int readIndex;
        float resonanceGain;
        float damping;
    } boreModel_;

    // Formant filter
    std::unique_ptr<juce::dsp::IIR::Filter<float>> formantFilter_;
    std::unique_ptr<juce::dsp::IIR::Filter<float>> brightnessFilter_;
    std::unique_ptr<juce::dsp::IIR::Filter<float>> growlFilter_;

    // Noise generators
    juce::Random noiseGenerator_;
    juce::AudioBuffer<float> noiseBuffer_;

    // Nonlinearity processing
    std::unique_ptr<juce::dsp::WaveShaper<float>> nonlinearProcessor_;

    // Smoothed parameters
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBreath_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFrequency_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedReedOpening_;

    // Voice age tracking
    juce::uint64 voiceStartTime_;
    float voiceAge_;

    // Analysis data
    float rmsLevel_;
    float peakLevel_;
    float fundamentalFrequency_;

    // Private helper methods
    void updateReedModel();
    void updateBoreModel();
    void updateFormantFilter();
    void updateNonlinearity();
    void generateBreathNoise();
    void processReedExcitation(juce::AudioBuffer<float>& buffer, int numSamples);
    void processBoreResonance(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyVibrato(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyFormantFiltering(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyNonlinearity(juce::AudioBuffer<float>& buffer, int numSamples);

    // Utility methods
    float calculateReedForce() const;
    float calculateJetVelocity() const;
    float calculateResonanceResponse() const;
    float getTonguingImpulse() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindModelVoice)
};

} // namespace Zenith