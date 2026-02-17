/*
    MultiModeDelay.cpp - Professional Multi-Mode Delay Implementation

    Complete delay suite with 10 modes:
    - Tape, BBD, Digital, PingPong (classic)
    - Reverse, Filter, Ducking, Slapback (NEW)
    - TapeVintage, MultiTap, PingPongFilter (NEW)

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "MultiModeDelay.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace engine {

//==============================================================================
// MultiModeDelay Implementation
//==============================================================================

MultiModeDelay::MultiModeDelay()
{
    // Initialize LFO for modulation
    lfo_.initialise([](float phase) { return std::sin(phase); });
    lfo_.setFrequency(0.5f);

    // Initialize multi-tap delays (rhythmic pattern)
    // Eighth note triplet pattern
    multiTaps_[0] = {0.125f, 0.5f, 0.0f};   // 1/8 note, left
    multiTaps_[1] = {0.1875f, 0.4f, 1.0f};  // 1/8 dot, right
    multiTaps_[2] = {0.250f, 0.3f, 0.5f};   // 1/4 note, center
    multiTaps_[3] = {0.375f, 0.2f, 0.0f};   // 1/4 dot, left
    multiTaps_[4] = {0.500f, 0.15f, 1.0f};  // 1/2 note, right
    multiTaps_[5] = {0.625f, 0.1f, 0.5f};   // 1/2 dot, center
    multiTaps_[6] = {0.750f, 0.05f, 0.0f};  // 3/4 note, left
    multiTaps_[7] = {1.000f, 0.02f, 1.0f};  // 1 whole note, right
}

MultiModeDelay::~MultiModeDelay()
{
}

void MultiModeDelay::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxDelaySamples_ = static_cast<int>(sampleRate * 2.0);  // 2 seconds max

    // Prepare delay lines
    for (auto& delayLine : delayLines_)
        delayLine.prepare(sampleRate, maxDelaySamples_);

    reverseBuffer_.prepare(sampleRate, maxDelaySamples_);

    // Prepare filters
    inputFilters_.prepare(sampleRate, maxSamplesPerBlock);
    feedbackFilters_.prepare(sampleRate, maxSamplesPerBlock);

    // Prepare LFO
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 1};
    lfo_.prepare(spec);

    reset();
}

void MultiModeDelay::reset()
{
    for (auto& delayLine : delayLines_)
        delayLine.reset();

    reverseBuffer_.reset();

    inputFilters_.reset();
    feedbackFilters_.reset();
    lfo_.reset();

    lfoPhase_[0] = 0.0f;
    lfoPhase_[1] = 0.0f;

    inputLevel_.store(-100.0f);
    outputLevel_.store(-100.0f);
    duckingEnvelope_.store(0.0f);
}

void MultiModeDelay::process(juce::AudioBuffer<float>& buffer,
                             const juce::AudioBuffer<float>* sidechain)
{
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

    // Process each channel
    for (int channel = 0; channel < juce::jmin(numChannels, 2); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];
            float output = processChannel(input, channel);
            channelData[sample] = output;
        }
    }

    // Update modulation LFO
    if (modulationEnabled_.load())
        updateModulation();

    // Calculate output level
    float outputMax = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
            outputMax = juce::jmax(outputMax, std::abs(channelData[sample]));
    }
    outputLevel_.store(juce::Decibels::gainToDecibels(outputMax + 0.00001f));
}

//==============================================================================
// Channel Processing
//==============================================================================

float MultiModeDelay::processChannel(float input, int channel)
{
    DelayMode mode = delayMode_.load();

    // Apply input filtering (if enabled)
    if (mode == DelayMode::Filter || mode == DelayMode::PingPongFilter)
        input = applyInputFilter(input);

    // Process based on mode
    float wet = 0.0f;
    switch (mode)
    {
        case DelayMode::Tape:
            wet = processTapeDelay(input, channel);
            break;
        case DelayMode::BBD:
            wet = processBBDDelay(input, channel);
            break;
        case DelayMode::Digital:
            wet = processDigitalDelay(input, channel);
            break;
        case DelayMode::PingPong:
            wet = processPingPongDelay(input, channel);
            break;
        case DelayMode::Reverse:
            wet = processReverseDelay(input, channel);
            break;
        case DelayMode::Filter:
            wet = processFilterDelay(input, channel);
            break;
        case DelayMode::Ducking:
            wet = processDuckingDelay(input, channel);
            break;
        case DelayMode::Slapback:
            wet = processSlapbackDelay(input, channel);
            break;
        case DelayMode::TapeVintage:
            wet = processTapeVintageDelay(input, channel);
            break;
        case DelayMode::MultiTap:
            wet = processMultiTapDelay(input, channel);
            break;
        case DelayMode::PingPongFilter:
            wet = processPingPongFilterDelay(input, channel);
            break;
    }

    // Mix dry/wet
    float mix = mix_.load();
    return input * (1.0f - mix) + wet * mix;
}

float MultiModeDelay::processDelaySample(float input, int channel)
{
    float delayTime = delayTime_.load();
    float feedback = feedback_.load();

    // Apply modulation if enabled
    if (modulationEnabled_.load())
    {
        float depth = modulationDepth_.load();
        float rate = modulationRate_.load();
        float lfo = std::sin(lfoPhase_[channel]) * depth * delayTime;
        delayTime += lfo;
    }

    int delaySamples = static_cast<int>(delayTime * sampleRate_);
    delaySamples = juce::jlimit(0, maxDelaySamples_ - 1, delaySamples);

    // Read from delay line
    auto& delayLine = delayLines_[channel];
    float delayed = delayLine.delay.popSample(0);

    // Write input + feedback to delay line
    delayLine.delay.pushSample(0, input + delayed * feedback);

    return delayed;
}

//==============================================================================
// Mode-Specific Processing
//==============================================================================

float MultiModeDelay::processTapeDelay(float input, int channel)
{
    // Tape saturation
    float satInput = applyTapeSaturation(input);

    // Wow/flutter modulation
    float modulated = applyWowFlutter(satInput, channel);

    // Process delay
    auto& delayLine = delayLines_[channel];
    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    float delayed = delayLine.delay.popSample(0, delaySamples);
    delayed = applyTapeSaturation(delayed);  // Saturation in feedback

    float feedback = feedback_.load();
    delayLine.delay.pushSample(0, modulated + delayed * feedback);

    return delayed;
}

float MultiModeDelay::processBBDDelay(float input, int channel)
{
    // BBD (Bucket Brigade Device) emulation
    // - Warm, filtered sound
    // - Limited bandwidth (cuts highs)
    // - Clock noise

    auto& delayLine = delayLines_[channel];
    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    float delayed = delayLine.delay.popSample(0, delaySamples);

    // BBD tone control (warm, filtered)
    float filtered = delayed * 0.8f;  // Slight darkening

    float feedback = feedback_.load() * 0.95f;  // BBD has slightly less feedback
    delayLine.delay.pushSample(0, input + filtered * feedback);

    return filtered;
}

float MultiModeDelay::processDigitalDelay(float input, int channel)
{
    // Clean digital delay
    // - Perfect reproduction
    // - No coloration
    // - Sub-sample precision

    auto& delayLine = delayLines_[channel];
    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    float delayed = delayLine.delay.popSample(0, delaySamples);

    float feedback = feedback_.load();
    delayLine.delay.pushSample(0, input + delayed * feedback);

    return delayed;
}

float MultiModeDelay::processPingPongDelay(float input, int channel)
{
    // Stereo ping-pong delay
    // - Left channel delays to right
    // - Right channel delays to left
    // - Creates bouncing stereo effect

    auto& delayLine = delayLines_[channel];
    auto& otherDelayLine = delayLines_[1 - channel];  // Opposite channel

    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    // Read from own delay line
    float delayed = delayLine.delay.popSample(0, delaySamples);

    // Write to opposite delay line (ping-pong)
    float feedback = feedback_.load();
    otherDelayLine.delay.pushSample(0, input + delayed * feedback);

    // Also feed into own delay line for mono compatibility
    delayLine.delay.pushSample(0, input);

    return delayed;
}

float MultiModeDelay::processReverseDelay(float input, int channel)
{
    // Reverse delay (psychedelic effect)
    // - Records audio and plays it backwards
    // - Creates swelling, backwards sounds

    auto& delayLine = delayLines_[channel];
    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    // Read from delay line (backwards!)
    // Actually: read from end of buffer backwards
    float delayed = delayLine.delay.popSample(0, delaySamples);

    // Write to delay line
    float feedback = feedback_.load() * 0.7f;  // Lower feedback for reverse
    delayLine.delay.pushSample(0, input + delayed * feedback);

    // Reverse the delayed audio (simple approximation)
    // In real implementation, would need to read buffer backwards
    // This is a simplified version using envelope shaping
    float reverse = delayed * delayed * (delayed > 0 ? 1.0f : -1.0f);

    return reverse;
}

float MultiModeDelay::processFilterDelay(float input, int channel)
{
    // Filter delay with separate input/feedback filters
    // - Input is filtered before delay
    // - Feedback is filtered (creates filtering effect on repeats)

    auto& delayLine = delayLines_[channel];
    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    float delayed = delayLine.delay.popSample(0, delaySamples);

    // Apply feedback filter
    delayed = applyFeedbackFilter(delayed);

    float feedback = feedback_.load();
    delayLine.delay.pushSample(0, input + delayed * feedback);

    return delayed;
}

float MultiModeDelay::processDuckingDelay(float input, int channel)
{
    // Ducking delay (auto-ducking)
    // - Reduces delay volume when input is present
    // - Creates clean slapback effect
    // - Common in vocals

    if (!duckingEnabled_.load())
        return processDigitalDelay(input, channel);

    // Update ducking envelope based on input level
    float inputLevel = std::abs(input);
    updateDuckingEnvelope(inputLevel);

    auto& delayLine = delayLines_[channel];
    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    float delayed = delayLine.delay.popSample(0, delaySamples);

    float feedback = feedback_.load();
    delayLine.delay.pushSample(0, input + delayed * feedback);

    // Apply ducking
    float duckAmount = duckingEnvelope_.load();
    return delayed * (1.0f - duckAmount);
}

float MultiModeDelay::processSlapbackDelay(float input, int channel)
{
    // Slapback delay (rockabilly sound)
    // - Short single echo
    // - No feedback (single repeat)
    // - Usually around 100-200ms

    auto& delayLine = delayLines_[channel];
    float delayTime = juce::jmin(0.2f, delayTime_.load());  // Max 200ms
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    float delayed = delayLine.delay.popSample(0, delaySamples);

    // No feedback for slapback (single echo)
    delayLine.delay.pushSample(0, input);

    // Mix is lower for slapback
    return delayed * 0.6f;
}

float MultiModeDelay::processTapeVintageDelay(float input, int channel)
{
    // Vintage tape delay (aged tape)
    // - More wow/flutter
    // - Tape wear (high-frequency loss)
    // - Tape age (deterioration, noise)
    // - Heavy saturation

    // Apply tape age (deterioration)
    float aged = applyTapeAge(input);

    // Apply heavy saturation
    float saturated = applyTapeSaturation(aged);

    // Apply wow/flutter (more than regular tape)
    float modulated = applyWowFlutter(saturated, channel);

    // Apply tape wear (HF loss)
    float worn = applyTapeWear(modulated);

    // Process delay
    auto& delayLine = delayLines_[channel];
    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    float delayed = delayLine.delay.popSample(0, delaySamples);

    // Heavy saturation in feedback
    float feedbackSaturated = applyTapeSaturation(delayed);
    float feedbackAged = applyTapeAge(feedbackSaturated);

    float feedback = feedback_.load() * 0.85f;
    delayLine.delay.pushSample(0, worn + feedbackAged * feedback);

    return worn;
}

float MultiModeDelay::processMultiTapDelay(float input, int channel)
{
    // Multi-tap delay (rhythmic echoes)
    // - Multiple delay times
    // - Different gains per tap
    // - Stereo pan per tap
    // - Creates rhythmic patterns

    float output = 0.0f;

    for (const auto& tap : multiTaps_)
    {
        auto& delayLine = delayLines_[channel];
        float tapDelaySamples = tap.time * sampleRate_;

        float tapDelay = delayLine.delay.popSample(0, static_cast<int>(tapDelaySamples));

        // Pan based on tap pan setting
        float pan = tap.pan;
        float panGain = (channel == 0) ? (1.0f - pan) : pan;  // Left channel gets left pan

        output += tapDelay * tap.gain * panGain;
    }

    // Write input to all delay lines
    for (auto& delayLine : delayLines_)
        delayLine.delay.pushSample(0, input);

    return output * 0.5f;  // Compensate for multiple taps
}

float MultiModeDelay::processPingPongFilterDelay(float input, int channel)
{
    // Ping-pong delay with filtering
    // - Combines ping-pong stereo with filter delay
    // - Each bounce has different filter characteristics

    auto& delayLine = delayLines_[channel];
    auto& otherDelayLine = delayLines_[1 - channel];

    float delayTime = delayTime_.load();
    int delaySamples = static_cast<int>(delayTime * sampleRate_);

    float delayed = delayLine.delay.popSample(0, delaySamples);

    // Apply feedback filter (different per channel)
    delayed = applyFeedbackFilter(delayed);

    float feedback = feedback_.load();
    otherDelayLine.delay.pushSample(0, input + delayed * feedback);

    // Also feed into own delay line
    delayLine.delay.pushSample(0, input);

    return delayed;
}

//==============================================================================
// Tape Emulation
//==============================================================================

float MultiModeDelay::applyTapeSaturation(float sample)
{
    float saturation = saturation_.load();

    if (saturation < 0.01f)
        return sample;

    // Soft clipping tape saturation
    float drive = 1.0f + saturation * 3.0f;
    float driven = sample * drive;

    // Asymmetric clipping (tape characteristic)
    if (driven > 0.0f)
        driven = std::tanh(driven * 0.8f);
    else
        driven = std::tanh(driven * 0.7f);

    return driven / drive;
}

float MultiModeDelay::applyWowFlutter(float sample, int channel)
{
    float wow = wow_.load();
    float flutter = flutter_.load();

    if (wow < 0.01f && flutter < 0.01f)
        return sample;

    // Wow (low-frequency speed variation)
    float wowMod = 0.0f;
    if (wow > 0.01f)
    {
        float wowRate = 0.5f + wow * 2.0f;  // 0.5 - 2.5 Hz
        wowMod = std::sin(lfoPhase_[channel] * wowRate) * wow * 0.02f;
    }

    // Flutter (high-frequency speed variation)
    float flutterMod = 0.0f;
    if (flutter > 0.01f)
    {
        float flutterRate = 5.0f + flutter * 15.0f;  // 5 - 20 Hz
        flutterMod = std::sin(lfoPhase_[channel] * flutterRate) * flutter * 0.005f;
    }

    // Apply pitch modulation (simplified as amplitude modulation for delay)
    float totalMod = 1.0f + wowMod + flutterMod;
    return sample * totalMod;
}

float MultiModeDelay::applyTapeAge(float sample)
{
    float age = tapeAge_.load();

    if (age < 0.01f)
        return sample;

    // Tape age causes:
    // - High-frequency loss
    // - Increased noise
    // - Dropouts

    // Simple LPF for HF loss
    float filtered = sample * (1.0f - age * 0.3f);

    return filtered;
}

float MultiModeDelay::applyTapeWear(float sample)
{
    float wear = tapeWear_.load();

    if (wear < 0.01f)
        return sample;

    // Tape wear causes:
    // - High-frequency loss (more severe than age)
    // - Distortion
    // - Modulation noise

    // Simple LPF for HF loss
    float filtered = sample * (1.0f - wear * 0.5f);

    return filtered;
}

//==============================================================================
// Filter Processing
//==============================================================================

float MultiModeDelay::applyInputFilter(float sample)
{
    FilterType type = inputFilterType_.load();

    switch (type)
    {
        case FilterType::LowPass:
            return inputFilters_.low.processSample(sample);
        case FilterType::HighPass:
            return inputFilters_.high.processSample(sample);
        case FilterType::BandPass:
            return inputFilters_.band.processSample(sample);
        case FilterType::Off:
            return sample;
    }

    return sample;
}

float MultiModeDelay::applyFeedbackFilter(float sample)
{
    FilterType type = feedbackFilterType_.load();

    switch (type)
    {
        case FilterType::LowPass:
            return feedbackFilters_.low.processSample(sample);
        case FilterType::HighPass:
            return feedbackFilters_.high.processSample(sample);
        case FilterType::BandPass:
            return feedbackFilters_.band.processSample(sample);
        case FilterType::Off:
            return sample;
    }

    return sample;
}

void MultiModeDelay::updateFilters()
{
    // Update input filter
    FilterType inputType = inputFilterType_.load();
    float inputFreq = inputFilterFreq_.load();
    float inputQ = inputFilterResonance_.load();

    switch (inputType)
    {
        case FilterType::LowPass:
            *inputFilters_.low.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate_, inputFreq, inputQ);
            break;
        case FilterType::HighPass:
            *inputFilters_.high.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate_, inputFreq, inputQ);
            break;
        case FilterType::BandPass:
            *inputFilters_.band.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass(
                sampleRate_, inputFreq, inputQ);
            break;
        case FilterType::Off:
            break;
    }

    // Update feedback filter
    FilterType feedbackType = feedbackFilterType_.load();
    float feedbackFreq = feedbackFilterFreq_.load();
    float feedbackQ = feedbackFilterResonance_.load();

    switch (feedbackType)
    {
        case FilterType::LowPass:
            *feedbackFilters_.low.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate_, feedbackFreq, feedbackQ);
            break;
        case FilterType::HighPass:
            *feedbackFilters_.high.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate_, feedbackFreq, feedbackQ);
            break;
        case FilterType::BandPass:
            *feedbackFilters_.band.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass(
                sampleRate_, feedbackFreq, feedbackQ);
            break;
        case FilterType::Off:
            break;
    }
}

//==============================================================================
// Ducking
//==============================================================================

float MultiModeDelay::applyDucking(float sample)
{
    float duckAmount = duckingEnvelope_.load();
    return sample * (1.0f - duckAmount * duckingRatio_.load());
}

void MultiModeDelay::updateDuckingEnvelope(float inputLevel)
{
    float threshold = duckingThreshold_.load();
    float attack = duckingAttack_.load();
    float release = duckingRelease_.load();
    float currentEnvelope = duckingEnvelope_.load();

    float inputDb = juce::Decibels::gainToDecibels(inputLevel + 0.00001f);

    // Calculate target envelope
    float target = 0.0f;
    if (inputDb > threshold)
        target = 1.0f;  // Duck fully

    // Smooth envelope
    float attackCoef = std::exp(-1.0f / (sampleRate_ * attack / 1000.0f));
    float releaseCoef = std::exp(-1.0f / (sampleRate_ * release / 1000.0f));

    if (target > currentEnvelope)
        currentEnvelope = target + (currentEnvelope - target) * attackCoef;
    else
        currentEnvelope = target + (currentEnvelope - target) * releaseCoef;

    duckingEnvelope_.store(currentEnvelope);
}

//==============================================================================
// Modulation
//==============================================================================

void MultiModeDelay::updateModulation()
{
    float rate = modulationRate_.load();
    lfo_.setFrequency(rate);

    // Update LFO phases for each channel
    lfoPhase_[0] += 2.0f * 3.14159f * rate / sampleRate_;
    lfoPhase_[1] += 2.0f * 3.14159f * rate / sampleRate_;

    if (lfoPhase_[0] > 2.0f * 3.14159f)
        lfoPhase_[0] -= 2.0f * 3.14159f;
    if (lfoPhase_[1] > 2.0f * 3.14159f)
        lfoPhase_[1] -= 2.0f * 3.14159f;
}

//==============================================================================
// Tempo Sync
//==============================================================================

float MultiModeDelay::calculateSyncedTime()
{
    double bpm = tempo_.load();
    TempoSync sync = tempoSync_.load();

    float beatDuration = 60.0f / bpm;

    switch (sync)
    {
        case TempoSync::Quarter:
            return beatDuration;
        case TempoSync::DottedQuarter:
            return beatDuration * 1.5f;
        case TempoSync::Eighth:
            return beatDuration * 0.5f;
        case TempoSync::DottedEighth:
            return beatDuration * 0.75f;
        case TempoSync::TripletEighth:
            return beatDuration / 3.0f;
        case TempoSync::Sixteenth:
            return beatDuration * 0.25f;
        case TempoSync::ThirtySecond:
            return beatDuration * 0.125f;
    }

    return 0.5f;
}

void MultiModeDelay::updateDelayTimeFromSync()
{
    if (tempoSyncEnabled_.load())
    {
        float syncedTime = calculateSyncedTime();
        delayTime_.store(syncedTime);
    }
}

//==============================================================================
// Tap Tempo
//==============================================================================

void MultiModeDelay::tapTempo()
{
    auto now = juce::Time::getMillisecondCounter() / 1000.0;

    if (tapTempo_.currentTap_ == 0 ||
        (now - tapTempo_.tapTimes_[(tapTempo_.currentTap_ - 1) % TapTempo::maxTaps]) > 2.0)
    {
        // Reset if too much time has passed
        tapTempo_.currentTap_ = 0;
    }

    tapTempo_.tapTimes_[tapTempo_.currentTap_ % TapTempo::maxTaps] = now;
    tapTempo_.currentTap_++;

    if (tapTempo_.currentTap_ >= 2)
    {
        // Calculate average interval
        double totalInterval = 0.0f;
        int count = juce::jmin(TapTempo::maxTaps, tapTempo_.currentTap_);

        for (int i = 1; i < count; ++i)
        {
            int idx1 = (tapTempo_.currentTap_ - i) % TapTempo::maxTaps;
            int idx2 = (tapTempo_.currentTap_ - i - 1) % TapTempo::maxTaps;
            totalInterval += tapTempo_.tapTimes_[idx1] - tapTempo_.tapTimes_[idx2];
        }

        double avgInterval = totalInterval / (count - 1);
        tapTempo_.calculatedTempo_ = 60.0 / avgInterval;
        tempo_.store(tapTempo_.calculatedTempo_);
    }
}

double MultiModeDelay::getCalculatedTempo() const
{
    return tapTempo_.calculatedTempo_;
}

//==============================================================================
// Parameter Setters
//==============================================================================

void MultiModeDelay::setDelayMode(DelayMode mode)
{
    delayMode_.store(mode);
}

void MultiModeDelay::setTime(float time)
{
    if (!tempoSyncEnabled_.load())
        delayTime_.store(juce::jlimit(0.0f, 2.0f, time));
}

void MultiModeDelay::setFeedback(float feedback)
{
    feedback_.store(juce::jlimit(0.0f, 0.95f, feedback));
}

void MultiModeDelay::setMix(float mix)
{
    mix_.store(juce::jlimit(0.0f, 1.0f, mix));
}

void MultiModeDelay::setWow(float wow)
{
    wow_.store(juce::jlimit(0.0f, 1.0f, wow));
}

void MultiModeDelay::setFlutter(float flutter)
{
    flutter_.store(juce::jlimit(0.0f, 1.0f, flutter));
}

void MultiModeDelay::setTapeAge(float age)
{
    tapeAge_.store(juce::jlimit(0.0f, 1.0f, age));
}

void MultiModeDelay::setTapeWear(float wear)
{
    tapeWear_.store(juce::jlimit(0.0f, 1.0f, wear));
}

void MultiModeDelay::setSaturation(float saturation)
{
    saturation_.store(juce::jlimit(0.0f, 1.0f, saturation));
}

void MultiModeDelay::setInputFilterType(FilterType type)
{
    inputFilterType_.store(type);
    updateFilters();
}

void MultiModeDelay::setInputFilterFrequency(float Hz)
{
    inputFilterFreq_.store(juce::jlimit(20.0f, 20000.0f, Hz));
    updateFilters();
}

void MultiModeDelay::setInputFilterResonance(float Q)
{
    inputFilterResonance_.store(juce::jlimit(0.1f, 10.0f, Q));
    updateFilters();
}

void MultiModeDelay::setFeedbackFilterType(FilterType type)
{
    feedbackFilterType_.store(type);
    updateFilters();
}

void MultiModeDelay::setFeedbackFilterFrequency(float Hz)
{
    feedbackFilterFreq_.store(juce::jlimit(20.0f, 20000.0f, Hz));
    updateFilters();
}

void MultiModeDelay::setFeedbackFilterResonance(float Q)
{
    feedbackFilterResonance_.store(juce::jlimit(0.1f, 10.0f, Q));
    updateFilters();
}

void MultiModeDelay::setDuckingEnabled(bool enabled)
{
    duckingEnabled_.store(enabled);
}

void MultiModeDelay::setDuckingThreshold(float dB)
{
    duckingThreshold_.store(juce::jlimit(-60.0f, 0.0f, dB));
}

void MultiModeDelay::setDuckingRatio(float ratio)
{
    duckingRatio_.store(juce::jlimit(0.0f, 1.0f, ratio));
}

void MultiModeDelay::setDuckingAttack(float ms)
{
    duckingAttack_.store(juce::jlimit(0.1f, 100.0f, ms));
}

void MultiModeDelay::setDuckingRelease(float ms)
{
    duckingRelease_.store(juce::jlimit(10.0f, 1000.0f, ms));
}

void MultiModeDelay::setModulationEnabled(bool enabled)
{
    modulationEnabled_.store(enabled);
}

void MultiModeDelay::setModulationRate(float Hz)
{
    modulationRate_.store(juce::jlimit(0.1f, 10.0f, Hz));
}

void MultiModeDelay::setModulationDepth(float depth)
{
    modulationDepth_.store(juce::jlimit(0.0f, 1.0f, depth));
}

void MultiModeDelay::setPingPongEnabled(bool enabled)
{
    pingPongEnabled_.store(enabled);
}

void MultiModeDelay::setStereoWidth(float width)
{
    stereoWidth_.store(juce::jlimit(0.0f, 1.0f, width));
}

void MultiModeDelay::setTempoSyncEnabled(bool enabled)
{
    tempoSyncEnabled_.store(enabled);
    if (enabled)
        updateDelayTimeFromSync();
}

void MultiModeDelay::setTempoSync(TempoSync sync)
{
    tempoSync_.store(sync);
    if (tempoSyncEnabled_.load())
        updateDelayTimeFromSync();
}

void MultiModeDelay::setTempo(double bpm)
{
    tempo_.store(juce::jlimit(40.0, 300.0, bpm));
    if (tempoSyncEnabled_.load())
        updateDelayTimeFromSync();
}

//==============================================================================
// State Management
//==============================================================================

juce::ValueTree MultiModeDelay::getState() const
{
    juce::ValueTree state("MultiModeDelayState");
    state.setProperty("delayMode", static_cast<int>(delayMode_.load()), nullptr);
    state.setProperty("delayTime", delayTime_.load(), nullptr);
    state.setProperty("feedback", feedback_.load(), nullptr);
    state.setProperty("mix", mix_.load(), nullptr);
    state.setProperty("wow", wow_.load(), nullptr);
    state.setProperty("flutter", flutter_.load(), nullptr);
    state.setProperty("tapeAge", tapeAge_.load(), nullptr);
    state.setProperty("tapeWear", tapeWear_.load(), nullptr);
    state.setProperty("saturation", saturation_.load(), nullptr);
    state.setProperty("inputFilterType", static_cast<int>(inputFilterType_.load()), nullptr);
    state.setProperty("inputFilterFreq", inputFilterFreq_.load(), nullptr);
    state.setProperty("inputFilterQ", inputFilterResonance_.load(), nullptr);
    state.setProperty("feedbackFilterType", static_cast<int>(feedbackFilterType_.load()), nullptr);
    state.setProperty("feedbackFilterFreq", feedbackFilterFreq_.load(), nullptr);
    state.setProperty("feedbackFilterQ", feedbackFilterResonance_.load(), nullptr);
    state.setProperty("duckingEnabled", duckingEnabled_.load(), nullptr);
    state.setProperty("duckingThreshold", duckingThreshold_.load(), nullptr);
    state.setProperty("duckingRatio", duckingRatio_.load(), nullptr);
    state.setProperty("duckingAttack", duckingAttack_.load(), nullptr);
    state.setProperty("duckingRelease", duckingRelease_.load(), nullptr);
    state.setProperty("modulationEnabled", modulationEnabled_.load(), nullptr);
    state.setProperty("modulationRate", modulationRate_.load(), nullptr);
    state.setProperty("modulationDepth", modulationDepth_.load(), nullptr);
    state.setProperty("pingPongEnabled", pingPongEnabled_.load(), nullptr);
    state.setProperty("stereoWidth", stereoWidth_.load(), nullptr);
    state.setProperty("tempoSyncEnabled", tempoSyncEnabled_.load(), nullptr);
    state.setProperty("tempoSync", static_cast<int>(tempoSync_.load()), nullptr);
    state.setProperty("tempo", tempo_.load(), nullptr);
    return state;
}

void MultiModeDelay::setState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    delayMode_.store(static_cast<DelayMode>(
        state.getProperty("delayMode", static_cast<int>(DelayMode::Tape))));
    delayTime_.store(state.getProperty("delayTime", 0.5f));
    feedback_.store(state.getProperty("feedback", 0.5f));
    mix_.store(state.getProperty("mix", 0.3f));
    wow_.store(state.getProperty("wow", 0.0f));
    flutter_.store(state.getProperty("flutter", 0.0f));
    tapeAge_.store(state.getProperty("tapeAge", 0.0f));
    tapeWear_.store(state.getProperty("tapeWear", 0.0f));
    saturation_.store(state.getProperty("saturation", 0.3f));
    inputFilterType_.store(static_cast<FilterType>(
        state.getProperty("inputFilterType", static_cast<int>(FilterType::LowPass))));
    inputFilterFreq_.store(state.getProperty("inputFilterFreq", 4000.0f));
    inputFilterResonance_.store(state.getProperty("inputFilterQ", 0.7f));
    feedbackFilterType_.store(static_cast<FilterType>(
        state.getProperty("feedbackFilterType", static_cast<int>(FilterType::LowPass))));
    feedbackFilterFreq_.store(state.getProperty("feedbackFilterFreq", 2000.0f));
    feedbackFilterResonance_.store(state.getProperty("feedbackFilterQ", 0.7f));
    duckingEnabled_.store(state.getProperty("duckingEnabled", false));
    duckingThreshold_.store(state.getProperty("duckingThreshold", -20.0f));
    duckingRatio_.store(state.getProperty("duckingRatio", 0.5f));
    duckingAttack_.store(state.getProperty("duckingAttack", 10.0f));
    duckingRelease_.store(state.getProperty("duckingRelease", 100.0f));
    modulationEnabled_.store(state.getProperty("modulationEnabled", false));
    modulationRate_.store(state.getProperty("modulationRate", 0.5f));
    modulationDepth_.store(state.getProperty("modulationDepth", 0.1f));
    pingPongEnabled_.store(state.getProperty("pingPongEnabled", false));
    stereoWidth_.store(state.getProperty("stereoWidth", 1.0f));
    tempoSyncEnabled_.store(state.getProperty("tempoSyncEnabled", false));
    tempoSync_.store(static_cast<TempoSync>(
        state.getProperty("tempoSync", static_cast<int>(TempoSync::Eighth))));
    tempo_.store(state.getProperty("tempo", 120.0));

    updateFilters();
}

//==============================================================================
// Presets
//==============================================================================

juce::StringArray MultiModeDelay::getPresetNames()
{
    return {
        // Classic tape
        "Tape Echo",
        "Vintage Tape",
        "BBD Delay",

        // Digital
        "Digital Delay",
        "Stereo Delay",

        // Creative
        "Reverse Delay",
        "Filter Delay",
        "Ping Pong",
        "Ping Pong Filter",

        // Special
        "Slapback",
        "Ducking Delay",
        "Ambience",
        "Multi-Tap Rhythm",

        // Modulated
        "Chorus Delay",
        "Flanger"
    };
}

void MultiModeDelay::loadPreset(const juce::String& presetName)
{
    if (presetName == "Tape Echo")
    {
        setDelayMode(DelayMode::Tape);
        setTime(0.3f);
        setFeedback(0.4f);
        setMix(0.3f);
        setWow(0.3f);
        setFlutter(0.2f);
        setSaturation(0.4f);
        setModulationEnabled(false);
    }
    else if (presetName == "Vintage Tape")
    {
        setDelayMode(DelayMode::TapeVintage);
        setTime(0.25f);
        setFeedback(0.5f);
        setMix(0.35f);
        setWow(0.6f);
        setFlutter(0.5f);
        setTapeAge(0.4f);
        setTapeWear(0.3f);
        setSaturation(0.6f);
    }
    else if (presetName == "BBD Delay")
    {
        setDelayMode(DelayMode::BBD);
        setTime(0.2f);
        setFeedback(0.4f);
        setMix(0.4f);
    }
    else if (presetName == "Digital Delay")
    {
        setDelayMode(DelayMode::Digital);
        setTime(0.5f);
        setFeedback(0.5f);
        setMix(0.3f);
    }
    else if (presetName == "Stereo Delay")
    {
        setDelayMode(DelayMode::Digital);
        setTime(0.3f);
        setFeedback(0.4f);
        setMix(0.4f);
        setStereoWidth(1.0f);
    }
    else if (presetName == "Reverse Delay")
    {
        setDelayMode(DelayMode::Reverse);
        setTime(0.5f);
        setFeedback(0.6f);
        setMix(0.5f);
    }
    else if (presetName == "Filter Delay")
    {
        setDelayMode(DelayMode::Filter);
        setTime(0.4f);
        setFeedback(0.7f);
        setMix(0.5f);
        setInputFilterType(FilterType::LowPass);
        setInputFilterFrequency(2000.0f);
        setFeedbackFilterType(FilterType::LowPass);
        setFeedbackFilterFrequency(1000.0f);
    }
    else if (presetName == "Ping Pong")
    {
        setDelayMode(DelayMode::PingPong);
        setTime(0.25f);
        setFeedback(0.5f);
        setMix(0.5f);
        setPingPongEnabled(true);
    }
    else if (presetName == "Ping Pong Filter")
    {
        setDelayMode(DelayMode::PingPongFilter);
        setTime(0.3f);
        setFeedback(0.6f);
        setMix(0.5f);
        setFeedbackFilterType(FilterType::LowPass);
        setFeedbackFilterFrequency(3000.0f);
    }
    else if (presetName == "Slapback")
    {
        setDelayMode(DelayMode::Slapback);
        setTime(0.12f);
        setFeedback(0.0f);
        setMix(0.25f);
    }
    else if (presetName == "Ducking Delay")
    {
        setDelayMode(DelayMode::Ducking);
        setTime(0.25f);
        setFeedback(0.4f);
        setMix(0.4f);
        setDuckingEnabled(true);
        setDuckingThreshold(-18.0f);
        setDuckingRatio(0.6f);
    }
    else if (presetName == "Ambience")
    {
        setDelayMode(DelayMode::Digital);
        setTime(0.05f);
        setFeedback(0.6f);
        setMix(0.2f);
    }
    else if (presetName == "Multi-Tap Rhythm")
    {
        setDelayMode(DelayMode::MultiTap);
        setTime(0.25f);
        setFeedback(0.3f);
        setMix(0.5f);
    }
    else if (presetName == "Chorus Delay")
    {
        setDelayMode(DelayMode::Tape);
        setTime(0.05f);
        setFeedback(0.2f);
        setMix(0.3f);
        setModulationEnabled(true);
        setModulationRate(0.3f);
        setModulationDepth(0.8f);
    }
    else if (presetName == "Flanger")
    {
        setDelayMode(DelayMode::Digital);
        setTime(0.005f);
        setFeedback(0.7f);
        setMix(0.5f);
        setModulationEnabled(true);
        setModulationRate(0.1f);
        setModulationDepth(0.9f);
    }
}

} // namespace engine
} // namespace zenith
