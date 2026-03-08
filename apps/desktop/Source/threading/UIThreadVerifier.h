/*
  ==============================================================================

    UIThreadGuard.h
    Created: 2026-02-19
    Month 11, Gap #3 - UI Thread Verification

    Thread verification and UI violation detection.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <map>
#include <mutex>

namespace zenith {

//==============================================================================
/**
 * @brief Thread violation information
 */
struct ThreadViolation {
    juce::String violatedFunction;
    juce::String expectedThread;
    juce::String actualThread;
    juce::Time timestamp;
    juce::String stackTrace;

    juce::String toString() const {
        return juce::String::formatted(
            "Thread violation in %s: Expected %s, got %s at %s",
            violatedFunction,
            expectedThread,
            actualThread,
            timestamp.toString(true, true)
        );
    }
};

//==============================================================================
/**
 * @brief Thread types
 */
enum class ThreadType {
    Unknown,
    UIThread,
    AudioThread,
    BackgroundThread,
    NetworkThread
};

//==============================================================================
/**
 * @brief Thread guard configuration
 */
struct ThreadGuardConfig {
    bool enableStrictChecking = true;       // Assert on violations
    bool enableLogging = true;              // Log all violations
    bool enableStackTrace = false;          // Capture stack traces (expensive)
    bool violationThreshold = 5;            // Max violations before action
};

//==============================================================================
/**
 * @brief UI thread guard - verifies thread safety rules
 *
 * Features:
 * - Detects UI thread violations
 * - Thread identification
 * - Violation tracking
 * - Assert on violations (optional)
 * - Violation callbacks
 *
 * Usage:
 * ```cpp
 * // At start of UI-only function:
 * UIThreadVerifier::getInstance().verifyUIThread();
 *
 * // Or use RAII:
 * UIThreadScope scope;  // Asserts construction is on UI thread
 * ```
 */
class UIThreadVerifier {
public:
    //==========================================================================
    explicit UIThreadVerifier(const ThreadGuardConfig& config = {});
    ~UIThreadVerifier();

    //==========================================================================
    /**
     * @brief Verify current thread is UI thread
     * @return true if on UI thread
     */
    bool verifyUIThread(const juce::String& functionName = "");

    //==========================================================================
    /**
     * @brief Verify current thread is audio thread
     */
    bool verifyAudioThread(const juce::String& functionName = "");

    //==========================================================================
    /**
     * @brief Get current thread type
     */
    ThreadType getCurrentThreadType() const;

    //==========================================================================
    /**
     * @brief Get thread name for type
     */
    static juce::String getThreadName(ThreadType type);

    //==========================================================================
    /**
     * @brief Get thread ID as string
     */
    static juce::String getCurrentThreadId();

    //==========================================================================
    /**
     * @brief Check if on UI thread
     */
    static bool isUIThread();

    //==========================================================================
    /**
     * @brief Check if on audio thread
     */
    static bool isAudioThread();

    //==========================================================================
    /**
     * @brief Register audio thread
     */
    void registerAudioThread(juce::Thread::ThreadID threadId);

    //==========================================================================
    /**
     * @brief Unregister audio thread
     */
    void unregisterAudioThread(juce::Thread::ThreadID threadId);

    //==========================================================================
    /**
     * @brief Get violation count
     */
    size_t getViolationCount() const;

    //==========================================================================
    /**
     * @brief Get violation history
     */
    std::vector<ThreadViolation> getViolations() const;

    //==========================================================================
    /**
     * @brief Clear violations
     */
    void clearViolations();

    //==========================================================================
    /**
     * @brief Set violation callback
     */
    using ViolationCallback = std::function<void(const ThreadViolation&)>;
    void setViolationCallback(ViolationCallback callback);

    //==========================================================================
    /**
     * @brief Enable/disable strict mode
     */
    void setStrictMode(bool enabled);

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static UIThreadVerifier& getInstance();

private:
    //==========================================================================
    ThreadGuardConfig config_;
    std::atomic<bool> strictMode_{true};

    // Registered audio threads
    std::set<juce::Thread::ThreadID> audioThreads_;
    mutable std::mutex audioThreadsMutex_;

    // Violation tracking
    std::vector<ThreadViolation> violations_;
    mutable std::mutex violationsMutex_;
    std::atomic<size_t> violationCount_{0};

    // Callback
    ViolationCallback violationCallback_;

    //==========================================================================
    void recordViolation(const juce::String& function,
                       ThreadType expected,
                       ThreadType actual);

    juce::String captureStackTrace() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIThreadVerifier)
};

//==============================================================================
/**
 * @brief RAII UI thread scope guard
 *
 * Verifies that the scope is entered on UI thread.
 * Useful for protecting UI-only functions.
 */
class UIThreadScope {
public:
    explicit UIThreadScope(const juce::String& functionName = "")
        : functionName_(functionName)
        , verifier_(UIThreadVerifier::getInstance()) {

        verified_ = verifier_.verifyUIThread(functionName);
    }

    ~UIThreadScope() {
        // Optional: Re-verify on destruction
    }

    bool wasVerified() const { return verified_; }

private:
    juce::String functionName_;
    UIThreadVerifier& verifier_;
    bool verified_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIThreadScope)
};

//==============================================================================
/**
 * @brief RAII audio thread scope guard
 */
class AudioThreadScope {
public:
    explicit AudioThreadScope(const juce::String& functionName = "")
        : functionName_(functionName)
        , verifier_(UIThreadVerifier::getInstance()) {

        verified_ = verifier_.verifyAudioThread(functionName);
    }

    ~AudioThreadScope() = default;

    bool wasVerified() const { return verified_; }

private:
    juce::String functionName_;
    UIThreadVerifier& verifier_;
    bool verified_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioThreadScope)
};

//==============================================================================
// Convenience macros
#define VERIFY_UI_THREAD() \
    zenith::UIThreadVerifier::getInstance().verifyUIThread(JUCE_CURRENT_FUNCTION)

#define VERIFY_AUDIO_THREAD() \
    zenith::UIThreadVerifier::getInstance().verifyAudioThread(JUCE_CURRENT_FUNCTION)

#define UI_SCOPE() \
    zenith::UIThreadScope _ui_scope(JUCE_CURRENT_FUNCTION)

#define AUDIO_SCOPE() \
    zenith::AudioThreadScope _audio_scope(JUCE_CURRENT_FUNCTION)

} // namespace zenith
