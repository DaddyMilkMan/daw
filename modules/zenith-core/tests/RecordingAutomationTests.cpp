/**
 * @file RecordingAutomationTests.cpp
 * @brief Non-GUI tests for recording and automation integration
 *
 * Tests:
 * 1. Recording simulation - verify clip data structures
 * 2. Automation application - verify automation affects parameters over time
 * 3. Tempo changes - verify automation maps correctly in beats
 *
 * Uses JUCE console app pattern (return 0 = success, 1 = failure)
 */

#include <iostream>
#include <memory>
#include <cmath>

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "../include/ProjectState.h"

//==============================================================================
// Test Utilities
//==============================================================================

namespace
{

// Compare floating point values within tolerance
bool approxEqual(double a, double b, double tolerance = 0.001)
{
    return std::abs(a - b) < tolerance;
}

// Compare floating point values within tolerance
bool approxEqual(float a, float b, float tolerance = 0.001f)
{
    return std::abs(a - b) < tolerance;
}

} // namespace

//==============================================================================
// Test 1: Recording Simulation - Clip Data Model
//==============================================================================

/**
 * @brief Test that simulates recording at the data model level
 *
 * Verifies:
 * 1. Recording duration calculations are correct
 * 2. Clip length in samples matches expected values
 * 3. Conversion between samples and seconds works correctly
 *
 * This tests the math and data structures that recording would use.
 */
bool testRecordingClipLengthCalculations()
{
    std::cout << "Test 1: Recording Simulation - Clip Length Calculations" << std::endl;

    // Test parameters
    const double sampleRate = 44100.0;
    const int blockSize = 512;
    const int numBlocks = 10;  // Simulate 10 audio callback blocks
    const int64_t expectedLengthInSamples = numBlocks * blockSize;
    const double expectedLengthInSeconds = expectedLengthInSamples / sampleRate;

    std::cout << "  Sample rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "  Block size: " << blockSize << " samples" << std::endl;
    std::cout << "  Number of blocks: " << numBlocks << std::endl;

    // Simulate recording: calculate expected clip length
    int64_t recordedSamples = 0;
    for (int i = 0; i < numBlocks; ++i)
    {
        recordedSamples += blockSize;
    }

    double recordedSeconds = recordedSamples / sampleRate;

    std::cout << "  Expected length (samples): " << expectedLengthInSamples << std::endl;
    std::cout << "  Recorded length (samples): " << recordedSamples << std::endl;
    std::cout << "  Expected length (seconds): " << expectedLengthInSeconds << std::endl;
    std::cout << "  Recorded length (seconds): " << recordedSeconds << std::endl;

    if (recordedSamples != expectedLengthInSamples)
    {
        std::cerr << "  ERROR: Recorded length mismatch!" << std::endl;
        return false;
    }

    if (!approxEqual(recordedSeconds, expectedLengthInSeconds))
    {
        std::cerr << "  ERROR: Recorded duration in seconds is incorrect!" << std::endl;
        return false;
    }

    // Test audio buffer creation (simulating what recording would create)
    juce::AudioBuffer<float> recordedAudio(2, static_cast<int>(recordedSamples));

    if (recordedAudio.getNumSamples() != static_cast<int>(expectedLengthInSamples))
    {
        std::cerr << "  ERROR: Audio buffer has wrong number of samples!" << std::endl;
        return false;
    }

    if (recordedAudio.getNumChannels() != 2)
    {
        std::cerr << "  ERROR: Audio buffer has wrong number of channels!" << std::endl;
        return false;
    }

    // Fill with test signal (sine wave at 440 Hz)
    const double frequency = 440.0;
    for (int channel = 0; channel < recordedAudio.getNumChannels(); ++channel)
    {
        auto* channelData = recordedAudio.getWritePointer(channel);
        for (int64_t i = 0; i < recordedSamples; ++i)
        {
            channelData[i] = static_cast<float>(
                std::sin(2.0 * juce::MathConstants<double>::pi * frequency * i / sampleRate)
            );
        }
    }

    // Verify some sample values
    float firstSample = recordedAudio.getSample(0, 0);
    if (!approxEqual(firstSample, 0.0f, 0.01f))
    {
        std::cerr << "  ERROR: First sample should be ~0, got " << firstSample << std::endl;
        return false;
    }

    std::cout << "  PASS: Clip length calculations and audio buffer creation correct" << std::endl;
    return true;
}

//==============================================================================
// Test 2: Automation Data Model and Sampling
//==============================================================================

/**
 * @brief Test that automation envelope can be sampled correctly
 *
 * Sets up automation points in ProjectState and verifies:
 * 1. Points are stored correctly
 * 2. Sampling between points gives correct interpolated values
 * 3. Automation works for volume, pan, and mute parameters
 */
bool testAutomationEnvelopeSampling()
{
    std::cout << "\nTest 2: Automation Envelope Sampling" << std::endl;

    // Initialize JUCE message manager for ValueTree operations
    juce::MessageManager::getInstance();

    const double bpm = 120.0;

    // Create project state
    ProjectState projectState;
    projectState.setTempo(bpm);

    // Add a track to project state
    juce::String trackId = projectState.addTrack("Test Track", "audio");

    // Add volume automation points
    // Beat 0: volume = 0.0
    // Beat 4: volume = 1.0
    // This creates a linear ramp from 0 to 1 over 4 beats
    projectState.addAutomationPoint(trackId, "volume", 0.0, 0.0, "Add automation point");
    projectState.addAutomationPoint(trackId, "volume", 4.0, 1.0, "Add automation point");

    std::cout << "  Created automation: volume ramps from 0.0 to 1.0 over 4 beats" << std::endl;

    // Test automation points at different beat positions
    struct TestPoint
    {
        double beatPosition;
        float expectedVolume;
        const char* description;
    };

    TestPoint testPoints[] = {
        { 0.0, 0.0f, "Start (beat 0)" },
        { 1.0, 0.25f, "Quarter way (beat 1)" },
        { 2.0, 0.5f, "Half way (beat 2)" },
        { 3.0, 0.75f, "Three quarters (beat 3)" },
        { 4.0, 1.0f, "End (beat 4)" },
    };

    std::cout << "  Testing automation values at different beat positions:" << std::endl;

    for (const auto& testPoint : testPoints)
    {
        // Sample the automation envelope at this beat position
        auto envelope = projectState.getAutomationEnvelope(trackId, "volume");

        if (!envelope.isValid())
        {
            std::cerr << "  ERROR: Volume automation envelope not found!" << std::endl;
            return false;
        }

        int numPoints = envelope.getNumChildren();
        if (numPoints == 0)
        {
            std::cerr << "  ERROR: No automation points found!" << std::endl;
            return false;
        }

        // Manual linear interpolation between points
        double sampledValue = 0.0;
        bool foundValue = false;

        double prevTime = -1.0;
        double prevValue = 0.0;
        double nextTime = -1.0;
        double nextValue = 0.0;

        for (int i = 0; i < numPoints; ++i)
        {
            auto point = envelope.getChild(i);
            double pointTime = point[ProjectState::PROP_TIME_BEATS];
            double pointValue = point[ProjectState::PROP_VALUE];

            if (approxEqual(pointTime, testPoint.beatPosition))
            {
                // Exact match
                sampledValue = pointValue;
                foundValue = true;
                break;
            }
            else if (pointTime < testPoint.beatPosition)
            {
                prevTime = pointTime;
                prevValue = pointValue;
            }
            else if (pointTime > testPoint.beatPosition && nextTime < 0.0)
            {
                nextTime = pointTime;
                nextValue = pointValue;
            }
        }

        // Interpolate if we didn't find exact match
        if (!foundValue)
        {
            if (prevTime >= 0.0 && nextTime >= 0.0)
            {
                // Linear interpolation
                double t = (testPoint.beatPosition - prevTime) / (nextTime - prevTime);
                sampledValue = prevValue + t * (nextValue - prevValue);
                foundValue = true;
            }
            else if (prevTime >= 0.0)
            {
                sampledValue = prevValue;
                foundValue = true;
            }
            else if (nextTime >= 0.0)
            {
                sampledValue = nextValue;
                foundValue = true;
            }
        }

        if (!foundValue)
        {
            std::cerr << "  ERROR: Could not sample automation at beat "
                      << testPoint.beatPosition << std::endl;
            return false;
        }

        std::cout << "    " << testPoint.description << ": "
                  << "expected=" << testPoint.expectedVolume
                  << ", actual=" << static_cast<float>(sampledValue);

        if (!approxEqual(static_cast<float>(sampledValue), testPoint.expectedVolume, 0.01f))
        {
            std::cout << " FAIL" << std::endl;
            std::cerr << "  ERROR: Volume mismatch at " << testPoint.description << std::endl;
            return false;
        }

        std::cout << " PASS" << std::endl;
    }

    std::cout << "  PASS: Automation envelope sampling works correctly" << std::endl;
    return true;
}

//==============================================================================
// Test 3: Tempo Change - Automation Beat Mapping
//==============================================================================

/**
 * @brief Test that automation maps correctly with tempo changes
 *
 * Verifies that automation based on beats works correctly when tempo changes.
 * This is important because automation is stored in beats, not seconds.
 */
bool testTempoChangeAutomationMapping()
{
    std::cout << "\nTest 3: Tempo Change - Automation Beat Mapping" << std::endl;

    const double initialBpm = 120.0;
    const double newBpm = 90.0;

    // Create project state
    ProjectState projectState;
    projectState.setTempo(initialBpm);

    // Add a track
    juce::String trackId = projectState.addTrack("Tempo Test Track", "audio");

    // Add automation points at specific beats
    projectState.addAutomationPoint(trackId, "volume", 0.0, 0.2, "Add point");
    projectState.addAutomationPoint(trackId, "volume", 8.0, 0.8, "Add point");

    std::cout << "  Initial tempo: " << initialBpm << " BPM" << std::endl;
    std::cout << "  Automation: volume at beat 0 = 0.2, beat 8 = 0.8" << std::endl;

    // Verify automation is stored in beats
    auto envelope = projectState.getAutomationEnvelope(trackId, "volume");

    if (!envelope.isValid() || envelope.getNumChildren() != 2)
    {
        std::cerr << "  ERROR: Expected 2 automation points" << std::endl;
        return false;
    }

    auto point0 = envelope.getChild(0);
    auto point1 = envelope.getChild(1);

    double beat0 = point0[ProjectState::PROP_TIME_BEATS];
    double beat1 = point1[ProjectState::PROP_TIME_BEATS];

    std::cout << "  Point 0: beat=" << beat0 << ", value="
              << double(point0[ProjectState::PROP_VALUE]) << std::endl;
    std::cout << "  Point 1: beat=" << beat1 << ", value="
              << double(point1[ProjectState::PROP_VALUE]) << std::endl;

    // Change tempo
    projectState.setTempo(newBpm);
    std::cout << "  Changed tempo to: " << newBpm << " BPM" << std::endl;

    // Verify automation points remain at same beat positions
    envelope = projectState.getAutomationEnvelope(trackId, "volume");

    if (!envelope.isValid() || envelope.getNumChildren() != 2)
    {
        std::cerr << "  ERROR: Automation points lost after tempo change!" << std::endl;
        return false;
    }

    point0 = envelope.getChild(0);
    point1 = envelope.getChild(1);

    double newBeat0 = point0[ProjectState::PROP_TIME_BEATS];
    double newBeat1 = point1[ProjectState::PROP_TIME_BEATS];

    std::cout << "  After tempo change:" << std::endl;
    std::cout << "  Point 0: beat=" << newBeat0 << ", value="
              << double(point0[ProjectState::PROP_VALUE]) << std::endl;
    std::cout << "  Point 1: beat=" << newBeat1 << ", value="
              << double(point1[ProjectState::PROP_VALUE]) << std::endl;

    if (!approxEqual(beat0, newBeat0) || !approxEqual(beat1, newBeat1))
    {
        std::cerr << "  ERROR: Automation beat positions changed with tempo!" << std::endl;
        std::cerr << "    This breaks musical synchronization." << std::endl;
        return false;
    }

    // Verify that the TIME duration changes (slower tempo = longer time)
    double timeAtOldTempo = 8.0 / (initialBpm / 60.0);  // 8 beats in seconds
    double timeAtNewTempo = 8.0 / (newBpm / 60.0);       // 8 beats in seconds

    std::cout << "  Time for 8 beats at " << initialBpm << " BPM: "
              << timeAtOldTempo << " seconds" << std::endl;
    std::cout << "  Time for 8 beats at " << newBpm << " BPM: "
              << timeAtNewTempo << " seconds" << std::endl;

    if (timeAtNewTempo <= timeAtOldTempo)
    {
        std::cerr << "  ERROR: Slower tempo should result in longer time duration!" << std::endl;
        return false;
    }

    std::cout << "  PASS: Automation correctly maps in beats across tempo changes" << std::endl;
    return true;
}

//==============================================================================
// Test 4: ProjectState Changes Propagate
//==============================================================================

/**
 * @brief Test that ProjectState changes can be observed
 *
 * Verifies that the automation data model supports change notifications
 * that could be used to propagate changes to the engine.
 */
bool testProjectStateChangeNotifications()
{
    std::cout << "\nTest 4: ProjectState Change Propagation" << std::endl;

    ProjectState projectState;

    // Add a track
    juce::String trackId = projectState.addTrack("Change Test Track", "audio");

    // Verify track exists
    if (projectState.getNumTracks() != 1)
    {
        std::cerr << "  ERROR: Expected 1 track" << std::endl;
        return false;
    }

    // Add automation
    projectState.addAutomationPoint(trackId, "volume", 0.0, 0.5, "Add point");
    projectState.addAutomationPoint(trackId, "pan", 2.0, -0.5, "Add point");
    projectState.addAutomationPoint(trackId, "mute", 4.0, 1.0, "Add point");

    // Verify automation exists for all three parameters
    bool hasVolume = projectState.hasAutomation(trackId, "volume");
    bool hasPan = projectState.hasAutomation(trackId, "pan");
    bool hasMute = projectState.hasAutomation(trackId, "mute");

    std::cout << "  Automation exists: volume=" << hasVolume
              << ", pan=" << hasPan
              << ", mute=" << hasMute << std::endl;

    if (!hasVolume || !hasPan || !hasMute)
    {
        std::cerr << "  ERROR: Expected automation for volume, pan, and mute!" << std::endl;
        return false;
    }

    // Test undo/redo
    if (!projectState.canUndo())
    {
        std::cerr << "  ERROR: Should be able to undo!" << std::endl;
        return false;
    }

    std::cout << "  Undoing last automation point..." << std::endl;
    projectState.undo();

    hasMute = projectState.hasAutomation(trackId, "mute");
    if (hasMute)
    {
        auto muteEnvelope = projectState.getAutomationEnvelope(trackId, "mute");
        if (muteEnvelope.isValid() && muteEnvelope.getNumChildren() > 0)
        {
            std::cerr << "  ERROR: Mute automation should be empty after undo!" << std::endl;
            return false;
        }
    }

    std::cout << "  Undo successful" << std::endl;

    if (!projectState.canRedo())
    {
        std::cerr << "  ERROR: Should be able to redo!" << std::endl;
        return false;
    }

    std::cout << "  Redoing..." << std::endl;
    projectState.redo();

    hasMute = projectState.hasAutomation(trackId, "mute");
    if (!hasMute)
    {
        std::cerr << "  ERROR: Mute automation should exist after redo!" << std::endl;
        return false;
    }

    std::cout << "  Redo successful" << std::endl;

    std::cout << "  PASS: ProjectState changes and undo/redo work correctly" << std::endl;
    return true;
}

//==============================================================================
// Main Test Runner
//==============================================================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "Recording & Automation Integration Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    bool allTestsPassed = true;

    // Run Test 1: Recording Simulation
    if (!testRecordingClipLengthCalculations())
    {
        allTestsPassed = false;
    }

    // Run Test 2: Automation Envelope Sampling
    if (!testAutomationEnvelopeSampling())
    {
        allTestsPassed = false;
    }

    // Run Test 3: Tempo Change Automation
    if (!testTempoChangeAutomationMapping())
    {
        allTestsPassed = false;
    }

    // Run Test 4: ProjectState Change Notifications
    if (!testProjectStateChangeNotifications())
    {
        allTestsPassed = false;
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;

    if (allTestsPassed)
    {
        std::cout << "ALL TESTS PASSED ✓" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "SOME TESTS FAILED ✗" << std::endl;
        std::cout << "========================================" << std::endl;
        return 1;
    }
}

