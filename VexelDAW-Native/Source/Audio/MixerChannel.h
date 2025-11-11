/*
  ==============================================================================

    MixerChannel.h
    Created: 2025-11-11
    Author:  Vexel DAW

    Mixer channel strip with EQ, dynamics, and send/return processing

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Represents a mixer channel strip with professional signal processing.

    Each mixer channel provides:
    - Input gain
    - High-pass filter
    - 4-band parametric EQ
    - Compressor/Limiter
    - Send effects (up to 4 sends)
    - Pan and volume
    - Metering (input, output, gain reduction)

    All processing is lock-free and real-time safe.
*/
class MixerChannel : public juce::AudioSource,
                     public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    MixerChannel();
    ~MixerChannel() override;

    //==============================================================================
    // AudioSource interface
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    //==============================================================================
    // Input section
    void setInputGain(float gainInDb);
    float getInputGain() const { return inputGain.load(); }

    void setPhaseInvert(bool shouldInvert);
    bool isPhaseInverted() const { return phaseInvert.load(); }

    //==============================================================================
    // High-pass filter
    void setHighPassEnabled(bool enabled);
    bool isHighPassEnabled() const { return hpfEnabled.load(); }

    void setHighPassFrequency(float frequency);
    float getHighPassFrequency() const { return hpfFrequency.load(); }

    //==============================================================================
    // 4-Band Parametric EQ
    struct EQBand
    {
        std::atomic<bool> enabled{false};
        std::atomic<float> frequency{1000.0f};
        std::atomic<float> gain{0.0f};      // In dB
        std::atomic<float> q{0.707f};

        enum class Type { LowShelf, Peak, HighShelf };
        Type type = Type::Peak;
    };

    EQBand& getEQBand(int bandIndex);
    const EQBand& getEQBand(int bandIndex) const;

    //==============================================================================
    // Dynamics (Compressor)
    void setCompressorEnabled(bool enabled);
    bool isCompressorEnabled() const { return compressorEnabled.load(); }

    void setCompressorThreshold(float thresholdDb);
    float getCompressorThreshold() const { return compThreshold.load(); }

    void setCompressorRatio(float ratio);
    float getCompressorRatio() const { return compRatio.load(); }

    void setCompressorAttack(float attackMs);
    float getCompressorAttack() const { return compAttack.load(); }

    void setCompressorRelease(float releaseMs);
    float getCompressorRelease() const { return compRelease.load(); }

    void setCompressorMakeup(float makeupDb);
    float getCompressorMakeup() const { return compMakeup.load(); }

    float getGainReduction() const { return gainReduction.load(); }

    //==============================================================================
    // Send effects (4 aux sends)
    void setSendLevel(int sendIndex, float level);
    float getSendLevel(int sendIndex) const;

    void setSendPreFader(int sendIndex, bool preFader);
    bool isSendPreFader(int sendIndex) const;

    //==============================================================================
    // Output section
    void setVolume(float volume);
    float getVolume() const { return volume.load(); }

    void setPan(float pan);
    float getPan() const { return this->pan.load(); }

    void setMuted(bool shouldBeMuted);
    bool isMuted() const { return muted.load(); }

    void setSolo(bool shouldBeSolo);
    bool isSolo() const { return solo.load(); }

    //==============================================================================
    // Metering
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    float getInputPeak() const { return inputPeak.load(); }
    float getOutputPeak() const { return outputPeak.load(); }

    void resetPeaks();

    //==============================================================================
    // State management
    juce::ValueTree getState() const;
    void loadState(const juce::ValueTree& state);

private:
    //==============================================================================
    // Input section
    std::atomic<float> inputGain{0.0f};  // In dB
    std::atomic<bool> phaseInvert{false};

    //==============================================================================
    // High-pass filter
    std::atomic<bool> hpfEnabled{false};
    std::atomic<float> hpfFrequency{20.0f};
    juce::IIRFilter hpfFilterL, hpfFilterR;

    //==============================================================================
    // EQ section
    static constexpr int numEQBands = 4;
    EQBand eqBands[numEQBands];
    juce::IIRFilter eqFiltersL[numEQBands];
    juce::IIRFilter eqFiltersR[numEQBands];

    //==============================================================================
    // Dynamics section
    std::atomic<bool> compressorEnabled{false};
    std::atomic<float> compThreshold{-10.0f};
    std::atomic<float> compRatio{4.0f};
    std::atomic<float> compAttack{10.0f};
    std::atomic<float> compRelease{100.0f};
    std::atomic<float> compMakeup{0.0f};
    std::atomic<float> gainReduction{0.0f};

    // Compressor state
    float envelopeFollower = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    //==============================================================================
    // Send effects
    static constexpr int numSends = 4;
    std::atomic<float> sendLevels[numSends];
    std::atomic<bool> sendPreFader[numSends];

    //==============================================================================
    // Output section
    std::atomic<float> volume{0.8f};
    std::atomic<float> pan{0.0f};
    std::atomic<bool> muted{false};
    std::atomic<bool> solo{false};

    //==============================================================================
    // Metering
    std::atomic<float> inputLevel{0.0f};
    std::atomic<float> outputLevel{0.0f};
    std::atomic<float> inputPeak{0.0f};
    std::atomic<float> outputPeak{0.0f};

    //==============================================================================
    // Processing state
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    //==============================================================================
    // Helper methods
    void processInput(juce::AudioBuffer<float>& buffer);
    void processHighPass(juce::AudioBuffer<float>& buffer);
    void processEQ(juce::AudioBuffer<float>& buffer);
    void processCompressor(juce::AudioBuffer<float>& buffer);
    void processOutput(juce::AudioBuffer<float>& buffer);
    void updateMeters(const juce::AudioBuffer<float>& buffer, bool isInput);
    void updateFilterCoefficients();
    void updateCompressorCoefficients();

    float dbToGain(float db) const { return juce::Decibels::decibelsToGain(db); }
    float gainToDb(float gain) const { return juce::Decibels::gainToDecibels(gain); }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel)
};
