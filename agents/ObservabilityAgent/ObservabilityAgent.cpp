/**
 * @file ObservabilityAgent.cpp
 * @brief Implementation of ObservabilityAgent
 */

#include "ObservabilityAgent.h"

namespace zenith {

ObservabilityAgent::ObservabilityAgent() {
    // Constructor
}

ObservabilityAgent::~ObservabilityAgent() {
    // Destructor
}

void ObservabilityAgent::initialize() {
    initialized_.store(true);
    cpuUsage_.store(0.0);
    memoryUsage_.store(0);
    underrunCount_.store(0);
    
    DBG("ObservabilityAgent initialized");
}

void ObservabilityAgent::recordMetric(
    MetricType type,
    double value,
    std::chrono::microseconds timestamp) 
{
    // AUDIO THREAD SAFE - no allocations, no locks
    
    // Use current time if no timestamp provided
    if (timestamp.count() == 0) {
        // Note: std::chrono::steady_clock is generally RT-safe
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    }
    
    // Update atomic counters based on metric type
    switch (type) {
        case MetricType::CpuUsage:
            cpuUsage_.store(value);
            break;
        case MetricType::MemoryUsage:
            memoryUsage_.store(static_cast<size_t>(value));
            break;
        case MetricType::BufferUnderrun:
            underrunCount_.fetch_add(1);
            break;
        case MetricType::AudioLatency:
            // TODO: Store latency metrics
            break;
        case MetricType::Custom:
            // TODO: Handle custom metrics
            break;
    }
    
    // TODO: Add to lock-free ring buffer for detailed history
}

double ObservabilityAgent::getCpuUsage() const {
    return cpuUsage_.load();
}

size_t ObservabilityAgent::getMemoryUsage() const {
    return memoryUsage_.load();
}

int ObservabilityAgent::getUnderrunCount() const {
    return underrunCount_.load();
}

juce::String ObservabilityAgent::exportMetrics() const {
    // BACKGROUND THREAD SAFE
    
    // TODO: Implement full metrics export in JSON format
    // This will read from the lock-free ring buffer and
    // format all collected metrics
    
    juce::String json;
    json << "{\n";
    json << "  \"cpu_usage\": " << getCpuUsage() << ",\n";
    json << "  \"memory_usage\": " << getMemoryUsage() << ",\n";
    json << "  \"underrun_count\": " << getUnderrunCount() << "\n";
    json << "}";
    
    return json;
}

} // namespace zenith
