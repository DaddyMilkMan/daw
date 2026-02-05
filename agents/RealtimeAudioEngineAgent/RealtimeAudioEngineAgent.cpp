/*
  ==============================================================================
    agents/RealtimeAudioEngineAgent/RealtimeAudioEngineAgent.cpp
    Real-time audio engine coordinator agent implementation.
  ==============================================================================
*/

#include "RealtimeAudioEngineAgent.h"
#include <thread>
#include <chrono>

namespace zenith {
namespace agents {

//==============================================================================
RealtimeAudioEngineAgent::RealtimeAudioEngineAgent() {
  // Initialize with safe defaults
}

RealtimeAudioEngineAgent::~RealtimeAudioEngineAgent() {
  // Ensure audio is stopped before destruction
  if (isRunning_.load(std::memory_order_acquire)) {
    stop();
  }
}

//==============================================================================
void RealtimeAudioEngineAgent::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midi) noexcept {
  // RAII helper to ensure flag is cleared even if we return early
  struct ScopedProcessing {
    std::atomic<bool>& flag;
    ScopedProcessing(std::atomic<bool>& f) : flag(f) {
      flag.store(true, std::memory_order_seq_cst);
    }
    ~ScopedProcessing() {
      flag.store(false, std::memory_order_release);
    }
  };

  ScopedProcessing scoped(insideAudioCallback_);

  // Check if we should be running (must be seq_cst to pair with store in stop())
  if (!isRunning_.load(std::memory_order_seq_cst)) {
    buffer.clear();
    midi.clear();
    return;
  }

  // RT-safe processing - no allocations, no locks
  
  // 1. Process pending commands from UI/Management threads
  processCommands();
  
  // TODO: Process plugin chain
  // TODO: Update metrics atomically (partially done below)
  
  const auto numSamples = buffer.getNumSamples();
  metrics_.samplesProcessed.fetch_add(numSamples, std::memory_order_relaxed);
  
  // Placeholder: clear buffer (silence)
  buffer.clear();
}

//==============================================================================
void RealtimeAudioEngineAgent::initialize(double sampleRate, int bufferSize) {
  jassert(sampleRate > 0.0 && bufferSize > 0);
  
  sampleRate_.store(sampleRate, std::memory_order_release);
  bufferSize_.store(bufferSize, std::memory_order_release);
  
  // Reset Fifo
  commandFifo_.reset();
  
  // TODO: Prepare plugin chain
  // TODO: Setup routing graph
}

//==============================================================================
bool RealtimeAudioEngineAgent::queueCommand(const EngineEvent& command) {
    int start1, size1, start2, size2;
    commandFifo_.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0) {
        commandBuffer_[start1] = command;
        commandFifo_.finishedWrite(1);
        return true;
    }
    
    // Buffer full
    return false;
}

void RealtimeAudioEngineAgent::processCommands() noexcept {
    int start1, size1, start2, size2;
    commandFifo_.prepareToRead(commandFifo_.getNumReady(), start1, size1, start2, size2);

    if (size1 > 0) {
        for (int i = 0; i < size1; ++i) {
            // const auto& event = commandBuffer_[start1 + i];
            // TODO: Apply event to internal state (graph, plugins, etc.)
        }
    }
    if (size2 > 0) {
        for (int i = 0; i < size2; ++i) {
            // const auto& event = commandBuffer_[start2 + i];
            // TODO: Apply event to internal state
        }
    }

    commandFifo_.finishedRead(size1 + size2);
}

void RealtimeAudioEngineAgent::start() {
  isRunning_.store(true, std::memory_order_release);
  // TODO: Signal audio thread to begin processing
}

void RealtimeAudioEngineAgent::stop() {
  isRunning_.store(false, std::memory_order_seq_cst);

  // Wait for audio thread to finish current block
  while (insideAudioCallback_.load(std::memory_order_seq_cst)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // TODO: Cleanup any pending commands
}

bool RealtimeAudioEngineAgent::isRunning() const noexcept {
  return isRunning_.load(std::memory_order_acquire);
}

} // namespace agents
} // namespace zenith
