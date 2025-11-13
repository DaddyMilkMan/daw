/**
 * @file ClipTrackTests.cpp
 * @brief Offline unit tests for W13 clip rendering and sample-accurate scheduling
 *
 * W13.1 Test Coverage:
 * - Sample-accurate clip placement (aligned and offset start)
 * - Fade-in and fade-out rendering (linear ramps)
 * - Bounds validation (no OOB reads, correct overlap math)
 *
 * Design:
 * - Headless tests (no audio device, no UI)
 * - Pre-allocated buffers and synthetic test data
 * - Deterministic validation (exact float comparisons with epsilon)
 * - Only compiled in DEBUG builds
 */

#if JUCE_DEBUG && ZENITH_ENABLE_PHASE1_AUDIO

#include <JuceHeader.h>
#include "../engine/Track.h"
#include "../engine/Clip.h"
#include "../engine/nodes/GainPanNode.h"

using namespace zenith;

//==============================================================================
/**
 * @class ClipTrackTests
 * @brief JUCE UnitTest for Track + Clip rendering validation
 */
class ClipTrackTests : public juce::UnitTest
{
public:
    ClipTrackTests()
        : juce::UnitTest("W13: Clip + Track Rendering", "Audio Engine")
    {
    }

    void runTest() override
    {
        // Test 1: Single clip, aligned to block start
        beginTest("Test 1: Single clip aligned to block start");
        testAlignedClipRendering();

        // Test 2: Single clip, offset start (mid-block)
        beginTest("Test 2: Single clip with offset start");
        testOffsetClipRendering();

        // Test 3: Fade-in and fade-out sanity
        beginTest("Test 3: Fade-in and fade-out rendering");
        testFadeRendering();

        // Test 4: W11.0 FX chain (GainPanNode with bypass)
        beginTest("Test 4: FX chain applies gain and bypass");
        testFxChainGainAndBypass();
    }

private:
    //==========================================================================
    // Test Case 1: Aligned Clip (startSample = 0)
    //==========================================================================

    void testAlignedClipRendering()
    {
        const int clipLength = 100;
        const int blockSize = 64;
        const double sampleRate = 48000.0;

        // Create synthetic PCM: ramp from 0.0 to 1.0 over 100 samples
        auto pcmBuffer = std::make_shared<juce::AudioBuffer<float>>(1, clipLength);
        for (int n = 0; n < clipLength; ++n)
            pcmBuffer->setSample(0, n, static_cast<float>(n) / 100.0f);

        // Create clip at timeline position 0
        Clip clip;
        clip.pcm = pcmBuffer;
        clip.startSample = 0;
        clip.lengthSamples = clipLength;
        clip.srcOffset = 0;
        clip.gain = 1.0f;
        clip.fadeInSamples = 0;   // No fades for this test
        clip.fadeOutSamples = 0;

        expectTrue(clip.isValid(), "Clip should be valid");

        // Create track and add clip
        Track track;
        track.prepareToPlay(blockSize, sampleRate);
        track.addClip(clip);

        expectEquals(track.getNumClips(), 1, "Track should have 1 clip");

        // Render first block [0, 64)
        juce::AudioBuffer<float> mixBuffer(2, blockSize);  // Stereo output
        mixBuffer.clear();

        track.processBlock(mixBuffer, blockSize, 0);

        // Validate output: first 64 samples should match ramp [0..63]
        const float* outL = mixBuffer.getReadPointer(0);
        const float* outR = mixBuffer.getReadPointer(1);

        for (int n = 0; n < blockSize; ++n)
        {
            const float expected = static_cast<float>(n) / 100.0f;
            expectWithinAbsoluteError(outL[n], expected, 1e-6f, "Left channel sample " + juce::String(n));
            expectWithinAbsoluteError(outR[n], expected, 1e-6f, "Right channel sample " + juce::String(n));
        }

        DBG("Test 1 PASS: Aligned clip rendered correctly");
    }

    //==========================================================================
    // Test Case 2: Offset Clip (startSample = 32)
    //==========================================================================

    void testOffsetClipRendering()
    {
        const int clipLength = 100;
        const int blockSize = 64;
        const double sampleRate = 48000.0;

        // Create synthetic PCM: ramp from 0.0 to 1.0 over 100 samples
        auto pcmBuffer = std::make_shared<juce::AudioBuffer<float>>(1, clipLength);
        for (int n = 0; n < clipLength; ++n)
            pcmBuffer->setSample(0, n, static_cast<float>(n) / 100.0f);

        // Create clip starting at timeline position 32
        Clip clip;
        clip.pcm = pcmBuffer;
        clip.startSample = 32;
        clip.lengthSamples = clipLength;
        clip.srcOffset = 0;
        clip.gain = 1.0f;
        clip.fadeInSamples = 0;
        clip.fadeOutSamples = 0;

        expectTrue(clip.isValid(), "Clip should be valid");

        // Create track and add clip
        Track track;
        track.prepareToPlay(blockSize, sampleRate);
        track.addClip(clip);

        // Render block [0, 64) - clip starts at sample 32
        juce::AudioBuffer<float> mixBuffer(2, blockSize);
        mixBuffer.clear();

        track.processBlock(mixBuffer, blockSize, 0);

        const float* outL = mixBuffer.getReadPointer(0);

        // Validate: samples [0..31] should be silent
        for (int n = 0; n < 32; ++n)
        {
            expectWithinAbsoluteError(outL[n], 0.0f, 1e-6f,
                                     "Sample " + juce::String(n) + " should be silent");
        }

        // Validate: samples [32..63] should match ramp[0..31]
        for (int n = 32; n < blockSize; ++n)
        {
            const float expected = static_cast<float>(n - 32) / 100.0f;
            expectWithinAbsoluteError(outL[n], expected, 1e-6f,
                                     "Sample " + juce::String(n) + " should match ramp");
        }

        DBG("Test 2 PASS: Offset clip rendered correctly");
    }

    //==========================================================================
    // Test Case 3: Fade-In and Fade-Out
    //==========================================================================

    void testFadeRendering()
    {
        const int clipLength = 100;
        const int blockSize = 128;  // Larger block to cover entire clip
        const double sampleRate = 48000.0;
        const int fadeInSamples = 16;
        const int fadeOutSamples = 16;

        // Create synthetic PCM: constant 1.0 (so we can see the fade shape clearly)
        auto pcmBuffer = std::make_shared<juce::AudioBuffer<float>>(1, clipLength);
        pcmBuffer->clear();
        juce::FloatVectorOperations::fill(pcmBuffer->getWritePointer(0), 1.0f, clipLength);

        // Create clip with fades
        Clip clip;
        clip.pcm = pcmBuffer;
        clip.startSample = 0;
        clip.lengthSamples = clipLength;
        clip.srcOffset = 0;
        clip.gain = 1.0f;
        clip.fadeInSamples = fadeInSamples;
        clip.fadeOutSamples = fadeOutSamples;

        expectTrue(clip.isValid(), "Clip with fades should be valid");

        // Create track and add clip
        Track track;
        track.prepareToPlay(blockSize, sampleRate);
        track.addClip(clip);

        // Render entire clip [0, 100)
        juce::AudioBuffer<float> mixBuffer(2, blockSize);
        mixBuffer.clear();

        track.processBlock(mixBuffer, blockSize, 0);

        const float* outL = mixBuffer.getReadPointer(0);

        // Validate fade-in zone [0..15]: should ramp from ~0.0 to ~1.0
        expectWithinAbsoluteError(outL[0], 0.0f, 0.1f, "Fade-in start should be near 0.0");
        expectWithinAbsoluteError(outL[fadeInSamples - 1], 1.0f, 0.1f,
                                 "Fade-in end should be near 1.0");

        // Validate steady zone [16..83]: should be ~1.0
        const int steadyStart = fadeInSamples;
        const int steadyEnd = clipLength - fadeOutSamples;
        for (int n = steadyStart; n < steadyEnd; ++n)
        {
            expectWithinAbsoluteError(outL[n], 1.0f, 0.01f,
                                     "Steady zone sample " + juce::String(n) + " should be 1.0");
        }

        // Validate fade-out zone [84..99]: should ramp from ~1.0 to ~0.0
        expectWithinAbsoluteError(outL[clipLength - fadeOutSamples], 1.0f, 0.1f,
                                 "Fade-out start should be near 1.0");
        expectWithinAbsoluteError(outL[clipLength - 1], 0.0f, 0.1f,
                                 "Fade-out end should be near 0.0");

        // Validate: no sample exceeds baseGain (no spikes)
        for (int n = 0; n < clipLength; ++n)
        {
            expect(outL[n] >= -0.01f && outL[n] <= 1.01f,
                  "Sample " + juce::String(n) + " out of range: " + juce::String(outL[n]));
        }

        DBG("Test 3 PASS: Fades rendered correctly");
    }

    //==========================================================================
    // Test Case 4: W11.0 FX Chain (GainPanNode + Bypass)
    //==========================================================================

    void testFxChainGainAndBypass()
    {
        const int clipLength = 100;
        const int blockSize = 64;
        const double sampleRate = 48000.0;

        // Create synthetic PCM: constant 0.5 (so gain effects are clear)
        auto pcmBuffer = std::make_shared<juce::AudioBuffer<float>>(1, clipLength);
        pcmBuffer->clear();
        juce::FloatVectorOperations::fill(pcmBuffer->getWritePointer(0), 0.5f, clipLength);

        // Create clip at timeline position 0
        Clip clip;
        clip.pcm = pcmBuffer;
        clip.startSample = 0;
        clip.lengthSamples = clipLength;
        clip.srcOffset = 0;
        clip.gain = 1.0f;
        clip.fadeInSamples = 0;   // No fades for this test
        clip.fadeOutSamples = 0;

        expectTrue(clip.isValid(), "Clip should be valid");

        // Create track and add clip
        Track track;
        track.prepareToPlay(blockSize, sampleRate);
        track.addClip(clip);

        juce::AudioBuffer<float> mixBuffer(2, blockSize);

        // =====================================================
        // Part A: Render WITHOUT FX (baseline)
        // =====================================================

        mixBuffer.clear();
        track.processBlock(mixBuffer, blockSize, 0);

        const float* outL = mixBuffer.getReadPointer(0);
        float peakWithoutFx = juce::FloatVectorOperations::findMaximum(outL, blockSize);

        expectWithinAbsoluteError(peakWithoutFx, 0.5f, 0.01f,
                                 "Peak without FX should be ~0.5");

        DBG("Test 4a PASS: Without FX, peak = " + juce::String(peakWithoutFx, 3));

        // =====================================================
        // Part B: Add GainPanNode with gain=2.0 in slot 0
        // =====================================================

        auto gainNode = std::make_unique<GainPanNode>();
        gainNode->setGain(2.0f);
        gainNode->setPan(0.0f);  // Center
        track.setFxNode(0, std::move(gainNode));

        mixBuffer.clear();
        track.processBlock(mixBuffer, blockSize, 0);

        outL = mixBuffer.getReadPointer(0);
        float peakWithGain = juce::FloatVectorOperations::findMaximum(outL, blockSize);

        expectWithinAbsoluteError(peakWithGain, 1.0f, 0.01f,
                                 "Peak with gain=2.0 should be ~1.0 (0.5 * 2.0)");

        DBG("Test 4b PASS: With gain=2.0, peak = " + juce::String(peakWithGain, 3));

        // =====================================================
        // Part C: Bypass FX (should return to ~0.5)
        // =====================================================

        track.setFxBypassed(0, true);

        mixBuffer.clear();
        track.processBlock(mixBuffer, blockSize, 0);

        outL = mixBuffer.getReadPointer(0);
        float peakBypassed = juce::FloatVectorOperations::findMaximum(outL, blockSize);

        expectWithinAbsoluteError(peakBypassed, 0.5f, 0.01f,
                                 "Peak with FX bypassed should be ~0.5");

        DBG("Test 4c PASS: With FX bypassed, peak = " + juce::String(peakBypassed, 3));

        // =====================================================
        // Part D: Re-enable FX (should return to ~1.0)
        // =====================================================

        track.setFxBypassed(0, false);

        mixBuffer.clear();
        track.processBlock(mixBuffer, blockSize, 0);

        outL = mixBuffer.getReadPointer(0);
        float peakReEnabled = juce::FloatVectorOperations::findMaximum(outL, blockSize);

        expectWithinAbsoluteError(peakReEnabled, 1.0f, 0.01f,
                                 "Peak with FX re-enabled should be ~1.0");

        DBG("Test 4d PASS: With FX re-enabled, peak = " + juce::String(peakReEnabled, 3));

        DBG("Test 4 PASS: FX chain applies gain and bypass correctly");
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipTrackTests)
};

// Register test with JUCE test runner
static ClipTrackTests clipTrackTests;

#endif  // JUCE_DEBUG && ZENITH_ENABLE_PHASE1_AUDIO
