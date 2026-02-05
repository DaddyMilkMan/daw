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

 * @file AudioRenderer.cpp
 * @brief Simplified audio rendering implementation
 */


#include <algorithm>

namespace zenith {

AudioRenderer::AudioRenderer() {
    DBG("AudioRenderer: Constructor");
}

AudioRenderer::~AudioRenderer() {
    DBG("AudioRenderer: Destructor");
    shutdown();
}

void AudioRenderer::initialize(int numInputChannels, int numOutputChannels, double sampleRate) {
    if (initialized_) {
        DBG("AudioRenderer: Already initialized");
        return;
    }

    DBG("AudioRenderer: Initializing with " + juce::String(numInputChannels) +
        " inputs, " + juce::String(numOutputChannels) + " outputs, " +
        juce::String(sampleRate) + " Hz");

    numInputChannels_ = numInputChannels;
    numOutputChannels_ = numOutputChannels;
    sampleRate_ = sampleRate;

    // Initialize master limiter
    masterLimiter_ = std::make_unique<MasterLimiter>();
    masterLimiter_->initialize(sampleRate);

    initialized_ = true;
    DBG("AudioRenderer: Initialization complete");
}

void AudioRenderer::shutdown() {
    if (!initialized_) {
        return;
    }

    DBG("AudioRenderer: Shutting down");

    masterPlugins_.clear();
    masterLimiter_.reset();
    initialized_ = false;

    DBG("AudioRenderer: Shutdown complete");
}

bool AudioRenderer::isInitialized() const {
    return initialized_;
}

void AudioRenderer::processAudioBlock(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    juce::int64 playheadPosition,
    const std::vector<Track*>& tracks,
    const std::vector<AuxBus*>& auxBuses,
    const juce::MidiBuffer* incomingMidi) {

    if (!initialized_ || masterMuted_.load()) {
        // Clear output buffer if muted
        for (int channel = 0; channel < numOutputChannels; ++channel) {
            if (outputChannelData[channel] != nullptr) {
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
            }
        }
        return;
    }

    // Create master buffer
    juce::AudioBuffer<float> masterBuffer(numOutputChannels, numSamples);
    masterBuffer.clear();

    // Process tracks
    processTracks(tracks, auxBuses, incomingMidi, numSamples, playheadPosition);

    // Mix to master
    mixToMaster(/* track buffers would be populated here */, masterBuffer, numSamples);

    // Apply master effects
    applyMasterEffects(masterBuffer);

    // Apply test tone if enabled
    if (testToneEnabled_.load()) {
        applyTestTone(masterBuffer, numSamples);
    }

    // Copy to output
    for (int channel = 0; channel < numOutputChannels; ++channel) {
        if (outputChannelData[channel] != nullptr && channel < masterBuffer.getNumSamples()) {
            juce::FloatVectorOperations::copy(
                outputChannelData[channel],
                masterBuffer.getReadPointer(channel),
                numSamples);
        }
    }

    // Update metering
    updateMasterMeters(masterBuffer, numSamples);
}

void AudioRenderer::processTracks(
    const std::vector<Track*>& tracks,
    const std::vector<AuxBus*>& auxBuses,
    const juce::MidiBuffer* incomingMidi,
    int numSamples,
    juce::int64 playheadPosition) {

    // TODO: Implement actual track processing
    // For now, just log
    DBG("AudioRenderer: Processing " + juce::String(tracks.size()) +
        " tracks, " + juce::String(auxBuses.size()) + " aux buses");
}

void AudioRenderer::mixToMaster(
    const std::vector<juce::AudioBuffer<float>>& trackBuffers,
    juce::AudioBuffer<float>& masterBuffer,
    int numSamples) {

    // TODO: Implement actual mixing
    // For now, just apply volume
    float volume = masterVolume_.load();
    masterBuffer.applyGain(volume);
}

void AudioRenderer::applyMasterEffects(juce::AudioBuffer<float>& buffer) {
    // Apply master plugins
    for (const auto& plugin : masterPlugins_) {
        if (plugin && !plugin->isSuspended()) {
            // TODO: Process through plugin
        }
    }

    // Apply master limiter
    if (masterLimiter_ && masterLimiter_->isEnabled()) {
        masterLimiter_->process(buffer);
    }
}

void AudioRenderer::applyTestTone(juce::AudioBuffer<float>& buffer, int numSamples) {
    // Simple 440Hz sine wave
    const double frequency = 440.0;
    const double amplitude = 0.1; // -20dB
    const double phaseIncrement = (2.0 * juce::MathConstants<double>::pi * frequency) / sampleRate_;

    for (int sample = 0; sample < numSamples; ++sample) {
        double sampleValue = amplitude * std::sin(testTonePhase_);
        testTonePhase_ += phaseIncrement;

        // Wrap phase
        if (testTonePhase_ > 2.0 * juce::MathConstants<double>::pi) {
            testTonePhase_ -= 2.0 * juce::MathConstants<double>::pi;
        }

        // Add to all channels
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            buffer.addSample(channel, sample, static_cast<float>(sampleValue));
        }
    }
}

void AudioRenderer::setRenderContext(const AudioRenderContext& context) {
    renderContext_ = context;
    DBG("AudioRenderer: Render context updated");
}

const AudioRenderContext& AudioRenderer::getRenderContext() const {
    return renderContext_;
}

void AudioRenderer::setMasterVolume(float volume) {
    masterVolume_.store(juce::jlimit(0.0f, 2.0f, volume));
}

float AudioRenderer::getMasterVolume() const {
    return masterVolume_.load();
}

void AudioRenderer::setMasterMute(bool muted) {
    masterMuted_.store(muted);
}

bool AudioRenderer::isMasterMuted() const {
    return masterMuted_.load();
}

void AudioRenderer::addMasterPlugin(std::shared_ptr<juce::AudioPluginInstance> plugin) {
    if (plugin) {
        masterPlugins_.push_back(plugin);
        DBG("AudioRenderer: Added master plugin");
    }
}

void AudioRenderer::removeMasterPlugin(int index) {
    if (index >= 0 && index < masterPlugins_.size()) {
        masterPlugins_.erase(masterPlugins_.begin() + index);
        DBG("AudioRenderer: Removed master plugin at index " + juce::String(index));
    }
}

int AudioRenderer::getNumMasterPlugins() const {
    return masterPlugins_.size();
}

std::shared_ptr<juce::AudioPluginInstance> AudioRenderer::getMasterPlugin(int index) const {
    return (index >= 0 && index < masterPlugins_.size()) ? masterPlugins_[index] : nullptr;
}

void AudioRenderer::setMasterLimiterEnabled(bool enabled) {
    if (masterLimiter_) {
        masterLimiter_->setEnabled(enabled);
    }
}

bool AudioRenderer::isMasterLimiterEnabled() const {
    return masterLimiter_ && masterLimiter_->isEnabled();
}

void AudioRenderer::setMasterLimiterCeiling(float ceilingDb) {
    if (masterLimiter_) {
        masterLimiter_->setCeiling(ceilingDb);
    }
}

float AudioRenderer::getMasterLimiterCeiling() const {
    return masterLimiter_ ? masterLimiter_->getCeiling() : -0.1f;
}

float AudioRenderer::getMasterLimiterGainReduction() const {
    return masterLimiter_ ? masterLimiter_->getGainReduction() : 0.0f;
}

int AudioRenderer::getMasterLimiterLatency() const {
    return masterLimiter_ ? masterLimiter_->getLatencySamples() : 0;
}

float AudioRenderer::getMasterLevel() const {
    return masterLevel_.load();
}

float AudioRenderer::getMasterPeakLevel() const {
    return masterPeakLevel_.load();
}

void AudioRenderer::resetPeakMeters() {
    resetMasterMeters();
}

void AudioRenderer::recalculatePDC() {
    updateTrackLatencies();
}

int AudioRenderer::getTrackLatency(int trackIndex) const {
    // TODO: Implement actual track latency calculation
    return 0;
}

int AudioRenderer::getMasterLatency() const {
    return getMasterLimiterLatency();
}

int AudioRenderer::getMaxTrackLatency() const {
    // TODO: Implement actual maximum track latency
    return 0;
}

bool AudioRenderer::isPDCEnabled() const {
    return pdcEnabled_.load();
}

void AudioRenderer::setPDCEnabled(bool enabled) {
    pdcEnabled_.store(enabled);
}

void AudioRenderer::setSampleRate(double sampleRate) {
    if (sampleRate > 0) {
        sampleRate_ = sampleRate;
        if (masterLimiter_) {
            masterLimiter_->initialize(sampleRate);
        }
    }
}

void AudioRenderer::setBufferSize(int bufferSize) {
    bufferSize_ = bufferSize;
}

void AudioRenderer::enableTestTone(bool enabled) {
    testToneEnabled_.store(enabled);
    if (!enabled) {
        testTonePhase_ = 0.0;
    }
}

bool AudioRenderer::isTestToneEnabled() const {
    return testToneEnabled_.load();
}

void AudioRenderer::resetMasterMeters() {
    masterLevel_.store(0.0f);
    masterPeakLevel_.store(0.0f);
}

void AudioRenderer::updateMasterMeters(const juce::AudioBuffer<float>& buffer, int numSamples) {
    float level = 0.0f;
    float peak = 0.0f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        channelMaxLevel(buffer.getReadPointer(channel), numSamples, level, peak);
    }

    masterLevel_.store(level);
    masterPeakLevel_.store(std::max(masterPeakLevel_.load(), peak));
}

void AudioRenderer::updateTrackLatencies() {
    // TODO: Implement actual track latency updates
}

} // namespace zenith