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

// NOTE: AudioRenderer is now stateless. Prepare/reset are on AudioRenderContext.

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

  // RT-Safety: Disable denormals to prevent CPU spikes with near-zero floats
  juce::ScopedNoDenormals noDenormals;

  // Clear output buffer
  outputBuffer.clear();

  // Get routing snapshot (lock-free)
  const auto *snapshot = routingGraph.getSnapshot();
  // FIXED: processingOrder is in topology
  if (snapshot == nullptr || snapshot->topology->processingOrder.empty()) {
    return;
  }

  // Pre-clear aux bus buffers
  const size_t numBuses = juce::jmin(auxBuses.size(), context.auxBusBuffers.size());
  for (size_t i = 0; i < numBuses; ++i) {
    context.auxBusBuffers[i].clear();
    
    // INJECT FEEDBACK: Mix signal from previous block (delay)
    if (i < context.auxBusFeedbackBuffers.size()) {
       auto& feedback = context.auxBusFeedbackBuffers[i];
       // Simply add feedback buffer contents to input. 
       // Feedback buffer contains what was written by feedback sources in previous block.
       for (int ch = 0; ch < juce::jmin(context.auxBusBuffers[i].getNumChannels(), feedback.getNumChannels()); ++ch) {
           context.auxBusBuffers[i].addFrom(ch, 0, feedback, ch, 0, numSamples);
       }
       feedback.clear(); // Clear for next block capture
    }
  }

  // Build aux buffer pointers for tracks (RT-safe stack allocation or fixed member)
  // We'll use a local array for safety since it's small (max 16 aux buses usually)
  static constexpr int kMaxAuxBuses = 32;
  std::array<juce::AudioBuffer<float> *, kMaxAuxBuses> auxBufferPtrs;
  size_t actualAuxCount = 0;
  for (size_t i = 0; i < numBuses && actualAuxCount < kMaxAuxBuses; ++i) {
    auxBufferPtrs[actualAuxCount++] = &context.auxBusBuffers[i];
  }
  
  // Initialize vector with standard (forward) pointers
  context.auxBufferPtrsVector.clear();
  for(size_t i = 0; i < actualAuxCount; ++i) context.auxBufferPtrsVector.push_back(auxBufferPtrs[i]);

  // Process nodes in topological order using FAST LOOKUP
  for (const auto &nodeId : snapshot->topology->processingOrder) {
    // 1. Try to find a Track using fast lookup
    auto trackIt = snapshot->trackLookup.find(nodeId);
    if (trackIt != snapshot->trackLookup.end()) {
      if (auto trackPtr = trackIt->second.lock()) {
        Track* track = trackPtr.get();
        int trackIdx = track->getTrackIndex(); 

      // Handle frozen tracks - play back their freeze buffer (RT-safe)
      if (track->isFrozen()) {
        auto freezeBuffer = track->getFreezeBuffer();
        if (freezeBuffer != nullptr && trackIdx >= 0 && trackIdx < (int)context.trackBuffers.size()) {
          auto &trackBuffer = context.trackBuffers[trackIdx];
          trackBuffer.clear();

          const juce::int64 readPos = playheadPosition;
          const int bufferLength = freezeBuffer->getNumSamples();
          
          if (readPos >= 0 && readPos < bufferLength) {
             const int samplesToRead = juce::jmin(numSamples, static_cast<int>(bufferLength - readPos));
             if (samplesToRead > 0) {
                 for (int ch = 0; ch < juce::jmin(trackBuffer.getNumChannels(), freezeBuffer->getNumChannels()); ++ch) {
                     trackBuffer.copyFrom(ch, 0, *freezeBuffer, ch, (int)readPos, samplesToRead);
                 }
             }
          }

          if (auto* processor = track->getProcessor()) {
              juce::MidiBuffer dummyMidi;
              juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);
              processor->processBlock(trackInfo, dummyMidi, {}, nullptr);
          }

          // Mix frozen track using SIMD if possible
          // FIXED: connections is in topology
          for (const auto &conn : snapshot->topology->connections) {
            if (conn.sourceId == nodeId && conn.destId == "master") {
              if (conn.gain != 1.0f) trackBuffer.applyGain(conn.gain);
              
              for (int ch = 0; ch < juce::jmin(outputBuffer.getNumChannels(), trackBuffer.getNumChannels()); ++ch) {
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
      
      // INJECT TRACK FEEDBACK (if any)
      // Tracks can receive feedback on input if routed that way? 
      // Current design doesn't really support general Track Input routing from other tracks except Sidechain.
      // But if we supported it:
      // if (trackIdx < context.trackFeedbackBuffers.size()) { ... mix ... clear ... }

      // Skip processing if track is currently being frozen (prevent race condition)
      if (track->isBeingFrozen()) {
          continue;
      }

      // PREPARE ROUTING FOR THIS TRACK
      // 1. Reset Aux Ptrs to Default (Forward)
      for(size_t i = 0; i < actualAuxCount; ++i) context.auxBufferPtrsVector[i] = auxBufferPtrs[i];
      
      bool isFeedbackSource = false;

      // 2. Scan Connections for this node to handle Feedback Routing
      for (const auto &conn : snapshot->topology->connections) {
          if (conn.sourceId == nodeId) {
              if (conn.isFeedback) {
                  isFeedbackSource = true;
                  // If sending to Aux Bus in Feedback loop, swap pointer!
                  if (auto auxIt = snapshot->auxBusLookup.find(conn.destId); auxIt != snapshot->auxBusLookup.end()) {
                      if (auto auxPtr = auxIt->second.lock()) {
                          int auxIdx = auxPtr->getBusIndex();
                          if (auxIdx >= 0 && auxIdx < (int)context.auxBufferPtrsVector.size()) {
                              // Point to Feedback Buffer instead of Input Buffer
                              if (auxIdx < (int)context.auxBusFeedbackBuffers.size()) {
                                  context.auxBufferPtrsVector[auxIdx] = &context.auxBusFeedbackBuffers[auxIdx];
                              }
                          }
                      }
                  }
              }
          }
      }

      juce::AudioSourceChannelInfo trackInfo(&trackBuffer, 0, numSamples);

      const juce::MidiBuffer *trackMidiInput = (incomingMidi != nullptr && !incomingMidi->isEmpty() &&
                                              track->getType() == Track::Type::Instrument && track->isArmed()) 
                                              ? incomingMidi : nullptr;

      const juce::AudioBuffer<float>* sidechainBuffer = nullptr;
      if (auto* sourceTrack = track->getSidechainSource()) {
          int sourceIdx = sourceTrack->getTrackIndex();
          if (sourceIdx >= 0 && sourceIdx < (int)context.trackBuffers.size()) {
              // Check if Sidechain connection is Feedback
              bool isSidechainFeedback = false;
              for (const auto &conn : snapshot->topology->connections) {
                  if (conn.sourceId == sourceTrack->getId() && conn.destId == nodeId && conn.isFeedback) {
                      isSidechainFeedback = true;
                      break;
                  }
              }
              
              if (isSidechainFeedback && sourceIdx < (int)context.trackFeedbackBuffers.size()) {
                   sidechainBuffer = &context.trackFeedbackBuffers[sourceIdx];
              } else {
                   sidechainBuffer = &context.trackBuffers[sourceIdx];
              }
          }
      }


      track->getNextAudioBlock(trackInfo, playheadPosition, trackMidiInput,
                                context.auxBufferPtrsVector, tempoMap, sidechainBuffer);

      if (pdcEnabled_.load()) {
        applyPDCDelay(context, trackBuffer, static_cast<int>(trackIdx), numSamples);
      }
      
      // SAVE FEEDBACK SOURCE
      if (isFeedbackSource) {
           if (trackIdx < (int)context.trackFeedbackBuffers.size()) {
               // Copy output to feedback buffer for next block usage
               for (int ch = 0; ch < juce::jmin(trackBuffer.getNumChannels(), context.trackFeedbackBuffers[trackIdx].getNumChannels()); ++ch) {
                   context.trackFeedbackBuffers[trackIdx].copyFrom(ch, 0, trackBuffer, ch, 0, numSamples);
               }
           }
      }

      // FIXED: connections is in topology
      for (const auto &conn : snapshot->topology->connections) {
        if (conn.sourceId == nodeId && conn.destId == "master") {
          for (int ch = 0; ch < juce::jmin(outputBuffer.getNumChannels(), trackBuffer.getNumChannels()); ++ch) {
            outputBuffer.addFrom(ch, 0, trackBuffer.getReadPointer(ch), numSamples, conn.gain);
          }
          break;
        }
      }
      }
      continue;
    }

    // 2. Try to find an Aux Bus using fast lookup
    auto busIt = snapshot->auxBusLookup.find(nodeId);
    if (busIt != snapshot->auxBusLookup.end()) {
       if (auto busPtr = busIt->second.lock()) {
         AuxBus* bus = busPtr.get();
         
         // Fast lookup via cached index
         int busIdx = bus->getBusIndex();

         if (busIdx >= 0 && busIdx < (int)context.auxBusBuffers.size()) {
           auto &busBuffer = context.auxBusBuffers[busIdx];
           juce::AudioSourceChannelInfo auxInfo(&busBuffer, 0, numSamples);
           bus->getNextAudioBlock(auxInfo);
           
           // Check if this Aux Bus is a source of feedback (e.g. Aux -> Aux or Aux -> Track)
           bool isFeedbackSource = false;
            for (const auto &conn : snapshot->topology->connections) {
                  if (conn.sourceId == nodeId && conn.isFeedback) {
                      isFeedbackSource = true; 
                      break; 
                  }
            }
            
            if (isFeedbackSource && busIdx < (int)context.auxBusFeedbackBuffers.size()) {
                 // Save output
                 for (int ch = 0; ch < juce::jmin(busBuffer.getNumChannels(), context.auxBusFeedbackBuffers[busIdx].getNumChannels()); ++ch) {
                     context.auxBusFeedbackBuffers[busIdx].copyFrom(ch, 0, busBuffer, ch, 0, numSamples);
                 }
            }

           // FIXED: connections is in topology
           for (const auto &conn : snapshot->topology->connections) {
             if (conn.sourceId == nodeId && conn.destId == "master") {
               for (int ch = 0; ch < juce::jmin(outputBuffer.getNumChannels(), busBuffer.getNumChannels()); ++ch) {
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
  // Process master bus plugins
  processMasterPlugins(outputBuffer, masterPlugins);
  // Apply master limiter (final clipping protection)
  masterLimiter.process(outputBuffer);

  // Apply TPDF Dither
  dither_.process(outputBuffer, 24); 

  // Update metering
  updateMasterMeters(outputBuffer);
}

//==============================================================================
int AudioRenderer::calculatePDC(AudioRenderContext& context, std::span<Track *const> tracks) {
  int maxLatency = 0;

  for (size_t i = 0; i < tracks.size() && i < context.trackLatencies.size(); ++i) {
    if (tracks[i]) {
      int trackLatency = 0;

      // Sum latency from all plugins
      for (int p = 0; p < tracks[i]->getNumPlugins(); ++p) {
        auto *plugin = tracks[i]->getPlugin(p);
        if (plugin != nullptr) {
          trackLatency += plugin->getLatencySamples();
        }
      }

      context.trackLatencies[i] = trackLatency;
      maxLatency = juce::jmax(maxLatency, trackLatency);
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

  // No delay needed if track already has max latency, or delay exceeds buffer
  // capacity Note: Use > (not >=) because delay buffer can handle up to
  // kMaxPDCLatencySamples-1
  if (delayNeeded <= 0 || delayNeeded > constants::kMaxPDCLatencySamples - 1) {
    return;
  }

  if (trackIndex < 0 || trackIndex >= (int)context.pdcDelayBuffers.size()) {
    return;
  }

  auto &delayBuffer = context.pdcDelayBuffers[trackIndex];
  int &writePos = context.pdcDelayWritePos[trackIndex];

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
  for (int ch = 0; ch < numBufferChannels && ch < numDelayBufferChannels; ++ch) {
    float* channelPtr = channelData[ch];
    float* delayChannelPtr = delayBuffer.getWritePointer(ch);
    
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
    std::span<const std::shared_ptr<juce::AudioPluginInstance>> plugins) {

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
    // Method used to update clips when not playing, or for UI sync.
    // For now, no-op as implicit updates happen during processing.
    juce::ignoreUnused(tracks, playheadPosition);
}

} // namespace zenith
