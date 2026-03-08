/*
  ==============================================================================

    LatencyCompensator.cpp
    Implementation of automatic plugin delay compensation

  ==============================================================================
*/

#include "LatencyCompensator.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// LatencyCompensator Implementation
//==============================================================================

LatencyCompensator::LatencyCompensator() {
    std::cout << "LatencyCompensator: Initialized" << std::endl;
}

LatencyCompensator::~LatencyCompensator() {
    std::cout << "LatencyCompensator: Shut down ("
              << statistics_.totalPluginsCompensated << " plugins, "
              << statistics_.totalLatencySamples << " samples total)" << std::endl;
}

//==============================================================================
void LatencyCompensator::registerPluginLatency(
    const juce::String& pluginId,
    int latencySamples)
{
    if (!enabled_) {
        return;
    }

    // Validate latency
    if (!isValidLatency(latencySamples)) {
        LatencyCompensationEvent event;
        event.error = LatencyCompensationError::InvalidLatencyValue;
        event.pluginId = pluginId;
        event.description = "Invalid latency: " + juce::String(latencySamples) + " samples";
        event.reportedLatency = latencySamples;
        event.severity = 8.0;

        reportError(event);
        return;
    }

    // Check if plugin already registered
    auto it = pluginLatencies_.find(pluginId);
    if (it != pluginLatencies_.end()) {
        // Update existing
        it->second.latencySamples = latencySamples;
        it->second.compensatedSamples = latencySamples;
        it->second.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
    } else {
        // Add new
        PluginLatencyInfo info;
        info.pluginId = pluginId;
        info.latencySamples = latencySamples;
        info.compensatedSamples = latencySamples;
        info.isValid = true;
        info.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;

        pluginLatencies_[pluginId] = info;
        statistics_.totalPluginsCompensated++;
    }

    updateStatistics();
}

//==============================================================================
void LatencyCompensator::updatePluginLatency(
    const juce::String& pluginId,
    int latencySamples)
{
    // Same as register
    registerPluginLatency(pluginId, latencySamples);
}

//==============================================================================
void LatencyCompensator::removePlugin(const juce::String& pluginId) {
    auto it = pluginLatencies_.find(pluginId);
    if (it != pluginLatencies_.end()) {
        pluginLatencies_.erase(it);
        statistics_.totalPluginsCompensated--;
        updateStatistics();
    }
}

//==============================================================================
int LatencyCompensator::getTotalLatency() const {
    int total = 0;
    for (const auto& pair : pluginLatencies_) {
        total += pair.second.compensatedSamples;
    }
    return total;
}

//==============================================================================
int LatencyCompensator::getPluginCompensation(const juce::String& pluginId) const {
    auto it = pluginLatencies_.find(pluginId);
    if (it != pluginLatencies_.end()) {
        return it->second.compensatedSamples;
    }
    return 0;
}

//==============================================================================
std::vector<PluginLatencyInfo> LatencyCompensator::getAllPluginLatencies() const {
    std::vector<PluginLatencyInfo> result;
    for (const auto& pair : pluginLatencies_) {
        result.push_back(pair.second);
    }
    return result;
}

//==============================================================================
bool LatencyCompensator::isValidLatency(int latencySamples) const {
    // Latency must be non-negative and reasonable
    // Most plugins have latency < 100,000 samples (~2 seconds at 48kHz)
    return latencySamples >= 0 && latencySamples < 100000;
}

//==============================================================================
void LatencyCompensator::resetStatistics() {
    statistics_.totalPluginsCompensated = static_cast<int>(pluginLatencies_.size());
    statistics_.totalLatencySamples = getTotalLatency();
    statistics_.compensationErrors = 0;
    statistics_.averageLatency = 0.0;

    // Calculate average
    if (!pluginLatencies_.empty()) {
        double sum = 0.0;
        for (const auto& pair : pluginLatencies_) {
            sum += pair.second.compensatedSamples;
        }
        statistics_.averageLatency = sum / pluginLatencies_.size();
    }

    std::cout << "LatencyCompensator: Statistics reset" << std::endl;
}

//==============================================================================
void LatencyCompensator::clear() {
    pluginLatencies_.clear();
    resetStatistics();
    std::cout << "LatencyCompensator: Cleared all plugin latencies" << std::endl;
}

//==============================================================================
// Private Methods
//==============================================================================

void LatencyCompensator::reportError(const LatencyCompensationEvent& event) {
    statistics_.compensationErrors++;

    if (eventCallback_) {
        eventCallback_(event);
    }

    if (event.severity >= 7.0) {
        std::cerr << "LatencyCompensator: " << event.toString() << std::endl;
    }
}

void LatencyCompensator::updateStatistics() {
    statistics_.totalLatencySamples = getTotalLatency();

    if (!pluginLatencies_.empty()) {
        double sum = 0.0;
        for (const auto& pair : pluginLatencies_) {
            sum += pair.second.compensatedSamples;
        }
        statistics_.averageLatency = sum / pluginLatencies_.size();
    } else {
        statistics_.averageLatency = 0.0;
    }
}

} // namespace zenith
