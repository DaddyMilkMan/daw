/*
  ==============================================================================

    UIThreadVerifier.cpp
    Implementation of UI thread verifier

  ==============================================================================
*/

#include "UIThreadVerifier.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
UIThreadVerifier::UIThreadVerifier(const ThreadGuardConfig& config)
    : config_(config) {

    std::cout << "UIThreadVerifier: Initialized" << std::endl;
    std::cout << "  Strict checking: "
              << (config_.enableStrictChecking ? "enabled" : "disabled") << std::endl;
    std::cout << "  Logging: "
              << (config_.enableLogging ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
UIThreadVerifier::~UIThreadVerifier() {
    std::cout << "UIThreadVerifier: Shut down" << std::endl;

    if (violationCount_ > 0) {
        std::cout << "  Total violations: " << violationCount_ << std::endl;
    }
}

//==============================================================================
bool UIThreadVerifier::verifyUIThread(const juce::String& functionName) {
    ThreadType current = getCurrentThreadType();

    if (current != ThreadType::UIThread) {
        recordViolation(functionName.isEmpty() ? "UI function" : functionName,
                       ThreadType::UIThread,
                       current);
        return false;
    }

    return true;
}

//==============================================================================
bool UIThreadVerifier::verifyAudioThread(const juce::String& functionName) {
    ThreadType current = getCurrentThreadType();

    if (current != ThreadType::AudioThread) {
        recordViolation(functionName.isEmpty() ? "Audio function" : functionName,
                       ThreadType::AudioThread,
                       current);
        return false;
    }

    return true;
}

//==============================================================================
ThreadType UIThreadVerifier::getCurrentThreadType() const {
    if (isUIThread()) {
        return ThreadType::UIThread;
    }

    juce::Thread::ThreadID currentId = juce::Thread::getCurrentThreadId();

    std::lock_guard<std::mutex> lock(audioThreadsMutex_);
    if (audioThreads_.find(currentId) != audioThreads_.end()) {
        return ThreadType::AudioThread;
    }

    return ThreadType::BackgroundThread;
}

//==============================================================================
juce::String UIThreadVerifier::getThreadName(ThreadType type) {
    switch (type) {
        case ThreadType::UIThread: return "UI Thread";
        case ThreadType::AudioThread: return "Audio Thread";
        case ThreadType::BackgroundThread: return "Background Thread";
        case ThreadType::NetworkThread: return "Network Thread";
        case ThreadType::Unknown: return "Unknown Thread";
    }
    return "Unknown";
}

//==============================================================================
juce::String UIThreadVerifier::getCurrentThreadId() {
    return "0x" + juce::String::toHexString(
        reinterpret_cast<juce::uint64>(juce::Thread::getCurrentThreadId())
    );
}

//==============================================================================
bool UIThreadVerifier::isUIThread() {
    return juce::MessageManager::getInstance()->isThisTheMessageThread();
}

//==============================================================================
bool UIThreadVerifier::isAudioThread() {
    auto& instance = getInstance();
    juce::Thread::ThreadID currentId = juce::Thread::getCurrentThreadId();

    std::lock_guard<std::mutex> lock(instance.audioThreadsMutex_);
    return instance.audioThreads_.find(currentId) != instance.audioThreads_.end();
}

//==============================================================================
void UIThreadVerifier::registerAudioThread(juce::Thread::ThreadID threadId) {
    std::lock_guard<std::mutex> lock(audioThreadsMutex_);
    audioThreads_.insert(threadId);

    std::cout << "UIThreadVerifier: Registered audio thread "
              << "0x" + juce::String::toHexString(
                  reinterpret_cast<juce::uint64>(threadId)
              ) << std::endl;
}

//==============================================================================
void UIThreadVerifier::unregisterAudioThread(juce::Thread::ThreadID threadId) {
    std::lock_guard<std::mutex> lock(audioThreadsMutex_);
    audioThreads_.erase(threadId);
}

//==============================================================================
size_t UIThreadVerifier::getViolationCount() const {
    return violationCount_.load(std::memory_order_relaxed);
}

//==============================================================================
std::vector<ThreadViolation> UIThreadVerifier::getViolations() const {
    std::lock_guard<std::mutex> lock(violationsMutex_);
    return violations_;
}

//==============================================================================
void UIThreadVerifier::clearViolations() {
    std::lock_guard<std::mutex> lock(violationsMutex_);
    violations_.clear();
    violationCount_.store(0, std::memory_order_relaxed);

    std::cout << "UIThreadVerifier: Violations cleared" << std::endl;
}

//==============================================================================
void UIThreadVerifier::setViolationCallback(ViolationCallback callback) {
    violationCallback_ = std::move(callback);
}

//==============================================================================
void UIThreadVerifier::setStrictMode(bool enabled) {
    strictMode_.store(enabled);

    std::cout << "UIThreadVerifier: Strict mode "
              << (enabled ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
UIThreadVerifier& UIThreadVerifier::getInstance() {
    static UIThreadVerifier instance;
    return instance;
}

//==============================================================================
void UIThreadVerifier::recordViolation(const juce::String& function,
                                       ThreadType expected,
                                       ThreadType actual) {
    violationCount_.fetch_add(1, std::memory_order_relaxed);

    ThreadViolation violation;
    violation.violatedFunction = function;
    violation.expectedThread = getThreadName(expected);
    violation.actualThread = getThreadName(actual);
    violation.timestamp = juce::Time::getCurrentTime();

    if (config_.enableStackTrace) {
        violation.stackTrace = captureStackTrace();
    }

    {
        std::lock_guard<std::mutex> lock(violationsMutex_);
        violations_.push_back(violation);

        // Keep only last 1000 violations
        if (violations_.size() > 1000) {
            violations_.erase(violations_.begin());
        }
    }

    if (config_.enableLogging) {
        std::cerr << "UIThreadVerifier: VIOLATION - "
                  << violation.toString() << std::endl;
    }

    if (violationCallback_) {
        violationCallback_(violation);
    }

    if (strictMode_.load()) {
        // In debug builds, assert
        jassertfalse;
    }
}

//==============================================================================
juce::String UIThreadVerifier::captureStackTrace() const {
    // Platform-specific stack trace capture would go here
    // For now, return placeholder
    return "[Stack trace capture not implemented]";
}

} // namespace zenith
