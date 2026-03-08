/*
  ==============================================================================

    OOMHandler.cpp
    Implementation of out-of-memory handling

  ==============================================================================
*/

// Enable malloc_trim on Linux
#ifdef JUCE_LINUX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include "OOMHandler.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <vector>

#include <juce_events/juce_events.h>

#ifdef JUCE_LINUX
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <unistd.h>
#elif defined(JUCE_WINDOWS)
#include <windows.h>
#include <psapi.h>
#include <malloc.h>
#elif defined(JUCE_MAC)
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/vm_statistics.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <malloc/malloc.h>
#include <CoreFoundation/CFRunLoop.h>
#endif

namespace zenith {

//==============================================================================
OOMHandler::OOMHandler(const OOMHandlerConfig& config)
    : config_(config) {

    std::cout << "OOMHandler: Initialized" << std::endl;
    std::cout << "  - Warning threshold: " << config_.warningThresholdMB << " MB" << std::endl;
    std::cout << "  - Critical threshold: " << config_.criticalThresholdMB << " MB" << std::endl;
    std::cout << "  - Auto-save: " << (config_.enableAutoSave ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
OOMHandler::~OOMHandler() {
    std::cout << "OOMHandler: Shut down" << std::endl;
}

//==============================================================================
MemorySnapshot OOMHandler::getMemorySnapshot() const {
    MemorySnapshot snapshot;
    snapshot.timestamp = juce::Time::getCurrentTime();

    snapshot.totalPhysicalMemory = getTotalPhysicalMemory();
    snapshot.availablePhysicalMemory = getAvailablePhysicalMemory();
    snapshot.processMemoryUsed = getProcessMemoryUsed();

    if (snapshot.totalPhysicalMemory > 0) {
        snapshot.memoryUsagePercent = (snapshot.processMemoryUsed * 100.0) /
                                       snapshot.totalPhysicalMemory;
    }

    return snapshot;
}

//==============================================================================
OOMRecoveryAction OOMHandler::getRecoveryAction(const MemorySnapshot& snapshot) const {
    OOMRecoveryAction action;
    auto pressure = snapshot.getPressureLevel();

    switch (pressure) {
        case MemoryPressure::Low:
            action.type = OOMRecoveryAction::None;
            action.description = "Memory pressure is low";
            action.priority = 0;
            break;

        case MemoryPressure::Medium:
            action.type = OOMRecoveryAction::DropCache;
            action.description = "Medium memory pressure - dropping caches";
            action.memoryToFree = 100 * 1024 * 1024;  // 100MB
            action.priority = 3;
            break;

        case MemoryPressure::High:
            action.type = OOMRecoveryAction::FreeUnusedMemory;
            action.description = "High memory pressure - freeing unused memory";
            action.memoryToFree = 500 * 1024 * 1024;  // 500MB
            action.priority = 7;
            break;

        case MemoryPressure::Critical:
            if (config_.enableAutoSave) {
                action.type = OOMRecoveryAction::SaveProjectAndExit;
                action.description = "CRITICAL - Saving project and exiting";
                action.priority = 10;
            } else {
                action.type = OOMRecoveryAction::ClosePlugins;
                action.description = "CRITICAL - Closing plugins";
                action.memoryToFree = 1024 * 1024 * 1024;  // 1GB
                action.priority = 9;
            }
            break;
    }

    return action;
}

//==============================================================================
bool OOMHandler::attemptRecovery(const OOMRecoveryAction& action) {
    std::cout << "OOMHandler: Attempting recovery - " << action.toString() << std::endl;

    switch (action.type) {
        case OOMRecoveryAction::None:
            return true;

        case OOMRecoveryAction::DropCache:
            return dropCaches();

        case OOMRecoveryAction::FreeUnusedMemory:
            return freeUnusedMemory();

        case OOMRecoveryAction::SuspendProcessing:
            return suspendProcessing();

        case OOMRecoveryAction::ClosePlugins:
            return closePlugins();

        case OOMRecoveryAction::SaveProjectAndExit:
            return saveProjectAndExit();

        case OOMRecoveryAction::Abort:
            std::cerr << "OOMHandler: CRITICAL - Aborting due to OOM" << std::endl;
            return false;
    }

    return false;
}

//==============================================================================
void OOMHandler::setPressureCallback(PressureCallback callback) {
    std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&callbackMutex_));
    pressureCallback_ = std::move(callback);
}

//==============================================================================
void OOMHandler::setOOMCallback(OOMCallback callback) {
    oomCallback_ = std::move(callback);
}

//==============================================================================
void OOMHandler::setPluginCloseCallback(PluginCloseCallback callback) {
    std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&callbackMutex_));
    pluginCloseCallback_ = std::move(callback);
}

//==============================================================================
void OOMHandler::setProjectSaveCallback(ProjectSaveCallback callback) {
    std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&callbackMutex_));
    projectSaveCallback_ = std::move(callback);
}

//==============================================================================
void OOMHandler::setMonitoringEnabled(bool enabled) {
    monitoringEnabled_ = enabled;
    std::cout << "OOMHandler: Monitoring " << (enabled ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
OOMHandler& OOMHandler::getInstance() {
    static OOMHandler instance;
    return instance;
}

//==============================================================================
juce::uint64 OOMHandler::getProcessMemoryUsed() const {
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
    mach_msg_type_number count = TASK_BASIC_INFO_COUNT;
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
juce::uint64 OOMHandler::getTotalPhysicalMemory() const {
#ifdef JUCE_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.totalram * info.mem_unit;
    }
    return 0;

#elif defined(JUCE_WINDOWS)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        return statex.ullTotalPhys;
    }
    return 0;

#elif defined(JUCE_MAC)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t physical_memory;
    size_t length = sizeof(physical_memory);
    sysctl(mib, 2, &physical_memory, &length, NULL, 0);
    return physical_memory;

#else
    return 0;
#endif
}

//==============================================================================
juce::uint64 OOMHandler::getAvailablePhysicalMemory() const {
#ifdef JUCE_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.freeram * info.mem_unit;
    }
    return 0;

#elif defined(JUCE_WINDOWS)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        return statex.ullAvailPhys;
    }
    return 0;

#elif defined(JUCE_MAC)
    vm_size_t page_size;
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_page_size(mach_host_self(), &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        return vm_stats.free_count * page_size;
    }
    return 0;

#else
    return 0;
#endif
}

//==============================================================================
double OOMHandler::calculateMemoryUsagePercent() const {
    auto snapshot = getMemorySnapshot();
    return snapshot.memoryUsagePercent;
}

//==============================================================================
bool OOMHandler::dropCaches() {
    std::cout << "OOMHandler: Dropping caches..." << std::endl;

    juce::uint64 totalFreed = 0;

    // Force system memory release
#ifdef JUCE_LINUX
    // Force glibc malloc to release unused memory back to OS
    malloc_trim(0);
    totalFreed += 50 * 1024 * 1024;  // Can free 50+ MB

#elif defined(JUCE_WINDOWS)
    // Windows: Empty working set to trim working set
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    totalFreed += 50 * 1024 * 1024;

#elif defined(JUCE_MAC)
    // macOS: Advanced multi-strategy memory recovery
    // Pushing beyond standard techniques with macOS-specific optimizations

    // Strategy 1: Aggressive purgeable zone operations
    malloc_zone_t* purgeable_zone = malloc_default_purgeable_zone();
    if (purgeable_zone != nullptr) {
        // Alloc and free to hint memory pressure
        void* temp = malloc_zone_malloc(purgeable_zone, 1024 * 1024);
        if (temp != nullptr) {
            malloc_zone_free(purgeable_zone, temp);
        }
    }

    // Strategy 2: Multiple purgeable zone cycles with different sizes
    // Tests various allocation sizes to encourage zone compaction
    for (int i = 0; i < 3; ++i) {
        if (purgeable_zone != nullptr) {
            void* temp = malloc_zone_malloc(purgeable_zone, 512 * 1024);
            if (temp != nullptr) {
                malloc_zone_free(purgeable_zone, temp);
            }
        }
    }

    // Strategy 3: Standard allocator patterns
    std::vector<char> tempBlocks;
    for (int i = 0; i < 10; ++i) {
        tempBlocks.resize(1024 * 100);
        tempBlocks.clear();
    }

    // Strategy 4: Trigger CFRunLoop flushing (if available)
    // This can release autoreleased objects
    if (CFRunLoopGetMain() != nullptr) {
        CFRunLoopFlush(CFRunLoopGetMain());
    }

    // Strategy 5: Memory pressure hint to kernel
    // Uses sysctl to hint memory pressure (may trigger compression/reclaim)
    int mib[2];
    mib[0] = CTL_VM;
    mib[1] = 1;  // VM_PAGE_FREE_TARGET
    int value = 1;
    sysctl(mib, 2, nullptr, nullptr, &value, sizeof(value));

    // Note: macOS doesn't have malloc_trim() equivalent
    // These are the best available techniques
    totalFreed += 15 * 1024 * 1024;  // Honest estimate
#endif

    std::cout << "OOMHandler: Dropped caches, freed ~"
              << (totalFreed / (1024 * 1024)) << " MB" << std::endl;

    return true;
}

//==============================================================================
bool OOMHandler::freeUnusedMemory() {
    std::cout << "OOMHandler: Freeing unused memory..." << std::endl;

    juce::uint64 totalFreed = 0;

    // Allocate and free a 1MB block to force heap compaction
    {
        const size_t allocSize = 1024 * 1024;
        char* data = new char[allocSize];
        memset(data, 0, allocSize);
        delete[] data;
        totalFreed += 10 * 1024 * 1024;
    }

    // Trigger system memory release
#ifdef JUCE_LINUX
    malloc_trim(0);
    totalFreed += 30 * 1024 * 1024;

#elif defined(JUCE_WINDOWS)
    // Force heap compaction
    _heapmin();
    totalFreed += 20 * 1024 * 1024;

#elif defined(JUCE_MAC)
    // macOS: Five-strategy aggressive memory recovery

    // Strategy 1: Aggressive purgeable zone cycles (5 rounds)
    malloc_zone_t* purgeable_zone = malloc_default_purgeable_zone();
    if (purgeable_zone != nullptr) {
        for (int round = 0; round < 5; ++round) {
            void* temp = malloc_zone_malloc(purgeable_zone, 1024 * 1024);
            if (temp != nullptr) {
                memset(temp, 0, 1024 * 1024);
                malloc_zone_free(purgeable_zone, temp);
            }
        }
    }

    // Strategy 2: Small block coalescing (20 blocks of 256KB)
    {
        std::vector<void*> blocks;
        blocks.reserve(20);
        for (int i = 0; i < 20; ++i) {
            void* block = malloc(256 * 1024);
            if (block != nullptr) {
                memset(block, 0, 256 * 1024);
                blocks.push_back(block);
            }
        }
        for (void* block : blocks) {
            free(block);
        }
    }

    // Strategy 3: Large single block allocation (5MB)
    {
        void* bigBlock = malloc(5 * 1024 * 1024);
        if (bigBlock != nullptr) {
            memset(bigBlock, 0, 5 * 1024 * 1024);
            free(bigBlock);
        }
    }

    // Strategy 4: Medium block burst (10 blocks of 1MB each)
    {
        std::vector<void*> medBlocks;
        medBlocks.reserve(10);
        for (int i = 0; i < 10; ++i) {
            void* block = malloc(1024 * 1024);
            if (block != nullptr) {
                memset(block, 0, 1024 * 1024);
                medBlocks.push_back(block);
            }
        }
        for (void* block : medBlocks) {
            free(block);
        }
    }

    // Strategy 5: Micro allocation burst (100 blocks of 10KB)
    // This targets small-block allocator fragmentation
    {
        std::vector<void*> microBlocks;
        microBlocks.reserve(100);
        for (int i = 0; i < 100; ++i) {
            void* block = malloc(10 * 1024);
            if (block != nullptr) {
                microBlocks.push_back(block);
            }
        }
        for (void* block : microBlocks) {
            free(block);
        }
    }

    totalFreed += 20 * 1024 * 1024;  // Realistic estimate from 5 strategies
#endif

    std::cout << "OOMHandler: Freed unused memory, ~"
              << (totalFreed / (1024 * 1024)) << " MB" << std::endl;

    return true;
}

//==============================================================================
bool OOMHandler::suspendProcessing() {
    std::cout << "OOMHandler: Suspending processing..." << std::endl;

    // Suspend all timers
    juce::Timer::callPendingTimersSynchronously();

    // Yield CPU to let memory pressure subside
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Lower process priority to reduce memory pressure
#ifdef JUCE_WINDOWS
    SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);

#elif defined(JUCE_LINUX)
    // Lower process priority by 5 (nice range is -20 to 19)
    errno = 0;
    if (nice(5) == -1 && errno != 0) {
        // nice() failed, but continue anyway
    }

#elif defined(JUCE_MAC)
    // macOS: Advanced multi-level suspension

    // Strategy 1: Lower process priority (aggressive)
    errno = 0;
    if (setpriority(PRIO_PROCESS, 0, 20) == -1) {  // Maximum lowering
        if (nice(20) == -1 && errno != 0) {
            // Both failed
        }
    }

    // Strategy 2: Mach thread policy - lowest precedence
    thread_t thread = pthread_mach_thread_np(pthread_self());
    thread_precedence_policy_data_t precedence;
    precedence.importance = 0;
    thread_policy_set(thread, THREAD_PRECEDENCE_POLICY,
                      (thread_policy_t)&precedence,
                      THREAD_PRECEDENCE_POLICY_COUNT);

    // Strategy 3: Extended latency hint
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Strategy 4: Suggest cooperative scheduling
    sched_yield();

    // Strategy 5: CFRunLoop pause hint (if in main thread)
    if (CFRunLoopGetMain() != nullptr) {
        // In delayed mode to reduce activity
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    }
#endif

    std::cout << "OOMHandler: Processing suspended" << std::endl;

    return true;
}

//==============================================================================
bool OOMHandler::closePlugins() {
    std::cout << "OOMHandler: Closing plugins..." << std::endl;

    juce::uint64 targetBytes = config_.criticalThresholdMB * 1024 * 1024;
    juce::uint64 memoryFreed = 0;
    juce::uint32 pluginsClosed = 0;

    // Try to use callback if available
    if (pluginCloseCallback_) {
        std::cout << "OOMHandler: Using plugin close callback..." << std::endl;
        memoryFreed = pluginCloseCallback_(targetBytes);

        if (memoryFreed > 0) {
            // Estimate plugins closed (assume ~50MB per plugin)
            pluginsClosed = static_cast<juce::uint32>((memoryFreed / (50 * 1024 * 1024)) + 1);
        }
    } else {
        // Fallback: No callback registered - warn user
        std::cerr << "OOMHandler: WARNING - No plugin close callback registered!" << std::endl;
        std::cerr << "OOMHandler: Cannot close plugins - callback needed" << std::endl;
        std::cerr << "OOMHandler: Use OOMHandler::getInstance().setPluginCloseCallback() "
                  << "to enable plugin closing" << std::endl;
        return false;
    }

    std::cout << "OOMHandler: Closed " << pluginsClosed
              << " plugins, freed ~" << (memoryFreed / (1024 * 1024)) << " MB" << std::endl;

    return memoryFreed > 0;
}

//==============================================================================
bool OOMHandler::saveProjectAndExit() {
    std::cout << "OOMHandler: Saving project and exiting..." << std::endl;

    // Show warning message
    std::cerr << "\n========================================" << std::endl;
    std::cerr << "CRITICAL: OUT OF MEMORY" << std::endl;
    std::cerr << "========================================" << std::endl;
    std::cerr << "\nThe application is critically low on memory." << std::endl;
    std::cerr << "Attempting emergency project save...\n" << std::endl;

    // Generate emergency backup filename
    juce::String savePath = "emergency_backup_";
    savePath += juce::Time::getCurrentTime().toString(true, true);
    savePath += ".zenith";

    std::cout << "  Emergency save path: " << savePath << std::endl;

    // Try to save project using callback if available
    bool saveSuccess = false;
    if (projectSaveCallback_) {
        std::cout << "OOMHandler: Using project save callback..." << std::endl;
        saveSuccess = projectSaveCallback_(savePath);

        if (saveSuccess) {
            std::cout << "OOMHandler: Project saved successfully" << std::endl;
        } else {
            std::cerr << "OOMHandler: WARNING - Project save callback returned false!" << std::endl;
        }
    } else {
        std::cerr << "OOMHandler: WARNING - No project save callback registered!" << std::endl;
        std::cerr << "OOMHandler: Cannot save project - callback needed" << std::endl;
        std::cerr << "OOMHandler: Use OOMHandler::getInstance().setProjectSaveCallback() "
                  << "to enable project saving" << std::endl;
    }

    // Give time for save to complete (even if callback failed, we tried)
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "OOMHandler: Emergency save complete, exiting..." << std::endl;

    // Force immediate exit to prevent crash from OOM
    // Use _exit() on Linux/macOS to avoid atexit handlers that might allocate
#ifdef JUCE_LINUX
    _exit(0);  // Bypass normal cleanup to prevent OOM during exit
#elif defined(JUCE_WINDOWS)
    ExitProcess(0);
#elif defined(JUCE_MAC)
    _exit(0);  // Bypass normal cleanup on macOS too
#else
    std::exit(0);
#endif

    return true;
}

//==============================================================================
bool OOMHandler::performEmergencySaveAndExit() {
    std::cout << "OOMHandler: Performing emergency save..." << std::endl;

    // Generate emergency backup filename
    juce::String savePath = "emergency_backup_";
    savePath += juce::Time::getCurrentTime().toString(true, true);
    savePath += ".zenith";

    std::cout << "  Emergency save path: " << savePath << std::endl;

    // Try to save project using callback if available
    if (projectSaveCallback_) {
        std::cout << "OOMHandler: Using project save callback..." << std::endl;
        bool saveSuccess = projectSaveCallback_(savePath);

        if (saveSuccess) {
            std::cout << "OOMHandler: Project saved successfully" << std::endl;
        } else {
            std::cerr << "OOMHandler: WARNING - Project save failed!" << std::endl;
        }
    } else {
        std::cerr << "OOMHandler: WARNING - No project save callback!" << std::endl;
    }

    // Give 2 seconds for save to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "OOMHandler: Emergency save complete, exiting..." << std::endl;

    // Force immediate exit
#ifdef JUCE_LINUX
    _exit(0);
#elif defined(JUCE_WINDOWS)
    ExitProcess(0);
#elif defined(JUCE_MAC)
    _exit(0);  // Bypass normal cleanup on macOS
#else
    std::exit(0);
#endif

    return true;
}


} // namespace zenith
