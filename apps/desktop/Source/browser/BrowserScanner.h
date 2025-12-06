/*
  ==============================================================================

    BrowserScanner.h
    Created: 2025-12-05
    Author:  Zenith DAW

    Asynchronous file system scanner for the Universal Media Browser.
    Runs on a background thread to prevent UI freezing.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "BrowserData.h"
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>

namespace zenith {

/**
 * @brief Async file scanner for the browser
 * 
 * Scans directories recursively in a background thread.
 * Thread-safe queue for returning results to message thread.
 */
class BrowserScanner : private juce::Thread
{
public:
    BrowserScanner();
    ~BrowserScanner() override;

    //==============================================================================
    // Scanning Control
    //==============================================================================
    
    /**
     * @brief Start scanning a directory asynchronously
     * @param directory Root directory to scan
     * @param recursive Whether to scan subdirectories
     * @param maxDepth Maximum recursion depth (-1 for unlimited)
     */
    void startScan(const juce::File& directory, bool recursive = true, int maxDepth = 5);
    
    /**
     * @brief Cancel current scan operation
     */
    void cancelScan();
    
    /**
     * @brief Check if currently scanning
     */
    bool isScanning() const { return isThreadRunning(); }

    //==============================================================================
    // Results
    //==============================================================================
    
    /**
     * @brief Get pending scanned items (call from message thread)
     * @return Vector of newly discovered items
     */
    std::vector<std::shared_ptr<BrowserItem>> getNewItems();
    
    /**
     * @brief Get scan progress (0.0 - 1.0)
     */
    float getProgress() const { return progress_.load(); }
    
    /**
     * @brief Get current status message
     */
    juce::String getStatusMessage() const;

    //==============================================================================
    // Callbacks (called on message thread via AsyncUpdater)
    //==============================================================================
    
    std::function<void(const std::vector<std::shared_ptr<BrowserItem>>&)> onItemsDiscovered;
    std::function<void()> onScanComplete;
    std::function<void(float, const juce::String&)> onProgressUpdated;

private:
    void run() override;
    
    void scanDirectory(const juce::File& dir, std::shared_ptr<BrowserItem> parentNode, int currentDepth);
    BrowserItemType getTypeForFile(const juce::File& file) const;
    void enqueueItem(std::shared_ptr<BrowserItem> item);
    void notifyProgress(float progress, const juce::String& message);

    // Scan parameters
    juce::File scanRoot_;
    bool recursive_ = true;
    int maxDepth_ = 5;
    
    // Thread-safe results queue
    std::mutex queueMutex_;
    std::vector<std::shared_ptr<BrowserItem>> pendingItems_;
    
    // Progress tracking
    std::atomic<float> progress_{0.0f};
    std::atomic<int> filesScanned_{0};
    std::atomic<int> totalFiles_{0};
    juce::String statusMessage_;
    mutable std::mutex statusMutex_;
    
    // Supported extensions
    juce::StringArray audioExtensions_;
    juce::StringArray midiExtensions_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserScanner)
};

} // namespace zenith
