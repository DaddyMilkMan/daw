/*
  ==============================================================================
    agents/ClockSyncAgent/ClockSyncAgent.h
    Precision clock synchronization for distributed audio systems.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace zenith {
namespace agents {

//==============================================================================
/**
    ClockSyncAgent provides nanosecond-precision time synchronization across
    distributed audio systems, networks, and external hardware.
*/
class ClockSyncAgent {
public:
  //==============================================================================
  using Timestamp = std::chrono::nanoseconds;
  using HighResClock = std::chrono::high_resolution_clock;
  
  enum class TimeSource {
    LocalClock,
    NetworkPTP,
    NetworkNTP,
    MIDIClock,
    MTC,
    WordClock,
    Invalid
  };
  
  struct SyncStatus {
    TimeSource currentSource{TimeSource::LocalClock};
    bool synchronized{false};
    double driftPPM{0.0}; // Parts per million
    int64_t offsetNanoseconds{0};
    double latencyMs{0.0};
  };

  //==============================================================================
  ClockSyncAgent();
  ~ClockSyncAgent();

  //==============================================================================
  // Time Query (RT-safe)
  
  /// Get current synchronized timestamp (RT-safe, lock-free)
  Timestamp getCurrentTime() const noexcept;
  
  /// Convert sample position to timestamp
  Timestamp samplesToTimestamp(int64_t samplePosition, double sampleRate) const noexcept;
  
  /// Convert timestamp to sample position
  int64_t timestampToSamples(Timestamp timestamp, double sampleRate) const noexcept;
  
  //==============================================================================
  // Synchronization Control (UI thread)
  
  /// Set preferred time source
  void setTimeSource(TimeSource source);
  
  /// Get current synchronization status
  SyncStatus getSyncStatus() const;
  
  /// Force synchronization update
  void resynchronize();
  
  /// Enable/disable automatic drift compensation
  void setDriftCompensationEnabled(bool enabled);

private:
  //==============================================================================
  std::atomic<TimeSource> currentSource_{TimeSource::LocalClock};
  std::atomic<bool> synchronized_{false};
  std::atomic<int64_t> clockOffsetNs_{0};
  std::atomic<double> driftCompensation_{1.0};
  std::atomic<bool> driftCompensationEnabled_{true};
  
  // TODO: Add PTP/NTP client
  // TODO: Add MIDI clock parser
  // TODO: Add drift detector with filtering
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClockSyncAgent)
};

} // namespace agents
} // namespace zenith
