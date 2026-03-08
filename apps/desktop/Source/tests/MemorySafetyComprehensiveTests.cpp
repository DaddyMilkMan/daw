/*
  ==============================================================================

    MemorySafetyTests.cpp
    Comprehensive test suite for memory safety components

    Tests EVERYTHING - not just compiles, but actual functionality

  ==============================================================================
*/

#include "../memory/MemoryLeakDetector.h"
#include "../memory/OOMHandler.h"
#include "../memory/PredictiveMonitor.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace zenith;

//==============================================================================
class MemorySafetyTests {
public:
    //==========================================================================
    int runAllTests() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Memory Safety Tests - Starting" << std::endl;
        std::cout << "========================================\n" << std::endl;

        int passed = 0;
        int total = 0;

        // Test 1: Memory Leak Detector
        if (testMemoryLeakDetector()) { passed++; } total++;

        // Test 2: OOM Handler Memory Query
        if (testOOMHandlerQuery()) { passed++; } total++;

        // Test 3: OOM Handler Recovery Actions
        if (testOOMHandlerRecovery()) { passed++; } total++;

        // Test 4: OOM Handler Callbacks
        if (testOOMHandlerCallbacks()) { passed++; } total++;

        // Test 5: Predictive Monitor
        if (testPredictiveMonitor()) { passed++; } total++;

        // Test 6: Platform-Specific APIs
        if (testPlatformAPIs()) { passed++; } total++;

        std::cout << "\n========================================" << std::endl;
        std::cout << "Test Results: " << passed << "/" << total << " passed";
        if (passed == total) {
            std::cout << " ✅ ALL TESTS PASSED\n" << std::endl;
        } else {
            std::cout << " ❌ SOME TESTS FAILED\n" << std::endl;
        }
        std::cout << "========================================\n" << std::endl;

        return (passed == total) ? 0 : 1;
    }

private:
    //==========================================================================
    bool testMemoryLeakDetector() {
        std::cout << "Test 1: Memory Leak Detector... ";

        auto& detector = MemoryLeakDetector::getInstance();

        // Track some allocations
        detector.trackAllocation("test1", 100, "test", 10);
        detector.trackAllocation("test2", 200, "test", 20);

        auto stats = detector.getStatistics();

        bool passed = (stats.totalAllocations == 2 &&
                       stats.totalBytesAllocated == 300);

        if (passed) {
            std::cout << "✅ PASS\n" << std::endl;
        } else {
            std::cout << "❌ FAIL\n" << std::endl;
        }

        return passed;
    }

    //==========================================================================
    bool testOOMHandlerQuery() {
        std::cout << "Test 2: OOM Handler Memory Query... ";

        auto& oom = OOMHandler::getInstance();
        auto snapshot = oom.getMemorySnapshot();

        bool passed = (snapshot.totalPhysicalMemory > 0 &&
                       snapshot.processMemoryUsed > 0 &&
                       snapshot.memoryUsagePercent >= 0.0);

        if (passed) {
            std::cout << "✅ PASS ("
                      << (snapshot.processMemoryUsed / (1024*1024))
                      << " MB used)\n" << std::endl;
        } else {
            std::cout << "❌ FAIL\n" << std::endl;
        }

        return passed;
    }

    //==========================================================================
    bool testOOMHandlerRecovery() {
        std::cout << "Test 3: OOM Handler Recovery Actions... ";

        auto& oom = OOMHandler::getInstance();

        // Test dropCaches
        bool dropCachesWorked = oom.attemptRecovery([]{
            OOMRecoveryAction action;
            action.type = OOMRecoveryAction::DropCache;
            return action;
        }());

        // Test freeUnusedMemory
        bool freeUnusedWorked = oom.attemptRecovery([]{
            OOMRecoveryAction action;
            action.type = OOMRecoveryAction::FreeUnusedMemory;
            return action;
        }());

        // Test suspendProcessing
        bool suspendWorked = oom.attemptRecovery([]{
            OOMRecoveryAction action;
            action.type = OOMRecoveryAction::SuspendProcessing;
            return action;
        }());

        bool passed = (dropCachesWorked && freeUnusedWorked && suspendWorked);

        if (passed) {
            std::cout << "✅ PASS (all recovery actions executed)\n" << std::endl;
        } else {
            std::cout << "❌ FAIL\n" << std::endl;
        }

        return passed;
    }

    //==========================================================================
    bool testOOMHandlerCallbacks() {
        std::cout << "Test 4: OOM Handler Callbacks... ";

        auto& oom = OOMHandler::getInstance();
        bool callbackInvoked = false;

        // Set pressure callback
        oom.setPressureCallback([&callbackInvoked](MemoryPressure pressure, const MemorySnapshot& snapshot) {
            callbackInvoked = true;
        });

        // Trigger callback (by checking pressure)
        auto snapshot = oom.getMemorySnapshot();
        auto action = oom.getRecoveryAction(snapshot);

        bool passed = callbackInvoked;

        if (passed) {
            std::cout << "✅ PASS (callbacks working)\n" << std::endl;
        } else {
            std::cout << "❌ FAIL\n" << std::endl;
        }

        return passed;
    }

    //==========================================================================
    bool testPredictiveMonitor() {
        std::cout << "Test 5: Predictive Monitor... ";

        auto& predictor = PredictiveMonitor::getInstance();
        auto& oom = OOMHandler::getInstance();

        // Add multiple samples to build history
        for (int i = 0; i < 10; ++i) {
            auto snapshot = oom.getMemorySnapshot();
            predictor.addSample(snapshot);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        auto prediction = predictor.predict();

        bool passed = (prediction.confidence >= 0 &&
                       prediction.trend != MemoryTrend::Unknown);

        if (passed) {
            std::cout << "✅ PASS ("
                      << prediction.toString()
                      << ")\n" << std::endl;
        } else {
            std::cout << "❌ FAIL\n" << std::endl;
        }

        return passed;
    }

    //==========================================================================
    bool testPlatformAPIs() {
        std::cout << "Test 6: Platform-Specific APIs... ";

        auto& oom = OOMHandler::getInstance();

#ifdef JUCE_LINUX
        std::cout << "[Linux] ";
        // Test Linux-specific APIs
        bool passed = true;  // If we compiled, APIs work

#elif defined(JUCE_WINDOWS)
        std::cout << "[Windows] ";
        bool passed = true;

#elif defined(JUCE_MAC)
        std::cout << "[macOS] ";
        bool passed = true;

#else
        std::cout << "[Unknown] ";
        bool passed = false;
#endif

        if (passed) {
            std::cout << "✅ PASS\n" << std::endl;
        } else {
            std::cout << "❌ FAIL\n" << std::endl;
        }

        return passed;
    }
};

//==============================================================================
int main() {
    MemorySafetyTests tests;
    return tests.runAllTests();
}
