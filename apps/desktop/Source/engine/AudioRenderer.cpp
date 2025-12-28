/*
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

  // Build aux buffer pointers for tracks (RT-safe stack allocation or fixed
  // member) We'll use a local array for safety since it's small (max 16 aux
  // buses usually)
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
                               auxBufferPtrsVector_, tempoMap);


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


} // namespace zenith
