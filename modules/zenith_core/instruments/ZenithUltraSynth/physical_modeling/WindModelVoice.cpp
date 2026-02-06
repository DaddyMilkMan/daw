/*
  ==============================================================================

    WindModelVoice.cpp
    Created: [Date] Author: Claude AI
    Complete implementation of wind instrument physical modeling

  ==============================================================================
*/

#include "WindModel.h"
#include "../../../DSP/EnvelopeFollower.h"
#include "../../../DSP/PitchDetector.h"

namespace Zenith
{

WindModelVoice::WindModelVoice()
    : frequency_(0.0f)
    , velocity_(0.0f)
    , breathPressure_(0.0f)
    , embouchureType_(Embouchure::Clarinet)
    , isActive_(false)
    , vibratoDepth_(0.0f)
    , vibratoRate_(5.0f)
    , toneColor_(0.5f)
    , growl_(0.0f)
    , reedStiffness_(0.5f)
    , reedOpening_(0.5f)
    , boreLength_(0.5f)
    , boreDiameter_(0.5f)
    , breathNoise_(0.1f)
    , tonguingPosition_(0.5f)
    , tonguingHardness_(0.5f)
    , resonanceBoost_(0.2f)
    , resonanceFrequency_(1000.0f)
    , noiseFloor_(0.001f)
    , nonlinearity_(0.1f)
    , pitchBend_(0.0f)
    , pressure_(0.0f)
    , timbre_(0.0f)
    , sampleRate_(44100.0)
    , bufferSize_(512)
    , currentBreathPressure_(0.0f)
    , vibratoPhase_(0.0f)
    , fundamentalFrequency_(0.0f)
    , voiceStartTime_(0)
    , voiceAge_(0.0f)
    , rmsLevel_(0.0f)
    , peakLevel_(0.0f)
{
    initializeParameters();
    initializeAudioProcessing();
}

WindModelVoice::~WindModelVoice()
{
    reset();
}

void WindModelVoice::initializeParameters()
{
    // Initialize with default parameters
    setBreathPressure(0.5f);
    setEmbouchureType(Embouchure::Clarinet);
    setVibratoDepth(0.0f);
    setVibratoRate(5.0f);
    setToneColor(0.5f);
    setGrowl(0.0f);
    setReedStiffness(0.5f);
    setReedOpening(0.5f);
    setBoreLength(0.5f);
    setBoreDiameter(0.5f);
    setBreathNoise(0.1f);
    setTonguingPosition(0.5f);
    setTonguingHardness(0.5f);
    setResonanceBoost(0.2f);
    setResonanceFrequency(1000.0f);
    setNoiseFloor(0.001f);
    setNonlinearity(0.1f);
    setSmoothingTime(50.0f);
    
    // Initialize MPE parameters
    setPitchBend(0.0f);
    setPressure(0.0f);
    setTimbre(0.0f);
}

void WindModelVoice::initializeAudioProcessing()
{
    // Initialize bore model
    int boreDelayLength = static_cast<int>(sampleRate_ * boreLength_ * 0.01); // Convert to samples
    boreModel_.delayLine.resize(boreDelayLength, 0.0f);
    boreModel_.writeIndex = 0;
    boreModel_.readIndex = 0;
    boreModel_.resonanceGain = 0.8f;
    boreModel_.damping = 0.1f;
    
    // Initialize filters
    formantFilter_ = std::make_unique<juce::dsp::IIR::Filter<float>>();
    brightnessFilter_ = std::make_unique<juce::dsp::IIR::Filter<float>>();
    growlFilter_ = std::make_unique<juce::dsp::IIR::Filter<float>>();
    
    // Initialize noise buffer
    noiseBuffer_.setSize(2, bufferSize_);
    
    // Initialize nonlinearity processor
    nonlinearProcessor_ = std::make_unique<juce::dsp::WaveShaper<float>>();
    updateNonlinearity();
    
    // Initialize smoothed parameters
    smoothedBreath_.reset(sampleRate_, getSmoothingTime() / 1000.0);
    smoothedFrequency_.reset(sampleRate_, getSmoothingTime() / 1000.0);
    smoothedReedOpening_.reset(sampleRate_, getSmoothingTime() / 1000.0);
    
    // Update models
    updateReedModel();
    updateBoreModel();
    updateFormantFilter();
}

void WindModelVoice::noteOn(float frequency, float velocity, float breathPressure)
{
    try {
        frequency_ = frequency;
        velocity_ = juce::jlimit(0.0f, 1.0f, velocity);
        breathPressure_ = juce::jlimit(0.0f, 1.0f, breathPressure);
        isActive_ = true;
        
        voiceStartTime_ = juce::Time::getMillisecondCounterHiRes();
        voiceAge_ = 0.0f;
        
        // Initialize fundamental frequency
        fundamentalFrequency_ = frequency;
        
        // Set smoothed parameters
        smoothedBreath_.setTargetValue(breathPressure_);
        smoothedFrequency_.setTargetValue(frequency);
        
        // Reset bore model
        std::fill(boreModel_.delayLine.begin(), boreModel_.delayLine.end(), 0.0f);
        boreModel_.writeIndex = 0;
        boreModel_.readIndex = 0;
        
        // Apply MPE parameters
        applyMPEParameters();
        
        DBG("WindModelVoice: NoteOn - Freq: " << frequency << " Breath: " << breathPressure);
    }
    catch (const std::exception& e) {
        DBG("WindModelVoice: Error in noteOn - " << e.what());
        isActive_ = false;
    }
}

void WindModelVoice::noteOff()
{
    if (isActive_) {
        // Gradual release
        smoothedBreath_.setTargetValue(0.0f);
        isActive_ = false;
    }
}

void WindModelVoice::reset()
{
    isActive_ = false;
    std::fill(boreModel_.delayLine.begin(), boreModel_.delayLine.end(), 0.0f);
    std::fill(noiseBuffer_.getWritePointer(0), noiseBuffer_.getWritePointer(0) + noiseBuffer_.getNumSamples(), 0.0f);
    
    // Reset reed model
    reedModel_.velocity = 0.0f;
    reedModel_.force = 0.0f;
    
    voiceAge_ = 0.0f;
    rmsLevel_ = 0.0f;
    peakLevel_ = 0.0f;
}

void WindModelVoice::process(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!isActive_ || numSamples <= 0) return;
    
    // Generate breath noise
    generateBreathNoise();
    
    // Process reed excitation
    processReedExcitation(buffer, numSamples);
    
    // Process bore resonance
    processBoreResonance(buffer, numSamples);
    
    // Apply vibrato
    applyVibrato(buffer, numSamples);
    
    // Apply formant filtering
    applyFormantFiltering(buffer, numSamples);
    
    // Apply nonlinearity
    applyNonlinearity(buffer, numSamples);
    
    // Apply growl effect
    if (growl_ > 0.0f) {
        applyGrowl(buffer, numSamples);
    }
    
    // Update analysis data
    updateAnalysis(buffer, numSamples);
    
    // Update voice age
    updateAge();
}

void WindModelVoice::processReedExcitation(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Update reed model
    updateReedModel();
    
    // Calculate jet velocity
    float jetVelocity = calculateJetVelocity();
    
    // Generate reed excitation signal
    for (int sample = 0; sample < numSamples; ++sample) {
        // Get current breath pressure
        float breathPressure = smoothedBreath_.getNextValue();
        
        // Calculate reed force
        float reedForce = calculateReedForce();
        
        // Generate excitation
        float excitation = jetVelocity * breathPressure * reedForce;
        
        // Apply tonguing
        float tonguingImpulse = getTonguingImpulse();
        excitation += tonguingImpulse;
        
        // Add to buffer
        leftChannel[sample] += excitation;
        rightChannel[sample] += excitation;
    }
}

void WindModelVoice::processBoreResonance(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Calculate bore delay length
    float effectiveFrequency = frequency_ * (1.0f + pitchBend_ * 0.05f);
    float boreDelayTime = boreLength_ * 1000.0f / effectiveFrequency; // Convert to ms
    int boreDelaySamples = static_cast<int>(sampleRate_ * boreDelayTime / 1000.0f);
    boreDelaySamples = juce::jlimit(10, 10000, boreDelaySamples);
    
    // Process through bore model
    for (int sample = 0; sample < numSamples; ++sample) {
        // Get input signal
        float inputSample = leftChannel[sample];
        
        // Read from delay line
        float delayedSample = boreModel_.delayLine[boreModel_.readIndex];
        
        // Apply bore resonance
        float resonanceResponse = calculateResonanceResponse();
        float outputSample = inputSample + delayedSample * resonanceResponse;
        
        // Apply damping
        outputSample *= (1.0f - boreModel_.damping);
        
        // Write to delay line
        boreModel_.delayLine[boreModel_.writeIndex] = outputSample;
        
        // Update indices
        boreModel_.writeIndex = (boreModel_.writeIndex + 1) % boreModel_.delayLine.size();
        boreModel_.readIndex = (boreModel_.readIndex + 1) % boreModel_.delayLine.size();
        
        // Update output
        leftChannel[sample] = outputSample;
        rightChannel[sample] = outputSample;
    }
}

void WindModelVoice::generateBreathNoise()
{
    float* leftNoise = noiseBuffer_.getWritePointer(0);
    float* rightNoise = noiseBuffer_.getWritePointer(1);
    
    // Generate white noise
    for (int sample = 0; sample < bufferSize_; ++sample) {
        float noiseSample = (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
        leftNoise[sample] = noiseSample * breathNoise_;
        rightNoise[sample] = noiseSample * breathNoise_;
    }
    
    // Apply low-pass filter for breath texture
    auto breathCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate_, 2000.0f);
    breathCoefficients->type = juce::dsp::IIR::Coefficients<float>::Type::lowPass;
    
    // Process noise through filter
    for (int channel = 0; channel < 2; ++channel) {
        float* channelData = noiseBuffer_.getWritePointer(channel);
        for (int sample = 0; sample < bufferSize_; ++sample) {
            channelData[sample] = breathCoefficients->processSample(channelData[sample]);
        }
    }
}

void WindModelVoice::applyVibrato(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Update vibrato phase
    vibratoPhase_ += (2.0 * juce::MathConstants<float>::pi * vibratoRate_) / sampleRate_;
    if (vibratoPhase_ > 2.0 * juce::MathConstants<float>::pi) {
        vibratoPhase_ -= 2.0 * juce::MathConstants<float>::pi;
    }
    
    // Apply vibrato to frequency
    float vibratoAmount = vibratoDepth_ * std::sin(vibratoPhase_);
    
    for (int sample = 0; sample < numSamples; ++sample) {
        float vibratoFactor = 1.0f + vibratoAmount;
        float vibratedSample = leftChannel[sample] * vibratoFactor;
        
        leftChannel[sample] = vibratedSample;
        rightChannel[sample] = vibratedSample;
    }
}

void WindModelVoice::applyFormantFiltering(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Update formant filter
    updateFormantFilter();
    
    // Apply formant filter
    formantFilter_->processSamples(leftChannel, numSamples);
    formantFilter_->processSamples(rightChannel, numSamples);
}

void WindModelVoice::applyNonlinearity(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Update nonlinearity
    updateNonlinearity();
    
    // Apply wave shaper
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = channel == 0 ? leftChannel : rightChannel;
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = nonlinearProcessor_->processSample(channelData[sample]);
        }
    }
}

void WindModelVoice::applyGrowl(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Apply growl filter
    growlFilter_->processSamples(leftChannel, numSamples);
    growlFilter_->processSamples(rightChannel, numSamples);
}

float WindModelVoice::calculateReedForce() const
{
    // Reed force based on opening and stiffness
    float openingFactor = 1.0f - reedOpening_;
    float stiffnessFactor = reedStiffness_;
    
    // Nonlinear reed behavior
    float velocityRatio = reedModel_.velocity / (reedModel_.velocity + 1.0f);
    float force = openingFactor * stiffnessFactor * velocityRatio;
    
    return force;
}

float WindModelVoice::calculateJetVelocity() const
{
    // Jet velocity based on breath pressure and reed opening
    float pressureDiff = currentBreathPressure_ - reedModel_.opening;
    float jetVelocity = std::sqrt(juce::jmax(0.0f, pressureDiff)) * 0.1f;
    
    return jetVelocity;
}

float WindModelVoice::calculateResonanceResponse() const
{
    // Bore resonance response
    float resonanceFactor = resonanceBoost_ * boreModel_.resonanceGain;
    float frequencyResponse = 1.0f / (1.0f + std::pow(frequency_ / resonanceFrequency_, 2));
    
    return resonanceFactor * frequencyResponse;
}

float WindModelVoice::getTonguingImpulse() const
{
    // Generate tonguing impulse
    float impulse = 0.0f;
    
    if (tonguingHardness_ > 0.5f) {
        // Hard tongue - sharp impulse
        impulse = tonguingHardness_ * 0.1f * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
    } else {
        // Soft tongue - smoother impulse
        impulse = tonguingHardness_ * 0.05f * std::sin(2.0 * juce::MathConstants<float>::pi * tonguingPosition_);
    }
    
    return impulse;
}

void WindModelVoice::updateReedModel()
{
    // Update reed velocity
    reedModel_.velocity = currentBreathPressure_ * 0.1f;
    
    // Update reed force
    reedModel_.force = calculateReedForce();
    
    // Apply pressure smoothing
    currentBreathPressure_ = smoothedBreath_.getCurrentValue();
}

void WindModelVoice::updateBoreModel()
{
    // Update bore resonance gain based on diameter
    boreModel_.resonanceGain = 0.5f + boreDiameter_ * 0.5f;
    
    // Update damping based on length
    boreModel_.damping = 0.01f + boreLength_ * 0.1f;
}

void WindModelVoice::updateFormantFilter()
{
    // Set formant frequency based on embouchure type
    float formantFreq = resonanceFrequency_;
    
    switch (embouchureType_) {
        case Embouchure::Clarinet:
            formantFreq = 1500.0f + toneColor_ * 500.0f;
            break;
        case Embouchure::Saxophone:
            formantFreq = 800.0f + toneColor_ * 400.0f;
            break;
        case Embouchure::Flute:
            formantFreq = 2000.0f + toneColor_ * 800.0f;
            break;
        case Embouchure::Brass:
            formantFreq = 500.0f + toneColor_ * 300.0f;
            break;
    }
    
    // Update formant filter
    auto formantCoefficients = juce::dsp::IIR::Coefficients<float>::makePeak(sampleRate_, formantFreq, 10.0f, resonanceBoost_);
    formantFilter_->setCoefficients(formantCoefficients);
    
    // Update brightness filter
    float brightnessFreq = 1000.0f + toneColor_ * 5000.0f;
    auto brightnessCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate_, brightnessFreq);
    brightnessFilter_->setCoefficients(brightnessCoefficients);
}

void WindModelVoice::updateNonlinearity()
{
    // Create nonlinearity curve based on amount
    auto function = [this](float x) {
        // Soft clipping with asymmetric behavior
        float threshold = 0.5f + nonlinearity_ * 0.5f;
        float amount = nonlinearity_;
        
        if (std::abs(x) < threshold) {
            return x * (1.0f - amount * 0.5f);
        } else {
            float sign = x > 0 ? 1.0f : -1.0f;
            float excess = std::abs(x) - threshold;
            return sign * (threshold + excess * (1.0f - amount * 0.8f));
        }
    };
    
    nonlinearProcessor_->functionToUse = function;
    nonlinearProcessor_-> samplesPerBlock = bufferSize_;
}

void WindModelVoice::updateAnalysis(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Calculate RMS level
    float sum = 0.0f;
    float peak = 0.0f;
    float channelSum = 0.0f;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample) {
            float sampleValue = std::abs(channelData[sample]);
            sum += sampleValue * sampleValue;
            peak = juce::jmax(peak, sampleValue);
        }
        channelSum += sum;
    }
    
    rmsLevel_ = std::sqrt(channelSum / (buffer.getNumChannels() * numSamples));
    peakLevel_ = peak;
}

void WindModelVoice::updateAge()
{
    voiceAge_ = (juce::Time::getMillisecondCounterHiRes() - voiceStartTime_) / 1000.0f;
}

// Parameter setters
void WindModelVoice::setBreathPressure(float pressure)
{
    breathPressure_ = juce::jlimit(0.0f, 1.0f, pressure);
    smoothedBreath_.setTargetValue(breathPressure_);
}

void WindModelVoice::setEmbouchureType(Embouchure type)
{
    embouchureType_ = type;
    updateFormantFilter();
}

void WindModelVoice::setVibratoDepth(float depth)
{
    vibratoDepth_ = juce::jlimit(0.0f, 0.2f, depth);
}

void WindModelVoice::setVibratoRate(float rate)
{
    vibratoRate_ = juce::jlimit(0.1f, 20.0f, rate);
}

void WindModelVoice::setToneColor(float color)
{
    toneColor_ = juce::jlimit(0.0f, 1.0f, color);
    updateFormantFilter();
}

void WindModelVoice::setGrowl(float growl)
{
    growl_ = juce::jlimit(0.0f, 1.0f, growl);
    
    // Update growl filter
    if (growl_ > 0.0f) {
        float growlFreq = 100.0f + growl_ * 200.0f;
        auto growlCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate_, growlFreq);
        growlFilter_->setCoefficients(growlCoefficients);
    }
}

void WindModelVoice::setReedStiffness(float stiffness)
{
    reedStiffness_ = juce::jlimit(0.1f, 1.0f, stiffness);
    updateReedModel();
}

void WindModelVoice::setReedOpening(float opening)
{
    reedOpening_ = juce::jlimit(0.1f, 1.0f, opening);
    smoothedReedOpening_.setTargetValue(reedOpening_);
    updateReedModel();
}

void WindModelVoice::setBoreLength(float length)
{
    boreLength_ = juce::jlimit(0.1f, 1.0f, length);
    updateBoreModel();
}

void WindModelVoice::setBoreDiameter(float diameter)
{
    boreDiameter_ = juce::jlimit(0.1f, 1.0f, diameter);
    updateBoreModel();
}

void WindModelVoice::setBreathNoise(float amount)
{
    breathNoise_ = juce::jlimit(0.0f, 1.0f, amount);
}

void WindModelVoice::setTonguingPosition(float position)
{
    tonguingPosition_ = juce::jlimit(0.0f, 1.0f, position);
}

void WindModelVoice::setTonguingHardness(float hardness)
{
    tonguingHardness_ = juce::jlimit(0.0f, 1.0f, hardness);
}

void WindModelVoice::setResonanceBoost(float boost)
{
    resonanceBoost_ = juce::jlimit(0.0f, 2.0f, boost);
    updateFormantFilter();
}

void WindModelVoice::setResonanceFrequency(float frequency)
{
    resonanceFrequency_ = juce::jlimit(100.0f, 5000.0f, frequency);
    updateFormantFilter();
}

void WindModelVoice::setNoiseFloor(float floor)
{
    noiseFloor_ = juce::jmax(0.0f, floor);
}

void WindModelVoice::setNonlinearity(float amount)
{
    nonlinearity_ = juce::jlimit(0.0f, 1.0f, amount);
    updateNonlinearity();
}

void WindModelVoice::setPitchBend(float bendAmount)
{
    pitchBend_ = juce::jlimit(-1.0f, 1.0f, bendAmount);
    smoothedFrequency_.setTargetValue(frequency_ * (1.0f + pitchBend_ * 0.05f));
}

void WindModelVoice::setPressure(float pressure)
{
    pressure_ = juce::jlimit(0.0f, 1.0f, pressure);
    smoothedBreath_.setTargetValue(breathPressure_ * pressure);
}

void WindModelVoice::setTimbre(float timbre)
{
    timbre_ = juce::jlimit(0.0f, 1.0f, timbre);
    toneColor_ = timbre;
    updateFormantFilter();
}

void WindModelVoice::setSmoothingTime(float timeMs)
{
    if (timeMs > 0.0f) {
        smoothedBreath_.reset(sampleRate_, timeMs / 1000.0f);
        smoothedFrequency_.reset(sampleRate_, timeMs / 1000.0f);
        smoothedReedOpening_.reset(sampleRate_, timeMs / 1000.0f);
    }
}

float WindModelVoice::getSmoothingTime() const
{
    return 50.0f; // Default smoothing time
}

// Getters
bool WindModelVoice::isActive() const
{
    return isActive_;
}

float WindModelVoice::getRmsLevel() const
{
    return rmsLevel_;
}

float WindModelVoice::getPeakLevel() const
{
    return peakLevel_;
}

float WindModelVoice::getFundamentalFrequency() const
{
    return fundamentalFrequency_;
}

float WindModelVoice::getBreathAmount() const
{
    return currentBreathPressure_;
}

float WindModelVoice::getVoiceAge() const
{
    return voiceAge_;
}

void WindModelVoice::updateAge()
{
    voiceAge_ = (juce::Time::getMillisecondCounterHiRes() - voiceStartTime_) / 1000.0f;
}

} // namespace Zenith
