/*
  ==============================================================================

    PluginResourceMonitor.h
    Created: 2026-02-19
    Month 10, Gap #3 - Plugin Resource Monitoring

    Real-time monitoring of plugin resource usage (CPU, memory, I/O).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Resource usage snapshot for a single plugin
 */
struct PluginResourceSnapshot {
    juce::String pluginId;
    juce::String pluginName;
    juce::Time timestamp;

    // CPU usage
    double cpuPercent = 0.0;
    juce::uint64 processingTimeUs = 0;
    double averageProcessingTimeMs = 0.0;

    // Memory usage
    juce::uint64 memoryBytes = 0;
    juce::uint64 peakMemoryBytes = 0;

    // I/O usage
    juce::uint32 fileHandleCount = 0;
    juce::uint32 threadCount = 0;

    // Performance metrics
    juce::uint64 totalProcessCalls = 0;
    juce::uint64 overruns = 0;  // Times processing exceeded limit

    bool isOverloaded(double maxCpuPercent = 80.0,
                     juce::uint64 maxMemoryMB = 512) const {
        return cpuPercent > maxCpuPercent ||
               memoryBytes > (maxMemoryMB * 1024 * 1024);
    }

    juce::String toString() const {
        return juce::String::formatted(
            "%s: CPU %.1f%%, Mem %llu MB, Time %.2f ms, Calls %llu",
            pluginName,
            cpuPercent,
            memoryBytes / (1024 * 1024),
            averageProcessingTimeMs,
            totalProcessCalls
        );
    }
};

//==============================================================================
/**
 * @brief Resource statistics over time
 */
struct PluginResourceStats {
    juce::String pluginId;
    double averageCpuPercent = 0.0;
    double peakCpuPercent = 0.0;
    juce::uint64 averageMemoryBytes = 0;
    juce::uint64 peakMemoryBytes = 0;
    juce::uint64 totalProcessCalls = 0;
    juce::uint64 totalOverruns = 0;
    juce::Time firstSeen;
    juce::Time lastSeen;

    juce::String toString() const {
        return juce::String::formatted(
            "%s: Avg CPU %.1f%%, Peak CPU %.1f%%, Peak Mem %llu MB, Overruns %llu",
            pluginId,
            averageCpuPercent,
            peakCpuPercent,
            peakMemoryBytes / (1024 * 1024),
            totalOverruns
        );
    }
};

//==============================================================================
/**
 * @brief Resource monitor configuration
 */
struct ResourceMonitorConfig {
    juce::uint32 samplingIntervalMs = 100;       // Sample every 100ms
    juce::uint32 historySize = 3600;              // Keep 1 hour of history (at 100ms)
    bool enableCpuMonitoring = true;
    bool enableMemoryMonitoring = true;
    bool enableIoMonitoring = true;
    double cpuWarningThreshold = 80.0;           // Warn at 80% CPU
    juce::uint64 memoryWarningThresholdMB = 512; // Warn at 512MB
    juce::uint32 processingTimeWarningMs = 50;   // Warn at 50ms per block
    bool enableAlerts = true;                    // Alert on threshold exceeded
};

//==============================================================================
/**
 * @brief Real-time plugin resource monitor
 *
 * Features:
 * - Continuous CPU usage monitoring
 * - Memory usage tracking
 * - I/O handle counting
 * - Processing time measurement
 * - Thread count monitoring
 * - Statistics aggregation
 * - Threshold-based alerting
 * - Historical data tracking
 */
class PluginResourceMonitor {
public:
    //==========================================================================
    explicit PluginResourceMonitor(const ResourceMonitorConfig& config = {});
    ~PluginResourceMonitor();

    //==========================================================================
    /**
     * @brief Start monitoring a plugin
     */
    void startMonitoring(juce::AudioPluginInstance* plugin,
                        const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Stop monitoring a plugin
     */
    void stopMonitoring(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Record processing start
     */
    void beginProcess(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Record processing end
     */
    void endProcess(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Get current snapshot for plugin
     */
    PluginResourceSnapshot getSnapshot(const juce::String& pluginId) const;

    //==========================================================================
    /**
     * @brief Get all current snapshots
     */
    std::vector<PluginResourceSnapshot> getAllSnapshots() const;

    //==========================================================================
    /**
     * @brief Get aggregated statistics for plugin
     */
    PluginResourceStats getStatistics(const juce::String& pluginId) const;

    //==========================================================================
    /**
     * @brief Get all plugin statistics
     */
    std::vector<PluginResourceStats> getAllStatistics() const;

    //==========================================================================
    /**
     * @brief Get top CPU consumers
     */
    std::vector<PluginResourceSnapshot> getTopCpuConsumers(juce::uint32 count = 10);

    //==========================================================================
    /**
     * @brief Get top memory consumers
     */
    std::vector<PluginResourceSnapshot> getTopMemoryConsumers(juce::uint32 count = 10);

    //==========================================================================
    /**
     * @brief Get plugins exceeding thresholds
     */
    std::vector<PluginResourceSnapshot> getOverloadedPlugins();

    //==========================================================================
    /**
     * @brief Set threshold alert callback
     */
    using AlertCallback = std::function<void(const PluginResourceSnapshot&)>;
    void setAlertCallback(AlertCallback callback);

    //==========================================================================
    /**
     * @brief Enable/disable monitoring
     */
    void setMonitoringEnabled(bool enabled);

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static PluginResourceMonitor& getInstance();

private:
    //==========================================================================
    ResourceMonitorConfig config_;
    std::atomic<bool> enabled_{true};

    // Per-plugin monitoring data
    struct MonitorData {
        juce::AudioPluginInstance* plugin = nullptr;
        juce::String pluginId;
        juce::String pluginName;
        juce::Time startTime;

        // Timing
        std::chrono::high_resolution_clock::time_point processStartTime;
        bool processing = false;

        // Current snapshot
        PluginResourceSnapshot current;

        // History
        std::vector<PluginResourceSnapshot> history;

        // Aggregated stats
        PluginResourceStats stats;
    };

    std::map<juce::String, MonitorData> monitoredPlugins_;
    mutable std::mutex pluginsMutex_;

    // Alert callback
    AlertCallback alertCallback_;

    //==========================================================================
    void updateSnapshot(MonitorData& data);
    void checkThresholds(const MonitorData& data);
    void addToHistory(MonitorData& data);

    juce::uint64 getPluginMemoryUsage(juce::AudioPluginInstance* plugin) const;
    juce::uint32 getPluginThreadCount(juce::AudioPluginInstance* plugin) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginResourceMonitor)
};

//==============================================================================
/**
 * @brief RAII resource monitoring scope
 *
 * Automatically tracks processing time for a plugin.
 */
class ResourceMonitoringScope {
public:
    ResourceMonitoringScope(PluginResourceMonitor& monitor,
                           const juce::String& pluginId)
        : monitor_(monitor)
        , pluginId_(pluginId) {
        monitor_.beginProcess(pluginId_);
    }

    ~ResourceMonitoringScope() {
        monitor_.endProcess(pluginId_);
    }

private:
    PluginResourceMonitor& monitor_;
    juce::String pluginId_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResourceMonitoringScope)
};

} // namespace zenith
