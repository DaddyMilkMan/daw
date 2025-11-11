/**
 * @file WinRtAudioPriority.h
 * @brief RAII helper for Windows MMCSS (Multimedia Class Scheduler Service) audio thread priority
 *
 * Uses AvSetMmThreadCharacteristicsW to boost audio thread priority, preventing glitches
 * in low-latency scenarios. Safe fallback if MMCSS is unavailable.
 *
 * IMPORTANT: Only call from audio processing threads, NOT from GUI threads.
 */

#pragma once

#ifdef _WIN32

#include <JuceHeader.h>

//==============================================================================
/**
 * @class MMCSSAudioPriority
 * @brief RAII wrapper for MMCSS "Pro Audio" thread characteristics
 *
 * Usage:
 * ```cpp
 * void audioDeviceIOCallback(const float** inputChannelData, ...) {
 *     static MMCSSAudioPriority audioPriority("Pro Audio"); // Register once
 *     // ... process audio ...
 * }
 * ```
 *
 * Or for manual control:
 * ```cpp
 * MMCSSAudioPriority priority("Pro Audio");
 * if (!priority.isActive()) {
 *     DBG("MMCSS failed, audio may glitch under load");
 * }
 * // ... do real-time work ...
 * // priority.reset() called automatically on destruction
 * ```
 *
 * Performance notes:
 * - Minimal overhead: ~1 µs to register, no per-call cost
 * - Thread-local: only affects calling thread
 * - Safe to call multiple times (idempotent)
 */
class MMCSSAudioPriority
{
public:
    /**
     * @brief Registers current thread with MMCSS
     * @param taskName Task name: "Pro Audio" (< 10ms buffers) or "Audio" (>= 10ms)
     * @param priority Optional priority index (0 = highest, 7 = lowest). Default: 0
     *
     * "Pro Audio" provides the highest real-time priority suitable for sub-10ms latencies.
     * "Audio" provides slightly lower priority for >= 10ms buffers.
     */
    explicit MMCSSAudioPriority(const wchar_t* taskName = L"Pro Audio", DWORD priority = 0);

    /**
     * @brief Reverts thread priority to normal
     */
    ~MMCSSAudioPriority();

    // Non-copyable, non-movable (thread-local resource)
    MMCSSAudioPriority(const MMCSSAudioPriority&) = delete;
    MMCSSAudioPriority& operator=(const MMCSSAudioPriority&) = delete;

    /**
     * @brief Check if MMCSS is active for this thread
     * @return true if AvSetMmThreadCharacteristicsW succeeded, false if failed
     */
    bool isActive() const noexcept { return mmcssHandle != nullptr; }

    /**
     * @brief Get the task index assigned by MMCSS
     * @return Task index, or 0 if MMCSS is not active
     */
    DWORD getTaskIndex() const noexcept { return taskIndex; }

    /**
     * @brief Manually revert MMCSS (useful for testing)
     */
    void reset();

private:
    HANDLE mmcssHandle = nullptr;
    DWORD taskIndex = 0;

    // Function pointers (dynamically loaded from avrt.dll)
    using AvSetMmThreadCharacteristicsWFunc = HANDLE(WINAPI*)(LPCWSTR, LPDWORD);
    using AvSetMmThreadPriorityFunc = BOOL(WINAPI*)(HANDLE, AVRT_PRIORITY);
    using AvRevertMmThreadCharacteristicsFunc = BOOL(WINAPI*)(HANDLE);

    static AvSetMmThreadCharacteristicsWFunc avSetMmThreadCharacteristicsW;
    static AvSetMmThreadPriorityFunc avSetMmThreadPriority;
    static AvRevertMmThreadCharacteristicsFunc avRevertMmThreadCharacteristics;
    static bool mmcssAvailable;

    // Load avrt.dll once
    static void loadMMCSSFunctions();
};

#endif // _WIN32
