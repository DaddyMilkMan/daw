/*
  ==============================================================================

    OOMHandler.h
    Created: 2026-02-19
    Month 9, Gap #4 - Out-of-Memory Handling

    Graceful out-of-memory recovery system.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>
#include <vector>
#include <atomic>

namespace zenith {

//==============================================================================
/**
 * @brief Memory pressure level
 */
enum class MemoryPressure {
    Low,        // Plenty of memory available
    Medium,     // Memory usage is noticeable
    High,       // Memory is tight
    Critical    // Out-of-memory imminent
};

//==============================================================================
/**
 * @brief OOM recovery action
 */
struct OOMRecoveryAction {
    enum Type {
        None,                    // No action needed
        DropCache,              // Clear caches
        FreeUnusedMemory,        // Free unused buffers
        SuspendProcessing,      // Suspend non-critical processing
        ClosePlugins,           // Unload plugins
        SaveProjectAndExit,     // Emergency save and exit
        Abort                   // Last resort
    };

    Type type = None;
    juce::String description;
    juce::uint64 memoryToFree = 0;  // Bytes to attempt to free
    int priority = 0;           // 0-10, higher = more urgent

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case None: typeStr = "None"; break;
            case DropCache: typeStr = "Drop Cache"; break;
            case FreeUnusedMemory: typeStr = "Free Unused"; break;
            case SuspendProcessing: typeStr = "Suspend"; break;
            case ClosePlugins: typeStr = "Close Plugins"; break;
            case SaveProjectAndExit: typeStr = "Save & Exit"; break;
            case Abort: typeStr = "Abort"; break;
        }
        return "[" + typeStr + "] " + description;
    }
};

//==============================================================================
/**
 * @brief Memory snapshot
 */
struct MemorySnapshot {
    juce::uint64 totalPhysicalMemory = 0;
    juce::uint64 availablePhysicalMemory = 0;
    juce::uint64 totalVirtualMemory = 0;
    juce::uint64 availableVirtualMemory = 0;
    juce::uint64 processMemoryUsed = 0;
    double memoryUsagePercent = 0.0;
    juce::Time timestamp;

    MemoryPressure getPressureLevel() const {
        double usage = memoryUsagePercent;
        if (usage < 50.0) return MemoryPressure::Low;
        if (usage < 75.0) return MemoryPressure::Medium;
        if (usage < 90.0) return MemoryPressure::High;
        return MemoryPressure::Critical;
    }

    juce::String toString() const {
        return juce::String::formatted(
            "Memory: %.1f%% used (%llu MB / %llu MB)",
            memoryUsagePercent,
            processMemoryUsed / (1024 * 1024),
            totalPhysicalMemory / (1024 * 1024)
        );
    }
};

//==============================================================================
/**
 * @brief OOM handler configuration
 */
struct OOMHandlerConfig {
    juce::uint64 warningThresholdMB = 1024;        // 1GB
    juce::uint64 criticalThresholdMB = 2048;       // 2GB
    bool enableAutoSave = true;
    bool enablePluginUnload = true;
    juce::uint32 memoryCheckIntervalMs = 1000;     // Check every second
};

//==============================================================================
/**
 * @brief Out-of-memory handler
 *
 * Features:
 * - Monitors memory pressure
 * - Takes graduated recovery actions
 * - Emergency project saving
 * - Plugin unloading for memory recovery
 * - Memory pressure callbacks
 */
class OOMHandler {
public:
    //==========================================================================
    OOMHandler(const OOMHandlerConfig& config = {});
    ~OOMHandler();

    //==========================================================================
    /**
     * @brief Check current memory status
     */
    MemorySnapshot getMemorySnapshot() const;

    //==========================================================================
    /**
     * @brief Get recommended recovery action
     */
    OOMRecoveryAction getRecoveryAction(const MemorySnapshot& snapshot) const;

    //==========================================================================
    /**
     * @brief Attempt memory recovery
     * @return True if recovery succeeded
     */
    bool attemptRecovery(const OOMRecoveryAction& action);

    //==========================================================================
    /**
     * @brief Set memory pressure callback
     */
    using PressureCallback = std::function<void(MemoryPressure, const MemorySnapshot&)>;
    void setPressureCallback(PressureCallback callback);

    //==========================================================================
    /**
     * @brief Set OOM callback (called when OOM is imminent)
     */
    using OOMCallback = std::function<void()>;
    void setOOMCallback(OOMCallback callback);

    //==========================================================================
    /**
     * @brief Set plugin close callback
     *
     * Callback should close plugins and return bytes freed.
     * Signature: uint64 bytesFreed = callback(uint64 targetBytesToFree)
     */
    using PluginCloseCallback = std::function<juce::uint64(juce::uint64 targetBytesToFree)>;
    void setPluginCloseCallback(PluginCloseCallback callback);

    //==========================================================================
    /**
     * @brief Set project save callback
     *
     * Callback should perform emergency project save.
     * Signature: bool success = callback(const juce::String& backupPath)
     */
    using ProjectSaveCallback = std::function<bool(const juce::String& backupPath)>;
    void setProjectSaveCallback(ProjectSaveCallback callback);

    //==========================================================================
    /**
     * @brief Enable/disable monitoring
     */
    void setMonitoringEnabled(bool enabled);

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static OOMHandler& getInstance();

private:
    //==========================================================================
    OOMHandlerConfig config_;
    std::atomic<bool> monitoringEnabled_{true};
    PressureCallback pressureCallback_;
    OOMCallback oomCallback_;
    PluginCloseCallback pluginCloseCallback_;
    ProjectSaveCallback projectSaveCallback_;
    mutable std::mutex callbackMutex_;

    //==========================================================================
    juce::uint64 getProcessMemoryUsed() const;
    juce::uint64 getTotalPhysicalMemory() const;
    juce::uint64 getAvailablePhysicalMemory() const;
    double calculateMemoryUsagePercent() const;

    //==========================================================================
    bool dropCaches();
    bool freeUnusedMemory();
    bool suspendProcessing();
    bool closePlugins();
    bool saveProjectAndExit();

    //==========================================================================
    bool performEmergencySaveAndExit();
};

} // namespace zenith
