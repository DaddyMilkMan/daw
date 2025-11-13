/**
 * @file Phase1AudioTests.cpp
 * @brief Headless regression tests for W10+W10.2+W10.3 (scheduler + voices + fades)
 *
 * Tests run offline without audio device, directly exercising the same
 * Engine/Mixer/AudioTrack/voices path that real playback uses.
 *
 * Compile with: ZENITH_ENABLE_PHASE1_AUDIO=ON
 */

#include <JuceHeader.h>
#include "../include/Engine.h"
#include "../include/AudioTrack.h"
#include <vector>
#include <cmath>

// Compile-time guard - these tests require Phase 1 audio
#if !defined(ZENITH_ENABLE_PHASE1_AUDIO) || !ZENITH_ENABLE_PHASE1_AUDIO
    #error "Phase1AudioTests requires ZENITH_ENABLE_PHASE1_AUDIO=ON"
#endif

//==============================================================================
// Test Utilities
//==============================================================================

namespace TestUtil {

/** Create a simple mono PCM clip with constant value (or impulse). */
juce::AudioBuffer<float> createTestClip(int numSamples, float value = 1.0f)
{
    juce::AudioBuffer<float> clip(1, numSamples);
    for (int i = 0; i < numSamples; ++i)
        clip.setSample(0, i, value);
    return clip;
}

/** Floating-point comparison with epsilon. */
bool approxEqual(float a, float b, float epsilon = 1e-3f)
{
    return std::abs(a - b) < epsilon;
}

/** Assert with descriptive message. */
void assertWithMessage(bool condition, const juce::String& message)
{
    if (!condition)
    {
        DBG("ASSERTION FAILED: " + message);
        jassertfalse;
        throw std::runtime_error(message.toStdString());
    }
}

} // namespace TestUtil

//==============================================================================
// Test Harness
//==============================================================================

class Phase1TestHarness
{
public:
    Phase1TestHarness(int sampleRate, int blockSize)
        : sampleRate_(sampleRate), blockSize_(blockSize)
    {
        // Create engine (no audio device)
        engine_ = std::make_unique<Engine>();

        // Initialize engine
        engine_->prepareToPlay(sampleRate, blockSize);

        // Allocate test buffer (stereo)
        testBuffer_.setSize(2, blockSize);

        DBG("Test harness initialized: " + juce::String(sampleRate) + " Hz, " + juce::String(blockSize) + " samples/block");
    }

    ~Phase1TestHarness()
    {
        engine_->releaseResources();
    }

    /** Add a track and return its index. */
    int addTrack(const juce::String& name)
    {
        auto* mixer = &engine_->getMixer();
        auto* track = mixer->addTrack(name);
        return track->getIndex();
    }

    /** Add a ClipDef to a track (message-thread operation). */
    void addClipDef(int trackIndex, int32_t clipId, juce::AudioBuffer<float>* audioData,
                   int64_t startSample, int64_t lengthSamples, float gain,
                   int fadeInSamples, int fadeOutSamples)
    {
        auto* mixer = &engine_->getMixer();
        auto* track = mixer->getTrack(trackIndex);
        jassert(track != nullptr);

        // Access clipDefs_ (normally private, but we're a test harness)
        // For now, use the public API we have: we need to add a ClipDef directly
        // Since we don't have a public API yet, we'll use the old AudioClip API
        // and manually schedule StartClip events to trigger voice allocation.

        // Store clip data for later reference
        clipData_[clipId] = ClipData{audioData, startSample, lengthSamples, gain, fadeInSamples, fadeOutSamples};
        trackForClip_[clipId] = trackIndex;
    }

    /** Schedule a StartClip event. */
    void scheduleStartClip(int trackIndex, int32_t clipId, int64_t atSample)
    {
        bool success = engine_->scheduleClipStart(trackIndex, clipId, atSample);
        jassert(success);
    }

    /** Schedule a StopClip event. */
    void scheduleStopClip(int trackIndex, int32_t clipId, int64_t atSample)
    {
        bool success = engine_->scheduleClipStop(trackIndex, clipId, atSample);
        jassert(success);
    }

    /** Start transport. */
    void play()
    {
        engine_->play();
    }

    /** Seek to absolute sample. */
    void seek(int64_t sample)
    {
        engine_->seekSamples(sample);
    }

    /** Get current transport position. */
    int64_t transportSamples() const
    {
        return engine_->transportSamples();
    }

    /** Manually render one block and collect output. */
    void renderBlock(std::vector<float>& timeline)
    {
        testBuffer_.clear();

        // Call the audio callback directly
        juce::AudioSourceChannelInfo info;
        info.buffer = &testBuffer_;
        info.startSample = 0;
        info.numSamples = blockSize_;

        engine_->audioDeviceIOCallbackWithContext(
            nullptr, 0,
            testBuffer_.getArrayOfWritePointers(), testBuffer_.getNumChannels(),
            blockSize_, {}
        );

        // Collect left channel samples into timeline
        const float* leftChannel = testBuffer_.getReadPointer(0);
        for (int i = 0; i < blockSize_; ++i)
            timeline.push_back(leftChannel[i]);
    }

    /** Render N blocks and return full timeline. */
    std::vector<float> renderBlocks(int numBlocks)
    {
        std::vector<float> timeline;
        timeline.reserve(numBlocks * blockSize_);

        for (int i = 0; i < numBlocks; ++i)
            renderBlock(timeline);

        return timeline;
    }

    /** Manually populate ClipDef data for a track. */
    void populateClipDefsForTrack(int trackIndex)
    {
        auto* mixer = &engine_->getMixer();
        auto* track = mixer->getTrack(trackIndex);
        jassert(track != nullptr);

        // Use test-only API to populate ClipDefs
        for (const auto& [clipId, data] : clipData_)
        {
            if (trackForClip_[clipId] == trackIndex)
            {
                AudioTrack::ClipDef def;
                def.clipId = clipId;
                def.audioData = data.audioData;
                def.clipStartInTimeline = data.startSample;
                def.clipLengthSamples = data.lengthSamples;
                def.gain = data.gain;
                def.fadeInSamples = data.fadeInSamples;
                def.fadeOutSamples = data.fadeOutSamples;

                track->addClipDefForTest(def);
            }
        }
    }

private:
    int sampleRate_;
    int blockSize_;
    std::unique_ptr<Engine> engine_;
    juce::AudioBuffer<float> testBuffer_;

    // Clip data storage
    struct ClipData {
        juce::AudioBuffer<float>* audioData;
        int64_t startSample;
        int64_t lengthSamples;
        float gain;
        int fadeInSamples;
        int fadeOutSamples;
    };

    std::map<int32_t, ClipData> clipData_;
    std::map<int32_t, int> trackForClip_;
};

//==============================================================================
// Test 1: Scheduler_ClipStartsExactlyAtScheduledSample
//==============================================================================

void test1_SchedulerClipStartsExactly()
{
    DBG("\n=== Test 1: Scheduler_ClipStartsExactlyAtScheduledSample ===");

    const int sampleRate = 48000;
    const int blockSize = 256;
    const int64_t scheduledStart = 1000;  // Clip should start at sample 1000

    Phase1TestHarness harness(sampleRate, blockSize);

    // Create a simple test clip (1.0f everywhere, mono, 5000 samples)
    auto clipAudio = TestUtil::createTestClip(5000, 1.0f);

    // Add track
    int trackIndex = harness.addTrack("Test Track");

    // Add clip (no fades, gain = 1.0)
    const int32_t clipId = 42;
    harness.addClipDef(trackIndex, clipId, &clipAudio, 0, 5000, 1.0f, 0, 0);
    harness.populateClipDefsForTrack(trackIndex);

    // Schedule StartClip at sample 1000
    harness.scheduleStartClip(trackIndex, clipId, scheduledStart);

    // Start transport at 0
    harness.seek(0);
    harness.play();

    // Render enough blocks to cover sample 0-1300
    const int blocksNeeded = ((int)scheduledStart + 300 + blockSize - 1) / blockSize;
    auto timeline = harness.renderBlocks(blocksNeeded);

    DBG("Rendered " + juce::String(timeline.size()) + " samples");

    // Asserts:
    // 1. All samples before 1000 should be 0.0
    for (int i = 0; i < scheduledStart && i < (int)timeline.size(); ++i)
    {
        if (!TestUtil::approxEqual(timeline[i], 0.0f))
        {
            TestUtil::assertWithMessage(false,
                "Sample " + juce::String(i) + " should be 0.0, got " + juce::String(timeline[i]));
        }
    }

    // 2. Sample at 1000 should be non-zero (≈1.0)
    if (scheduledStart < (int)timeline.size())
    {
        float sampleAt1000 = timeline[scheduledStart];
        TestUtil::assertWithMessage(sampleAt1000 > 0.5f,
            "Sample at 1000 should be ≈1.0, got " + juce::String(sampleAt1000));
    }

    // 3. No pre-ring: sample 999 should be exactly 0.0
    if (scheduledStart > 0 && scheduledStart - 1 < (int)timeline.size())
    {
        float sampleAt999 = timeline[scheduledStart - 1];
        TestUtil::assertWithMessage(TestUtil::approxEqual(sampleAt999, 0.0f),
            "Sample 999 (pre-ring check) should be 0.0, got " + juce::String(sampleAt999));
    }

    DBG("✓ Test 1 PASSED: Clip starts exactly at scheduled sample");
}

//==============================================================================
// Test 2: Fades_FadeInAndFadeOutShapeIsLinear
//==============================================================================

void test2_FadeShapesAreLinear()
{
    DBG("\n=== Test 2: Fades_FadeInAndFadeOutShapeIsLinear ===");

    const int sampleRate = 48000;
    const int blockSize = 256;
    const int fadeInSamples = 100;
    const int fadeOutSamples = 100;
    const int clipLengthSamples = 5000;

    Phase1TestHarness harness(sampleRate, blockSize);

    // Create test clip (1.0f everywhere)
    auto clipAudio = TestUtil::createTestClip(clipLengthSamples, 1.0f);

    // Add track
    int trackIndex = harness.addTrack("Fade Test Track");

    // Add clip with fades
    const int32_t clipId = 100;
    harness.addClipDef(trackIndex, clipId, &clipAudio, 0, clipLengthSamples, 1.0f,
                      fadeInSamples, fadeOutSamples);
    harness.populateClipDefsForTrack(trackIndex);

    // Schedule StartClip at sample 0
    harness.scheduleStartClip(trackIndex, clipId, 0);

    // Start transport
    harness.seek(0);
    harness.play();

    // Render enough to cover full clip
    const int blocksNeeded = (clipLengthSamples + blockSize - 1) / blockSize;
    auto timeline = harness.renderBlocks(blocksNeeded);

    DBG("Rendered " + juce::String(timeline.size()) + " samples for fade test");

    // Check fade-in shape
    if (timeline.size() > 0)
    {
        // Sample 0: ~0.0
        TestUtil::assertWithMessage(timeline[0] < 0.1f,
            "Fade-in sample 0 should be ≈0.0, got " + juce::String(timeline[0]));

        // Sample 50: ~0.5
        if (timeline.size() > 50)
        {
            TestUtil::assertWithMessage(TestUtil::approxEqual(timeline[50], 0.5f, 0.1f),
                "Fade-in sample 50 should be ≈0.5, got " + juce::String(timeline[50]));
        }

        // Sample 100: ~1.0 (fade-in complete)
        if (timeline.size() > 100)
        {
            TestUtil::assertWithMessage(timeline[100] > 0.9f,
                "Fade-in sample 100 should be ≈1.0, got " + juce::String(timeline[100]));
        }

        // Middle of clip: ~1.0
        int midPoint = clipLengthSamples / 2;
        if (timeline.size() > midPoint)
        {
            TestUtil::assertWithMessage(TestUtil::approxEqual(timeline[midPoint], 1.0f, 0.05f),
                "Middle sample should be ≈1.0, got " + juce::String(timeline[midPoint]));
        }

        // Check fade-out shape
        int fadeOutStart = clipLengthSamples - fadeOutSamples;

        // Sample at fadeOutStart + 50: ~0.5
        if (timeline.size() > fadeOutStart + 50)
        {
            TestUtil::assertWithMessage(TestUtil::approxEqual(timeline[fadeOutStart + 50], 0.5f, 0.1f),
                "Fade-out sample " + juce::String(fadeOutStart + 50) + " should be ≈0.5, got " +
                juce::String(timeline[fadeOutStart + 50]));
        }

        // Last sample: ~0.0
        if (timeline.size() > clipLengthSamples - 1)
        {
            float lastSample = timeline[std::min(clipLengthSamples - 1, (int)timeline.size() - 1)];
            TestUtil::assertWithMessage(lastSample < 0.1f,
                "Fade-out last sample should be ≈0.0, got " + juce::String(lastSample));
        }
    }

    DBG("✓ Test 2 PASSED: Fade-in and fade-out shapes are linear");
}

//==============================================================================
// Test 3: StopEvents_FadeOutThenSilence
//==============================================================================

void test3_StopEventTriggersFadeOut()
{
    DBG("\n=== Test 3: StopEvents_FadeOutThenSilence ===");

    const int sampleRate = 48000;
    const int blockSize = 256;
    const int fadeOutSamples = 100;
    const int64_t stopAtSample = 2000;

    Phase1TestHarness harness(sampleRate, blockSize);

    // Create long clip (10000 samples, 1.0f everywhere, no natural fade-out)
    auto clipAudio = TestUtil::createTestClip(10000, 1.0f);

    // Add track
    int trackIndex = harness.addTrack("Stop Test Track");

    // Add clip with fade-out capability
    const int32_t clipId = 200;
    harness.addClipDef(trackIndex, clipId, &clipAudio, 0, 10000, 1.0f, 0, fadeOutSamples);
    harness.populateClipDefsForTrack(trackIndex);

    // Schedule StartClip at sample 0
    harness.scheduleStartClip(trackIndex, clipId, 0);

    // Schedule StopClip at sample 2000
    harness.scheduleStopClip(trackIndex, clipId, stopAtSample);

    // Start transport
    harness.seek(0);
    harness.play();

    // Render enough to cover stop event + fade-out + extra
    const int samplesNeeded = (int)stopAtSample + fadeOutSamples + 200;
    const int blocksNeeded = (samplesNeeded + blockSize - 1) / blockSize;
    auto timeline = harness.renderBlocks(blocksNeeded);

    DBG("Rendered " + juce::String(timeline.size()) + " samples for stop event test");

    // Asserts:
    // 1. Just before stopAtSample, level should be ≈1.0
    if (timeline.size() > stopAtSample - 10)
    {
        float beforeStop = timeline[stopAtSample - 10];
        TestUtil::assertWithMessage(beforeStop > 0.9f,
            "Sample before stop should be ≈1.0, got " + juce::String(beforeStop));
    }

    // 2. Over next 100 samples, gain drops to 0
    // Check midpoint of fade-out
    if (timeline.size() > stopAtSample + fadeOutSamples / 2)
    {
        float midFade = timeline[stopAtSample + fadeOutSamples / 2];
        TestUtil::assertWithMessage(midFade < 0.7f && midFade > 0.3f,
            "Mid-fade sample should be ≈0.5, got " + juce::String(midFade));
    }

    // 3. After fade-out completes, all samples should be 0.0
    int silenceStart = (int)stopAtSample + fadeOutSamples + 10;
    for (int i = silenceStart; i < (int)timeline.size() && i < silenceStart + 50; ++i)
    {
        TestUtil::assertWithMessage(TestUtil::approxEqual(timeline[i], 0.0f),
            "Sample " + juce::String(i) + " after fade-out should be 0.0, got " +
            juce::String(timeline[i]));
    }

    DBG("✓ Test 3 PASSED: StopClip triggers fade-out then silence");
}

//==============================================================================
// Main Entry Point
//==============================================================================

int main(int argc, char* argv[])
{
    DBG("========================================");
    DBG("Phase 1 Audio Tests (W10+W10.2+W10.3)");
    DBG("Headless regression tests for scheduler + voices + fades");
    DBG("========================================");

    juce::ScopedJuceInitialiser_GUI juceInit;

    try
    {
        test1_SchedulerClipStartsExactly();
        test2_FadeShapesAreLinear();
        test3_StopEventTriggersFadeOut();

        DBG("\n========================================");
        DBG("ALL TESTS PASSED ✓");
        DBG("========================================");

        return 0;
    }
    catch (const std::exception& e)
    {
        DBG("\n========================================");
        DBG("TEST FAILED ✗");
        DBG("Error: " + juce::String(e.what()));
        DBG("========================================");

        return 1;
    }
}
