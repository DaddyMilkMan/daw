/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "ConvolutionReverb.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace effects {

//==============================================================================
// ConvolutionReverb Implementation
//==============================================================================

ConvolutionReverb::ConvolutionReverb()
{
    // Register parameters
    addParameter({"preDelay", "Pre-Delay", 0.0f, 200.0f, 20.0f, true, 0.0f});
    addParameter({"decayTime", "Decay Time", 10.0f, 200.0f, 100.0f, true, 0.0f});
    addParameter({"size", "Size", 0.0f, 200.0f, 100.0f, true, 0.0f});
    addParameter({"density", "Density", 0.0f, 100.0f, 100.0f, true, 0.0f});
    addParameter({"highCutDamping", "High Cut Damping", 1000.0f, 16000.0f, 10000.0f, true, 0.0f});
    addParameter({"lowCutDamping", "Low Cut Damping", 20.0f, 1000.0f, 100.0f, true, 0.0f});
    addParameter({"earlyReflectionsMix", "Early Reflections Mix", 0.0f, 1.0f, 0.3f, true, 0.0f});
    addParameter({"wetMix", "Wet Mix", 0.0f, 1.0f, 0.5f, true, 0.0f});
    addParameter({"stereoWidth", "Stereo Width", 0.0f, 2.0f, 1.0f, true, 0.0f});
    addParameter({"reverseEnabled", "Reverse", 0.0f, 1.0f, 0.0f, true, 0.0f});
    addParameter({"eqLow", "EQ Low", -12.0f, 12.0f, 0.0f, true, 0.0f});
    addParameter({"eqMid", "EQ Mid", -12.0f, 12.0f, 0.0f, true, 0.0f});
    addParameter({"eqHigh", "EQ High", -12.0f, 12.0f, 0.0f, true, 0.0f});
}

ConvolutionReverb::~ConvolutionReverb()
{
}

//==============================================================================
void ConvolutionReverb::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    // Prepare convolution engine
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2};
    convolution_.prepare(spec);

    // Prepare pre-delay (max 200ms)
    maxPreDelaySamples_ = static_cast<int>(sampleRate * 0.2);
    preDelayLine_.prepare(spec);
    preDelayLine_.setMaximumDelayInSamples(static_cast<int>(maxPreDelaySamples_));

    // Prepare EQ filters
    eqLow_.prepare(spec);
    eqMid_.prepare(spec);
    eqHigh_.prepare(spec);

    // Prepare damping filters
    highCutDampingFilter_.prepare(spec);
    lowCutDampingFilter_.prepare(spec);

    updateFilters();

    // Prepare wet buffers
    wetBuffer_.setSize(2, maxSamplesPerBlock);
    earlyReflectionsBuffer_.setSize(2, maxSamplesPerBlock);

    // Load default impulse response (Cathedral Hall)
    loadImpulseResponse("Cathedral Hall");
}

void ConvolutionReverb::reset()
{
    convolution_.reset();
    preDelayLine_.reset();
    eqLow_.reset();
    eqMid_.reset();
    eqHigh_.reset();
    highCutDampingFilter_.reset();
    lowCutDampingFilter_.reset();
}

void ConvolutionReverb::process(juce::AudioBuffer<float>& buffer,
                                const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Apply pre-delay
    int preDelaySamples = static_cast<int>((preDelay_.load() / 1000.0f) * sampleRate_);
    preDelayLine_.setDelay(preDelaySamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float delayed = preDelayLine_.popSample(ch, 0);
            preDelayLine_.pushSample(ch, channelData[i]);
            channelData[i] = delayed;
        }
    }

    // Process through convolution reverb
    wetBuffer_.makeCopyOf(buffer);
    juce::dsp::AudioBlock<float> block(wetBuffer_);
    juce::dsp::ProcessContextReplacing<float> context(block);
    convolution_.process(context);

    // Apply damping filters
    juce::dsp::AudioBlock<float> wetBlock(wetBuffer_);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
    highCutDampingFilter_.process(wetContext);
    lowCutDampingFilter_.process(wetContext);

    // Apply EQ to wet signal
    eqLow_.process(wetContext);
    eqMid_.process(wetContext);
    eqHigh_.process(wetContext);

    // Apply stereo width
    applyStereoWidth(wetBuffer_);

    // Apply decay time scaling
    float decayScale = decayTime_.load() / 100.0f;
    wetBuffer_.applyGain(decayScale);

    // Apply density
    float densityGain = density_.load() / 100.0f;
    wetBuffer_.applyGain(densityGain);

    // Mix wet and dry
    float wet = wetMix_.load();
    float dry = 1.0f - wet;
    float wetGain = std::sqrt(wet);
    float dryGain = std::sqrt(dry);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        auto* wetChan = wetBuffer_.getReadPointer(ch);
        auto* dryChan = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            out[i] = wetChan[i] * wetGain + dryChan[i] * dryGain;
        }
    }
}

//==============================================================================
bool ConvolutionReverb::loadImpulseResponse(const juce::File& file)
{
    // Load from file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (!reader)
        return false;

    // Read impulse response
    std::vector<float> irData(reader->lengthInSamples);
    reader->read(&irData.data()[0], 2, reader->lengthInSamples, false);

    // Load into convolution engine
    juce::dsp::Convolution::ConvolutionEngineType engineType =
        juce::dsp::Convolution::ConvolutionEngineType::engineTypeGeneric;

    convolution_.loadImpulseResponse(
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        juce::dsp::Convolution::Normalise::yes,
        reader->sampleRate,
        irData.data(),
        static_cast<int>(irData.size())
    );

    currentImpulseName_ = file.getFileNameWithoutWhiteSpace().toStdString();
    currentImpulseLength_ = static_cast<int>(irData.size());
    currentImpulseSampleRate_ = reader->sampleRate;

    return true;
}

bool ConvolutionReverb::loadImpulseResponse(const juce::String& builtinName)
{
    // Generate built-in impulse response
    ImpulseResponseInfo info;

    if (builtinName == "Cathedral Hall")
    {
        info.name = "Cathedral Hall";
        info.category = "Halls";
        info.decayTime = 3.5f;
        info.preDelay = 0.02f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Concert Hall")
    {
        info.name = "Concert Hall";
        info.category = "Halls";
        info.decayTime = 2.2f;
        info.preDelay = 0.015f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Small Room")
    {
        info.name = "Small Room";
        info.category = "Rooms";
        info.decayTime = 0.5f;
        info.preDelay = 0.0f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Medium Room")
    {
        info.name = "Medium Room";
        info.category = "Rooms";
        info.decayTime = 0.8f;
        info.preDelay = 0.0f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Living Room")
    {
        info.name = "Living Room";
        info.category = "Rooms";
        info.decayTime = 1.0f;
        info.preDelay = 0.0f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "EMT 140 Plate")
    {
        info.name = "EMT 140 Plate";
        info.category = "Plates";
        info.decayTime = 2.5f;
        info.preDelay = 0.0f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Stockhausen Plate")
    {
        info.name = "Stockhausen Plate";
        info.category = "Plates";
        info.decayTime = 3.0f;
        info.preDelay = 0.0f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Echo Chamber")
    {
        info.name = "Echo Chamber";
        info.category = "Chambers";
        info.decayTime = 1.5f;
        info.preDelay = 0.01f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Drum Chamber")
    {
        info.name = "Drum Chamber";
        info.category = "Chambers";
        info.decayTime = 0.9f;
        info.preDelay = 0.005f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Spring Reverb")
    {
        info.name = "Spring Reverb";
        info.category = "Special";
        info.decayTime = 1.8f;
        info.preDelay = 0.0f;
        info.sampleRate = 44100;
    }
    else if (builtinName == "Reverse Gate")
    {
        info.name = "Reverse Gate";
        info.category = "Special";
        info.decayTime = 2.0f;
        info.preDelay = 0.0f;
        info.sampleRate = 44100;
    }
    else
    {
        // Default to Cathedral Hall
        return loadImpulseResponse("Cathedral Hall");
    }

    // Generate impulse response
    std::vector<float> irData = generateBuiltinImpulse(info);

    // If reverse is enabled, reverse the impulse
    if (reverseEnabled_.load())
    {
        std::reverse(irData.begin(), irData.end());
    }

    // Load into convolution engine
    convolution_.loadImpulseResponse(
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        juce::dsp::Convolution::Normalise::yes,
        info.sampleRate,
        irData.data(),
        static_cast<int>(irData.size())
    );

    currentImpulseName_ = info.name.toStdString();
    currentImpulseLength_ = static_cast<int>(irData.size());
    currentImpulseSampleRate_ = info.sampleRate;

    // Set pre-delay
    setPreDelay(info.preDelay * 1000.0f);  // convert to ms

    return true;
}

void ConvolutionReverb::loadImpulseResponseFromMemory(const juce::String& name,
                                                      const std::vector<float>& data,
                                                      int sampleRate)
{
    convolution_.loadImpulseResponse(
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        juce::dsp::Convolution::Normalise::yes,
        sampleRate,
        data.data(),
        static_cast<int>(data.size())
    );

    currentImpulseName_ = name.toStdString();
    currentImpulseLength_ = static_cast<int>(data.size());
    currentImpulseSampleRate_ = sampleRate;
}

juce::String ConvolutionReverb::getCurrentImpulseName() const
{
    return currentImpulseName_;
}

int ConvolutionReverb::getCurrentImpulseLength() const
{
    return currentImpulseLength_;
}

float ConvolutionReverb::getCurrentImpulseLengthSeconds() const
{
    return static_cast<float>(currentImpulseLength_) / static_cast<float>(currentImpulseSampleRate_);
}

int ConvolutionReverb::getCurrentImpulseSampleRate() const
{
    return currentImpulseSampleRate_;
}

juce::StringArray ConvolutionReverb::getBuiltinImpulseNames()
{
    return {
        "Cathedral Hall",
        "Concert Hall",
        "Small Room",
        "Medium Room",
        "Living Room",
        "EMT 140 Plate",
        "Stockhausen Plate",
        "Echo Chamber",
        "Drum Chamber",
        "Spring Reverb",
        "Reverse Gate"
    };
}

juce::StringArray ConvolutionReverb::getImpulseCategories()
{
    return {
        "Halls",
        "Rooms",
        "Plates",
        "Chambers",
        "Special"
    };
}

//==============================================================================
void ConvolutionReverb::setPreDelay(float milliseconds)
{
    preDelay_.store(juce::jlimit(0.0f, 200.0f, milliseconds));
}

void ConvolutionReverb::setDecayTime(float percentage)
{
    decayTime_.store(juce::jlimit(10.0f, 200.0f, percentage));
}

void ConvolutionReverb::setSize(float percentage)
{
    size_.store(juce::jlimit(0.0f, 200.0f, percentage));
}

void ConvolutionReverb::setDensity(float percentage)
{
    density_.store(juce::jlimit(0.0f, 100.0f, percentage));
}

void ConvolutionReverb::setHighCutDamping(float frequencyHz)
{
    highCutDamping_.store(juce::jlimit(1000.0f, 16000.0f, frequencyHz));
    updateFilters();
}

void ConvolutionReverb::setLowCutDamping(float frequencyHz)
{
    lowCutDamping_.store(juce::jlimit(20.0f, 1000.0f, frequencyHz));
    updateFilters();
}

void ConvolutionReverb::setEarlyReflectionsMix(float mix)
{
    earlyReflectionsMix_.store(juce::jlimit(0.0f, 1.0f, mix));
}

void ConvolutionReverb::setWetMix(float mix)
{
    wetMix_.store(juce::jlimit(0.0f, 1.0f, mix));
}

void ConvolutionReverb::setStereoWidth(float width)
{
    stereoWidth_.store(juce::jlimit(0.0f, 2.0f, width));
}

void ConvolutionReverb::setReverseEnabled(bool enabled)
{
    reverseEnabled_.store(enabled);

    // Reload current IR with reverse applied
    if (enabled)
    {
        juce::String currentName = getCurrentImpulseName();
        if (!currentName.isEmpty())
            loadImpulseResponse(currentName);
    }
}

void ConvolutionReverb::setEQLow(float dB)
{
    eqLowParam_.store(juce::jlimit(-12.0f, 12.0f, dB));
    updateFilters();
}

void ConvolutionReverb::setEQMid(float dB)
{
    eqMidParam_.store(juce::jlimit(-12.0f, 12.0f, dB));
    updateFilters();
}

void ConvolutionReverb::setEQHigh(float dB)
{
    eqHighParam_.store(juce::jlimit(-12.0f, 12.0f, dB));
    updateFilters();
}

//==============================================================================
void ConvolutionReverb::updateFilters()
{
    // Update EQ filters
    auto lowShelf = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate_, 200.0f, 0.5f, juce::Decibels::decibelsToGain(eqLowParam_.load()));
    *eqLow_.state = lowShelf;

    auto peak = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate_, 1000.0f, 1.0f, juce::Decibels::decibelsToGain(eqMid_.load()));
    *eqMid_.state = peak;

    auto highShelf = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate_, 5000.0f, 0.5f, juce::Decibels::decibelsToGain(eqHigh_.load()));
    *eqHigh_.state = highShelf;

    // Update damping filters
    auto lowCut = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(
        sampleRate_, lowCutDamping_.load());
    *lowCutDampingFilter_.state = lowCut;

    auto highCut = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        sampleRate_, highCutDamping_.load());
    *highCutDampingFilter_.state = highCut;
}

void ConvolutionReverb::applyStereoWidth(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
        return;

    float width = stereoWidth_.load();

    // Extract mid/side
    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        float mid = (left[i] + right[i]) * 0.5f;
        float side = (left[i] - right[i]) * 0.5f;

        // Apply width to side
        side *= width;

        // Reconstruct left/right
        left[i] = mid + side;
        right[i] = mid - side;
    }
}

//==============================================================================
std::vector<float> ConvolutionReverb::generateBuiltinImpulse(const ImpulseResponseInfo& info)
{
    if (info.category == "Halls")
        return generateHallImpulse(info.decayTime, info.sampleRate);
    else if (info.category == "Rooms")
        return generateRoomImpulse(info.decayTime, info.sampleRate);
    else if (info.category == "Plates")
        return generatePlateImpulse(info.decayTime, info.sampleRate);
    else if (info.category == "Chambers")
        return generateChamberImpulse(info.decayTime, info.sampleRate);
    else if (info.category == "Special")
    {
        if (info.name.contains("Spring"))
            return generateSpringImpulse(info.decayTime, info.sampleRate);
        else if (info.name.contains("Reverse"))
            return generateReverseImpulse(info.decayTime, info.sampleRate);
    }

    // Default to hall
    return generateHallImpulse(info.decayTime, info.sampleRate);
}

std::vector<float> ConvolutionReverb::generateHallImpulse(float decayTime, int sampleRate)
{
    // Generate hall-like impulse with exponential decay and early reflections
    int lengthSamples = static_cast<int>(decayTime * sampleRate);
    std::vector<float> impulse(lengthSamples, 0.0f);

    // Early reflections (first 100ms)
    int earlyReflections = static_cast<int>(0.1f * sampleRate);

    // Add early reflection spikes
    std::vector<int> earlyTimes = {0, 15, 27, 41, 58, 73, 89};
    std::vector<float> earlyGains = {1.0f, 0.6f, 0.4f, 0.3f, 0.2f, 0.15f, 0.1f};

    for (size_t i = 0; i < earlyTimes.size() && earlyTimes[i] < lengthSamples; ++i)
    {
        impulse[earlyTimes[i]] = earlyGains[i];
        if (earlyTimes[i] + 1 < lengthSamples)
            impulse[earlyTimes[i] + 1] = earlyGains[i] * 0.5f;
    }

    // Diffuse tail (exponential decay with noise)
    float decayConstant = -std::log(0.001f) / (lengthSamples - earlyReflections);

    for (int i = earlyReflections; i < lengthSamples; ++i)
    {
        float envelope = std::exp(-decayConstant * (i - earlyReflections));
        float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        impulse[i] = noise * envelope * 0.3f;
    }

    return impulse;
}

std::vector<float> ConvolutionReverb::generateRoomImpulse(float decayTime, int sampleRate)
{
    // Generate room-like impulse (shorter, brighter)
    int lengthSamples = static_cast<int>(decayTime * sampleRate);
    std::vector<float> impulse(lengthSamples, 0.0f);

    // Few early reflections
    std::vector<int> earlyTimes = {0, 12, 23, 35, 50};
    std::vector<float> earlyGains = {1.0f, 0.5f, 0.3f, 0.2f, 0.1f};

    for (size_t i = 0; i < earlyTimes.size() && earlyTimes[i] < lengthSamples; ++i)
    {
        impulse[earlyTimes[i]] = earlyGains[i];
    }

    // Shorter diffuse tail
    float decayConstant = -std::log(0.001f) / (lengthSamples * 0.5f);

    for (int i = 50; i < lengthSamples; ++i)
    {
        float envelope = std::exp(-decayConstant * i);
        float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        impulse[i] = noise * envelope * 0.2f;
    }

    return impulse;
}

std::vector<float> ConvolutionReverb::generatePlateImpulse(float decayTime, int sampleRate)
{
    // Generate plate-like impulse (metallic, dense reflections)
    int lengthSamples = static_cast<int>(decayTime * sampleRate);
    std::vector<float> impulse(lengthSamples, 0.0f);

    // Very dense early reflections
    for (int i = 0; i < 200 && i < lengthSamples; ++i)
    {
        float gain = std::exp(-i * 0.01f);
        float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        impulse[i] = noise * gain * 0.5f;
    }

    // Smooth diffuse tail
    float decayConstant = -std::log(0.001f) / lengthSamples;

    for (int i = 200; i < lengthSamples; ++i)
    {
        float envelope = std::exp(-decayConstant * i);
        float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        impulse[i] = noise * envelope * 0.25f;
    }

    return impulse;
}

std::vector<float> ConvolutionReverb::generateChamberImpulse(float decayTime, int sampleRate)
{
    // Generate chamber-like impulse (bright, metallic)
    int lengthSamples = static_cast<int>(decayTime * sampleRate);
    std::vector<float> impulse(lengthSamples, 0.0f);

    // Dense, bright early reflections
    for (int i = 0; i < 150 && i < lengthSamples; ++i)
    {
        float gain = std::exp(-i * 0.02f);
        float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        impulse[i] = noise * gain * 0.6f;
    }

    // Bright diffuse tail
    float decayConstant = -std::log(0.001f) / lengthSamples;

    for (int i = 150; i < lengthSamples; ++i)
    {
        float envelope = std::exp(-decayConstant * i);
        float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        impulse[i] = noise * envelope * 0.3f;
    }

    return impulse;
}

std::vector<float> ConvolutionReverb::generateSpringImpulse(float decayTime, int sampleRate)
{
    // Generate spring-like impulse (bouncy, metallic)
    int lengthSamples = static_cast<int>(decayTime * sampleRate);
    std::vector<float> impulse(lengthSamples, 0.0f);

    // Spring characteristic: repeating decaying bursts
    float burstInterval = 0.05f * sampleRate;  // 50ms between bursts
    float burstDecay = 0.7f;

    for (int burst = 0; burst < 10; ++burst)
    {
        int burstStart = static_cast<int>(burst * burstInterval);
        float burstGain = std::pow(burstDecay, burst);

        for (int i = 0; i < 100 && burstStart + i < lengthSamples; ++i)
        {
            float envelope = std::exp(-i * 0.05f);
            float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
            impulse[burstStart + i] += noise * envelope * burstGain * 0.4f;
        }
    }

    return impulse;
}

std::vector<float> ConvolutionReverb::generateReverseImpulse(float decayTime, int sampleRate)
{
    // Generate reverse gate impulse (swell up, then cut)
    int lengthSamples = static_cast<int>(decayTime * sampleRate);
    std::vector<float> impulse(lengthSamples, 0.0f);

    // Swell up (reverse envelope)
    for (int i = 0; i < lengthSamples; ++i)
    {
        float envelope = static_cast<float>(i) / static_cast<float>(lengthSamples);
        envelope = envelope * envelope;  // Quadratic curve
        float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        impulse[i] = noise * envelope * 0.5f;
    }

    return impulse;
}

//==============================================================================
juce::ValueTree ConvolutionReverb::getState() const
{
    juce::ValueTree state("ConvolutionReverb");

    state.setProperty("impulseName", currentImpulseName_, nullptr);
    state.setProperty("preDelay", preDelay_.load(), nullptr);
    state.setProperty("decayTime", decayTime_.load(), nullptr);
    state.setProperty("size", size_.load(), nullptr);
    state.setProperty("density", density_.load(), nullptr);
    state.setProperty("highCutDamping", highCutDamping_.load(), nullptr);
    state.setProperty("lowCutDamping", lowCutDamping_.load(), nullptr);
    state.setProperty("earlyReflectionsMix", earlyReflectionsMix_.load(), nullptr);
    state.setProperty("wetMix", wetMix_.load(), nullptr);
    state.setProperty("stereoWidth", stereoWidth_.load(), nullptr);
    state.setProperty("reverseEnabled", reverseEnabled_.load(), nullptr);
    state.setProperty("eqLow", eqLow_.load(), nullptr);
    state.setProperty("eqMid", eqMid_.load(), nullptr);
    state.setProperty("eqHigh", eqHigh_.load(), nullptr);

    return state;
}

void ConvolutionReverb::setState(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getType() != juce::String("ConvolutionReverb"))
        return;

    juce::String impulseName = state.getProperty("impulseName", "Cathedral Hall");
    preDelay_.store(state.getProperty("preDelay", 20.0f));
    decayTime_.store(state.getProperty("decayTime", 100.0f));
    size_.store(state.getProperty("size", 100.0f));
    density_.store(state.getProperty("density", 100.0f));
    highCutDamping_.store(state.getProperty("highCutDamping", 10000.0f));
    lowCutDamping_.store(state.getProperty("lowCutDamping", 100.0f));
    earlyReflectionsMix_.store(state.getProperty("earlyReflectionsMix", 0.3f));
    wetMix_.store(state.getProperty("wetMix", 0.5f));
    stereoWidth_.store(state.getProperty("stereoWidth", 1.0f));
    reverseEnabled_.store(state.getProperty("reverseEnabled", false));
    eqLow_.store(state.getProperty("eqLow", 0.0f));
    eqMid_.store(state.getProperty("eqMid", 0.0f));
    eqHigh_.store(state.getProperty("eqHigh", 0.0f));

    // Reload impulse response
    loadImpulseResponse(impulseName);
    updateFilters();
}

//==============================================================================
// Presets
void ConvolutionReverb::loadPreset(const juce::String& presetName)
{
    if (presetName == "Cathedral")
    {
        loadImpulseResponse("Cathedral Hall");
        setPreDelay(20.0f);
        setDecayTime(100.0f);
        setHighCutDamping(8000.0f);
        setLowCutDamping(100.0f);
        setWetMix(0.5f);
        setStereoWidth(1.0f);
    }
    else if (presetName == "Small Room")
    {
        loadImpulseResponse("Small Room");
        setPreDelay(0.0f);
        setDecayTime(80.0f);
        setHighCutDamping(10000.0f);
        setLowCutDamping(200.0f);
        setWetMix(0.3f);
        setStereoWidth(0.8f);
    }
    else if (presetName == "Plate")
    {
        loadImpulseResponse("EMT 140 Plate");
        setPreDelay(0.0f);
        setDecayTime(90.0f);
        setHighCutDamping(6000.0f);
        setLowCutDamping(200.0f);
        setWetMix(0.5f);
        setStereoWidth(1.2f);
    }
    else if (presetName == "Ambience")
    {
        loadImpulseResponse("Small Room");
        setPreDelay(5.0f);
        setDecayTime(50.0f);
        setHighCutDamping(12000.0f);
        setLowCutDamping(300.0f);
        setWetMix(0.2f);
        setStereoWidth(1.0f);
    }
    else if (presetName == "Drum Room")
    {
        loadImpulseResponse("Drum Chamber");
        setPreDelay(10.0f);
        setDecayTime(70.0f);
        setHighCutDamping(7000.0f);
        setLowCutDamping(150.0f);
        setWetMix(0.35f);
        setStereoWidth(1.0f);
    }
}

juce::StringArray ConvolutionReverb::getPresetNames()
{
    return {
        "Cathedral",
        "Small Room",
        "Plate",
        "Ambience",
        "Drum Room"
    };
}

} // namespace effects
} // namespace zenith
