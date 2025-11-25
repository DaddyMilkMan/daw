/*
  ==============================================================================

    MixerChannel.h
    Ported from: ZenithDAW-Native/Source/Audio/MixerChannel.h (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Mixer channel strip with EQ, dynamics, and send/return processing

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - No container changes needed (uses arrays/atomics)

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

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
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) [[maybe_unused]] override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    //==============================================================================
    // Input section
    void setInputGain(float gainInDb) [[maybe_unused]];
    float getInputGain() const { return inputGain.load(); }

    void setPhaseInvert(bool shouldInvert) [[maybe_unused]];
    bool isPhaseInverted() const { return phaseInvert.load(); }

    //==============================================================================
    // High-pass filter
    void setHighPassEnabled(bool enabled) [[maybe_unused]];
    bool isHighPassEnabled() const { return hpfEnabled.load(); }

    void setHighPassFrequency(float frequency) [[maybe_unused]];
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
    void setCompressorEnabled(bool enabled) [[maybe_unused]];
    bool isCompressorEnabled() const { return compressorEnabled.load(); }

    void setCompressorThreshold(float thresholdDb) [[maybe_unused]];
    float getCompressorThreshold() const { return compThreshold.load(); }

    void setCompressorRatio(float ratio) [[maybe_unused]];
    float getCompressorRatio() const { return compRatio.load(); }

    void setCompressorAttack(float attackMs) [[maybe_unused]];
    float getCompressorAttack() const { return compAttack.load(); }

    void setCompressorRelease(float releaseMs) [[maybe_unused]];
    float getCompressorRelease() const { return compRelease.load(); }

    void setCompressorMakeup(float makeupDb) [[maybe_unused]];
    float getCompressorMakeup() const { return compMakeup.load(); }

    float getGainReduction() const { return gainReduction.load(); }

    //==============================================================================
    // Send effects (4 aux sends)
    void setSendLevel(int sendIndex, float level) [[maybe_unused]];
    float getSendLevel(int sendIndex) const;

    void setSendPreFader(int sendIndex, bool preFader) [[maybe_unused]];
    bool isSendPreFader(int sendIndex) const;

    //==============================================================================
    // Output section
    void setVolume(float volume) [[maybe_unused]];
    float getVolume() const { return volume.load(); }

    void setPan(float pan) [[maybe_unused]];
    float getPan() const { return this->pan.load(); }

    void setMuted(bool shouldBeMuted) [[maybe_unused]];
    bool isMuted() const { return muted.load(); }

    void setSolo(bool shouldBeSolo) [[maybe_unused]];
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
    void updateMeters(const juce::AudioBuffer<float>& buffer, bool isInput) [[maybe_unused]];
    void updateFilterCoefficients();
    void updateCompressorCoefficients();

    float dbToGain(float db) const { return juce::Decibels::decibelsToGain(db); }
    float gainToDb(float gain) const { return juce::Decibels::gainToDecibels(gain); }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel)
};

} // namespace zenith

