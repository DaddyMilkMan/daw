/*
  ==============================================================================

    BufferManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #1)

    Dynamic audio buffer size management for XRUN prevention.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Buffer size change event
 */
struct BufferSizeEvent {
    int oldSize = 0;
    int newSize = 0;
    int sampleRate = 0;
    double timestamp = 0.0;
    juce::String reason;  // Why the change occurred

    juce::String toString() const {
        return "Buffer size: " + juce::String(oldSize) +
               " -> " + juce::String(newSize) +
               " (" + reason + ")";
    }
};

//==============================================================================
/**
 * @brief Buffer size configuration
 */
struct BufferSizeConfig {
    std::vector<int> availableSizes = {64, 128, 256, 512, 1024, 2048};
    int minSize = 64;
    int maxSize = 2048;
    int preferredSize = 512;  // User preference
    bool allowAutoIncrease = true;
    bool allowAutoDecrease = false;  // Don't auto-decrease (user preference)
    double decreaseDelaySeconds = 30.0;  // Wait before considering decrease
};

//==============================================================================
/**
 * @brief Buffer manager statistics
 */
struct BufferManagerStatistics {
    int currentSize = 0;
    int initialSize = 0;
    int totalChanges = 0;
    int autoIncreases = 0;
    int autoDecreases = 0;
    double lastChangeTime = 0.0;
    juce::String lastChangeReason;

    juce::String toString() const {
        return "Buffer Manager: " + juce::String(currentSize) +
               " samples (" + juce::String(totalChanges) + " changes)";
    }
};

//==============================================================================
/**
 * @brief Dynamic buffer size manager
 *
 * Features:
 * - Automatic buffer size increase based on CPU load
 * - Optional automatic decrease (with delay)
 * - User preference tracking
 * - Safe buffer size changes (no audio interruption)
 * - Change history and statistics
 * - Validation of buffer sizes
 */
class BufferManager {
public:
    //==========================================================================
    BufferManager();
    ~BufferManager();

    //==========================================================================
    /**
     * @brief Initialize with current buffer size
     */
    void initialize(int bufferSize, int sampleRate);

    //==========================================================================
    /**
     * @brief Request buffer size change
     * @param newSize Requested buffer size
     * @param reason Reason for change
     * @return true if change will be made
     */
    bool requestBufferSizeChange(int newSize, const juce::String& reason = "");

    //==========================================================================
    /**
     * @brief Increase buffer size to next available size
     * @param reason Reason for increase
     * @return New buffer size (or current if can't increase)
     */
    int increaseBufferSize(const juce::String& reason = "CPU overload");

    //==========================================================================
    /**
     * @brief Decrease buffer size to next smaller size
     * @param reason Reason for decrease
     * @return New buffer size (or current if can't decrease)
     */
    int decreaseBufferSize(const juce::String& reason = "CPU normal");

    //==========================================================================
    /**
     * @brief Get current buffer size
     */
    int getCurrentBufferSize() const {
        return currentBufferSize_;
    }

    //==========================================================================
    /**
     * @brief Set current buffer size (external change)
     * Use this when buffer size is changed by user or system
     */
    void setCurrentBufferSize(int size);

    //==========================================================================
    /**
     * @brief Get sample rate
     */
    int getSampleRate() const {
        return sampleRate_;
    }

    //==========================================================================
    /**
     * @brief Set sample rate
     */
    void setSampleRate(int rate) {
        sampleRate_ = rate;
    }

    //==========================================================================
    /**
     * @brief Get recommended buffer size
     * Based on current load and user preference
     */
    int getRecommendedBufferSize() const;

    //==========================================================================
    /**
     * @brief Check if buffer size can be increased
     */
    bool canIncrease() const;

    //==========================================================================
    /**
     * @brief Check if buffer size can be decreased
     */
    bool canDecrease() const;

    //==========================================================================
    /**
     * @brief Get next larger buffer size
     */
    int getNextLargerSize() const;

    //==========================================================================
    /**
     * @brief Get next smaller buffer size
     */
    int getNextSmallerSize() const;

    //==========================================================================
    /**
     * @brief Validate buffer size
     * @return true if size is valid
     */
    bool isValidBufferSize(int size) const;

    //==========================================================================
    /**
     * @brief Get configuration
     */
    BufferSizeConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const BufferSizeConfig& config) {
        config_ = config;
    }

    //==========================================================================
    /**
     * @brief Get available buffer sizes
     */
    std::vector<int> getAvailableSizes() const {
        return config_.availableSizes;
    }

    //==========================================================================
    /**
     * @brief Get change history
     */
    std::vector<BufferSizeEvent> getHistory() const {
        return history_;
    }

    //==========================================================================
    /**
     * @brief Clear history
     */
    void clearHistory() {
        history_.clear();
    }

    //==========================================================================
    /**
     * @brief Get statistics
     */
    BufferManagerStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Register callback for buffer size changes
     * @param callback Function to call when buffer size changes
     */
    void setChangeCallback(std::function<void(const BufferSizeEvent&)> callback) {
        changeCallback_ = callback;
    }

    //==========================================================================
    /**
     * @brief Enable/disable automatic size changes
     */
    void setAutoAdjustEnabled(bool enable) {
        autoAdjustEnabled_ = enable;
    }

    //==========================================================================
    /**
     * @brief Check if auto-adjust is enabled
     */
    bool isAutoAdjustEnabled() const {
        return autoAdjustEnabled_;
    }

private:
    //==========================================================================
    void recordChange(int oldSize, int newSize, const juce::String& reason);
    int findClosestValidSize(int size) const;

    //==========================================================================
    // Current state
    int currentBufferSize_ = 512;
    int sampleRate_ = 48000;

    // Configuration
    BufferSizeConfig config_;
    bool autoAdjustEnabled_ = true;

    // History
    std::vector<BufferSizeEvent> history_;
    static constexpr int maxHistorySize = 50;

    // Statistics
    BufferManagerStatistics statistics_;

    // Callbacks
    std::function<void(const BufferSizeEvent&)> changeCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BufferManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for buffer manager
 */
class BufferManagerHolder {
public:
    static BufferManager& getInstance() {
        static BufferManager instance;
        return instance;
    }

    BufferManagerHolder(const BufferManagerHolder&) = delete;
    BufferManagerHolder& operator=(const BufferManagerHolder&) = delete;

private:
    BufferManagerHolder() = default;
    ~BufferManagerHolder() = default;
};

} // namespace zenith
