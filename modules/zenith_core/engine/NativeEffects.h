/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../engine/EffectProcessor.h"
#include "NativeAutoTuneEffect.h"
#include "ZenithDeEsser.h"
#include "ZenithTransientShaper.h"
#include "ZenithVoiceChanger.h"
#include "ZenithChannelStrip.h"
#include "../effects/MultibandCompressor.h"
#include "../effects/ConvolutionReverb.h"
#include "../effects/LinearPhaseEQ.h"
#include "../effects/DynamicEQ.h"
#include "../effects/AlgorithmicReverb.h"
#include <memory>

namespace zenith {
namespace engine {

//==============================================================================
/**
 * Native De-Esser effect (wraps ZenithDeEsser as native effect)
 */
class NativeDeEsserEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeDeEsserEffect();
    ~NativeDeEsserEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "De-Esser"; }
    EffectType getType() const override { return EffectType::Dynamics; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // De-Esser parameters
    void setThreshold(float dB) { threshold_ = juce::jlimit(-60.0f, 0.0f, dB); }
    float getThreshold() const { return threshold_.load(); }

    void setFrequency(float Hz) { frequency_ = juce::jlimit(2000.0f, 10000.0f, Hz); }
    float getFrequency() const { return frequency_.load(); }

    void setAmount(float amount) { amount_ = juce::jlimit(0.0f, 1.0f, amount); }
    float getAmount() const { return amount_.load(); }

    void setListenMode(bool listen) { listenMode_ = listen; }
    bool isListenMode() const { return listenMode_.load(); }

private:
    //==============================================================================
    // Wrap the existing plugin-based effect
    std::unique_ptr<ZenithDeEsser> pluginEffect_;

    // Parameters (atomic for thread safety)
    std::atomic<float> threshold_{-20.0f};
    std::atomic<float> frequency_{5000.0f};
    std::atomic<float> amount_{0.5f};
    std::atomic<bool> listenMode_{false};

    juce::MidiBuffer dummyMidi_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeDeEsserEffect)
};

//==============================================================================
/**
 * Native Transient Shaper effect
 */
class NativeTransientShaperEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeTransientShaperEffect();
    ~NativeTransientShaperEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Transient Shaper"; }
    EffectType getType() const override { return EffectType::Dynamics; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Transient Shaper parameters
    void setAttackAmount(float amount);
    void setSustainAmount(float amount);
    void setLink(bool link);

private:
    //==============================================================================
    std::unique_ptr<ZenithTransientShaper> pluginEffect_;
    std::atomic<float> attackAmount_{0.5f};
    std::atomic<float> sustainAmount_{0.5f};
    std::atomic<bool> link_{true};
    juce::MidiBuffer dummyMidi_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeTransientShaperEffect)
};

//==============================================================================
/**
 * Native Voice Changer effect
 */
class NativeVoiceChangerEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeVoiceChangerEffect();
    ~NativeVoiceChangerEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Voice Changer"; }
    EffectType getType() const override { return EffectType::Other; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Voice Changer parameters
    enum class Algorithm {
        FormantShift,
        PitchShift,
        Robot,
        Telephone,
        Radio
    };

    void setAlgorithm(Algorithm algo);
    void setMix(float mix);

private:
    //==============================================================================
    std::unique_ptr<ZenithVoiceChanger> pluginEffect_;
    std::atomic<int> algorithm_{0};
    std::atomic<float> mix_{1.0f};
    juce::MidiBuffer dummyMidi_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeVoiceChangerEffect)
};

//==============================================================================
/**
 * Native Channel Strip effect
 */
class NativeChannelStripEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeChannelStripEffect();
    ~NativeChannelStripEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Channel Strip"; }
    EffectType getType() const override { return EffectType::Utility; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Channel Strip parameters
    void setInputGain(float dB);
    void setOutputGain(float dB);
    void setLowEq(float dB);
    void setMidEq(float dB);
    void setHighEq(float dB);

private:
    //==============================================================================
    std::unique_ptr<ZenithChannelStrip> pluginEffect_;
    std::atomic<float> inputGain_{0.0f};
    std::atomic<float> outputGain_{0.0f};
    std::atomic<float> lowEq_{0.0f};
    std::atomic<float> midEq_{0.0f};
    std::atomic<float> highEq_{0.0f};
    juce::MidiBuffer dummyMidi_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeChannelStripEffect)
};

//==============================================================================
/**
 * Native Multiband Compressor effect
 */
class NativeMultibandCompressorEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeMultibandCompressorEffect();
    ~NativeMultibandCompressorEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Multiband Compressor"; }
    EffectType getType() const override { return EffectType::Dynamics; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Crossover controls
    void setCrossoverFrequency(int index, float Hz);
    float getCrossoverFrequency(int index) const;

    // Per-band controls
    void setBandEnabled(int band, bool enabled);
    bool isBandEnabled(int band) const;

    void setBandSolo(int band, bool solo);
    bool isBandSolo(int band) const;

    void setBandMute(int band, bool mute);
    bool isBandMute(int band) const;

    void setThreshold(int band, float dB);
    float getThreshold(int band) const;

    void setRatio(int band, float ratio);
    float getRatio(int band) const;

    void setAttack(int band, float ms);
    float getAttack(int band) const;

    void setRelease(int band, float ms);
    float getRelease(int band) const;

    void setKnee(int band, float dB);
    float getKnee(int band) const;

    void setMakeupGain(int band, float dB);
    float getMakeupGain(int band) const;

    // Metering
    float getGainReduction(int band) const;
    float getInputLevel(int band) const;
    float getOutputLevel(int band) const;

    // Global controls
    void setWetDryMix(float mix);
    float getWetDryMix() const;

    void setOutputGain(float dB);
    float getOutputGain() const;

    void setBandsLinked(bool linked);
    bool areBandsLinked() const;

    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

private:
    //==============================================================================
    // Direct DSP implementation (no plugin wrapper needed)
    std::unique_ptr<effects::MultibandCompressor> multibandCompressor_;

    juce::MidiBuffer dummyMidi_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeMultibandCompressorEffect)
};

//==============================================================================
/**
 * Native Convolution Reverb effect
 */
class NativeConvolutionReverbEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeConvolutionReverbEffect();
    ~NativeConvolutionReverbEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Convolution Reverb"; }
    EffectType getType() const override { return EffectType::Reverb; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Impulse Response management
    bool loadImpulseResponse(const juce::File& file);
    bool loadImpulseResponse(const juce::String& builtinName);
    juce::String getCurrentImpulseName() const;
    float getCurrentImpulseLengthSeconds() const;

    // Controls
    void setPreDelay(float ms);
    float getPreDelay() const;

    void setDecayTime(float percentage);
    float getDecayTime() const;

    void setSize(float percentage);
    float getSize() const;

    void setDensity(float percentage);
    float getDensity() const;

    void setHighCutDamping(float Hz);
    float getHighCutDamping() const;

    void setLowCutDamping(float Hz);
    float getLowCutDamping() const;

    void setEarlyReflectionsMix(float mix);
    float getEarlyReflectionsMix() const;

    void setWetMix(float mix);
    float getWetMix() const;

    void setStereoWidth(float width);
    float getStereoWidth() const;

    void setReverseEnabled(bool enabled);
    bool isReverseEnabled() const;

    // EQ for reverb tail
    void setEQLow(float dB);
    float getEQLow() const;

    void setEQMid(float dB);
    float getEQMid() const;

    void setEQHigh(float dB);
    float getEQHigh() const;

    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();
    static juce::StringArray getBuiltinImpulseNames();

private:
    //==============================================================================
    // Direct DSP implementation (no plugin wrapper needed)
    std::unique_ptr<effects::ConvolutionReverb> convolutionReverb_;

    juce::MidiBuffer dummyMidi_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeConvolutionReverbEffect)
};

//==============================================================================
/**
 * Native Linear Phase EQ effect
 */
class NativeLinearPhaseEQEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeLinearPhaseEQEffect();
    ~NativeLinearPhaseEQEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Linear Phase EQ"; }
    EffectType getType() const override { return EffectType::Filter; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Phase mode
    enum class PhaseMode { Linear, Minimum };
    void setPhaseMode(PhaseMode mode);
    PhaseMode getPhaseMode() const;

    void setFilterOrder(int order);

    // Band controls (up to 16 bands)
    static constexpr int maxBands = 16;
    void setNumBands(int num);
    void setBandType(int index, effects::LinearPhaseEQ::FilterType type);
    void setBandFrequency(int index, float frequency);
    void setBandGain(int index, float gain);
    void setBandQ(int index, float q);
    void setBandEnabled(int index, bool enabled);
    void setBandSolo(int index, bool solo);

    // Spectrum analyzer
    void setSpectrumAnalyzerEnabled(bool enabled);
    bool isSpectrumAnalyzerEnabled() const;
    const float* getSpectrumData() const;
    int getSpectrumSize() const;

    // Match EQ
    void startMatchEQ();
    void stopMatchEQ();
    bool isMatchingEQ() const;

    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

private:
    //==============================================================================
    std::unique_ptr<effects::LinearPhaseEQ> linearPhaseEQ_;

    juce::MidiBuffer dummyMidi_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeLinearPhaseEQEffect)
};

//==============================================================================
/**
 * Native Dynamic EQ effect
 */
class NativeDynamicEQEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeDynamicEQEffect();
    ~NativeDynamicEQEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Dynamic EQ"; }
    EffectType getType() const override { return EffectType::Filter; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    static constexpr int maxBands = 8;

    void setNumBands(int num);
    void setBandFrequency(int band, float frequency);
    void setBandQ(int band, float q);
    void setBandEnabled(int band, bool enabled);
    void setBandSolo(int band, bool solo);

    void setThreshold(int band, float dB);
    void setRatio(int band, float ratio);
    void setAttack(int band, float ms);
    void setRelease(int band, float ms);
    void setKnee(int band, float dB);
    void setMakeupGain(int band, float dB);

    void setExternalSidechain(int band, bool enabled);
    void setSidechainChannel(int band, int channel);
    void setAutoThresholdEnabled(int band, bool enabled);
    void learnThreshold(int band);

    float getGainReduction(int band) const;
    float getInputLevel(int band) const;
    float getOutputLevel(int band) const;

    void setBandsLinked(bool linked);
    void setWetDryMix(float mix);
    void setOutputGain(float dB);

    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

private:
    //==============================================================================
    std::unique_ptr<effects::DynamicEQ> dynamicEQ_;

    juce::MidiBuffer dummyMidi_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeDynamicEQEffect)
};

//==============================================================================
/**
 * Native Algorithmic Reverb effect
 */
class NativeAlgorithmicReverbEffect : public EffectProcessor
{
public:
    //==============================================================================
    NativeAlgorithmicReverbEffect();
    ~NativeAlgorithmicReverbEffect() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Algorithmic Reverb"; }
    EffectType getType() const override { return EffectType::Reverb; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Reverb parameters
    void setRoomSize(float size);
    float getRoomSize() const { return roomSize_.load(); }

    void setDamping(float damping);
    float getDamping() const { return damping_.load(); }

    void setWetLevel(float wet);
    float getWetLevel() const { return wetLevel_.load(); }

    void setDecayTime(float decay);
    float getDecayTime() const { return decayTime_.load(); }

    void setPreDelay(float predelay);
    float getPreDelay() const { return preDelay_.load(); }

    void setDiffusion(float diffusion);
    float getDiffusion() const { return diffusion_.load(); }

    void setModulation(float mod);
    float getModulation() const { return modulation_.load(); }

private:
    //==============================================================================
    std::unique_ptr<AlgorithmicReverb> algorithmicReverb_;

    // Parameters (atomic for thread safety)
    std::atomic<float> roomSize_{0.5f};
    std::atomic<float> damping_{0.5f};
    std::atomic<float> wetLevel_{0.3f};
    std::atomic<float> decayTime_{2.0f};
    std::atomic<float> preDelay_{0.02f};
    std::atomic<float> diffusion_{0.5f};
    std::atomic<float> modulation_{0.1f};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeAlgorithmicReverbEffect)
};

//==============================================================================
/**
 * Factory for creating native effects
 */
class NativeEffectFactory
{
public:
    //==============================================================================
    enum class EffectType
    {
        AutoTune,
        DeEsser,
        TransientShaper,
        VoiceChanger,
        ChannelStrip,
        MultibandCompressor,
        ConvolutionReverb,
        LinearPhaseEQ,
        DynamicEQ,
        AlgorithmicReverb
    };

    //==============================================================================
    static std::unique_ptr<EffectProcessor> create(EffectType type);

    //==============================================================================
    // Helper methods for specific effects
    static std::unique_ptr<NativeAutoTuneEffect> createAutoTune();
    static std::unique_ptr<NativeDeEsserEffect> createDeEsser();
    static std::unique_ptr<NativeTransientShaperEffect> createTransientShaper();
    static std::unique_ptr<NativeVoiceChangerEffect> createVoiceChanger();
    static std::unique_ptr<NativeChannelStripEffect> createChannelStrip();
    static std::unique_ptr<NativeMultibandCompressorEffect> createMultibandCompressor();
    static std::unique_ptr<NativeConvolutionReverbEffect> createConvolutionReverb();
    static std::unique_ptr<NativeLinearPhaseEQEffect> createLinearPhaseEQ();
    static std::unique_ptr<NativeDynamicEQEffect> createDynamicEQ();
    static std::unique_ptr<NativeAlgorithmicReverbEffect> createAlgorithmicReverb();
};

} // namespace engine
} // namespace zenith
