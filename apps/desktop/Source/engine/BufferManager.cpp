/*
  ==============================================================================

    BufferManager.cpp
    Implementation of dynamic buffer size management

  ==============================================================================
*/

#include "BufferManager.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// BufferManager Implementation
//==============================================================================

BufferManager::BufferManager()
    : currentBufferSize_(512)
{
    statistics_.currentSize = currentBufferSize_;
    statistics_.initialSize = currentBufferSize_;
    std::cout << "BufferManager: Initialized (buffer size: "
              << currentBufferSize_ << ")" << std::endl;
}

BufferManager::~BufferManager() {
    std::cout << "BufferManager: Shut down ("
              << statistics_.totalChanges << " changes)" << std::endl;
}

//==============================================================================
void BufferManager::initialize(int bufferSize, int sampleRate) {
    currentBufferSize_ = bufferSize;
    sampleRate_ = sampleRate;

    statistics_.currentSize = currentBufferSize_;
    statistics_.initialSize = currentBufferSize_;

    std::cout << "BufferManager: Initialized ("
              << bufferSize << " samples at "
              << sampleRate << " Hz)" << std::endl;
}

//==============================================================================
bool BufferManager::requestBufferSizeChange(int newSize, const juce::String& reason) {
    if (!autoAdjustEnabled_) {
        return false;
    }

    // Validate new size
    if (!isValidBufferSize(newSize)) {
        std::cerr << "BufferManager: Invalid buffer size: "
                  << newSize << std::endl;
        return false;
    }

    // Check if same as current
    if (newSize == currentBufferSize_) {
        return false;
    }

    // Record and make change
    int oldSize = currentBufferSize_;
    currentBufferSize_ = newSize;

    recordChange(oldSize, newSize, reason);

    // Notify callback
    if (changeCallback_) {
        BufferSizeEvent event;
        event.oldSize = oldSize;
        event.newSize = newSize;
        event.sampleRate = sampleRate_;
        event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
        event.reason = reason;

        changeCallback_(event);
    }

    return true;
}

//==============================================================================
int BufferManager::increaseBufferSize(const juce::String& reason) {
    if (!config_.allowAutoIncrease) {
        return currentBufferSize_;
    }

    int nextSize = getNextLargerSize();
    if (nextSize > currentBufferSize_) {
        requestBufferSizeChange(nextSize, reason);
        statistics_.autoIncreases++;
    }

    return currentBufferSize_;
}

//==============================================================================
int BufferManager::decreaseBufferSize(const juce::String& reason) {
    if (!config_.allowAutoDecrease) {
        return currentBufferSize_;
    }

    // Check delay before decreasing
    double timeSinceLastChange = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0 -
                                 statistics_.lastChangeTime;

    if (timeSinceLastChange < config_.decreaseDelaySeconds) {
        return currentBufferSize_;  // Too soon
    }

    int nextSize = getNextSmallerSize();
    if (nextSize > 0 && nextSize < currentBufferSize_) {
        requestBufferSizeChange(nextSize, reason);
        statistics_.autoDecreases++;
    }

    return currentBufferSize_;
}

//==============================================================================
void BufferManager::setCurrentBufferSize(int size) {
    int oldSize = currentBufferSize_;
    currentBufferSize_ = size;
    statistics_.currentSize = size;

    recordChange(oldSize, size, "External change");
}

//==============================================================================
int BufferManager::getRecommendedBufferSize() const {
    // Start with user preference
    int recommended = config_.preferredSize;

    // If user preference is not valid, use current size
    if (!isValidBufferSize(recommended)) {
        recommended = currentBufferSize_;
    }

    return recommended;
}

//==============================================================================
bool BufferManager::canIncrease() const {
    int nextSize = getNextLargerSize();
    return nextSize > currentBufferSize_;
}

//==============================================================================
bool BufferManager::canDecrease() const {
    int nextSize = getNextSmallerSize();
    return nextSize > 0 && nextSize < currentBufferSize_;
}

//==============================================================================
int BufferManager::getNextLargerSize() const {
    for (int size : config_.availableSizes) {
        if (size > currentBufferSize_ && size <= config_.maxSize) {
            return size;
        }
    }
    return currentBufferSize_;  // Already at max
}

//==============================================================================
int BufferManager::getNextSmallerSize() const {
    for (auto it = config_.availableSizes.rbegin(); it != config_.availableSizes.rend(); ++it) {
        if (*it < currentBufferSize_ && *it >= config_.minSize) {
            return *it;
        }
    }
    return currentBufferSize_;  // Already at min
}

//==============================================================================
bool BufferManager::isValidBufferSize(int size) const {
    // Check if size is in available list
    for (int available : config_.availableSizes) {
        if (size == available) {
            return true;
        }
    }
    return false;
}

//==============================================================================
// Private Methods
//==============================================================================

void BufferManager::recordChange(int oldSize, int newSize, const juce::String& reason) {
    BufferSizeEvent event;
    event.oldSize = oldSize;
    event.newSize = newSize;
    event.sampleRate = sampleRate_;
    event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
    event.reason = reason;

    // Add to history
    history_.push_back(event);

    // Limit history size
    if (static_cast<int>(history_.size()) > maxHistorySize) {
        history_.erase(history_.begin());
    }

    // Update statistics
    statistics_.totalChanges++;
    statistics_.lastChangeTime = event.timestamp;
    statistics_.lastChangeReason = reason;

    std::cout << "BufferManager: " << event.toString() << std::endl;
}

int BufferManager::findClosestValidSize(int size) const {
    if (config_.availableSizes.empty()) {
        return size;
    }

    // Find closest valid size
    int closest = config_.availableSizes[0];
    int minDiff = std::abs(size - closest);

    for (int available : config_.availableSizes) {
        int diff = std::abs(size - available);
        if (diff < minDiff) {
            minDiff = diff;
            closest = available;
        }
    }

    return closest;
}

} // namespace zenith
