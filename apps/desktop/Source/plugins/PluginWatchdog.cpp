/*
  ==============================================================================

    PluginWatchdog.cpp
    Implementation of plugin watchdog system

  ==============================================================================
*/

#include "PluginWatchdog.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
PluginWatchdog::PluginWatchdog(const WatchdogConfig& config)
    : config_(config) {

    std::cout << "PluginWatchdog: Initialized" << std::endl;
    std::cout << "  Heartbeat interval: " << config_.heartbeatIntervalMs << " ms" << std::endl;
    std::cout << "  Hang timeout: " << config_.hangDetectionTimeoutMs << " ms" << std::endl;
    std::cout << "  Kill timeout: " << config_.killTimeoutMs << " ms" << std::endl;
    std::cout << "  Auto-kill: " << (config_.enableAutoKill ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
PluginWatchdog::~PluginWatchdog() {
    std::cout << "PluginWatchdog: Shut down" << std::endl;

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    if (!monitoredPlugins_.empty()) {
        std::cout << "  Monitoring " << monitoredPlugins_.size() << " plugins" << std::endl;
    }
}

//==============================================================================
void PluginWatchdog::registerPlugin(juce::AudioPluginInstance* plugin,
                                   const juce::String& pluginId) {
    if (plugin == nullptr || pluginId.isEmpty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    MonitorData data;
    data.plugin = plugin;
    data.pluginId = pluginId;
    data.pluginName = plugin->getName();
    data.health = PluginHealth::Healthy;
    data.lastHeartbeat = juce::Time::getCurrentTime();
    data.registrationTime = data.lastHeartbeat;
    data.restartAttempts = 0;
    data.isMarkedForKilling = false;

    monitoredPlugins_[pluginId] = std::move(data);

    std::cout << "PluginWatchdog: Registered "
              << monitoredPlugins_[pluginId].pluginName
              << " (" << pluginId << ")" << std::endl;
}

//==============================================================================
void PluginWatchdog::unregisterPlugin(const juce::String& pluginId) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it != monitoredPlugins_.end()) {
        std::cout << "PluginWatchdog: Unregistered "
                  << it->second.pluginName << " (" << pluginId << ")" << std::endl;
        monitoredPlugins_.erase(it);
    }
}

//==============================================================================
void PluginWatchdog::heartbeat(const juce::String& pluginId) {
    if (!enabled_.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it == monitoredPlugins_.end()) {
        return;
    }

    it->second.lastHeartbeat = juce::Time::getCurrentTime();

    // If plugin was marked as unhealthy, check if it recovered
    if (it->second.health != PluginHealth::Healthy) {
        it->second.health = PluginHealth::Healthy;

        WatchdogEvent event;
        event.type = WatchdogEvent::PluginRecovered;
        event.pluginId = pluginId;
        event.pluginName = it->second.pluginName;
        event.timestamp = juce::Time::getCurrentTime();
        event.description = "Plugin recovered and sending heartbeats";

        recordEvent(event);
    }
}

//==============================================================================
PluginHealth PluginWatchdog::getHealth(const juce::String& pluginId) const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it != monitoredPlugins_.end()) {
        return it->second.health;
    }

    return PluginHealth::Unknown;
}

//==============================================================================
juce::uint64 PluginWatchdog::getTimeSinceLastHeartbeat(const juce::String& pluginId) const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it != monitoredPlugins_.end()) {
        juce::Time now = juce::Time::getCurrentTime();
        juce::RelativeTime elapsed = now - it->second.lastHeartbeat;
        return static_cast<juce::uint64>(elapsed.inMilliseconds());
    }

    return 0;
}

//==============================================================================
void PluginWatchdog::checkAllPlugins() {
    if (!enabled_.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    for (auto& pair : monitoredPlugins_) {
        checkPlugin(pair.second);
    }
}

//==============================================================================
bool PluginWatchdog::killPlugin(const juce::String& pluginId) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = monitoredPlugins_.find(pluginId);
    if (it == monitoredPlugins_.end()) {
        return false;
    }

    MonitorData& data = it->second;

    std::cerr << "PluginWatchdog: KILLING "
              << data.pluginName << " (" << pluginId << ")" << std::endl;

    // Mark for killing (actual termination would happen at a higher level)
    data.isMarkedForKilling = true;
    data.health = PluginHealth::Crashed;

    WatchdogEvent event;
    event.type = WatchdogEvent::PluginKilled;
    event.pluginId = pluginId;
    event.pluginName = data.pluginName;
    event.timestamp = juce::Time::getCurrentTime();
    event.description = "Plugin killed by watchdog";

    recordEvent(event);

    return true;
}

//==============================================================================
std::vector<WatchdogEvent> PluginWatchdog::getEventHistory() const {
    std::lock_guard<std::mutex> lock(eventsMutex_);
    return eventHistory_;
}

//==============================================================================
void PluginWatchdog::clearEventHistory() {
    std::lock_guard<std::mutex> lock(eventsMutex_);
    eventHistory_.clear();

    std::cout << "PluginWatchdog: Event history cleared" << std::endl;
}

//==============================================================================
void PluginWatchdog::setEventCallback(EventCallback callback) {
    eventCallback_ = std::move(callback);
}

//==============================================================================
void PluginWatchdog::setMonitoringEnabled(bool enabled) {
    enabled_.store(enabled);

    std::cout << "PluginWatchdog: Monitoring "
              << (enabled ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
PluginWatchdog& PluginWatchdog::getInstance() {
    static PluginWatchdog instance;
    return instance;
}

//==============================================================================
void PluginWatchdog::checkPlugin(MonitorData& data) {
    if (data.isMarkedForKilling) {
        return;  // Already marked for termination
    }

    juce::Time now = juce::Time::getCurrentTime();
    juce::RelativeTime elapsed = now - data.lastHeartbeat;
    juce::uint64 elapsedMs = static_cast<juce::uint64>(elapsed.inMilliseconds());

    // Check for hang
    if (elapsedMs > config_.hangDetectionTimeoutMs) {
        if (data.health != PluginHealth::Hung) {
            data.health = PluginHealth::Hung;

            WatchdogEvent event;
            event.type = WatchdogEvent::PluginHung;
            event.pluginId = data.pluginId;
            event.pluginName = data.pluginName;
            event.timestamp = now;
            event.durationMs = elapsedMs;
            event.description = juce::String::formatted(
                "Plugin hasn't sent heartbeat for %llu ms",
                elapsedMs
            );

            recordEvent(event);

            std::cerr << "PluginWatchdog: HANG detected - "
                      << data.pluginName << " (" << data.pluginId << ") "
                      << "last heartbeat " << elapsedMs << " ms ago" << std::endl;
        }

        // Check if should kill
        if (elapsedMs > config_.killTimeoutMs && config_.enableAutoKill) {
            killPlugin(data.pluginId);
        }
    } else if (elapsedMs > config_.heartbeatIntervalMs * 2) {
        // Heartbeat missed but not yet hung
        WatchdogEvent event;
        event.type = WatchdogEvent::HeartbeatMissed;
        event.pluginId = data.pluginId;
        event.pluginName = data.pluginName;
        event.timestamp = now;
        event.durationMs = elapsedMs;
        event.description = "Heartbeat missed";

        recordEvent(event);
    }
}

//==============================================================================
void PluginWatchdog::recordEvent(const WatchdogEvent& event) {
    {
        std::lock_guard<std::mutex> lock(eventsMutex_);
        eventHistory_.push_back(event);

        // Keep only last 1000 events
        if (eventHistory_.size() > 1000) {
            eventHistory_.erase(eventHistory_.begin());
        }
    }

    if (config_.enableLogging) {
        std::cout << "PluginWatchdog: " << event.toString() << std::endl;
    }

    if (eventCallback_) {
        eventCallback_(event);
    }
}

//==============================================================================
juce::String PluginWatchdog::getPluginName(const juce::String& pluginId) const {
    auto it = monitoredPlugins_.find(pluginId);
    if (it != monitoredPlugins_.end()) {
        return it->second.pluginName;
    }
    return pluginId;
}

} // namespace zenith
