/*
  ==============================================================================

    PluginSandbox.cpp
    Implementation of plugin sandboxing

  ==============================================================================
*/

#include "PluginSandbox.h"
#include <iostream>
#include <algorithm>

#ifdef JUCE_LINUX
#include <sys/resource.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#elif defined(JUCE_WINDOWS)
#include <windows.h>
#include <psapi.h>
#elif defined(JUCE_MAC)
#include <mach/mach.h>
#include <mach/mach_host.h>
#endif

namespace zenith {

//==============================================================================
PluginSandbox::PluginSandbox(juce::AudioPluginInstance* plugin,
                            const PluginSandboxConfig& config)
    : plugin_(plugin)
    , config_(config) {

    if (plugin_ == nullptr) {
        throw std::invalid_argument("Plugin instance cannot be null");
    }

    std::cout << "PluginSandbox: Initialized for plugin: "
              << getPluginName() << std::endl;
    std::cout << "  Limits: " << config_.limits.toString() << std::endl;
    std::cout << "  Strict mode: " << (config_.enableStrictMode ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
PluginSandbox::~PluginSandbox() {
    setActive(false);

    std::cout << "PluginSandbox: Shut down for "
              << getPluginName() << std::endl;

    if (violationCount_ > 0) {
        std::cout << "  Total violations: " << violationCount_ << std::endl;
    }
}

//==============================================================================
void PluginSandbox::process(juce::AudioBuffer<float>& buffer,
                           juce::MidiBuffer& midi,
                           bool recording) {
    if (!active_.load() || plugin_ == nullptr) {
        return;
    }

    beforeProcess();

    // Time the processing
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Process audio
        plugin_->processBlock(buffer, midi);

    } catch (const std::exception& e) {
        reportViolation(SandboxViolation::CrashDetected,
                      "Exception during process: " + juce::String(e.what()),
                      0, 0);

        if (crashCallback_) {
            crashCallback_(getPluginName());
        }

        if (config_.enableAutoRecovery) {
            // Try to recover - clear buffer to prevent garbage output
            buffer.clear();
        }

    } catch (...) {
        reportViolation(SandboxViolation::CrashDetected,
                      "Unknown exception during process",
                      0, 0);

        if (crashCallback_) {
            crashCallback_(getPluginName());
        }

        if (config_.enableAutoRecovery) {
            buffer.clear();
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime
    );

    afterProcess();

    // Check limits
    juce::uint64 processingTimeUs = duration.count();
    checkCpuLimit(processingTimeUs);
    checkTimeout(processingTimeUs);
    checkMemoryLimit();

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        totalProcessingTimeUs_ += processingTimeUs;
        processCallCount_++;
        stats_.currentProcessingTimeMs =
            static_cast<juce::uint32>(processingTimeUs / 1000);
        stats_.averageProcessingTimeMs =
            static_cast<double>(totalProcessingTimeUs_) / processCallCount_ / 1000.0;
        stats_.totalProcessCalls = processCallCount_;
    }
}

//==============================================================================
void PluginSandbox::processBlock(juce::AudioBuffer<float>& inputBuffer,
                                juce::AudioBuffer<float>& outputBuffer,
                                juce::MidiBuffer& midi) {
    if (!active_.load() || plugin_ == nullptr) {
        return;
    }

    beforeProcess();

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Process with separate buffers
        plugin_->processBlock(inputBuffer, midi);

        // Copy to output if needed
        if (&inputBuffer != &outputBuffer) {
            outputBuffer.makeCopyOf(inputBuffer);
        }

    } catch (const std::exception& e) {
        reportViolation(SandboxViolation::CrashDetected,
                      "Exception during processBlock: " + juce::String(e.what()),
                      0, 0);

        if (crashCallback_) {
            crashCallback_(getPluginName());
        }

        if (config_.enableAutoRecovery) {
            outputBuffer.clear();
        }

    } catch (...) {
        reportViolation(SandboxViolation::CrashDetected,
                      "Unknown exception during processBlock",
                      0, 0);

        if (crashCallback_) {
            crashCallback_(getPluginName());
        }

        if (config_.enableAutoRecovery) {
            outputBuffer.clear();
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime
    );

    afterProcess();

    juce::uint64 processingTimeUs = duration.count();
    checkCpuLimit(processingTimeUs);
    checkTimeout(processingTimeUs);
    checkMemoryLimit();

    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        totalProcessingTimeUs_ += processingTimeUs;
        processCallCount_++;
        stats_.currentProcessingTimeMs =
            static_cast<juce::uint32>(processingTimeUs / 1000);
        stats_.averageProcessingTimeMs =
            static_cast<double>(totalProcessingTimeUs_) / processCallCount_ / 1000.0;
        stats_.totalProcessCalls = processCallCount_;
    }
}

//==============================================================================
SandboxStats PluginSandbox::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);

    SandboxStats stats = stats_;
    stats.currentMemoryUsage = getCurrentMemoryUsage();
    stats.currentCpuPercent = getCurrentCpuPercent();
    stats.currentThreadCount = getCurrentThreadCount();

    return stats;
}

//==============================================================================
void PluginSandbox::setLimits(const SandboxLimits& newLimits) {
    config_.limits = newLimits;

    std::cout << "PluginSandbox: Limits updated for "
              << getPluginName() << std::endl;
    std::cout << "  New limits: " << newLimits.toString() << std::endl;
}

//==============================================================================
void PluginSandbox::resetViolations() {
    violationCount_.store(0);

    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.totalViolations = 0;

    std::cout << "PluginSandbox: Violations reset for "
              << getPluginName() << std::endl;
}

//==============================================================================
void PluginSandbox::setActive(bool active) {
    active_.store(active);

    std::cout << "PluginSandbox: "
              << (active ? "Activated" : "Deactivated")
              << " for " << getPluginName() << std::endl;
}

//==============================================================================
void PluginSandbox::setViolationCallback(ViolationCallback callback) {
    violationCallback_ = std::move(callback);
}

//==============================================================================
void PluginSandbox::setCrashCallback(CrashCallback callback) {
    crashCallback_ = std::move(callback);
}

//==============================================================================
juce::String PluginSandbox::getPluginName() const {
    if (plugin_ != nullptr) {
        return plugin_->getName();
    }
    return "Unknown";
}

//==============================================================================
void PluginSandbox::beforeProcess() {
    lastProcessStart_ = std::chrono::high_resolution_clock::now();
}

//==============================================================================
void PluginSandbox::afterProcess() {
    // Update statistics after processing
}

//==============================================================================
void PluginSandbox::checkMemoryLimit() {
    juce::uint64 currentUsage = getCurrentMemoryUsage();

    if (currentUsage > config_.limits.maxMemoryBytes) {
        reportViolation(SandboxViolation::MemoryLimitExceeded,
                      "Memory limit exceeded",
                      currentUsage,
                      config_.limits.maxMemoryBytes);
    }

    // Update stats
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.currentMemoryUsage = currentUsage;
}

//==============================================================================
void PluginSandbox::checkCpuLimit(juce::uint64 processingTimeUs) {
    // Calculate CPU percentage based on processing time vs real time
    double cpuPercent = 0.0;

    if (processingTimeUs > 0) {
        // Rough estimate: if we use 100% of a core for the entire time slice
        cpuPercent = (processingTimeUs / 1000.0) / config_.limits.maxProcessingTimeMs * 100.0;
    }

    if (cpuPercent > config_.limits.maxCpuPercent) {
        reportViolation(SandboxViolation::CpuLimitExceeded,
                      "CPU limit exceeded",
                      static_cast<juce::uint64>(cpuPercent),
                      static_cast<juce::uint64>(config_.limits.maxCpuPercent));
    }

    // Update stats
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.currentCpuPercent = cpuPercent;
}

//==============================================================================
void PluginSandbox::checkTimeout(juce::uint64 processingTimeUs) {
    juce::uint32 processingTimeMs = static_cast<juce::uint32>(processingTimeUs / 1000);

    if (processingTimeMs > config_.limits.maxProcessingTimeMs) {
        reportViolation(SandboxViolation::TimeoutExceeded,
                      "Processing timeout",
                      processingTimeMs,
                      config_.limits.maxProcessingTimeMs);
    }
}

//==============================================================================
void PluginSandbox::reportViolation(SandboxViolation::Type type,
                                   const juce::String& description,
                                   juce::uint64 violationValue,
                                   juce::uint64 limitValue) {
    violationCount_.fetch_add(1, std::memory_order_relaxed);

    SandboxViolation violation;
    violation.type = type;
    violation.description = description;
    violation.pluginName = getPluginName();
    violation.timestamp = juce::Time::getCurrentTime();
    violation.violationValue = violationValue;
    violation.limitValue = limitValue;

    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.totalViolations++;
    stats_.lastViolationTime = violation.timestamp;

    if (config_.enableLogging) {
        std::cerr << "PluginSandbox: VIOLATION - " << violation.toString() << std::endl;
    }

    if (violationCallback_) {
        violationCallback_(violation);
    }

    if (config_.enableStrictMode) {
        enforceStrictMode(violation);
    }
}

//==============================================================================
void PluginSandbox::enforceStrictMode(const SandboxViolation& violation) {
    juce::uint64 count = violationCount_.load(std::memory_order_relaxed);

    if (count >= config_.violationThreshold) {
        std::cerr << "PluginSandbox: CRITICAL - Violation threshold reached ("
                  << count << "), deactivating plugin" << std::endl;

        setActive(false);

        if (crashCallback_) {
            crashCallback_(getPluginName() + " - Too many violations");
        }
    }
}

//==============================================================================
juce::uint64 PluginSandbox::getCurrentMemoryUsage() const {
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

#elif defined(JUCE_MAC)
    struct task_basic_info info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info,
                  &count) == KERN_SUCCESS) {
        return info.resident_size;
    }
    return 0;

#else
    return 0;
#endif
}

//==============================================================================
double PluginSandbox::getCurrentCpuPercent() const {
    // Platform-specific CPU usage would go here
    // For now, return the last calculated value from checkCpuLimit
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_.currentCpuPercent;
}

//==============================================================================
juce::uint32 PluginSandbox::getCurrentThreadCount() const {
    // Platform-specific thread count would go here
    // For now, return a reasonable estimate
#ifdef JUCE_LINUX
    FILE* file = fopen("/proc/self/status", "r");
    if (file) {
        char line[128];
        int threads = 0;
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "Threads:", 8) == 0) {
                sscanf(line + 8, "%d", &threads);
                fclose(file);
                return static_cast<juce::uint32>(threads);
            }
        }
        fclose(file);
    }
    return 1;

#elif defined(JUCE_WINDOWS)
    DWORD processID = GetCurrentProcessId();
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (hProcess) {
        DWORD threadCount = 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te32;
            te32.dwSize = sizeof(THREADENTRY32);
            if (Thread32First(hSnapshot, &te32)) {
                do {
                    if (te32.th32OwnerProcessID == processID) {
                        threadCount++;
                    }
                } while (Thread32Next(hSnapshot, &te32));
            }
            CloseHandle(hSnapshot);
        }
        CloseHandle(hProcess);
        return threadCount;
    }
    return 1;

#else
    return 1;
#endif
}

} // namespace zenith
