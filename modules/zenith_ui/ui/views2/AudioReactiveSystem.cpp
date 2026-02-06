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

#include "AudioReactiveSystem.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

AudioReactiveSystem::AudioReactiveSystem()
    : fft_(512) // Use 512-point FFT
{
    // Initialize beat history
    beatHistory_.fill(0.5f);

    // Setup audio buffer
    audioBuffer_.setSize(2, 2048); // Stereo, 2048 samples

    // Initialize FFT data
    fftData_.fill(0.0f);

    // Start timer for visual updates
    startTimer(16); // 60 FPS visual updates

    lastProcessTime_ = juce::Time::getMillisecondCounter();
}

AudioReactiveSystem::~AudioReactiveSystem() {
    isAudioEnabled_ = false;
    isAudioSetup_ = false;
    audioBuffer_.clear();
    stopTimer();
}

//==============================================================================
// Audio Device Setup
//==============================================================================

bool AudioReactiveSystem::setupAudioAnalysis(juce::AudioDeviceManager& deviceManager,
                                           double sampleRate,
                                           int bufferSize)
{
    sampleRate_ = sampleRate;
    bufferSize_ = bufferSize;

    // Prepare audio buffer
    audioBuffer_.setSize(2, bufferSize * 4); // 4x buffer for smooth analysis
    audioBuffer_.clear();

    // Register this as an audio callback
    // Note: We add ourselves as a callback to receive audio input
    // Register this as an audio callback
    deviceManager.addAudioCallback(this);
    isAudioSetup_ = true;
    isAudioEnabled_ = true;

    // Initialize effects with default values
    effects_.pulseEffect.amplitude = 0.0f;
    effects_.pulseEffect.frequency = 2.0f;
    effects_.pulseEffect.phase = 0.0f;
    effects_.pulseEffect.beatSynced = true;

    effects_.colorMod.hueShift = 0.0f;
    effects_.colorMod.saturation = 1.0f;
    effects_.colorMod.brightness = 1.0f;
    effects_.colorMod.respondToBass = true;
    effects_.colorMod.respondToMids = false;
    effects_.colorMod.respondToTreble = false;

    effects_.distortionEffect.amount = 0.0f;
    effects_.distortionEffect.frequency = 5000.0f;
    effects_.distortionEffect.color = juce::Colours::red;

    // Add default glow effects
    addGlowEffect(0, 20.0f, juce::Colour::fromFloatRGBA(1.0f, 0.5f, 0.2f, 0.8f)); // Bass
    addGlowEffect(12, 15.0f, juce::Colour::fromFloatRGBA(0.2f, 1.0f, 0.5f, 0.6f)); // Mids
    addGlowEffect(24, 10.0f, juce::Colour::fromFloatRGBA(0.2f, 0.5f, 1.0f, 0.4f)); // Treble

    return true;

    return false;
}

void AudioReactiveSystem::shutdownAudioAnalysis(juce::AudioDeviceManager& deviceManager) {
    isAudioEnabled_ = false;
    isAudioSetup_ = false;

    // Remove audio callback
    // Remove audio callback
    deviceManager.removeAudioCallback(this);

    audioBuffer_.clear();
}

//==============================================================================
// Configuration
//==============================================================================

void AudioReactiveSystem::setReactiveConfig(const AudioReactiveConfig& config) {
    config_ = config;
}

void AudioReactiveSystem::setEnabled(bool enabled) {
    isAudioEnabled_ = enabled;
}

//==============================================================================
// Visual Effects
//==============================================================================

int AudioReactiveSystem::addGlowEffect(int frequency, float size, const juce::Colour& color) {
    if (frequency < 0 || frequency >= 32) {
        return -1;
    }

    AudioReactiveEffects::GlowEffect effect;
    effect.frequency = static_cast<float>(frequency);
    effect.size = size;
    effect.color = color;
    effect.intensity = 0.0f;
    effect.targetIntensity = 0.0f;

    effects_.glowEffects.push_back(effect);
    return static_cast<int>(effects_.glowEffects.size()) - 1;
}

void AudioReactiveSystem::removeGlowEffect(int index) {
    if (index >= 0 && index < static_cast<int>(effects_.glowEffects.size())) {
        effects_.glowEffects.erase(effects_.glowEffects.begin() + index);
    }
}

void AudioReactiveSystem::configurePulseEffect(float frequency, bool beatSynced) {
    effects_.pulseEffect.frequency = frequency;
    effects_.pulseEffect.beatSynced = beatSynced;
}

void AudioReactiveSystem::configureColorModulation(bool respondToBass,
                                                bool respondToMids,
                                                bool respondToTreble) {
    effects_.colorMod.respondToBass = respondToBass;
    effects_.colorMod.respondToMids = respondToMids;
    effects_.colorMod.respondToTreble = respondToTreble;
}

void AudioReactiveSystem::configureDistortionEffect(float amount, float frequency, const juce::Colour& color) {
    effects_.distortionEffect.amount = amount;
    effects_.distortionEffect.frequency = frequency;
    effects_.distortionEffect.color = color;
}

//==============================================================================
// Audio Data Access
//==============================================================================

juce::Colour AudioReactiveSystem::getAudioReactiveColor(int trackIndex, int sceneIndex,
                                                     const juce::Colour& baseColor) const
{
    if (!isAudioEnabled_) {
        return baseColor;
    }

    // Calculate color modulation based on analysis
    float hueShift = effects_.colorMod.hueShift;
    float saturation = effects_.colorMod.saturation;
    float brightness = effects_.colorMod.brightness;

    // Apply bass response to hue
    if (effects_.colorMod.respondToBass) {
        hueShift += analysisData_.bassLevel * config_.bassSensitivity * 0.1f;
    }

    // Apply mid response to saturation
    if (effects_.colorMod.respondToMids) {
        saturation += (analysisData_.midLevel - 50.0f) * config_.midSensitivity * 0.2f;
        saturation = juce::jlimit(0.0f, 1.0f, saturation);
    }

    // Apply treble response to brightness
    if (effects_.colorMod.respondToTreble) {
        brightness += (analysisData_.trebleLevel - 50.0f) * config_.trebleSensitivity * 0.2f;
        brightness = juce::jlimit(0.0f, 1.0f, brightness);
    }

    // Convert to HSV, apply modulation, convert back
    float h, s, v;
    baseColor.getHSB(h, s, v);

    h = fmod(h + hueShift, 360.0f);
    s *= saturation;
    v *= brightness;

    return juce::Colour::fromHSV(h / 360.0f, s, v, 1.0f);
}

float AudioReactiveSystem::getAudioReactivePhase(float basePhase) const {
    if (!isAudioEnabled_) {
        return basePhase;
    }

    // Enhance phase based on beat detection
    if (analysisData_.isOnBeat) {
        return fmod(basePhase + effects_.pulseEffect.phase, juce::MathConstants<float>::twoPi);
    }

    // Apply tempo-based animation scaling
    float tempoFactor = 1.0f + (analysisData_.tempo / 120.0f - 1.0f) * 0.5f;

    return basePhase * tempoFactor;
}

float AudioReactiveSystem::getGlowIntensity(int frequency) const {
    if (!isAudioEnabled_ || frequency < 0 || frequency >= 32) {
        return 0.0f;
    }

    // Find closest glow effect
    float minDistance = std::numeric_limits<float>::max();
    float closestIntensity = 0.0f;

    for (const auto& effect : effects_.glowEffects) {
        float distance = std::abs(effect.frequency - static_cast<float>(frequency));
        if (distance < minDistance) {
            minDistance = distance;
            closestIntensity = effect.intensity;
        }
    }

    return closestIntensity * config_.glowIntensityScale;
}

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void AudioReactiveSystem::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    // Initialize audio processing
    audioBuffer_.setSize(device->getActiveInputChannels().countNumberOfSetBits(), bufferSize_);
}

void AudioReactiveSystem::audioDeviceStopped() {
    // Clear audio data
    audioBuffer_.clear();
}

void AudioReactiveSystem::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                        int numInputChannels,
                                                        float* const* outputChannelData,
                                                        int numOutputChannels,
                                                        int numSamples,
                                                        const juce::AudioIODeviceCallbackContext& context)
{
    if (!isAudioEnabled_ || numInputChannels == 0) {
        return;
    }
    
    // Clear output if needed (we don't output audio, just analyze)
    for (int i = 0; i < numOutputChannels; ++i)
        if (outputChannelData[i])
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);

    // Copy input data to analysis buffer
    int samplesToCopy = std::min(numSamples, bufferSize_);
    
    if (numInputChannels > 0 && inputChannelData[0]) {
        audioBuffer_.copyFrom(0, 0, inputChannelData[0], samplesToCopy);
    }
    
    if (numInputChannels > 1 && inputChannelData[1] && audioBuffer_.getNumChannels() > 1) {
        audioBuffer_.copyFrom(1, 0, inputChannelData[1], samplesToCopy);
    }

    // Process audio analysis
    if (numInputChannels > 0 && inputChannelData[0]) {
        processAudio(inputChannelData[0], numSamples);
    }
}

//==============================================================================
// Audio Analysis Methods
//==============================================================================

void AudioReactiveSystem::processAudio(const float* inputAudio, int numSamples) {
    if (!isAudioEnabled_ || numSamples < 64) {
        return;
    }

    // Perform FFT analysis on smaller buffer for real-time performance
    const int fftSize = 512;
    const float* audioPtr = inputAudio;

    // Apply window function
    std::array<float, fftSize> windowedData;
    for (int i = 0; i < fftSize && i < numSamples; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
        windowedData[i] = audioPtr[i] * window;
    }

    // Fill remaining with zeros
    for (int i = numSamples; i < fftSize; ++i) {
        windowedData[i] = 0.0f;
    }

    performFFT(windowedData.data(), fftSize);
    analyzeFrequencies();
    detectTransients(inputAudio, numSamples);
    detectTempoAndBeat();
    updateAnalysisData();
}

void AudioReactiveSystem::performFFT(const float* audioData, int fftSize) {
    // Prepare real FFT data
    for (int i = 0; i < fftSize / 2; ++i) {
        fftData_[i] = audioData[i];
    }

    // Perform FFT
    fft_.performRealOnlyForwardTransform(fftData_.data());

    // Convert to magnitude spectrum
    for (int i = 0; i < fftSize / 2; ++i) {
        fftData_[i] = std::abs(fftData_[i]);
    }
}

void AudioReactiveSystem::analyzeFrequencies() {
    // Convert FFT data to frequency bands
    const int numBands = 32;
    const int bandWidth = 512 / numBands;

    analysisData_.frequencies.fill(0.0f);

    for (int band = 0; band < numBands; ++band) {
        float sum = 0.0f;
        int start = band * bandWidth;
        int end = juce::jmin(start + bandWidth, 256);

        for (int i = start; i < end; ++i) {
            sum += fftData_[i];
        }

        float average = sum / static_cast<float>(end - start);
        analysisData_.frequencies[band] = average;
    }

    // Calculate frequency averages
    float bassSum = 0.0f, midSum = 0.0f, trebleSum = 0.0f;

    for (int i = 0; i < 4; ++i) { // Bass: first 4 bands
        bassSum += analysisData_.frequencies[i];
    }
    analysisData_.bassLevel = bassSum / 4.0f * 100.0f;

    for (int i = 8; i < 16; ++i) { // Mids: bands 8-16
        midSum += analysisData_.frequencies[i];
    }
    analysisData_.midLevel = midSum / 8.0f * 100.0f;

    for (int i = 24; i < 32; ++i) { // Treble: bands 24-32
        trebleSum += analysisData_.frequencies[i];
    }
    analysisData_.trebleLevel = trebleSum / 8.0f * 100.0f;

    // Calculate overall level
    float sum = 0.0f;
    for (float freq : analysisData_.frequencies) {
        sum += freq;
    }
    analysisData_.overallLevel = sum / 32.0f / 1000.0f; // Normalize

    // Calculate spectral centroid
    float weightedSum = 0.0f;
    float totalSum = 0.0f;
    for (int i = 0; i < 32; ++i) {
        float frequency = (i * 22050.0f) / 32.0f; // Map to Hz
        weightedSum += frequency * analysisData_.frequencies[i];
        totalSum += analysisData_.frequencies[i];
    }
    analysisData_.spectralCentroid = totalSum > 0 ? weightedSum / totalSum : 0.0f;

    // Calculate zero crossing rate
    float crossings = 0.0f;
    for (int i = 1; i < 512; ++i) {
        if ((audioBuffer_.getSample(0, i - 1) < 0) != (audioBuffer_.getSample(0, i) < 0)) {
            crossings++;
        }
    }
    analysisData_.zeroCrossingRate = crossings / 512.0f;
}

void AudioReactiveSystem::detectTempoAndBeat() {
    // Simple beat detection using energy difference
    float currentEnergy = analysisData_.overallLevel;
    float previousEnergy = previousAnalysisData_.overallLevel;

    // Beat detection threshold
    float beatThreshold = 0.3f;
    float energyDifference = currentEnergy - previousEnergy;

    // Update beat phase based on tempo
    if (analysisData_.tempo > 0) {
        double beatInterval = 60.0 / analysisData_.tempo;
        analysisData_.phase = fmod((juce::Time::getMillisecondCounter() / 1000.0), beatInterval) / beatInterval;

        // Check if on beat
        if (energyDifference > beatThreshold && analysisData_.phase < 0.1) {
            analysisData_.isOnBeat = true;
            analysisData_.beatIntensity = energyDifference;

            // Trigger beat callback
            if (onBeatDetected) {
                onBeatDetected();
            }
        } else {
            analysisData_.isOnBeat = false;
        }
    }

    // Update beat history
    beatHistory_[beatHistoryIndex_] = energyDifference;
    beatHistoryIndex_ = (beatHistoryIndex_ + 1) % beatHistory_.size();
}

void AudioReactiveSystem::detectTransients(const float* audioData, int numSamples) {
    // Simple transient detection using zero crossing + amplitude
    float threshold = 0.1f;
    analysisData_.hasTransient = false;
    analysisData_.transientStrength = 0.0f;

    for (int i = 1; i < numSamples; ++i) {
        float prevSample = audioData[i - 1];
        float currentSample = audioData[i];

        // Zero crossing + high amplitude
        if ((prevSample < 0) != (currentSample < 0) && std::abs(currentSample) > threshold) {
            analysisData_.hasTransient = true;
            analysisData_.transientStrength = std::abs(currentSample);
            analysisData_.lastTransientTime = juce::Time::getMillisecondCounter();
            break;
        }
    }
}

void AudioReactiveSystem::updateAnalysisData() {
    // Apply smoothing to all values
    float smoothingFactor = config_.levelSmoothing;

    analysisData_.overallLevel = previousAnalysisData_.overallLevel * smoothingFactor +
                                 analysisData_.overallLevel * (1.0f - smoothingFactor);

    analysisData_.bassLevel = previousAnalysisData_.bassLevel * smoothingFactor +
                             analysisData_.bassLevel * (1.0f - smoothingFactor);

    analysisData_.midLevel = previousAnalysisData_.midLevel * smoothingFactor +
                            analysisData_.midLevel * (1.0f - smoothingFactor);

    analysisData_.trebleLevel = previousAnalysisData_.trebleLevel * smoothingFactor +
                              analysisData_.trebleLevel * (1.0f - smoothingFactor);

    // Update peak level
    if (analysisData_.overallLevel > analysisData_.peakLevel) {
        analysisData_.peakLevel = analysisData_.overallLevel;
    } else {
        analysisData_.peakLevel *= 0.95f; // Decay
    }

    // Store previous data
    previousAnalysisData_ = analysisData_;
}

//==============================================================================
// Visual Effect Methods
//==============================================================================

void AudioReactiveSystem::updateGlowEffects() {
    for (auto& effect : effects_.glowEffects) {
        // Calculate target intensity based on frequency response
        float targetIntensity = 0.0f;

        if (effect.frequency < 8) { // Bass
            targetIntensity = analysisData_.bassLevel * config_.bassSensitivity / 100.0f;
        } else if (effect.frequency < 20) { // Mids
            targetIntensity = analysisData_.midLevel * config_.midSensitivity / 100.0f;
        } else { // Treble
            targetIntensity = analysisData_.trebleLevel * config_.trebleSensitivity / 100.0f;
        }

        // Apply transient response
        if (analysisData_.hasTransient) {
            targetIntensity += analysisData_.transientStrength * config_.transientSensitivity;
        }

        // Smooth intensity transition
        float smoothing = 0.3f;
        effect.targetIntensity = targetIntensity;
        effect.intensity = effect.intensity * smoothing + effect.targetIntensity * (1.0f - smoothing);
    }
}

void AudioReactiveSystem::updatePulseEffect() {
    // Update pulse phase
    effects_.pulseEffect.phase += effects_.pulseEffect.frequency * 2.0f * juce::MathConstants<float>::pi / 60.0f;

    // Apply beat synchronization
    if (effects_.pulseEffect.beatSynced && analysisData_.isOnBeat) {
        effects_.pulseEffect.phase = 0.0f;
    }

    // Calculate pulse amplitude
    effects_.pulseEffect.amplitude = 0.5f + 0.5f * std::sin(effects_.pulseEffect.phase);

    // Scale by beat intensity
    effects_.pulseEffect.amplitude *= (1.0f + analysisData_.beatIntensity * 0.5f);
}

void AudioReactiveSystem::updateColorModulation() {
    if (config_.enableBassReactivity) {
        effects_.colorMod.hueShift = analysisData_.bassLevel * 0.5f * config_.colorShiftScale;
    }

    if (config_.enableMidReactivity) {
        effects_.colorMod.saturation = 0.8f + analysisData_.midLevel * 0.004f * config_.colorShiftScale;
    }

    if (config_.enableTrebleReactivity) {
        effects_.colorMod.brightness = 0.8f + analysisData_.trebleLevel * 0.004f * config_.colorShiftScale;
    }
}

void AudioReactiveSystem::updateDistortionEffect() {
    // Apply distortion based on high frequency content
    if (analysisData_.trebleLevel > 70.0f && analysisData_.hasTransient) {
        effects_.distortionEffect.amount = (analysisData_.trebleLevel - 70.0f) * 0.02f *
                                          config_.transientSensitivity;
    } else {
        effects_.distortionEffect.amount *= 0.9f; // Decay
    }
}

//==============================================================================
// Timer Callback
//==============================================================================

void AudioReactiveSystem::timerCallback() {
    if (!isAudioEnabled_) {
        return;
    }

    // Update visual effects
    updateGlowEffects();
    updatePulseEffect();
    updateColorModulation();
    updateDistortionEffect();

    // Trigger visual update callback
    if (onVisualUpdate) {
        onVisualUpdate();
    }
}

} // namespace zenith::ui