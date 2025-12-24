/*
  ==============================================================================

    AudioRenderer.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Audio graph rendering implementation.

  ==============================================================================
*/

#include "AudioRenderer.h"
#include "../dsp/MasterLimiter.h"
#include "../dsp/SIMDHelpers.h"
#include "AuxBus.h"
#include "Clip.h"
#include "TempoMap.h"
#include "Track.h"

namespace zenith {

//==============================================================================
void AudioRenderer::prepare(double sampleRate, int blockSize, size_t numTracks,
                            size_t numAuxBuses) {
  sampleRate_ = sampleRate;
  blockSize_ = blockSize;

  // Allocate track buffers
  trackBuffers_.clear();
  trackBuffers_.resize(numTracks);
  for (auto &buffer : trackBuffers_) {
    buffer.setSize(2, blockSize);
    buffer.clear();
  }

  // Allocate aux bus buffers
  auxBusBuffers_.clear();
  auxBusBuffers_.resize(numAuxBuses);
  for (auto &buffer : auxBusBuffers_) {
    buffer.setSize(2, blockSize);
    buffer.clear();
  }

  // Allocate PDC buffers
  trackLatencies_.resize(numTracks, 0);
  pdcDelayBuffers_.resize(numTracks);
  pdcDelayWritePos_.resize(numTracks, 0);

  for (size_t i = 0; i < numTracks; ++i) {
    pdcDelayBuffers_[i].setSize(2, constants::kMaxPDCLatencySamples);
    pdcDelayBuffers_[i].clear();
  }

  // Prepare dither
  dither_.prepare(2); // Stereo

  // [DSP Optimization] Pre-reserve vector capacity for RT-safety
  auxBufferPtrsVector_.reserve(kMaxAuxBuses);

  DBG("AudioRenderer: Prepared with " + juce::String(numTracks) + " tracks, " +
      juce::String(numAuxBuses) + " aux buses");
}

//==============================================================================
void AudioRenderer::reset() {
  for (auto &buffer : trackBuffers_) {
    buffer.clear();
  }
  for (auto &buffer : auxBusBuffers_) {
    buffer.clear();
  }
  for (auto &buffer : pdcDelayBuffers_) {
    buffer.clear();
  }
  std::fill(pdcDelayWritePos_.begin(), pdcDelayWritePos_.end(), 0);

  masterLevel_.store(0.0f);
  masterPeakLevel_.store(0.0f);
}

//==============================================================================
void AudioRenderer::renderAudioGraph(
    juce::AudioBuffer<float> &outputBuffer, int numSamples,
    juce::int64 playheadPosition, std::span<Track *const> tracks,
    std::span<AuxBus *const> auxBuses, const RoutingGraph &routingGraph,
    MasterLimiter &masterLimiter,
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> &masterPlugins,
    const TempoMap *tempoMap, const juce::MidiBuffer *incomingMidi,
    const float *const *inputChannelData, int numInputChannels) noexcept {

  // RT-Safety: Disable denormals to prevent CPU spikes with near-zero floats
  juce::ScopedNoDenormals noDenormals;

  // Clear output buffer
  outputBuffer.clear();

  // Get routing snapshot (lock-free)
  const auto *snapshot = routingGraph.getSnapshot();
  if (snapshot == nullptr || snapshot->topology == nullptr ||
      snapshot->topology->processingOrder.empty()) {
    return;
  }

  // Pre-clear aux bus buffers
  const size_t numBuses = juce::jmin(auxBuses.size(), auxBusBuffers_.size());
  for (size_t i = 0; i < numBuses; ++i) {
    auxBusBuffers_[i].clear();
  }

  // Build aux buffer pointers for tracks (RT-safe stack allocation or fixed
  // member) We'll use a local array for safety since it's small (max 16 aux
  // buses usually)
  std::array<juce::AudioBuffer<float> *, kMaxAuxBuses> auxBufferPtrs;
  size_t actualAuxCount = 0;
  for (size_t i = 0; i < numBuses && actualAuxCount < kMaxAuxBuses; ++i) {
    auxBufferPtrs[actualAuxCount++] = &auxBusBuffers_[i];
  }

  // Wrap in a vector-like view for getNextAudioBlock compatibility
  // Note: Track::getNextAudioBlock takes std::vector<juce::AudioBuffer<float>
  // *>. This is a violation of RT-safety if we create the vector here, but if
  // we pass a pre-allocated one, it's fine. However, the signature expects
  // std::vector. We'll have to use a member variable vector to avoid
  // allocation.
  auxBufferPtrsVector_.clear();
  for (size_t i = 0; i < actualAuxCount; ++i)
    auxBufferPtrsVector_.push_back(auxBufferPtrs[i]);

  // Process nodes in topological order using FAST LOOKUP
  for (const auto &nodeId : snapshot->topology->processingOrder) {
    // 1. Try to find a Track using fast lookup
    auto trackIt = snapshot->trackLookup.find(nodeId);
    if (trackIt != snapshot->trackLookup.end() && trackIt->second != nullptr) {
      Track *track = trackIt->second;
      int trackIdx = track->getTrackIndex();

      // Handle frozen tracks - play back their freeze buffer (RT-safe)
      if (track->isFrozen()) {
        auto freezeBuffer = track->getFreezeBuffer();
        if (freezeBuffer != nullptr && trackIdx >= 0 &&
            trackIdx < (int)trackBuffers_.size()) {
          auto &trackBuffer = trackBuffers_[trackIdx];
          trackBuffer.clear();

          const juce::int64 readPos = playheadPosition;
          const int bufferLength = freezeBuffer->getNumSamples();

          if (readPos >= 0 && readPos < bufferLength) {
            const int samplesToRead = juce::jmin(
                numSamples, static_cast<int>(bufferLength - readPos));
            if (samplesToRead > 0) {
              for (int ch = 0; ch < juce::jmin(trackBuffer.getNumChannels(),
                                               freezeBuffer->getNumChannels());
                   ++ch) {
                trackBuffer.copyFrom(ch, 0, *freezeBuffer, ch, (int)readPos,
                                     samplesToRead);
              }
            }
          }

          track->applyGainAndPan(trackBuffer, numSamples);

          // Mix frozen track using SIMD if possible
          for (const auto &conn : snapshot->topology->connections) {
            if (conn.sourceId == nodeId && conn.destId == "master") {
              if (conn.gain != 1.0f)
                trackBuffer.applyGain(conn.gain);

              for (int ch = 0; ch < juce::jmin(outputBuffer.getNumChannels(),
                                               trackBuffer.getNumChannels());
                   ++ch) {
                juce::FloatVectorOperations::add(
                    outputBuffer.getWritePointer(ch),
                    trackBuffer.getReadPointer(ch), numSamples);
              }
              break;
            }
          }
        }
        continue;
      }

      if (trackIdx < 0 || trackIdx >= (int)trackBuffers_.size())
        continue;

      auto &trackBuffer = trackBuffers_[trackIdx];
      trackBuffer.clear();
      juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);

      const juce::MidiBuffer *trackMidiInput =
          (incomingMidi != nullptr && !incomingMidi->isEmpty() &&
           track->getType() == Track::Type::Instrument && track->isArmed())
              ? incomingMidi
              : nullptr;

      track->getNextAudioBlock(trackInfo, playheadPosition, trackMidiInput,
                               auxBufferPtrsVector_, tempoMap);
<<<<<<< HEAD

      // Input Monitoring Logic
      if (inputChannelData != nullptr && track->isInputMonitorEnabled()) {
        const int inputChIndex = track->getInputChannel();
        // Assuming stereo tracks: Map Input N -> Left, Input N+1 -> Right
        // If mono input selected for stereo track, map Input N to both.
        // For simplicity: Map Input N to Left, Input N+1 to Right if available.

        for (int ch = 0; ch < trackBuffer.getNumChannels(); ++ch) {
          const int sourceCh = inputChIndex + ch;
          if (sourceCh < numInputChannels &&
              inputChannelData[sourceCh] != nullptr) {
            // Add input signal (mix with existing clip audio)
            trackBuffer.addFrom(ch, 0, inputChannelData[sourceCh], numSamples);
          } else if (ch > 0 && inputChIndex < numInputChannels &&
                     inputChannelData[inputChIndex] != nullptr) {
            // Fallback: If Right input missing but Left exists, map Left to
            // Right (Mono -> Stereo) Simple heuristic for now.
            trackBuffer.addFrom(ch, 0, inputChannelData[inputChIndex],
                                numSamples);
          }
        }
      }
=======
>>>>>>> origin/feat/effects-suite

      if (pdcEnabled_.load()) {
        applyPDCDelay(trackBuffer, static_cast<int>(trackIdx), numSamples);
      }

      for (const auto &conn : snapshot->topology->connections) {
        if (conn.sourceId == nodeId && conn.destId == "master") {
          for (int ch = 0; ch < juce::jmin(outputBuffer.getNumChannels(),
                                           trackBuffer.getNumChannels());
               ++ch) {
            outputBuffer.addFrom(ch, 0, trackBuffer.getReadPointer(ch),
                                 numSamples, conn.gain);
          }
          break;
        }
      }
      continue;
    }

    // 2. Try to find an Aux Bus using fast lookup
    auto busIt = snapshot->auxBusLookup.find(nodeId);
    if (busIt != snapshot->auxBusLookup.end() && busIt->second != nullptr) {
      AuxBus *bus = busIt->second;
      // We need the index of the bus for the buffer
      // For now, let's look it up in the auxBuses span for the index
      // (Optimization: AuxBus could also store its index)
      size_t busIdx = 0;
      bool found = false;
      for (size_t i = 0; i < auxBuses.size(); ++i) {
        if (auxBuses[i] == bus) {
          busIdx = i;
          found = true;
          break;
        }
      }

      if (found && busIdx < auxBusBuffers_.size()) {
        auto &busBuffer = auxBusBuffers_[busIdx];
        juce::AudioSourceChannelInfo auxInfo(&busBuffer, 0, numSamples);
        bus->getNextAudioBlock(auxInfo);

        for (const auto &conn : snapshot->topology->connections) {
          if (conn.sourceId == nodeId && conn.destId == "master") {
            for (int ch = 0; ch < juce::jmin(outputBuffer.getNumChannels(),
                                             busBuffer.getNumChannels());
                 ++ch) {
              outputBuffer.addFrom(ch, 0, busBuffer, ch, 0, numSamples,
                                   conn.gain);
            }
            break;
          }
        }
      }
      continue;
    }
  }

  // Process master bus plugins
  processMasterPlugins(outputBuffer, masterPlugins);

  // Apply master limiter (final clipping protection)
  masterLimiter.process(outputBuffer);

  // Apply TPDF Dither (always on for 24-bit/16-bit DACs, or internal float
  // dither) Even if float, TPDF helps prevents truncation quantization noise if
  // converted later. Standard practice for DAWs to dither the final monitoring
  // output.
  dither_.process(outputBuffer, 24); // Assume 24-bit DAC monitoring

  // Update metering
  updateMasterMeters(outputBuffer);
}

//==============================================================================
int AudioRenderer::calculatePDC(std::span<Track *const> tracks) {
  int maxLatency = 0;

  for (size_t i = 0; i < tracks.size() && i < trackLatencies_.size(); ++i) {
    if (tracks[i]) {
      int trackLatency = 0;

      // Sum latency from all plugins
      for (int p = 0; p < tracks[i]->getNumPlugins(); ++p) {
        auto *plugin = tracks[i]->getPlugin(p);
        if (plugin != nullptr) {
          trackLatency += plugin->getLatencySamples();
        }
      }

      trackLatencies_[i] = trackLatency;
      maxLatency = juce::jmax(maxLatency, trackLatency);
    }
  }

  maxTrackLatency_.store(maxLatency);
  return maxLatency;
}

//==============================================================================
void AudioRenderer::applyPDCDelay(juce::AudioBuffer<float> &buffer,
                                  int trackIndex, int numSamples) {
  if (trackIndex < 0 ||
      trackIndex >= static_cast<int>(trackLatencies_.size())) {
    return;
  }

  const int trackLatency = trackLatencies_[trackIndex];
  const int maxLatency = maxTrackLatency_.load();
  const int delayNeeded = maxLatency - trackLatency;

  // No delay needed if track already has max latency, or delay exceeds buffer
  // capacity Note: Use > (not >=) because delay buffer can handle up to
  // kMaxPDCLatencySamples-1
  if (delayNeeded <= 0 || delayNeeded > constants::kMaxPDCLatencySamples - 1) {
    return;
  }

  auto &delayBuffer = pdcDelayBuffers_[trackIndex];
  int &writePos = pdcDelayWritePos_[trackIndex];

  // Cache channel pointers and counts for real-time performance
  auto *const *channelData = buffer.getArrayOfWritePointers();
  const int numBufferChannels = buffer.getNumChannels();
  const int numDelayBufferChannels = delayBuffer.getNumChannels();

  // Calculate initial read position
  const int initialReadPos =
      (writePos - delayNeeded + constants::kMaxPDCLatencySamples) %
      constants::kMaxPDCLatencySamples;

  // Process channel-by-channel for better cache locality
  // JUCE's AudioBuffer stores each channel's data in a contiguous memory block
  for (int ch = 0; ch < numBufferChannels && ch < numDelayBufferChannels;
       ++ch) {
    float *channelPtr = channelData[ch];
    float *delayChannelPtr = delayBuffer.getWritePointer(ch);

    int readPos = initialReadPos;
    int localWritePos = writePos;

    for (int i = 0; i < numSamples; ++i) {
      // Read the delayed sample from the circular buffer
      const float delayedSample = delayChannelPtr[readPos];

      // Store the incoming sample into the circular buffer
      delayChannelPtr[localWritePos] = channelPtr[i];

      // Replace the current sample with the delayed one
      channelPtr[i] = delayedSample;

      // Advance positions
      readPos = (readPos + 1) % constants::kMaxPDCLatencySamples;
      localWritePos = (localWritePos + 1) % constants::kMaxPDCLatencySamples;
    }
  }

  // Increment write position by numSamples after processing all channels
  writePos = (writePos + numSamples) % constants::kMaxPDCLatencySamples;
}

//==============================================================================
void AudioRenderer::processMasterPlugins(
    juce::AudioBuffer<float> &buffer,
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> &plugins) {

  if (plugins.empty()) {
    return;
  }

  juce::MidiBuffer midi; // Master bus doesn't handle MIDI

  for (auto &plugin : plugins) {
    if (plugin != nullptr && !plugin->isSuspended()) {
      plugin->processBlock(buffer, midi);
    }
  }
}

//==============================================================================
void AudioRenderer::updateMasterMeters(const juce::AudioBuffer<float> &buffer) {
  // Find peak level using SIMD helper
  float peak = simd::findPeak(buffer);

  // Smooth the level for display
  float currentLevel = masterLevel_.load();
  currentLevel = currentLevel * constants::kMeterSmoothingFactor +
                 peak * (1.0f - constants::kMeterSmoothingFactor);
  masterLevel_.store(currentLevel);

  // Update peak hold
  float currentPeak = masterPeakLevel_.load();
  if (peak > currentPeak) {
    masterPeakLevel_.store(peak);
  } else {
    // Decay peak
    masterPeakLevel_.store(currentPeak * constants::kPeakMeterDecay);
  }
}

int AudioRenderer::getTrackLatency(int trackIndex) const {
  if (trackIndex >= 0 &&
      trackIndex < static_cast<int>(trackLatencies_.size())) {
    return trackLatencies_[trackIndex];
  }
  return 0;
}

int AudioRenderer::getMasterLatency() const { return masterLatency_.load(); }

void AudioRenderer::updateMasterLatency(
    const std::vector<std::unique_ptr<juce::AudioPluginInstance>>
        &masterPlugins,
    int limiterLatency) {
  int totalLatency = limiterLatency;

  for (const auto &plugin : masterPlugins) {
    if (plugin != nullptr) {
      totalLatency += plugin->getLatencySamples();
    }
  }

  masterLatency_.store(totalLatency);
}

void AudioRenderer::updateClipPositions(std::span<Track *const> tracks,
                                        juce::int64 playheadPosition) noexcept {
  for (auto *track : tracks) {
    if (track != nullptr) {
      // Update clip scheduling/positions based on playhead
      // This ensures clips are ready for processing in the render callback
      track->updateClipPositions(playheadPosition);
    }
  }
}

} // namespace zenith
