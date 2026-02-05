/*
  ==============================================================================
    agents/ScheduledTransportAgent/ScheduledTransportAgent.cpp
    Timeline and transport management implementation.
  ==============================================================================
*/

#include "ScheduledTransportAgent.h"

namespace zenith {
namespace agents {

//==============================================================================
ScheduledTransportAgent::ScheduledTransportAgent() {
  // Initialize with default transport state
}

ScheduledTransportAgent::~ScheduledTransportAgent() {
  // Ensure transport is stopped
  stop();
}

//==============================================================================
// Transport Control

void ScheduledTransportAgent::addListener(Listener* listener) {
  listeners_.add(listener);
}

void ScheduledTransportAgent::removeListener(Listener* listener) {
  listeners_.remove(listener);
}

void ScheduledTransportAgent::play() {
  state_.store(TransportState::Playing, std::memory_order_release);
  listeners_.call(&Listener::transportStateChanged, TransportState::Playing);
}

void ScheduledTransportAgent::stop() {
  state_.store(TransportState::Stopped, std::memory_order_release);
  listeners_.call(&Listener::transportStateChanged, TransportState::Stopped);
  // TODO: Clear pending scheduled events
}

void ScheduledTransportAgent::record() {
  state_.store(TransportState::Recording, std::memory_order_release);
  listeners_.call(&Listener::transportStateChanged, TransportState::Recording);
  // TODO: Enable recording mode
}

void ScheduledTransportAgent::pause() {
  state_.store(TransportState::Paused, std::memory_order_release);
  listeners_.call(&Listener::transportStateChanged, TransportState::Paused);
}

void ScheduledTransportAgent::rewind() {
  samplePosition_.store(0, std::memory_order_release);
}

void ScheduledTransportAgent::setPosition(int64_t samplePosition) {
  jassert(samplePosition >= 0);
  samplePosition_.store(samplePosition, std::memory_order_release);
}

//==============================================================================
// Configuration

void ScheduledTransportAgent::setTempo(double tempo) {
  jassert(tempo > 0.0 && tempo < 999.0);
  tempo_.store(tempo, std::memory_order_release);
}

void ScheduledTransportAgent::setTimeSignature(int numerator, int denominator) {
  jassert(numerator > 0 && denominator > 0);
  timeSignatureNumerator_.store(numerator, std::memory_order_release);
  timeSignatureDenominator_.store(denominator, std::memory_order_release);
}

void ScheduledTransportAgent::setLoopRegion(int64_t startSample, int64_t endSample) {
  jassert(startSample >= 0 && endSample > startSample);
  loopStart_.store(startSample, std::memory_order_release);
  loopEnd_.store(endSample, std::memory_order_release);
}

void ScheduledTransportAgent::setLoopEnabled(bool enabled) {
  loopEnabled_.store(enabled, std::memory_order_release);
}

//==============================================================================
// Real-time Processing

void ScheduledTransportAgent::advance(int numSamples, double sampleRate) noexcept {
  if (state_.load(std::memory_order_acquire) == TransportState::Stopped) {
    return;
  }
  
  auto currentPos = samplePosition_.load(std::memory_order_acquire);
  currentPos += numSamples;
  
  // TODO: Handle loop wraparound
  // TODO: Process scheduled events in range
  // TODO: Update beat/bar position
  
  samplePosition_.store(currentPos, std::memory_order_release);
}

ScheduledTransportAgent::Position ScheduledTransportAgent::getPosition() const noexcept {
  Position pos;
  pos.samplePosition = samplePosition_.load(std::memory_order_acquire);
  pos.tempo = tempo_.load(std::memory_order_acquire);
  pos.timeSignatureNumerator = timeSignatureNumerator_.load(std::memory_order_acquire);
  pos.timeSignatureDenominator = timeSignatureDenominator_.load(std::memory_order_acquire);
  
  // TODO: Calculate beats, bars, PPQ from sample position
  
  return pos;
}

ScheduledTransportAgent::TransportState ScheduledTransportAgent::getState() const noexcept {
  return state_.load(std::memory_order_acquire);
}

} // namespace agents
} // namespace zenith
