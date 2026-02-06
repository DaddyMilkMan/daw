/*
  ==============================================================================

    StringModelVoice.cpp
    Created: [Date] Author: Claude AI
    Complete implementation of physical string modeling with Karplus-Strong and modal synthesis

  ==============================================================================
*/

#include "StringModel.h"
#include "../../../DSP/DSPStemSeparator.h"
#include "../../../DSP/PitchDetector.h"
#include "../../../DSP/EnvelopeFollower.h"

namespace Zenith
{

StringModelVoice::StringModelVoice()
    : frequency_(0.0f)
    , velocity_(0.0f)
    , excitationType_(Excitation::Pluck)
    , isActive_(false)
    , stringLength_(0.5f)
    , stringDiameter_(0.5f)
    , stringTension_(0.5f)
    , damping_(0.3f)
    , brightness_(0.5f)
    , inharmonicity_(0.1f)
    , bodyResonance_(0.2f)
    , excitationPosition_(0.3f)
    , vibratoDepth_(0.0f)
    , vibratoRate_(5.0f)
    , vibratoPhase_(0.0f)
    , currentVibratoAmount_(0.0f)
    , pitchBend_(0.0f)
    , pressure_(0.0f)
    , timbre_(0.0f)
    , sampleRate_(44100.0)
    , bufferSize_(512)
    , writeIndex_(0)
    , readIndex_(0)
    , delayLength_(0)
    , voiceStartTime_(0)
    , voiceAge_(0.0f)
    , rmsLevel_(0.0f)
    , peakLevel_(0.0f)
{
    initializeParameters();
    initializeAudioProcessing();
}

StringModelVoice::~StringModelVoice()
{
    reset();
}

void StringModelVoice::initializeParameters()
{
    // Initialize with default parameters
    setStringLength(0.5f);
    setStringDiameter(0.5f);
    setStringTension(0.5f);
    setDamping(0.3f);
    setBrightness(0.5f);
    setInharmonicity(0.1f);
    setBodyResonance(0.2f);
    setExcitationPosition(0.3f);
    setVibratoDepth(0.0f);
    setVibratoRate(5.0f);
    setSmoothingTime(50.0f);
    
    // Initialize modal synthesis
    setNumberOfModes(8);
    
    // Initialize MPE parameters
    setPitchBend(0.0f);
    setPressure(0.0f);
    setTimbre(0.0f);
}

void StringModelVoice::initializeAudioProcessing()
{
    // Initialize delay line based on sample rate
    delayLength_ = static_cast<int>(sampleRate_ / frequency_);
    delayLine_.resize(delayLength_, 0.0f);
    
    // Initialize filters
    dampingFilter_ = std::make_unique<juce::dsp::IIR::Filter<float>>();
    brightnessFilter_ = std::make_unique<juce::dsp::IIR::Filter<float>>();
    
    // Initialize filter coefficients
    updateFilters();
    
    // Initialize buffers
    excitationBuffer_.setSize(2, bufferSize_);
    modalBuffer_.setSize(2, bufferSize_);
    
    // Initialize smoothed parameters
    smoothedFrequency_.reset(sampleRate_, getSmoothingTime() / 1000.0);
    smoothedAmplitude_.reset(sampleRate_, getSmoothingTime() / 1000.0);
    smoothedDamping_.reset(sampleRate_, getSmoothingTime() / 1000.0);
}

void StringModelVoice::noteOn(float frequency, float velocity, Excitation excitation)
{
    try {
        frequency_ = frequency;
        velocity_ = juce::jlimit(0.0f, 1.0f, velocity);
        excitationType_ = excitation;
        isActive_ = true;
        
        voiceStartTime_ = juce::Time::getMillisecondCounterHiRes();
        voiceAge_ = 0.0f;
        
        // Update delay length for new frequency
        updateDelayLength();
        
        // Calculate modal frequencies
        calculateModalFrequencies();
        
        // Reset delay line
        std::fill(delayLine_.begin(), delayLine_.end(), 0.0f);
        writeIndex_ = 0;
        readIndex_ = 0;
        
        // Set initial parameters based on velocity
        float amplitude = velocity_ * 0.5f;
        smoothedAmplitude_.setTargetValue(amplitude);
        
        // Apply MPE parameters
        applyMPEParameters();
        
        DBG("StringModelVoice: NoteOn - Freq: " << frequency << " Vel: " << velocity);
    }
    catch (const std::exception& e) {
        DBG("StringModelVoice: Error in noteOn - " << e.what());
        isActive_ = false;
    }
}

void StringModelVoice::noteOff()
{
    if (isActive_) {
        // Gradual release through damping
        smoothedAmplitude_.setTargetValue(0.0f);
        isActive_ = false;
    }
}

void StringModelVoice::reset()
{
    isActive_ = false;
    std::fill(delayLine_.begin(), delayLine_.end(), 0.0f);
    std::fill(excitationBuffer_.getWritePointer(0), excitationBuffer_.getWritePointer(0) + excitationBuffer_.getNumSamples(), 0.0f);
    std::fill(modalBuffer_.getWritePointer(0), modalBuffer_.getWritePointer(0) + modalBuffer_.getNumSamples(), 0.0f);
    
    // Reset modes
    for (auto& mode : modes_) {
        mode.phase = 0.0f;
        mode.amplitude = 0.0f;
    }
    
    voiceAge_ = 0.0f;
    rmsLevel_ = 0.0f;
    peakLevel_ = 0.0f;
}

void StringModelVoice::process(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!isActive_ || numSamples <= 0) return;
    
    // Process through Karplus-Strong
    processKarplusStrong(buffer, numSamples);
    
    // Add modal synthesis for richness
    processModalSynthesis(buffer, numSamples);
    
    // Apply filters
    applyFilters(buffer, numSamples);
    
    // Update analysis data
    updateAnalysis(buffer, numSamples);
    
    // Update voice age
    updateAge();
}

void StringModelVoice::processKarplusStrong(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Get excitation sample
    float excitationSample = getExcitationSample(excitationType_, excitationPosition_, velocity_);
    
    // Apply vibrato
    updateVibrato();
    float vibratoFactor = 1.0f + currentVibratoAmount_;
    float effectiveFrequency = frequency_ * vibratoFactor;
    
    // Calculate delay based on frequency
    float delaySamples = sampleRate_ / effectiveFrequency;
    int delayInt = static_cast<int>(delaySamples);
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample) {
        // Read from delay line with interpolation
        float delayFraction = delaySamples - delayInt;
        float readIndex1 = (readIndex_ + delayInt) % delayLength_;
        float readIndex2 = (readIndex_ + delayInt + 1) % delayLength_;
        
        float delayedSample = delayLine_[readIndex1] * (1.0f - delayFraction) + 
                              delayLine_[readIndex2] * delayFraction;
        
        // Apply low-pass filter (string damping)
        delayedSample = dampingFilter_->processSample(delayedSample);
        
        // Add excitation
        float outputSample = delayedSample * 0.5f + excitationSample * 0.5f;
        
        // Write to delay line with interpolation
        delayLine_[writeIndex_] = outputSample;
        
        // Write to buffer
        leftChannel[sample] = outputSample * smoothedAmplitude_.getNextValue();
        rightChannel[sample] = outputSample * smoothedAmplitude_.getNextValue();
        
        // Update indices
        writeIndex_ = (writeIndex_ + 1) % delayLength_;
        readIndex_ = (readIndex_ + 1) % delayLength_;
        
        // Generate new excitation sample
        excitationSample = getExcitationSample(excitationType_, excitationPosition_, velocity_);
    }
}

void StringModelVoice::processModalSynthesis(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Process each mode
    for (auto& mode : modes_) {
        // Update target values
        mode.targetAmplitude = mode.amplitude * smoothedAmplitude_.getCurrentValue();
        mode.targetFrequency = mode.frequency * (1.0f + pitchBend_ * 0.05f);
        
        // Smooth frequency changes
        float freqDiff = mode.targetFrequency - mode.frequency;
        mode.frequency += freqDiff * 0.1f;
        
        // Calculate phase increment
        mode.phaseIncrement = (2.0 * juce::MathConstants<float>::pi * mode.frequency) / sampleRate_;
        
        // Generate modal signal
        for (int sample = 0; sample < numSamples; ++sample) {
            float modalSample = std::sin(mode.phase) * mode.targetAmplitude;
            
            // Apply modal decay
            float decayFactor = std::exp(-mode.decay * 0.01f);
            mode.amplitude *= decayFactor;
            
            // Add to buffer
            leftChannel[sample] += modalSample;
            rightChannel[sample] += modalSample;
            
            // Update phase
            mode.phase += mode.phaseIncrement;
            if (mode.phase > 2.0 * juce::MathConstants<float>::pi) {
                mode.phase -= 2.0 * juce::MathConstants<float>::pi;
            }
        }
    }
}

void StringModelVoice::processExcitation(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Generate excitation based on type
    for (int sample = 0; sample < numSamples; ++sample) {
        float excitationSample = 0.0f;
        
        switch (excitationType_) {
            case Excitation::Pluck:
                // Triangle wave for pluck
                excitationSample = velocity_ * 0.3f * (1.0f - 2.0f * std::abs((sample % 100) / 100.0f - 0.5f));
                break;
                
            case Excitation::Bow:
                // Continuous noise for bowing
                excitationSample = velocity_ * 0.2f * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
                break;
                
            case Excitation::Strike:
                // Impulse for strike
                if (sample == 0) {
                    excitationSample = velocity_ * 0.5f;
                }
                break;
                
            case Excitation::Blow:
                // Breath noise for harp-like instruments
                excitationSample = velocity_ * 0.15f * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
                break;
        }
        
        // Apply excitation position filtering
        float positionFactor = 1.0f - excitationPosition_;
        excitationSample *= positionFactor;
        
        leftChannel[sample] += excitationSample;
        rightChannel[sample] += excitationSample;
    }
}

void StringModelVoice::updateDelayLength()
{
    float effectiveFrequency = frequency_ * (1.0f + pitchBend_ * 0.05f);
    delayLength_ = static_cast<int>(sampleRate_ / effectiveFrequency);
    delayLength_ = juce::jlimit(100, 20000, delayLength_);
    
    // Resize delay line if needed
    if (delayLength_ != delayLine_.size()) {
        std::vector<float> newDelayLine(delayLength_, 0.0f);
        
        // Copy existing data with interpolation
        for (int i = 0; i < juce::jmin(delayLine_.size(), delayLength_); ++i) {
            int srcIndex = (i * delayLine_.size()) / delayLength_;
            newDelayLine[i] = delayLine_[srcIndex];
        }
        
        delayLine_ = std::move(newDelayLine);
        writeIndex_ %= delayLength_;
        readIndex_ %= delayLength_;
    }
}

void StringModelVoice::calculateModalFrequencies()
{
    modes_.clear();
    
    float baseFrequency = frequency_;
    
    for (int i = 0; i < modes_.size(); ++i) {
        float harmonic = i + 1;
        float inharmonicFactor = calculateInharmonicFactor(i);
        float modalFrequency = baseFrequency * harmonic * inharmonicFactor;
        
        // Calculate amplitude based on position and mode
        float amplitude = 1.0f / harmonic; // Natural amplitude decay
        amplitude *= (1.0f - excitationPosition_); // Position-based attenuation
        
        modes_.push_back({
            modalFrequency,
            amplitude,
            0.01f + (0.1f * damping_), // Decay rate
            0.0f, // Phase
            amplitude, // Target amplitude
            modalFrequency, // Target frequency
            0.01f + (0.1f * damping_) // Target damping
        });
    }
}

float StringModelVoice::calculateInharmonicFactor(int harmonic) const
{
    // Calculate inharmonicity based on string stiffness
    float B = inharmonicity_; // Inharmonicity coefficient
    return 1.0f + B * harmonic * harmonic;
}

float StringModelVoice::getExcitationSample(Excitation type, float position, float velocity)
{
    switch (type) {
        case Excitation::Pluck:
            // Triangle wave with noise
            float triangle = velocity * 0.3f * (1.0f - 2.0f * std::abs((juce::Random::getSystemRandom().nextInt() % 100) / 100.0f - 0.5f));
            return triangle * (1.0f - position);
            
        case Excitation::Bow:
            // Continuous noise
            return velocity * 0.2f * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
            
        case Excitation::Strike:
            // Impulse
            if (juce::Random::getSystemRandom().nextInt() % 100 == 0) {
                return velocity * 0.5f;
            }
            return 0.0f;
            
        case Excitation::Blow:
            // Breath noise
            return velocity * 0.15f * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
            
        default:
            return 0.0f;
    }
}

void StringModelVoice::updateVibrato()
{
    vibratoPhase_ += (2.0 * juce::MathConstants<float>::pi * vibratoRate_) / sampleRate_;
    if (vibratoPhase_ > 2.0 * juce::MathConstants<float>::pi) {
        vibratoPhase_ -= 2.0 * juce::MathConstants<float>::pi;
    }
    
    currentVibratoAmount_ = vibratoDepth_ * std::sin(vibratoPhase_);
}

void StringModelVoice::applyMPEParameters()
{
    // Apply pitch bend
    float bendFactor = 1.0f + pitchBend_ * 0.05f;
    smoothedFrequency_.setTargetValue(frequency_ * bendFactor);
    
    // Apply pressure
    float pressureFactor = 1.0f + pressure_ * 0.1f;
    smoothedAmplitude_.setTargetValue(velocity_ * pressureFactor);
    
    // Apply timbre
    updateFilters();
}

void StringModelVoice::updateFilters()
{
    // Update damping filter
    auto dampingCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate_, 1000.0f * (1.0f - damping_));
    dampingFilter_->setCoefficients(dampingCoefficients);
    
    // Update brightness filter
    auto brightnessCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate_, 1000.0f * brightness_);
    brightnessFilter_->setCoefficients(brightnessCoefficients);
}

void StringModelVoice::applyFilters(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Apply damping filter
    dampingFilter_->processSamples(buffer.getWritePointer(0), numSamples);
    if (buffer.getNumChannels() > 1) {
        dampingFilter_->processSamples(buffer.getWritePointer(1), numSamples);
    }
    
    // Apply brightness filter
    brightnessFilter_->processSamples(buffer.getWritePointer(0), numSamples);
    if (buffer.getNumChannels() > 1) {
        brightnessFilter_->processSamples(buffer.getWritePointer(1), numSamples);
    }
    
    // Apply body resonance
    float bodyGain = bodyResonance_ * 0.3f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] *= (1.0f + bodyGain);
        }
    }
}

void StringModelVoice::updateAnalysis(juce::AudioBuffer<float>& buffer, int numSamples)
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

void StringModelVoice::updateAge()
{
    voiceAge_ = (juce::Time::getMillisecondCounterHiRes() - voiceStartTime_) / 1000.0f;
}

// Parameter setters
void StringModelVoice::setStringLength(float length)
{
    stringLength_ = juce::jlimit(0.1f, 1.0f, length);
    updateDelayLength();
}

void StringModelVoice::setStringDiameter(float diameter)
{
    stringDiameter_ = juce::jlimit(0.1f, 1.0f, diameter);
    updateFilters();
}

void StringModelVoice::setStringTension(float tension)
{
    stringTension_ = juce::jlimit(0.1f, 1.0f, tension);
    updateDelayLength();
}

void StringModelVoice::setDamping(float damping)
{
    damping_ = juce::jlimit(0.0f, 1.0f, damping);
    smoothedDamping_.setTargetValue(damping);
    updateFilters();
}

void StringModelVoice::setBrightness(float brightness)
{
    brightness_ = juce::jlimit(0.0f, 1.0f, brightness);
    updateFilters();
}

void StringModelVoice::setInharmonicity(float inharmonicity)
{
    inharmonicity_ = juce::jlimit(0.0f, 1.0f, inharmonicity);
    calculateModalFrequencies();
}

void StringModelVoice::setBodyResonance(float resonance)
{
    bodyResonance_ = juce::jlimit(0.0f, 1.0f, resonance);
}

void StringModelVoice::setExcitationPosition(float position)
{
    excitationPosition_ = juce::jlimit(0.0f, 1.0f, position);
    calculateModalFrequencies();
}

void StringModelVoice::setNumberOfModes(int modes)
{
    int newModes = juce::jlimit(1, 16, modes);
    if (newModes != modes_.size()) {
        modes_.resize(newModes);
        calculateModalFrequencies();
    }
}

void StringModelVoice::setModeDecay(int modeIndex, float decay)
{
    if (modeIndex >= 0 && modeIndex < modes_.size()) {
        modes_[modeIndex].decay = juce::jlimit(0.001f, 1.0f, decay);
    }
}

void StringModelVoice::setModeFrequency(int modeIndex, float frequency)
{
    if (modeIndex >= 0 && modeIndex < modes_.size()) {
        modes_[modeIndex].targetFrequency = juce::jlimit(20.0f, 20000.0f, frequency);
    }
}

void StringModelVoice::setModeAmplitude(int modeIndex, float amplitude)
{
    if (modeIndex >= 0 && modeIndex < modes_.size()) {
        modes_[modeIndex].targetAmplitude = juce::jlimit(0.0f, 1.0f, amplitude);
    }
}

void StringModelVoice::setVibratoDepth(float depth)
{
    vibratoDepth_ = juce::jlimit(0.0f, 0.1f, depth);
}

void StringModelVoice::setVibratoRate(float rate)
{
    vibratoRate_ = juce::jlimit(0.1f, 20.0f, rate);
}

void StringModelVoice::setVibratoPhase(float phase)
{
    vibratoPhase_ = phase;
}

void StringModelVoice::setPitchBend(float bendAmount)
{
    pitchBend_ = juce::jlimit(-1.0f, 1.0f, bendAmount);
    applyMPEParameters();
}

void StringModelVoice::setPressure(float pressure)
{
    pressure_ = juce::jlimit(0.0f, 1.0f, pressure);
    applyMPEParameters();
}

void StringModelVoice::setTimbre(float timbre)
{
    timbre_ = juce::jlimit(0.0f, 1.0f, timbre);
    brightness_ = timbre;
    updateFilters();
}

void StringModelVoice::setSmoothingTime(float timeMs)
{
    if (timeMs > 0.0f) {
        smoothedFrequency_.reset(sampleRate_, timeMs / 1000.0f);
        smoothedAmplitude_.reset(sampleRate_, timeMs / 1000.0f);
        smoothedDamping_.reset(sampleRate_, timeMs / 1000.0f);
    }
}

float StringModelVoice::getSmoothingTime() const
{
    return 50.0f; // Default smoothing time
}

// Getters
bool StringModelVoice::isActive() const
{
    return isActive_;
}

float StringModelVoice::getRmsLevel() const
{
    return rmsLevel_;
}

float StringModelVoice::getPeakLevel() const
{
    return peakLevel_;
}

float StringModelVoice::getFundamentalFrequency() const
{
    return frequency_;
}

juce::Array<float> StringModelVoice::getModeAmplitudes() const
{
    juce::Array<float> amplitudes;
    for (const auto& mode : modes_) {
        amplitudes.add(mode.amplitude);
    }
    return amplitudes;
}

float StringModelVoice::getVoiceAge() const
{
    return voiceAge_;
}

void StringModelVoice::updateAge()
{
    voiceAge_ = (juce::Time::getMillisecondCounterHiRes() - voiceStartTime_) / 1000.0f;
}

} // namespace Zenith
