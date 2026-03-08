/*
  ==============================================================================

    PluginWatchdog.h
    Created: 2026-02-19
    Month 10, Gap #4 - Plugin Watchdog

    Plugin health monitoring with hang detection and auto-recovery.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <map>
#include <mutex>
#include <functional>
#include <chrono>

namespace zenith {

//==============================================================================
/**
 * @brief Plugin health status
 */
enum class PluginHealth {
    Healthy,
    Degraded,
    Hung,
    Crashed,
    Unknown
};

//==============================================================================
/**
 * @brief Watchdog event
 */
struct WatchdogEvent {
    enum Type {
        None,
        PluginHung,
        PluginRecovered,
        PluginKilled,
        HeartbeatMissed,
        TimeoutExceeded
    };

    Type type = None;
    juce::String pluginId;
    juce::String pluginName;
    juce::Time timestamp;
    juce::String description;
    juce::uint64 durationMs = 0;

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case None: typeStr = "None"; break;
            case PluginHung: typeStr = "Hung"; break;
            case PluginRecovered: typeStr = "Recovered"; break;
            case PluginKilled: typeStr = "Killed"; break;
            case HeartbeatMissed: typeStr = "Heartbeat Missed"; break;
            case TimeoutExceeded: typeStr = "Timeout"; break;
        }

        return juce::String::formatted(
            "[%s] %s (%s): %s (%llu ms)",
            typeStr,
            pluginName,
            pluginId,
            description,
            durationMs
        );
    }
};

//==============================================================================
/**
 * @brief Watchdog configuration
 */
struct WatchdogConfig {
    juce::uint32 heartbeatIntervalMs = 100;      // Expected heartbeat every 100ms
    juce::uint32 hangDetectionTimeoutMs = 5000;  // 5 seconds without response = hung
    juce::uint32 killTimeoutMs = 10000;          // 10 seconds hung = kill
    bool enableAutoKill = true;                  // Auto-kill hung plugins
    bool enableAutoRestart = true;               // Auto-restart after kill
    juce::uint32 maxRestartAttempts = 3;         // Max restart attempts
    bool enableLogging = true;                   // Log all events
};

//==============================================================================
/**
 * @brief Plugin watchdog for hang detection and recovery
 *
 * Features:
 * - Heartbeat monitoring
 * - Hang detection
 * - Automatic termination of hung plugins
 * - Auto-restart capability
 * - Health status tracking
 * - Event logging
 */
class PluginWatchdog {
public:
    //==========================================================================
    explicit PluginWatchdog(const WatchdogConfig& config = {});
    ~PluginWatchdog();

    //==========================================================================
    /**
     * @brief Register plugin for monitoring
     */
    void registerPlugin(juce::AudioPluginInstance* plugin,
                       const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Unregister plugin from monitoring
     */
    void unregisterPlugin(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Send heartbeat (call during processing)
     */
    void heartbeat(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Get plugin health status
     */
    PluginHealth getHealth(const juce::String& pluginId) const;

    //==========================================================================
    /**
     * @brief Get time since last heartbeat
     */
    juce::uint64 getTimeSinceLastHeartbeat(const juce::String& pluginId) const;

    //==========================================================================
    /**
     * @brief Manually check all plugins
     */
    void checkAllPlugins();

    //==========================================================================
    /**
     * @brief Kill plugin forcefully
     */
    bool killPlugin(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Get event history
     */
    std::vector<WatchdogEvent> getEventHistory() const;

    //==========================================================================
    /**
     * @brief Clear event history
     */
    void clearEventHistory();

    //==========================================================================
    /**
     * @brief Set event callback
     */
    using EventCallback = std::function<void(const WatchdogEvent&)>;
    void setEventCallback(EventCallback callback);

    //==========================================================================
    /**
     * @brief Enable/disable monitoring
     */
    void setMonitoringEnabled(bool enabled);

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static PluginWatchdog& getInstance();

private:
    //==========================================================================
    WatchdogConfig config_;
    std::atomic<bool> enabled_{true};

    // Per-plugin monitoring data
    struct MonitorData {
        juce::AudioPluginInstance* plugin = nullptr;
        juce::String pluginId;
        juce::String pluginName;
        PluginHealth health = PluginHealth::Unknown;
        juce::Time lastHeartbeat;
        juce::Time registrationTime;
        juce::uint32 restartAttempts = 0;
        bool isMarkedForKilling = false;
    };

    std::map<juce::String, MonitorData> monitoredPlugins_;
    mutable std::mutex pluginsMutex_;

    // Event history
    std::vector<WatchdogEvent> eventHistory_;
    mutable std::mutex eventsMutex_;

    // Callback
    EventCallback eventCallback_;

    //==========================================================================
    void checkPlugin(MonitorData& data);
    void recordEvent(const WatchdogEvent& event);
    juce::String getPluginName(const juce::String& pluginId) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWatchdog)
};

//==============================================================================
/**
 * @brief RAII heartbeat scope
 *
 * Automatically sends heartbeats during processing.
 */
class WatchdogHeartbeat {
public:
    WatchdogHeartbeat(PluginWatchdog& watchdog, const juce::String& pluginId)
        : watchdog_(watchdog)
        , pluginId_(pluginId)
        , enabled_(true) {
    }

    ~WatchdogHeartbeat() {
        if (enabled_) {
            watchdog_.heartbeat(pluginId_);
        }
    }

    void disable() { enabled_ = false; }

private:
    PluginWatchdog& watchdog_;
    juce::String pluginId_;
    bool enabled_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WatchdogHeartbeat)
};

} // namespace zenith
