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
    RealTimeAudioBuffer.cpp
    Production real-time audio buffer implementation
  ==============================================================================
*/


#include <chrono>
#include <thread>

namespace zenith {
namespace audio {

// RealTimeAudioBuffer Implementation
RealTimeAudioBuffer::RealTimeAudioBuffer(int numChannels, int bufferSize)
    : numChannels(numChannels), bufferSize(bufferSize) {
    
    // Initialize channel buffers
    channelBuffers.resize(numChannels);
    for (int ch = 0; ch < numChannels; ++ch) {
        channelBuffers[ch] = std::make_unique<LockFreeRingBuffer<float, 65536>>();
    }
    
    // Initialize level monitoring
    // Initialize level monitoring
    for (int ch = 0; ch < numChannels; ++ch) {
        channelLevels.push_back(std::make_unique<std::atomic<float>>(0.0f));
        channelClipping.push_back(std::make_unique<std::atomic<bool>>(false));
    }
}

RealTimeAudioBuffer::~RealTimeAudioBuffer() = default;

bool RealTimeAudioBuffer::writeAudio(const juce::AudioBuffer<float>& source) {
    if (source.getNumChannels() != numChannels) {
        return false;
    }
    
    bool success = true;
    
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* channelData = source.getReadPointer(ch);
        
        for (int sample = 0; sample < source.getNumSamples(); ++sample) {
            if (!channelBuffers[ch]->push(channelData[sample])) {
                success = false;  // Buffer overflow
                dropouts.fetch_add(1);
                break;
            }
        }
    }
    
    // Update levels
    updateLevels(source);
    
    // RT-SAFE: Estimate latency from buffer fill level instead of timing
    estimateLatencyFromBufferLevel();
    
    return success;
}

bool RealTimeAudioBuffer::readAudio(juce::AudioBuffer<float>& destination) {
    if (destination.getNumChannels() != numChannels) {
        return false;
    }
    
    bool success = true;
    
    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = destination.getWritePointer(ch);
        
        for (int sample = 0; sample < destination.getNumSamples(); ++sample) {
            if (!channelBuffers[ch]->pop(channelData[sample])) {
                channelData[sample] = 0.0f;  // Buffer underflow
                success = false;
                dropouts.fetch_add(1);
            }
        }
    }
    
    // RT-SAFE: No latency tracking here (removed mutex)
    
    return success;
}

void RealTimeAudioBuffer::setNumChannels(int channels) {
    if (channels == numChannels) return;
    
    numChannels = channels;
    channelBuffers.resize(channels);
    channelLevels.clear();
    channelClipping.clear();
    
    for (int ch = 0; ch < channels; ++ch) {
        if (!channelBuffers[ch]) {
            channelBuffers[ch] = std::make_unique<LockFreeRingBuffer<float, 65536>>();
        }
        channelLevels.push_back(std::make_unique<std::atomic<float>>(0.0f));
        channelClipping.push_back(std::make_unique<std::atomic<bool>>(false));
    }
}

void RealTimeAudioBuffer::setBufferSize(int size) {
    bufferSize = size;
}

float RealTimeAudioBuffer::getLevel(int channel) const {
    if (channel >= 0 && channel < numChannels) {
        return channelLevels[channel]->load();
    }
    return 0.0f;
}

bool RealTimeAudioBuffer::isClipping(int channel) const {
    if (channel >= 0 && channel < numChannels) {
        return channelClipping[channel]->load();
    }
    return false;
}

void RealTimeAudioBuffer::resetLevels() {
    for (int ch = 0; ch < numChannels; ++ch) {
        channelLevels[ch]->store(0.0f);
        channelClipping[ch]->store(false);
    }
}

float RealTimeAudioBuffer::getAverageLatency() const {
    return averageLatency.load();
}

void RealTimeAudioBuffer::resetStatistics() {
    dropouts.store(0);
    averageLatency.store(0.0f);
    resetLevels();
}

void RealTimeAudioBuffer::updateLevels(const juce::AudioBuffer<float>& buffer) {
    // Hysteresis threshold to reduce unnecessary atomic writes on audio thread
    // Only update if level changes by more than 0.5dB - reduces CPU cache pressure
    static constexpr float kLevelUpdateThreshold = 0.5f;
    
    for (int ch = 0; ch < numChannels && ch < buffer.getNumChannels(); ++ch) {
        const float* channelData = buffer.getReadPointer(ch);
        float maxLevel = 0.0f;
        bool clipping = false;
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float absSample = std::abs(channelData[sample]);
            maxLevel = std::max(maxLevel, absSample);
            
            if (absSample > 0.99f) {
                clipping = true;
            }
        }
        
        // Convert to dB
        float levelDb = maxLevel > 0.001f ? 20.0f * std::log10(maxLevel) : -60.0f;
        
        // Only update atomic if value changed significantly (hysteresis)
        float currentLevel = channelLevels[ch]->load(std::memory_order_relaxed);
        if (std::abs(levelDb - currentLevel) > kLevelUpdateThreshold) {
            channelLevels[ch]->store(levelDb, std::memory_order_relaxed);
        }
        
        // Only update clipping flag if it changed (avoid unnecessary atomic writes)
        bool currentClipping = channelClipping[ch]->load(std::memory_order_relaxed);
        if (clipping != currentClipping) {
            channelClipping[ch]->store(clipping, std::memory_order_relaxed);
        }
    }
}

void RealTimeAudioBuffer::estimateLatencyFromBufferLevel() {
    // RT-SAFE: Calculate latency from ring buffer fill level
    // This is an approximation but doesn't require mutex or syscalls
    if (numChannels > 0 && channelBuffers[0]) {
        size_t fillLevel = channelBuffers[0]->size();
        // Estimate latency in ms based on buffer fill level
        // Assuming ~44.1kHz sample rate, each sample = ~0.0227ms
        float estimatedLatencyMs = static_cast<float>(fillLevel) * 0.0227f;
        averageLatency.store(estimatedLatencyMs);
    }
}

// AudioDeviceManager Implementation
AudioDeviceManager::AudioDeviceManager() {
    deviceManager = std::make_unique<juce::AudioDeviceManager>();
}

AudioDeviceManager::~AudioDeviceManager() {
    shutdown();
}

bool AudioDeviceManager::initialize(double sampleRate, int bufferSize) {
    currentSampleRate = sampleRate;
    currentBufferSize = bufferSize;
    
    // Initialize device manager
    juce::String error = deviceManager->initialise(
        2,  // input channels
        2,  // output channels
        nullptr,  // no default device
        true,  // select default device
        juce::String(),  // preferred device
        nullptr   // preferred setup options
    );
    
    if (error.isNotEmpty()) {
        lastError = "Failed to initialize audio device: " + error;
        return false;
    }
    
    // Get current device
    currentDevice = deviceManager->getCurrentAudioDevice();
    if (!currentDevice) {
        lastError = "No audio device found";
        return false;
    }
    
    // Configure device
    if (!setSampleRate(sampleRate) || !setBufferSize(bufferSize)) {
        return false;
    }
    
    isActive = true;
    return true;
}

void AudioDeviceManager::shutdown() {
    if (isActive) {
        deviceManager->closeAudioDevice();
        isActive = false;
        currentDevice = nullptr;
    }
}

std::vector<juce::String> AudioDeviceManager::getAvailableInputDevices() const {
    std::vector<juce::String> devices;
    const auto& deviceTypes = deviceManager->getAvailableDeviceTypes();
    
    for (auto* deviceType : deviceTypes) {
        auto deviceNames = deviceType->getDeviceNames(true);  // input devices
        for (const auto& name : deviceNames) {
            devices.push_back(name);
        }
    }
    
    return devices;
}

std::vector<juce::String> AudioDeviceManager::getAvailableOutputDevices() const {
    std::vector<juce::String> devices;
    const auto& deviceTypes = deviceManager->getAvailableDeviceTypes();

    
    for (auto* deviceType : deviceTypes) {
        auto deviceNames = deviceType->getDeviceNames(false);  // output devices
        for (const auto& name : deviceNames) {
            devices.push_back(name);
        }
    }
    
    return devices;
}

bool AudioDeviceManager::setSampleRate(double sampleRate) {
    if (!currentDevice) return false;
    
    auto supportedRates = currentDevice->getAvailableSampleRates();
    for (auto rate : supportedRates) {
        if (std::abs(rate - sampleRate) < 0.001) {
            currentSampleRate = sampleRate;
            return true;
        }
    }
    
    lastError = "Sample rate not supported";
    return false;
}

bool AudioDeviceManager::setBufferSize(int bufferSize) {
    if (!currentDevice) return false;
    
    auto supportedSizes = currentDevice->getAvailableBufferSizes();
    for (auto size : supportedSizes) {
        if (size == bufferSize) {
            currentBufferSize = bufferSize;
            return true;
        }
    }
    
    lastError = "Buffer size not supported";
    return false;
}

double AudioDeviceManager::getCurrentSampleRate() const {
    return currentSampleRate;
}

int AudioDeviceManager::getCurrentBufferSize() const {
    return currentBufferSize;
}

void AudioDeviceManager::setAudioCallback(juce::AudioIODeviceCallback* callback) {
    deviceManager->addAudioCallback(callback);
}

void AudioDeviceManager::removeAudioCallback() {
    deviceManager->removeAudioCallback(nullptr);
}

float AudioDeviceManager::getInputLevel(int channel) const {
    if (channel >= 0 && channel < inputLevels.size()) {
        return inputLevels[channel];
    }
    return 0.0f;
}

float AudioDeviceManager::getOutputLevel(int channel) const {
    if (channel >= 0 && channel < outputLevels.size()) {
        return outputLevels[channel];
    }
    return 0.0f;
}

bool AudioDeviceManager::isDeviceActive() const {
    return isActive && currentDevice != nullptr;
}

juce::String AudioDeviceManager::getLastError() const {
    return lastError;
}

bool AudioDeviceManager::hasErrors() const {
    return lastError.isNotEmpty();
}

// SampleRateConverter Implementation
SampleRateConverter::SampleRateConverter() {
    buildSincKernel(0.45);  // Default cutoff
}

SampleRateConverter::~SampleRateConverter() = default;

bool SampleRateConverter::convert(const juce::AudioBuffer<float>& input,
                                 juce::AudioBuffer<float>& output,
                                 double inputSampleRate,
                                 double outputSampleRate,
                                 Quality quality) {
    
    if (inputSampleRate == outputSampleRate) {
        output = input;
        return true;
    }
    
    double ratio = outputSampleRate / inputSampleRate;
    
    switch (quality) {
        case Quality::Linear:
            return convertLinear(input, output, ratio);
        case Quality::Sinc:
        case Quality::Best:
            return convertSinc(input, output, ratio);
        default:
            return convertLinear(input, output, ratio);
    }
}

void SampleRateConverter::setQuality(Quality quality) {
    currentQuality = quality;
    
    if (quality == Quality::Sinc || quality == Quality::Best) {
        buildSincKernel(0.45);
    }
}

double SampleRateConverter::getLatency() const {
    switch (currentQuality) {
        case Quality::Linear:
            return 1.0 / 44100.0;  // 1 sample
        case Quality::Sinc:
        case Quality::Best:
            return kernelSize / 2.0 / 44100.0;  // Half kernel size
        default:
            return 0.0;
    }
}

bool SampleRateConverter::convertLinear(const juce::AudioBuffer<float>& input,
                                       juce::AudioBuffer<float>& output,
                                       double ratio) {
    
    int outputSamples = static_cast<int>(input.getNumSamples() * ratio);
    output.setSize(input.getNumChannels(), outputSamples);
    
    for (int ch = 0; ch < input.getNumChannels(); ++ch) {
        const float* inData = input.getReadPointer(ch);
        float* outData = output.getWritePointer(ch);
        
        for (int outSample = 0; outSample < outputSamples; ++outSample) {
            float inSample = outSample / ratio;
            int index = static_cast<int>(inSample);
            float fraction = inSample - index;
            
            if (index >= input.getNumSamples() - 1) {
                outData[outSample] = inData[input.getNumSamples() - 1];
            } else {
                outData[outSample] = inData[index] * (1.0f - fraction) + inData[index + 1] * fraction;
            }
        }
    }
    
    return true;
}

bool SampleRateConverter::convertSinc(const juce::AudioBuffer<float>& input,
                                      juce::AudioBuffer<float>& output,
                                      double ratio) {
    
    int outputSamples = static_cast<int>(input.getNumSamples() * ratio);
    output.setSize(input.getNumChannels(), outputSamples);
    
    for (int ch = 0; ch < input.getNumChannels(); ++ch) {
        const float* inData = input.getReadPointer(ch);
        float* outData = output.getWritePointer(ch);
        
        for (int outSample = 0; outSample < outputSamples; ++outSample) {
            float inSample = outSample / ratio;
            int centerIndex = static_cast<int>(inSample);
            
            float sum = 0.0f;
            float kernelSum = 0.0f;
            
            for (int i = 0; i < kernelSize; ++i) {
                int index = centerIndex - kernelSize / 2 + i;
                
                if (index >= 0 && index < input.getNumSamples()) {
                    float offset = inSample - index;
                    int kernelIndex = static_cast<int>((offset + kernelSize / 2) * sincKernel.size() / kernelSize);
                    
                    if (kernelIndex >= 0 && kernelIndex < static_cast<int>(sincKernel.size())) {
                        float kernelValue = sincKernel[kernelIndex];
                        sum += inData[index] * kernelValue;
                        kernelSum += kernelValue;
                    }
                }
            }
            
            outData[outSample] = kernelSum > 0.0f ? sum / kernelSum : 0.0f;
        }
    }
    
    return true;
}

void SampleRateConverter::buildSincKernel(double cutoff) {
    sincKernel.resize(kernelSize);
    
    for (int i = 0; i < kernelSize; ++i) {
        float t = (i - kernelSize / 2.0f) / kernelSize * 2.0f;
        
        if (t == 0.0f) {
            sincKernel[i] = 1.0f;
        } else {
            sincKernel[i] = std::sin(juce::MathConstants<float>::pi * cutoff * t) / (juce::MathConstants<float>::pi * cutoff * t);
        }
        
        // Apply window (Hamming)
        sincKernel[i] *= 0.54f - 0.46f * std::cos(2.0f * juce::MathConstants<float>::pi * i / (kernelSize - 1));
    }
}

// RealTimeAudioProcessor Implementation
RealTimeAudioProcessor::RealTimeAudioProcessor() {
    inputBuffer = std::make_unique<RealTimeAudioBuffer>(2, 512);
    outputBuffer = std::make_unique<RealTimeAudioBuffer>(2, 512);
    sampleRateConverter = std::make_unique<SampleRateConverter>();
}

RealTimeAudioProcessor::~RealTimeAudioProcessor() = default;

bool RealTimeAudioProcessor::initialize(int channels, double sampleRate, int bufferSize) {
    numChannels = channels;
    this->sampleRate = sampleRate;
    this->bufferSize = bufferSize;
    
    inputBuffer->setNumChannels(channels);
    inputBuffer->setBufferSize(bufferSize);
    outputBuffer->setNumChannels(channels);
    outputBuffer->setBufferSize(bufferSize);
    
    lastProcessTime = juce::Time::getCurrentTime();
    
    return true;
}

void RealTimeAudioProcessor::shutdown() {
    resetPerformanceCounters();
}

void RealTimeAudioProcessor::processAudio(juce::AudioBuffer<float>& buffer) {
    // Use the RT-safe template version with a no-op lambda
    processAudioWithCallback(buffer, [](juce::AudioBuffer<float>&) {
        // Default no-op processing
    });
}

// NOTE: processAudioWithCallback is now a template in RealTimeAudioBuffer.h
// This eliminates std::function allocation on the audio thread

bool RealTimeAudioProcessor::checkRealTimeSafety() const {
    // ⚠️ REAL-TIME THREAD ONLY
    // In a production environment, we should verify the thread ID 
    // against the one that was granted RT priority.
    
    // For now, we check the flag set during initialization/start
    if (!realTimePriority.load()) {
        return false;
    }
    
    return true;
}

void RealTimeAudioProcessor::updatePerformanceMetrics() {
    // Update performance counters
    lastProcessTime = juce::Time::getCurrentTime();
}

int RealTimeAudioProcessor::getLatencySamples() const {
    return static_cast<int>(targetLatency * sampleRate);
}

double RealTimeAudioProcessor::getLatencySeconds() const {
    return targetLatency;
}

void RealTimeAudioProcessor::setTargetLatency(double seconds) {
    targetLatency = juce::jlimit(0.001, 0.1, seconds);  // 1ms to 100ms
}

float RealTimeAudioProcessor::getCpuUsage() const {
    return cpuUsage.load();
}

int RealTimeAudioProcessor::getUnderruns() const {
    return underruns.load();
}

int RealTimeAudioProcessor::getOverruns() const {
    return overruns.load();
}

void RealTimeAudioProcessor::resetPerformanceCounters() {
    cpuUsage.store(0.0f);
    underruns.store(0);
    overruns.store(0);
}

bool RealTimeAudioProcessor::isRealTimeSafe() const {
    return realTimePriority.load();
}

void RealTimeAudioProcessor::setRealTimePriority(bool enabled) {
    realTimePriority.store(enabled);
    
    if (enabled) {
        // Linux implementation (JACK/ALSA threads usually handle this, but for internal workers:)
#if JUCE_LINUX
        struct sched_param param;
        param.sched_priority = 90; // High priority for audio
        int result = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
        if (result != 0) {
            // Priority set failed - LOG THE ERROR!
            juce::Logger::writeToLog("WARNING: Failed to set real-time priority (error " + 
                                   juce::String(result) + "). Run with sudo or set capabilities.");
            realTimePriority.store(false); // Don't lie about having RT priority
            return;
        }
        juce::Logger::writeToLog("Real-time priority (SCHED_FIFO) enabled successfully");
#endif
    }
}

} // namespace audio
} // namespace zenith
