/*
    MasterLimiterTest.cpp - Test program for MasterLimiter implementations

    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "MasterLimiter.h"
#include "MasterLimiterOptimized.h"
#include "MasterLimiterBenchmark.h"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace zenith;

void testBasicFunctionality() {
    std::cout << "Testing basic functionality..." << std::endl;

    // Test original implementation
    MasterLimiter original;
    original.prepare(48000.0, 512);
    original.setCeiling(-0.1f);
    original.setAttack(0.01f);
    original.setRelease(50.0f);

    // Test optimized implementation
    MasterLimiterOptimized optimized;
    optimized.prepare(48000.0, 512);
    optimized.setCeiling(-0.1f);
    optimized.setAttack(0.01f);
    optimized.setRelease(50.0f);

    // Create test signal
    juce::AudioBuffer<float> testBuffer(2, 512);
    testBuffer.clear();

    // Create a signal that needs limiting
    float* samples = testBuffer.getWritePointer(0);
    for (int i = 0; i < 512; ++i) {
        samples[i] = 1.5f * std::sin(2.0 * M_PI * 440.0 * i / 48000.0);
    }

    // Process with both implementations
    original.process(testBuffer);
    optimized.process(testBuffer);

    // Check results
    float originalGR = original.getGainReductionDb();
    float optimizedGR = optimized.getGainReductionDb();

    std::cout << "Original Gain Reduction: " << std::fixed << std::setprecision(2) << originalGR << " dB" << std::endl;
    std::cout << "Optimized Gain Reduction: " << std::fixed << std::setprecision(2) << optimizedGR << " dB" << std::endl;

    // They should be similar (within reasonable tolerance)
    float diff = std::abs(originalGR - optimizedGR);
    std::cout << "Difference: " << std::fixed << std::setprecision(2) << diff << " dB" << std::endl;

    assert(diff < 1.0f); // Allow 1 dB difference
    std::cout << "✓ Basic functionality test passed" << std::endl << std::endl;
}

void testPerformanceComparison() {
    std::cout << "Testing performance comparison..." << std::endl;

    const int iterations = 1000;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

    // Create test buffer
    juce::AudioBuffer<float> testBuffer(2, blockSize);
    testBuffer.clear();
    float* samples = testBuffer.getWritePointer(0);
    for (int i = 0; i < blockSize; ++i) {
        samples[i] = 1.2f * std::sin(2.0 * M_PI * 440.0 * i / sampleRate);
    }

    // Test original implementation
    MasterLimiter original;
    original.prepare(sampleRate, blockSize);

    auto startTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        original.process(testBuffer);
    }
    auto endTime = std::chrono::high_resolution_clock::now();

    double originalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // Test optimized implementation
    MasterLimiterOptimized optimized;
    optimized.prepare(sampleRate, blockSize);

    startTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        optimized.process(testBuffer);
    }
    endTime = std::chrono::high_resolution_clock::now();

    double optimizedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // Calculate improvement
    double improvement = (originalTime - optimizedTime) / originalTime * 100.0;

    std::cout << "Original Time: " << std::fixed << std::setprecision(2) << originalTime << " ms" << std::endl;
    std::cout << "Optimized Time: " << std::fixed << std::setprecision(2) << optimizedTime << " ms" << std::endl;
    std::cout << "Performance Improvement: " << std::fixed << std::setprecision(2) << improvement << "%" << std::endl;
    std::cout << "✓ Performance comparison completed" << std::endl << std::endl;
}

void testParameterChanges() {
    std::cout << "Testing parameter changes..." << std::endl;

    MasterLimiterOptimized optimized;
    optimized.prepare(48000.0, 512);

    // Test parameter setting
    optimized.setCeiling(-1.0f);
    optimized.setAttack(1.0f);
    optimized.setRelease(100.0f);
    optimized.setEnabled(false);

    assert(optimized.getCeiling() == -1.0f);
    assert(optimized.getAttack() == 1.0f);
    assert(optimized.getRelease() == 100.0f);
    assert(!optimized.isEnabled());

    std::cout << "✓ Parameter changes test passed" << std::endl << std::endl;
}

void testLatency() {
    std::cout << "Testing latency calculation..." << std::endl;

    MasterLimiterOptimized optimized;
    optimized.prepare(48000.0, 512);

    int latency = optimized.getLatency();
    std::cout << "Calculated latency: " << latency << " samples" << std::endl;

    // Latency should be positive and reasonable
    assert(latency > 0);
    assert(latency < 1000); // Less than 1000 samples at 48kHz

    std::cout << "✓ Latency test passed" << std::endl << std::endl;
}

void runFullBenchmark() {
    std::cout << "Running full benchmark..." << std::endl;
    MasterLimiterBenchmark::runBenchmark();
    MasterLimiterBenchmark::runAudioQualityTest();
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "Zenith DAW - MasterLimiter Test Suite" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;

    try {
        testBasicFunctionality();
        testPerformanceComparison();
        testParameterChanges();
        testLatency();
        runFullBenchmark();

        std::cout << "==================================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "==================================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}