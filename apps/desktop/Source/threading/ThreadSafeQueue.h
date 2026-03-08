/*
  ==============================================================================

    ThreadSafeQueue.h
    Created: 2026-02-19
    Month 11, Gap #1 - Thread-Safe Queue

    Thread-safe queue for inter-thread communication.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief Thread-safe queue with mutex protection
 *
 * Features:
 * - Fully thread-safe operations
 * - Blocking and non-blocking operations
 * - Condition variable for efficient waiting
 * - Batch operations
 * - Clear semantics
 *
 * Usage:
 * ```cpp
 * ThreadSafeQueue<int> queue;
 * queue.push(42);
 * int value = queue.pop();  // Blocks if empty
 *
 * if (auto val = queue.tryPop()) {
 *     // Use val.value()
 * }
 * ```
 */
template <typename T>
class ThreadSafeQueue {
public:
    //==========================================================================
    ThreadSafeQueue() = default;

    //==========================================================================
    /**
     * @brief Push value to queue (thread-safe)
     */
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        condition_.notify_one();
    }

    //==========================================================================
    /**
     * @brief Push value to queue (move semantics)
     */
    void push(T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        condition_.notify_one();
    }

    //==========================================================================
    /**
     * @brief Pop value from queue (blocks if empty)
     * @return Popped value
     */
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty(); });

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    //==========================================================================
    /**
     * @brief Try to pop without blocking
     * @return juce::Optional containing value, or empty if queue empty
     */
    juce::Optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return juce::Optional<T>();
        }

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    //==========================================================================
    /**
     * @brief Try to pop with timeout
     * @param timeoutMs Maximum time to wait in milliseconds
     * @return juce::Optional containing value, or empty if timeout
     */
    juce::Optional<T> tryPop(juce::uint32 timeoutMs) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (condition_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                               [this] { return !queue_.empty(); })) {
            T value = std::move(queue_.front());
            queue_.pop();
            return value;
        }

        return juce::Optional<T>();
    }

    //==========================================================================
    /**
     * @brief Check if queue is empty
     */
    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    //==========================================================================
    /**
     * @brief Get queue size
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    //==========================================================================
    /**
     * @brief Clear all items
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
    }

    //==========================================================================
    /**
     * @brief Pop multiple items at once
     */
    std::vector<T> popBatch(size_t maxItems) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<T> batch;
        batch.reserve(juce::jmin(maxItems, queue_.size()));

        while (!queue_.empty() && batch.size() < maxItems) {
            batch.push_back(std::move(queue_.front()));
            queue_.pop();
        }

        return batch;
    }

private:
    //==========================================================================
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<T> queue_;
};

//==============================================================================
/**
 * @brief Priority queue for thread-safe prioritized operations
 */
template <typename T, typename Priority = int>
class PriorityQueue {
public:
    //==========================================================================
    struct Item {
        T value;
        Priority priority;

        bool operator>(const Item& other) const {
            return priority > other.priority;  // Lower priority number = higher priority
        }
    };

    //==========================================================================
    PriorityQueue() = default;

    //==========================================================================
    /**
     * @brief Push with priority
     */
    void push(const T& value, Priority priority) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push({value, priority});
        condition_.notify_one();
    }

    //==========================================================================
    /**
     * @brief Pop highest priority item
     */
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty(); });

        T value = queue_.top().value;
        queue_.pop();
        return value;
    }

    //==========================================================================
    /**
     * @brief Try to pop without blocking
     */
    juce::Optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return juce::Optional<T>();
        }

        T value = queue_.top().value;
        queue_.pop();
        return value;
    }

    //==========================================================================
    /**
     * @brief Check if empty
     */
    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    //==========================================================================
    /**
     * @brief Get size
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    //==========================================================================
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue_;
};

//==============================================================================
/**
 * @brief Thread-safe function queue for deferred execution
 */
class FunctionQueue {
public:
    //==========================================================================
    using Function = std::function<void()>;

    //==========================================================================
    FunctionQueue() = default;

    //==========================================================================
    /**
     * @brief Add function to queue
     */
    void push(Function&& func) {
        queue_.push(std::move(func));
    }

    //==========================================================================
    /**
     * @brief Pop and execute function
     * @return true if function was executed, false if queue empty
     */
    bool popAndExecute() {
        auto func = queue_.tryPop();
        if (func.hasValue()) {
            (*func)();
            return true;
        }
        return false;
    }

    //==========================================================================
    /**
     * @brief Execute all pending functions
     * @return Number of functions executed
     */
    size_t executeAll() {
        size_t count = 0;
        while (popAndExecute()) {
            count++;
        }
        return count;
    }

    //==========================================================================
    /**
     * @brief Get number of pending functions
     */
    size_t size() const {
        return queue_.size();
    }

    //==========================================================================
    /**
     * @brief Check if empty
     */
    bool isEmpty() const {
        return queue_.isEmpty();
    }

    //==========================================================================
    /**
     * @brief Clear all pending functions
     */
    void clear() {
        queue_.clear();
    }

private:
    //==========================================================================
    ThreadSafeQueue<Function> queue_;
};

} // namespace zenith
