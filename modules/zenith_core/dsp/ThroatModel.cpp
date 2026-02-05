/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ThroatModel.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Vocal tract physical modeling implementation.

  ==============================================================================

*/

#include "ThroatModel.h"
#include <cmath>

namespace zenith {
namespace dsp {

//==============================================================================
// FormantFilter implementation
void ThroatModel::FormantFilter::calculateCoefficients()
{
    // Normalize frequency
    float omega = 2.0f * juce::MathConstants<float>::pi * frequency / 44100.0f;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    
    float alpha = sinOmega / (2.0f * bandwidth / frequency);
    
    // Bandpass filter coefficients
    b0 = alpha;
    b1 = 0.0f;
    b2 = -alpha;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cosOmega;
    a2 = 1.0f - alpha;
    
    // Normalize
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    a0 = 1.0f;
}

float ThroatModel::FormantFilter::process(float input)
{
    float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output * gain;
}

//==============================================================================
ThroatModel::ThroatModel()
{
}

ThroatModel::~ThroatModel()
{
}

//==============================================================================
void ThroatModel::prepare(double sampleRate, int maxBlockSize)
{
    sampleRate_ = sampleRate;
    
    // Initialize tube sections
    for (auto& section : sections_)
    {
        int delayLength = static_cast<int>(sampleRate * 0.001f);  // 1ms default
        section.forwardDelay.resize(delayLength, 0.0f);
        section.backwardDelay.resize(delayLength, 0.0f);
        section.writePos = 0;
    }
    
    // Initialize formant filters
    // Typical vowel formants (can be modified by character parameter)
    formants_[0].frequency = 800.0f;   // F1
    formants_[0].bandwidth = 80.0f;
    formants_[0].gain = 1.0f;
    
    formants_[1].frequency = 1200.0f;  // F2
    formants_[1].bandwidth = 100.0f;
    formants_[1].gain = 0.8f;
    
    formants_[2].frequency = 2500.0f;  // F3
    formants_[2].bandwidth = 120.0f;
    formants_[2].gain = 0.6f;
    
    formants_[3].frequency = 3500.0f;  // F4
    formants_[3].bandwidth = 150.0f;
    formants_[3].gain = 0.4f;
    
    for (auto& formant : formants_)
    {
        formant.calculateCoefficients();
    }
    
    updateVocalTractShape();
}

void ThroatModel::reset()
{
    for (auto& section : sections_)
    {
        std::fill(section.forwardDelay.begin(), section.forwardDelay.end(), 0.0f);
        std::fill(section.backwardDelay.begin(), section.backwardDelay.end(), 0.0f);
        section.writePos = 0;
    }
    
    for (auto& formant : formants_)
    {
        formant.x1 = formant.x2 = 0.0f;
        formant.y1 = formant.y2 = 0.0f;
    }
}

//==============================================================================
void ThroatModel::updateVocalTractShape()
{
    // Calculate vocal tract dimensions based on parameters
    float length = length_.load();
    float width = width_.load();
    float character = character_.load();
    
    // Map length to total delay time (2-10cm vocal tract)
    // Speed of sound ~34300 cm/s
    // Round trip = 2 * length / speed
    float tractLengthCm = 8.0f + length * 8.0f;  // 8-16cm
    float totalDelayMs = (2.0f * tractLengthCm) / 343.0f * 1000.0f;
    
    // Distribute among sections
    float delayPerSection = totalDelayMs / kNumSections;
    
    for (auto& section : sections_)
    {
        int delaySamples = static_cast<int>(delayPerSection * sampleRate_ / 1000.0f);
        delaySamples = juce::jmax(1, delaySamples);
        
        if (static_cast<int>(section.forwardDelay.size()) != delaySamples)
        {
            section.forwardDelay.resize(delaySamples, 0.0f);
            section.backwardDelay.resize(delaySamples, 0.0f);
        }
    }
    
    // Calculate section areas based on character (vowel shape)
    for (int i = 0; i < kNumSections; ++i)
    {
        float position = i / static_cast<float>(kNumSections - 1);  // 0-1 along tract
        
        // Base area from width parameter
        float area = 0.5f + width * 1.5f;  // 0.5-2.0 cm^2
        
        // Modify by character (vowel shape)
        if (character < 0.33f)
        {
            // Toward "ah" - open throughout
            area *= (1.0f + 0.3f * std::sin(position * juce::MathConstants<float>::pi));
        }
        else if (character < 0.66f)
        {
            // Toward "ee" - constriction at middle
            float constriction = std::sin(position * juce::MathConstants<float>::pi * 2.0f);
            area *= (0.6f + 0.4f * constriction);
        }
        else
        {
            // Toward "oo" - constriction at lips, expansion at back
            area *= (0.4f + 0.6f * (1.0f - position));
        }
        
        sections_[i].area = area;
    }
    
    // Calculate reflection coefficients between sections
    for (int i = 0; i < kNumSections - 1; ++i)
    {
        float a1 = sections_[i].area;
        float a2 = sections_[i + 1].area;
        sections_[i].reflection = (a2 - a1) / (a2 + a1);
    }
    sections_[kNumSections - 1].reflection = 0.9f;  // Lip termination
    
    // Update formant frequencies based on character
    float shift = formantShift_.load();
    
    if (character < 0.33f)
    {
        // "ah" - low F1, mid F2
        formants_[0].frequency = 800.0f + shift * 100.0f;
        formants_[1].frequency = 1200.0f + shift * 150.0f;
        formants_[2].frequency = 2500.0f + shift * 200.0f;
    }
    else if (character < 0.66f)
    {
        // "ee" - low F1, high F2
        formants_[0].frequency = 400.0f + shift * 100.0f;
        formants_[1].frequency = 2200.0f + shift * 150.0f;
        formants_[2].frequency = 3000.0f + shift * 200.0f;
    }
    else
    {
        // "oo" - low F1, low F2
        formants_[0].frequency = 300.0f + shift * 100.0f;
        formants_[1].frequency = 800.0f + shift * 150.0f;
        formants_[2].frequency = 2200.0f + shift * 200.0f;
    }
    
    for (auto& formant : formants_)
    {
        formant.calculateCoefficients();
    }
}

//==============================================================================
void ThroatModel::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled_.load())
        return;
    
    // Update vocal tract shape if parameters changed significantly
    static float lastLength = -1.0f;
    static float lastCharacter = -1.0f;
    
    if (std::abs(length_.load() - lastLength) > 0.01f ||
        std::abs(character_.load() - lastCharacter) > 0.01f)
    {
        updateVocalTractShape();
        lastLength = length_.load();
        lastCharacter = character_.load();
    }
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        
        // Process through formant filters (parallel)
        std::vector<float> formantOutput(numSamples, 0.0f);
        
        for (auto& formant : formants_)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                formantOutput[i] += formant.process(channelData[i]);
            }
        }
        
        // Mix formant output back
        float formantMix = 0.7f;  // How much formant shaping to apply
        for (int i = 0; i < numSamples; ++i)
        {
            channelData[i] = channelData[i] * (1.0f - formantMix) + formantOutput[i] * formantMix;
        }
        
        // Add breathiness
        if (breathiness_.load() > 0.0f)
        {
            addBreathiness(channelData, numSamples);
        }
        
        // Apply formant shift if needed
        if (std::abs(formantShift_.load()) > 0.5f)
        {
            applyFormantShift(channelData, numSamples);
        }
    }
}

void ThroatModel::addBreathiness(float* samples, int numSamples)
{
    float breathiness = breathiness_.load();
    
    for (int i = 0; i < numSamples; ++i)
    {
        // Generate pink-ish noise
        float noise = (noiseGenerator_.nextFloat() * 2.0f - 1.0f);
        noise += (noiseGenerator_.nextFloat() * 2.0f - 1.0f);
        noise *= 0.5f;
        
        // Highpass the noise for aspiration character
        static float noiseState = 0.0f;
        noiseState = noiseState * 0.9f + noise * 0.1f;
        float highpassNoise = noise - noiseState;
        
        // Mix in breathiness
        samples[i] += highpassNoise * breathiness * 0.3f;
    }
}

void ThroatModel::applyFormantShift(float* samples, int numSamples)
{
    // Simple formant shift using filtering
    // In a full implementation, this would use LPC resynthesis
    
    float shift = formantShift_.load();
    
    // Tilt EQ to simulate formant shift
    static float prevSample = 0.0f;
    float tilt = shift / 100.0f;  // -0.12 to +0.12
    
    for (int i = 0; i < numSamples; ++i)
    {
        float current = samples[i];
        samples[i] = current - tilt * (current - prevSample);
        prevSample = current;
    }
}

//==============================================================================
void ThroatModel::loadPreset(Preset preset)
{
    switch (preset)
    {
        case Preset::Default:
            setLength(0.5f);
            setWidth(0.5f);
            setBreathiness(0.0f);
            setCharacter(0.5f);
            setFormantShift(0.0f);
            break;
            
        case Preset::Soprano:
            setLength(0.2f);
            setWidth(0.3f);
            setBreathiness(0.1f);
            setCharacter(0.3f);
            setFormantShift(3.0f);
            break;
            
        case Preset::Alto:
            setLength(0.35f);
            setWidth(0.4f);
            setBreathiness(0.05f);
            setCharacter(0.4f);
            setFormantShift(1.0f);
            break;
            
        case Preset::Tenor:
            setLength(0.5f);
            setWidth(0.5f);
            setBreathiness(0.0f);
            setCharacter(0.5f);
            setFormantShift(0.0f);
            break;
            
        case Preset::Baritone:
            setLength(0.65f);
            setWidth(0.6f);
            setBreathiness(0.0f);
            setCharacter(0.6f);
            setFormantShift(-2.0f);
            break;
            
        case Preset::Bass:
            setLength(0.8f);
            setWidth(0.7f);
            setBreathiness(0.0f);
            setCharacter(0.7f);
            setFormantShift(-4.0f);
            break;
            
        case Preset::Child:
            setLength(0.15f);
            setWidth(0.25f);
            setBreathiness(0.05f);
            setCharacter(0.2f);
            setFormantShift(5.0f);
            break;
            
        case Preset::Monster:
            setLength(1.0f);
            setWidth(1.0f);
            setBreathiness(0.2f);
            setCharacter(0.8f);
            setFormantShift(-8.0f);
            break;
            
        case Preset::Robot:
            setLength(0.5f);
            setWidth(0.5f);
            setBreathiness(0.0f);
            setCharacter(0.5f);
            setFormantShift(0.0f);
            break;
            
        case Preset::Telephone:
            setLength(0.5f);
            setWidth(0.3f);
            setBreathiness(0.0f);
            setCharacter(0.5f);
            setFormantShift(0.0f);
            // Will apply bandpass effect
            break;
    }
    
    updateVocalTractShape();
}

} // namespace dsp
} // namespace zenith
