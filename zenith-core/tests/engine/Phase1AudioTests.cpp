/**
 * @file Phase1AudioTests.cpp
 * @brief Headless test harness for Phase 1 audio engine
 *
 * Tests clip rendering, sample-accurate scheduling, and fade envelopes
 * without requiring an audio device or UI.
 */

#include <JuceHeader.h>
#include "../../Source/engine/Mixer.h"
#include "../../Source/engine/Track.h"
#include "../../Source/engine/Clip.h"
#include "../../include/Engine.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <iomanip>

//==============================================================================
// Test Framework
//==============================================================================

static int g_testsRun = 0;
static int g_testsFailed = 0;

#define TEST(name) \
    static void name(); \
    static struct name##_Register { \
        name##_Register() { \
            std::cout << "Running test: " #name << std::endl; \
            g_testsRun++; \
            name(); \
            std::cout << "✓ " #name << " passed\n" << std::endl; \
        } \
    } name##_instance; \
    static void name()

#define ASSERT_NEAR(actual, expected, epsilon) \
    do { \
        float _actual = (actual); \
        float _expected = (expected); \
        float _epsilon = (epsilon); \
        if (std::abs(_actual - _expected) > _epsilon) { \
            std::cerr << "ASSERTION FAILED: " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "  Expected: " << _expected << std::endl; \
            std::cerr << "  Actual: " << _actual << std::endl; \
            std::cerr << "  Epsilon: " << _epsilon << std::endl; \
            g_testsFailed++; \
            std::exit(1); \
        } \
    } while (0)

#define ASSERT_EQ(actual, expected) \
    do { \
        auto _actual = (actual); \
        auto _expected = (expected); \
        if (_actual != _expected) { \
            std::cerr << "ASSERTION FAILED: " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "  Expected: " << _expected << std::endl; \
            std::cerr << "  Actual: " << _actual << std::endl; \
            g_testsFailed++; \
            std::exit(1); \
        } \
    } while (0)

//==============================================================================
// Test Helpers
//==============================================================================

/**
 * @brief Create an impulse clip (1.0f at index 0, zeros elsewhere)
 */
zenith::Clip createImpulseClip(int lengthSamples)
{
    zenith::Clip clip;
    clip.pcm = std::make_shared<juce::AudioBuffer<float>>(1, lengthSamples);
    clip.pcm->clear();
    clip.pcm->setSample(0, 0, 1.0f); // Impulse at start
    clip.lengthSamples = lengthSamples;
    clip.srcOffset = 0;
    clip.gain = 1.0f;
    clip.fadeInSamples = 0;
    clip.fadeOutSamples = 0;
    return clip;
}

/**
 * @brief Create a flat clip (all samples = 1.0f)
 */
zenith::Clip createFlatClip(int lengthSamples, float value = 1.0f)
{
    zenith::Clip clip;
    clip.pcm = std::make_shared<juce::AudioBuffer<float>>(1, lengthSamples);
    clip.pcm->clear();
    for (int i = 0; i < lengthSamples; ++i)
        clip.pcm->setSample(0, i, value);
    clip.lengthSamples = lengthSamples;
    clip.srcOffset = 0;
    clip.gain = 1.0f;
    clip.fadeInSamples = 0;
    clip.fadeOutSamples = 0;
    return clip;
}

/**
 * @brief Render a single segment and append to timeline
 */
void renderSegment(zenith::Mixer& mixer,
                   juce::AudioBuffer<float>& mixBuffer,
                   std::vector<float>& timeline,
                   int segmentLength,
                   int64_t timelineSample)
{
    mixBuffer.clear();
    mixer.processSegment(mixBuffer, 0, segmentLength, timelineSample);

    // Append to timeline (mono)
    const float* src = mixBuffer.getReadPointer(0, 0);
    for (int i = 0; i < segmentLength; ++i)
        timeline.push_back(src[i]);
}

/**
 * @brief Find first non-zero sample in timeline
 * @return Index of first non-zero sample, or -1 if all zeros
 */
int findFirstNonZero(const std::vector<float>& timeline, float threshold = 0.001f)
{
    for (size_t i = 0; i < timeline.size(); ++i)
    {
        if (std::abs(timeline[i]) > threshold)
            return static_cast<int>(i);
    }
    return -1;
}

/**
 * @brief Print timeline excerpt for debugging
 */
void printTimeline(const std::vector<float>& timeline, int start, int end)
{
    std::cout << std::fixed << std::setprecision(3);
    for (int i = start; i < end && i < static_cast<int>(timeline.size()); ++i)
    {
        std::cout << "  [" << i << "] = " << timeline[i] << std::endl;
    }
}

//==============================================================================
// Test 1: Clip starts exactly at scheduled sample
//==============================================================================

TEST(ClipStartsExactlyAtScheduledSample)
{
    // Setup
    zenith::Mixer mixer;
    int trackIdx = mixer.addTrack();
    zenith::Track* track = mixer.getTrack(trackIdx);
    ASSERT_EQ(track != nullptr, true);

    // Create impulse clip (1.0f at first sample)
    const int64_t clipId = 1001;
    const int64_t scheduleAt = 1000; // Start at sample 1000
    zenith::Clip clip = createImpulseClip(500);

    zenith::ClipDef clipDef(clipId, clip);
    track->addClipDefinition(clipDef);

    // Trigger clip start at sample 1000
    Engine::TransportEvent startEvent(
        Engine::TransportEventType::ClipStart,
        scheduleAt,
        trackIdx,
        clipId
    );

    // Render timeline
    const int blockSize = 256;
    const int totalSamples = 1200;
    juce::AudioBuffer<float> mixBuffer(1, blockSize);
    std::vector<float> timeline;

    int64_t currentSample = 0;
    while (currentSample < totalSamples)
    {
        // Fire event exactly at sample 1000
        if (currentSample <= scheduleAt && currentSample + blockSize > scheduleAt)
        {
            int offsetInBlock = static_cast<int>(scheduleAt - currentSample);
            track->handleEventRT(startEvent, offsetInBlock, scheduleAt);
        }

        int samplesToRender = std::min(blockSize, static_cast<int>(totalSamples - currentSample));
        renderSegment(mixer, mixBuffer, timeline, samplesToRender, currentSample);
        currentSample += samplesToRender;
    }

    // Assertions
    ASSERT_EQ(timeline.size(), static_cast<size_t>(totalSamples));

    // All samples before 1000 must be exactly 0.0
    for (int i = 0; i < scheduleAt; ++i)
    {
        ASSERT_NEAR(timeline[i], 0.0f, 0.0001f);
    }

    // First non-zero sample must be at index 1000
    int firstNonZero = findFirstNonZero(timeline);
    ASSERT_EQ(firstNonZero, static_cast<int>(scheduleAt));

    // Sample at 1000 should be ≈1.0 (impulse)
    ASSERT_NEAR(timeline[scheduleAt], 1.0f, 0.01f);

    std::cout << "  First non-zero sample at index: " << firstNonZero << std::endl;
    std::cout << "  Value at index 1000: " << timeline[1000] << std::endl;
}

//==============================================================================
// Test 2: Fade-in and fade-out have correct linear shape
//==============================================================================

TEST(FadeInAndFadeOutHaveCorrectShape)
{
    // Setup
    zenith::Mixer mixer;
    int trackIdx = mixer.addTrack();
    zenith::Track* track = mixer.getTrack(trackIdx);

    // Create flat clip (all 1.0f) with fades
    const int64_t clipId = 2001;
    const int fadeInSamples = 100;
    const int fadeOutSamples = 100;
    const int steadySamples = 200;
    const int totalClipLength = fadeInSamples + steadySamples + fadeOutSamples;

    zenith::Clip clip = createFlatClip(totalClipLength, 1.0f);
    clip.fadeInSamples = fadeInSamples;
    clip.fadeOutSamples = fadeOutSamples;

    zenith::ClipDef clipDef(clipId, clip);
    track->addClipDefinition(clipDef);

    // Start clip at sample 0
    Engine::TransportEvent startEvent(
        Engine::TransportEventType::ClipStart,
        0,
        trackIdx,
        clipId
    );
    track->handleEventRT(startEvent, 0, 0);

    // Trigger fade-out at the right time
    const int64_t fadeOutStartSample = fadeInSamples + steadySamples;
    Engine::TransportEvent stopEvent(
        Engine::TransportEventType::ClipStop,
        fadeOutStartSample,
        trackIdx,
        clipId
    );

    // Render timeline
    const int blockSize = 64;
    const int totalSamples = totalClipLength + 50; // Extra padding
    juce::AudioBuffer<float> mixBuffer(1, blockSize);
    std::vector<float> timeline;

    int64_t currentSample = 0;
    while (currentSample < totalSamples)
    {
        // Trigger stop event at fadeOutStartSample
        if (currentSample <= fadeOutStartSample && currentSample + blockSize > fadeOutStartSample)
        {
            int offsetInBlock = static_cast<int>(fadeOutStartSample - currentSample);
            track->handleEventRT(stopEvent, offsetInBlock, fadeOutStartSample);
        }

        int samplesToRender = std::min(blockSize, static_cast<int>(totalSamples - currentSample));
        renderSegment(mixer, mixBuffer, timeline, samplesToRender, currentSample);
        currentSample += samplesToRender;
    }

    // Assertions: Fade-in
    std::cout << "  Checking fade-in (0 → " << fadeInSamples << ")..." << std::endl;
    ASSERT_NEAR(timeline[0], 0.0f, 0.01f); // Start at 0

    // Middle of fade-in should be ≈0.5
    int fadeInMid = fadeInSamples / 2;
    float expectedMidGain = static_cast<float>(fadeInMid) / fadeInSamples;
    ASSERT_NEAR(timeline[fadeInMid], expectedMidGain, 0.02f);
    std::cout << "    Fade-in mid [" << fadeInMid << "] = " << timeline[fadeInMid]
              << " (expected ≈" << expectedMidGain << ")" << std::endl;

    // End of fade-in should be ≈1.0
    ASSERT_NEAR(timeline[fadeInSamples - 1], 1.0f, 0.02f);

    // Assertions: Steady region (all ≈1.0)
    std::cout << "  Checking steady region (" << fadeInSamples << " → " << fadeOutStartSample << ")..." << std::endl;
    for (int i = fadeInSamples; i < fadeOutStartSample; ++i)
    {
        ASSERT_NEAR(timeline[i], 1.0f, 0.02f);
    }
    std::cout << "    Steady region all ≈1.0 ✓" << std::endl;

    // Assertions: Fade-out
    std::cout << "  Checking fade-out (" << fadeOutStartSample << " → " << (fadeOutStartSample + fadeOutSamples) << ")..." << std::endl;

    // Start of fade-out should still be ≈1.0
    ASSERT_NEAR(timeline[fadeOutStartSample], 1.0f, 0.02f);

    // Middle of fade-out
    int fadeOutMid = fadeOutStartSample + fadeOutSamples / 2;
    float expectedFadeOutMidGain = 1.0f - (static_cast<float>(fadeOutSamples / 2) / fadeOutSamples);
    ASSERT_NEAR(timeline[fadeOutMid], expectedFadeOutMidGain, 0.05f);
    std::cout << "    Fade-out mid [" << fadeOutMid << "] = " << timeline[fadeOutMid]
              << " (expected ≈" << expectedFadeOutMidGain << ")" << std::endl;

    // End of fade-out should be ≈0.0
    int fadeOutEnd = fadeOutStartSample + fadeOutSamples - 1;
    if (fadeOutEnd < static_cast<int>(timeline.size()))
    {
        ASSERT_NEAR(timeline[fadeOutEnd], 0.0f, 0.05f);
    }

    // After fade-out completes, all samples should be 0.0
    std::cout << "  Checking silence after fade-out..." << std::endl;
    for (int i = fadeOutStartSample + fadeOutSamples; i < static_cast<int>(timeline.size()); ++i)
    {
        ASSERT_NEAR(timeline[i], 0.0f, 0.001f);
    }
    std::cout << "    Post-fade-out silence ✓" << std::endl;
}

//==============================================================================
// Test 3: StopClip triggers fade-out and silence after completion
//==============================================================================

TEST(StopClipTriggersFadeOutAndSilenceAfterEnd)
{
    // Setup
    zenith::Mixer mixer;
    int trackIdx = mixer.addTrack();
    zenith::Track* track = mixer.getTrack(trackIdx);

    // Create long flat clip with fade-out
    const int64_t clipId = 3001;
    const int fadeOutSamples = 100;
    const int clipLength = 1000;

    zenith::Clip clip = createFlatClip(clipLength, 1.0f);
    clip.fadeInSamples = 0; // No fade-in
    clip.fadeOutSamples = fadeOutSamples;

    zenith::ClipDef clipDef(clipId, clip);
    track->addClipDefinition(clipDef);

    // Start clip at sample 0
    Engine::TransportEvent startEvent(
        Engine::TransportEventType::ClipStart,
        0,
        trackIdx,
        clipId
    );
    track->handleEventRT(startEvent, 0, 0);

    // Schedule stop at sample 500 (middle of clip)
    const int64_t stopAtSample = 500;
    Engine::TransportEvent stopEvent(
        Engine::TransportEventType::ClipStop,
        stopAtSample,
        trackIdx,
        clipId
    );

    // Render timeline
    const int blockSize = 128;
    const int totalSamples = stopAtSample + fadeOutSamples + 100; // Extra padding
    juce::AudioBuffer<float> mixBuffer(1, blockSize);
    std::vector<float> timeline;

    int64_t currentSample = 0;
    while (currentSample < totalSamples)
    {
        // Trigger stop event at stopAtSample
        if (currentSample <= stopAtSample && currentSample + blockSize > stopAtSample)
        {
            int offsetInBlock = static_cast<int>(stopAtSample - currentSample);
            track->handleEventRT(stopEvent, offsetInBlock, stopAtSample);
        }

        int samplesToRender = std::min(blockSize, static_cast<int>(totalSamples - currentSample));
        renderSegment(mixer, mixBuffer, timeline, samplesToRender, currentSample);
        currentSample += samplesToRender;
    }

    // Assertions: Before stop, signal is ≈1.0
    std::cout << "  Checking pre-stop region (0 → " << stopAtSample << ")..." << std::endl;
    for (int i = 0; i < stopAtSample; ++i)
    {
        ASSERT_NEAR(timeline[i], 1.0f, 0.01f);
    }
    std::cout << "    Pre-stop all ≈1.0 ✓" << std::endl;

    // At stop point, should still be ≈1.0 (fade hasn't started yet)
    ASSERT_NEAR(timeline[stopAtSample], 1.0f, 0.01f);

    // During fade-out, signal should ramp down smoothly
    std::cout << "  Checking fade-out ramp (" << stopAtSample << " → " << (stopAtSample + fadeOutSamples) << ")..." << std::endl;

    // Check no discontinuities in fade-out
    for (int i = stopAtSample + 1; i < stopAtSample + fadeOutSamples; ++i)
    {
        if (i >= static_cast<int>(timeline.size())) break;

        // Value should be decreasing monotonically
        if (i > stopAtSample)
        {
            float current = timeline[i];
            float previous = timeline[i - 1];

            // Allow small increases due to float precision, but check general trend
            if (current > previous + 0.05f)
            {
                std::cerr << "Non-monotonic fade at [" << i << "]: " << previous << " → " << current << std::endl;
                ASSERT_EQ(false, true); // Fail
            }
        }
    }
    std::cout << "    Fade-out monotonic ✓" << std::endl;

    // After fade-out completes, all samples should be 0.0
    std::cout << "  Checking silence after fade-out..." << std::endl;
    int silenceStart = stopAtSample + fadeOutSamples;
    for (int i = silenceStart; i < static_cast<int>(timeline.size()); ++i)
    {
        ASSERT_NEAR(timeline[i], 0.0f, 0.001f);
    }
    std::cout << "    Post-fade-out silence ✓" << std::endl;

    // Print excerpt for manual inspection
    std::cout << "  Timeline excerpt around stop point:" << std::endl;
    printTimeline(timeline, stopAtSample - 5, stopAtSample + 15);
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[])
{
    juce::ignoreUnused(argc, argv);

    std::cout << "========================================" << std::endl;
    std::cout << "Phase 1 Audio Engine Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // JUCE init (minimal, no GUI)
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Tests run via static constructors

    std::cout << "========================================" << std::endl;
    std::cout << "Test Results" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests run: " << g_testsRun << std::endl;
    std::cout << "Tests failed: " << g_testsFailed << std::endl;
    std::cout << std::endl;

    if (g_testsFailed == 0)
    {
        std::cout << "✓ All tests passed!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "✗ Some tests failed" << std::endl;
        return 1;
    }
}
