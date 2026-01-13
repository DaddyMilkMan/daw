/**
 * @file RealtimeAudioEngineAgent.cpp
 * @brief Implementation of RealtimeAudioEngineAgent
 */

#include "RealtimeAudioEngineAgent.h"
#include <sstream>
#include <iomanip>

namespace zenith {

RealtimeAudioEngineAgent::RealtimeAudioEngineAgent() {
}

RealtimeAudioEngineAgent::~RealtimeAudioEngineAgent() {
}

void RealtimeAudioEngineAgent::initialize(double sampleRate, int bufferSize) {
    sampleRate_ = sampleRate;
    bufferSize_ = bufferSize;
    
    // Reset metrics
    maxCallbackDuration_.store(0.0);
    underrunCount_.store(0);
    currentLoad_.store(0.0f);
    totalCallbacks_ = 0;
}

void RealtimeAudioEngineAgent::collectMetrics(double callbackDurationUs, bool bufferUnderrun) {
    // RT-safe: only atomic operations, no allocations or locks
    
    // Update max callback duration
    double currentMax = maxCallbackDuration_.load();
    while (callbackDurationUs > currentMax) {
        if (maxCallbackDuration_.compare_exchange_weak(currentMax, callbackDurationUs)) {
            break;
        }
    }
    
    // Increment underrun count if needed
    if (bufferUnderrun) {
        underrunCount_.fetch_add(1);
    }
    
    // Calculate and update load percentage
    double availableTimeUs = (bufferSize_ / sampleRate_) * 1000000.0;
    float load = static_cast<float>((callbackDurationUs / availableTimeUs) * 100.0);
    currentLoad_.store(load);
}

std::string RealtimeAudioEngineAgent::analyzePerformance() {
    // MESSAGE THREAD ONLY - can allocate and format strings
    
    std::ostringstream report;
    report << std::fixed << std::setprecision(2);
    
    report << "=== Realtime Audio Engine Performance ===\n";
    report << "Sample Rate: " << sampleRate_ << " Hz\n";
    report << "Buffer Size: " << bufferSize_ << " samples\n";
    report << "Max Callback Duration: " << maxCallbackDuration_.load() << " μs\n";
    report << "Underrun Count: " << underrunCount_.load() << "\n";
    report << "Current Load: " << currentLoad_.load() << "%\n";
    
    // Generate recommendations
    float load = currentLoad_.load();
    if (load > 80.0f) {
        report << "\n⚠️  WARNING: Audio load exceeds 80%\n";
        report << "Recommendations:\n";
        report << "- Increase buffer size to reduce processing pressure\n";
        report << "- Disable non-essential plugins or effects\n";
        report << "- Check for RT-unsafe operations on audio thread\n";
    } else if (load > 50.0f) {
        report << "\n⚡ Audio load is moderate\n";
    } else {
        report << "\n✅ Audio load is healthy\n";
    }
    
    if (underrunCount_.load() > 0) {
        report << "\n❌ Buffer underruns detected!\n";
        report << "Action required: Investigate audio thread blocking\n";
    }
    
    return report.str();
}

float RealtimeAudioEngineAgent::getAudioLoad() const {
    return currentLoad_.load();
}

} // namespace zenith
