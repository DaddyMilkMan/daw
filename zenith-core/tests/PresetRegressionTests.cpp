/*
  ==============================================================================

    PresetRegressionTests.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Comprehensive regression tests for presets and CommandAPI:
    - Preset loading for ZenithPolySynth and ZenithSampler
    - Audio rendering tests (non-zero output, no NaN/Inf)
    - CommandAPI integration tests
    - Golden-value tests for preset regression detection

  ==============================================================================
*/

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <chrono>
#include <algorithm>

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include "../Source/instruments/InstrumentRegistry.h"
#include "../Source/instruments/InstrumentPreset.h"
#include "../Source/instruments/RegisterBuiltInInstruments.h"
#include "../include/Engine.h"
#include "../include/CommandAPI.h"

using namespace zenith;

//==============================================================================
// Test Results Tracking
//==============================================================================

struct TestFailure
{
    std::string testName;
    std::string category;
    std::string description;
};

std::vector<TestFailure> g_failures;
int g_totalTests = 0;
int g_passedTests = 0;

void reportFailure(const std::string& testName,
                   const std::string& category,
                   const std::string& description)
{
    TestFailure failure;
    failure.testName = testName;
    failure.category = category;
    failure.description = description;
    g_failures.push_back(failure);

    std::cerr << "[FAIL] " << testName << " (" << category << "): "
              << description << std::endl;
}

void recordTestResult(const std::string& testName, bool passed)
{
    g_totalTests++;
    if (passed)
    {
        g_passedTests++;
        std::cout << "[PASS] " << testName << std::endl;
    }
}

//==============================================================================
// Audio Utility Functions
//==============================================================================

struct AudioStats
{
    float maxAmplitude = 0.0f;
    float rmsLevel = 0.0f;
    bool hasNaN = false;
    bool hasInf = false;
    bool hasNonZero = false;
};

AudioStats analyzeAudioBuffer(const juce::AudioBuffer<float>& buffer)
{
    AudioStats stats;
    double sumSquares = 0.0;
    int totalSamples = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float sample = samples[i];

            if (std::isnan(sample))
                stats.hasNaN = true;
            if (std::isinf(sample))
                stats.hasInf = true;
            if (std::abs(sample) > 0.0001f)
                stats.hasNonZero = true;

            float absSample = std::abs(sample);
            stats.maxAmplitude = std::max(stats.maxAmplitude, absSample);
            sumSquares += sample * sample;
            totalSamples++;
        }
    }

    if (totalSamples > 0)
    {
        stats.rmsLevel = std::sqrt(sumSquares / totalSamples);
    }

    return stats;
}

//==============================================================================
// Test 1: Preset Loading Tests
//==============================================================================

bool testPresetLoading(const std::string& instrumentId,
                       const std::string& presetName,
                       const ZenithInstrumentPreset& preset)
{
    std::string testName = instrumentId + "::" + presetName + " [Load]";
    bool passed = true;

    // Create instrument instance
    auto instrument = InstrumentRegistry::getInstance().createInstrument(
        juce::String(instrumentId));

    if (instrument == nullptr)
    {
        reportFailure(testName, "Preset Loading",
                     "Failed to create instrument instance");
        recordTestResult(testName, false);
        return false;
    }

    // Apply preset parameters
    try
    {
        for (const auto& [paramId, value] : preset.parameters)
        {
            instrument->setParameterValue(paramId, value);
        }
    }
    catch (const std::exception& e)
    {
        reportFailure(testName, "Preset Loading",
                     "Exception applying preset: " + std::string(e.what()));
        passed = false;
    }

    recordTestResult(testName, passed);
    return passed;
}

//==============================================================================
// Test 2: Audio Rendering Tests
//==============================================================================

bool testAudioRendering(const std::string& instrumentId,
                        const std::string& presetName,
                        const ZenithInstrumentPreset& preset,
                        bool expectNonZeroOutput = true)
{
    std::string testName = instrumentId + "::" + presetName + " [Audio]";
    bool passed = true;

    // Create and configure instrument
    auto instrument = InstrumentRegistry::getInstance().createInstrument(
        juce::String(instrumentId));

    if (instrument == nullptr)
    {
        reportFailure(testName, "Audio Rendering", "Failed to create instrument");
        recordTestResult(testName, false);
        return false;
    }

    // Apply preset
    for (const auto& [paramId, value] : preset.parameters)
    {
        instrument->setParameterValue(paramId, value);
    }

    auto* processor = instrument->getProcessor();
    if (processor == nullptr)
    {
        reportFailure(testName, "Audio Rendering", "Processor is null");
        recordTestResult(testName, false);
        return false;
    }

    try
    {
        // Prepare to play
        const double sampleRate = 44100.0;
        const int samplesPerBlock = 512;
        processor->prepareToPlay(sampleRate, samplesPerBlock);

        // Create audio and MIDI buffers
        juce::AudioBuffer<float> buffer(2, samplesPerBlock);
        juce::MidiBuffer midiBuffer;

        // Add test MIDI note (C4, velocity 100, full block duration)
        midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
        midiBuffer.addEvent(juce::MidiMessage::noteOff(1, 60), samplesPerBlock - 1);

        // Clear buffer
        buffer.clear();

        // Process the block
        processor->processBlock(buffer, midiBuffer);

        // Analyze output
        AudioStats stats = analyzeAudioBuffer(buffer);

        // Check for anomalies
        if (stats.hasNaN)
        {
            reportFailure(testName, "Audio Anomaly", "Output contains NaN values");
            passed = false;
        }

        if (stats.hasInf)
        {
            reportFailure(testName, "Audio Anomaly", "Output contains Inf values");
            passed = false;
        }

        if (stats.maxAmplitude > 10.0f)
        {
            reportFailure(testName, "Audio Anomaly",
                         "Output contains extremely high values (max: " +
                         std::to_string(stats.maxAmplitude) + ")");
            passed = false;
        }

        // Check for non-zero output (expected for most presets with MIDI input)
        if (expectNonZeroOutput && !stats.hasNonZero)
        {
            reportFailure(testName, "Audio Output",
                         "Expected non-zero output but got silence (RMS: " +
                         std::to_string(stats.rmsLevel) + ")");
            passed = false;
        }

        // Release resources
        processor->releaseResources();
    }
    catch (const std::exception& e)
    {
        reportFailure(testName, "Audio Rendering",
                     "Exception thrown: " + std::string(e.what()));
        passed = false;
    }
    catch (...)
    {
        reportFailure(testName, "Audio Rendering", "Unknown exception thrown");
        passed = false;
    }

    recordTestResult(testName, passed);
    return passed;
}

//==============================================================================
// Test 3: Multi-Block Audio Rendering Test
//==============================================================================

bool testMultiBlockRendering(const std::string& instrumentId,
                             const std::string& presetName,
                             const ZenithInstrumentPreset& preset,
                             int numBlocks = 10)
{
    std::string testName = instrumentId + "::" + presetName + " [MultiBlock]";
    bool passed = true;

    auto instrument = InstrumentRegistry::getInstance().createInstrument(
        juce::String(instrumentId));

    if (instrument == nullptr)
    {
        reportFailure(testName, "Multi-Block Rendering", "Failed to create instrument");
        recordTestResult(testName, false);
        return false;
    }

    // Apply preset
    for (const auto& [paramId, value] : preset.parameters)
    {
        instrument->setParameterValue(paramId, value);
    }

    auto* processor = instrument->getProcessor();
    if (processor == nullptr)
    {
        reportFailure(testName, "Multi-Block Rendering", "Processor is null");
        recordTestResult(testName, false);
        return false;
    }

    try
    {
        const double sampleRate = 44100.0;
        const int samplesPerBlock = 512;
        processor->prepareToPlay(sampleRate, samplesPerBlock);

        // Render multiple blocks
        for (int block = 0; block < numBlocks; ++block)
        {
            juce::AudioBuffer<float> buffer(2, samplesPerBlock);
            juce::MidiBuffer midiBuffer;

            // Add MIDI note on first block
            if (block == 0)
            {
                midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            }
            // Add note off on last block
            else if (block == numBlocks - 1)
            {
                midiBuffer.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
            }

            buffer.clear();
            processor->processBlock(buffer, midiBuffer);

            // Check for anomalies
            AudioStats stats = analyzeAudioBuffer(buffer);

            if (stats.hasNaN)
            {
                reportFailure(testName, "Multi-Block Anomaly",
                             "NaN in block " + std::to_string(block));
                passed = false;
                break;
            }

            if (stats.hasInf)
            {
                reportFailure(testName, "Multi-Block Anomaly",
                             "Inf in block " + std::to_string(block));
                passed = false;
                break;
            }
        }

        processor->releaseResources();
    }
    catch (const std::exception& e)
    {
        reportFailure(testName, "Multi-Block Rendering",
                     "Exception: " + std::string(e.what()));
        passed = false;
    }

    recordTestResult(testName, passed);
    return passed;
}

//==============================================================================
// Test 4: Golden-Value Regression Test
//==============================================================================

bool testGoldenValuePreset()
{
    std::string testName = "Golden-Value [Supersaw Classic]";
    bool passed = true;

    // Expected parameter values for "Supersaw Classic" preset
    std::map<std::string, float> expectedParams = {
        {"osc_type", 0.5f},
        {"filter_cutoff", 0.9f},
        {"filter_resonance", 0.3f},
        {"attack", 0.1f},
        {"decay", 0.2f},
        {"sustain", 0.8f},
        {"release", 0.35f}
    };

    // Load the preset
    ZenithPresetManager presetManager;
    auto presets = presetManager.getPresetsForInstrument("zenith_poly_synth");

    const ZenithInstrumentPreset* goldenPreset = nullptr;
    for (const auto& preset : presets)
    {
        if (preset.name == "Supersaw Classic")
        {
            goldenPreset = &preset;
            break;
        }
    }

    if (goldenPreset == nullptr)
    {
        reportFailure(testName, "Golden-Value",
                     "Preset 'Supersaw Classic' not found");
        recordTestResult(testName, false);
        return false;
    }

    // Verify each parameter matches expected value
    const float tolerance = 0.0001f;
    for (const auto& [paramId, expectedValue] : expectedParams)
    {
        auto it = goldenPreset->parameters.find(paramId);
        if (it == goldenPreset->parameters.end())
        {
            reportFailure(testName, "Golden-Value",
                         "Parameter '" + paramId + "' missing from preset");
            passed = false;
            continue;
        }

        float actualValue = it->second;
        float diff = std::abs(actualValue - expectedValue);

        if (diff > tolerance)
        {
            reportFailure(testName, "Golden-Value",
                         "Parameter '" + paramId + "' value mismatch: expected " +
                         std::to_string(expectedValue) + ", got " +
                         std::to_string(actualValue) + " (diff: " +
                         std::to_string(diff) + ")");
            passed = false;
        }
    }

    recordTestResult(testName, passed);
    return passed;
}

//==============================================================================
// Test 5: CommandAPI Integration Tests
//==============================================================================

class TestMessageListener : public juce::MessageListener
{
public:
    void handleMessage(const juce::Message&) override {}
};

bool testCommandAPI_ListInstruments()
{
    std::string testName = "CommandAPI::listInstruments";
    bool passed = true;

    try
    {
        TestMessageListener listener;
        Engine engine(&listener, 44100.0, 512);
        CommandAPI commandAPI(engine);

        juce::var command = juce::JSON::parse(R"({
            "command": "listInstruments"
        })");

        juce::var result = commandAPI.executeCommand(command);

        if (!result.hasProperty("success") || !result["success"])
        {
            reportFailure(testName, "CommandAPI",
                         "Command failed: " + result.toString().toStdString());
            passed = false;
        }
        else if (!result.hasProperty("instruments"))
        {
            reportFailure(testName, "CommandAPI",
                         "Result missing 'instruments' property");
            passed = false;
        }
        else
        {
            auto instruments = result["instruments"];
            if (!instruments.isArray() || instruments.size() == 0)
            {
                reportFailure(testName, "CommandAPI",
                             "Instruments array is empty");
                passed = false;
            }
        }
    }
    catch (const std::exception& e)
    {
        reportFailure(testName, "CommandAPI",
                     "Exception: " + std::string(e.what()));
        passed = false;
    }

    recordTestResult(testName, passed);
    return passed;
}

bool testCommandAPI_ListPresets()
{
    std::string testName = "CommandAPI::listPresets";
    bool passed = true;

    try
    {
        TestMessageListener listener;
        Engine engine(&listener, 44100.0, 512);
        CommandAPI commandAPI(engine);

        juce::var command = juce::JSON::parse(R"({
            "command": "listPresets",
            "instrumentId": "zenith_poly_synth"
        })");

        juce::var result = commandAPI.executeCommand(command);

        if (!result.hasProperty("success") || !result["success"])
        {
            reportFailure(testName, "CommandAPI",
                         "Command failed: " + result.toString().toStdString());
            passed = false;
        }
        else if (!result.hasProperty("presets"))
        {
            reportFailure(testName, "CommandAPI",
                         "Result missing 'presets' property");
            passed = false;
        }
    }
    catch (const std::exception& e)
    {
        reportFailure(testName, "CommandAPI",
                     "Exception: " + std::string(e.what()));
        passed = false;
    }

    recordTestResult(testName, passed);
    return passed;
}

bool testCommandAPI_CreateTrackAndAssignInstrument()
{
    std::string testName = "CommandAPI::createTrack+assignInstrument";
    bool passed = true;

    try
    {
        TestMessageListener listener;
        Engine engine(&listener, 44100.0, 512);
        CommandAPI commandAPI(engine);

        // Create instrument track
        juce::var createCommand = juce::JSON::parse(R"({
            "command": "createTrack",
            "type": "instrument",
            "name": "Test Synth Track"
        })");

        juce::var createResult = commandAPI.executeCommand(createCommand);

        if (!createResult.hasProperty("success") || !createResult["success"])
        {
            reportFailure(testName, "CommandAPI",
                         "Failed to create track: " + createResult.toString().toStdString());
            recordTestResult(testName, false);
            return false;
        }

        int trackId = createResult["trackId"];

        // Assign instrument to track
        juce::var assignCommand = juce::JSON::parse(juce::String::formatted(R"({
            "command": "setTrackInstrument",
            "trackId": %d,
            "instrumentId": "zenith_poly_synth"
        })", trackId));

        juce::var assignResult = commandAPI.executeCommand(assignCommand);

        if (!assignResult.hasProperty("success") || !assignResult["success"])
        {
            reportFailure(testName, "CommandAPI",
                         "Failed to assign instrument: " + assignResult.toString().toStdString());
            passed = false;
        }
    }
    catch (const std::exception& e)
    {
        reportFailure(testName, "CommandAPI",
                     "Exception: " + std::string(e.what()));
        passed = false;
    }

    recordTestResult(testName, passed);
    return passed;
}

bool testCommandAPI_LoadPreset()
{
    std::string testName = "CommandAPI::loadPreset";
    bool passed = true;

    try
    {
        TestMessageListener listener;
        Engine engine(&listener, 44100.0, 512);
        CommandAPI commandAPI(engine);

        // Create track
        juce::var createCommand = juce::JSON::parse(R"({
            "command": "createTrack",
            "type": "instrument"
        })");
        juce::var createResult = commandAPI.executeCommand(createCommand);

        if (!createResult["success"])
        {
            reportFailure(testName, "CommandAPI", "Failed to create track");
            recordTestResult(testName, false);
            return false;
        }

        int trackId = createResult["trackId"];

        // Assign instrument
        juce::var assignCommand = juce::JSON::parse(juce::String::formatted(R"({
            "command": "setTrackInstrument",
            "trackId": %d,
            "instrumentId": "zenith_poly_synth"
        })", trackId));
        commandAPI.executeCommand(assignCommand);

        // Load preset
        juce::var loadCommand = juce::JSON::parse(juce::String::formatted(R"({
            "command": "loadPreset",
            "trackId": %d,
            "presetName": "Supersaw Classic"
        })", trackId));

        juce::var loadResult = commandAPI.executeCommand(loadCommand);

        if (!loadResult.hasProperty("success") || !loadResult["success"])
        {
            reportFailure(testName, "CommandAPI",
                         "Failed to load preset: " + loadResult.toString().toStdString());
            passed = false;
        }
    }
    catch (const std::exception& e)
    {
        reportFailure(testName, "CommandAPI",
                     "Exception: " + std::string(e.what()));
        passed = false;
    }

    recordTestResult(testName, passed);
    return passed;
}

bool testCommandAPI_SetParameter()
{
    std::string testName = "CommandAPI::setInstrumentParameter";
    bool passed = true;

    try
    {
        TestMessageListener listener;
        Engine engine(&listener, 44100.0, 512);
        CommandAPI commandAPI(engine);

        // Create track with instrument
        juce::var createCommand = juce::JSON::parse(R"({
            "command": "createTrack",
            "type": "instrument"
        })");
        juce::var createResult = commandAPI.executeCommand(createCommand);
        int trackId = createResult["trackId"];

        juce::var assignCommand = juce::JSON::parse(juce::String::formatted(R"({
            "command": "setTrackInstrument",
            "trackId": %d,
            "instrumentId": "zenith_poly_synth"
        })", trackId));
        commandAPI.executeCommand(assignCommand);

        // Set parameter
        juce::var setParamCommand = juce::JSON::parse(juce::String::formatted(R"({
            "command": "setInstrumentParameter",
            "trackId": %d,
            "parameterId": "filter_cutoff",
            "value": 0.75
        })", trackId));

        juce::var setResult = commandAPI.executeCommand(setParamCommand);

        if (!setResult.hasProperty("success") || !setResult["success"])
        {
            reportFailure(testName, "CommandAPI",
                         "Failed to set parameter: " + setResult.toString().toStdString());
            passed = false;
        }
    }
    catch (const std::exception& e)
    {
        reportFailure(testName, "CommandAPI",
                     "Exception: " + std::string(e.what()));
        passed = false;
    }

    recordTestResult(testName, passed);
    return passed;
}

//==============================================================================
// Test Runner
//==============================================================================

void runPresetTests(const std::string& instrumentId, int maxPresetsToTest = 5)
{
    ZenithPresetManager presetManager;
    auto presets = presetManager.getPresetsForInstrument(instrumentId);

    if (presets.empty())
    {
        std::cout << "[INFO] No presets found for " << instrumentId << std::endl;
        return;
    }

    std::cout << "[INFO] Testing " << std::min((int)presets.size(), maxPresetsToTest)
              << " presets for " << instrumentId << std::endl;

    int count = 0;
    for (const auto& preset : presets)
    {
        if (count >= maxPresetsToTest)
            break;

        testPresetLoading(instrumentId, preset.name, preset);
        testAudioRendering(instrumentId, preset.name, preset);
        testMultiBlockRendering(instrumentId, preset.name, preset, 5);

        count++;
    }
}

//==============================================================================
// Main
//==============================================================================

int main()
{
    std::cout << "\n=== Zenith DAW Preset & CommandAPI Regression Tests ===" << std::endl;
    std::cout << std::endl;

    // Initialize JUCE
    juce::MessageManager::getInstance();

    // Register built-in instruments
    RegisterBuiltInInstruments();

    std::cout << "=== Test Suite 1: Preset Loading & Audio Rendering ===" << std::endl;
    std::cout << std::endl;

    // Test ZenithPolySynth presets (sample 5 presets)
    std::cout << "--- Testing ZenithPolySynth Presets ---" << std::endl;
    runPresetTests("zenith_poly_synth", 5);
    std::cout << std::endl;

    // Test ZenithSampler presets (sample 3 presets)
    std::cout << "--- Testing ZenithSampler Presets ---" << std::endl;
    runPresetTests("zenith_sampler", 3);
    std::cout << std::endl;

    std::cout << "=== Test Suite 2: Golden-Value Regression Test ===" << std::endl;
    std::cout << std::endl;
    testGoldenValuePreset();
    std::cout << std::endl;

    std::cout << "=== Test Suite 3: CommandAPI Integration Tests ===" << std::endl;
    std::cout << std::endl;
    testCommandAPI_ListInstruments();
    testCommandAPI_ListPresets();
    testCommandAPI_CreateTrackAndAssignInstrument();
    testCommandAPI_LoadPreset();
    testCommandAPI_SetParameter();
    std::cout << std::endl;

    // Print summary
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << g_totalTests << std::endl;
    std::cout << "Passed: " << g_passedTests << std::endl;
    std::cout << "Failed: " << (g_totalTests - g_passedTests) << std::endl;
    std::cout << std::endl;

    if (g_failures.empty())
    {
        std::cout << "✓ All regression tests passed!" << std::endl;
    }
    else
    {
        std::cout << "✗ " << g_failures.size() << " test failures detected:" << std::endl;
        std::cout << std::endl;

        // Group failures by category
        std::map<std::string, std::vector<TestFailure>> failuresByCategory;
        for (const auto& failure : g_failures)
        {
            failuresByCategory[failure.category].push_back(failure);
        }

        for (const auto& [category, failures] : failuresByCategory)
        {
            std::cout << category << " failures (" << failures.size() << "):" << std::endl;
            for (const auto& failure : failures)
            {
                std::cout << "  - " << failure.testName << ": "
                         << failure.description << std::endl;
            }
            std::cout << std::endl;
        }
    }

    // Cleanup
    juce::MessageManager::deleteInstance();

    return (g_totalTests == g_passedTests) ? 0 : 1;
}
