/**
 * @file ObservabilityAgent.cpp
 * @brief Implementation of ObservabilityAgent
 */

#include "ObservabilityAgent.h"
#include <sstream>
#include <iomanip>
#include <random>

namespace zenith {

ObservabilityAgent::ObservabilityAgent() {
}

ObservabilityAgent::~ObservabilityAgent() {
}

void ObservabilityAgent::initialize() {
    // MESSAGE THREAD ONLY
    
    // Initialize metrics
    audioLoad_.store(0.0);
    cpuUsage_.store(0.0);
    bufferUnderrunCount_.store(0);
    totalCallbacks_.store(0);
    
    // Clear any existing data
    metrics_.clear();
    completedSpans_.clear();
    activeSpans_.clear();
}

void ObservabilityAgent::recordMetric(const char* name, double value, MetricType type) {
    // AUDIO THREAD SAFE for common metrics
    // Uses atomic operations for frequently updated metrics
    
    // Fast path for common metrics using atomics
    if (std::string(name) == "audio_load") {
        audioLoad_.store(value);
        return;
    }
    if (std::string(name) == "cpu_usage") {
        cpuUsage_.store(value);
        return;
    }
    if (std::string(name) == "buffer_underruns" && type == MetricType::Counter) {
        bufferUnderrunCount_.fetch_add(static_cast<uint64_t>(value));
        return;
    }
    if (std::string(name) == "total_callbacks" && type == MetricType::Counter) {
        totalCallbacks_.fetch_add(1);
        return;
    }
    
    // TODO: For other metrics, use a lock-free queue to post to message thread
    // This is a placeholder - production code should use juce::AbstractFifo
}

uint64_t ObservabilityAgent::startSpan(const char* spanName) {
    // AUDIO THREAD SAFE - RT-safe operations only
    
    uint64_t spanId = nextSpanId_.fetch_add(1);
    uint64_t timestamp = getTimestampMicros();
    
    // TODO: In production, use lock-free queue to communicate with message thread
    // For now, this is a placeholder that demonstrates the interface
    
    return spanId;
}

void ObservabilityAgent::endSpan(uint64_t spanId) {
    // AUDIO THREAD SAFE - RT-safe operations only
    
    uint64_t timestamp = getTimestampMicros();
    
    // TODO: In production, use lock-free queue to post span completion
    // The message thread would then move the span to completedSpans_
}

double ObservabilityAgent::getMetric(const std::string& name) const {
    // Thread-safe for atomic metrics
    
    if (name == "audio_load") {
        return audioLoad_.load();
    }
    if (name == "cpu_usage") {
        return cpuUsage_.load();
    }
    if (name == "buffer_underruns") {
        return static_cast<double>(bufferUnderrunCount_.load());
    }
    if (name == "total_callbacks") {
        return static_cast<double>(totalCallbacks_.load());
    }
    
    // For other metrics, would need to check metrics_ map (message thread only)
    return 0.0;
}

std::string ObservabilityAgent::exportPrometheusMetrics() {
    // MESSAGE THREAD ONLY - can allocate and format strings
    
    std::ostringstream output;
    
    // Export atomic metrics
    output << "# HELP zenith_audio_load Current audio processing load percentage\n";
    output << "# TYPE zenith_audio_load gauge\n";
    output << "zenith_audio_load " << audioLoad_.load() << "\n\n";
    
    output << "# HELP zenith_cpu_usage CPU usage percentage\n";
    output << "# TYPE zenith_cpu_usage gauge\n";
    output << "zenith_cpu_usage " << cpuUsage_.load() << "\n\n";
    
    output << "# HELP zenith_buffer_underruns_total Total buffer underruns\n";
    output << "# TYPE zenith_buffer_underruns_total counter\n";
    output << "zenith_buffer_underruns_total " << bufferUnderrunCount_.load() << "\n\n";
    
    output << "# HELP zenith_audio_callbacks_total Total audio callbacks processed\n";
    output << "# TYPE zenith_audio_callbacks_total counter\n";
    output << "zenith_audio_callbacks_total " << totalCallbacks_.load() << "\n\n";
    
    // TODO: Export other metrics from metrics_ map
    
    return output.str();
}

std::vector<TraceSpan> ObservabilityAgent::exportTraceSpans() {
    // MESSAGE THREAD ONLY
    
    // Return completed spans and clear the list
    std::vector<TraceSpan> spans = completedSpans_;
    completedSpans_.clear();
    
    return spans;
}

std::string ObservabilityAgent::generateReport() {
    // MESSAGE THREAD ONLY - can allocate and format strings
    
    std::ostringstream report;
    report << std::fixed << std::setprecision(2);
    
    report << "=== Observability Agent Report ===\n\n";
    
    // Current metrics
    report << "Current Metrics:\n";
    report << "  Audio Load: " << audioLoad_.load() << "%\n";
    report << "  CPU Usage: " << cpuUsage_.load() << "%\n";
    report << "  Buffer Underruns: " << bufferUnderrunCount_.load() << "\n";
    report << "  Total Callbacks: " << totalCallbacks_.load() << "\n";
    
    // Trace spans
    report << "\nActive Trace Spans: " << activeSpans_.size() << "\n";
    report << "Completed Trace Spans: " << completedSpans_.size() << "\n";
    
    if (!completedSpans_.empty()) {
        report << "\nRecent Spans:\n";
        size_t count = 0;
        for (auto it = completedSpans_.rbegin(); 
             it != completedSpans_.rend() && count < 5; 
             ++it, ++count) {
            double durationMs = (it->endTimestamp - it->startTimestamp) / 1000.0;
            report << "  - " << it->name << ": " << durationMs << " ms\n";
        }
    }
    
    // Health check
    double load = audioLoad_.load();
    uint64_t underruns = bufferUnderrunCount_.load();
    
    report << "\nHealth Status:\n";
    if (load < 50.0 && underruns == 0) {
        report << "  ✅ System is healthy\n";
    } else if (load < 80.0 && underruns < 10) {
        report << "  ⚠️  System is stressed but operational\n";
    } else {
        report << "  ❌ System is experiencing issues\n";
    }
    
    return report.str();
}

uint64_t ObservabilityAgent::getTimestampMicros() const {
    // RT-safe: just reading the clock
    auto now = std::chrono::high_resolution_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    );
    return static_cast<uint64_t>(micros.count());
}

std::string ObservabilityAgent::generateTraceId() const {
    // MESSAGE THREAD ONLY - generates random trace ID
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << gen();
    return ss.str();
}

} // namespace zenith
