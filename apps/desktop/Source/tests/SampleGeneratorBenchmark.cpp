/*
  ==============================================================================

    SampleGeneratorBenchmark.cpp
    Created: 2024-05-24
    Author:  Zenith DAW

    Benchmark for SampleGenerator startup performance.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../utils/SampleGenerator.h"
#include "../instruments/ContentPaths.h"

namespace zenith {
namespace tests {

class SampleGeneratorBenchmark : public juce::UnitTest {
public:
  SampleGeneratorBenchmark() : juce::UnitTest("SampleGeneratorBenchmark", "Performance") {}

  void runTest() override {
    runSyncTest();
    runAsyncTest();
  }

  void runSyncTest() {
    beginTest("Sample Generation Performance (Sync)");

    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                             .getChildFile("ZenithBenchmark_Sync_" + juce::String(juce::Random::getSystemRandom().nextInt()));
    if (tempDir.exists()) tempDir.deleteRecursively();
    tempDir.createDirectory();

    ContentPaths::getInstance().setContentRoot(tempDir);

    double start = juce::Time::getMillisecondCounterHiRes();
    SampleGenerator::generateMissingSamples(nullptr);
    double end = juce::Time::getMillisecondCounterHiRes();

    logMessage("Sync execution time: " + juce::String(end - start, 3) + " ms");

    // Check files
    auto samplesDir = tempDir.getChildFile("Examples").getChildFile("SampleMaps").getChildFile("Samples");
    if (samplesDir.exists()) {
        juce::Array<juce::File> files;
        samplesDir.findChildFiles(files, juce::File::findFiles, false, "*.wav");
        logMessage("Generated " + juce::String(files.size()) + " sample files (Sync).");
    }

    tempDir.deleteRecursively();
  }

  void runAsyncTest() {
    beginTest("Sample Generation Performance (Async)");

    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                             .getChildFile("ZenithBenchmark_Async_" + juce::String(juce::Random::getSystemRandom().nextInt()));
    if (tempDir.exists()) tempDir.deleteRecursively();
    tempDir.createDirectory();

    ContentPaths::getInstance().setContentRoot(tempDir);

    juce::ThreadPool pool(1); // Single thread pool

    double start = juce::Time::getMillisecondCounterHiRes();

    // Use a WaitableEvent to measure completion time (optional, but good for verification)
    juce::WaitableEvent event;

    SampleGenerator::generateMissingSamples(&pool, [&event](){
        event.signal();
    });

    double afterCall = juce::Time::getMillisecondCounterHiRes();
    double dispatchTime = afterCall - start;

    logMessage("Async dispatch time (Main Thread Block): " + juce::String(dispatchTime, 3) + " ms");

    // Wait for completion to ensure logic actually runs
    bool completed = event.wait(5000);
    double finish = juce::Time::getMillisecondCounterHiRes();

    if (completed) {
        logMessage("Total async time (wall clock): " + juce::String(finish - start, 3) + " ms");

        auto samplesDir = tempDir.getChildFile("Examples").getChildFile("SampleMaps").getChildFile("Samples");
        if (samplesDir.exists()) {
            juce::Array<juce::File> files;
            samplesDir.findChildFiles(files, juce::File::findFiles, false, "*.wav");
            logMessage("Generated " + juce::String(files.size()) + " sample files (Async).");
        }
    } else {
        logMessage("Async generation timed out!");
    }

    tempDir.deleteRecursively();
    ContentPaths::getInstance().setContentRoot(juce::File());
  }
};

static SampleGeneratorBenchmark sampleGeneratorBenchmark;

} // namespace tests
} // namespace zenith

#ifdef SAMPLE_GENERATOR_BENCHMARK_STANDALONE
int main() {
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
#endif
