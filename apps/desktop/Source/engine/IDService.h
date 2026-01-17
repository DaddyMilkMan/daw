#pragma once

#include <juce_core/juce_core.h>
#include <mutex>
#include <unordered_map>
#include <atomic>

namespace zenith {

/**
 * @brief Thread-safe specific ID generation service
 * 
 * Manages unique ID generation for project elements (Tracks, Clips, etc.)
 * using a prefix-based counter system.
 */
class IDService {
public:
    IDService() = default;
    ~IDService() = default;

    /**
     * @brief Generates a unique ID with the given prefix
     * @param prefix ID prefix (e.g. "TRACK", "CLIP")
     * @return Unique ID string (e.g. "TRACK-1", "CLIP-42")
     */
    juce::String generateId(const juce::String& prefix) {
        const std::lock_guard<std::mutex> lock(mutex_);
        
        // Find or initialize counter for this prefix
        if (counters_.find(prefix) == counters_.end()) {
            counters_[prefix] = 0;
        }
        
        // Increment and format
        counters_[prefix]++;
        return prefix + "-" + juce::String(counters_[prefix]);
    }

    /**
     * @brief Resets the counter for a specific prefix
     * @param prefix ID prefix to reset
     * @param nextValue Next value to be returned by generateId
     */
    void setNextId(const juce::String& prefix, int nextValue) {
        const std::lock_guard<std::mutex> lock(mutex_);
        counters_[prefix] = nextValue - 1; // Subtract 1 because generateId increments first
    }

    /**
     * @brief Updates the counter if the given ID is higher than current
     * Useful when loading projects to ensure next ID is safe
     * @param currentId Existing ID string (e.g. "TRACK-5")
     */
    void updateCounterFromId(const juce::String& currentId) {
        const std::lock_guard<std::mutex> lock(mutex_);
        
        int hyphenPos = currentId.lastIndexOf("-");
        if (hyphenPos > 0) {
            juce::String prefix = currentId.substring(0, hyphenPos);
            juce::String numberPart = currentId.substring(hyphenPos + 1);
            int number = numberPart.getIntValue();
            
            if (counters_.find(prefix) == counters_.end() || number > counters_[prefix]) {
                counters_[prefix] = number;
            }
        }
    }

    /**
     * @brief Clears all counters
     */
    void resetAll() {
        const std::lock_guard<std::mutex> lock(mutex_);
        counters_.clear();
    }

private:
    std::mutex mutex_;
    std::unordered_map<juce::String, int> counters_;
};

} // namespace zenith
