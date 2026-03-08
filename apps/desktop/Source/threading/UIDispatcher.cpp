/*
  ==============================================================================

    UIDispatcher.cpp
    Implementation of UI thread dispatcher

  ==============================================================================
*/

#include "UIDispatcher.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
UIDispatcher::UIDispatcher(const UIDispatcherConfig& config)
    : config_(config) {

    std::cout << "UIDispatcher: Initialized" << std::endl;
    std::cout << "  Max queue size: " << config_.maxQueueSize << std::endl;
    std::cout << "  Processing interval: " << config_.processingIntervalMs << " ms" << std::endl;

    startTimer(config_.processingIntervalMs);
}

//==============================================================================
UIDispatcher::~UIDispatcher() {
    stopTimer();

    if (config_.enableStatistics) {
        auto stats = getStatistics();
        std::cout << "UIDispatcher: Shut down" << std::endl;
        std::cout << "  " << stats.toString() << std::endl;
    }
}

//==============================================================================
void UIDispatcher::callAsync(std::function<void()> callback) {
    if (callback == nullptr) {
        return;
    }

    if (normalQueue_.size() >= config_.maxQueueSize) {
        droppedMessages_.fetch_add(1, std::memory_order_relaxed);
        std::cerr << "UIDispatcher: Queue full, message dropped" << std::endl;
        return;
    }

    normalQueue_.push(std::move(callback));
    totalMessages_.fetch_add(1, std::memory_order_relaxed);
}

//==============================================================================
void UIDispatcher::callAsync(std::function<void()> callback, UIPriority priority) {
    if (callback == nullptr) {
        return;
    }

    // Map UIPriority to int (lower number = higher priority)
    int priorityValue = static_cast<int>(priority);

    priorityQueue_.push(std::move(callback), priorityValue);
    totalMessages_.fetch_add(1, std::memory_order_relaxed);
}

//==============================================================================
void UIDispatcher::callSynchronous(std::function<void()> callback) {
    if (callback == nullptr) {
        return;
    }

    if (isUIThread()) {
        // Already on UI thread, execute directly
        callback();
        return;
    }

    // Not on UI thread - need to block until execution completes
    std::atomic<bool> executed{false};
    std::atomic<bool> ready{false};

    callAsync([&]() {
        callback();
        executed = true;
        ready = true;
    }, UIPriority::High);

    // Wait for completion (with timeout to avoid deadlock)
    juce::uint32 timeoutMs = 5000;  // 5 second timeout
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!ready.load()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime
        ).count();

        if (elapsed > timeoutMs) {
            std::cerr << "UIDispatcher: Synchronous call timeout!" << std::endl;
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//==============================================================================
void UIDispatcher::postMessage(const UIMessage& message) {
    if (!message.isValid()) {
        return;
    }

    if (message.callback != nullptr) {
        callAsync(message.callback, message.priority);
    }

    if (config_.enableLogging && !message.message.isEmpty()) {
        std::cout << "UIDispatcher: " << message.senderId
                  << " - " << message.message << std::endl;
    }
}

//==============================================================================
void UIDispatcher::postParameterChange(const juce::String& parameterId,
                                       float newValue,
                                       const juce::String& senderId) {
    UIMessage message;
    message.type = UIMessageType::ParameterChange;
    message.senderId = senderId;
    message.message = juce::String::formatted("Parameter %s changed to %.3f",
                                               parameterId, newValue);
    message.priority = UIPriority::Normal;

    postMessage(message);
}

//==============================================================================
void UIDispatcher::postNotification(const juce::String& text,
                                    UIMessageType type) {
    std::cerr << "UIDispatcher NOTIFICATION: " << text << std::endl;

    // TODO: Show in UI notification system
}

//==============================================================================
void UIDispatcher::processMessages() {
    processQueue();
    processPriorityQueue();

    lastProcessTime_ = juce::Time::getCurrentTime();
}

//==============================================================================
bool UIDispatcher::isUIThread() const {
    return juce::MessageManager::getInstance()->isThisTheMessageThread();
}

//==============================================================================
void UIDispatcher::assertUIThread() const {
    if (!isUIThread()) {
        std::cerr << "UIDispatcher: FATAL - UI thread assertion failed!" << std::endl;
        std::cerr << "  Current thread is NOT UI thread" << std::endl;

        // In debug builds, this should trigger a breakpoint
        jassertfalse;
    }
}

//==============================================================================
size_t UIDispatcher::getPendingCount() const {
    return normalQueue_.size() + priorityQueue_.size();
}

//==============================================================================
UIDispatcher::Statistics UIDispatcher::getStatistics() const {
    Statistics stats;
    stats.totalMessages = totalMessages_.load(std::memory_order_relaxed);
    stats.totalCallbacks = totalCallbacks_.load(std::memory_order_relaxed);
    stats.droppedMessages = droppedMessages_.load(std::memory_order_relaxed);
    stats.lastProcessTime = lastProcessTime_;

    if (stats.totalCallbacks > 0) {
        stats.averageProcessingTimeMs =
            static_cast<double>(totalProcessingTimeUs_) / stats.totalCallbacks / 1000.0;
    }

    return stats;
}

//==============================================================================
void UIDispatcher::clear() {
    normalQueue_.clear();

    std::cerr << "UIDispatcher: Cleared all pending messages" << std::endl;
}

//==============================================================================
void UIDispatcher::setProcessingEnabled(bool enabled) {
    if (enabled) {
        startTimer(config_.processingIntervalMs);
    } else {
        stopTimer();
    }

    std::cout << "UIDispatcher: Processing "
              << (enabled ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
UIDispatcher& UIDispatcher::getInstance() {
    static UIDispatcher instance;
    return instance;
}

//==============================================================================
void UIDispatcher::timerCallback() {
    processMessages();
}

//==============================================================================
void UIDispatcher::processQueue() {
    auto startTime = std::chrono::high_resolution_clock::now();

    size_t processed = normalQueue_.executeAll();
    totalCallbacks_.fetch_add(processed, std::memory_order_relaxed);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime
    );

    totalProcessingTimeUs_ += duration.count();
}

//==============================================================================
void UIDispatcher::processPriorityQueue() {
    auto startTime = std::chrono::high_resolution_clock::now();

    size_t processed = 0;
    const size_t maxPerFrame = 50;  // Process max 50 priority callbacks per frame

    while (processed < maxPerFrame) {
        auto callback = priorityQueue_.tryPop();
        if (!callback.hasValue()) {
            break;
        }

        (*callback)();
        processed++;
    }

    totalCallbacks_.fetch_add(processed, std::memory_order_relaxed);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime
    );

    totalProcessingTimeUs_ += duration.count();
}

} // namespace zenith
