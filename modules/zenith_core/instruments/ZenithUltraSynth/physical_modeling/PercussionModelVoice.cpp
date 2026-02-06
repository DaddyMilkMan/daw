/*
  ==============================================================================

    PercussionModelVoice.cpp
    Created: [Date] Author: Claude AI
    Complete implementation of modal synthesis for drums and percussion

  ==============================================================================
*/

#include "PercussionModel.h"
#include "../../../DSP/EnvelopeFollower.h"
#include "../../../DSP/MasterLimiter.h"

namespace Zenith
{

PercussionModelVoice::PercussionModelVoice()
    : velocity_(0.0f)
    , drumType_(DrumType::Kick)
    , isActive_(false)
    , decay_(0.5f)
    , tone_(0.5f)
    , snares_(0.0f)
    , cymbalComplexity_(0.5f)
    , headType_(0.5f)
    , shellSize_(0.5f)
    , drumPosition_(0.5f)
    , stickHardness_(0.5f)
    , stickPosition_(0.5f)
    , resonance_(0.3f)
    , noiseAmount_(0.3f)
    , noiseColor_(0.5f)
    , metallicResonance_(0.0f)
    , airResonance_(0.0f)
    , attack_(0.01f)
    , decayShape_(0.5f)
    , sustain_(0.0f)
    , release_(0.1f)
    , pitchBend_(0.0f)
    , pressure_(0.0f)
    , timbre_(0.0f)
    , sampleRate_(44100.0)
    , bufferSize_(512)
    , voiceStartTime_(0)
    , voiceAge_(0.0f)
    , rmsLevel_(0.0f)
    , peakLevel_(0.0f)
    , fundamentalFrequency_(0.0f)
{
    initializeParameters();
    initializeAudioProcessing();
}

PercussionModelVoice::~PercussionModelVoice()
{
    reset();
}

void PercussionModelVoice::initializeParameters()
{
    // Initialize with default parameters
    setDecay(0.5f);
    setTone(0.5f);
    setSnares(0.0f);
    setCymbalComplexity(0.5f);
    setHeadType(0.5f);
    setShellSize(0.5f);
    setDrumPosition(0.5f);
    setStickHardness(0.5f);
    setStickPosition(0.5f);
    setResonance(0.3f);
    setNoiseAmount(0.3f);
    setNoiseColor(0.5f);
    setMetallicResonance(0.0f);
    setAirResonance(0.0f);
    setAttack(0.01f);
    setDecayShape(0.5f);
    setSustain(0.0f);
    setRelease(0.1f);
    setSmoothingTime(50.0f);
    
    // Initialize MPE parameters
    setPitchBend(0.0f);
    setPressure(0.0f);
    setTimbre(0.0f);
    
    // Initialize envelope
    envelope_.value = 0.0f;
    envelope_.attackTime = attack_;
    envelope_.decayTime = decay_;
    envelope_.sustainLevel = sustain_;
    envelope_.releaseTime = release_;
    envelope_.state = 0; // attack
    envelope_.phase = 0.0f;
}

void PercussionModelVoice::initializeAudioProcessing()
{
    // Initialize modes based on drum type
    initializeModes();
    
    // Initialize filters
    toneFilter_ = std::make_unique<juce::dsp::IIR::Filter<float>>();
    noiseFilter_ = std::make_unique<juce::dsp::IIR::Filter<float>>();
    metallicFilter_ = std::make_unique<juce::dsp::IIR::Filter<float>>();
    
    // Initialize buffers
    excitationBuffer_.setSize(2, bufferSize_);
    noiseBuffer_.setSize(2, bufferSize_);
    
    // Initialize smoothed parameters
    smoothedAmplitude_.reset(sampleRate_, getSmoothingTime() / 1000.0);
    smoothedFrequency_.reset(sampleRate_, getSmoothingTime() / 1000.0);
    smoothedDecay_.reset(sampleRate_, getSmoothingTime() / 1000.0);
    
    // Update filters
    updateFilters();
}

void PercussionModelVoice::noteOn(float velocity, DrumType type)
{
    try {
        velocity_ = juce::jlimit(0.0f, 1.0f, velocity);
        drumType_ = type;
        isActive_ = true;
        
        voiceStartTime_ = juce::Time::getMillisecondCounterHiRes();
        voiceAge_ = 0.0f;
        
        // Set fundamental frequency based on drum type
        fundamentalFrequency_ = getDrumFundamentalFrequency();
        smoothedFrequency_.setTargetValue(fundamentalFrequency_);
        
        // Initialize envelope
        envelope_.phase = 0.0f;
        envelope_.state = 0; // attack
        envelope_.attackTime = attack_;
        envelope_.decayTime = decay_;
        envelope_.sustainLevel = sustain_;
        envelope_.releaseTime = release_;
        
        // Reset modes
        for (auto& mode : modes_) {
            mode.phase = 0.0f;
            mode.amplitude = velocity_ * 0.5f;
        }
        
        // Initialize metal modes for cymbals
        if (drumType_ == DrumType::Cymbal) {
            initializeMetalModes();
        }
        
        // Reset noise generator
        noiseGenerator_.setSeedRandomly();
        
        DBG("PercussionModelVoice: NoteOn - Type: " << static_cast<int>(type) << " Vel: " << velocity);
    }
    catch (const std::exception& e) {
        DBG("PercussionModelVoice: Error in noteOn - " << e.what());
        isActive_ = false;
    }
}

void PercussionModelVoice::noteOff()
{
    if (isActive_) {
        // Start release phase
        envelope_.state = 3; // release
        isActive_ = false;
    }
}

void PercussionModelVoice::reset()
{
    isActive_ = false;
    std::fill(excitationBuffer_.getWritePointer(0), excitationBuffer_.getWritePointer(0) + excitationBuffer_.getNumSamples(), 0.0f);
    std::fill(noiseBuffer_.getWritePointer(0), noiseBuffer_.getWritePointer(0) + noiseBuffer_.getNumSamples(), 0.0f);
    
    // Reset modes
    for (auto& mode : modes_) {
        mode.phase = 0.0f;
        mode.amplitude = 0.0f;
    }
    
    // Reset metal modes
    for (auto& metalMode : metalModes_) {
        metalMode.phase = 0.0f;
        metalMode.amplitude = 0.0f;
    }
    
    voiceAge_ = 0.0f;
    rmsLevel_ = 0.0f;
    peakLevel_ = 0.0f;
    fundamentalFrequency_ = 0.0f;
}

void PercussionModelVoice::process(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!isActive_ && envelope_.value <= 0.0f) return;
    
    // Generate excitation signal
    processExcitation(buffer, numSamples);
    
    // Generate noise
    processNoise(buffer, numSamples);
    
    // Process modal synthesis
    processModalSynthesis(buffer, numSamples);
    
    // Process metal modes for cymbals
    if (drumType_ == DrumType::Cymbal) {
        processMetalModes(buffer, numSamples);
    }
    
    // Process snares
    if (snares_ > 0.0f && drumType_ == DrumType::Snare) {
        processSnares(buffer, numSamples);
    }
    
    // Apply filters
    applyFilters(buffer, numSamples);
    
    // Update envelope
    updateEnvelope();
    
    // Apply envelope to buffer
    float envelopeValue = getDrumEnvelopeValue();
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] *= envelopeValue;
        }
    }
    
    // Update analysis data
    updateAnalysis(buffer, numSamples);
    
    // Update voice age
    updateAge();
}

void PercussionModelVoice::initializeModes()
{
    modes_.clear();
    
    // Base frequencies for different drum types
    float baseFreq = getDrumFundamentalFrequency();
    
    // Number of modes based on drum type
    int numModes = getDrumModeCount();
    
    for (int i = 0; i < numModes; ++i) {
        float modalFreq = baseFreq * (i + 1) * getDrumInharmonicity(i);
        float modalAmp = 1.0f / (i + 1); // Natural amplitude decay
        float modalDamping = getDrumDamping(i);
        
        modes_.push_back({
            modalFreq,
            modalAmp,
            modalDamping,
            0.0f, // Phase
            modalFreq * 0.01f, // Phase increment
            modalAmp, // Target amplitude
            modalFreq, // Target frequency
            modalDamping // Target damping
        });
    }
}

void PercussionModelVoice::initializeMetalModes()
{
    metalModes_.clear();
    
    // Initialize cymbal modes
    int numMetalModes = static_cast<int>(cymbalComplexity_ * 16) + 4;
    
    for (int i = 0; i < numMetalModes; ++i) {
        float metalFreq = 1000.0f * (i + 1) * (1.0f + cymbalComplexity_ * 0.5f);
        float metalAmp = 1.0f / (i + 1) * (1.0f - i * 0.1f);
        float metalDecay = 0.01f + (0.1f * decay_);
        
        metalModes_.push_back({
            metalFreq,
            metalAmp,
            metalDecay,
            0.0f // Phase
        });
    }
}

void PercussionModelVoice::processModalSynthesis(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Process each mode
    for (auto& mode : modes_) {
        // Update target values
        mode.targetAmplitude = mode.amplitude * smoothedAmplitude_.getCurrentValue();
        mode.targetFrequency = mode.frequency * (1.0f + pitchBend_ * 0.02f);
        
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

void PercussionModelVoice::processMetalModes(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Process each metal mode
    for (auto& metalMode : metalModes_) {
        // Calculate phase increment
        float phaseIncrement = (2.0 * juce::MathConstants<float>::pi * metalMode.frequency) / sampleRate_;
        
        // Generate metal signal
        for (int sample = 0; sample < numSamples; ++sample) {
            float metalSample = std::sin(metalMode.phase) * metalMode.amplitude;
            
            // Apply metal decay
            float decayFactor = std::exp(-metalMode.decay * 0.005f);
            metalMode.amplitude *= decayFactor;
            
            // Add to buffer
            leftChannel[sample] += metalSample;
            rightChannel[sample] += metalSample;
            
            // Update phase
            metalMode.phase += phaseIncrement;
            if (metalMode.phase > 2.0 * juce::MathConstants<float>::pi) {
                metalMode.phase -= 2.0 * juce::MathConstants<float>::pi;
            }
        }
    }
}

void PercussionModelVoice::processExcitation(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Generate excitation based on drum type
    for (int sample = 0; sample < numSamples; ++sample) {
        float excitationSample = 0.0f;
        
        switch (drumType_) {
            case DrumType::Kick:
                // Low frequency thump
                excitationSample = velocity_ * 0.5f * std::sin(2.0 * juce::MathConstants<float>::pi * 60.0f * sample / sampleRate_);
                break;
                
            case DrumType::Snare:
                // High frequency click
                excitationSample = velocity_ * 0.3f * std::sin(2.0 * juce::MathConstants<float>::pi * 200.0f * sample / sampleRate_);
                break;
                
            case DrumType::Tom:
                // Mid frequency thump
                excitationSample = velocity_ * 0.4f * std::sin(2.0 * juce::MathConstants<float>::pi * 150.0f * sample / sampleRate_);
                break;
                
            case DrumType::Hihat:
                // Closed hi-hat sound
                excitationSample = velocity_ * 0.2f * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
                break;
                
            case DrumType::Cymbal:
                // Cymbal strike
                excitationSample = velocity_ * 0.15f * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
                break;
                
            default:
                // Generic percussion
                excitationSample = velocity_ * 0.2f * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
                break;
        }
        
        // Apply stick hardness
        excitationSample *= (0.5f + stickHardness_ * 0.5f);
        
        // Apply position
        excitationSample *= (1.0f - stickPosition_);
        
        leftChannel[sample] += excitationSample;
        rightChannel[sample] += excitationSample;
    }
}

void PercussionModelVoice::processNoise(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Generate noise
    for (int sample = 0; sample < numSamples; ++sample) {
        float noiseSample = (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
        noiseSample *= noiseAmount_;
        
        leftChannel[sample] += noiseSample;
        rightChannel[sample] += noiseSample;
    }
}

void PercussionModelVoice::processSnares(juce::AudioBuffer<float>& buffer, int numSamples)
{
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Generate snare rattle
    for (int sample = 0; sample < numSamples; ++sample) {
        float snareSample = (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
        snareSample *= snares_ * 0.3f;
        
        // Apply modulation
        float modulation = std::sin(2.0 * juce::MathConstants<float>::pi * 200.0f * sample / sampleRate_);
        snareSample *= modulation;
        
        leftChannel[sample] += snareSample;
        rightChannel[sample] += snareSample;
    }
}

void PercussionModelVoice::updateEnvelope()
{
    envelope_.phase += 1.0f / sampleRate_;
    
    switch (envelope_.state) {
        case 0: // attack
            envelope_.value = envelope_.phase / envelope_.attackTime;
            if (envelope_.phase >= envelope_.attackTime) {
                envelope_.phase = 0.0f;
                envelope_.state = 1; // decay
            }
            break;
            
        case 1: // decay
            float decayProgress = envelope_.phase / envelope_.decayTime;
            envelope_.value = 1.0f - decayProgress * (1.0f - envelope_.sustainLevel);
            if (envelope_.phase >= envelope_.decayTime) {
                envelope_.phase = 0.0f;
                envelope_.state = 2; // sustain
                envelope_.value = envelope_.sustainLevel;
            }
            break;
            
        case 2: // sustain
            envelope_.value = envelope_.sustainLevel;
            break;
            
        case 3: // release
            float releaseProgress = envelope_.phase / envelope_.releaseTime;
            envelope_.value = envelope_.sustainLevel * (1.0f - releaseProgress);
            if (envelope_.phase >= envelope_.releaseTime) {
                envelope_.value = 0.0f;
            }
            break;
    }
    
    envelope_.value = juce::jlimit(0.0f, 1.0f, envelope_.value);
}

void PercussionModelVoice::applyFilters(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Apply tone filter
    toneFilter_->processSamples(buffer.getWritePointer(0), numSamples);
    if (buffer.getNumChannels() > 1) {
        toneFilter_->processSamples(buffer.getWritePointer(1), numSamples);
    }
    
    // Apply noise filter
    noiseFilter_->processSamples(buffer.getWritePointer(0), numSamples);
    if (buffer.getNumChannels() > 1) {
        noiseFilter_->processSamples(buffer.getWritePointer(1), numSamples);
    }
    
    // Apply metallic filter
    metallicFilter_->processSamples(buffer.getWritePointer(0), numSamples);
    if (buffer.getNumChannels() > 1) {
        metallicFilter_->processSamples(buffer.getWritePointer(1), numSamples);
    }
    
    // Apply stereo imaging based on drum position
    float panPosition = (drumPosition_ - 0.5f) * 2.0f; // -1 to 1
    for (int sample = 0; sample < numSamples; ++sample) {
        float leftSample = buffer.getWritePointer(0)[sample];
        float rightSample = buffer.getWritePointer(1)[sample];
        
        float balance = (panPosition > 0) ? 1.0f + panPosition : 1.0f;
        float pan = (panPosition > 0) ? 1.0f : 1.0f + std::abs(panPosition);
        
        buffer.getWritePointer(0)[sample] = leftSample * balance / pan;
        buffer.getWritePointer(1)[sample] = rightSample * (2.0f - balance) / pan;
    }
}

void PercussionModelVoice::updateFilters()
{
    // Update tone filter
    float toneFreq = 100.0f + tone_ * 5000.0f;
    auto toneCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate_, toneFreq);
    toneFilter_->setCoefficients(toneCoefficients);
    
    // Update noise filter
    float noiseFreq = 1000.0f + noiseColor_ * 4000.0f;
    auto noiseCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate_, noiseFreq);
    noiseFilter_->setCoefficients(noiseCoefficients);
    
    // Update metallic filter
    float metallicFreq = 2000.0f + metallicResonance_ * 3000.0f;
    auto metallicCoefficients = juce::dsp::IIR::Coefficients<float>::makePeak(sampleRate_, metallicFreq, 10.0f, metallicResonance_);
    metallicFilter_->setCoefficients(metallicCoefficients);
}

void PercussionModelVoice::updateAnalysis(juce::AudioBuffer<float>& buffer, int numSamples)
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

void PercussionModelVoice::updateAge()
{
    voiceAge_ = (juce::Time::getMillisecondCounterHiRes() - voiceStartTime_) / 1000.0f;
}

float PercussionModelVoice::getDrumFundamentalFrequency() const
{
    switch (drumType_) {
        case DrumType::Kick:
            return 60.0f + shellSize_ * 40.0f; // 60-100 Hz
        case DrumType::Snare:
            return 200.0f + shellSize_ * 100.0f; // 200-300 Hz
        case DrumType::Tom:
            return 100.0f + shellSize_ * 200.0f; // 100-300 Hz
        case DrumType::Hihat:
            return 800.0f + shellSize_ * 400.0f; // 800-1200 Hz
        case DrumType::Cymbal:
            return 500.0f + shellSize_ * 1000.0f; // 500-1500 Hz
        default:
            return 150.0f + shellSize_ * 150.0f; // Default 150-300 Hz
    }
}

int PercussionModelVoice::getDrumModeCount() const
{
    switch (drumType_) {
        case DrumType::Kick:
            return 4; // Few modes for kick
        case DrumType::Snare:
            return 8; // More modes for snare
        case DrumType::Tom:
            return 6; // Medium modes for tom
        case DrumType::Hihat:
            return 10; // Many modes for hi-hat
        case DrumType::Cymbal:
            return 16; // Many modes for cymbal
        default:
            return 6; // Default mode count
    }
}

float PercussionModelVoice::getDrumInharmonicity(int modeIndex) const
{
    float inharmonicity = 1.0f + headType_ * 0.1f * modeIndex * modeIndex;
    return inharmonicity;
}

float PercussionModelVoice::getDrumDamping(int modeIndex) const
{
    float damping = 0.01f + decay_ * 0.1f * (1.0f + modeIndex * 0.1f);
    return damping;
}

float PercussionModelVoice::getDrumEnvelopeValue() const
{
    return envelope_.value;
}

// Parameter setters
void PercussionModelVoice::setDecay(float decay)
{
    decay_ = juce::jlimit(0.0f, 1.0f, decay);
    smoothedDecay_.setTargetValue(decay_);
}

void PercussionModelVoice::setTone(float tone)
{
    tone_ = juce::jlimit(0.0f, 1.0f, tone);
    updateFilters();
}

void PercussionModelVoice::setSnares(float snares)
{
    snares_ = juce::jlimit(0.0f, 1.0f, snares);
}

void PercussionModelVoice::setCymbalComplexity(float complexity)
{
    cymbalComplexity_ = juce::jlimit(0.0f, 1.0f, complexity);
    if (drumType_ == DrumType::Cymbal) {
        initializeMetalModes();
    }
}

void PercussionModelVoice::setHeadType(float type)
{
    headType_ = juce::jlimit(0.0f, 1.0f, type);
    initializeModes();
}

void PercussionModelVoice::setShellSize(float size)
{
    shellSize_ = juce::jlimit(0.1f, 1.0f, size);
    initializeModes();
}

void PercussionModelVoice::setDrumPosition(float position)
{
    drumPosition_ = juce::jlimit(0.0f, 1.0f, position);
}

void PercussionModelVoice::setStickHardness(float hardness)
{
    stickHardness_ = juce::jlimit(0.0f, 1.0f, hardness);
}

void PercussionModelVoice::setStickPosition(float position)
{
    stickPosition_ = juce::jlimit(0.0f, 1.0f, position);
}

void PercussionModelVoice::setResonance(float resonance)
{
    resonance_ = juce::jlimit(0.0f, 1.0f, resonance);
    updateFilters();
}

void PercussionModelVoice::setNumberOfModes(int modes)
{
    int newModes = juce::jlimit(1, 32, modes);
    if (newModes != modes_.size()) {
        modes_.resize(newModes);
        updateModeParameters();
    }
}

void PercussionModelVoice::setModeFrequency(int modeIndex, float frequency)
{
    if (modeIndex >= 0 && modeIndex < modes_.size()) {
        modes_[modeIndex].targetFrequency = juce::jlimit(20.0f, 20000.0f, frequency);
    }
}

void PercussionModelVoice::setModeDamping(int modeIndex, float damping)
{
    if (modeIndex >= 0 && modeIndex < modes_.size()) {
        modes_[modeIndex].targetDamping = juce::jlimit(0.001f, 1.0f, damping);
    }
}

void PercussionModelVoice::setModeAmplitude(int modeIndex, float amplitude)
{
    if (modeIndex >= 0 && modeIndex < modes_.size()) {
        modes_[modeIndex].targetAmplitude = juce::jlimit(0.0f, 1.0f, amplitude);
    }
}

void PercussionModelVoice::setModeTuning(int modeIndex, float tuning)
{
    if (modeIndex >= 0 && modeIndex < modes_.size()) {
        modes_[modeIndex].targetFrequency *= (1.0f + tuning * 0.1f);
    }
}

void PercussionModelVoice::setAttack(float attack)
{
    attack_ = juce::jlimit(0.001f, 1.0f, attack);
    envelope_.attackTime = attack_;
}

void PercussionModelVoice::setDecayShape(float shape)
{
    decayShape_ = juce::jlimit(0.0f, 1.0f, shape);
    envelope_.decayTime = decay_ * (1.0f + decayShape_);
}

void PercussionModelVoice::setSustain(float sustain)
{
    sustain_ = juce::jlimit(0.0f, 1.0f, sustain);
    envelope_.sustainLevel = sustain_;
}

void PercussionModelVoice::setRelease(float release)
{
    release_ = juce::jlimit(0.001f, 2.0f, release);
    envelope_.releaseTime = release_;
}

void PercussionModelVoice::setNoiseAmount(float amount)
{
    noiseAmount_ = juce::jlimit(0.0f, 1.0f, amount);
}

void PercussionModelVoice::setNoiseColor(float color)
{
    noiseColor_ = juce::jlimit(0.0f, 1.0f, color);
    updateFilters();
}

void PercussionModelVoice::setMetallicResonance(float amount)
{
    metallicResonance_ = juce::jlimit(0.0f, 1.0f, amount);
    updateFilters();
}

void PercussionModelVoice::setAirResonance(float amount)
{
    airResonance_ = juce::jlimit(0.0f, 1.0f, amount);
}

void PercussionModelVoice::setPitchBend(float bendAmount)
{
    pitchBend_ = juce::jlimit(-1.0f, 1.0f, bendAmount);
    smoothedFrequency_.setTargetValue(fundamentalFrequency_ * (1.0f + pitchBend_ * 0.02f));
}

void PercussionModelVoice::setPressure(float pressure)
{
    pressure_ = juce::jlimit(0.0f, 1.0f, pressure);
    smoothedAmplitude_.setTargetValue(velocity_ * pressure);
}

void PercussionModelVoice::setTimbre(float timbre)
{
    timbre_ = juce::jlimit(0.0f, 1.0f, timbre);
    tone_ = timbre;
    updateFilters();
}

void PercussionModelVoice::setSmoothingTime(float timeMs)
{
    if (timeMs > 0.0f) {
        smoothedAmplitude_.reset(sampleRate_, timeMs / 1000.0f);
        smoothedFrequency_.reset(sampleRate_, timeMs / 1000.0f);
        smoothedDecay_.reset(sampleRate_, timeMs / 1000.0f);
    }
}

float PercussionModelVoice::getSmoothingTime() const
{
    return 50.0f; // Default smoothing time
}

// Getters
bool PercussionModelVoice::isActive() const
{
    return isActive_;
}

float PercussionModelVoice::getRmsLevel() const
{
    return rmsLevel_;
}

float PercussionModelVoice::getPeakLevel() const
{
    return peakLevel_;
}

float PercussionModelVoice::getFundamentalFrequency() const
{
    return fundamentalFrequency_;
}

juce::Array<float> PercussionModelVoice::getModeAmplitudes() const
{
    juce::Array<float> amplitudes;
    for (const auto& mode : modes_) {
        amplitudes.add(mode.amplitude);
    }
    return amplitudes;
}

float PercussionModelVoice::getVoiceAge() const
{
    return voiceAge_;
}

void PercussionModelVoice::updateAge()
{
    voiceAge_ = (juce::Time::getMillisecondCounterHiRes() - voiceStartTime_) / 1000.0f;
}

} // namespace Zenith
