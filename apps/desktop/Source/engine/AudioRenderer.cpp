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
AudioRenderer::AudioRenderer() {
  scratchMidi_.ensureSize(65536);
}

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
  if (snapshot == nullptr || snapshot->renderList.empty()) {
    return;
  }

  // Use pre-allocated pointers to avoid std::vector allocations
  context.auxBufferPtrsVector.clear();
  for (auto &buffer : context.auxBusBuffers) {
      buffer.clear();
      context.auxBufferPtrsVector.push_back(&buffer);
  }

  for (const auto &rn : snapshot->renderList) {
    if (rn.type == RoutingGraph::RenderNode::Type::Track) {
      Track* track = rn.track;
      int trackIdx = rn.bufferIndex;
         
      if (trackIdx < 0 || trackIdx >= (int)context.trackBuffers.size())
        continue;

      auto &trackBuffer = context.trackBuffers[trackIdx];
      trackBuffer.clear();

      if (track->isFrozen()) {
        auto freezeBuffer = track->getFreezeBuffer();
        if (freezeBuffer != nullptr) {
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
              scratchMidi_.clear();
              juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);
              processor->processBlock(trackInfo, scratchMidi_, context.auxBufferPtrsVector, nullptr);
          }

          if (rn.hasMasterSend) {
            if (rn.masterGain != 1.0f) trackBuffer.applyGain(rn.masterGain);
            
            for (int ch = 0; ch < std::min(outputBuffer.getNumChannels(), trackBuffer.getNumChannels()); ++ch) {
                juce::FloatVectorOperations::add(outputBuffer.getWritePointer(ch), 
                                              trackBuffer.getReadPointer(ch), 
                                              numSamples);
            }
          }
        }
        continue;
      }

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
        applyPDCDelay(context, trackBuffer, trackIdx, numSamples);
      }

      if (rn.hasMasterSend) {
          for (int ch = 0; ch < std::min(outputBuffer.getNumChannels(), trackBuffer.getNumChannels()); ++ch) {
            outputBuffer.addFrom(ch, 0, trackBuffer.getReadPointer(ch), numSamples, rn.masterGain);
          }
      }
    } else if (rn.type == RoutingGraph::RenderNode::Type::Bus) {
       AuxBus* bus = rn.auxBus;
       int busIdx = rn.bufferIndex;

       if (busIdx >= 0 && busIdx < (int)context.auxBusBuffers.size()) {
        auto &busBuffer = context.auxBusBuffers[busIdx];
        juce::AudioSourceChannelInfo auxInfo(&busBuffer, 0, numSamples);
        bus->getNextAudioBlock(auxInfo);

        if (rn.hasMasterSend) {
            for (int ch = 0; ch < std::min(outputBuffer.getNumChannels(), busBuffer.getNumChannels()); ++ch) {
              outputBuffer.addFrom(ch, 0, busBuffer, ch, 0, numSamples, rn.masterGain);
            }
        }
      }
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

  const int numBufferChannels = buffer.getNumChannels();
  const int numDelayBufferChannels = delayBuffer.getNumChannels();
  const int maxDelay = constants::kMaxPDCLatencySamples;

  const int initialReadPos = (writePos - delayNeeded + maxDelay) % maxDelay;

  for (int ch = 0; ch < numBufferChannels && ch < numDelayBufferChannels; ++ch) {
    float* channelPtr = buffer.getWritePointer(ch);
    float* delayChannelPtr = delayBuffer.getWritePointer(ch);
    
    int readP = initialReadPos;
    int writeP = writePos;
    
    // Process in chunks to handle buffer wrapping and improve performance
    int samplesProcessed = 0;
    while (samplesProcessed < numSamples) {
        int samplesToProcess = std::min(numSamples - samplesProcessed,
                                        std::min(maxDelay - readP, maxDelay - writeP));
        
        // We need to swap samples: output = delayed, delayed = input
        // Since we don't have a "swap" vector operation, we'll do it in a small loop
        // but without modulo.
        float* src = &channelPtr[samplesProcessed];
        float* dly = &delayChannelPtr[readP];
        float* dlyW = &delayChannelPtr[writeP];

        if (readP == writeP) {
            // Special case: no delay (shouldn't happen here due to delayNeeded > 0 check)
            // but just in case, we do nothing.
        } else {
            for (int i = 0; i < samplesToProcess; ++i) {
                float in = src[i];
                src[i] = dly[i];
                dlyW[i] = in;
            }
        }

        readP = (readP + samplesToProcess) % maxDelay;
        writeP = (writeP + samplesToProcess) % maxDelay;
        samplesProcessed += samplesToProcess;
    }
  }

  writePos = (writePos + numSamples) % maxDelay;
}

//==============================================================================
void AudioRenderer::processMasterPlugins(
    juce::AudioBuffer<float> &buffer,
    std::span<const std::shared_ptr<juce::AudioPluginInstance>> plugins) {

  if (plugins.empty()) {
    return;
  }

  scratchMidi_.clear();

  for (auto &plugin : plugins) {
    if (plugin != nullptr && !plugin->isSuspended()) {
      plugin->processBlock(buffer, scratchMidi_);
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
