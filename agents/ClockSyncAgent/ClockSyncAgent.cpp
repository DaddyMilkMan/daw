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

    // Check for tempo jumps / discontinuity if we have enough history
    if (historyCount_ > 10) {
      // Predict expected arrival using last 2 points (crude linear extrapolation for jump detection)
      // Note: A full regression prediction would be better but this is cheaper for a quick check.
      // We'll use the regression values if available, but for now let's just use the last interval.

      // Accessing historyBuffer safely - since this is the only writer thread for historyBuffer_,
      // we can read it.
      size_t lastIdx = (historyIdx_ == 0) ? (kMidiHistorySize - 1) : (historyIdx_ - 1);
      size_t prevIdx = (lastIdx == 0) ? (kMidiHistorySize - 1) : (lastIdx - 1);

      int64_t lastTime = historyBuffer_[lastIdx].timeNs;
      int64_t prevTime = historyBuffer_[prevIdx].timeNs;
      int64_t interval = lastTime - prevTime;

      int64_t expectedNs = lastTime + interval;
      int64_t errorNs = std::abs(nowNs - expectedNs);

      // Threshold: 20ms (approx 1 beat at 3000 BPM, or huge jitter)
      // At 120 BPM, 1 tick = ~20.8ms. So 20ms error is basically missing a tick or double speed.
      if (errorNs > 20000000) {
        historyCount_ = 0; // Reset history
        // Don't reset midiTickCounter_ here as we might just be catching up,
        // unless it's a huge jump which the regression will handle by resetting slope.
      }
    }

    // Update History
    historyBuffer_[historyIdx_] = { midiTickCounter_, nowNs };
    historyIdx_ = (historyIdx_ + 1) % kMidiHistorySize;
    if (historyCount_ < kMidiHistorySize) {
      historyCount_++;
    }

    midiTickCounter_++;
    updateMidiRegression();
  }
  // Start (0xFA)
  else if (status == 0xFA) {
    isMidiRunning_.store(true, std::memory_order_release);
    midiTickCounter_ = 0;
    historyCount_ = 0;
    historyIdx_ = 0;
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
    midiTickCounter_ = songPositionBeats * 6;
    historyCount_ = 0; // Reset regression history as we jumped time
    historyIdx_ = 0;
  }
}

void ClockSyncAgent::updateMidiRegression() {
  if (historyCount_ < 2) return;

  // Linear Regression: Time = m * Tick + c
  // We want to find m (ns per tick) and c (offset at tick 0)

  double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
  int64_t n = static_cast<int64_t>(historyCount_);

  // To avoid floating point issues with large Tick/Time values,
  // we normalize X relative to the oldest point in the buffer.
  // We still use absolute Y (Time) but calculating slope is safer with relative X.

  // Find the start index in the circular buffer
  size_t startIdx = (historyIdx_ + kMidiHistorySize - historyCount_) % kMidiHistorySize;
  int64_t baseTick = historyBuffer_[startIdx].tick;

  for (size_t i = 0; i < historyCount_; ++i) {
    size_t idx = (startIdx + i) % kMidiHistorySize;
    double x = static_cast<double>(historyBuffer_[idx].tick - baseTick);
    double y = static_cast<double>(historyBuffer_[idx].timeNs);

    sumX += x;
    sumY += y;
    sumXY += x * y;
    sumX2 += x * x;
  }

  double m = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
  double c_rel = (sumY - m * sumX) / n; // Intercept relative to baseTick

  // "c" in absolute terms (though we don't strictly need it if we calculate offset directly)
  // double c_abs = c_rel - m * baseTick;

  // Calculate current predicted MIDI time for the latest tick
  // Latest tick is (midiTickCounter_ - 1) because we incremented it after adding the point
  int64_t latestTick = midiTickCounter_ - 1;
  double predictedNowNs = m * (latestTick - baseTick) + c_rel;

  // Update Atomics

  // 1. Drift Compensation
  // Nominal tick at 120 BPM is ~20.8ms (20833333ns).
  // This is purely informational for the UI unless we want to normalize time.
  // However, ClockSyncAgent uses driftCompensation_ to scale the LOCAL clock.
  // If we want getCurrentTime() to return MIDI time, we have two options:
  // A. Warping: drift = 1.0. Offset = MIDI_Time - Local_Time.
  // B. Scaling: drift = MIDI_Rate / Local_Rate.

  // The architecture seems to support "drift" as a multiplier on local monotonic time.
  // If local clock is perfect, m should be exactly the expected tick duration.
  // But MIDI tempo varies. "Drift" usually implies error relative to a fixed standard (like 48kHz).
  // Here, MIDI *defines* the time.

  // Strategy:
  // We want getCurrentTime() == predictedNowNs (at this moment).
  // getCurrentTime() = (LocalNow + Offset) * Drift
  // Let's simplify: Set Drift = 1.0 (unless we are compensating for sample rate mismatch)
  // and put all correction into Offset.
  // Offset = PredictedMidiTime - LocalNow

  // Wait, if we leave Drift=1.0, getCurrentTime will advance at LocalClock speed.
  // If MIDI is 140 BPM, it advances faster. We need Drift to match the tempo IF
  // getCurrentTime is supposed to return "Song Time" (musical time).
  // BUT: getCurrentTime() returns nanoseconds. Nanoseconds are absolute.
  // If MIDI clock runs fast (e.g. sender's crystal is +1%), then "1 second of MIDI time"
  // takes 0.99 seconds of wall clock.
  // So yes, we should probably just sync the "phase" (Offset) and let the "drift"
  // be the ratio between MIDI rate and Local rate?
  // Actually, usually Sync means aligning the timelines.
  // If TimeSource::MIDIClock is selected, getCurrentTime() returns the time on the MIDI master's clock.
  // If the MIDI master is perfect, m should be stable.

  // For now, let's stick to the Plan:
  // Formula: T_MIDI = m * currentTick + c
  // Offset = T_MIDI - T_Local
  // If we update Offset continuously, we effectively lock phase.

  // Get the actual local time of the last point
  size_t lastIdx = (historyIdx_ == 0) ? (kMidiHistorySize - 1) : (historyIdx_ - 1);
  int64_t actualNowNs = historyBuffer_[lastIdx].timeNs;

  int64_t offset = static_cast<int64_t>(predictedNowNs) - actualNowNs;

  // Store results
  // We add 'offset' to the CURRENT offset? No, ClockSyncAgent::getCurrentTime adds clockOffsetNs_ to LocalTime.
  // So clockOffsetNs_ should be (TargetTime - LocalTime).
  // TargetTime here is predictedNowNs. LocalTime is actualNowNs.
  // So clockOffsetNs_ = predictedNowNs - actualNowNs.

  // Wait, predictedNowNs is the "smoothed" time for the current tick.
  // actualNowNs is the "raw" local timestamp of the current tick.
  // So offset is the error of the current tick vs the regression line.
  // This is NOT the offset between Local Clock 0 and MIDI Clock 0.
  // It is the "phase correction" to put the current local time onto the regression line.

  // Actually, there's a misunderstanding of what `getCurrentTime()` should return.
  // If it returns a monotonic timestamp that is "synced", it usually means:
  // MasterTime = SlaveTime + Offset.
  // Here, MIDI is Master.
  // We received a tick at 'actualNowNs' (Slave Time).
  // What is the Master Time?
  // MIDI Clock doesn't send absolute timestamps. It sends Ticks.
  // We have to DECIDE what "Time" it is.
  // Usually, we pick a reference (e.g. First Tick = Time 0, or Time = System Time at First Tick).
  // If we want `getCurrentTime` to return something close to `HighResClock::now()` but smoothed to MIDI,
  // then we are doing exactly that.

  clockOffsetNs_.store(static_cast<int64_t>(offset), std::memory_order_release);

  // Update drift for stats (optional, or used for interpolation if we were doing that)
  // For now, keep drift at 1.0 or calculate it if needed.
  // The plan said: driftCompensation_ = nominalTickDurationNs_ / m;
  // But we don't have a fixed nominalTickDurationNs_ (tempo changes).
  // So let's leave drift at 1.0 unless we want to detect sample rate drift.
  driftCompensation_.store(1.0, std::memory_order_release);

  synchronized_.store(true, std::memory_order_release);
}

} // namespace agents
} // namespace zenith
