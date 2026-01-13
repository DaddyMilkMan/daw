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
  
  // TODO: Calculate actual network latency
  status.latencyMs = 0.0;
  
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

} // namespace agents
} // namespace zenith
