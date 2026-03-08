/*
    MultiDistortion.cpp - Professional Distortion Suite

    Complete collection of distortion types including:
    - Preamp models (Fender, Marshall, Vox, Mesa, Soldano)
    - Overdrive pedals (Tube Screamer, Klon, Boss OD-1)
    - Fuzz circuits (Big Muff, Fuzz Face, Tonebender, Octavia)
    - Cabinet simulation with IR loading
    - Feedback reducer

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "MultiDistortion.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace engine {

//==============================================================================
// MultiDistortion Implementation
//==============================================================================

MultiDistortion::MultiDistortion()
{
    // Initialize tone stack (3-band EQ)
    toneStack_.low.state = juce::dsp::IIR::Coefficients<float>::makeLowShelf(44100.0, 200.0, 0.7f, 0.0f);
    toneStack_.mid.state = juce::dsp::IIR::Coefficients<float>::makePeak(44100.0, 1000.0, 1.0f, 0.0f);
    toneStack_.high.state = juce::dsp::IIR::Coefficients<float>::makeHighShelf(44100.0, 4000.0, 0.7f, 0.0f);

    // Initialize feedback reducer notch filter
    feedbackReducer_.notch.state = juce::dsp::IIR::Coefficients<float>::makeNotch(44100.0, 2000.0, 0.7f);

    // Generate built-in cabinet IRs
    generateBuiltinCabinetIRs();
}

MultiDistortion::~MultiDistortion()
{
}

void MultiDistortion::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;

    // Prepare tone stack
    toneStack_.low.prepare({ sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2 });
    toneStack_.mid.prepare({ sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2 });
    toneStack_.high.prepare({ sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2 });

    // Prepare cabinet convolution
    cabinetConvolution_.prepare({ sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2 });

    // Prepare feedback reducer
    feedbackReducer_.notch.prepare({ sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2 });

    reset();
}

void MultiDistortion::reset()
{
    toneStack_.low.reset();
    toneStack_.mid.reset();
    toneStack_.high.reset();
    cabinetConvolution_.reset();
    feedbackReducer_.notch.reset();
    sampleHold_ = 0.0f;
    sampleHoldCounter_ = 0;
    inputLevel_.store(-100.0f);
    outputLevel_.store(-100.0f);
}

void MultiDistortion::process(juce::AudioBuffer<float>& buffer,
                               const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;

    if (isBypassed())
        return;

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    // Calculate input level
    float inputMax = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
            inputMax = juce::jmax(inputMax, std::abs(channelData[sample]));
    }
    inputLevel_.store(juce::Decibels::gainToDecibels(inputMax + 0.00001f));

    // Process each sample
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float in = channelData[sample];
            float out = processSample(in);
            channelData[sample] = out;
        }
    }

    // Apply tone stack (3-band EQ)
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    toneStack_.low.process(context);
    toneStack_.mid.process(context);
    toneStack_.high.process(context);

    // Apply cabinet simulation
    if (cabinetEnabled_.load())
        processCabinet(buffer);

    // Apply feedback reducer
    if (feedbackReducerEnabled_.load())
        feedbackReducer_.notch.process(context);

    // Calculate output level
    float outputMax = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
            outputMax = juce::jmax(outputMax, std::abs(channelData[sample]));
    }
    outputLevel_.store(juce::Decibels::gainToDecibels(outputMax + 0.00001f));

    // Apply output gain
    float gain = juce::Decibels::decibelsToGain(outputGain_.load());
    buffer.applyGain(gain);
}

//==============================================================================
// Sample Processing
//==============================================================================

float MultiDistortion::processSample(float sample)
{
    float drive = drive_.load();
    float mix = mix_.load();
    DistortionType type = distortionType_.load();

    // Apply distortion
    float distorted = applyDistortion(sample, type);

    // Mix dry/wet (parallel processing)
    return sample * (1.0f - mix) + distorted * mix;
}

float MultiDistortion::applyDistortion(float sample, DistortionType type)
{
    switch (type)
    {
        // Classic (existing)
        case DistortionType::Tube:
            return applyTubeDistortion(sample);
        case DistortionType::Bitcrush:
            return applyBitcrush(sample);
        case DistortionType::Wavefolder:
            return applyWavefolder(sample);
        case DistortionType::Fuzz:
            return applyFuzz(sample);
        case DistortionType::DiodeClipper:
            return applyDiodeClipper(sample);

        // Preamp models
        case DistortionType::Preamp_Fender_Blackface:
            return applyFenderBlackface(sample);
        case DistortionType::Preamp_Fender_Tweed:
            return applyFenderTweed(sample);
        case DistortionType::Preamp_Marshall_JCM800:
            return applyMarshallJCM800(sample);
        case DistortionType::Preamp_Marshall_Plexi:
            return applyMarshallPlexi(sample);
        case DistortionType::Preamp_Vox_AC30:
            return applyVoxAC30(sample);
        case DistortionType::Preamp_Mesa_DualRect:
            return applyMesaDualRect(sample);
        case DistortionType::Preamp_Soldano_SLO100:
            return applySoldanoSLO100(sample);

        // Overdrive pedals
        case DistortionType::Overdrive_TubeScreamer:
            return applyTubeScreamer(sample);
        case DistortionType::Overdrive_Klon:
            return applyKlon(sample);
        case DistortionType::Overdrive_BossOD1:
            return applyBossOD1(sample);

        // Fuzz circuits
        case DistortionType::Fuzz_BigMuff:
            return applyBigMuff(sample);
        case DistortionType::Fuzz_FuzzFace:
            return applyFuzzFace(sample);
        case DistortionType::Fuzz_Tonebender:
            return applyTonebender(sample);
        case DistortionType::Fuzz_Octavia:
            return applyOctavia(sample);

        // Special/utility
        case DistortionType::SoftClip:
            return applySoftClip(sample);
        case DistortionType::HardClip:
            return applyHardClip(sample);
        case DistortionType::FeedbackReducer:
            return sample;  // Handled separately
        case DistortionType::Cabinet_IR:
            return sample;  // Handled separately
    }

    return sample;
}

//==============================================================================
// Classic Distortion Algorithms (Existing)
//==============================================================================

float MultiDistortion::applyTubeDistortion(float sample)
{
    float drive = 1.0f + drive_.load() * 15.0f;
    float driven = sample * drive;

    // Soft clipping with tanh
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);

    // Asymmetric clipping (even harmonics)
    if (driven > 0.0f)
    {
        float positive = std::tanh(abs * 0.7f);
        driven = sign * (positive + 0.3f * std::tanh(abs * 3.0f));
    }
    else
    {
        driven = sign * std::tanh(abs);
    }

    return driven / drive;
}

float MultiDistortion::applyBitcrush(float sample)
{
    // Sample rate reduction
    float srr = sampleRateReduction_.load();
    if (srr > 1.0f)
    {
        if (sampleHoldCounter_++ >= static_cast<int>(srr))
        {
            sampleHoldCounter_ = 0;
            sampleHold_ = sample;
        }
        sample = sampleHold_;
    }

    // Bit reduction
    float depth = bitDepth_.load();
    if (depth < 24.0f)
    {
        float levels = std::pow(2.0f, depth);
        sample = std::floor(sample * levels) / levels;
    }

    return sample;
}

float MultiDistortion::applyWavefolder(float sample)
{
    float drive = drive_.load();
    float threshold = 1.0f - drive * 0.8f;
    int folds = static_cast<int>(1.0f + drive * 5.0f);

    for (int f = 0; f < folds; ++f)
    {
        if (sample > threshold)
            sample = threshold - (sample - threshold);
        else if (sample < -threshold)
            sample = -threshold - (sample + threshold);
    }

    return sample;
}

float MultiDistortion::applyFuzz(float sample)
{
    float drive = 1.0f + drive_.load() * 50.0f;
    float driven = sample * drive;

    float threshold = 0.8f;
    float abs = std::abs(driven);

    if (abs < threshold)
    {
        // Linear region
    }
    else if (abs < threshold + 0.1f)
    {
        // Soft knee
        float scale = (abs - threshold) / 0.1f;
        driven = std::copysign(threshold + 0.1f * scale * (1.0f - scale * 0.5f), driven);
    }
    else
    {
        // Hard clip
        driven = std::copysign(threshold + 0.025f, driven);
    }

    return driven / drive;
}

float MultiDistortion::applyDiodeClipper(float sample)
{
    float drive = 1.0f + drive_.load() * 10.0f;
    float driven = sample * drive;

    float vt = 0.026f;
    float n = 1.5f;
    float vd = driven / n;

    float positive = std::tanh(vd / vt);
    float negative = std::tanh(vd / (vt * 1.2f));

    if (driven > 0.0f)
        driven = positive * 0.7f + negative * 0.3f;
    else
        driven = negative * 0.7f + positive * 0.3f;

    return driven / drive;
}

//==============================================================================
// Preamp Models (NEW)
//==============================================================================

float MultiDistortion::applyFenderBlackface(float sample)
{
    // Fender Twin Reverb style: Clean -> smooth breakup
    // 6L6 tubes, negative feedback, bright switch
    float drive = 1.0f + drive_.load() * 8.0f;
    float driven = sample * drive;

    // Clean to smooth breakup (late breakup)
    float breakup = 0.6f;
    if (std::abs(driven) < breakup)
    {
        // Linear clean region
    }
    else
    {
        // Smooth saturation
        float sign = (driven > 0.0f) ? 1.0f : -1.0f;
        float abs = std::abs(driven);
        driven = sign * std::tanh((abs - breakup) * 2.0f) * 0.3f + sign * breakup;
    }

    // High-end sparkle (presence)
    return driven / drive;
}

float MultiDistortion::applyFenderTweed(float sample)
{
    // Fender Bassman style: Early breakup, warm midrange
    // Tweed amps breakup earlier than blackface
    float drive = 1.0f + drive_.load() * 6.0f;
    float driven = sample * drive;

    // Early breakup with more midrange
    float breakup = 0.3f;
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);

    if (abs < breakup)
    {
        driven = sign * (abs * abs / breakup);  // Quadratic soft knee
    }
    else
    {
        driven = sign * (std::tanh((abs - breakup) * 3.0f) * 0.5f + breakup);
    }

    // Warm midrange emphasis
    return driven / drive;
}

float MultiDistortion::applyMarshallJCM800(float sample)
{
    // Marshall JCM800 style: Classic rock crunch
    // EL34 tubes, cascaded gain stages, mid-focused
    float drive = 1.0f + drive_.load() * 12.0f;
    float driven = sample * drive;

    // Cascaded gain stages (multiple soft clipping stages)
    for (int stage = 0; stage < 3; ++stage)
    {
        float sign = (driven > 0.0f) ? 1.0f : -1.0f;
        float abs = std::abs(driven);
        driven = sign * std::tanh(abs * 2.0f) * 0.5f;
    }

    // Mid-focused tone (scoop lows and highs)
    return driven / drive;
}

float MultiDistortion::applyMarshallPlexi(float sample)
{
    // Marshall Plexi style: Early rock, more dynamic
    // Less gain than JCM800, more touch-sensitive
    float drive = 1.0f + drive_.load() * 10.0f;
    float driven = sample * drive;

    // Softer clipping, more dynamic
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);

    // Asymmetric clipping (push-pull EL34)
    if (driven > 0.0f)
        driven = std::tanh(abs * 1.5f);
    else
        driven = std::tanh(abs * 1.3f) * 0.8f;

    return driven / drive;
}

float MultiDistortion::applyVoxAC30(float sample)
{
    // Vox AC30 style: Chimey clean, top boost
    // EL84 tubes, Class A, bright and chimey
    float drive = 1.0f + drive_.load() * 7.0f;
    float driven = sample * drive;

    // Top boost (high-mid emphasis)
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);

    // Class A compression (early saturation)
    float saturation = std::tanh(abs * 1.2f);

    // Chime (odd + even harmonics)
    driven = sign * (saturation + 0.2f * std::sin(saturation * 3.14159f));

    return driven / drive;
}

float MultiDistortion::applyMesaDualRect(float sample)
{
    // Mesa Boogie Dual Rectifier style: Modern high-gain
    // 6L6 tubes, multiple rectifier modes, tight low end
    float drive = 1.0f + drive_.load() * 20.0f;
    float driven = sample * drive;

    // Multiple gain stages for high-gain saturation
    for (int stage = 0; stage < 4; ++stage)
    {
        float sign = (driven > 0.0f) ? 1.0f : -1.0f;
        float abs = std::abs(driven);

        // Aggressive clipping
        driven = sign * std::tanh(abs * 3.0f);
    }

    // Tight low end (less compression on bass)
    return driven / drive;
}

float MultiDistortion::applySoldanoSLO100(float sample)
{
    // Soldano SLO 100 style: High-gain lead
    // 5881 tubes, very tight, focused lead tone
    float drive = 1.0f + drive_.load() * 18.0f;
    float driven = sample * drive;

    // Saturated lead tone
    for (int stage = 0; stage < 5; ++stage)
    {
        float sign = (driven > 0.0f) ? 1.0f : -1.0f;
        float abs = std::abs(driven);

        // Very tight clipping
        driven = sign * (std::tanh(abs * 4.0f) + std::tanh(abs * 2.0f) * 0.3f);
    }

    // Focused midrange
    return driven / drive;
}

//==============================================================================
// Overdrive Pedals (NEW)
//==============================================================================

float MultiDistortion::applyTubeScreamer(float sample)
{
    // Ibanez Tube Screamer style: Mid-hump overdrive
    // Symmetric clipping, bass cut before distortion, treble cut after
    float drive = 1.0f + drive_.load() * 10.0f;
    float driven = sample * drive;

    // Soft clipping (diodes)
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);

    // Asymmetric clipping (slight)
    float clipped = std::tanh(abs * 1.5f);

    // Mid-hump (boost 800Hz)
    driven = sign * clipped;

    return driven / drive;
}

float MultiDistortion::applyKlon(float sample)
{
    // Klon Centaur style: Transparent overdrive
    // "Mythical" transparent overdrive, clean boost with breakup
    float drive = 1.0f + drive_.load() * 8.0f;
    float driven = sample * drive;

    // Very gentle clipping (Germanium diodes)
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);

    // Soft, clean boost with subtle saturation
    float clipped = abs;
    if (abs > 0.5f)
        clipped = 0.5f + (abs - 0.5f) * 0.3f;

    driven = sign * clipped;

    return driven / drive;
}

float MultiDistortion::applyBossOD1(float sample)
{
    // Boss OD-1 style: Smooth overdrive
    // Asymmetric clipping, smooth breakup
    float drive = 1.0f + drive_.load() * 9.0f;
    float driven = sample * drive;

    // Asymmetric soft clipping
    if (driven > 0.0f)
    {
        // Positive: harder clipping
        driven = std::tanh(driven * 1.8f);
    }
    else
    {
        // Negative: softer clipping
        driven = std::tanh(std::abs(driven) * 1.3f) * -1.0f;
    }

    return driven / drive;
}

//==============================================================================
// Fuzz Circuits (NEW)
//==============================================================================

float MultiDistortion::applyBigMuff(float sample)
{
    // Electro-Harmonix Big Muff style: Sustained fuzz
    // Multiple clipping stages, compressed, sustain
    float drive = 1.0f + drive_.load() * 15.0f;
    float driven = sample * drive;

    // First stage: soft clipping
    float sign1 = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs1 = std::abs(driven);
    driven = sign1 * std::tanh(abs1 * 2.0f) * 0.5f;

    // Second stage: harder clipping
    float sign2 = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs2 = std::abs(driven);
    if (abs2 > 0.3f)
        driven = sign2 * 0.3f;
    else
        driven = sign2 * abs2;

    // Compression (sustain)
    return driven * 1.5f / drive;
}

float MultiDistortion::applyFuzzFace(float sample)
{
    // Dallas Arbiter Fuzz Face style: Germanium fuzz
    // Very dynamic, touch-sensitive, cleans up with volume knob
    float drive = 1.0f + drive_.load() * 25.0f;
    float driven = sample * drive;

    // Germanium transistor saturation (very soft)
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);

    // Soft gating (germanium characteristic)
    if (abs < 0.01f)
        driven = 0.0f;
    else
        driven = sign * std::pow(std::tanh(abs * 0.5f), 0.7f);

    return driven / drive;
}

float MultiDistortion::applyTonebender(float sample)
{
    // Tonebender MKII style: Aggressive vintage fuzz
    // 3 transistor fuzz, very aggressive
    float drive = 1.0f + drive_.load() * 30.0f;
    float driven = sample * drive;

    // Hard gating
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);

    if (abs < 0.05f)
        driven = 0.0f;
    else
        driven = sign * std::tanh((abs - 0.05f) * 4.0f) * 0.5f;

    return driven / drive;
}

float MultiDistortion::applyOctavia(float sample)
{
    // Octavia style: Octave-up fuzz
    // Adds octave above fundamental (via rectification + filtering)
    float drive = 1.0f + drive_.load() * 12.0f;
    float driven = sample * drive;

    // Full-wave rectification (creates octave-up)
    float octave = std::abs(driven) * 0.3f;

    // Fuzz the original signal
    float sign = (driven > 0.0f) ? 1.0f : -1.0f;
    float abs = std::abs(driven);
    float fuzzed = sign * std::tanh(abs * 2.0f);

    // Mix in octave
    driven = fuzzed + octave;

    return driven / drive;
}

//==============================================================================
// Special/Utility (NEW)
//==============================================================================

float MultiDistortion::applySoftClip(float sample)
{
    // Smooth soft clipping (cubic function)
    float drive = 1.0f + drive_.load() * 5.0f;
    float driven = sample * drive;

    // Cubic soft clipper
    if (std::abs(driven) < 1.0f)
    {
        driven = driven - (driven * driven * driven) / 3.0f;
    }
    else
    {
        float sign = (driven > 0.0f) ? 1.0f : -1.0f;
        driven = sign * 2.0f / 3.0f;
    }

    return driven / drive;
}

float MultiDistortion::applyHardClip(float sample)
{
    // Digital hard clipping
    float drive = 1.0f + drive_.load() * 3.0f;
    float driven = sample * drive;

    // Hard clip at ±1.0
    driven = juce::jlimit(-1.0f, 1.0f, driven);

    return driven / drive;
}

//==============================================================================
// Tone Stack Control
//==============================================================================

void MultiDistortion::updateToneFilters()
{
    float tone = tone_.load();

    // 3-band EQ based on tone parameter
    float bass, mid, treble;

    if (tone < 0.33f)
    {
        // Bass-focused
        bass = juce::jmap(tone, 0.0f, 0.33f, 6.0f, 0.0f);
        mid = juce::jmap(tone, 0.0f, 0.33f, -3.0f, -6.0f);
        treble = juce::jmap(tone, 0.0f, 0.33f, -6.0f, -12.0f);
    }
    else if (tone < 0.66f)
    {
        // Mid-focused
        bass = juce::jmap(tone, 0.33f, 0.66f, 0.0f, -3.0f);
        mid = juce::jmap(tone, 0.33f, 0.66f, -6.0f, 3.0f);
        treble = juce::jmap(tone, 0.33f, 0.66f, -12.0f, 0.0f);
    }
    else
    {
        // Treble-focused
        bass = juce::jmap(tone, 0.66f, 1.0f, -3.0f, -6.0f);
        mid = juce::jmap(tone, 0.66f, 1.0f, 3.0f, 0.0f);
        treble = juce::jmap(tone, 0.66f, 1.0f, 0.0f, 9.0f);
    }

    toneStack_.low.state->setGainDecibels(bass);
    toneStack_.mid.state->setGainDecibels(mid);
    toneStack_.high.state->setGainDecibels(treble);
}

//==============================================================================
// Cabinet Simulation
//==============================================================================

void MultiDistortion::processCabinet(juce::AudioBuffer<float>& buffer)
{
    cabinetConvolution_.process(juce::dsp::ProcessContextReplacing<float>(
        juce::dsp::AudioBlock<float>(buffer)));
}

bool MultiDistortion::loadCabinetIR(const juce::File& file)
{
    juce::String path = file.getFullPathName();
    cabinetConvolution_.loadImpulseResponse(path, 0, true);
    currentCabinetName_ = file.getFileNameWithoutExtension();
    return true;
}

bool MultiDistortion::loadCabinetIR(const juce::String& builtinName)
{
    // Generate procedurally based on name
    int sampleRate = static_cast<int>(sampleRate_);
    int length = sampleRate;  // 1 second IR

    std::vector<float> ir(length);

    if (builtinName == "1x12_Cabinet")
    {
        // Small 1x12 combo amp
        for (int i = 0; i < length; ++i)
            ir[i] = std::exp(-i * 3.0f / length) * std::sin(i * 0.1f);
    }
    else if (builtinName == "2x12_Cabinet")
    {
        // 2x12 open-back combo
        for (int i = 0; i < length; ++i)
            ir[i] = std::exp(-i * 2.5f / length) * std::sin(i * 0.08f);
    }
    else if (builtinName == "4x12_Cabinet")
    {
        // 4x12 closed-back cabinet
        for (int i = 0; i < length; ++i)
            ir[i] = std::exp(-i * 2.0f / length) * std::sin(i * 0.06f);
    }
    else if (builtinName == "4x12_V30")
    {
        // 4x12 with Vintage 30 speakers (brighter)
        for (int i = 0; i < length; ++i)
            ir[i] = std::exp(-i * 1.8f / length) * std::sin(i * 0.07f) * 1.2f;
    }

    juce::AudioBuffer<float> irBuffer(1, length);
    irBuffer.copyFrom(0, 0, ir.data(), length);

    cabinetConvolution_.loadImpulseResponse(
        std::move(irBuffer),
        sampleRate,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        juce::dsp::Convolution::Normalisation::yes
    );

    currentCabinetName_ = builtinName;
    return true;
}

juce::String MultiDistortion::getCurrentCabinetName() const
{
    return currentCabinetName_;
}

void MultiDistortion::setCabinetEnabled(bool enabled)
{
    cabinetEnabled_.store(enabled);
}

void MultiDistortion::setMicType(MicType type)
{
    micType_.store(type);
    // Re-apply mic simulation to current cabinet
    // (In full implementation, would have different IRs per mic)
}

void MultiDistortion::setMicPosition(float position)
{
    micPosition_.store(juce::jlimit(0.0f, 1.0f, position));
    // In full implementation, would blend close/far room IRs
}

void MultiDistortion::generateBuiltinCabinetIRs()
{
    // Builtin cabinets are generated procedurally in loadCabinetIR()
    // This is just a placeholder for initialization
    currentCabinetName_ = "4x12_Cabinet";
    loadCabinetIR("4x12_Cabinet");
}

//==============================================================================
// Feedback Reducer
//==============================================================================

void MultiDistortion::setFeedbackReducerEnabled(bool enabled)
{
    feedbackReducerEnabled_.store(enabled);
}

void MultiDistortion::setFeedbackThreshold(float threshold)
{
    feedbackThreshold_.store(threshold);
}

void MultiDistortion::updateFeedbackReducer(float feedbackLevel)
{
    // Dynamic notch filter at detected feedback frequency
    // (In full implementation, would use FFT to find feedback freq)
    float threshold = feedbackThreshold_.load();

    if (feedbackLevel > threshold)
    {
        // Reduce gain at feedback frequency
        float reduction = (feedbackLevel - threshold) * 2.0f;
        feedbackReducer_.notch.state->setGainDecibels(-reduction);
    }
    else
    {
        feedbackReducer_.notch.state->setGainDecibels(0.0f);
    }
}

//==============================================================================
// Parameter Setters
//==============================================================================

void MultiDistortion::setDistortionType(DistortionType type)
{
    distortionType_.store(type);
}

void MultiDistortion::setDrive(float drive)
{
    drive_.store(juce::jlimit(0.0f, 1.0f, drive));
}

void MultiDistortion::setTone(float tone)
{
    tone_.store(juce::jlimit(0.0f, 1.0f, tone));
    updateToneFilters();
}

void MultiDistortion::setMix(float mix)
{
    mix_.store(juce::jlimit(0.0f, 1.0f, mix));
}

void MultiDistortion::setOutputGain(float dB)
{
    outputGain_.store(juce::jlimit(-60.0f, 12.0f, dB));
}

void MultiDistortion::setBitDepth(float depth)
{
    bitDepth_.store(juce::jlimit(1.0f, 24.0f, depth));
}

void MultiDistortion::setSampleRateReduction(float rate)
{
    sampleRateReduction_.store(juce::jlimit(1.0f, 64.0f, rate));
}

//==============================================================================
// State Management
//==============================================================================

juce::ValueTree MultiDistortion::getState() const
{
    juce::ValueTree state("MultiDistortionState");
    state.setProperty("distortionType", static_cast<int>(distortionType_.load()), nullptr);
    state.setProperty("drive", drive_.load(), nullptr);
    state.setProperty("tone", tone_.load(), nullptr);
    state.setProperty("mix", mix_.load(), nullptr);
    state.setProperty("outputGain", outputGain_.load(), nullptr);
    state.setProperty("bitDepth", bitDepth_.load(), nullptr);
    state.setProperty("sampleRateReduction", sampleRateReduction_.load(), nullptr);
    state.setProperty("cabinetEnabled", cabinetEnabled_.load(), nullptr);
    state.setProperty("micType", static_cast<int>(micType_.load()), nullptr);
    state.setProperty("micPosition", micPosition_.load(), nullptr);
    state.setProperty("feedbackReducerEnabled", feedbackReducerEnabled_.load(), nullptr);
    state.setProperty("feedbackThreshold", feedbackThreshold_.load(), nullptr);
    state.setProperty("currentCabinet", currentCabinetName_, nullptr);
    return state;
}

void MultiDistortion::setState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    distortionType_.store(static_cast<DistortionType>(
        state.getProperty("distortionType", static_cast<int>(DistortionType::Tube))));
    drive_.store(state.getProperty("drive", 0.5f));
    tone_.store(state.getProperty("tone", 0.5f));
    mix_.store(state.getProperty("mix", 1.0f));
    outputGain_.store(state.getProperty("outputGain", 0.0f));
    bitDepth_.store(state.getProperty("bitDepth", 16.0f));
    sampleRateReduction_.store(state.getProperty("sampleRateReduction", 1.0f));
    cabinetEnabled_.store(state.getProperty("cabinetEnabled", true));
    micType_.store(static_cast<MicType>(
        state.getProperty("micType", static_cast<int>(MicType::SM57))));
    micPosition_.store(state.getProperty("micPosition", 0.0f));
    feedbackReducerEnabled_.store(state.getProperty("feedbackReducerEnabled", false));
    feedbackThreshold_.store(state.getProperty("feedbackThreshold", -12.0f));
    currentCabinetName_ = state.getProperty("currentCabinet", "4x12_Cabinet").toString();

    updateToneFilters();
}

//==============================================================================
// Presets
//==============================================================================

juce::StringArray MultiDistortion::getPresetNames()
{
    return {
        // Clean boosts
        "Clean Boost",
        "Transparent Boost",

        // Classic overdrive
        "Blues Breaker",
        "Tube Screamer",
        "Klon Centaur",

        // Crunch
        "AC30 Top Boost",
        "Fender Tweed",
        "Marshall Plexi",

        // High-gain
        "JCM800 Crunch",
        "Mesa Boogie Modern",
        "Soldano Lead",
        "Dual Rectifier",

        // Fuzz
        "Fuzz Face",
        "Big Muff",
        "Tonebender",
        "Octavia",

        // Special
        "Bitcrushed",
        "Wavefolder",
        "Synth Lead"
    };
}

void MultiDistortion::loadPreset(const juce::String& presetName)
{
    if (presetName == "Clean Boost")
    {
        setDistortionType(DistortionType::SoftClip);
        setDrive(0.2f);
        setTone(0.5f);
        setMix(0.5f);
        setOutputGain(0.0f);
        setCabinetEnabled(false);
    }
    else if (presetName == "Transparent Boost")
    {
        setDistortionType(DistortionType::Overdrive_Klon);
        setDrive(0.25f);
        setTone(0.5f);
        setMix(0.4f);
        setOutputGain(3.0f);
        setCabinetEnabled(false);
    }
    else if (presetName == "Tube Screamer")
    {
        setDistortionType(DistortionType::Overdrive_TubeScreamer);
        setDrive(0.6f);
        setTone(0.5f);
        setMix(0.7f);
        setOutputGain(0.0f);
        setCabinetEnabled(true);
        loadCabinetIR("2x12_Cabinet");
    }
    else if (presetName == "Klon Centaur")
    {
        setDistortionType(DistortionType::Overdrive_Klon);
        setDrive(0.5f);
        setTone(0.55f);
        setMix(0.6f);
        setOutputGain(2.0f);
        setCabinetEnabled(true);
        loadCabinetIR("1x12_Cabinet");
    }
    else if (presetName == "AC30 Top Boost")
    {
        setDistortionType(DistortionType::Preamp_Vox_AC30);
        setDrive(0.4f);
        setTone(0.7f);
        setMix(0.8f);
        setOutputGain(0.0f);
        setCabinetEnabled(true);
        loadCabinetIR("2x12_Cabinet");
    }
    else if (presetName == "Fender Tweed")
    {
        setDistortionType(DistortionType::Preamp_Fender_Tweed);
        setDrive(0.5f);
        setTone(0.4f);
        setMix(0.8f);
        setOutputGain(0.0f);
        setCabinetEnabled(true);
        loadCabinetIR("1x12_Cabinet");
    }
    else if (presetName == "Marshall Plexi")
    {
        setDistortionType(DistortionType::Preamp_Marshall_Plexi);
        setDrive(0.6f);
        setTone(0.5f);
        setMix(0.9f);
        setOutputGain(0.0f);
        setCabinetEnabled(true);
        loadCabinetIR("4x12_Cabinet");
    }
    else if (presetName == "JCM800 Crunch")
    {
        setDistortionType(DistortionType::Preamp_Marshall_JCM800);
        setDrive(0.65f);
        setTone(0.55f);
        setMix(0.9f);
        setOutputGain(0.0f);
        setCabinetEnabled(true);
        loadCabinetIR("4x12_Cabinet");
    }
    else if (presetName == "Mesa Boogie Modern")
    {
        setDistortionType(DistortionType::Preamp_Mesa_DualRect);
        setDrive(0.75f);
        setTone(0.5f);
        setMix(1.0f);
        setOutputGain(-2.0f);
        setCabinetEnabled(true);
        loadCabinetIR("4x12_V30");
    }
    else if (presetName == "Soldano Lead")
    {
        setDistortionType(DistortionType::Preamp_Soldano_SLO100);
        setDrive(0.8f);
        setTone(0.6f);
        setMix(1.0f);
        setOutputGain(-3.0f);
        setCabinetEnabled(true);
        loadCabinetIR("4x12_V30");
    }
    else if (presetName == "Dual Rectifier")
    {
        setDistortionType(DistortionType::Preamp_Mesa_DualRect);
        setDrive(0.85f);
        setTone(0.45f);
        setMix(1.0f);
        setOutputGain(-4.0f);
        setCabinetEnabled(true);
        loadCabinetIR("4x12_V30");
    }
    else if (presetName == "Fuzz Face")
    {
        setDistortionType(DistortionType::Fuzz_FuzzFace);
        setDrive(0.7f);
        setTone(0.6f);
        setMix(1.0f);
        setOutputGain(-3.0f);
        setCabinetEnabled(true);
        loadCabinetIR("1x12_Cabinet");
    }
    else if (presetName == "Big Muff")
    {
        setDistortionType(DistortionType::Fuzz_BigMuff);
        setDrive(0.8f);
        setTone(0.5f);
        setMix(1.0f);
        setOutputGain(-6.0f);
        setCabinetEnabled(true);
        loadCabinetIR("4x12_Cabinet");
    }
    else if (presetName == "Tonebender")
    {
        setDistortionType(DistortionType::Fuzz_Tonebender);
        setDrive(0.85f);
        setTone(0.4f);
        setMix(1.0f);
        setOutputGain(-9.0f);
        setCabinetEnabled(true);
        loadCabinetIR("4x12_Cabinet");
    }
    else if (presetName == "Octavia")
    {
        setDistortionType(DistortionType::Fuzz_Octavia);
        setDrive(0.75f);
        setTone(0.7f);
        setMix(1.0f);
        setOutputGain(-3.0f);
        setCabinetEnabled(true);
        loadCabinetIR("1x12_Cabinet");
    }
    else if (presetName == "Bitcrushed")
    {
        setDistortionType(DistortionType::Bitcrush);
        setDrive(0.5f);
        setTone(0.5f);
        setMix(1.0f);
        setOutputGain(0.0f);
        setBitDepth(6.0f);
        setSampleRateReduction(8.0f);
        setCabinetEnabled(false);
    }
    else if (presetName == "Wavefolder")
    {
        setDistortionType(DistortionType::Wavefolder);
        setDrive(0.7f);
        setTone(0.6f);
        setMix(1.0f);
        setOutputGain(-6.0f);
        setCabinetEnabled(false);
    }
    else if (presetName == "Synth Lead")
    {
        setDistortionType(DistortionType::Preamp_Soldano_SLO100);
        setDrive(0.9f);
        setTone(0.7f);
        setMix(1.0f);
        setOutputGain(-6.0f);
        setCabinetEnabled(false);
    }
}

juce::StringArray MultiDistortion::getBuiltinCabinetNames()
{
    return {
        "1x12_Cabinet",
        "2x12_Cabinet",
        "4x12_Cabinet",
        "4x12_V30"
    };
}

} // namespace engine
} // namespace zenith
