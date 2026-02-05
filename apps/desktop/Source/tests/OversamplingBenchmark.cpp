/*
  ==============================================================================

    OversamplingBenchmark.cpp
    Created: 2025-12-10
    Author:  Zenith DAW

    Benchmark for ZenithPolySynthVoice oversampling switching performance.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../instruments/ZenithPolySynthVoice.h"

namespace zenith {
namespace tests {

class OversamplingBenchmark : public juce::UnitTest {
public:
  OversamplingBenchmark() : juce::UnitTest("OversamplingBenchmark", "OversamplingPerformance") {}

  void runTest() override {
    beginTest("Quality Preset Switching Performance");

    ZenithPolySynthVoice voice;
    // Prepare voice
    voice.setCurrentPlaybackSampleRate(44100.0);
    voice.setSampleRate(44100.0);

    const int numIterations = 100;
    double totalTime = 0.0;

    // Warm up
    voice.setQualityPreset(QualityPreset::Medium);
    voice.setQualityPreset(QualityPreset::High);

    double start = juce::Time::getMillisecondCounterHiRes();

    for (int i = 0; i < numIterations; ++i) {
        // Toggle between Medium (2x) and High (4x) which currently triggers reallocation
        voice.setQualityPreset(QualityPreset::Medium);
        voice.setQualityPreset(QualityPreset::High);

        // Toggle between Low (1x) and High (4x)
        voice.setQualityPreset(QualityPreset::Low);
        voice.setQualityPreset(QualityPreset::High);
    }

    double end = juce::Time::getMillisecondCounterHiRes();
    totalTime = end - start;

    // 4 switches per iteration
    double avgTimePerSwitch = totalTime / (numIterations * 4.0);

    logMessage("Total time for " + juce::String(numIterations * 4) + " switches: " + juce::String(totalTime, 3) + " ms");
    logMessage("Average time per switch: " + juce::String(avgTimePerSwitch, 4) + " ms");

    expect(totalTime > 0.0);
  }
};

static OversamplingBenchmark oversamplingBenchmark;

} // namespace tests
} // namespace zenith

#ifdef OVERSAMPLING_BENCHMARK_STANDALONE
int main() {
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
#endif
