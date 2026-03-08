/*
  ==============================================================================

    MemoryPool.h
    Created: 2026-02-19
    Month 9, Gap #7 - Memory Pool Management

    Fixed-size memory pool for efficient audio buffer allocation.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <stack>
#include <mutex>
#include <atomic>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Memory pool statistics
 */
struct MemoryPoolStats {
    juce::uint32 totalBlocks = 0;
    juce::uint32 allocatedBlocks = 0;
    juce::uint32 freeBlocks = 0;
    juce::uint32 totalAllocations = 0;
    juce::uint32 totalDeallocations = 0;
    juce::uint64 peakAllocations = 0;
    juce::uint64 bytesUsed = 0;
    juce::uint64 bytesTotal = 0;
    double utilizationPercent = 0.0;

    juce::String toString() const {
        return juce::String::formatted(
            "Pool: %u/%u blocks (%.1f%% used) | %llu/%llu MB | Allocs: %u",
            allocatedBlocks,
            totalBlocks,
            utilizationPercent,
            bytesUsed / (1024 * 1024),
            bytesTotal / (1024 * 1024),
            totalAllocations
        );
    }
};

//==============================================================================
/**
 * @brief Memory pool block
 */
struct MemoryPoolBlock {
    void* data = nullptr;
    juce::uint32 size = 0;
    juce::uint32 blockId = 0;
    bool inUse = false;

    juce::String toString() const {
        return juce::String::formatted(
            "Block %u: %u bytes, %s",
            blockId,
            size,
            inUse ? "in use" : "free"
        );
    }
};

//==============================================================================
/**
 * @brief Memory pool configuration
 */
struct MemoryPoolConfig {
    juce::uint32 blockSize = 4096;          // Size of each block (bytes)
    juce::uint32 numBlocks = 256;           // Number of blocks in pool
    bool enableAutoResize = false;          // Automatically grow pool
    juce::uint32 maxBlocks = 1024;          // Maximum blocks if auto-resize
    bool enableThreadSafety = true;         // Enable mutex protection
    bool enableStatistics = true;           // Track usage statistics
    juce::uint32 alignment = 16;            // Memory alignment (bytes)
};

//==============================================================================
/**
 * @brief Fixed-size memory pool for efficient allocation
 *
 * Features:
 * - Fast allocation/deallocation (O(1))
 * - Reduces fragmentation
 * - Thread-safe operations
 * - Statistics tracking
 * - Automatic pool growth (optional)
 * - Memory alignment support
 *
 * Use Cases:
 * - Audio buffer pools
 * - DSP processing buffers
 * - Temporary calculation buffers
 * - High-frequency allocations
 */
class MemoryPool {
public:
    //==========================================================================
    /**
     * @brief RAII handle for pooled memory
     */
    class Handle {
    public:
        Handle() noexcept
            : data_(nullptr)
            , size_(0)
            , pool_(nullptr)
            , blockId_(0) {
        }

        Handle(void* data, juce::uint32 size, MemoryPool* pool, juce::uint32 blockId) noexcept
            : data_(data)
            , size_(size)
            , pool_(pool)
            , blockId_(blockId) {
        }

        ~Handle() {
            reset();
        }

        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;

        Handle(Handle&& other) noexcept
            : data_(other.data_)
            , size_(other.size_)
            , pool_(other.pool_)
            , blockId_(other.blockId_) {

            other.data_ = nullptr;
            other.pool_ = nullptr;
        }

        Handle& operator=(Handle&& other) noexcept {
            if (this != &other) {
                reset();

                data_ = other.data_;
                size_ = other.size_;
                pool_ = other.pool_;
                blockId_ = other.blockId_;

                other.data_ = nullptr;
                other.pool_ = nullptr;
            }
            return *this;
        }

        void* getData() noexcept { return data_; }
        const void* getData() const noexcept { return data_; }
        juce::uint32 getSize() const noexcept { return size_; }
        bool isValid() const noexcept { return data_ != nullptr; }

        void reset() {
            if (pool_ != nullptr && data_ != nullptr) {
                pool_->deallocate(data_, blockId_);
            }
            data_ = nullptr;
            pool_ = nullptr;
        }

    private:
        void* data_;
        juce::uint32 size_;
        MemoryPool* pool_;
        juce::uint32 blockId_;
    };

    //==========================================================================
    explicit MemoryPool(const MemoryPoolConfig& config = {});
    ~MemoryPool();

    //==========================================================================
    /**
     * @brief Allocate block from pool
     * @return Handle to allocated memory, or invalid handle if pool exhausted
     */
    Handle allocate();

    //==========================================================================
    /**
     * @brief Allocate with specific size (must be <= block size)
     */
    Handle allocate(juce::uint32 size);

    //==========================================================================
    /**
     * @brief Deallocate block back to pool
     */
    void deallocate(void* ptr, juce::uint32 blockId);

    //==========================================================================
    /**
     * @brief Get pool statistics
     */
    MemoryPoolStats getStatistics() const;

    //==========================================================================
    /**
     * @brief Get number of free blocks
     */
    juce::uint32 getFreeBlockCount() const;

    //==========================================================================
    /**
     * @brief Get number of allocated blocks
     */
    juce::uint32 getAllocatedBlockCount() const;

    //==========================================================================
    /**
     * @brief Check if pool is exhausted
     */
    bool isExhausted() const;

    //==========================================================================
    /**
     * @brief Check if pointer belongs to this pool
     */
    bool isFromPool(void* ptr) const;

    //==========================================================================
    /**
     * @brief Clear all allocations (for debugging/emergency)
     */
    void clear();

    //==========================================================================
    /**
     * @brief Resize pool (only works if enabled in config)
     */
    bool resize(juce::uint32 newNumBlocks);

    //==========================================================================
    /**
     * @brief Get block size
     */
    juce::uint32 getBlockSize() const noexcept { return config_.blockSize; }

    //==========================================================================
    /**
     * @brief Get number of blocks
     */
    juce::uint32 getNumBlocks() const noexcept { return config_.numBlocks; }

    //==========================================================================
    /**
     * @brief Set exhaustion callback
     */
    using ExhaustionCallback = std::function<void()>;
    void setExhaustionCallback(ExhaustionCallback callback);

private:
    //==========================================================================
    MemoryPoolConfig config_;
    std::vector<juce::HeapBlock<char>> blocks_;
    std::stack<juce::uint32> freeList_;
    mutable std::mutex mutex_;

    // Statistics
    std::atomic<juce::uint32> allocatedBlocks_{0};
    std::atomic<juce::uint64> totalAllocations_{0};
    std::atomic<juce::uint64> totalDeallocations_{0};
    std::atomic<juce::uint64> peakAllocations_{0};

    ExhaustionCallback exhaustionCallback_;

    //==========================================================================
    void initializePool();
    void growPool();
    void updatePeakAllocations();
    bool validatePointer(void* ptr) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MemoryPool)
};

//==============================================================================
/**
 * @brief Specialized audio buffer pool
 */
class AudioBufferPool {
public:
    //==========================================================================
    explicit AudioBufferPool(juce::uint32 numChannels = 2,
                            juce::uint32 maxSamples = 512,
                            juce::uint32 numBuffers = 64);

    ~AudioBufferPool();

    //==========================================================================
    /**
     * @brief Allocate audio buffer
     */
    MemoryPool::Handle allocateBuffer(juce::uint32 numSamples);

    //==========================================================================
    /**
     * @brief Get buffer pool configuration
     */
    const MemoryPool& getPool() const noexcept { return pool_; }

    //==========================================================================
    /**
     * @brief Get pool statistics
     */
    MemoryPoolStats getStatistics() const { return pool_.getStatistics(); }

private:
    juce::uint32 numChannels_;
    juce::uint32 maxSamples_;
    MemoryPool pool_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioBufferPool)
};

} // namespace zenith
