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
#include <algorithm> // Added for std::min

namespace zenith {

//==============================================================================
void AudioRenderer::renderAudioGraph(
    AudioRenderContext& context,
    juce::AudioBuffer<float> &outputBuffer, int numSamples,
    juce::int64 playheadPosition,
    std::span<Track* const> tracks,
    std::span<AuxBus* const> auxBuses,
    const RoutingGraph &routingGraph, MasterLimiter &masterLimiter,
    std::span<const std::shared_ptr<juce::AudioPluginInstance>> masterPlugins,
    const TempoMap *tempoMap, const juce::MidiBuffer *incomingMidi,
    const float *const *inputChannelData,
    int numInputChannels) noexcept {

  juce::ScopedNoDenormals noDenormals;
  outputBuffer.clear();

  const auto *snapshot = routingGraph.getSnapshot();
  if (snapshot == nullptr || snapshot->topology->processingOrder.empty()) {
    return;
  }

  const size_t numBuses = std::min(auxBuses.size(), context.auxBusBuffers.size());
  for (size_t i = 0; i < numBuses; ++i) {
    context.auxBusBuffers[i].clear();
  }

  static constexpr int kMaxAuxBuses = 32;
  std::array<juce::AudioBuffer<float> *, kMaxAuxBuses> auxBufferPtrs;
  size_t actualAuxCount = 0;
  for (size_t i = 0; i < numBuses && actualAuxCount < kMaxAuxBuses; ++i) {
    auxBufferPtrs[actualAuxCount++] = &context.auxBusBuffers[i];
  }
  
  context.auxBufferPtrsVector.clear();
  for(size_t i = 0; i < actualAuxCount; ++i) context.auxBufferPtrsVector.push_back(auxBufferPtrs[i]);

  for (const auto &nodeId : snapshot->topology->processingOrder) {
    auto trackIt = snapshot->trackLookup.find(nodeId);
    if (trackIt != snapshot->trackLookup.end()) {
      if (auto trackPtr = trackIt->second.lock()) {
         Track* track = trackPtr.get();
         int trackIdx = track->getTrackIndex();
         
      if (track->isFrozen()) {
        auto freezeBuffer = track->getFreezeBuffer();
        if (freezeBuffer != nullptr && trackIdx >= 0 && trackIdx < (int)context.trackBuffers.size()) {
          auto &trackBuffer = context.trackBuffers[trackIdx];
          trackBuffer.clear();

          const juce::int64 readPos = playheadPosition;
          const int bufferLength = freezeBuffer->getNumSamples();
          
          if (readPos >= 0 && readPos < bufferLength) {
             const int samplesToRead = std::min(numSamples, static_cast<int>(bufferLength - readPos));
             if (samplesToRead > 0) {
                 for (int ch = 0; ch < std::min(trackBuffer.getNumChannels(), freezeBuffer->getNumChannels()); ++ch) {
                     trackBuffer.copyFrom(ch, 0, *freezeBuffer, ch, (int)readPos, samplesToRead);
                 }
             }
          }

          if (auto* processor = track->getProcessor()) {
              juce::MidiBuffer dummyMidi;
              juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);
              processor->processBlock(trackInfo, dummyMidi, {}, nullptr);
          }

          for (const auto &conn : snapshot->topology->connections) {
            if (conn.sourceId == nodeId && conn.destId == "master") {
              if (conn.gain != 1.0f) trackBuffer.applyGain(conn.gain);
              
              for (int ch = 0; ch < std::min(outputBuffer.getNumChannels(), trackBuffer.getNumChannels()); ++ch) {
                  juce::FloatVectorOperations::add(outputBuffer.getWritePointer(ch), 
                                                trackBuffer.getReadPointer(ch), 
                                                numSamples);
              }
              break; 
            }
          }
        }
        continue;
      }

      if (trackIdx < 0 || trackIdx >= (int)context.trackBuffers.size())
        continue;

      auto &trackBuffer = context.trackBuffers[trackIdx];
      trackBuffer.clear();

      if (track->isBeingFrozen()) {
          continue;
      }

      juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);

      const juce::MidiBuffer *trackMidiInput = (incomingMidi != nullptr && !incomingMidi->isEmpty() &&
                                              track->getType() == Track::Type::Instrument && track->isArmed()) 
                                              ? incomingMidi : nullptr;

      const juce::AudioBuffer<float>* sidechainBuffer = nullptr;
      if (auto* sourceTrack = track->getSidechainSource()) {
          int sourceIdx = sourceTrack->getTrackIndex();
          if (sourceIdx >= 0 && sourceIdx < (int)context.trackBuffers.size()) {
              sidechainBuffer = &context.trackBuffers[sourceIdx];
          }
      }

      track->getNextAudioBlock(trackInfo, playheadPosition, trackMidiInput,
                                context.auxBufferPtrsVector, tempoMap, sidechainBuffer);

      if (context.pdcDelayBuffers.size() > 0) { 
        applyPDCDelay(context, trackBuffer, static_cast<int>(trackIdx), numSamples);
      }

      for (const auto &conn : snapshot->topology->connections) {
        if (conn.sourceId == nodeId && conn.destId == "master") {
          for (int ch = 0; ch < std::min(outputBuffer.getNumChannels(), trackBuffer.getNumChannels()); ++ch) {
            outputBuffer.addFrom(ch, 0, trackBuffer.getReadPointer(ch), numSamples, conn.gain);
          }
          break;
        }
      }
      }
      continue;
    }

    auto busIt = snapshot->auxBusLookup.find(nodeId);
    if (busIt != snapshot->auxBusLookup.end()) {
       if (auto busPtr = busIt->second.lock()) {
         AuxBus* bus = busPtr.get();
       size_t busIdx = 0;
       bool found = false;
       for (size_t i = 0; i < auxBuses.size(); ++i) {
           if (auxBuses[i] == bus) {
               busIdx = i;
               found = true;
               break;
           }
       }

       if (found && busIdx < context.auxBusBuffers.size()) {
        auto &busBuffer = context.auxBusBuffers[busIdx];
        juce::AudioSourceChannelInfo auxInfo(&busBuffer, 0, numSamples);
        bus->getNextAudioBlock(auxInfo);

        for (const auto &conn : snapshot->topology->connections) {
          if (conn.sourceId == nodeId && conn.destId == "master") {
            for (int ch = 0; ch < std::min(outputBuffer.getNumChannels(), busBuffer.getNumChannels()); ++ch) {
              outputBuffer.addFrom(ch, 0, busBuffer, ch, 0, numSamples, conn.gain);
            }
            break;
          }
        }
        }
      }
      continue;
    }
  }
  processMasterPlugins(outputBuffer, masterPlugins);
  masterLimiter.process(outputBuffer);
  dither_.process(outputBuffer, 24); 
  updateMasterMeters(outputBuffer);
}

//==============================================================================
int AudioRenderer::calculatePDC(AudioRenderContext& context, std::span<Track *const> tracks) {
  int maxLatency = 0;

  for (size_t i = 0; i < tracks.size() && i < context.trackLatencies.size(); ++i) {
    if (tracks[i]) {
      int trackLatency = 0;

      for (int p = 0; p < tracks[i]->getNumPlugins(); ++p) {
        auto *plugin = tracks[i]->getPlugin(p);
        if (plugin != nullptr) {
          trackLatency += plugin->getLatencySamples();
        }
      }

      context.trackLatencies[i] = trackLatency;
      maxLatency = std::max(maxLatency, trackLatency);
    }
  }

  context.maxTrackLatency = maxLatency;
  return maxLatency;
}

//==============================================================================
void AudioRenderer::applyPDCDelay(AudioRenderContext& context, juce::AudioBuffer<float> &buffer,
                                  int trackIndex, int numSamples) {
  if (trackIndex < 0 ||
      trackIndex >= static_cast<int>(context.trackLatencies.size())) {
    return;
  }

  const int trackLatency = context.trackLatencies[trackIndex];
  const int maxLatency = context.maxTrackLatency;
  const int delayNeeded = maxLatency - trackLatency;

  if (delayNeeded <= 0 || delayNeeded > constants::kMaxPDCLatencySamples - 1) {
    return;
  }

  if (trackIndex < 0 || trackIndex >= (int)context.pdcDelayBuffers.size()) {
    return;
  }

  auto &delayBuffer = context.pdcDelayBuffers[trackIndex];
  int &writePos = context.pdcDelayWritePos[trackIndex];

  auto *const *channelData = buffer.getArrayOfWritePointers();
  const int numBufferChannels = buffer.getNumChannels();
  const int numDelayBufferChannels = delayBuffer.getNumChannels();

  const int initialReadPos =
      (writePos - delayNeeded + constants::kMaxPDCLatencySamples) %
      constants::kMaxPDCLatencySamples;

  for (int ch = 0; ch < numBufferChannels && ch < numDelayBufferChannels; ++ch) {
    float* channelPtr = channelData[ch];
    float* delayChannelPtr = delayBuffer.getWritePointer(ch);
    
    int readPos = initialReadPos;
    int localWritePos = writePos;
    
    for (int i = 0; i < numSamples; ++i) {
      const float delayedSample = delayChannelPtr[readPos];
      delayChannelPtr[localWritePos] = channelPtr[i];
      channelPtr[i] = delayedSample;
      
      readPos = (readPos + 1) % constants::kMaxPDCLatencySamples;
      localWritePos = (localWritePos + 1) % constants::kMaxPDCLatencySamples;
    }
  }

  writePos = (writePos + numSamples) % constants::kMaxPDCLatencySamples;
}

//==============================================================================
void AudioRenderer::processMasterPlugins(
    juce::AudioBuffer<float> &buffer,
    std::span<const std::shared_ptr<juce::AudioPluginInstance>> plugins) {

  if (plugins.empty()) {
    return;
  }

  juce::MidiBuffer midi;

  for (auto &plugin : plugins) {
    if (plugin != nullptr && !plugin->isSuspended()) {
      plugin->processBlock(buffer, midi);
    }
  }
}

//==============================================================================
void AudioRenderer::updateMasterMeters(const juce::AudioBuffer<float> &buffer) {
  float peak = simd::findPeak(buffer);

  float currentLevel = masterLevel_.load();
  currentLevel = currentLevel * constants::kMeterSmoothingFactor +
                 peak * (1.0f - constants::kMeterSmoothingFactor);
  masterLevel_.store(currentLevel);

  float currentPeak = masterPeakLevel_.load();
  if (peak > currentPeak) {
    masterPeakLevel_.store(peak);
  } else {
    masterPeakLevel_.store(currentPeak * constants::kPeakMeterDecay);
  }
}

int AudioRenderer::getMasterLatency() const {
  return masterLatency_.load();
}

void AudioRenderer::updateMasterLatency(
    std::span<const std::shared_ptr<juce::AudioPluginInstance>> masterPlugins,
    int limiterLatency) {
  int totalLatency = limiterLatency;
  
  for (const auto& plugin : masterPlugins) {
    if (plugin != nullptr) {
      totalLatency += plugin->getLatencySamples();
    }
  }
  
  masterLatency_.store(totalLatency);
}

void AudioRenderer::updateClipPositions(std::span<Track* const> tracks, 
                                        juce::int64 playheadPosition) noexcept {
    juce::ignoreUnused(tracks, playheadPosition);
}

} // namespace zenith
