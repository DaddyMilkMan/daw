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

//==============================================================================

#include "AudioRenderer.h"
#include "../MasterLimiter.h"
#include "../Track.h" // AudioRenderer uses Track methods; needs complete type
#include "../TempoMap.h"
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
    mixToMaster(renderContext_->trackBuffers, masterBuffer, numSamples);

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
    resetMasterMeters();
}

void AudioRenderer::processTracks(
    const std::vector<Track*>& tracks,
    const std::vector<AuxBus*>& auxBuses,
    const juce::MidiBuffer* incomingMidi,
    int numSamples,
    juce::int64 playheadPosition) {

    // Initialize track and aux buffers if needed
    if (!renderContext_) {
        renderContext_ = std::make_unique<AudioRenderContext>();
    }

    // Initialize track buffers
    if (renderContext_->trackBuffers.size() != tracks.size()) {
        renderContext_->trackBuffers.resize(tracks.size());

        for (size_t i = 0; i < tracks.size(); ++i) {
            auto* track = tracks[i];
            if (track) {
                // Tracks don't currently expose a channel-count API. Use the
                // renderer output channel count (stereo minimum) so buffers
                // match the master mixdown shape.
                int numChannels = std::max(2, numOutputChannels_);
                renderContext_->trackBuffers[i] = juce::AudioBuffer<float>(numChannels, numSamples);
                renderContext_->trackBuffers[i].clear();
            } else {
                renderContext_->trackBuffers[i] = juce::AudioBuffer<float>(2, numSamples);
                renderContext_->trackBuffers[i].clear();
            }
        }
    }

    // Initialize aux bus buffers
    if (renderContext_->auxBusBuffers.size() != auxBuses.size()) {
        renderContext_->auxBusBuffers.resize(auxBuses.size());

        for (size_t i = 0; i < auxBuses.size(); ++i) {
            auto* auxBus = auxBuses[i];
            if (auxBus) {
                int numChannels = 2; // Aux buses typically stereo
                renderContext_->auxBusBuffers[i] = juce::AudioBuffer<float>(numChannels, numSamples);
                renderContext_->auxBusBuffers[i].clear();
            } else {
                renderContext_->auxBusBuffers[i] = juce::AudioBuffer<float>(2, numSamples);
                renderContext_->auxBusBuffers[i].clear();
            }
        }
    }

    // Initialize PDC buffers if needed
    if (pdcEnabled_.load() && renderContext_->pdcDelayBuffers.size() != tracks.size()) {
        renderContext_->pdcDelayBuffers.resize(tracks.size());
        renderContext_->pdcDelayWritePos.resize(tracks.size(), 0);
        renderContext_->trackLatencies.resize(tracks.size(), 0);

        for (size_t i = 0; i < tracks.size(); ++i) {
            int maxLatency = 512; // Max latency samples
            renderContext_->pdcDelayBuffers[i] = juce::AudioBuffer<float>(2, maxLatency);
            renderContext_->pdcDelayBuffers[i].clear();
        }
    }

    // Process each track
    for (size_t i = 0; i < tracks.size(); ++i) {
        auto* track = tracks[i];
        if (!track || !track->isEnabled()) {
            continue;
        }

        auto& trackBuffer = renderContext_->trackBuffers[i];

        // Track-level processing via TrackProcessor
        if (track->getProcessor()) {
            // Create audio source channel info for the track buffer
            juce::AudioSourceChannelInfo bufferInfo(&trackBuffer, 0, numSamples);

            // Get empty MIDI buffer for track processing
            auto& emptyMidi = track->getProcessor()->getEmptyMidiBuffer();

            // Get aux buffers for this track
            std::vector<juce::AudioBuffer<float>*> auxBuffers;
            auxBuffers.reserve(auxBuses.size());

            for (auto* auxBus : auxBuses) {
                if (auxBus) {
                    int auxIndex = static_cast<int>(&auxBus - &auxBuses[0]);
                    if (auxIndex < renderContext_->auxBusBuffers.size()) {
                        auxBuffers.push_back(&renderContext_->auxBusBuffers[auxIndex]);
                    } else {
                        auxBuffers.push_back(nullptr);
                    }
                }
            }

            // Process track via TrackProcessor
            track->getNextAudioBlock(bufferInfo, playheadPosition, incomingMidi,
                                    auxBuffers, nullptr, nullptr);
        } else {
            // Fallback: process track directly
            juce::AudioSourceChannelInfo bufferInfo(&trackBuffer, 0, numSamples);
            track->getNextAudioBlock(bufferInfo, playheadPosition, incomingMidi, {}, nullptr, nullptr);
        }

        // Apply track solo/mute logic
        if (track->isMuted()) {
            trackBuffer.clear();
        } else if (track->isSoloed()) {
            // Solo logic: if this track is soloed, silence non-soloed tracks
            for (size_t j = 0; j < tracks.size(); ++j) {
                if (i != j && tracks[j] && !tracks[j]->isSoloed() && tracks[j]->isEnabled()) {
                    renderContext_->trackBuffers[j].clear();
                }
            }
        }

        // Apply PDC if enabled
        if (pdcEnabled_.load() && i < renderContext_->pdcDelayBuffers.size()) {
            applyPDC(trackBuffer, renderContext_->pdcDelayBuffers[i],
                     renderContext_->pdcDelayWritePos[i],
                     renderContext_->trackLatencies[i]);
        }
    }
}

void AudioRenderer::mixToMaster(
    const std::vector<juce::AudioBuffer<float>>& trackBuffers,
    juce::AudioBuffer<float>& masterBuffer,
    int numSamples) {

    // Clear master buffer
    masterBuffer.clear();

    // Mix all enabled tracks to master
    for (const auto& trackBuffer : trackBuffers) {
        if (trackBuffer.getNumSamples() > 0) {
            // Mix track buffer to master
            for (int channel = 0; channel < trackBuffer.getNumChannels() &&
                 channel < masterBuffer.getNumChannels(); ++channel) {
                const float* sourceChannel = trackBuffer.getReadPointer(channel);
                if (sourceChannel) {
                    for (int sample = 0; sample < numSamples && sample < masterBuffer.getNumSamples(); ++sample) {
                        float sourceSample = sourceChannel[sample];
                        float masterSample = masterBuffer.getSample(channel, sample);
                        masterBuffer.setSample(channel, sample, masterSample + sourceSample);
                    }
                }
            }
        }
    }

    // Apply master volume
    float volume = masterVolume_.load();
    masterBuffer.applyGain(volume);

    // Update metering
    updateMasterMetersInternal(masterBuffer, numSamples);
}

void AudioRenderer::applyMasterEffects(juce::AudioBuffer<float>& buffer) {
    // Create temporary buffer for plugin processing if needed
    juce::AudioBuffer<float> tempBuffer;

    // Process master plugins in order
    for (const auto& plugin : masterPlugins_) {
        if (plugin && !plugin->isSuspended()) {
            // Use buffer directly for processing
            juce::MidiBuffer emptyMidi;
            juce::AudioBuffer<float>* pluginBuffer = &buffer;

            // Process audio through plugin
            plugin->processBlock(*pluginBuffer, emptyMidi);
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
    renderContext_ = std::make_unique<AudioRenderContext>(context);
    DBG("AudioRenderer: Render context updated");
}

const AudioRenderContext& AudioRenderer::getRenderContext() const {
    return *renderContext_;
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

// Update master meters (not declared in header but needed)
void AudioRenderer::updateMasterMetersInternal(const juce::AudioBuffer<float>& buffer, int numSamples) {
    float level = 0.0f;
    float peak = 0.0f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        // Simple channel level calculation
        const float* channelData = buffer.getReadPointer(channel);
        float channelRMS = 0.0f;
        float channelPeak = 0.0f;

        for (int sample = 0; sample < numSamples; ++sample) {
            float sampleValue = std::abs(channelData[sample]);
            channelRMS += sampleValue * sampleValue;
            channelPeak = std::max(channelPeak, sampleValue);
        }

        channelRMS = std::sqrt(channelRMS / numSamples);
        level = std::max(level, channelRMS);
        peak = std::max(peak, channelPeak);
    }

    masterLevel_.store(level);
    masterPeakLevel_.store(std::max(masterPeakLevel_.load(), peak));
}

void AudioRenderer::updateTrackLatencies() {
    if (!renderContext_ || !pdcEnabled_.load()) {
        return;
    }

    // TODO: This renderer currently doesn't have access to the Track list when
    // recalculatePDC() is called (no stored snapshot). Until that wiring exists,
    // keep PDC latencies at 0 so the engine compiles and runs without relying
    // on undefined state.
    std::fill(renderContext_->trackLatencies.begin(), renderContext_->trackLatencies.end(), 0);

    // Calculate maximum latency
    int maxLatency = 0;
    for (int latency : renderContext_->trackLatencies) {
        maxLatency = std::max(maxLatency, latency);
    }

    renderContext_->maxTrackLatency = maxLatency;
}

void AudioRenderer::applyPDC(juce::AudioBuffer<float>& trackBuffer,
                           juce::AudioBuffer<float>& delayBuffer,
                           int& delayWritePos,
                           int trackLatency) {

    if (trackLatency <= 0 || delayBuffer.getNumSamples() < trackLatency) {
        return; // No PDC needed or buffer too small
    }

    int bufferSize = delayBuffer.getNumSamples();
    int numSamples = trackBuffer.getNumSamples();
    int numChannels = std::min(trackBuffer.getNumChannels(), delayBuffer.getNumChannels());

    // Read from delay buffer (delayed audio)
    for (int channel = 0; channel < numChannels; ++channel) {
        const float* delayReadPtr = delayBuffer.getReadPointer(channel);
        float* trackWritePtr = trackBuffer.getWritePointer(channel);

        if (delayReadPtr && trackWritePtr) {
            for (int sample = 0; sample < numSamples; ++sample) {
                int readPos = (delayWritePos + sample) % bufferSize;
                float delayedSample = delayReadPtr[readPos];
                trackWritePtr[sample] += delayedSample;
            }
        }
    }

    // Write to delay buffer (current audio for next iteration)
    for (int channel = 0; channel < numChannels; ++channel) {
        const float* trackReadPtr = trackBuffer.getReadPointer(channel);
        float* delayWritePtr = delayBuffer.getWritePointer(channel);

        if (trackReadPtr && delayWritePtr) {
            // Shift existing delay content
            for (int sample = bufferSize - 1; sample >= trackLatency; --sample) {
                delayWritePtr[sample] = delayWritePtr[sample - trackLatency];
            }

            // Write new content
            for (int sample = 0; sample < trackLatency && sample < numSamples; ++sample) {
                delayWritePtr[sample] = trackReadPtr[sample];
            }
        }
    }

    // Update write position
    delayWritePos = (delayWritePos + numSamples) % bufferSize;
}

} // namespace zenith
