/*
  ==============================================================================

    UIDispatcher.h
    Created: 2026-02-19
    Month 11, Gap #2 - UI Thread Dispatcher

    Thread-safe UI message dispatcher for audio-to-UI communication.

  ==============================================================================
*/

#pragma once

#include "ThreadSafeQueue.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <atomic>
#include <memory>

namespace zenith {

//==============================================================================
/**
 * @brief UI callback priority
 */
enum class UIPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

//==============================================================================
/**
 * @brief UI message type
 */
enum class UIMessageType {
    Generic,
    ParameterChange,
    StateChange,
    Notification,
    Warning,
    Error
};

//==============================================================================
/**
 * @brief UI message
 */
struct UIMessage {
    UIMessageType type = UIMessageType::Generic;
    juce::String senderId;
    juce::String message;
    std::function<void()> callback;
    UIPriority priority = UIPriority::Normal;
    juce::Time timestamp;

    UIMessage() : timestamp(juce::Time::getCurrentTime()) {}

    bool isValid() const {
        return callback != nullptr || !message.isEmpty();
    }
};

//==============================================================================
/**
 * @brief Dispatcher configuration
 */
struct UIDispatcherConfig {
    juce::uint32 maxQueueSize = 1000;          // Maximum messages in queue
    juce::uint32 processingIntervalMs = 16;    // Process every ~16ms (60fps)
    bool enableLogging = false;                // Log all messages
    bool enableStatistics = true;              // Track statistics
};

//==============================================================================
/**
 * @brief UI thread dispatcher for thread-safe UI updates
 *
 * Features:
 * - Thread-safe message passing from any thread to UI thread
 * - Priority-based message handling
 * - Deferred callback execution on UI thread
 * - Automatic message throttling
 * - Statistics tracking
 *
 * Usage:
 * ```cpp
 * // From audio thread or any other thread:
 * UIDispatcher::getInstance().callAsync([]() {
 *     // This runs on UI thread
 *     someLabel->setText("Updated");
 * });
 *
 * // Or with priority:
 * UIDispatcher::getInstance().callAsync([]() {
 *     updateCriticalUI();
 * }, UIPriority::High);
 * ```
 */
class UIDispatcher : private juce::Timer {
public:
    //==========================================================================
    explicit UIDispatcher(const UIDispatcherConfig& config = {});
    ~UIDispatcher();

    //==========================================================================
    /**
     * @brief Call function on UI thread asynchronously
     */
    void callAsync(std::function<void()> callback);

    //==========================================================================
    /**
     * @brief Call function on UI thread with priority
     */
    void callAsync(std::function<void()> callback, UIPriority priority);

    //==========================================================================
    /**
     * @brief Call function on UI thread synchronously (blocks caller)
     * WARNING: Only use from non-UI threads!
     */
    void callSynchronous(std::function<void()> callback);

    //==========================================================================
    /**
     * @brief Post UI message
     */
    void postMessage(const UIMessage& message);

    //==========================================================================
    /**
     * @brief Post parameter change notification
     */
    void postParameterChange(const juce::String& parameterId,
                            float newValue,
                            const juce::String& senderId = "");

    //==========================================================================
    /**
     * @brief Post notification (user-facing)
     */
    void postNotification(const juce::String& text,
                         UIMessageType type = UIMessageType::Notification);

    //==========================================================================
    /**
     * @brief Process pending messages
     * Called automatically by timer, but can be called manually
     */
    void processMessages();

    //==========================================================================
    /**
     * @brief Check if calling from UI thread
     */
    bool isUIThread() const;

    //==========================================================================
    /**
     * @brief Assert that we're on UI thread
     */
    void assertUIThread() const;

    //==========================================================================
    /**
     * @brief Get pending message count
     */
    size_t getPendingCount() const;

    //==========================================================================
    /**
     * @brief Get statistics
     */
    struct Statistics {
        juce::uint64 totalMessages = 0;
        juce::uint64 totalCallbacks = 0;
        juce::uint64 droppedMessages = 0;
        double averageProcessingTimeMs = 0.0;
        juce::Time lastProcessTime;

        juce::String toString() const {
            return juce::String::formatted(
                "Messages: %llu, Callbacks: %llu, Dropped: %llu, Avg Time: %.2f ms",
                totalMessages,
                totalCallbacks,
                droppedMessages,
                averageProcessingTimeMs
            );
        }
    };

    Statistics getStatistics() const;

    //==========================================================================
    /**
     * @brief Clear all pending messages
     */
    void clear();

    //==========================================================================
    /**
     * @brief Enable/disable processing
     */
    void setProcessingEnabled(bool enabled);

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static UIDispatcher& getInstance();

private:
    //==========================================================================
    void timerCallback() override;
    void processQueue();
    void processPriorityQueue();

    UIDispatcherConfig config_;
    FunctionQueue normalQueue_;
    PriorityQueue<std::function<void()>, int> priorityQueue_;

    // Statistics
    std::atomic<juce::uint64> totalMessages_{0};
    std::atomic<juce::uint64> totalCallbacks_{0};
    std::atomic<juce::uint64> droppedMessages_{0};
    juce::uint64 totalProcessingTimeUs_ = 0;

    mutable std::mutex statsMutex_;
    juce::Time lastProcessTime_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIDispatcher)
};

//==============================================================================
/**
 * @brief RAII helper for UI thread assertion
 */
class UIThreadGuard {
public:
    UIThreadGuard()
        : dispatcher_(UIDispatcher::getInstance()) {

        dispatcher_.assertUIThread();
    }

    ~UIThreadGuard() = default;

private:
    UIDispatcher& dispatcher_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIThreadGuard)
};

//==============================================================================
/**
 * @ Convenience macro to assert UI thread
 */
#define JUCE_ASSERT_UI_THREAD_UIDISPATCH() \
    zenith::UIDispatcher::getInstance().assertUIThread()

} // namespace zenith
