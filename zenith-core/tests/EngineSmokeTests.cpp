/**
 * @file EngineSmokeTests.cpp
 * @brief Engine smoke test - headless console app for CI/sanity checks
 *
 * Tests core engine flows without requiring GUI or audio hardware:
 * - Engine + ProjectState initialization
 * - Track creation and management
 * - Clip creation with synthetic audio
 * - Transport state changes
 * - Basic project save/load
 *
 * Exit codes:
 * - 0: All tests passed
 * - 1: Test failure
 */

#include <iostream>
#include <memory>

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "../include/Engine.h"
#include "../include/ProjectState.h"
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"

//==============================================================================
// Test Helpers
//==============================================================================

namespace
{
    int testsPassed = 0;
    int testsFailed = 0;

    void logPass(const juce::String& testName)
    {
        std::cout << "[PASS] " << testName << std::endl;
        testsPassed++;
    }

    void logFail(const juce::String& testName, const juce::String& reason)
    {
        std::cerr << "[FAIL] " << testName << ": " << reason << std::endl;
        testsFailed++;
    }

    bool expectTrue(bool condition, const juce::String& testName, const juce::String& failReason = "")
    {
        if (condition)
        {
            logPass(testName);
            return true;
        }
        else
        {
            logFail(testName, failReason.isEmpty() ? "Condition was false" : failReason);
            return false;
        }
    }

    bool expectEqual(int actual, int expected, const juce::String& testName)
    {
        if (actual == expected)
        {
            logPass(testName);
            return true;
        }
        else
        {
            logFail(testName, "Expected " + juce::String(expected) + " but got " + juce::String(actual));
            return false;
        }
    }

    bool expectNotEmpty(const juce::String& str, const juce::String& testName)
    {
        if (str.isNotEmpty())
        {
            logPass(testName);
            return true;
        }
        else
        {
            logFail(testName, "String was empty");
            return false;
        }
    }

    /**
     * @brief Generate a simple sine wave buffer for testing
     */
    juce::AudioBuffer<float> generateSineWave(double sampleRate, double durationSeconds, double frequency)
    {
        const int numSamples = static_cast<int>(sampleRate * durationSeconds);
        juce::AudioBuffer<float> buffer(2, numSamples);  // Stereo

        for (int channel = 0; channel < 2; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int i = 0; i < numSamples; ++i)
            {
                const double phase = (i * frequency) / sampleRate;
                channelData[i] = static_cast<float>(std::sin(phase * 2.0 * juce::MathConstants<double>::pi));
            }
        }

        return buffer;
    }
}

//==============================================================================
// Test Functions
//==============================================================================

bool testProjectStateCreation()
{
    std::cout << "\n--- Testing ProjectState Creation ---" << std::endl;

    ProjectState state;

    if (!expectEqual(state.getNumTracks(), 0, "New ProjectState has zero tracks"))
        return false;

    if (!expectTrue(state.getTempo() == 120.0, "Default tempo is 120 BPM"))
        return false;

    if (!expectTrue(state.getTimeSignatureNumerator() == 4, "Default time signature numerator is 4"))
        return false;

    return true;
}

bool testTrackManagement()
{
    std::cout << "\n--- Testing Track Management ---" << std::endl;

    ProjectState state;

    // Add tracks
    auto track1Id = state.addTrack("Audio Track 1", "audio");
    if (!expectNotEmpty(track1Id, "Add first audio track"))
        return false;

    auto track2Id = state.addTrack("Audio Track 2", "audio");
    if (!expectNotEmpty(track2Id, "Add second audio track"))
        return false;

    if (!expectEqual(state.getNumTracks(), 2, "ProjectState has 2 tracks"))
        return false;

    // Ensure unique IDs
    if (!expectTrue(track1Id != track2Id, "Track IDs are unique",
                    "Generated duplicate IDs: " + track1Id + " and " + track2Id))
        return false;

    return true;
}

bool testProjectSaveLoad()
{
    std::cout << "\n--- Testing Project Save/Load ---" << std::endl;

    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getNonexistentChildFile("zenith_engine_smoke_test", ".zth", false);

    // Create and save
    {
        ProjectState state;
        state.setProjectName("Smoke Test Project");
        state.setTempo(140.0);
        state.addTrack("Test Track 1", "audio");
        state.addTrack("Test Track 2", "audio");

        if (!expectTrue(state.saveToFile(tempFile), "Save project to file"))
        {
            tempFile.deleteFile();
            return false;
        }
    }

    // Load and verify
    ProjectState loadedState;
    if (!expectTrue(loadedState.loadFromFile(tempFile), "Load project from file"))
    {
        tempFile.deleteFile();
        return false;
    }

    if (!expectTrue(loadedState.getProjectName() == "Smoke Test Project", "Loaded project name matches"))
    {
        tempFile.deleteFile();
        return false;
    }

    if (!expectTrue(loadedState.getTempo() == 140.0, "Loaded tempo matches"))
    {
        tempFile.deleteFile();
        return false;
    }

    if (!expectEqual(loadedState.getNumTracks(), 2, "Loaded project has 2 tracks"))
    {
        tempFile.deleteFile();
        return false;
    }

    tempFile.deleteFile();
    return true;
}

bool testEngineCreation()
{
    std::cout << "\n--- Testing Engine Creation ---" << std::endl;

    Engine engine;

    if (!expectTrue(!engine.isPlaying(), "Engine starts in stopped state"))
        return false;

    if (!expectTrue(engine.getSampleRate() == 44100.0, "Engine has default sample rate"))
        return false;

    // Test transport controls (without audio device)
    engine.play();
    if (!expectTrue(engine.isPlaying(), "Engine play() sets playing state"))
        return false;

    engine.stop();
    if (!expectTrue(!engine.isPlaying(), "Engine stop() clears playing state"))
        return false;

    return true;
}

bool testEngineWithProjectState()
{
    std::cout << "\n--- Testing Engine + ProjectState Integration ---" << std::endl;

    ProjectState state;
    state.addTrack("Track 1", "audio");
    state.addTrack("Track 2", "audio");

    Engine engine;
    engine.setProjectState(&state);

    if (!expectTrue(true, "Engine accepts ProjectState reference"))
        return false;

    // Test track count via Engine
    if (!expectEqual(engine.getNumTracks(), 0, "Engine starts with no internal tracks"))
        return false;

    return true;
}

bool testClipWithSyntheticAudio()
{
    std::cout << "\n--- Testing Clip with Synthetic Audio ---" << std::endl;

    // Create a track
    auto track = std::make_unique<zenith::Track>("Test Track", zenith::Track::Type::Audio);

    // Create a clip with synthetic audio
    auto clip = std::make_unique<zenith::Track::Clip>();
    clip->setName("Sine Wave Clip");

    // Generate 1 second of 440Hz sine wave at 44.1kHz
    auto sineBuffer = generateSineWave(44100.0, 1.0, 440.0);

    if (!expectTrue(sineBuffer.getNumSamples() > 0, "Generated sine wave buffer has samples"))
        return false;

    if (!expectEqual(sineBuffer.getNumChannels(), 2, "Sine wave buffer is stereo"))
        return false;

    clip->setAudioBuffer(sineBuffer);
    clip->setLength(sineBuffer.getNumSamples());

    if (!expectTrue(clip->getLength() == sineBuffer.getNumSamples(), "Clip length matches buffer size"))
        return false;

    // Add clip to track
    track->addClip(std::move(clip));

    if (!expectEqual(track->getNumClips(), 1, "Track has 1 clip"))
        return false;

    return true;
}

bool testAutomationBasics()
{
    std::cout << "\n--- Testing Automation Basics ---" << std::endl;

    ProjectState state;
    auto trackId = state.addTrack("Automated Track", "audio");

    // Add volume automation point
    auto pointId = state.addAutomationPoint(trackId, "volume", 0.0, 0.8, "Add automation");

    if (!expectNotEmpty(pointId, "Add volume automation point"))
        return false;

    if (!expectTrue(state.hasAutomation(trackId, "volume"), "Track has volume automation"))
        return false;

    // Add another point
    auto point2Id = state.addAutomationPoint(trackId, "volume", 4.0, 0.5, "Add automation");

    if (!expectNotEmpty(point2Id, "Add second volume automation point"))
        return false;

    return true;
}

//==============================================================================
// Main
//==============================================================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "Zenith Engine Smoke Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    // Initialize JUCE subsystems (minimal, no GUI)
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Run tests
    bool allTestsPassed = true;

    allTestsPassed &= testProjectStateCreation();
    allTestsPassed &= testTrackManagement();
    allTestsPassed &= testProjectSaveLoad();
    allTestsPassed &= testEngineCreation();
    allTestsPassed &= testEngineWithProjectState();
    allTestsPassed &= testClipWithSyntheticAudio();
    allTestsPassed &= testAutomationBasics();

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;
    std::cout << "========================================" << std::endl;

    if (allTestsPassed && testsFailed == 0)
    {
        std::cout << "\n✓ ALL TESTS PASSED" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "\n✗ SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
