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

#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <span>
#include <vector>

#include "../dsp/Dither.h"
#include "EngineConstants.h"
#include "RoutingGraph.h"

namespace zenith {

// Forward declarations
class Track;
class AuxBus;
class MasterLimiter;
class TempoMap;

//==============================================================================
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
  std::array<juce::AudioBuffer<float>*, kMaxTrackBuses> auxBufferPtrsArray;
  int numActiveAuxPtrs = 0;

  // Configuration
  double sampleRate = constants::kDefaultSampleRate;
  int blockSize = constants::kDefaultBufferSize;

  // Helper to resize all buffers
  void prepare(double newSampleRate, int newBlockSize, size_t numTracks,
               size_t numAuxBuses) {
    sampleRate = newSampleRate;
    blockSize = newBlockSize;

    // Resize track buffers
    if (trackBuffers.size() != numTracks) {
      trackBuffers.resize(numTracks);
    }
    for (auto &buffer : trackBuffers) {
      buffer.setSize(2, blockSize);
      buffer.clear();
    }

    // Resize aux buffers
    if (auxBusBuffers.size() != numAuxBuses) {
      auxBusBuffers.resize(numAuxBuses);
    }
    for (auto &buffer : auxBusBuffers) {
      buffer.setSize(2, blockSize);
      buffer.clear();
    }

    // Resize PDC buffers
    if (pdcDelayBuffers.size() != numTracks) {
      pdcDelayBuffers.resize(numTracks);
      pdcDelayWritePos.resize(numTracks, 0);
      trackLatencies.resize(numTracks, 0);
    }
    for (auto &buffer : pdcDelayBuffers) {
      buffer.setSize(2, constants::kMaxPDCLatencySamples);
      // Don't necessarily clear delay buffers, they hold state across blocks!
      // But if resizing/initializing, we might want to.
    }

    auxBufferPtrsArray.fill(nullptr); 
  }

  void reset() {
    for (auto &buffer : trackBuffers) buffer.clear();
    for (auto &buffer : auxBusBuffers) buffer.clear();
    for (auto &buffer : pdcDelayBuffers) buffer.clear();
    std::fill(pdcDelayWritePos.begin(), pdcDelayWritePos.end(), 0);
    std::fill(trackLatencies.begin(), trackLatencies.end(), 0);
    maxTrackLatency = 0;
  }
};

//==============================================================================
/**
    Audio graph renderer for the engine.

    Handles the core audio rendering pipeline:
    - Track rendering with plugin processing
    - Aux bus processing and mixing
    - Master bus processing with limiter
    - PDC (plugin delay compensation)
    
    Now STATELESS: Requires an AudioRenderContext to operate.
*/
class AudioRenderer {
public:
  //==========================================================================
  AudioRenderer();
  ~AudioRenderer() = default;

  //==========================================================================
  // Rendering
  //==========================================================================

  /**
   * @brief Render the audio graph to output buffer
   * @param context The render context (buffers, PDC state) to use
   * @param outputBuffer Output buffer to fill
   * @param numSamples Number of samples to render
   * @param playheadPosition Current playhead position in samples
   * @param tracks Vector of tracks to render
   * @param auxBuses Vector of aux buses
   * @param routingGraph Routing graph for signal flow
   * @param masterLimiter Master bus limiter
   * @param masterPlugins Master bus plugin chain
   * @param tempoMap Tempo map for automation
   * @param incomingMidi Optional incoming MIDI buffer
   * @param inputChannelData Optional input channel data
   * @param numInputChannels Number of input channels
   * @note AUDIO THREAD ONLY
   */
  void renderAudioGraph(
      AudioRenderContext& context,
      juce::AudioBuffer<float> &outputBuffer, int numSamples,
      juce::int64 playheadPosition, std::span<Track *const> tracks,
      std::span<AuxBus *const> auxBuses, const RoutingGraph &routingGraph,
      MasterLimiter &masterLimiter,
      std::span<const std::shared_ptr<juce::AudioPluginInstance>> masterPlugins,
      const TempoMap *tempoMap, const juce::MidiBuffer *incomingMidi = nullptr,
      const float *const *inputChannelData = nullptr,
      int numInputChannels = 0) noexcept;

  /**
   * @brief Update playhead position for all clips in all tracks
   * @param tracks List of tracks to synchronize
   * @param playheadPosition Current position in samples
   */
  void updateClipPositions(std::span<Track *const> tracks,
                           juce::int64 playheadPosition) noexcept;

  //==========================================================================
  // PDC (Plugin Delay Compensation)
  //==========================================================================

  /**
   * @brief Calculate and update PDC for all tracks in the context
   * @param context Render context
   * @param tracks Vector of tracks
   * @return Maximum latency in samples
   */
  int calculatePDC(AudioRenderContext& context, std::span<Track *const> tracks);

  /**
   * @brief Enable/disable PDC
   */
  void setPDCEnabled(bool enabled) { pdcEnabled_.store(enabled); }
  bool isPDCEnabled() const { return pdcEnabled_.load(); }

  //==========================================================================
  // Metering
  //==========================================================================

  /**
   * @brief Get current master output level
   */
  float getMasterLevel() const { return masterLevel_.load(); }

  /**
   * @brief Get peak master output level
   */
  float getMasterPeakLevel() const { return masterPeakLevel_.load(); }

  /**
   * @brief Reset peak meters
   */
  void resetPeakMeters() { masterPeakLevel_.store(0.0f); }

  //==========================================================================
  // Latency Query
  //==========================================================================

  /**
   * @brief Get master bus latency in samples
   */
  int getMasterLatency() const;

  /**
   * @brief Update cached master latency value
   * @param masterPlugins List of master plugins
   * @param limiterLatency Latency of the master limiter
   */
  void updateMasterLatency(
      std::span<const std::shared_ptr<juce::AudioPluginInstance>> masterPlugins,
      int limiterLatency);

  static constexpr int kMaxAuxBuses = 32;

private:
  //==========================================================================
  // Internal Methods
  //==========================================================================

  /**
   * @brief Apply PDC delay to a buffer
   */
  void applyPDCDelay(AudioRenderContext& context, juce::AudioBuffer<float> &buffer, int trackIndex,
                     int numSamples);

  /**
   * @brief Process master bus plugins
   */
  void processMasterPlugins(
      juce::AudioBuffer<float> &buffer,
      std::span<const std::shared_ptr<juce::AudioPluginInstance>> plugins);

  /**
   * @brief Update output metering
   */
  void updateMasterMeters(const juce::AudioBuffer<float> &buffer);

  //==========================================================================
  // State
  //==========================================================================

  // PDC enabled state (global setting)
  std::atomic<bool> pdcEnabled_{false};

  // Metering (atomic for lock-free GUI access)
  std::atomic<float> masterLevel_{0.0f};
  std::atomic<float> masterPeakLevel_{0.0f};
  std::atomic<int> masterLatency_{0};

  // Dither
  zenith::dsp::Dither dither_;
  juce::MidiBuffer scratchMidi_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRenderer)
};

} // namespace zenith
