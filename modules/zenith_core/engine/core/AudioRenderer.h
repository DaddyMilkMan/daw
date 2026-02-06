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

#pragma once

//==============================================================================

#include "IAudioRenderer.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <vector>

namespace zenith {
class TempoMap;
}

namespace zenith {

class MasterLimiter;

/**
 * @struct AudioRenderContext
 * @brief Holds all mutable buffers and state required for a single render pass.
 *
 * This allows AudioRenderer to be stateless and re-entrant (different contexts
 * for Live Engine vs Offline Export).
 */
struct AudioRenderContext {
    // Buffers
    std::vector<juce::AudioBuffer<float>> trackBuffers;
    std::vector<juce::AudioBuffer<float>> auxBusBuffers;

    // PDC State
    std::vector<juce::AudioBuffer<float>> pdcDelayBuffers;
    std::vector<int> pdcDelayWritePos;
    std::vector<int> trackLatencies;
    int maxTrackLatency = 0;

    // Optimizations
    // Pre-allocated array for aux buffers to avoid RT allocations
    static constexpr int kMaxTrackBuses = 128;
};

class AudioRenderer : public IAudioRenderer {
public:
    AudioRenderer();
    ~AudioRenderer() override;

    //==========================================================================
    // IAudioRenderer Implementation
    //==========================================================================

    void initialize(int numInputChannels, int numOutputChannels, double sampleRate) override;
    void shutdown() override;
    bool isInitialized() const override;

    void processAudioBlock(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        juce::int64 playheadPosition,
        const std::vector<Track*>& tracks,
        const std::vector<AuxBus*>& auxBuses,
        const juce::MidiBuffer* incomingMidi = nullptr) override;

    void setRenderContext(const AudioRenderContext& context) override;
    const AudioRenderContext& getRenderContext() const override;

    void setMasterVolume(float volume) override;
    float getMasterVolume() const override;
    void setMasterMute(bool muted) override;
    bool isMasterMuted() const override;

    void addMasterPlugin(std::shared_ptr<juce::AudioPluginInstance> plugin) override;
    void removeMasterPlugin(int index) override;
    int getNumMasterPlugins() const override;
    std::shared_ptr<juce::AudioPluginInstance> getMasterPlugin(int index) const override;

    void setMasterLimiterEnabled(bool enabled) override;
    bool isMasterLimiterEnabled() const override;
    void setMasterLimiterCeiling(float ceilingDb) override;
    float getMasterLimiterCeiling() const override;
    float getMasterLimiterGainReduction() const override;
    int getMasterLimiterLatency() const override;

    float getMasterLevel() const override;
    float getMasterPeakLevel() const override;
    void resetPeakMeters() override;

    void recalculatePDC() override;
    int getTrackLatency(int trackIndex) const override;
    int getMasterLatency() const override;
    int getMaxTrackLatency() const override;
    bool isPDCEnabled() const override;
    void setPDCEnabled(bool enabled) override;

    void setSampleRate(double sampleRate) override;
    double getSampleRate() const { return sampleRate_; }
    void setBufferSize(int bufferSize) override;
    void enableTestTone(bool enabled) override;
    bool isTestToneEnabled() const override;

private:
    //==========================================================================
    // Audio Processing
    //==========================================================================

    void processTracks(
        const std::vector<Track*>& tracks,
        const std::vector<AuxBus*>& auxBuses,
        const juce::MidiBuffer* incomingMidi,
        int numSamples,
        juce::int64 playheadPosition);

    void mixToMaster(
        const std::vector<juce::AudioBuffer<float>>& trackBuffers,
        juce::AudioBuffer<float>& masterBuffer,
        int numSamples);

    void applyMasterEffects(juce::AudioBuffer<float>& buffer);
    void applyTestTone(juce::AudioBuffer<float>& buffer, int numSamples);

    //==========================================================================
    // State
    //==========================================================================

    bool initialized_{false};
    std::atomic<float> masterVolume_{1.0f};
    std::atomic<bool> masterMuted_{false};
    std::atomic<bool> testToneEnabled_{false};
    std::atomic<bool> pdcEnabled_{false};

    //==========================================================================
    // Audio Data
    //==========================================================================

    int numInputChannels_{0};
    int numOutputChannels_{0};
    double sampleRate_{44100.0};
    int bufferSize_{512};

    std::unique_ptr<AudioRenderContext> renderContext_;
    std::vector<std::shared_ptr<juce::AudioPluginInstance>> masterPlugins_;
    std::unique_ptr<MasterLimiter> masterLimiter_;

    //==========================================================================
    // Metering
    //==========================================================================

    mutable std::atomic<float> masterLevel_{0.0f};
    mutable std::atomic<float> masterPeakLevel_{0.0f};

    //==========================================================================
    // Test Tone
    //==========================================================================

    double testTonePhase_{0.0};

    //==========================================================================
    // Helpers
    //==========================================================================

    void resetMasterMeters();
    void updateTrackLatencies();

    // Private helper functions
    void updateMasterMetersInternal(const juce::AudioBuffer<float>& buffer, int numSamples);
    void applyPDC(juce::AudioBuffer<float>& trackBuffer,
                 juce::AudioBuffer<float>& delayBuffer,
                 int& delayWritePos,
                 int trackLatency);
};

} // namespace zenith