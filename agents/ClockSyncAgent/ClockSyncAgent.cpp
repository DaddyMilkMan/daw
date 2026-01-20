/*
  ==============================================================================
    agents/ClockSyncAgent/ClockSyncAgent.cpp
    Precision clock synchronization implementation.
  ==============================================================================
*/

#include "ClockSyncAgent.h"

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
  
  // TODO: Initialize appropriate sync protocol
  // TODO: Start synchronization process
  
  if (source == TimeSource::LocalClock) {
    synchronized_.store(true, std::memory_order_release);
    clockOffsetNs_.store(0, std::memory_order_release);
    driftCompensation_.store(1.0, std::memory_order_release);
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
  // TODO: Force synchronization update based on current source
  // TODO: Measure clock offset
  // TODO: Update drift compensation
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

} // namespace agents
} // namespace zenith
