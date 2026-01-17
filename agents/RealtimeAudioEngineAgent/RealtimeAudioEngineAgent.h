/*
  ==============================================================================
    agents/RealtimeAudioEngineAgent/RealtimeAudioEngineAgent.h
    Real-time audio engine coordinator agent for lock-free audio processing.
  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>

namespace zenith {
namespace agents {

//==============================================================================
/**
    RealtimeAudioEngineAgent coordinates real-time audio processing with
    lock-free thread communication and zero-allocation audio callbacks.
    
    This agent ensures thread-safe operations, manages plugin chains,
    handles dynamic routing, and monitors real-time performance.
*/
class RealtimeAudioEngineAgent {
public:
  //==============================================================================
  RealtimeAudioEngineAgent();
  ~RealtimeAudioEngineAgent();

  //==============================================================================
  // Audio Processing (RT-safe)
  
  /// Process audio block in real-time thread (must be RT-safe)
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) noexcept;
  
  //==============================================================================
  // Configuration (called from UI thread)
  
  /// Initialize audio engine with device configuration
  void initialize(double sampleRate, int bufferSize);
  
  /// Start audio processing
  void start();
  
  /// Stop audio processing
  void stop();
  
  /// Check if engine is currently processing
  bool isRunning() const noexcept;
  
  //==============================================================================
  // Metrics (RT-safe read)
  
  struct Metrics {
    std::atomic<float> cpuUsage{0.0f};
    std::atomic<int> bufferUnderruns{0};
    std::atomic<int64_t> samplesProcessed{0};
    std::atomic<bool> overloadDetected{false};
  };
  
  const Metrics& getMetrics() const noexcept { return metrics_; }

private:
  //==============================================================================
  // Member variables
  std::atomic<bool> isRunning_{false};
  std::atomic<double> sampleRate_{44100.0};
  std::atomic<int> bufferSize_{512};
  
  Metrics metrics_;
  
  // TODO: Add lock-free command queue
  // TODO: Add plugin chain management
  // TODO: Add routing graph
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealtimeAudioEngineAgent)
};

} // namespace agents
} // namespace zenith
