/*
  ==============================================================================

    AsyncOperation.h
    Created: 2026-02-19
    Month 11, Gap #4 - Async Operations

    Safe asynchronous operations with cancellation support.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <future>
#include <atomic>
#include <memory>
#include <mutex>

namespace zenith {

//==============================================================================
/**
 * @brief Operation state
 */
enum class OperationState {
    Pending,
    Running,
    Completed,
    Cancelled,
    Failed
};

//==============================================================================
/**
 * @brief Async operation result
 */
template <typename T>
struct OperationResult {
    T value;
    bool succeeded = false;
    juce::String errorMessage;
    OperationState state = OperationState::Completed;

    bool isReady() const {
        return state == OperationState::Completed ||
               state == OperationState::Cancelled ||
               state == OperationState::Failed;
    }
};

//==============================================================================
// Specialization for void operations
template <>
struct OperationResult<void> {
    bool succeeded = false;
    juce::String errorMessage;
    OperationState state = OperationState::Completed;

    bool isReady() const {
        return state == OperationState::Completed ||
               state == OperationState::Cancelled ||
               state == OperationState::Failed;
    }
};

//==============================================================================
/**
 * @brief Cancellation token
 */
class CancellationToken {
public:
    //==========================================================================
    CancellationToken() : cancelled_(std::make_shared<std::atomic<bool>>(false)) {}

    //==========================================================================
    static CancellationToken create() {
        return CancellationToken();
    }

    //==========================================================================
    void cancel() {
        cancelled_->store(true);
    }

    //==========================================================================
    bool isCancelled() const {
        return cancelled_->load();
    }

    //==========================================================================
    bool operator==(const CancellationToken& other) const {
        return cancelled_ == other.cancelled_;
    }

private:
    //==========================================================================
    std::shared_ptr<std::atomic<bool>> cancelled_;
};

//==============================================================================
/**
 * @brief Async operation with future/promise pattern
 *
 * Features:
 * - Non-blocking execution
 * - Cancellation support
 * - Result callback
 * - Exception handling
 * - Thread-safe
 */
template <typename T>
class AsyncOperation {
public:
    //==========================================================================
    using ResultCallback = std::function<void(const OperationResult<T>&)>;
    using WorkFunction = std::function<T(CancellationToken)>;

    //==========================================================================
    AsyncOperation() = default;
    AsyncOperation(const AsyncOperation&) = delete;
    AsyncOperation& operator=(const AsyncOperation&) = delete;

    AsyncOperation(AsyncOperation&& other) noexcept
        : future_(std::move(other.future_)),
          state_(other.state_.load()),
          token_(other.token_),
          result_(std::move(other.result_)) {}

    AsyncOperation& operator=(AsyncOperation&& other) noexcept {
        if (this != &other) {
            future_ = std::move(other.future_);
            state_.store(other.state_.load());
            token_ = other.token_;
            result_ = std::move(other.result_);
        }
        return *this;
    }

    //==========================================================================
    /**
     * @brief Execute work asynchronously
     */
    void execute(WorkFunction work, ResultCallback callback = nullptr) {
        // Reset state
        state_.store(OperationState::Running);
        token_ = CancellationToken();
        result_ = OperationResult<T>();

        // Launch async task
        future_ = std::async(std::launch::async, [this, work, callback]() {
            try {
                // Check if cancelled before starting
                if (token_.isCancelled()) {
                    state_.store(OperationState::Cancelled);
                    if (callback) {
                        result_.state = OperationState::Cancelled;
                        callOnUIThread(callback);
                    }
                    return result_;
                }

                // Execute work
                T value = work(token_);

                // Check cancellation during work
                if (token_.isCancelled()) {
                    state_.store(OperationState::Cancelled);
                    if (callback) {
                        result_.state = OperationState::Cancelled;
                        callOnUIThread(callback);
                    }
                    return result_;
                }

                // Success
                result_.value = value;
                result_.succeeded = true;
                result_.state = OperationState::Completed;
                state_.store(OperationState::Completed);

                if (callback) {
                    callOnUIThread(callback);
                }

            } catch (const std::exception& e) {
                // Failure
                result_.errorMessage = e.what();
                result_.state = OperationState::Failed;
                state_.store(OperationState::Failed);

                if (callback) {
                    callOnUIThread(callback);
                }
            }

            return result_;
        });
    }

    //==========================================================================
    /**
     * @brief Cancel the operation
     */
    void cancel() {
        token_.cancel();
        state_.store(OperationState::Cancelled);
    }

    //==========================================================================
    /**
     * @brief Check if operation is done
     */
    bool isDone() const {
        auto s = state_.load();
        return s == OperationState::Completed ||
               s == OperationState::Cancelled ||
               s == OperationState::Failed;
    }

    //==========================================================================
    /**
     * @brief Check if operation is cancelled
     */
    bool isCancelled() const {
        return state_.load() == OperationState::Cancelled ||
               token_.isCancelled();
    }

    //==========================================================================
    /**
     * @brief Get current state
     */
    OperationState getState() const {
        return state_.load();
    }

    //==========================================================================
    /**
     * @brief Get result (blocks if not ready)
     */
    OperationResult<T> getResult() {
        if (future_.valid()) {
            future_.wait();
            return result_;
        }
        return OperationResult<T>{};
    }

    //==========================================================================
    /**
     * @brief Try to get result without blocking
     */
    juce::Optional<OperationResult<T>> tryGetResult() {
        if (isDone() && future_.valid()) {
            return result_;
        }
        return juce::Optional<OperationResult<T>>();
    }

    //==========================================================================
    /**
     * @brief Get cancellation token
     */
    CancellationToken getToken() const {
        return token_;
    }

private:
    //==========================================================================
    std::future<OperationResult<T>> future_;
    std::atomic<OperationState> state_{OperationState::Pending};
    CancellationToken token_;
    OperationResult<T> result_;

    //==========================================================================
    void callOnUIThread(ResultCallback callback) {
        // Ensure callback runs on UI thread
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
            callback(result_);
        } else {
            juce::MessageManager::callAsync([callback, result = result_]() {
                callback(result);
            });
        }
    }
};

//==============================================================================
/**
 * @brief Specialization for void operations
 */
template <>
class AsyncOperation<void> {
public:
    //==========================================================================
    using ResultCallback = std::function<void(const OperationResult<void>&)>;
    using WorkFunction = std::function<void(CancellationToken)>;

    //==========================================================================
    AsyncOperation() = default;
    AsyncOperation(const AsyncOperation&) = delete;
    AsyncOperation& operator=(const AsyncOperation&) = delete;

    AsyncOperation(AsyncOperation&& other) noexcept
        : future_(std::move(other.future_)),
          state_(other.state_.load()),
          token_(other.token_),
          result_(std::move(other.result_)) {}

    AsyncOperation& operator=(AsyncOperation&& other) noexcept {
        if (this != &other) {
            future_ = std::move(other.future_);
            state_.store(other.state_.load());
            token_ = other.token_;
            result_ = std::move(other.result_);
        }
        return *this;
    }

    //==========================================================================
    void execute(WorkFunction work, ResultCallback callback = nullptr) {
        state_.store(OperationState::Running);
        token_ = CancellationToken();
        result_ = OperationResult<void>{};

        future_ = std::async(std::launch::async, [this, work, callback]() {
            try {
                if (token_.isCancelled()) {
                    state_.store(OperationState::Cancelled);
                    if (callback) {
                        result_.state = OperationState::Cancelled;
                        callOnUIThread(callback);
                    }
                    return result_;
                }

                work(token_);

                if (token_.isCancelled()) {
                    state_.store(OperationState::Cancelled);
                    if (callback) {
                        result_.state = OperationState::Cancelled;
                        callOnUIThread(callback);
                    }
                    return result_;
                }

                result_.succeeded = true;
                result_.state = OperationState::Completed;
                state_.store(OperationState::Completed);

                if (callback) {
                    callOnUIThread(callback);
                }

            } catch (const std::exception& e) {
                result_.errorMessage = e.what();
                result_.state = OperationState::Failed;
                state_.store(OperationState::Failed);

                if (callback) {
                    callOnUIThread(callback);
                }
            }

            return result_;
        });
    }

    //==========================================================================
    void cancel() {
        token_.cancel();
        state_.store(OperationState::Cancelled);
    }

    //==========================================================================
    bool isDone() const {
        auto s = state_.load();
        return s == OperationState::Completed ||
               s == OperationState::Cancelled ||
               s == OperationState::Failed;
    }

    //==========================================================================
    OperationState getState() const {
        return state_.load();
    }

private:
    //==========================================================================
    std::future<OperationResult<void>> future_;
    std::atomic<OperationState> state_{OperationState::Pending};
    CancellationToken token_;
    OperationResult<void> result_;

    void callOnUIThread(ResultCallback callback) {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
            callback(result_);
        } else {
            juce::MessageManager::callAsync([callback, result = result_]() {
                callback(result);
            });
        }
    }
};

//==============================================================================
/**
 * @·brief Utility functions for common async operations
 */
namespace AsyncOps {
    //==========================================================================
    /**
     * @brief Delay execution by specified milliseconds
     */
    inline AsyncOperation<void> delay(juce::uint32 milliseconds) {
        AsyncOperation<void> operation;
        operation.execute(
            [milliseconds](CancellationToken token) {
                auto start = std::chrono::steady_clock::now();
                auto duration = std::chrono::milliseconds(milliseconds);

                while (!token.isCancelled()) {
                    auto elapsed = std::chrono::steady_clock::now() - start;
                    if (elapsed >= duration) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            },
            nullptr
        );
        return std::move(operation);
    }

    //==========================================================================
    /**
     * @brief Execute function on background thread
     */
    template <typename T>
    inline AsyncOperation<T> run(std::function<T()> func) {
        AsyncOperation<T> operation;
        operation.execute(
            [func](CancellationToken token) {
                return func();
            },
            nullptr
        );
        return std::move(operation);
    }

    //==========================================================================
    /**
     * @brief Execute function on background thread with callback
     */
    template <typename T>
    inline AsyncOperation<T> run(std::function<T()> func,
                                 typename AsyncOperation<T>::ResultCallback callback) {
        AsyncOperation<T> operation;
        operation.execute(
            [func](CancellationToken token) {
                return func();
            },
            callback
        );
        return std::move(operation);
    }
}

} // namespace zenith
