/*
  ==============================================================================

    SampleGeneratorTests.cpp
    Created: 2024-05-24
    Author:  Zenith DAW

    Verification tests for SampleGenerator cancellation and atomicity.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../utils/SampleGenerator.h"
#include "../instruments/ContentPaths.h"

namespace zenith {
namespace tests {

class SampleGeneratorTests : public juce::UnitTest {
public:
  SampleGeneratorTests() : juce::UnitTest("SampleGeneratorTests", "Utils") {}

  void runTest() override {
    testCancellation();
  }

  void testCancellation() {
    beginTest("Sample Generation Cancellation");

    // Setup isolated temp environment
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                             .getChildFile("ZenithTest_Cancel_" + juce::String(juce::Random::getSystemRandom().nextInt()));
    if (tempDir.exists()) tempDir.deleteRecursively();
    tempDir.createDirectory();

    // Mock content paths
    ContentPaths::getInstance().setContentRoot(tempDir);

    juce::ThreadPool pool(1);

    // We want to trigger cancellation shortly after start.
    // Since we can't easily inject a delay into the job without modifying it for testing,
    // we rely on the fact that generating ~40 files takes ~200ms.
    // If we cancel after 10ms, we should see partial completion.

    std::atomic<bool> completed { false };
    SampleGenerator::generateMissingSamples(&pool, [&completed](){
        completed = true;
    });

    // Wait a tiny bit to let it start but not finish
    juce::Thread::sleep(10); // 10ms should generate a few files but not all 45

    // Trigger cancellation
    // removeAllJobs(true) sends the signal and waits for the job to exit
    pool.removeAllJobs(true, 2000);

    // Verify
    expect(!completed, "Job should not have called onComplete callback if cancelled");

    auto samplesDir = tempDir.getChildFile("Examples").getChildFile("SampleMaps").getChildFile("Samples");
    juce::Array<juce::File> files;
    if (samplesDir.exists()) {
        samplesDir.findChildFiles(files, juce::File::findFiles, false, "*.wav");
    }

    juce::Array<juce::File> tmpFiles;
    if (samplesDir.exists()) {
        samplesDir.findChildFiles(tmpFiles, juce::File::findFiles, false, "*.tmp");
    }

    logMessage("Files generated before cancel: " + juce::String(files.size()));

    expect(files.size() < 45, "Should not have generated all files (expected < 45, got " + juce::String(files.size()) + ")");
    expect(files.size() > 0, "Should have generated at least one file (got " + juce::String(files.size()) + ")");
    expect(tmpFiles.size() == 0, "No .tmp files should remain");

    // Check file integrity of one generated file
    if (files.size() > 0) {
        expect(files[0].getSize() > 0, "Generated file should not be empty");
    }

    // Cleanup
    tempDir.deleteRecursively();
    ContentPaths::getInstance().setContentRoot(juce::File());
  }
};

static SampleGeneratorTests sampleGeneratorTests;

} // namespace tests
} // namespace zenith

#ifdef SAMPLE_GENERATOR_TESTS_STANDALONE
int main() {
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
#endif
