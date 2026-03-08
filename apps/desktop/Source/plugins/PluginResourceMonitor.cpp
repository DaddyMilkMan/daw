/*
  ==============================================================================

    PluginResourceMonitor.cpp
    Implementation of plugin resource monitoring

  ==============================================================================
*/

#include "PluginResourceMonitor.h"
#include <iostream>
#include <algorithm>
#include <numeric>

#ifdef JUCE_LINUX
#include <sys/resource.h>
#include <unistd.h>
#elif defined(JUCE_WINDOWS)
#include <windows.h>
#include <psapi.h>
#endif

namespace zenith {

//==============================================================================
PluginResourceMonitor::PluginResourceMonitor(const ResourceMonitorConfig& config)
    : config_(config) {

    std::cout << "PluginResourceMonitor: Initialized" << std::endl;
    std::cout << "  Sampling interval: " << config_.samplingIntervalMs << " ms" << std::endl;
    std::cout << "  History size: " << config_.historySize << " samples" << std::endl;
    std::cout << "  CPU threshold: " << config_.cpuWarningThreshold << "%" << std::endl;
    std::cout << "  Memory threshold: " << config_.memoryWarningThresholdMB << " MB" << std::endl;
}

//==============================================================================
PluginResourceMonitor::~PluginResourceMonitor() {
    std::cout << "PluginResourceMonitor: Shut down" << std::endl;

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    if (!monitoredPlugins_.empty()) {
        std::cout << "  Monitored " << monitoredPlugins_.size() << " plugins" << std::endl;

        for (const auto& pair : monitoredPlugins_) {
            const auto& stats = pair.second.stats;
            std::cout << "    " << stats.toString() << std::endl;
        }
    }
}

//==============================================================================
void PluginResourceMonitor::startMonitoring(juce::AudioPluginInstance* plugin,
                                           const juce::String& pluginId) {
    if (plugin == nullptr || pluginId.isEmpty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    MonitorData data;
    data.plugin = plugin;
    data.pluginId = pluginId;
    data.pluginName = plugin->getName();
    data.startTime = juce::Time::getCurrentTime();
    data.processing = false;

    // Initialize stats
    data.stats.pluginId = pluginId;
    data.stats.firstSeen = data.startTime;
    data.stats.lastSeen = data.startTime;

    monitoredPlugins_[pluginId] = std::move(data);

    std::cout << "PluginResourceMonitor: Started monitoring "
              << monitoredPlugins_[pluginId].pluginName
              << " (" << pluginId << ")" << std::endl;
}

//==============================================================================
void PluginResourceMonitor::stopMonitoring(const juce::String& pluginId) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it != monitoredPlugins_.end()) {
        std::cout << "PluginResourceMonitor: Stopped monitoring "
                  << it->second.pluginName << " (" << pluginId << ")" << std::endl;

        // Print final statistics
        std::cout << "  " << it->second.stats.toString() << std::endl;

        monitoredPlugins_.erase(it);
    }
}

//==============================================================================
void PluginResourceMonitor::beginProcess(const juce::String& pluginId) {
    if (!enabled_.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it == monitoredPlugins_.end()) {
        return;
    }

    it->second.processStartTime = std::chrono::high_resolution_clock::now();
    it->second.processing = true;
}

//==============================================================================
void PluginResourceMonitor::endProcess(const juce::String& pluginId) {
    if (!enabled_.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it == monitoredPlugins_.end() || !it->second.processing) {
        return;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - it->second.processStartTime
    );

    it->second.processing = false;
    it->second.current.processingTimeUs = duration.count();
    it->second.current.totalProcessCalls++;

    // Update snapshot and stats
    updateSnapshot(it->second);

    // Check thresholds
    if (config_.enableAlerts) {
        checkThresholds(it->second);
    }

    // Add to history
    addToHistory(it->second);
}

//==============================================================================
PluginResourceSnapshot PluginResourceMonitor::getSnapshot(const juce::String& pluginId) const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it != monitoredPlugins_.end()) {
        return it->second.current;
    }

    return PluginResourceSnapshot{};
}

//==============================================================================
std::vector<PluginResourceSnapshot> PluginResourceMonitor::getAllSnapshots() const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    std::vector<PluginResourceSnapshot> snapshots;
    snapshots.reserve(monitoredPlugins_.size());

    for (const auto& pair : monitoredPlugins_) {
        snapshots.push_back(pair.second.current);
    }

    return snapshots;
}

//==============================================================================
PluginResourceStats PluginResourceMonitor::getStatistics(const juce::String& pluginId) const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it != monitoredPlugins_.end()) {
        return it->second.stats;
    }

    return PluginResourceStats{};
}

//==============================================================================
std::vector<PluginResourceStats> PluginResourceMonitor::getAllStatistics() const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    std::vector<PluginResourceStats> allStats;
    allStats.reserve(monitoredPlugins_.size());

    for (const auto& pair : monitoredPlugins_) {
        allStats.push_back(pair.second.stats);
    }

    // Sort by total process calls
    std::sort(allStats.begin(), allStats.end(),
              [](const PluginResourceStats& a, const PluginResourceStats& b) {
                  return a.totalProcessCalls > b.totalProcessCalls;
              });

    return allStats;
}

//==============================================================================
std::vector<PluginResourceSnapshot> PluginResourceMonitor::getTopCpuConsumers(juce::uint32 count) {
    auto snapshots = getAllSnapshots();

    std::sort(snapshots.begin(), snapshots.end(),
              [](const PluginResourceSnapshot& a, const PluginResourceSnapshot& b) {
                  return a.cpuPercent > b.cpuPercent;
              });

    if (snapshots.size() > count) {
        snapshots.resize(count);
    }

    return snapshots;
}

//==============================================================================
std::vector<PluginResourceSnapshot> PluginResourceMonitor::getTopMemoryConsumers(juce::uint32 count) {
    auto snapshots = getAllSnapshots();

    std::sort(snapshots.begin(), snapshots.end(),
              [](const PluginResourceSnapshot& a, const PluginResourceSnapshot& b) {
                  return a.memoryBytes > b.memoryBytes;
              });

    if (snapshots.size() > count) {
        snapshots.resize(count);
    }

    return snapshots;
}

//==============================================================================
std::vector<PluginResourceSnapshot> PluginResourceMonitor::getOverloadedPlugins() {
    auto snapshots = getAllSnapshots();

    std::vector<PluginResourceSnapshot> overloaded;
    overloaded.reserve(snapshots.size());

    for (const auto& snapshot : snapshots) {
        if (snapshot.isOverloaded(config_.cpuWarningThreshold,
                                  config_.memoryWarningThresholdMB)) {
            overloaded.push_back(snapshot);
        }
    }

    return overloaded;
}

//==============================================================================
void PluginResourceMonitor::setAlertCallback(AlertCallback callback) {
    alertCallback_ = std::move(callback);
}

//==============================================================================
void PluginResourceMonitor::setMonitoringEnabled(bool enabled) {
    enabled_.store(enabled);

    std::cout << "PluginResourceMonitor: Monitoring "
              << (enabled ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
PluginResourceMonitor& PluginResourceMonitor::getInstance() {
    static PluginResourceMonitor instance;
    return instance;
}

//==============================================================================
void PluginResourceMonitor::updateSnapshot(MonitorData& data) {
    // Update timestamp
    data.current.timestamp = juce::Time::getCurrentTime();
    data.stats.lastSeen = data.current.timestamp;

    // Calculate CPU percentage
    if (data.current.processingTimeUs > 0) {
        double processingTimeMs = data.current.processingTimeUs / 1000.0;
        data.current.averageProcessingTimeMs = processingTimeMs;

        // Estimate CPU usage based on processing time vs interval
        double cpuUsage = (processingTimeMs / config_.samplingIntervalMs) * 100.0;
        data.current.cpuPercent = juce::jmin(cpuUsage, 100.0);

        // Update stats
        data.stats.totalProcessCalls = data.current.totalProcessCalls;

        if (data.stats.totalProcessCalls > 0) {
            data.stats.averageCpuPercent =
                (data.stats.averageCpuPercent * (data.stats.totalProcessCalls - 1) +
                 data.current.cpuPercent) / data.stats.totalProcessCalls;
        }

        data.stats.peakCpuPercent = juce::jmax(data.stats.peakCpuPercent,
                                               data.current.cpuPercent);
    }

    // Update memory usage
    if (config_.enableMemoryMonitoring) {
        data.current.memoryBytes = getPluginMemoryUsage(data.plugin);
        data.current.peakMemoryBytes = juce::jmax(data.current.peakMemoryBytes,
                                                    data.current.memoryBytes);

        // Update stats
        if (data.stats.totalProcessCalls > 0) {
            data.stats.averageMemoryBytes =
                (data.stats.averageMemoryBytes * (data.stats.totalProcessCalls - 1) +
                 data.current.memoryBytes) / data.stats.totalProcessCalls;
        }

        data.stats.peakMemoryBytes = juce::jmax(data.stats.peakMemoryBytes,
                                               data.current.peakMemoryBytes);
    }

    // Update I/O stats
    if (config_.enableIoMonitoring) {
        data.current.threadCount = getPluginThreadCount(data.plugin);
    }

    // Check for overruns
    juce::uint32 processingTimeMs = static_cast<juce::uint32>(
        data.current.processingTimeUs / 1000
    );

    if (processingTimeMs > config_.processingTimeWarningMs) {
        data.current.overruns++;
        data.stats.totalOverruns++;
    }
}

//==============================================================================
void PluginResourceMonitor::checkThresholds(const MonitorData& data) {
    bool alertTriggered = false;

    if (data.current.cpuPercent > config_.cpuWarningThreshold) {
        std::cerr << "PluginResourceMonitor: WARNING - " << data.pluginName
                  << " CPU usage " << juce::String(data.current.cpuPercent, 1)
                  << "% exceeds threshold " << config_.cpuWarningThreshold << "%" << std::endl;
        alertTriggered = true;
    }

    if (data.current.memoryBytes > config_.memoryWarningThresholdMB * 1024 * 1024) {
        std::cerr << "PluginResourceMonitor: WARNING - " << data.pluginName
                  << " memory usage " << (data.current.memoryBytes / (1024 * 1024))
                  << " MB exceeds threshold " << config_.memoryWarningThresholdMB << " MB" << std::endl;
        alertTriggered = true;
    }

    if (alertTriggered && alertCallback_) {
        alertCallback_(data.current);
    }
}

//==============================================================================
void PluginResourceMonitor::addToHistory(MonitorData& data) {
    data.history.push_back(data.current);

    // Trim history to max size
    if (data.history.size() > config_.historySize) {
        data.history.erase(data.history.begin());
    }
}

//==============================================================================
juce::uint64 PluginResourceMonitor::getPluginMemoryUsage(juce::AudioPluginInstance* plugin) const {
    // This is a rough estimate - actual per-plugin memory is hard to get
    // In production, you'd use platform-specific APIs to track this

#ifdef JUCE_LINUX
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss * 1024;  // Convert to bytes

#elif defined(JUCE_WINDOWS)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;

#else
    return 0;
#endif
}

//==============================================================================
juce::uint32 PluginResourceMonitor::getPluginThreadCount(juce::AudioPluginInstance* plugin) const {
    // This would require platform-specific thread enumeration
    // For now, return a reasonable estimate
    return 1;
}

} // namespace zenith
