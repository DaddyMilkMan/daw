/*
  ==============================================================================
    agents/ClockSyncAgent/ClockSyncAgent.cpp
    Precision clock synchronization implementation.
  ==============================================================================
*/

#include "ClockSyncAgent.h"
#include "protocols/NTPProtocol.h"
#include "protocols/PTPProtocol.h"
#include <numeric>

namespace zenith {
namespace agents {

//==============================================================================
ClockSyncAgent::ClockSyncAgent() {
  // Initialize with local clock as default source
  synchronized_.store(true, std::memory_order_release);
}

ClockSyncAgent::~ClockSyncAgent() {
  // Cleanup sync resources
}

//==============================================================================
// Time Query (RT-safe)

ClockSyncAgent::Timestamp ClockSyncAgent::getCurrentTime() const noexcept {
  // Get high-resolution local time
  auto now = HighResClock::now();
  auto nanos = std::chrono::duration_cast<Timestamp>(now.time_since_epoch());
  
  // Apply clock offset if synchronized to external source
  auto offset = clockOffsetNs_.load(std::memory_order_acquire);
  nanos += Timestamp(offset);
  
  // Apply drift compensation
  auto drift = driftCompensation_.load(std::memory_order_acquire);
  if (driftCompensationEnabled_.load(std::memory_order_acquire)) {
    nanos = Timestamp(static_cast<int64_t>(nanos.count() * drift));
  }
  
  return nanos;
}

ClockSyncAgent::Timestamp ClockSyncAgent::samplesToTimestamp(
    int64_t samplePosition, double sampleRate) const noexcept {
  jassert(sampleRate > 0.0);
  
  // Convert samples to nanoseconds
  const double nanosPerSample = 1.0e9 / sampleRate;
  const int64_t nanos = static_cast<int64_t>(samplePosition * nanosPerSample);
  
  return Timestamp(nanos);
}

int64_t ClockSyncAgent::timestampToSamples(
    Timestamp timestamp, double sampleRate) const noexcept {
  jassert(sampleRate > 0.0);
  
  // Convert nanoseconds to samples
  const double samplesPerNano = sampleRate / 1.0e9;
  const int64_t samples = static_cast<int64_t>(timestamp.count() * samplesPerNano);
  
  return samples;
}

//==============================================================================
// Synchronization Control

void ClockSyncAgent::setTimeSource(TimeSource source) {
  currentSource_.store(source, std::memory_order_release);
  
  if (syncProtocol_) {
    syncProtocol_->stop();
    syncProtocol_.reset();
  }

  if (source == TimeSource::NetworkNTP) {
    syncProtocol_ = std::make_unique<protocols::NTPProtocol>();
  } else if (source == TimeSource::NetworkPTP) {
    syncProtocol_ = std::make_unique<protocols::PTPProtocol>();
  }
  
  if (syncProtocol_) {
    syncProtocol_->onOffsetChanged = [this](int64_t offset) {
      clockOffsetNs_.store(offset, std::memory_order_release);
      if (syncProtocol_) {
        driftCompensation_.store(syncProtocol_->getDrift(), std::memory_order_release);
      }
    };
    syncProtocol_->onSyncStateChanged = [this](bool sync) {
      synchronized_.store(sync, std::memory_order_release);
    };

    syncProtocol_->start();
    synchronized_.store(false, std::memory_order_release);
  } else if (source == TimeSource::LocalClock) {
    synchronized_.store(true, std::memory_order_release);
    clockOffsetNs_.store(0, std::memory_order_release);
    driftCompensation_.store(1.0, std::memory_order_release);
  } else if (source == TimeSource::MIDIClock) {
    // MIDI Clock: Reset synchronization state
    synchronized_.store(false, std::memory_order_release);
    clockOffsetNs_.store(0, std::memory_order_release);
    driftCompensation_.store(1.0, std::memory_order_release);
    midiTickCounter_.store(0, std::memory_order_release);
    historyCount_.store(0, std::memory_order_release);
    historyIdx_.store(0, std::memory_order_release);
    isMidiRunning_.store(false, std::memory_order_release);
  } else {
    synchronized_.store(false, std::memory_order_release);
  }
}

ClockSyncAgent::SyncStatus ClockSyncAgent::getSyncStatus() const {
  SyncStatus status;
  status.currentSource = currentSource_.load(std::memory_order_acquire);
  status.synchronized = synchronized_.load(std::memory_order_acquire);
  status.offsetNanoseconds = clockOffsetNs_.load(std::memory_order_acquire);
  
  // Calculate drift in PPM
  auto drift = driftCompensation_.load(std::memory_order_acquire);
  status.driftPPM = (drift - 1.0) * 1.0e6;
  
  // Calculate actual network latency
  status.latencyMs = latencyMs_.load(std::memory_order_acquire);
  
  return status;
}

void ClockSyncAgent::resynchronize() {
  auto source = currentSource_.load(std::memory_order_acquire);

  if (source == TimeSource::LocalClock) {
    clockOffsetNs_.store(0, std::memory_order_release);
    driftCompensation_.store(1.0, std::memory_order_release);
    synchronized_.store(true, std::memory_order_release);
  }
  else if (source == TimeSource::MIDIClock) {
    // Reset regression history to force fresh calculation on next ticks
    synchronized_.store(false, std::memory_order_release);
    clockOffsetNs_.store(0, std::memory_order_release);
    driftCompensation_.store(1.0, std::memory_order_release);

    // Clear history but preserve tick counter and running state
    historyCount_.store(0, std::memory_order_release);
    historyIdx_.store(0, std::memory_order_release);
  }
  else if (source == TimeSource::NetworkNTP || source == TimeSource::NetworkPTP) {
    if (syncProtocol_) {
      syncProtocol_->forceSync();
      // Update local state in case protocol has newer values already
      clockOffsetNs_.store(syncProtocol_->getOffset(), std::memory_order_release);
      driftCompensation_.store(syncProtocol_->getDrift(), std::memory_order_release);
    }
  }
}

void ClockSyncAgent::setDriftCompensationEnabled(bool enabled) {
  driftCompensationEnabled_.store(enabled, std::memory_order_release);
}

void ClockSyncAgent::updateNetworkMetrics(Timestamp t1, Timestamp t2, Timestamp t3, Timestamp t4) {
  // Calculate Round-Trip Time (RTT) and Clock Offset using NTP algorithm
  // RTT = (T4 - T1) - (T3 - T2)
  // Offset = ((T2 - T1) + (T3 - T4)) / 2

  auto t1_n = t1.count();
  auto t2_n = t2.count();
  auto t3_n = t3.count();
  auto t4_n = t4.count();

  int64_t rttNs = (t4_n - t1_n) - (t3_n - t2_n);
  // Clamp negative RTT (should not happen with monotonic clocks but possible with system adjustments)
  if (rttNs < 0) rttNs = 0;

  int64_t offsetNs = ((t2_n - t1_n) + (t3_n - t4_n)) / 2;

  // Update atomic state
  clockOffsetNs_.store(offsetNs, std::memory_order_release);

  // Store one-way latency in milliseconds (RTT / 2)
  double latencyMs = static_cast<double>(rttNs) / 2.0 / 1.0e6;
  latencyMs_.store(latencyMs, std::memory_order_release);

  // If we have valid metrics, we are synchronized
  synchronized_.store(true, std::memory_order_release);
}

//==============================================================================
// MIDI Synchronization (RT-safe)

void ClockSyncAgent::processMidiMessage(const juce::MidiMessage& message) {
  const auto* data = message.getRawData();
  if (message.getRawDataSize() < 1) return;

  const uint8_t status = data[0];

  // MIDI Clock (0xF8)
  if (status == 0xF8) {
    auto now = HighResClock::now();
    auto nowNs = std::chrono::duration_cast<Timestamp>(now.time_since_epoch()).count();

    // Load current history state atomically
    size_t currentHistoryCount = historyCount_.load(std::memory_order_acquire);
    size_t currentHistoryIdx = historyIdx_.load(std::memory_order_acquire);
    
    // Check for tempo jumps / discontinuity if we have enough history
    if (currentHistoryCount > 10) {
      // Predict expected arrival using last 2 points (crude linear extrapolation for jump detection)
      size_t lastIdx = getPreviousBufferIndex(currentHistoryIdx, 1);
      size_t prevIdx = getPreviousBufferIndex(currentHistoryIdx, 2);

      int64_t lastTime = historyBuffer_[lastIdx].timeNs;
      int64_t prevTime = historyBuffer_[prevIdx].timeNs;
      int64_t interval = lastTime - prevTime;

      int64_t expectedNs = lastTime + interval;
      int64_t errorNs = std::abs(nowNs - expectedNs);

      // Use named constant for threshold
      if (errorNs > kTempoJumpThresholdNs) {
        historyCount_.store(0, std::memory_order_release); // Reset history
      }
    }

    // Update History - load current tick counter atomically
    int64_t currentTickCounter = midiTickCounter_.load(std::memory_order_acquire);
    
    // Write to history buffer (this is safe because only MIDI thread writes)
    historyBuffer_[currentHistoryIdx] = { currentTickCounter, nowNs };
    
    // Update index and count atomically for UI thread readers
    size_t nextIdx = (currentHistoryIdx + 1) % kMidiHistorySize;
    historyIdx_.store(nextIdx, std::memory_order_release);
    
    if (currentHistoryCount < kMidiHistorySize) {
      historyCount_.store(currentHistoryCount + 1, std::memory_order_release);
    }

    // Increment and store the new tick counter
    int64_t newTickCounter = currentTickCounter + 1;
    midiTickCounter_.store(newTickCounter, std::memory_order_release);
    
    // Pass the new tick counter to regression for consistent calculation
    updateMidiRegression(newTickCounter);
  }
  // Start (0xFA)
  else if (status == 0xFA) {
    isMidiRunning_.store(true, std::memory_order_release);
    midiTickCounter_.store(0, std::memory_order_release);
    historyCount_.store(0, std::memory_order_release);
    historyIdx_.store(0, std::memory_order_release);
  }
  // Continue (0xFB)
  else if (status == 0xFB) {
    isMidiRunning_.store(true, std::memory_order_release);
    // Don't reset counters, just resume
  }
  // Stop (0xFC)
  else if (status == 0xFC) {
    isMidiRunning_.store(false, std::memory_order_release);
  }
  // Song Position Pointer (0xF2)
  else if (status == 0xF2 && message.getRawDataSize() == 3) {
    int positionLsb = data[1];
    int positionMsb = data[2];
    int songPositionBeats = (positionMsb << 7) | positionLsb;

    // SPP is in 16th notes (6 clocks per 16th note)
    midiTickCounter_.store(songPositionBeats * 6, std::memory_order_release);
    historyCount_.store(0, std::memory_order_release); // Reset regression history as we jumped time
    historyIdx_.store(0, std::memory_order_release);
  }
}

void ClockSyncAgent::updateMidiRegression(int64_t currentTickCounter) {
  // Load history state atomically for thread-safe reading
  size_t currentHistoryCount = historyCount_.load(std::memory_order_acquire);
  
  if (currentHistoryCount < 2) return;

  // Linear Regression: Time = m * Tick + c
  // Goal: Find m (nanoseconds per tick) and c (time offset at tick 0)
  // This smooths out MIDI transmission jitter by fitting a line through recent tick timestamps.
  //
  // Memory Ordering Note:
  // - historyCount_/historyIdx_ are atomic and loaded with acquire semantics
  // - historyBuffer_ is only written by MIDI thread, so reads are safe from same thread
  // - clockOffsetNs_/driftCompensation_ are stored with release semantics for cross-thread visibility

  double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
  int64_t n = static_cast<int64_t>(currentHistoryCount);

  // To avoid floating point precision issues with large tick/time values,
  // normalize X (tick) relative to the oldest point in the circular buffer.
  size_t currentHistoryIdx = historyIdx_.load(std::memory_order_acquire);
  size_t startIdx = getPreviousBufferIndex(currentHistoryIdx, currentHistoryCount);
  int64_t baseTick = historyBuffer_[startIdx].tick;

  // Accumulate regression sums
  for (size_t i = 0; i < currentHistoryCount; ++i) {
    size_t idx = (startIdx + i) % kMidiHistorySize;
    double x = static_cast<double>(historyBuffer_[idx].tick - baseTick);
    double y = static_cast<double>(historyBuffer_[idx].timeNs);

    sumX += x;
    sumY += y;
    sumXY += x * y;
    sumX2 += x * x;
  }

  // Calculate slope (m) and intercept (c) using least squares formula
  // m = (n*ΣXY - ΣX*ΣY) / (n*ΣX² - (ΣX)²)
  // c = (ΣY - m*ΣX) / n
  double m = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
  double c_rel = (sumY - m * sumX) / n; // Intercept relative to baseTick

  // Calculate predicted MIDI time for the most recent tick
  // Use the tick counter passed as parameter for consistency (it was incremented after adding the point)
  int64_t latestTick = currentTickCounter - 1;
  double predictedNowNs = m * (latestTick - baseTick) + c_rel;

  // Calculate clock offset for synchronization
  // The offset represents how much to adjust local time to match MIDI time.
  // Formula: Offset = PredictedMidiTime - ActualLocalTime
  //
  // When getCurrentTime() applies this offset:
  // SyncedTime = LocalTime + Offset = LocalTime + (MidiTime - LocalTime) = MidiTime
  //
  // This ensures getCurrentTime() returns a smoothed MIDI timeline value.
  
  size_t lastIdx = getPreviousBufferIndex(currentHistoryIdx, 1);
  int64_t actualNowNs = historyBuffer_[lastIdx].timeNs;
  int64_t offset = static_cast<int64_t>(predictedNowNs) - actualNowNs;

  // Update atomic state with release semantics for visibility to other threads
  // (e.g., audio thread calling getCurrentTime() or UI thread calling getSyncStatus())
  clockOffsetNs_.store(offset, std::memory_order_release);

  // Drift compensation is kept at 1.0 for MIDI Clock
  // Unlike network protocols (PTP/NTP) which measure crystal drift, MIDI Clock
  // defines the tempo, so there's no "drift" to compensate. The offset alone
  // synchronizes us to the MIDI master's timeline.
  driftCompensation_.store(1.0, std::memory_order_release);

  synchronized_.store(true, std::memory_order_release);
}

} // namespace agents
} // namespace zenith