/*
  ==============================================================================
    agents/RealtimeAudioEngineAgent/RealtimeAudioEngineAgent.cpp
    Real-time audio engine coordinator agent implementation.
  ==============================================================================
*/

#include "RealtimeAudioEngineAgent.h"

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
  // RT-safe processing - no allocations, no locks
  const auto startTicks = juce::Time::getHighResolutionTicks();
  
  // 1. Process pending commands from UI/Management threads
  processCommands();
  
  // TODO: Process plugin chain
  
  const auto numSamples = buffer.getNumSamples();
  
  // Placeholder: clear buffer (silence)
  buffer.clear();

  // Metrics Update
  const auto endTicks = juce::Time::getHighResolutionTicks();
  const double processingSeconds = juce::Time::highResolutionTicksToSeconds(endTicks - startTicks);

  // Calculate block duration
  // Use local copy of atomic sampleRate to ensure consistency within block
  const double sr = sampleRate_.load(std::memory_order_relaxed);
  const double blockDurationSeconds = (sr > 0.0) ? (static_cast<double>(numSamples) / sr) : 0.0;

  // Update CPU Usage (EMA)
  float instantCpu = 0.0f;
  if (blockDurationSeconds > 0.000001) {
    instantCpu = static_cast<float>(processingSeconds / blockDurationSeconds);
    // Clamp to reasonable range [0, 4.0] to avoid spikes messing up UI
    instantCpu = juce::jlimit(0.0f, 4.0f, instantCpu);
  }

  constexpr float alpha = 0.05f;
  cpuUsageSmoothed_ = (cpuUsageSmoothed_ * (1.0f - alpha)) + (instantCpu * alpha);
  metrics_.cpuUsage.store(cpuUsageSmoothed_, std::memory_order_relaxed);

  // Update Overload/Underrun
  // Tolerance 1% to avoid jitter false positives
  const bool overloaded = (processingSeconds > blockDurationSeconds * 1.01);
  metrics_.overloadDetected.store(overloaded, std::memory_order_relaxed);

  if (overloaded) {
    // Count deadline misses as underruns (fallback behavior)
    metrics_.bufferUnderruns.fetch_add(1, std::memory_order_relaxed);
  }

  metrics_.samplesProcessed.fetch_add(numSamples, std::memory_order_relaxed);
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
  isRunning_.store(false, std::memory_order_release);
  // TODO: Wait for audio thread to acknowledge stop
  // TODO: Cleanup any pending commands
}

bool RealtimeAudioEngineAgent::isRunning() const noexcept {
  return isRunning_.load(std::memory_order_acquire);
}

} // namespace agents
} // namespace zenith
