/*
  ==============================================================================

    PluginSandbox.h
    Created: 2026-02-19
    Month 10, Gap #1 - Plugin Sandboxing

    Process isolation and resource limiting for third-party plugins.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <functional>
#include <chrono>

namespace zenith {

//==============================================================================
/**
 * @brief Resource limits for sandboxed plugins
 */
struct SandboxLimits {
    juce::uint64 maxMemoryBytes = 512 * 1024 * 1024;  // 512 MB default
    double maxCpuPercent = 50.0;                       // 50% CPU max
    juce::uint32 maxProcessingTimeMs = 100;           // 100ms per block max
    juce::uint32 maxFileHandles = 256;                // Max open files
    bool allowNetworkAccess = false;                  // No network by default
    bool allowDiskWrite = false;                      // Read-only filesystem
    juce::uint32 maxThreadCount = 4;                  // Max threads

    juce::String toString() const {
        return juce::String::formatted(
            "Memory: %llu MB, CPU: %.1f%%, Time: %u ms, Files: %u, Network: %d, Disk: %d",
            maxMemoryBytes / (1024 * 1024),
            maxCpuPercent,
            maxProcessingTimeMs,
            maxFileHandles,
            allowNetworkAccess,
            allowDiskWrite
        );
    }
};

//==============================================================================
/**
 * @brief Sandbox violation type
 */
struct SandboxViolation {
    enum Type {
        None,
        MemoryLimitExceeded,
        CpuLimitExceeded,
        TimeoutExceeded,
        UnauthorizedFileAccess,
        UnauthorizedNetworkAccess,
        UnauthorizedDiskWrite,
        ThreadLimitExceeded,
        CrashDetected
    };

    Type type = None;
    juce::String description;
    juce::String pluginName;
    juce::Time timestamp;
    juce::uint64 violationValue = 0;  // Actual value that exceeded limit
    juce::uint64 limitValue = 0;      // Limit that was exceeded

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case None: typeStr = "None"; break;
            case MemoryLimitExceeded: typeStr = "Memory Limit"; break;
            case CpuLimitExceeded: typeStr = "CPU Limit"; break;
            case TimeoutExceeded: typeStr = "Timeout"; break;
            case UnauthorizedFileAccess: typeStr = "File Access"; break;
            case UnauthorizedNetworkAccess: typeStr = "Network Access"; break;
            case UnauthorizedDiskWrite: typeStr = "Disk Write"; break;
            case ThreadLimitExceeded: typeStr = "Thread Limit"; break;
            case CrashDetected: typeStr = "Crash"; break;
        }

        return juce::String::formatted(
            "[%s] %s: %s (value: %llu, limit: %llu)",
            typeStr,
            pluginName,
            description,
            violationValue,
            limitValue
        );
    }
};

//==============================================================================
/**
 * @brief Sandbox statistics
 */
struct SandboxStats {
    juce::uint64 currentMemoryUsage = 0;
    double currentCpuPercent = 0.0;
    juce::uint32 currentProcessingTimeMs = 0;
    juce::uint32 currentFileHandles = 0;
    juce::uint32 currentThreadCount = 0;
    juce::uint64 totalViolations = 0;
    juce::uint64 totalProcessCalls = 0;
    double averageProcessingTimeMs = 0.0;
    juce::Time lastViolationTime;

    bool isWithinLimits(const SandboxLimits& limits) const {
        return currentMemoryUsage <= limits.maxMemoryBytes &&
               currentCpuPercent <= limits.maxCpuPercent &&
               currentProcessingTimeMs <= limits.maxProcessingTimeMs &&
               currentFileHandles <= limits.maxFileHandles &&
               currentThreadCount <= limits.maxThreadCount;
    }

    juce::String toString() const {
        return juce::String::formatted(
            "Memory: %llu MB, CPU: %.1f%%, Time: %u ms, Files: %u, Threads: %u, Violations: %llu",
            currentMemoryUsage / (1024 * 1024),
            currentCpuPercent,
            currentProcessingTimeMs,
            currentFileHandles,
            currentThreadCount,
            totalViolations
        );
    }
};

//==============================================================================
/**
 * @brief Plugin sandbox configuration
 */
struct PluginSandboxConfig {
    SandboxLimits limits;
    bool enableStrictMode = true;              // Terminate on any violation
    bool enableLogging = true;                 // Log all violations
    bool enableAutoRecovery = true;            // Auto-restart on crash
    juce::uint32 violationThreshold = 3;       // Max violations before action
    bool enableResourceMonitoring = true;      // Real-time monitoring
    juce::uint32 monitoringIntervalMs = 100;   // Update interval
};

//==============================================================================
/**
 * @brief Plugin sandbox for safe third-party plugin execution
 *
 * Features:
 * - Resource limiting (memory, CPU, I/O)
 * - Sandboxed file system access
 * - Network access control
 * - Crash detection and recovery
 * - Real-time monitoring
 * - Violation tracking and reporting
 *
 * Architecture:
 * - Wraps juce::AudioPluginInstance
 * - Monitors resource usage during process()
 * - Enforces limits
 * - Reports violations
 * - Automatic recovery
 */
class PluginSandbox {
public:
    //==========================================================================
    PluginSandbox(juce::AudioPluginInstance* plugin,
                 const PluginSandboxConfig& config = {});

    ~PluginSandbox();

    //==========================================================================
    /**
     * @brief Process audio with sandboxing
     */
    void process(juce::AudioBuffer<float>& buffer,
                juce::MidiBuffer& midi,
                bool recording = false);

    //==========================================================================
    /**
     * @brief Process audio with separate input/output buffers
     */
    void processBlock(juce::AudioBuffer<float>& inputBuffer,
                     juce::AudioBuffer<float>& outputBuffer,
                     juce::MidiBuffer& midi);

    //==========================================================================
    /**
     * @brief Get current sandbox statistics
     */
    SandboxStats getStatistics() const;

    //==========================================================================
    /**
     * @brief Get current limits
     */
    SandboxLimits getLimits() const { return config_.limits; }

    //==========================================================================
    /**
     * @brief Update limits
     */
    void setLimits(const SandboxLimits& newLimits);

    //==========================================================================
    /**
     * @brief Get plugin instance
     */
    juce::AudioPluginInstance* getPlugin() const { return plugin_; }

    //==========================================================================
    /**
     * @brief Check if plugin is in violation state
     */
    bool isInViolation() const { return violationCount_ > 0; }

    //==========================================================================
    /**
     * @brief Get violation count
     */
    juce::uint64 getViolationCount() const { return violationCount_; }

    //==========================================================================
    /**
     * @brief Reset violation count
     */
    void resetViolations();

    //==========================================================================
    /**
     * @brief Check if sandbox is active
     */
    bool isActive() const { return active_.load(); }

    //==========================================================================
    /**
     * @brief Activate/deactivate sandbox
     */
    void setActive(bool active);

    //==========================================================================
    /**
     * @brief Set violation callback
     */
    using ViolationCallback = std::function<void(const SandboxViolation&)>;
    void setViolationCallback(ViolationCallback callback);

    //==========================================================================
    /**
     * @brief Set crash callback
     */
    using CrashCallback = std::function<void(const juce::String&)>;
    void setCrashCallback(CrashCallback callback);

    //==========================================================================
    /**
     * @brief Get plugin name
     */
    juce::String getPluginName() const;

private:
    //==========================================================================
    juce::AudioPluginInstance* plugin_;
    PluginSandboxConfig config_;
    std::atomic<bool> active_{true};
    std::atomic<juce::uint64> violationCount_{0};

    // Statistics tracking
    SandboxStats stats_;
    mutable std::mutex statsMutex_;

    // Callbacks
    ViolationCallback violationCallback_;
    CrashCallback crashCallback_;

    // Monitoring
    std::chrono::high_resolution_clock::time_point lastProcessStart_;
    juce::uint64 totalProcessingTimeUs_ = 0;
    juce::uint64 processCallCount_ = 0;

    //==========================================================================
    void beforeProcess();
    void afterProcess();

    void checkMemoryLimit();
    void checkCpuLimit(juce::uint64 processingTimeUs);
    void checkTimeout(juce::uint64 processingTimeUs);

    void reportViolation(SandboxViolation::Type type,
                        const juce::String& description,
                        juce::uint64 violationValue,
                        juce::uint64 limitValue);

    void enforceStrictMode(const SandboxViolation& violation);

    juce::uint64 getCurrentMemoryUsage() const;
    double getCurrentCpuPercent() const;
    juce::uint32 getCurrentThreadCount() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSandbox)
};

//==============================================================================
/**
 * @brief RAII sandbox scope for automatic cleanup
 */
class SandboxScope {
public:
    explicit SandboxScope(PluginSandbox& sandbox)
        : sandbox_(sandbox) {
        sandbox_.setActive(true);
    }

    ~SandboxScope() {
        sandbox_.setActive(false);
    }

private:
    PluginSandbox& sandbox_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SandboxScope)
};

} // namespace zenith
