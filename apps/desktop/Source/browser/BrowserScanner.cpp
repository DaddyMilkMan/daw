/*
  ==============================================================================

    BrowserScanner.cpp
    Created: 2025-12-05
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserScanner.h"

namespace zenith {

BrowserScanner::BrowserScanner()
    : juce::Thread("BrowserScanner")
{
    // Initialize supported extensions
    audioExtensions_ = { ".wav", ".aif", ".aiff", ".mp3", ".ogg", ".flac", ".m4a" };
    midiExtensions_ = { ".mid", ".midi" };
}

BrowserScanner::~BrowserScanner()
{
    isShuttingDown_->store(true);
    cancelScan();
}

void BrowserScanner::startScan(const juce::File& directory, bool recursive, int maxDepth)
{
    // Cancel any existing scan
    while (!threadShouldExit() && isThreadRunning())
    {
        cancelScan();
        waitForThreadToExit(2000);
    }
    
    // Reset state
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        pendingItems_.clear();
    }
    
    progress_.store(0.0f);
    filesScanned_.store(0);
    totalFiles_.store(0);
    
    // Set parameters
    scanRoot_ = directory;
    recursive_ = recursive;
    maxDepth_ = maxDepth;
    
    // Start the thread
    startThread();
}

void BrowserScanner::cancelScan()
{
    signalThreadShouldExit();
}

std::vector<std::shared_ptr<BrowserItem>> BrowserScanner::getNewItems()
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    std::vector<std::shared_ptr<BrowserItem>> items = std::move(pendingItems_);
    pendingItems_.clear();
    return items;
}

juce::String BrowserScanner::getStatusMessage() const
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return statusMessage_;
}

void BrowserScanner::run()
{
    if (!scanRoot_.isDirectory())
    {
        notifyProgress(1.0f, "Invalid directory");
        return;
    }
    
    notifyProgress(0.0f, "Counting files...");
    
    // Phase 1: Count total files for progress calculation
    int fileCount = 0;
    juce::RangedDirectoryIterator countIter(scanRoot_, recursive_, "*", 
                                            juce::File::findFilesAndDirectories);
    for (const auto& entry : countIter)
    {
        if (threadShouldExit()) return;
        fileCount++;
    }
    totalFiles_.store(fileCount);
    
    notifyProgress(0.0f, "Scanning " + juce::String(fileCount) + " items...");
    
    // Phase 2: Create root node and scan
    auto rootNode = std::make_shared<BrowserItem>(
        scanRoot_.getFullPathName(),
        scanRoot_.getFileName(),
        BrowserItemType::Folder
    );
    rootNode->isDirectory = true;
    
    scanDirectory(scanRoot_, rootNode, 0);
    
    // Enqueue the root with all its children
    enqueueItem(rootNode);
    
    notifyProgress(1.0f, "Scan complete");
    
    // Notify completion on message thread
    if (onScanComplete)
    {
        auto shutdownFlag = isShuttingDown_;
        auto callback = onScanComplete;
        juce::MessageManager::callAsync([shutdownFlag, callback]() {
            if (shutdownFlag->load()) return;
            if (callback) callback();
        });
    }
}

void BrowserScanner::scanDirectory(const juce::File& dir, std::shared_ptr<BrowserItem> parentNode, int currentDepth)
{
    if (threadShouldExit()) return;
    if (maxDepth_ >= 0 && currentDepth >= maxDepth_) return;
    
    juce::RangedDirectoryIterator iter(dir, false, "*", juce::File::findFilesAndDirectories);
    
    for (const auto& entry : iter)
    {
        if (threadShouldExit()) return;
        
        const juce::File& file = entry.getFile();
        
        if (file.isDirectory())
        {
            // Create folder node
            auto folderNode = std::make_shared<BrowserItem>(
                file.getFullPathName(),
                file.getFileName(),
                BrowserItemType::Folder
            );
            folderNode->isDirectory = true;
            folderNode->parent = parentNode;
            
            // Recursively scan subdirectory
            if (recursive_)
            {
                scanDirectory(file, folderNode, currentDepth + 1);
            }
            
            // Only add folders that have children (or we want to show empty folders)
            if (folderNode->hasChildren())
            {
                parentNode->addChild(folderNode);
            }
        }
        else
        {
            // Check if file is a supported type
            BrowserItemType type = getTypeForFile(file);
            
            if (type != BrowserItemType::Unknown)
            {
                auto fileNode = std::make_shared<BrowserItem>(
                    file.getFullPathName(),
                    file.getFileNameWithoutExtension(),
                    type
                );
                fileNode->parent = parentNode;
                
                // Fill metadata
                fileNode->metadata.format = file.getFileExtension().toUpperCase().substring(1);
                
                parentNode->addChild(fileNode);
            }
        }
        
        // Update progress
        int scanned = filesScanned_.fetch_add(1) + 1;
        int total = totalFiles_.load();
        if (total > 0)
        {
            float prog = static_cast<float>(scanned) / static_cast<float>(total);
            progress_.store(prog);
            
            // Update status periodically (every 50 files)
            if (scanned % 50 == 0)
            {
                notifyProgress(prog, "Scanned " + juce::String(scanned) + "/" + juce::String(total));
            }
        }
    }
}

BrowserItemType BrowserScanner::getTypeForFile(const juce::File& file) const
{
    juce::String ext = file.getFileExtension().toLowerCase();
    
    if (audioExtensions_.contains(ext))
        return BrowserItemType::AudioFile;
    
    if (midiExtensions_.contains(ext))
        return BrowserItemType::MidiFile;
    
    if (ext == ".zenith")
        return BrowserItemType::Project;
    
    return BrowserItemType::Unknown;
}

void BrowserScanner::enqueueItem(std::shared_ptr<BrowserItem> item)
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    pendingItems_.push_back(item);
    
    // Notify on message thread
    if (onItemsDiscovered)
    {
        auto items = pendingItems_;
        auto shutdownFlag = isShuttingDown_;
        auto callback = onItemsDiscovered;
        juce::MessageManager::callAsync([shutdownFlag, callback, items]() {
            if (shutdownFlag->load()) return;
            if (callback) callback(items);
        });
    }
}

void BrowserScanner::notifyProgress(float progress, const juce::String& message)
{
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        statusMessage_ = message;
    }
    
    if (onProgressUpdated)
    {
        auto shutdownFlag = isShuttingDown_;
        auto callback = onProgressUpdated;
        juce::MessageManager::callAsync([shutdownFlag, callback, progress, message]() {
            if (shutdownFlag->load()) return;
            if (callback) callback(progress, message);
        });
    }
}

} // namespace zenith
