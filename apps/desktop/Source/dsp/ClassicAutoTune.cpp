/*
  ==============================================================================

    ClassicAutoTune.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Classic Auto-Tune 5 algorithm recreation.
    
    This is NOT trying to be transparent. It's trying to sound like
    the iconic Auto-Tune effect from the 2000s.

  ==============================================================================
*/

#include "ClassicAutoTune.h"
#include <cmath>

namespace zenith {
namespace dsp {

//==============================================================================
void ClassicAutoTune::FormantFilter::setPeaking(float freq, float q, float gain, float sampleRate)
{
    float omega = 2.0f * juce::MathConstants<float>::pi * freq / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * q);
    float A = std::sqrt(gain);
    
    b0 = 1.0f + alpha * A;
    b1 = -2.0f * cosOmega;
    b2 = 1.0f - alpha * A;
    a0 = 1.0f + alpha / A;
    a1 = -2.0f * cosOmega;
    a2 = 1.0f - alpha / A;
    
    // Normalize
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    a0 = 1.0f;
}

float ClassicAutoTune::FormantFilter::process(float input)
{
    float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    return output;
}

//==============================================================================
ClassicAutoTune::ClassicAutoTune()
{
    scaleNotes_.fill(true);  // Chromatic by default
}

ClassicAutoTune::~ClassicAutoTune()
{
}

void ClassicAutoTune::prepare(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    
    // Setup delay buffer (creates the "step" effect)
    delayLength_ = static_cast<int>(sampleRate * 0.01);  // 10ms max delay
    delayBuffer_.resize(delayLength_, 0.0f);
    delayWritePos_ = 0;
    delayReadPos_ = 0;
    
    // Initialize formant filters
    for (auto& filter : formantFilters_)
    {
        filter.setPeaking(1000.0f, 2.0f, 1.0f, static_cast<float>(sampleRate));
    }
    
    reset();
}

void ClassicAutoTune::reset()
{
    currentSemitoneShift_ = 0.0f;
    targetSemitoneShift_ = 0.0f;
    smoothedPitch_ = 0.0f;
    
    std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);
    delayWritePos_ = 0;
    delayReadPos_ = 0;
    
    for (auto& filter : formantFilters_)
    {
        filter.x1 = filter.x2 = 0.0f;
        filter.y1 = filter.y2 = 0.0f;
    }
}

//==============================================================================
float ClassicAutoTune::midiNoteToFreq(int note) const
{
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

int ClassicAutoTune::freqToMidiNote(float freq) const
{
    if (freq <= 0.0f) return 0;
    return static_cast<int>(69 + 12 * std::log2(freq / 440.0f) + 0.5f);
}

float ClassicAutoTune::quantizeToScale(float pitchHz)
{
    if (!useScale_ || pitchHz <= 0.0f)
        return pitchHz;
    
    int midiNote = freqToMidiNote(pitchHz);
    int octave = midiNote / 12;
    int noteInOctave = midiNote % 12;
    
    // Find nearest note in scale
    int bestNote = midiNote;
    int minDistance = 12;
    
    for (int i = 0; i < 12; ++i)
    {
        if (scaleNotes_[i])
        {
            int testNote = octave * 12 + i;
            int distance = std::abs(testNote - midiNote);
            if (distance < minDistance)
            {
                minDistance = distance;
                bestNote = testNote;
            }
        }
    }
    
    return midiNoteToFreq(bestNote);
}

float ClassicAutoTune::correctPitchClassic(float inputPitch, float& smoothedPitch)
{
    if (inputPitch <= 0.0f)
    {
        smoothedPitch = 0.0f;
        return 0.0f;
    }
    
    // Quantize to scale (hard quantization is key to Classic sound)
    float targetPitch = quantizeToScale(inputPitch);
    
    // Calculate semitone shift
    float currentSemitones = 12.0f * std::log2(inputPitch / 440.0f);
    float targetSemitones = 12.0f * std::log2(targetPitch / 440.0f);
    float semitoneShift = targetSemitones - currentSemitones;
    
    // Apply humanize (adds slight variation for less robotic sound)
    float humanize = humanize_.load();
    if (humanize > 0.0f)
    {
        // Reduce correction amount based on humanize
        semitoneShift *= (1.0f - humanize * 0.5f);
    }
    
    // Smooth the pitch shift (creates the "step" sound)
    // Lower retune speed = faster correction = more robotic
    float retuneSpeed = retuneSpeed_.load();
    
    // Map retune speed (0-100) to smoothing coefficient
    // 0 = instant (coefficient = 1.0)
    // 100 = very slow (coefficient = 0.01)
    float speedFactor = (100.0f - retuneSpeed) / 100.0f;
    smoothingCoeff_ = 0.01f + speedFactor * 0.99f;
    
    // Exponential smoothing
    smoothedPitch += smoothingCoeff_ * (semitoneShift - smoothedPitch);
    
    return smoothedPitch;
}

void ClassicAutoTune::applyFormantShift(float* samples, int numSamples, float shiftSemitones)
{
    if (std::abs(shiftSemitones) < 0.5f)
        return;
    
    // Simple formant shift using peaking filters
    // Shift formants opposite to pitch shift
    float formantShift = -shiftSemitones * 0.5f;  // Compensate partially
    
    // Adjust filter frequencies based on shift
    float baseFreqs[] = {800.0f, 1200.0f, 2500.0f, 3500.0f};
    
    for (int f = 0; f < 4; ++f)
    {
        float newFreq = baseFreqs[f] * std::pow(2.0f, formantShift / 12.0f);
        formantFilters_[f].setPeaking(newFreq, 2.0f, 1.2f, static_cast<float>(sampleRate_));
    }
    
    // Process through filters
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = samples[i];
        for (auto& filter : formantFilters_)
        {
            sample = filter.process(sample);
        }
        samples[i] = sample;
    }
}

//==============================================================================
void ClassicAutoTune::process(const juce::AudioBuffer<float>& input,
                              juce::AudioBuffer<float>& output,
                              const std::vector<float>& detectedPitches)
{
    const int numSamples = input.getNumSamples();
    const int numChannels = input.getNumChannels();
    
    // Copy input to output
    for (int ch = 0; ch < numChannels; ++ch)
    {
        output.copyFrom(ch, 0, input, ch, 0, numSamples);
    }
    
    if (numChannels == 0 || numSamples == 0)
        return;
    
    // Process mono for pitch correction
    const float* inputMono = input.getReadPointer(0);
    float* outputMono = output.getWritePointer(0);
    
    float correctionAmount = correctionAmount_.load();
    bool preserveFormants = preserveFormants_.load();
    
    for (int i = 0; i < numSamples; ++i)
    {
        // Get detected pitch for this sample (interpolate if needed)
        int pitchIndex = static_cast<int>((static_cast<float>(i) / numSamples) * detectedPitches.size());
        pitchIndex = juce::jlimit(0, static_cast<int>(detectedPitches.size()) - 1, pitchIndex);
        float detectedPitch = detectedPitches[pitchIndex];
        
        float sample = inputMono[i];
        
        if (detectedPitch > 0.0f)
        {
            // Get pitch correction amount
            float pitchShift = correctPitchClassic(detectedPitch, smoothedPitch_);
            
            // Apply correction amount
            pitchShift *= correctionAmount;
            
            // Apply pitch shift (simple resampling approach for Classic mode)
            // This creates the characteristic "steppy" artifacts
            if (std::abs(pitchShift) > 0.01f)
            {
                // Simple pitch shift using delay line
                float shiftRatio = std::pow(2.0f, pitchShift / 12.0f);
                
                // Write to delay buffer
                delayBuffer_[delayWritePos_] = sample;
                delayWritePos_ = (delayWritePos_ + 1) % delayLength_;
                
                // Read with offset (creates pitch shift)
                int readOffset = static_cast<int>((1.0f - shiftRatio) * delayLength_ * 0.1f);
                int readPos = (delayWritePos_ + delayLength_ - readOffset) % delayLength_;
                
                sample = delayBuffer_[readPos];
            }
        }
        
        outputMono[i] = sample;
    }
    
    // Apply formant preservation if enabled
    if (preserveFormants && smoothedPitch_ != 0.0f)
    {
        applyFormantShift(outputMono, numSamples, smoothedPitch_);
    }
    
    // Copy mono to stereo if needed
    if (numChannels > 1)
    {
        for (int ch = 1; ch < numChannels; ++ch)
        {
            output.copyFrom(ch, 0, output, 0, 0, numSamples);
        }
    }
}

//==============================================================================
void ClassicAutoTune::loadPreset(Preset preset)
{
    switch (preset)
    {
        case Preset::Robot:
            retuneSpeed_ = 0.0f;      // Instant
            correctionAmount_ = 1.0f;  // Full
            humanize_ = 0.0f;          // No variation
            preserveFormants_ = true;
            break;
            
        case Preset::TPain:
            retuneSpeed_ = 15.0f;      // Fast
            correctionAmount_ = 1.0f;  // Full
            humanize_ = 0.1f;          // Slight variation
            preserveFormants_ = true;
            break;
            
        case Preset::Cher:
            retuneSpeed_ = 30.0f;      // Medium
            correctionAmount_ = 0.9f;  // Strong
            humanize_ = 0.2f;          // Some variation
            preserveFormants_ = true;
            break;
            
        case Preset::ModernClassic:
            retuneSpeed_ = 50.0f;      // Slower
            correctionAmount_ = 0.8f;  // Strong but not full
            humanize_ = 0.3f;          // More natural
            preserveFormants_ = true;
            break;
            
        case Preset::Subtle:
            retuneSpeed_ = 70.0f;      // Slow
            correctionAmount_ = 0.5f;  // Half
            humanize_ = 0.5f;          // Natural
            preserveFormants_ = false;
            break;
    }
}

} // namespace dsp
} // namespace zenith
