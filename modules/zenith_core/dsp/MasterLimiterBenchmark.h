/*
    MasterLimiterBenchmark.h - Benchmark tool for MasterLimiter performance

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

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "MasterLimiter.h"
#include "MasterLimiterOptimized.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>

namespace zenith {

/**
 * @class MasterLimiterBenchmark
 * @brief Performance benchmark tool for MasterLimiter implementations
 *
 * Compares performance between original and optimized MasterLimiter implementations
 */
class MasterLimiterBenchmark {
public:
  /**
   * @brief Run comprehensive benchmark
   * @param iterations Number of iterations for each test
   * @param sampleRates List of sample rates to test
   * @param blockSizes List of block sizes to test
   */
  static void runBenchmark(int iterations = 1000,
                          const std::vector<double>& sampleRates = {44100.0, 48000.0, 96000.0},
                          const std::vector<int>& blockSizes = {64, 128, 256, 512, 1024}) {

    std::cout << "==================================================" << std::endl;
    std::cout << "Zenith DAW - MasterLimiter Performance Benchmark" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;

    // Test signal generation
    std::vector<std::unique_ptr<juce::AudioBuffer<float>>> testBuffers;
    generateTestBuffers(testBuffers, blockSizes);

    for (auto sampleRate : sampleRates) {
      std::cout << "Sample Rate: " << sampleRate << " Hz" << std::endl;
      std::cout << "----------------------------" << std::endl;

      for (auto blockSize : blockSizes) {
        std::cout << "Block Size: " << blockSize << " samples" << std::endl;
        std::cout << "           Original     Optimized      Improvement" << std::endl;
        std::cout << "           (ms)         (ms)           (%)" << std::endl;

        // Find buffer for this block size
        auto bufferIt = std::find_if(testBuffers.begin(), testBuffers.end(),
            [blockSize](const auto& buf) {
                return buf->getNumSamples() == blockSize;
            });

        if (bufferIt != testBuffers.end()) {
          auto& buffer = *bufferIt;
          auto result = runSingleBenchmark(sampleRate, buffer.get(), iterations);

          std::cout << "Processing: ";
          std::cout << std::fixed << std::setprecision(4);
          std::cout << std::setw(10) << result.originalTime << "   ";
          std::cout << std::setw(10) << result.optimizedTime << "   ";
          std::cout << std::setw(10) << result.improvement << "%" << std::endl;

          std::cout << "Per Sample: ";
          std::cout << std::fixed << std::setprecision(2);
          std::cout << std::setw(10) << result.originalPerSample << " ns   ";
          std::cout << std::setw(10) << result.optimizedPerSample << " ns" << std::endl;
        }

        std::cout << std::endl;
      }
      std::cout << std::endl;
    }

    // Run detailed stress test
    runStressTest();
  }

  /**
   * @brief Run audio quality test to verify no sonic degradation
   */
  static void runAudioQualityTest() {
    std::cout << "==================================================" << std::endl;
    std::cout << "Zenith DAW - MasterLimiter Audio Quality Test" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;

    const int testIterations = 1000;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

    // Create test signals
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(2, blockSize);
    generateTestSignal(*buffer, sampleRate, TestSignalType::Complex);

    // Process with both implementations
    MasterLimiter original;
    MasterLimiterOptimized optimized;

    original.prepare(sampleRate, blockSize);
    optimized.prepare(sampleRate, blockSize);

    std::vector<float> originalOutput(blockSize * 2);
    std::vector<float> optimizedOutput(blockSize * 2);

    // Process and capture outputs
    for (int i = 0; i < testIterations; ++i) {
      buffer->makeCopyOf(originalOutput.data());
      original.process(*buffer);

      buffer->makeCopyOf(optimizedOutput.data());
      optimized.process(*buffer);
    }

    // Compare outputs
    double maxDiff = calculateMaxDifference(originalOutput, optimizedOutput);
    double rmsDiff = calculateRMSDifference(originalOutput, optimizedOutput);

    std::cout << "Audio Quality Test Results:" << std::endl;
    std::cout << "-------------------------" << std::endl;
    std::cout << "Max Difference: " << std::scientific << maxDiff << " dB" << std::endl;
    std::cout << "RMS Difference: " << std::scientific << rmsDiff << " dB" << std::endl;
    std::cout << "Status: " << (maxDiff < -120.0 ? "PASS" : "FAIL") << std::endl;
    std::cout << std::endl;
  }

private:
  /**
   * @brief Benchmark result structure
   */
  struct BenchmarkResult {
    double originalTime;
    double optimizedTime;
    double improvement;
    double originalPerSample;
    double optimizedPerSample;
  };

  /**
   * @brief Test signal types
   */
  enum class TestSignalType {
    Sine,
    Square,
    Complex,
    Noise
  };

  /**
   * @brief Generate test buffers for different block sizes
   */
  static void generateTestBuffers(std::vector<std::unique_ptr<juce::AudioBuffer<float>>>& buffers,
                                const std::vector<int>& blockSizes) {
    for (auto blockSize : blockSizes) {
      auto buffer = std::make_unique<juce::AudioBuffer<float>>(2, blockSize);
      generateTestSignal(*buffer, 48000.0, TestSignalType::Complex);
      buffers.push_back(std::move(buffer));
    }
  }

  /**
   * @brief Generate test signal
   */
  static void generateTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate, TestSignalType type) {
    const int numSamples = buffer.getNumSamples();
    const double freq = 440.0; // A4 note

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      float* samples = buffer.getWritePointer(ch);

      for (int sample = 0; sample < numSamples; ++sample) {
        double time = static_cast<double>(sample) / sampleRate;
        double value = 0.0;

        switch (type) {
          case TestSignalType::Sine:
            value = std::sin(2.0 * M_PI * freq * time);
            break;
          case TestSignalType::Square:
            value = std::sin(2.0 * M_PI * freq * time) > 0 ? 1.0f : -1.0f;
            break;
          case TestSignalType::Complex:
            // Complex mix of frequencies with some harmonics
            value = 0.5 * std::sin(2.0 * M_PI * freq * time);
            value += 0.3 * std::sin(2.0 * M_PI * freq * 2.0 * time);
            value += 0.2 * std::sin(2.0 * M_PI * freq * 3.0 * time);
            // Add some noise
            value += 0.1 * (static_cast<float>(rand()) / RAND_MAX - 0.5f);
            break;
          case TestSignalType::Noise:
            value = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
            break;
        }

        samples[sample] = static_cast<float>(value) * 0.8f; // Leave some headroom
      }
    }
  }

  /**
   * @brief Run single benchmark configuration
   */
  static BenchmarkResult runSingleBenchmark(double sampleRate, juce::AudioBuffer<float>* buffer, int iterations) {
    MasterLimiter original;
    MasterLimiterOptimized optimized;

    original.prepare(sampleRate, buffer->getNumSamples());
    optimized.prepare(sampleRate, buffer->getNumSamples());

    // Benchmark original implementation
    auto originalBuffer = buffer->makeCopyOf();
    double originalStartTime = getMilliseconds();

    for (int i = 0; i < iterations; ++i) {
      original.process(*originalBuffer);
    }

    double originalEndTime = getMilliseconds();
    double originalTime = originalEndTime - originalStartTime;

    // Benchmark optimized implementation
    auto optimizedBuffer = buffer->makeCopyOf();
    double optimizedStartTime = getMilliseconds();

    for (int i = 0; i < iterations; ++i) {
      optimized.process(*optimizedBuffer);
    }

    double optimizedEndTime = getMilliseconds();
    double optimizedTime = optimizedEndTime - optimizedStartTime;

    // Calculate metrics
    double improvement = originalTime > 0 ?
        (originalTime - optimizedTime) / originalTime * 100.0 : 0.0;

    double originalPerSample = originalTime > 0 ? originalTime / (iterations * buffer->getNumSamples()) * 1000000.0 : 0.0;
    double optimizedPerSample = optimizedTime > 0 ? optimizedTime / (iterations * buffer->getNumSamples()) * 1000000.0 : 0.0;

    return {originalTime, optimizedTime, improvement, originalPerSample, optimizedPerSample};
  }

  /**
   * @brief Run stress test
   */
  static void runStressTest() {
    std::cout << "==================================================" << std::endl;
    std::cout << "Zenith DAW - MasterLimiter Stress Test" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;

    const int stressIterations = 10000;
    const double sampleRate = 96000.0; // High sample rate stress test
    const int blockSize = 1024;

    auto buffer = std::make_unique<juce::AudioBuffer<float>>(2, blockSize);
    generateTestSignal(*buffer, sampleRate, TestSignalType::Complex);

    MasterLimiterOptimized optimized;
    optimized.prepare(sampleRate, blockSize);

    std::cout << "Running " << stressIterations << " iterations at " << sampleRate << " Hz..." << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < stressIterations; ++i) {
      optimized.process(*buffer);

      // Progress indicator
      if (i % 1000 == 0) {
        std::cout << "Progress: " << (i * 100) / stressIterations << "%" << std::endl;
      }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "Stress Test Complete!" << std::endl;
    std::cout << "---------------------" << std::endl;
    std::cout << "Total Time: " << duration.count() << " ms" << std::endl;
    std::cout << "Average Processing Time: "
              << std::fixed << std::setprecision(4)
              << duration.count() / static_cast<double>(stressIterations) << " ms" << std::endl;
    std::cout << "Throughput: "
              << std::fixed << std::setprecision(2)
              << (stressIterations * blockSize) / (duration.count() / 1000.0) << " samples/sec" << std::endl;
    std::cout << std::endl;
  }

  /**
   * @brief Calculate maximum difference between signals
   */
  static double calculateMaxDifference(const std::vector<float>& signal1, const std::vector<float>& signal2) {
    double maxDiff = 0.0;

    for (size_t i = 0; i < signal1.size(); ++i) {
      double diff = std::abs(signal1[i] - signal2[i]);
      maxDiff = juce::jmax(maxDiff, diff);
    }

    // Convert to dB
    return maxDiff > 0.0 ? 20.0 * std::log10(maxDiff) : -200.0;
  }

  /**
   * @brief Calculate RMS difference between signals
   */
  static double calculateRMSDifference(const std::vector<float>& signal1, const std::vector<float>& signal2) {
    double sumSquaredDiff = 0.0;

    for (size_t i = 0; i < signal1.size(); ++i) {
      double diff = signal1[i] - signal2[i];
      sumSquaredDiff += diff * diff;
    }

    double rmsDiff = std::sqrt(sumSquaredDiff / signal1.size());
    return rmsDiff > 0.0 ? 20.0 * std::log10(rmsDiff) : -200.0;
  }

  /**
   * @brief Get current time in milliseconds
   */
  static double getMilliseconds() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  }
};

} // namespace zenith