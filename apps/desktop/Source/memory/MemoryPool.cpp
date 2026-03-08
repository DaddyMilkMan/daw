/*
  ==============================================================================

    MemoryPool.cpp
    Implementation of memory pool

  ==============================================================================
*/

#include "MemoryPool.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
MemoryPool::MemoryPool(const MemoryPoolConfig& config)
    : config_(config) {

    initializePool();

    std::cout << "MemoryPool: Initialized" << std::endl;
    std::cout << "  - Block size: " << config_.blockSize << " bytes" << std::endl;
    std::cout << "  - Num blocks: " << config_.numBlocks << std::endl;
    std::cout << "  - Total pool: " << (config_.blockSize * config_.numBlocks / 1024) << " KB" << std::endl;
    std::cout << "  - Thread safety: " << (config_.enableThreadSafety ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
MemoryPool::~MemoryPool() {
    std::cout << "MemoryPool: Shut down" << std::endl;

    if (config_.enableStatistics) {
        auto stats = getStatistics();
        std::cout << "  - Total allocations: " << stats.totalAllocations << std::endl;
        std::cout << "  - Peak allocations: " << stats.peakAllocations << std::endl;
        std::cout << "  - Utilization: " << juce::String(stats.utilizationPercent, 1) << "%" << std::endl;
    }

    blocks_.clear();
}

//==============================================================================
void MemoryPool::initializePool() {
    blocks_.reserve(config_.numBlocks);

    for (juce::uint32 i = 0; i < config_.numBlocks; ++i) {
        juce::HeapBlock<char> block;
        block.allocate(config_.blockSize, config_.alignment);
        blocks_.push_back(std::move(block));
        freeList_.push(i);
    }
}

//==============================================================================
MemoryPool::Handle MemoryPool::allocate() {
    return allocate(config_.blockSize);
}

//==============================================================================
MemoryPool::Handle MemoryPool::allocate(juce::uint32 size) {
    if (size > config_.blockSize) {
        std::cerr << "MemoryPool: Requested size " << size
                  << " exceeds block size " << config_.blockSize << std::endl;
        return Handle();
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (freeList_.empty()) {
        if (config_.enableAutoResize && config_.numBlocks < config_.maxBlocks) {
            growPool();
        } else {
            std::cerr << "MemoryPool: Pool exhausted!" << std::endl;

            if (exhaustionCallback_) {
                exhaustionCallback_();
            }

            return Handle();
        }
    }

    juce::uint32 blockId = freeList_.top();
    freeList_.pop();

    void* data = blocks_[blockId].getData();

    allocatedBlocks_.fetch_add(1, std::memory_order_relaxed);
    totalAllocations_.fetch_add(1, std::memory_order_relaxed);
    updatePeakAllocations();

    return Handle(data, size, this, blockId);
}

//==============================================================================
void MemoryPool::deallocate(void* ptr, juce::uint32 blockId) {
    if (ptr == nullptr) {
        return;
    }

    if (!validatePointer(ptr)) {
        std::cerr << "MemoryPool: Attempting to deallocate invalid pointer" << std::endl;
        return;
    }

    if (blockId >= blocks_.size()) {
        std::cerr << "MemoryPool: Invalid block ID " << blockId << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        freeList_.push(blockId);
    }

    allocatedBlocks_.fetch_sub(1, std::memory_order_relaxed);
    totalDeallocations_.fetch_add(1, std::memory_order_relaxed);
}

//==============================================================================
MemoryPoolStats MemoryPool::getStatistics() const {
    MemoryPoolStats stats;

    stats.totalBlocks = config_.numBlocks;
    stats.allocatedBlocks = allocatedBlocks_.load(std::memory_order_relaxed);
    stats.freeBlocks = stats.totalBlocks - stats.allocatedBlocks;
    stats.totalAllocations = static_cast<juce::uint32>(
        totalAllocations_.load(std::memory_order_relaxed)
    );
    stats.totalDeallocations = static_cast<juce::uint32>(
        totalDeallocations_.load(std::memory_order_relaxed)
    );
    stats.peakAllocations = peakAllocations_.load(std::memory_order_relaxed);
    stats.bytesUsed = stats.allocatedBlocks * config_.blockSize;
    stats.bytesTotal = stats.totalBlocks * config_.blockSize;

    if (stats.bytesTotal > 0) {
        stats.utilizationPercent = (stats.bytesUsed * 100.0) / stats.bytesTotal;
    }

    return stats;
}

//==============================================================================
juce::uint32 MemoryPool::getFreeBlockCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<juce::uint32>(freeList_.size());
}

//==============================================================================
juce::uint32 MemoryPool::getAllocatedBlockCount() const {
    return allocatedBlocks_.load(std::memory_order_relaxed);
}

//==============================================================================
bool MemoryPool::isExhausted() const {
    return getFreeBlockCount() == 0;
}

//==============================================================================
bool MemoryPool::isFromPool(void* ptr) const {
    return validatePointer(ptr);
}

//==============================================================================
void MemoryPool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Reset free list to include all blocks
    freeList_ = std::stack<juce::uint32>();

    for (juce::uint32 i = 0; i < config_.numBlocks; ++i) {
        freeList_.push(i);
    }

    allocatedBlocks_.store(0, std::memory_order_relaxed);

    std::cout << "MemoryPool: Cleared all allocations" << std::endl;
}

//==============================================================================
bool MemoryPool::resize(juce::uint32 newNumBlocks) {
    if (!config_.enableAutoResize) {
        std::cerr << "MemoryPool: Auto-resize not enabled" << std::endl;
        return false;
    }

    if (newNumBlocks > config_.maxBlocks) {
        std::cerr << "MemoryPool: Requested size exceeds maximum" << std::endl;
        return false;
    }

    if (newNumBlocks <= config_.numBlocks) {
        return true;  // Already have enough blocks
    }

    std::lock_guard<std::mutex> lock(mutex_);

    juce::uint32 blocksToAdd = newNumBlocks - config_.numBlocks;

    for (juce::uint32 i = 0; i < blocksToAdd; ++i) {
        juce::HeapBlock<char> block;
        block.allocate(config_.blockSize, config_.alignment);
        blocks_.push_back(std::move(block));
        freeList_.push(static_cast<juce::uint32>(blocks_.size()) - 1);
    }

    config_.numBlocks = newNumBlocks;

    std::cout << "MemoryPool: Resized to " << newNumBlocks << " blocks" << std::endl;

    return true;
}

//==============================================================================
void MemoryPool::setExhaustionCallback(ExhaustionCallback callback) {
    exhaustionCallback_ = std::move(callback);
}

//==============================================================================
void MemoryPool::growPool() {
    juce::uint32 newSize = juce::jmin(
        config_.numBlocks * 2,
        config_.maxBlocks
    );

    std::cout << "MemoryPool: Growing pool from " << config_.numBlocks
              << " to " << newSize << " blocks" << std::endl;

    juce::uint32 blocksToAdd = newSize - config_.numBlocks;

    for (juce::uint32 i = 0; i < blocksToAdd; ++i) {
        juce::HeapBlock<char> block;
        block.allocate(config_.blockSize, config_.alignment);
        blocks_.push_back(std::move(block));
        freeList_.push(static_cast<juce::uint32>(blocks_.size()) - 1);
    }

    config_.numBlocks = newSize;
}

//==============================================================================
void MemoryPool::updatePeakAllocations() {
    juce::uint64 current = allocatedBlocks_.load(std::memory_order_relaxed);
    juce::uint64 peak = peakAllocations_.load(std::memory_order_relaxed);

    while (current > peak) {
        if (peakAllocations_.compare_exchange_weak(peak, current,
                                                   std::memory_order_relaxed)) {
            break;
        }
    }
}

//==============================================================================
bool MemoryPool::validatePointer(void* ptr) const {
    if (ptr == nullptr) {
        return false;
    }

    // Check if pointer falls within any of our blocks
    for (const auto& block : blocks_) {
        const char* blockStart = block.getData();
        const char* blockEnd = blockStart + config_.blockSize;
        const char* testPtr = static_cast<const char*>(ptr);

        if (testPtr >= blockStart && testPtr < blockEnd) {
            return true;
        }
    }

    return false;
}

//==============================================================================
//==============================================================================
AudioBufferPool::AudioBufferPool(juce::uint32 numChannels,
                                juce::uint32 maxSamples,
                                juce::uint32 numBuffers)
    : numChannels_(numChannels)
    , maxSamples_(maxSamples)
    , pool_(MemoryPoolConfig{
        numChannels * maxSamples * sizeof(float),  // blockSize
        numBuffers,                                // numBlocks
        true,                                      // enableAutoResize
        numBuffers * 4,                            // maxBlocks
        true,                                      // enableThreadSafety
        true,                                      // enableStatistics
        16                                         // alignment
    }) {

    std::cout << "AudioBufferPool: Initialized" << std::endl;
    std::cout << "  - Channels: " << numChannels << std::endl;
    std::cout << "  - Max samples: " << maxSamples << std::endl;
    std::cout << "  - Num buffers: " << numBuffers << std::endl;
}

//==============================================================================
AudioBufferPool::~AudioBufferPool() {
    std::cout << "AudioBufferPool: Shut down" << std::endl;
}

//==============================================================================
MemoryPool::Handle AudioBufferPool::allocateBuffer(juce::uint32 numSamples) {
    if (numSamples > maxSamples_) {
        std::cerr << "AudioBufferPool: Requested " << numSamples
                  << " samples exceeds max " << maxSamples_ << std::endl;
        return MemoryPool::Handle();
    }

    juce::uint32 sizeBytes = numChannels_ * numSamples * sizeof(float);
    return pool_.allocate(sizeBytes);
}

} // namespace zenith
