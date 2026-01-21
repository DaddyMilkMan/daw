/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.cpp
    Lock-free observability and metrics collection implementation.
  ==============================================================================
*/

#include "ObservabilityAgent.h"
#include <cstdio>

namespace zenith {
namespace agents {

//==============================================================================
ObservabilityAgent::ObservabilityAgent() : Thread("ObservabilityAgent") {
    // Initialize buffer
    buffer_.resize(BufferItems);
    startThread();
}

ObservabilityAgent::~ObservabilityAgent() {
    signalThreadShouldExit();
    notify();
    stopThread(1000);
}

//==============================================================================
// Metrics Collection (RT-safe)

void ObservabilityAgent::recordCounter(const char* name, double value) noexcept {
    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }

    char message[MessageSize];
    std::snprintf(message, sizeof(message), "COUNTER: %s = %.4f", name, value);
    log(message);

    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

void ObservabilityAgent::recordGauge(const char* name, double value) noexcept {
    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }

    char message[MessageSize];
    std::snprintf(message, sizeof(message), "GAUGE: %s = %.4f", name, value);
    log(message);

    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::startTimer() noexcept {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(now.time_since_epoch().count());
}

void ObservabilityAgent::endTimer(const char* name, uint64_t startTime) noexcept {
    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto endTime = static_cast<uint64_t>(now.time_since_epoch().count());

    // Assuming steady_clock uses nanoseconds (common implementation)
    // Convert to microseconds for readability
    auto durationNs = endTime - startTime;
    double durationUs = static_cast<double>(durationNs) / 1000.0;

    char message[MessageSize];
    std::snprintf(message, sizeof(message), "TIMER: %s = %.2f us", name, durationUs);
    log(message);

    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

//==============================================================================
// Logging (async, non-RT)

void ObservabilityAgent::log(const char* rawMessage) noexcept {
    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }

    int start1, size1, start2, size2;
    fifo_.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0) {
        // Copy to fixed size buffer slot
        // Safe string copy ensuring null termination if message is too long
        strncpy(buffer_[start1].data(), rawMessage, MessageSize - 1);
        buffer_[start1][MessageSize - 1] = '\0';
    }

    // If wrapping occurred (size2 > 0), logic handles it via prepareToWrite providing contiguous blocks.
    // Since we write 1 item, size2 will always be 0.

    fifo_.finishedWrite(size1 + size2);
}

void ObservabilityAgent::log(LogLevel level, const juce::String& message) {
    // Note: This helper might allocate memory (juce::String) so it's not strictly RT-safe
    // if the String wasn't already constructed.
    // However, it queues to the same FIFO, so it must be called from the producer thread (Audio Thread)
    // to avoid corruption, OR the FIFO must be protected (which it isn't).

    // Prepend level
    const char* levelStr = "INFO";
    switch (level) {
        case LogLevel::Debug:    levelStr = "DEBUG"; break;
        case LogLevel::Info:     levelStr = "INFO"; break;
        case LogLevel::Warning:  levelStr = "WARN"; break;
        case LogLevel::Error:    levelStr = "ERROR"; break;
        case LogLevel::Critical: levelStr = "CRIT"; break;
    }

    char buffer[MessageSize];
    std::snprintf(buffer, sizeof(buffer), "[%s] %s", levelStr, message.toRawUTF8());

    log(buffer);
}

void ObservabilityAgent::logStructured(LogLevel level,
                                       const juce::String& message,
                                       const juce::var& data) {
    // Basic implementation: Log message and simple string representation of var
    // In future: Serialize to JSON

    char buffer[MessageSize];
    // Truncate to ensure safety
    std::snprintf(buffer, sizeof(buffer), "%s | Data: %s", message.toRawUTF8(), data.toString().toRawUTF8());

    log(level, buffer);
}

//==============================================================================
// Configuration

void ObservabilityAgent::setEnabled(bool enabled) {
    enabled_.store(enabled, std::memory_order_release);
}

void ObservabilityAgent::setExportInterval(std::chrono::milliseconds interval) {
    // TODO: Configure periodic metrics export
}

std::vector<ObservabilityAgent::Metric> ObservabilityAgent::getMetrics() {
    std::vector<Metric> metrics;
    // TODO: Read from separate metrics storage if needed
    return metrics;
}

void ObservabilityAgent::clearMetrics() {
    metricsCollected_.store(0, std::memory_order_release);
}

//==============================================================================
// Threading

void ObservabilityAgent::run() {
    while (!threadShouldExit()) {
        flushToSink();
        wait(100); // 100ms interval
    }
    // Flush one last time on exit
    flushToSink();
}

void ObservabilityAgent::flushToSink() {
    int start1, size1, start2, size2;
    fifo_.prepareToRead(fifo_.getNumReady(), start1, size1, start2, size2);

    if (size1 > 0) {
        for (int i = 0; i < size1; ++i) {
            DBG(buffer_[start1 + i].data());
        }
    }

    if (size2 > 0) {
        for (int i = 0; i < size2; ++i) {
            DBG(buffer_[start2 + i].data());
        }
    }

    fifo_.finishedRead(size1 + size2);
}

} // namespace agents
} // namespace zenith
