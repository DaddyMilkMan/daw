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
    juce::int64 playheadPosition,
    const std::vector<std::shared_ptr<Track>> &tracks,
    const std::vector<std::shared_ptr<AuxBus>> &auxBuses,
    const RoutingGraph &routingGraph, MasterLimiter &masterLimiter,
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> &masterPlugins,
    const TempoMap *tempoMap, const juce::MidiBuffer *incomingMidi) {

  // Clear output buffer
  outputBuffer.clear();

  // Get routing snapshot (lock-free)
  const auto *snapshot = routingGraph.getSnapshot();
  if (snapshot == nullptr || snapshot->processingOrder.empty()) {
    return;
  }

  // Pre-clear aux bus buffers
  for (size_t i = 0; i < auxBuses.size() && i < auxBusBuffers_.size(); ++i) {
    auxBusBuffers_[i].clear();
  }

  // Build aux buffer pointers for tracks
  std::vector<juce::AudioBuffer<float> *> auxBufferPtrs;
  for (size_t i = 0; i < auxBusBuffers_.size(); ++i) {
    auxBufferPtrs.push_back(&auxBusBuffers_[i]);
  }

  // Process nodes in topological order
  for (const auto &nodeId : snapshot->processingOrder) {
    // 1. Try to find a Track
    Track *track = nullptr;
    size_t trackIdx = 0;

    for (size_t i = 0; i < tracks.size(); ++i) {
      if (tracks[i] && tracks[i]->getTrackId() == nodeId) {
        track = tracks[i].get();
        trackIdx = i;
        break;
      }
    }

    if (track != nullptr) {
      // Handle frozen tracks - play back their freeze file instead of
      // processing
      if (track->isFrozen()) {
        const juce::File &freezeFile = track->getFreezeFile();
        if (freezeFile.existsAsFile() && trackIdx < trackBuffers_.size()) {
          auto &trackBuffer = trackBuffers_[trackIdx];
          trackBuffer.clear();

          // Read from the freeze file at the current playhead position
          auto *freezeReader = track->getFreezeReader();
          if (freezeReader != nullptr) {
            // Calculate read position in freeze file
            const juce::int64 readPos = playheadPosition;
            const int samplesToRead = juce::jmin(
                numSamples,
                static_cast<int>(freezeReader->lengthInSamples - readPos));

            if (samplesToRead > 0 && readPos >= 0 &&
                readPos < freezeReader->lengthInSamples) {
              freezeReader->read(&trackBuffer, 0, samplesToRead, readPos, true,
                                 true);
            }
          }

          // Apply track volume/pan (frozen tracks still allow fader/pan)
          track->applyGainAndPan(trackBuffer, numSamples);

          // Mix frozen track to output based on graph connections
          for (const auto &conn : snapshot->connections) {
            if (conn.sourceId == nodeId && conn.destId == "master") {
              trackBuffer.applyGain(conn.gain);
              for (int channel = 0;
                   channel < juce::jmin(outputBuffer.getNumChannels(),
                                        trackBuffer.getNumChannels());
                   ++channel) {
                outputBuffer.addFrom(channel, 0,
                                     trackBuffer.getReadPointer(channel),
                                     numSamples);
              }
              break; // Assume single connection to master for now
            }
          }
        }
        continue; // Skip normal processing for frozen tracks
      }

      if (trackIdx >= trackBuffers_.size())
        continue;

      auto &trackBuffer = trackBuffers_[trackIdx];

      // Safety check
      if (trackBuffer.getNumSamples() < numSamples) {
        continue;
      }

      trackBuffer.clear();
      juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);

      // Route MIDI to armed instrument tracks
      const juce::MidiBuffer *trackMidiInput = nullptr;
      if (incomingMidi != nullptr && !incomingMidi->isEmpty() &&
          track->getType() == Track::Type::Instrument && track->isArmed()) {
        trackMidiInput = incomingMidi;
      }

      // Render track
      track->getNextAudioBlock(trackInfo, playheadPosition, trackMidiInput,
                               auxBufferPtrs, tempoMap);

      // Apply PDC if enabled
      if (pdcEnabled_.load() && trackIdx < trackLatencies_.size()) {
        applyPDCDelay(trackBuffer, static_cast<int>(trackIdx), numSamples);
      }

      // Mix to master output based on graph connections
      for (const auto &conn : snapshot->connections) {
        if (conn.sourceId == nodeId && conn.destId == "master") {
          // Apply connection gain (e.g. master fader/send level if modeled that
          // way) Note: Track fader is already applied in getNextAudioBlock via
          // applyGainAndPan? Usually getNextAudioBlock produces "post-fader"
          // audio if it includes fader. If connection gain is unity, this is
          // just a mix.

          for (int channel = 0;
               channel < juce::jmin(outputBuffer.getNumChannels(),
                                    trackBuffer.getNumChannels());
               ++channel) {
            outputBuffer.addFrom(channel, 0,
                                 trackBuffer.getReadPointer(channel),
                                 numSamples, conn.gain);
          }
          break;
        }
      }
      continue;
    }

    // 2. Try to find an Aux Bus
    AuxBus *bus = nullptr;
    size_t busIdx = 0;

    for (size_t i = 0; i < auxBuses.size(); ++i) {
      if (auxBuses[i] && auxBuses[i]->getId() == nodeId) {
        bus = auxBuses[i].get();
        busIdx = i;
        break;
      }
    }

    if (bus != nullptr) {
      if (busIdx < auxBusBuffers_.size()) {
        auto &busBuffer = auxBusBuffers_[busIdx];

        // Process the bus
        juce::AudioSourceChannelInfo auxInfo(&busBuffer, 0, numSamples);
        bus->getNextAudioBlock(auxInfo);

        // Mix to master output based on graph connections
        for (const auto &conn : snapshot->connections) {
          if (conn.sourceId == nodeId && conn.destId == "master") {
            for (int channel = 0;
                 channel < juce::jmin(outputBuffer.getNumChannels(),
                                      busBuffer.getNumChannels());
                 ++channel) {
              outputBuffer.addFrom(channel, 0, busBuffer, channel, 0,
                                   numSamples, conn.gain);
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

  // Update metering
  updateMasterMeters(outputBuffer);
}

//==============================================================================
int AudioRenderer::calculatePDC(
    const std::vector<std::shared_ptr<Track>> &tracks) {
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

  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    float *data = buffer.getWritePointer(ch);

    for (int i = 0; i < numSamples; ++i) {
      // Read delayed sample
      int readPos =
          (writePos - delayNeeded + constants::kMaxPDCLatencySamples) %
          constants::kMaxPDCLatencySamples;
      float delayed = delayBuffer.getSample(ch, readPos);

      // Write current sample
      delayBuffer.setSample(ch, writePos, data[i]);

      // Output delayed sample
      data[i] = delayed;

      writePos = (writePos + 1) % constants::kMaxPDCLatencySamples;
    }
  }
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
  if (trackIndex >= 0 && trackIndex < static_cast<int>(trackLatencies_.size())) {
    return trackLatencies_[trackIndex];
  }
  return 0;
}

int AudioRenderer::getMasterLatency() const {
  // Sum latency from all master plugins
  // Note: The master limiter has effectively zero latency since it's a simple lookahead limiter
  // We rely on the cached value that's updated during processing
  return masterLatency_.load();
}

void AudioRenderer::updateMasterLatency(
    const std::vector<std::unique_ptr<juce::AudioPluginInstance>> &masterPlugins,
    int limiterLatency) {
  int totalLatency = limiterLatency;
  
  for (const auto &plugin : masterPlugins) {
    if (plugin != nullptr) {
      totalLatency += plugin->getLatencySamples();
    }
  }
  
  masterLatency_.store(totalLatency);
}

} // namespace zenith
