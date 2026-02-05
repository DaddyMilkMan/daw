/*
  ==============================================================================
    agents/ScheduledTransportAgent/ScheduledTransportAgent.h
    Timeline and transport management with sample-accurate scheduling.
  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <cstdint>

namespace zenith {
namespace agents {

//==============================================================================
/**
    ScheduledTransportAgent manages playback timeline, tempo, and sample-accurate
    event scheduling for the DAW transport system.
*/
class ScheduledTransportAgent {
public:
  //==============================================================================
  enum class TransportState {
    Stopped,
    Playing,
    Recording,
    Paused
  };
  
  struct Position {
    int64_t samplePosition{0};
    double beats{0.0};
    double bars{0.0};
    double ppq{0.0}; // Pulses per quarter note
    double tempo{120.0};
    int timeSignatureNumerator{4};
    int timeSignatureDenominator{4};
  };

  //==============================================================================
  ScheduledTransportAgent();
  ~ScheduledTransportAgent();

  //==============================================================================
  // Transport Control (UI thread)
  
  void play();
  void stop();
  void record();
  void pause();
  void rewind();
  void setPosition(int64_t samplePosition);
  
  //==============================================================================
  // Configuration (UI thread)
  
  void setTempo(double tempo);
  void setTimeSignature(int numerator, int denominator);
  void setLoopRegion(int64_t startSample, int64_t endSample);
  void setLoopEnabled(bool enabled);
  
  //==============================================================================
  // Real-time Processing (RT thread)
  
  /// Advance transport by numSamples and trigger scheduled events
  void advance(int numSamples, double sampleRate) noexcept;
  
  /// Get current position (RT-safe)
  Position getPosition() const noexcept;
  
  /// Get current transport state (RT-safe)
  TransportState getState() const noexcept;

private:
  //==============================================================================
  std::atomic<TransportState> state_{TransportState::Stopped};
  std::atomic<int64_t> samplePosition_{0};
  std::atomic<double> tempo_{120.0};
  std::atomic<int> timeSignatureNumerator_{4};
  std::atomic<int> timeSignatureDenominator_{4};
  std::atomic<bool> loopEnabled_{false};
  std::atomic<int64_t> loopStart_{0};
  std::atomic<int64_t> loopEnd_{0};
  
  // TODO: Add lock-free tempo map
  // TODO: Add sample-accurate event queue
  // TODO: Add external sync handling
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScheduledTransportAgent)
};

} // namespace agents
} // namespace zenith
