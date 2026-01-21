/*
  ==============================================================================
    agents/ClockSyncAgent/ClockSyncAgent.h
    Precision clock synchronization for distributed audio systems.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "protocols/SyncProtocol.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <array>

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

  /// Update network synchronization metrics from protocol timestamps (t1=request sent, t2=request received, t3=response sent, t4=response received)
  void updateNetworkMetrics(Timestamp t1, Timestamp t2, Timestamp t3, Timestamp t4);

  //==============================================================================
  // MIDI Synchronization (RT-safe)

  /// Process incoming MIDI message for clock synchronization
  void processMidiMessage(const juce::MidiMessage& message);

private:
  //==============================================================================
  void updateMidiRegression(int64_t currentTickCounter);
  
  // Helper for circular buffer index calculation
  inline size_t getPreviousBufferIndex(size_t currentIdx, size_t offset = 1) const noexcept {
    return (currentIdx + kMidiHistorySize - offset) % kMidiHistorySize;
  }

  std::atomic<TimeSource> currentSource_{TimeSource::LocalClock};
  std::atomic<bool> synchronized_{false};
  std::atomic<int64_t> clockOffsetNs_{0};
  std::atomic<double> driftCompensation_{1.0};
  std::atomic<bool> driftCompensationEnabled_{true};
  std::atomic<double> latencyMs_{0.0};
  
  std::unique_ptr<protocols::SyncProtocol> syncProtocol_;

  // MIDI Clock State
  struct TickPoint {
    int64_t tick;
    int64_t timeNs;
  };

  static constexpr size_t kMidiHistorySize = 48;
  static constexpr int64_t kTempoJumpThresholdNs = 20'000'000; // 20ms: Max jitter before resetting history
  
  std::array<TickPoint, kMidiHistorySize> historyBuffer_;
  std::atomic<size_t> historyIdx_{0};      // Atomic: written by MIDI thread, may be read by UI
  std::atomic<size_t> historyCount_{0};    // Atomic: written by MIDI thread, may be read by UI
  std::atomic<int64_t> midiTickCounter_{0}; // Atomic: written by MIDI thread, may be read by UI
  std::atomic<bool> isMidiRunning_{false}; // Atomic for thread safety if accessed from UI
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClockSyncAgent)
};

} // namespace agents
} // namespace zenith
