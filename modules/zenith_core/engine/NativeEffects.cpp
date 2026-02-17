/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "NativeEffects.h"
#include <algorithm>

namespace zenith {
namespace engine {

//==============================================================================
// NativeDeEsserEffect Implementation
//==============================================================================
NativeDeEsserEffect::NativeDeEsserEffect()
{
    // Create the plugin-based effect
    pluginEffect_ = std::make_unique<ZenithDeEsser>();
}

NativeDeEsserEffect::~NativeDeEsserEffect()
{
}

void NativeDeEsserEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (pluginEffect_)
    {
        pluginEffect_->setPlayConfig(juce::PluginHost::getDefaultBusLayout());
        pluginEffect_->prepareToPlay(sampleRate, maxSamplesPerBlock);
    }
}

void NativeDeEsserEffect::reset()
{
    if (pluginEffect_)
        pluginEffect_->releaseResources();
}

void NativeDeEsserEffect::process(juce::AudioBuffer<float>& buffer,
                                   const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;

    if (isBypassed() || !pluginEffect_)
        return;

    // Update effect parameters from our atomic values
    // (The effect reads from its own APVTS, but we could sync if needed)

    // Process through the plugin
    pluginEffect_->processBlock(buffer, dummyMidi_);
}

juce::ValueTree NativeDeEsserEffect::getState() const
{
    juce::ValueTree state("DeEsserState");
    state.setProperty("threshold", threshold_.load(), nullptr);
    state.setProperty("frequency", frequency_.load(), nullptr);
    state.setProperty("amount", amount_.load(), nullptr);
    state.setProperty("listenMode", listenMode_.load(), nullptr);
    return state;
}

void NativeDeEsserEffect::setState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    threshold_ = state.getProperty("threshold", -20.0f);
    frequency_ = state.getProperty("frequency", 5000.0f);
    amount_ = state.getProperty("amount", 0.5f);
    listenMode_ = state.getProperty("listenMode", false);
}

//==============================================================================
// NativeTransientShaperEffect Implementation
//==============================================================================
NativeTransientShaperEffect::NativeTransientShaperEffect()
{
    pluginEffect_ = std::make_unique<ZenithTransientShaper>();
}

NativeTransientShaperEffect::~NativeTransientShaperEffect()
{
}

void NativeTransientShaperEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (pluginEffect_)
    {
        pluginEffect_->setPlayConfig(juce::PluginHost::getDefaultBusLayout());
        pluginEffect_->prepareToPlay(sampleRate, maxSamplesPerBlock);
    }
}

void NativeTransientShaperEffect::reset()
{
    if (pluginEffect_)
        pluginEffect_->releaseResources();
}

void NativeTransientShaperEffect::process(juce::AudioBuffer<float>& buffer,
                                          const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;

    if (isBypassed() || !pluginEffect_)
        return;

    pluginEffect_->processBlock(buffer, dummyMidi_);
}

juce::ValueTree NativeTransientShaperEffect::getState() const
{
    juce::ValueTree state("TransientShaperState");
    state.setProperty("attackAmount", attackAmount_.load(), nullptr);
    state.setProperty("sustainAmount", sustainAmount_.load(), nullptr);
    state.setProperty("link", link_.load(), nullptr);
    return state;
}

void NativeTransientShaperEffect::setState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    attackAmount_ = state.getProperty("attackAmount", 0.5f);
    sustainAmount_ = state.getProperty("sustainAmount", 0.5f);
    link_ = state.getProperty("link", true);
}

void NativeTransientShaperEffect::setAttackAmount(float amount)
{
    attackAmount_ = juce::jlimit(0.0f, 1.0f, amount);
}

void NativeTransientShaperEffect::setSustainAmount(float amount)
{
    sustainAmount_ = juce::jlimit(0.0f, 1.0f, amount);
}

void NativeTransientShaperEffect::setLink(bool link)
{
    link_ = link;
}

//==============================================================================
// NativeVoiceChangerEffect Implementation
//==============================================================================
NativeVoiceChangerEffect::NativeVoiceChangerEffect()
{
    pluginEffect_ = std::make_unique<ZenithVoiceChanger>();
}

NativeVoiceChangerEffect::~NativeVoiceChangerEffect()
{
}

void NativeVoiceChangerEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (pluginEffect_)
    {
        pluginEffect_->setPlayConfig(juce::PluginHost::getDefaultBusLayout());
        pluginEffect_->prepareToPlay(sampleRate, maxSamplesPerBlock);
    }
}

void NativeVoiceChangerEffect::reset()
{
    if (pluginEffect_)
        pluginEffect_->releaseResources();
}

void NativeVoiceChangerEffect::process(juce::AudioBuffer<float>& buffer,
                                       const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;

    if (isBypassed() || !pluginEffect_)
        return;

    pluginEffect_->processBlock(buffer, dummyMidi_);
}

juce::ValueTree NativeVoiceChangerEffect::getState() const
{
    juce::ValueTree state("VoiceChangerState");
    state.setProperty("algorithm", algorithm_.load(), nullptr);
    state.setProperty("mix", mix_.load(), nullptr);
    return state;
}

void NativeVoiceChangerEffect::setState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    algorithm_ = state.getProperty("algorithm", 0);
    mix_ = state.getProperty("mix", 1.0f);
}

void NativeVoiceChangerEffect::setAlgorithm(Algorithm algo)
{
    algorithm_ = static_cast<int>(algo);
}

void NativeVoiceChangerEffect::setMix(float mix)
{
    mix_ = juce::jlimit(0.0f, 1.0f, mix);
}

//==============================================================================
// NativeChannelStripEffect Implementation
//==============================================================================
NativeChannelStripEffect::NativeChannelStripEffect()
{
    pluginEffect_ = std::make_unique<ZenithChannelStrip>();
}

NativeChannelStripEffect::~NativeChannelStripEffect()
{
}

void NativeChannelStripEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (pluginEffect_)
    {
        pluginEffect_->setPlayConfig(juce::PluginHost::getDefaultBusLayout());
        pluginEffect_->prepareToPlay(sampleRate, maxSamplesPerBlock);
    }
}

void NativeChannelStripEffect::reset()
{
    if (pluginEffect_)
        pluginEffect_->releaseResources();
}

void NativeChannelStripEffect::process(juce::AudioBuffer<float>& buffer,
                                        const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;

    if (isBypassed() || !pluginEffect_)
        return;

    pluginEffect_->processBlock(buffer, dummyMidi_);
}

juce::ValueTree NativeChannelStripEffect::getState() const
{
    juce::ValueTree state("ChannelStripState");
    state.setProperty("inputGain", inputGain_.load(), nullptr);
    state.setProperty("outputGain", outputGain_.load(), nullptr);
    state.setProperty("lowEq", lowEq_.load(), nullptr);
    state.setProperty("midEq", midEq_.load(), nullptr);
    state.setProperty("highEq", highEq_.load(), nullptr);
    return state;
}

void NativeChannelStripEffect::setState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    inputGain_ = state.getProperty("inputGain", 0.0f);
    outputGain_ = state.getProperty("outputGain", 0.0f);
    lowEq_ = state.getProperty("lowEq", 0.0f);
    midEq_ = state.getProperty("midEq", 0.0f);
    highEq_ = state.getProperty("highEq", 0.0f);
}

void NativeChannelStripEffect::setInputGain(float dB)
{
    inputGain_ = juce::jlimit(-60.0f, 12.0f, dB);
}

void NativeChannelStripEffect::setOutputGain(float dB)
{
    outputGain_ = juce::jlimit(-60.0f, 12.0f, dB);
}

void NativeChannelStripEffect::setLowEq(float dB)
{
    lowEq_ = juce::jlimit(-12.0f, 12.0f, dB);
}

void NativeChannelStripEffect::setMidEq(float dB)
{
    midEq_ = juce::jlimit(-12.0f, 12.0f, dB);
}

void NativeChannelStripEffect::setHighEq(float dB)
{
    highEq_ = juce::jlimit(-12.0f, 12.0f, dB);
}

//==============================================================================
// NativeMultibandCompressorEffect Implementation
//==============================================================================
NativeMultibandCompressorEffect::NativeMultibandCompressorEffect()
{
    // Create the multiband compressor (direct DSP, no plugin wrapper)
    multibandCompressor_ = std::make_unique<effects::MultibandCompressor>();
}

NativeMultibandCompressorEffect::~NativeMultibandCompressorEffect()
{
}

void NativeMultibandCompressorEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (multibandCompressor_)
        multibandCompressor_->prepare(sampleRate, maxSamplesPerBlock);
}

void NativeMultibandCompressorEffect::reset()
{
    if (multibandCompressor_)
        multibandCompressor_->reset();
}

void NativeMultibandCompressorEffect::process(juce::AudioBuffer<float>& buffer,
                                              const juce::AudioBuffer<float>* sidechain)
{
    if (isBypassed() || !multibandCompressor_)
        return;

    multibandCompressor_->process(buffer, sidechain);
}

juce::ValueTree NativeMultibandCompressorEffect::getState() const
{
    if (multibandCompressor_)
        return multibandCompressor_->getState();
    return juce::ValueTree("MultibandCompressorState");
}

void NativeMultibandCompressorEffect::setState(const juce::ValueTree& state)
{
    if (multibandCompressor_)
        multibandCompressor_->setState(state);
}

void NativeMultibandCompressorEffect::setCrossoverFrequency(int index, float Hz)
{
    if (multibandCompressor_)
        multibandCompressor_->setCrossoverFrequency(index, Hz);
}

float NativeMultibandCompressorEffect::getCrossoverFrequency(int index) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getCrossoverFrequency(index);
    return 0.0f;
}

void NativeMultibandCompressorEffect::setBandEnabled(int band, bool enabled)
{
    if (multibandCompressor_)
        multibandCompressor_->setBandEnabled(band, enabled);
}

bool NativeMultibandCompressorEffect::isBandEnabled(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->isBandEnabled(band);
    return false;
}

void NativeMultibandCompressorEffect::setBandSolo(int band, bool solo)
{
    if (multibandCompressor_)
        multibandCompressor_->setBandSolo(band, solo);
}

bool NativeMultibandCompressorEffect::isBandSolo(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->isBandSolo(band);
    return false;
}

void NativeMultibandCompressorEffect::setBandMute(int band, bool mute)
{
    if (multibandCompressor_)
        multibandCompressor_->setBandMute(band, mute);
}

bool NativeMultibandCompressorEffect::isBandMute(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->isBandMute(band);
    return false;
}

void NativeMultibandCompressorEffect::setThreshold(int band, float dB)
{
    if (multibandCompressor_)
        multibandCompressor_->setThreshold(band, dB);
}

float NativeMultibandCompressorEffect::getThreshold(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getThreshold(band);
    return 0.0f;
}

void NativeMultibandCompressorEffect::setRatio(int band, float ratio)
{
    if (multibandCompressor_)
        multibandCompressor_->setRatio(band, ratio);
}

float NativeMultibandCompressorEffect::getRatio(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getRatio(band);
    return 1.0f;
}

void NativeMultibandCompressorEffect::setAttack(int band, float ms)
{
    if (multibandCompressor_)
        multibandCompressor_->setAttack(band, ms);
}

float NativeMultibandCompressorEffect::getAttack(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getAttack(band);
    return 10.0f;
}

void NativeMultibandCompressorEffect::setRelease(int band, float ms)
{
    if (multibandCompressor_)
        multibandCompressor_->setRelease(band, ms);
}

float NativeMultibandCompressorEffect::getRelease(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getRelease(band);
    return 100.0f;
}

void NativeMultibandCompressorEffect::setKnee(int band, float dB)
{
    if (multibandCompressor_)
        multibandCompressor_->setKnee(band, dB);
}

float NativeMultibandCompressorEffect::getKnee(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getKnee(band);
    return 0.0f;
}

void NativeMultibandCompressorEffect::setMakeupGain(int band, float dB)
{
    if (multibandCompressor_)
        multibandCompressor_->setMakeupGain(band, dB);
}

float NativeMultibandCompressorEffect::getMakeupGain(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getMakeupGain(band);
    return 0.0f;
}

float NativeMultibandCompressorEffect::getGainReduction(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getGainReduction(band);
    return 0.0f;
}

float NativeMultibandCompressorEffect::getInputLevel(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getInputLevel(band);
    return -100.0f;
}

float NativeMultibandCompressorEffect::getOutputLevel(int band) const
{
    if (multibandCompressor_)
        return multibandCompressor_->getOutputLevel(band);
    return -100.0f;
}

void NativeMultibandCompressorEffect::setWetDryMix(float mix)
{
    if (multibandCompressor_)
        multibandCompressor_->setWetDryMix(mix);
}

float NativeMultibandCompressorEffect::getWetDryMix() const
{
    if (multibandCompressor_)
        return multibandCompressor_->getWetDryMix();
    return 1.0f;
}

void NativeMultibandCompressorEffect::setOutputGain(float dB)
{
    if (multibandCompressor_)
        multibandCompressor_->setOutputGain(dB);
}

float NativeMultibandCompressorEffect::getOutputGain() const
{
    if (multibandCompressor_)
        return multibandCompressor_->getOutputGain();
    return 0.0f;
}

void NativeMultibandCompressorEffect::setBandsLinked(bool linked)
{
    if (multibandCompressor_)
        multibandCompressor_->setBandsLinked(linked);
}

bool NativeMultibandCompressorEffect::areBandsLinked() const
{
    if (multibandCompressor_)
        return multibandCompressor_->areBandsLinked();
    return false;
}

void NativeMultibandCompressorEffect::loadPreset(const juce::String& presetName)
{
    if (multibandCompressor_)
        multibandCompressor_->loadPreset(presetName);
}

juce::StringArray NativeMultibandCompressorEffect::getPresetNames()
{
    return effects::MultibandCompressor::getPresetNames();
}

//==============================================================================
// NativeConvolutionReverbEffect Implementation
//==============================================================================
NativeConvolutionReverbEffect::NativeConvolutionReverbEffect()
{
    convolutionReverb_ = std::make_unique<effects::ConvolutionReverb>();
}

NativeConvolutionReverbEffect::~NativeConvolutionReverbEffect()
{
}

void NativeConvolutionReverbEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (convolutionReverb_)
        convolutionReverb_->prepare(sampleRate, maxSamplesPerBlock);
}

void NativeConvolutionReverbEffect::reset()
{
    if (convolutionReverb_)
        convolutionReverb_->reset();
}

void NativeConvolutionReverbEffect::process(juce::AudioBuffer<float>& buffer,
                                            const juce::AudioBuffer<float>* sidechain)
{
    if (isBypassed() || !convolutionReverb_)
        return;

    convolutionReverb_->process(buffer, sidechain);
}

juce::ValueTree NativeConvolutionReverbEffect::getState() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getState();
    return juce::ValueTree("ConvolutionReverbState");
}

void NativeConvolutionReverbEffect::setState(const juce::ValueTree& state)
{
    if (convolutionReverb_)
        convolutionReverb_->setState(state);
}

bool NativeConvolutionReverbEffect::loadImpulseResponse(const juce::File& file)
{
    if (convolutionReverb_)
        return convolutionReverb_->loadImpulseResponse(file);
    return false;
}

bool NativeConvolutionReverbEffect::loadImpulseResponse(const juce::String& builtinName)
{
    if (convolutionReverb_)
        return convolutionReverb_->loadImpulseResponse(builtinName);
    return false;
}

juce::String NativeConvolutionReverbEffect::getCurrentImpulseName() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getCurrentImpulseName();
    return "";
}

float NativeConvolutionReverbEffect::getCurrentImpulseLengthSeconds() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getCurrentImpulseLengthSeconds();
    return 0.0f;
}

void NativeConvolutionReverbEffect::setPreDelay(float ms)
{
    if (convolutionReverb_)
        convolutionReverb_->setPreDelay(ms);
}

float NativeConvolutionReverbEffect::getPreDelay() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getPreDelay();
    return 0.0f;
}

void NativeConvolutionReverbEffect::setDecayTime(float percentage)
{
    if (convolutionReverb_)
        convolutionReverb_->setDecayTime(percentage);
}

float NativeConvolutionReverbEffect::getDecayTime() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getDecayTime();
    return 100.0f;
}

void NativeConvolutionReverbEffect::setSize(float percentage)
{
    if (convolutionReverb_)
        convolutionReverb_->setSize(percentage);
}

float NativeConvolutionReverbEffect::getSize() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getSize();
    return 100.0f;
}

void NativeConvolutionReverbEffect::setDensity(float percentage)
{
    if (convolutionReverb_)
        convolutionReverb_->setDensity(percentage);
}

float NativeConvolutionReverbEffect::getDensity() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getDensity();
    return 100.0f;
}

void NativeConvolutionReverbEffect::setHighCutDamping(float Hz)
{
    if (convolutionReverb_)
        convolutionReverb_->setHighCutDamping(Hz);
}

float NativeConvolutionReverbEffect::getHighCutDamping() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getHighCutDamping();
    return 10000.0f;
}

void NativeConvolutionReverbEffect::setLowCutDamping(float Hz)
{
    if (convolutionReverb_)
        convolutionReverb_->setLowCutDamping(Hz);
}

float NativeConvolutionReverbEffect::getLowCutDamping() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getLowCutDamping();
    return 100.0f;
}

void NativeConvolutionReverbEffect::setEarlyReflectionsMix(float mix)
{
    if (convolutionReverb_)
        convolutionReverb_->setEarlyReflectionsMix(mix);
}

float NativeConvolutionReverbEffect::getEarlyReflectionsMix() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getEarlyReflectionsMix();
    return 0.3f;
}

void NativeConvolutionReverbEffect::setWetMix(float mix)
{
    if (convolutionReverb_)
        convolutionReverb_->setWetMix(mix);
}

float NativeConvolutionReverbEffect::getWetMix() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getWetMix();
    return 0.5f;
}

void NativeConvolutionReverbEffect::setStereoWidth(float width)
{
    if (convolutionReverb_)
        convolutionReverb_->setStereoWidth(width);
}

float NativeConvolutionReverbEffect::getStereoWidth() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getStereoWidth();
    return 1.0f;
}

void NativeConvolutionReverbEffect::setReverseEnabled(bool enabled)
{
    if (convolutionReverb_)
        convolutionReverb_->setReverseEnabled(enabled);
}

bool NativeConvolutionReverbEffect::isReverseEnabled() const
{
    if (convolutionReverb_)
        return convolutionReverb_->isReverseEnabled();
    return false;
}

void NativeConvolutionReverbEffect::setEQLow(float dB)
{
    if (convolutionReverb_)
        convolutionReverb_->setEQLow(dB);
}

float NativeConvolutionReverbEffect::getEQLow() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getEQLow();
    return 0.0f;
}

void NativeConvolutionReverbEffect::setEQMid(float dB)
{
    if (convolutionReverb_)
        convolutionReverb_->setEQMid(dB);
}

float NativeConvolutionReverbEffect::getEQMid() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getEQMid();
    return 0.0f;
}

void NativeConvolutionReverbEffect::setEQHigh(float dB)
{
    if (convolutionReverb_)
        convolutionReverb_->setEQHigh(dB);
}

float NativeConvolutionReverbEffect::getEQHigh() const
{
    if (convolutionReverb_)
        return convolutionReverb_->getEQHigh();
    return 0.0f;
}

void NativeConvolutionReverbEffect::loadPreset(const juce::String& presetName)
{
    if (convolutionReverb_)
        convolutionReverb_->loadPreset(presetName);
}

juce::StringArray NativeConvolutionReverbEffect::getPresetNames()
{
    return effects::ConvolutionReverb::getPresetNames();
}

juce::StringArray NativeConvolutionReverbEffect::getBuiltinImpulseNames()
{
    return effects::ConvolutionReverb::getBuiltinImpulseNames();
}

//==============================================================================
// NativeLinearPhaseEQEffect Implementation
//==============================================================================
NativeLinearPhaseEQEffect::NativeLinearPhaseEQEffect()
{
    linearPhaseEQ_ = std::make_unique<effects::LinearPhaseEQ>();
}

NativeLinearPhaseEQEffect::~NativeLinearPhaseEQEffect()
{
}

void NativeLinearPhaseEQEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (linearPhaseEQ_)
        linearPhaseEQ_->prepare(sampleRate, maxSamplesPerBlock);
}

void NativeLinearPhaseEQEffect::reset()
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->reset();
}

void NativeLinearPhaseEQEffect::process(juce::AudioBuffer<float>& buffer,
                                        const juce::AudioBuffer<float>* sidechain)
{
    if (isBypassed() || !linearPhaseEQ_)
        return;

    linearPhaseEQ_->process(buffer, sidechain);
}

juce::ValueTree NativeLinearPhaseEQEffect::getState() const
{
    if (linearPhaseEQ_)
        return linearPhaseEQ_->getState();
    return juce::ValueTree("LinearPhaseEQState");
}

void NativeLinearPhaseEQEffect::setState(const juce::ValueTree& state)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setState(state);
}

void NativeLinearPhaseEQEffect::setPhaseMode(PhaseMode mode)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setPhaseMode(static_cast<effects::LinearPhaseEQ::PhaseMode>(mode));
}

NativeLinearPhaseEQEffect::PhaseMode NativeLinearPhaseEQEffect::getPhaseMode() const
{
    if (linearPhaseEQ_)
        return static_cast<PhaseMode>(linearPhaseEQ_->getPhaseMode());
    return PhaseMode::Minimum;
}

void NativeLinearPhaseEQEffect::setFilterOrder(int order)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setFilterOrder(order);
}

void NativeLinearPhaseEQEffect::setNumBands(int num)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setNumBands(num);
}

void NativeLinearPhaseEQEffect::setBandType(int index, effects::LinearPhaseEQ::FilterType type)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setBandType(index, type);
}

void NativeLinearPhaseEQEffect::setBandFrequency(int index, float frequency)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setBandFrequency(index, frequency);
}

void NativeLinearPhaseEQEffect::setBandGain(int index, float gain)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setBandGain(index, gain);
}

void NativeLinearPhaseEQEffect::setBandQ(int index, float q)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setBandQ(index, q);
}

void NativeLinearPhaseEQEffect::setBandEnabled(int index, bool enabled)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setBandEnabled(index, enabled);
}

void NativeLinearPhaseEQEffect::setBandSolo(int index, bool solo)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setBandSolo(index, solo);
}

void NativeLinearPhaseEQEffect::setSpectrumAnalyzerEnabled(bool enabled)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->setSpectrumAnalyzerEnabled(enabled);
}

bool NativeLinearPhaseEQEffect::isSpectrumAnalyzerEnabled() const
{
    if (linearPhaseEQ_)
        return linearPhaseEQ_->isSpectrumAnalyzerEnabled();
    return false;
}

const float* NativeLinearPhaseEQEffect::getSpectrumData() const
{
    if (linearPhaseEQ_)
        return linearPhaseEQ_->getSpectrumData();
    return nullptr;
}

int NativeLinearPhaseEQEffect::getSpectrumSize() const
{
    if (linearPhaseEQ_)
        return linearPhaseEQ_->getSpectrumSize();
    return 0;
}

void NativeLinearPhaseEQEffect::startMatchEQ()
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->startMatchEQ();
}

void NativeLinearPhaseEQEffect::stopMatchEQ()
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->stopMatchEQ();
}

bool NativeLinearPhaseEQEffect::isMatchingEQ() const
{
    if (linearPhaseEQ_)
        return linearPhaseEQ_->isMatchingEQ();
    return false;
}

void NativeLinearPhaseEQEffect::loadPreset(const juce::String& presetName)
{
    if (linearPhaseEQ_)
        linearPhaseEQ_->loadPreset(presetName);
}

juce::StringArray NativeLinearPhaseEQEffect::getPresetNames()
{
    return effects::LinearPhaseEQ::getPresetNames();
}

//==============================================================================
// NativeDynamicEQEffect Implementation
//==============================================================================
NativeDynamicEQEffect::NativeDynamicEQEffect()
{
    dynamicEQ_ = std::make_unique<effects::DynamicEQ>();
}

NativeDynamicEQEffect::~NativeDynamicEQEffect()
{
}

void NativeDynamicEQEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (dynamicEQ_)
        dynamicEQ_->prepare(sampleRate, maxSamplesPerBlock);
}

void NativeDynamicEQEffect::reset()
{
    if (dynamicEQ_)
        dynamicEQ_->reset();
}

void NativeDynamicEQEffect::process(juce::AudioBuffer<float>& buffer,
                                    const juce::AudioBuffer<float>* sidechain)
{
    if (isBypassed() || !dynamicEQ_)
        return;

    dynamicEQ_->process(buffer, sidechain);
}

juce::ValueTree NativeDynamicEQEffect::getState() const
{
    if (dynamicEQ_)
        return dynamicEQ_->getState();
    return juce::ValueTree("DynamicEQState");
}

void NativeDynamicEQEffect::setState(const juce::ValueTree& state)
{
    if (dynamicEQ_)
        dynamicEQ_->setState(state);
}

void NativeDynamicEQEffect::setNumBands(int num)
{
    if (dynamicEQ_)
        dynamicEQ_->setNumBands(num);
}

void NativeDynamicEQEffect::setBandFrequency(int band, float frequency)
{
    if (dynamicEQ_)
        dynamicEQ_->setBandFrequency(band, frequency);
}

void NativeDynamicEQEffect::setBandQ(int band, float q)
{
    if (dynamicEQ_)
        dynamicEQ_->setBandQ(band, q);
}

void NativeDynamicEQEffect::setBandEnabled(int band, bool enabled)
{
    if (dynamicEQ_)
        dynamicEQ_->setBandEnabled(band, enabled);
}

void NativeDynamicEQEffect::setBandSolo(int band, bool solo)
{
    if (dynamicEQ_)
        dynamicEQ_->setBandSolo(band, solo);
}

void NativeDynamicEQEffect::setThreshold(int band, float dB)
{
    if (dynamicEQ_)
        dynamicEQ_->setThreshold(band, dB);
}

void NativeDynamicEQEffect::setRatio(int band, float ratio)
{
    if (dynamicEQ_)
        dynamicEQ_->setRatio(band, ratio);
}

void NativeDynamicEQEffect::setAttack(int band, float ms)
{
    if (dynamicEQ_)
        dynamicEQ_->setAttack(band, ms);
}

void NativeDynamicEQEffect::setRelease(int band, float ms)
{
    if (dynamicEQ_)
        dynamicEQ_->setRelease(band, ms);
}

void NativeDynamicEQEffect::setKnee(int band, float dB)
{
    if (dynamicEQ_)
        dynamicEQ_->setKnee(band, dB);
}

void NativeDynamicEQEffect::setMakeupGain(int band, float dB)
{
    if (dynamicEQ_)
        dynamicEQ_->setMakeupGain(band, dB);
}

void NativeDynamicEQEffect::setExternalSidechain(int band, bool enabled)
{
    if (dynamicEQ_)
        dynamicEQ_->setExternalSidechain(band, enabled);
}

void NativeDynamicEQEffect::setSidechainChannel(int band, int channel)
{
    if (dynamicEQ_)
        dynamicEQ_->setSidechainChannel(band, channel);
}

void NativeDynamicEQEffect::setAutoThresholdEnabled(int band, bool enabled)
{
    if (dynamicEQ_)
        dynamicEQ_->setAutoThresholdEnabled(band, enabled);
}

void NativeDynamicEQEffect::learnThreshold(int band)
{
    if (dynamicEQ_)
        dynamicEQ_->learnThreshold(band);
}

float NativeDynamicEQEffect::getGainReduction(int band) const
{
    if (dynamicEQ_)
        return dynamicEQ_->getGainReduction(band);
    return 0.0f;
}

float NativeDynamicEQEffect::getInputLevel(int band) const
{
    if (dynamicEQ_)
        return dynamicEQ_->getInputLevel(band);
    return -100.0f;
}

float NativeDynamicEQEffect::getOutputLevel(int band) const
{
    if (dynamicEQ_)
        return dynamicEQ_->getOutputLevel(band);
    return -100.0f;
}

void NativeDynamicEQEffect::setBandsLinked(bool linked)
{
    if (dynamicEQ_)
        dynamicEQ_->setBandsLinked(linked);
}

void NativeDynamicEQEffect::setWetDryMix(float mix)
{
    if (dynamicEQ_)
        dynamicEQ_->setWetDryMix(mix);
}

void NativeDynamicEQEffect::setOutputGain(float dB)
{
    if (dynamicEQ_)
        dynamicEQ_->setOutputGain(dB);
}

void NativeDynamicEQEffect::loadPreset(const juce::String& presetName)
{
    if (dynamicEQ_)
        dynamicEQ_->loadPreset(presetName);
}

juce::StringArray NativeDynamicEQEffect::getPresetNames()
{
    return effects::DynamicEQ::getPresetNames();
}

//==============================================================================
// NativeEffectFactory Implementation
//==============================================================================
std::unique_ptr<EffectProcessor> NativeEffectFactory::create(EffectType type)
{
    switch (type)
    {
        case EffectType::AutoTune:
            return createAutoTune();
        case EffectType::DeEsser:
            return createDeEsser();
        case EffectType::TransientShaper:
            return createTransientShaper();
        case EffectType::VoiceChanger:
            return createVoiceChanger();
        case EffectType::ChannelStrip:
            return createChannelStrip();
        case EffectType::MultibandCompressor:
            return createMultibandCompressor();
        case EffectType::ConvolutionReverb:
            return createConvolutionReverb();
        case EffectType::LinearPhaseEQ:
            return createLinearPhaseEQ();
        case EffectType::DynamicEQ:
            return createDynamicEQ();
        case EffectType::AlgorithmicReverb:
            return createAlgorithmicReverb();
    }
    return nullptr;
}

std::unique_ptr<NativeAutoTuneEffect> NativeEffectFactory::createAutoTune()
{
    return std::make_unique<NativeAutoTuneEffect>();
}

std::unique_ptr<NativeDeEsserEffect> NativeEffectFactory::createDeEsser()
{
    return std::make_unique<NativeDeEsserEffect>();
}

std::unique_ptr<NativeTransientShaperEffect> NativeEffectFactory::createTransientShaper()
{
    return std::make_unique<NativeTransientShaperEffect>();
}

std::unique_ptr<NativeVoiceChangerEffect> NativeEffectFactory::createVoiceChanger()
{
    return std::make_unique<NativeVoiceChangerEffect>();
}

std::unique_ptr<NativeChannelStripEffect> NativeEffectFactory::createChannelStrip()
{
    return std::make_unique<NativeChannelStripEffect>();
}

std::unique_ptr<NativeMultibandCompressorEffect> NativeEffectFactory::createMultibandCompressor()
{
    return std::make_unique<NativeMultibandCompressorEffect>();
}

std::unique_ptr<NativeConvolutionReverbEffect> NativeEffectFactory::createConvolutionReverb()
{
    return std::make_unique<NativeConvolutionReverbEffect>();
}

std::unique_ptr<NativeLinearPhaseEQEffect> NativeEffectFactory::createLinearPhaseEQ()
{
    return std::make_unique<NativeLinearPhaseEQEffect>();
}

std::unique_ptr<NativeDynamicEQEffect> NativeEffectFactory::createDynamicEQ()
{
    return std::make_unique<NativeDynamicEQEffect>();
}

//==============================================================================
// NativeAlgorithmicReverbEffect Implementation
//==============================================================================
NativeAlgorithmicReverbEffect::NativeAlgorithmicReverbEffect()
{
    algorithmicReverb_ = std::make_unique<effects::AlgorithmicReverb>();
}

NativeAlgorithmicReverbEffect::~NativeAlgorithmicReverbEffect()
{
}

void NativeAlgorithmicReverbEffect::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    if (algorithmicReverb_)
    {
        algorithmicReverb_->prepare(sampleRate, maxSamplesPerBlock);
    }
}

void NativeAlgorithmicReverbEffect::reset()
{
    if (algorithmicReverb_)
    {
        algorithmicReverb_->reset();
    }
}

void NativeAlgorithmicReverbEffect::process(juce::AudioBuffer<float>& buffer,
                                             const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;

    if (isBypassed() || !algorithmicReverb_)
        return;

    algorithmicReverb_->process(buffer, wetLevel_.load());
}

juce::ValueTree NativeAlgorithmicReverbEffect::getState() const
{
    juce::ValueTree state("AlgorithmicReverbState");
    state.setProperty("roomSize", roomSize_.load(), nullptr);
    state.setProperty("damping", damping_.load(), nullptr);
    state.setProperty("wetLevel", wetLevel_.load(), nullptr);
    state.setProperty("decayTime", decayTime_.load(), nullptr);
    state.setProperty("preDelay", preDelay_.load(), nullptr);
    state.setProperty("diffusion", diffusion_.load(), nullptr);
    state.setProperty("modulation", modulation_.load(), nullptr);
    return state;
}

void NativeAlgorithmicReverbEffect::setState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    roomSize_ = state.getProperty("roomSize", 0.5f);
    damping_ = state.getProperty("damping", 0.5f);
    wetLevel_ = state.getProperty("wetLevel", 0.3f);
    decayTime_ = state.getProperty("decayTime", 2.0f);
    preDelay_ = state.getProperty("preDelay", 0.02f);
    diffusion_ = state.getProperty("diffusion", 0.5f);
    modulation_ = state.getProperty("modulation", 0.1f);
}

void NativeAlgorithmicReverbEffect::setRoomSize(float size)
{
    roomSize_ = juce::jlimit(0.0f, 1.0f, size);
    if (algorithmicReverb_)
        algorithmicReverb_->setRoomSize(roomSize_.load());
}

void NativeAlgorithmicReverbEffect::setDamping(float d)
{
    damping_ = juce::jlimit(0.0f, 1.0f, d);
    if (algorithmicReverb_)
        algorithmicReverb_->setDamping(damping_.load());
}

void NativeAlgorithmicReverbEffect::setWetLevel(float wet)
{
    wetLevel_ = juce::jlimit(0.0f, 1.0f, wet);
}

void NativeAlgorithmicReverbEffect::setDecayTime(float decay)
{
    decayTime_ = juce::jlimit(0.1f, 10.0f, decay);
    if (algorithmicReverb_)
        algorithmicReverb_->setDecayTime(decayTime_.load());
}

void NativeAlgorithmicReverbEffect::setPreDelay(float predelay)
{
    preDelay_ = juce::jlimit(0.0f, 0.2f, predelay);
    if (algorithmicReverb_)
        algorithmicReverb_->setPreDelay(preDelay_.load());
}

void NativeAlgorithmicReverbEffect::setDiffusion(float diff)
{
    diffusion_ = juce::jlimit(0.0f, 1.0f, diff);
    if (algorithmicReverb_)
        algorithmicReverb_->setDiffusion(diffusion_.load());
}

void NativeAlgorithmicReverbEffect::setModulation(float mod)
{
    modulation_ = juce::jlimit(0.0f, 1.0f, mod);
    if (algorithmicReverb_)
        algorithmicReverb_->setModulation(modulation_.load());
}

std::unique_ptr<NativeAlgorithmicReverbEffect> NativeEffectFactory::createAlgorithmicReverb()
{
    return std::make_unique<NativeAlgorithmicReverbEffect>();
}

} // namespace engine
} // namespace zenith
