/*
  ==============================================================================

    WavetableLoaderBenchmark.cpp
    Created: 2024-05-23
    Author:  Zenith DAW

    Benchmark for WavetableLoader resampling performance.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../instruments/WavetableLoader.h"

namespace zenith {
namespace tests {

class WavetableLoaderBenchmark : public juce::UnitTest {
public:
  WavetableLoaderBenchmark() : juce::UnitTest("WavetableLoaderBenchmark", "Performance") {}

  void runTest() override {
    beginTest("Resampling Performance");

    WavetableLoader loader;

    // Setup: Create a large buffer simulating multiple frames
    // We use a frame size of 2000 samples to force resampling to 2048 (WAVETABLE_FRAME_SIZE)
    const int sourceFrameSize = 2000;
    const int numFrames = 500; // Enough frames to make the test measurable
    const int numSamples = sourceFrameSize * numFrames;

    std::vector<float> data(numSamples);
    juce::Random rng;
    for (int i = 0; i < numSamples; ++i) {
        data[i] = rng.nextFloat() * 2.0f - 1.0f;
    }

    // Benchmark
    const int numIterations = 20;
    double totalTime = 0.0;

    for (int i = 0; i < numIterations; ++i) {
        double start = juce::Time::getMillisecondCounterHiRes();

        auto result = loader.loadFromBuffer(data.data(), numSamples, sourceFrameSize);

        double end = juce::Time::getMillisecondCounterHiRes();
        totalTime += (end - start);

        expect(result.success, "Load failed");
        expect(result.wavetable != nullptr, "Wavetable is null");
        expectEquals(result.wavetable->getNumFrames(), numFrames, "Incorrect number of frames");
    }

    double averageTime = totalTime / numIterations;
    logMessage(juce::String("Average time per loadFromBuffer (") + juce::String(numFrames) + " frames): " + juce::String(averageTime, 3) + " ms");
  }
};

static WavetableLoaderBenchmark wavetableLoaderBenchmark;

} // namespace tests
} // namespace zenith

#ifdef WAVETABLE_BENCHMARK_STANDALONE
int main() {
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
#endif
