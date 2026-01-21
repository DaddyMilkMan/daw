/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.cpp
    Lock-free observability and metrics collection implementation.
  ==============================================================================
*/

#include "ObservabilityAgent.h"

namespace zenith {
namespace agents {

ObservabilityAgent::ObservabilityAgent() : eventBuffer_(4096)
{
}

ObservabilityAgent::~ObservabilityAgent() {}

void ObservabilityAgent::pushToFifo(RawMetricEvent::Type type, const char* name, double value)
{
    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }

    // Single-Producer (Audio Thread) safe
    int start1, size1, start2, size2;
    fifo_.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0) {
        eventBuffer_[start1] = { type, name, value, 0 }; // Timestamp not strictly needed for this pass
    } else if (size2 > 0) {
        eventBuffer_[start2] = { type, name, value, 0 };
    }

    fifo_.finishedWrite(size1 + size2);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

void ObservabilityAgent::recordCounter(const char* name, int increment)
{
    pushToFifo(RawMetricEvent::Type::Counter, name, static_cast<double>(increment));
}

void ObservabilityAgent::recordGauge(const char* name, double value)
{
    pushToFifo(RawMetricEvent::Type::Gauge, name, value);
}

void ObservabilityAgent::startTimer(const char* name)
{
    // TODO: Implement RT-safe timer start (requires token/handle return)
    // For now, no-op or unsafe map implementation would go here.
}

void ObservabilityAgent::endTimer(const char* name)
{
    // TODO: Calculate duration
    // pushToFifo(RawMetricEvent::Type::Timer, name, durationSec);
}

std::vector<ObservabilityAgent::Metric> ObservabilityAgent::getMetrics()
{
    std::vector<Metric> results;

    int start1, size1, start2, size2;
    int numReady = fifo_.getNumReady();

    if (numReady == 0)
        return results;

    // Consumer side
    fifo_.prepareToRead(numReady, start1, size1, start2, size2);

    auto convertAndAdd = [&](int index) {
        const auto& raw = eventBuffer_[index];
        Metric m;
        m.name = raw.name; // const char* to std::string
        m.value = raw.value;

        switch (raw.type) {
            case RawMetricEvent::Type::Counter: m.type = Metric::Type::Counter; break;
            case RawMetricEvent::Type::Gauge:   m.type = Metric::Type::Gauge; break;
            case RawMetricEvent::Type::Timer:   m.type = Metric::Type::Timer; break;
        }
        results.push_back(m);
    };

    if (size1 > 0) {
        for (int i = 0; i < size1; ++i) convertAndAdd(start1 + i);
    }
    if (size2 > 0) {
        for (int i = 0; i < size2; ++i) convertAndAdd(start2 + i);
    }

    fifo_.finishedRead(size1 + size2);
    return results;
}

void ObservabilityAgent::setEnabled(bool enabled) {
  enabled_.store(enabled, std::memory_order_release);
}

void ObservabilityAgent::setExportInterval(std::chrono::milliseconds interval) {
  // TODO: Configure periodic metrics export
}

void ObservabilityAgent::clearMetrics() {
    fifo_.reset();
    metricsCollected_.store(0, std::memory_order_release);
}

void ObservabilityAgent::log(LogLevel level, const juce::String& message) {
    // Placeholder logging
    DBG("[" << static_cast<int>(level) << "] " << message);
}

void ObservabilityAgent::logStructured(LogLevel level, const juce::String& message, const juce::var& data) {
    log(level, message);
}

} // namespace agents
} // namespace zenith
