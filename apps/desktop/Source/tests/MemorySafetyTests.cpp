/*
  ==============================================================================

    MemorySafetyTests.cpp
    Created: 2026-02-19
    Comprehensive unit tests for memory safety components

  ==============================================================================
*/

#include "../memory/SafeBuffer.h"
#include "../memory/AudioBufferGuard.h"
#include "../memory/MemoryPool.h"
#include "../memory/MemoryProfiler.h"

#include <juce_core/juce_core.h>

namespace zenith {
namespace test {

//==============================================================================
// SafeBuffer Tests
//==============================================================================
class SafeBufferTest : public juce::UnitTest {
public:
    SafeBufferTest() : juce::UnitTest("SafeBuffer", "Zenith.Memory") {}

    void runTest() override {
        beginTest("Construction and basic properties");
        {
            SafeBuffer<int> buffer(100);
            expectEquals((int)buffer.size(), (int)100);
            expect(buffer.capacity() >= 100);
            expect(buffer.ownsMemory());
            expect(!buffer.isEmpty());
        }

        beginTest("Default construction");
        {
            SafeBuffer<float> buffer;
            expectEquals((int)buffer.size(), (int)0);
            expect(buffer.isEmpty());
            expect(!buffer.ownsMemory());
        }

        beginTest("Bounds-checked access");
        {
            SafeBuffer<int> buffer(10);

            // Valid access
            buffer.at(0) = 42;
            expectEquals(buffer.at(0), 42);

            buffer.at(9) = 99;
            expectEquals(buffer.at(9), 99);

            // Invalid access should throw
            bool threw = false;
            try {
                buffer.at(10);  // Out of bounds
            } catch (const std::out_of_range&) {
                threw = true;
            }
            expect(threw);
        }

        beginTest("Copy operations");
        {
            SafeBuffer<int> source(10);
            for (size_t i = 0; i < source.size(); ++i) {
                source.getData()[i] = (int)i;
            }

            SafeBuffer<int> dest(10);
            dest.copyFrom(source.getData(), 10);

            bool allMatch = true;
            for (size_t i = 0; i < 10; ++i) {
                if (dest.getData()[i] != (int)i) {
                    allMatch = false;
                    break;
                }
            }
            expect(allMatch);
        }

        beginTest("Clear and fill");
        {
            SafeBuffer<int> buffer(100);
            buffer.fill(42);

            bool allFilled = true;
            for (size_t i = 0; i < buffer.size(); ++i) {
                if (buffer.getData()[i] != 42) {
                    allFilled = false;
                    break;
                }
            }
            expect(allFilled);

            buffer.clear();

            bool allZero = true;
            for (size_t i = 0; i < buffer.size(); ++i) {
                if (buffer.getData()[i] != 0) {
                    allZero = false;
                    break;
                }
            }
            expect(allZero);
        }

        beginTest("Resize");
        {
            SafeBuffer<int> buffer(10);
            for (size_t i = 0; i < 10; ++i) {
                buffer.getData()[i] = (int)i;
            }

            buffer.resize(20);

            expectEquals((int)buffer.size(), (int)20);

            // First 10 elements should be preserved
            bool preserved = true;
            for (size_t i = 0; i < 10; ++i) {
                if (buffer.getData()[i] != (int)i) {
                    preserved = false;
                    break;
                }
            }
            expect(preserved);

            // New elements should be zero-initialized
            bool newZero = true;
            for (size_t i = 10; i < 20; ++i) {
                if (buffer.getData()[i] != 0) {
                    newZero = false;
                    break;
                }
            }
            expect(newZero);
        }

        beginTest("Copy constructor");
        {
            SafeBuffer<int> original(10);
            for (size_t i = 0; i < 10; ++i) {
                original.getData()[i] = (int)i;
            }

            SafeBuffer<int> copy(original);

            expectEquals(copy.size(), original.size());
            expect(copy.ownsMemory());

            bool allMatch = true;
            for (size_t i = 0; i < 10; ++i) {
                if (copy.getData()[i] != original.getData()[i]) {
                    allMatch = false;
                    break;
                }
            }
            expect(allMatch);

            // Modify copy, original should be unchanged
            copy.getData()[0] = 999;
            expectEquals(original.getData()[0], 0);
        }

        beginTest("Violation callback");
        {
            SafeBuffer<int> buffer(10);
            BufferViolation lastViolation;

            buffer.setViolationCallback([&lastViolation](const BufferViolation& v) {
                lastViolation = v;
            });

            bool threw = false;
            try {
                buffer.at(20);  // Out of bounds
            } catch (const std::out_of_range&) {
                threw = true;
            }

            expect(threw);
            expect(lastViolation.isViolation());
            expectEquals((int)lastViolation.requestedIndex, (int)20);
        }
    }
};

//==============================================================================
// AudioBufferGuard Tests
//==============================================================================
class AudioBufferGuardTest : public juce::UnitTest {
public:
    AudioBufferGuardTest() : juce::UnitTest("AudioBufferGuard", "Zenith.Memory") {}

    void runTest() override {
        beginTest("Construction and basic properties");
        {
            AudioBufferGuard<float> guard(2, 256);
            expect(guard.isValid());
            expectEquals(guard.getNumChannels(), 2);
            expectEquals(guard.getNumSamples(), 256);
            expect(guard.ownsMemory());
        }

        beginTest("Sample access with bounds checking");
        {
            AudioBufferGuard<float> guard(2, 256);

            // Valid access
            guard.setSample(0, 0, 1.0f);
            expectEquals(guard.getSample(0, 0), 1.0f);

            guard.setSample(1, 255, -1.0f);
            expectEquals(guard.getSample(1, 255), -1.0f);

            // Invalid access should throw
            bool threw = false;
            try {
                guard.setSample(2, 0, 0.0f);  // Invalid channel
            } catch (const std::out_of_range&) {
                threw = true;
            }
            expect(threw);

            threw = false;
            try {
                guard.getSample(0, 256);  // Invalid sample
            } catch (const std::out_of_range&) {
                threw = true;
            }
            expect(threw);
        }

        beginTest("Clear operations");
        {
            AudioBufferGuard<float> guard(2, 256);

            // Fill with data
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < 256; ++i) {
                    guard.setSample(ch, i, 1.0f);
                }
            }

            // Clear all
            guard.clear();

            // Verify all zeros
            bool allZero = true;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < 256; ++i) {
                    if (guard.getSample(ch, i) != 0.0f) {
                        allZero = false;
                        break;
                    }
                }
            }
            expect(allZero);
        }

        beginTest("Get read/write pointers");
        {
            AudioBufferGuard<float> guard(2, 256);

            float* writePtr = guard.getWritePointer(0);
            expect(writePtr != nullptr);

            writePtr[0] = 42.0f;

            const float* readPtr = guard.getReadPointer(0);
            expectEquals(readPtr[0], 42.0f);
        }

        beginTest("Copy between buffers");
        {
            AudioBufferGuard<float> source(2, 256);
            AudioBufferGuard<float> dest(2, 256);

            // Fill source
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < 256; ++i) {
                    source.setSample(ch, i, (float)(i * (ch + 1)));
                }
            }

            // Copy to dest
            dest.copyFrom(0, source, 0, 256);
            dest.copyFrom(1, source, 1, 256);

            // Verify
            bool matches = true;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < 256; ++i) {
                    if (dest.getSample(ch, i) != source.getSample(ch, i)) {
                        matches = false;
                        break;
                    }
                }
            }
            expect(matches);
        }

        beginTest("Apply gain");
        {
            AudioBufferGuard<float> guard(1, 256);

            for (int i = 0; i < 256; ++i) {
                guard.setSample(0, i, 1.0f);
            }

            guard.applyGain(0, 0, 256, 0.5f);

            bool allHalf = true;
            for (int i = 0; i < 256; ++i) {
                if (std::abs(guard.getSample(0, i) - 0.5f) > 0.001f) {
                    allHalf = false;
                    break;
                }
            }
            expect(allHalf);
        }

        beginTest("Resize");
        {
            AudioBufferGuard<float> guard(2, 256);

            guard.setSample(0, 0, 1.0f);
            guard.setSample(1, 0, 2.0f);

            guard.resize(4, 512);

            expectEquals(guard.getNumChannels(), 4);
            expectEquals(guard.getNumSamples(), 512);

            // Old data should be preserved (if supported)
            // Note: JUCE AudioBuffer::setSize with keepExisting=true
        }

        beginTest("Wrap existing buffer");
        {
            juce::AudioBuffer<float> existingBuffer(2, 256);
            existingBuffer.setSample(0, 0, 1.0f);

            AudioBufferGuard<float> guard(existingBuffer);

            expectEquals(guard.getNumChannels(), 2);
            expectEquals(guard.getNumSamples(), 256);
            expect(!guard.ownsMemory());
            expectEquals(guard.getSample(0, 0), 1.0f);
        }
    }
};

//==============================================================================
// MemoryPool Tests
//==============================================================================
class MemoryPoolTest : public juce::UnitTest {
public:
    MemoryPoolTest() : juce::UnitTest("MemoryPool", "Zenith.Memory") {}

    void runTest() override {
        beginTest("Pool construction and initialization");
        {
            MemoryPoolConfig config;
            config.blockSize = 1024;
            config.numBlocks = 100;

            MemoryPool pool(config);

            auto stats = pool.getStatistics();
            expectEquals((int)stats.totalBlocks, (int)100);
            expectEquals((int)stats.freeBlocks, (int)100);
            expectEquals((int)stats.allocatedBlocks, (int)0);
        }

        beginTest("Allocate and deallocate");
        {
            MemoryPoolConfig config;
            config.blockSize = 1024;
            config.numBlocks = 10;

            MemoryPool pool(config);

            // Allocate
            auto handle1 = pool.allocate();
            expect(handle1.isValid());
            expectEquals((int)handle1.getSize(), 1024);

            auto stats = pool.getStatistics();
            expectEquals((int)stats.allocatedBlocks, (int)1);
            expectEquals((int)stats.freeBlocks, (int)9);

            // Deallocate
            handle1.reset();

            stats = pool.getStatistics();
            expectEquals((int)stats.allocatedBlocks, (int)0);
            expectEquals((int)stats.freeBlocks, (int)10);
        }

        beginTest("Multiple allocations");
        {
            MemoryPoolConfig config;
            config.blockSize = 512;
            config.numBlocks = 10;

            MemoryPool pool(config);

            std::vector<MemoryPool::Handle> handles;

            for (int i = 0; i < 5; ++i) {
                handles.push_back(pool.allocate());
                expect(handles.back().isValid());
            }

            auto stats = pool.getStatistics();
            expectEquals((int)stats.allocatedBlocks, (int)5);

            // Free all
            handles.clear();

            stats = pool.getStatistics();
            expectEquals((int)stats.allocatedBlocks, (int)0);
        }

        beginTest("Pool exhaustion");
        {
            MemoryPoolConfig config;
            config.blockSize = 256;
            config.numBlocks = 3;
            config.enableAutoResize = false;

            MemoryPool pool(config);

            auto h1 = pool.allocate();
            auto h2 = pool.allocate();
            auto h3 = pool.allocate();

            expect(h1.isValid());
            expect(h2.isValid());
            expect(h3.isValid());

            // Pool exhausted
            auto h4 = pool.allocate();
            expect(!h4.isValid());

            expect(pool.isExhausted());
        }

        beginTest("Pool auto-resize");
        {
            MemoryPoolConfig config;
            config.blockSize = 256;
            config.numBlocks = 4;
            config.enableAutoResize = true;
            config.maxBlocks = 8;

            MemoryPool pool(config);

            // Allocate all blocks
            std::vector<MemoryPool::Handle> handles;
            for (int i = 0; i < 4; ++i) {
                handles.push_back(pool.allocate());
            }

            expect(pool.isExhausted());

            // Should auto-resize
            auto h5 = pool.allocate();
            expect(h5.isValid());

            auto stats = pool.getStatistics();
            expect(stats.totalBlocks > 4);
        }

        beginTest("Data integrity");
        {
            MemoryPoolConfig config;
            config.blockSize = 1024;
            config.numBlocks = 10;

            MemoryPool pool(config);

            auto handle = pool.allocate();
            expect(handle.isValid());

            // Write data
            int* data = static_cast<int*>(handle.getData());
            data[0] = 42;
            data[100] = 99;

            // Read back
            expectEquals(data[0], 42);
            expectEquals(data[100], 99);
        }

        beginTest("Pointer validation");
        {
            MemoryPoolConfig config;
            config.blockSize = 512;
            config.numBlocks = 10;

            MemoryPool pool(config);

            auto handle = pool.allocate();
            expect(pool.isFromPool(handle.getData()));

            int stackVar;
            expect(!pool.isFromPool(&stackVar));
        }

        beginTest("Statistics tracking");
        {
            MemoryPoolConfig config;
            config.blockSize = 1024;
            config.numBlocks = 100;

            MemoryPool pool(config);

            for (int i = 0; i < 10; ++i) {
                auto handle = pool.allocate();
                // Auto-deallocate at end of scope
            }

            auto stats = pool.getStatistics();
            expectEquals((int)stats.totalAllocations, 10);
            expectEquals((int)stats.totalDeallocations, 10);
            expect(stats.peakAllocations > 0);
        }

        beginTest("Audio buffer pool");
        {
            AudioBufferPool pool(2, 512, 64);

            auto handle = pool.allocateBuffer(256);
            expect(handle.isValid());

            auto stats = pool.getStatistics();
            expect(stats.totalBlocks > 0);
        }
    }
};

//==============================================================================
// MemoryProfiler Tests
//==============================================================================
class MemoryProfilerTest : public juce::UnitTest {
public:
    MemoryProfilerTest() : juce::UnitTest("MemoryProfiler", "Zenith.Memory") {}

    void runTest() override {
        beginTest("Profiler lifecycle");
        {
            MemoryProfiler& profiler = MemoryProfiler::getInstance();

            expect(!profiler.isRunning());

            profiler.start();
            expect(profiler.isRunning());

            profiler.stop();
            expect(!profiler.isRunning());
        }

        beginTest("Allocation tracking");
        {
            MemoryProfiler& profiler = MemoryProfiler::getInstance();
            profiler.reset();
            profiler.start();

            // Simulate some allocations
            void* ptr1 = malloc(100);
            profiler.recordAllocation(ptr1, 100, __FILE__, __LINE__, "Test");

            void* ptr2 = malloc(200);
            profiler.recordAllocation(ptr2, 200, __FILE__, __LINE__, "Test");

            auto stats = profiler.getCategoryStats("Test");
            expectEquals((int)stats.totalAllocations, (int)2);
            expectEquals((int)stats.currentBytes, (int)300);

            // Deallocate
            profiler.recordDeallocation(ptr1);
            free(ptr1);

            stats = profiler.getCategoryStats("Test");
            expectEquals((int)stats.currentBytes, (int)200);

            // Cleanup
            profiler.recordDeallocation(ptr2);
            free(ptr2);

            profiler.stop();
        }

        beginTest("Snapshot generation");
        {
            MemoryProfiler& profiler = MemoryProfiler::getInstance();
            profiler.reset();
            profiler.start();

            void* ptr = malloc(1024);
            profiler.recordAllocation(ptr, 1024, __FILE__, __LINE__, "SnapshotTest");

            auto snapshot = profiler.getSnapshot();
            expect(snapshot.currentBytes > 0);
            expect(snapshot.peakBytes > 0);
            expect(snapshot.totalAllocations > 0);

            profiler.recordDeallocation(ptr);
            free(ptr);

            profiler.stop();
        }

        beginTest("Category statistics");
        {
            MemoryProfiler& profiler = MemoryProfiler::getInstance();
            profiler.reset();
            profiler.start();

            // Allocate in different categories
            void* audioPtr = malloc(512);
            profiler.recordAllocation(audioPtr, 512, __FILE__, __LINE__, "Audio");

            void* uiPtr = malloc(256);
            profiler.recordAllocation(uiPtr, 256, __FILE__, __LINE__, "UI");

            auto allStats = profiler.getAllCategoryStats();
            expect(allStats.size() >= 2);

            profiler.recordDeallocation(audioPtr);
            profiler.recordDeallocation(uiPtr);
            free(audioPtr);
            free(uiPtr);

            profiler.stop();
        }

        beginTest("Report generation");
        {
            MemoryProfiler& profiler = MemoryProfiler::getInstance();
            profiler.reset();
            profiler.start();

            void* ptr = malloc(2048);
            profiler.recordAllocation(ptr, 2048, __FILE__, __LINE__, "ReportTest");

            juce::String report = profiler.generateReport();
            expect(report.contains("Memory Profiler Report"));
            expect(report.isNotEmpty());

            profiler.recordDeallocation(ptr);
            free(ptr);

            profiler.stop();
        }
    }
};

} // namespace test
} // namespace zenith
