/*
  ==============================================================================

    InstrumentValidationTests.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Comprehensive validation tests for the instrument stack:
    - Preset parameter validation (param IDs, value ranges)
    - Audio render testing (crash detection)
    - Instrument metadata validation
    - RT safety checks

  ==============================================================================
*/

#include <iostream>
#include <vector>
#include <string>
#include <chrono>

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../Source/instruments/InstrumentRegistry.h"
#include "../Source/instruments/InstrumentPreset.h"
#include "../Source/instruments/RegisterBuiltInInstruments.h"

using namespace zenith;

//==============================================================================
// Test Results Tracking
//==============================================================================

struct ValidationFailure
{
    std::string instrumentId;
    std::string presetName;
    std::string failureType;
    std::string description;
};

std::vector<ValidationFailure> g_failures;

void reportFailure(const std::string& instrumentId,
                   const std::string& presetName,
                   const std::string& failureType,
                   const std::string& description)
{
    ValidationFailure failure;
    failure.instrumentId = instrumentId;
    failure.presetName = presetName;
    failure.failureType = failureType;
    failure.description = description;
    g_failures.push_back(failure);

    std::cerr << "[FAIL] " << instrumentId << " / " << presetName
              << " - " << failureType << ": " << description << std::endl;
}

//==============================================================================
// Test 1: Validate Instrument Metadata
//==============================================================================

bool validateInstrumentMetadata(const juce::String& instrumentId,
                                const InstrumentMetadata* metadata)
{
    if (metadata == nullptr)
    {
        reportFailure(instrumentId.toStdString(), "N/A", "Metadata",
                     "Metadata is null");
        return false;
    }

    bool valid = true;

    // Check basic metadata fields
    if (metadata->name.isEmpty())
    {
        reportFailure(instrumentId.toStdString(), "N/A", "Metadata",
                     "Instrument name is empty");
        valid = false;
    }

    if (metadata->instrumentId.isEmpty())
    {
        reportFailure(instrumentId.toStdString(), "N/A", "Metadata",
                     "Instrument ID is empty");
        valid = false;
    }

    if (metadata->parameters.empty())
    {
        reportFailure(instrumentId.toStdString(), "N/A", "Metadata",
                     "No parameters defined");
        valid = false;
    }

    // Validate each parameter
    for (const auto& param : metadata->parameters)
    {
        if (param.id.isEmpty())
        {
            reportFailure(instrumentId.toStdString(), "N/A", "Metadata",
                         "Parameter has empty ID");
            valid = false;
        }

        if (param.name.isEmpty())
        {
            reportFailure(instrumentId.toStdString(), "N/A", "Metadata",
                         "Parameter '" + param.id.toStdString() + "' has empty name");
            valid = false;
        }

        // Check that minValue <= defaultValue <= maxValue
        if (param.defaultValue < param.minValue || param.defaultValue > param.maxValue)
        {
            reportFailure(instrumentId.toStdString(), "N/A", "Metadata",
                         "Parameter '" + param.id.toStdString() +
                         "' default value out of range [" +
                         std::to_string(param.minValue) + ", " +
                         std::to_string(param.maxValue) + "]");
            valid = false;
        }
    }

    return valid;
}

//==============================================================================
// Test 2: Validate Preset Parameters
//==============================================================================

bool validatePresetParameters(const ZenithInstrumentPreset& preset,
                              const InstrumentMetadata* metadata)
{
    bool valid = true;

    // Build a set of valid parameter IDs from metadata
    std::set<std::string> validParamIds;
    for (const auto& param : metadata->parameters)
    {
        validParamIds.insert(param.id.toStdString());
    }

    // Build a map of parameter metadata for range checking
    std::map<std::string, ParameterMetadata> paramMetadata;
    for (const auto& param : metadata->parameters)
    {
        paramMetadata[param.id.toStdString()] = param;
    }

    // Check all preset parameters exist in metadata
    for (const auto& [paramId, value] : preset.parameters)
    {
        if (validParamIds.find(paramId) == validParamIds.end())
        {
            reportFailure(preset.instrumentId, preset.name, "Parameter ID",
                         "Unknown parameter ID: " + paramId);
            valid = false;
            continue;
        }

        // Check value is within range
        const auto& meta = paramMetadata[paramId];
        if (value < meta.minValue || value > meta.maxValue)
        {
            reportFailure(preset.instrumentId, preset.name, "Parameter Range",
                         "Parameter '" + paramId + "' value " +
                         std::to_string(value) + " out of range [" +
                         std::to_string(meta.minValue) + ", " +
                         std::to_string(meta.maxValue) + "]");
            valid = false;
        }
    }

    // Check macro values are in [0, 1] range
    for (const auto& [macroId, value] : preset.macros)
    {
        if (value < 0.0f || value > 1.0f)
        {
            reportFailure(preset.instrumentId, preset.name, "Macro Range",
                         "Macro '" + macroId + "' value " +
                         std::to_string(value) + " out of range [0, 1]");
            valid = false;
        }
    }

    return valid;
}

//==============================================================================
// Test 3: Test Audio Rendering
//==============================================================================

bool testAudioRendering(Instrument* instrument,
                        const juce::String& instrumentId,
                        const std::string& presetName)
{
    auto* processor = instrument->getProcessor();
    if (processor == nullptr)
    {
        reportFailure(instrumentId.toStdString(), presetName, "Audio Rendering",
                     "Processor is null");
        return false;
    }

    bool valid = true;

    try
    {
        // Prepare to play
        const double sampleRate = 44100.0;
        const int samplesPerBlock = 512;
        processor->prepareToPlay(sampleRate, samplesPerBlock);

        // Create audio and MIDI buffers
        juce::AudioBuffer<float> buffer(2, samplesPerBlock);
        juce::MidiBuffer midiBuffer;

        // Add a test MIDI note
        midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
        midiBuffer.addEvent(juce::MidiMessage::noteOff(1, 60), samplesPerBlock - 1);

        // Time the render
        auto startTime = std::chrono::high_resolution_clock::now();

        // Process the block (this should not crash)
        processor->processBlock(buffer, midiBuffer);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count();

        // Check for audio anomalies
        bool hasNaN = false;
        bool hasInf = false;
        float maxSample = 0.0f;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* samples = buffer.getReadPointer(ch);
            for (int i = 0; i < samplesPerBlock; ++i)
            {
                float sample = samples[i];
                if (std::isnan(sample))
                    hasNaN = true;
                if (std::isinf(sample))
                    hasInf = true;
                maxSample = std::max(maxSample, std::abs(sample));
            }
        }

        if (hasNaN)
        {
            reportFailure(instrumentId.toStdString(), presetName, "Audio Anomaly",
                         "Output contains NaN values");
            valid = false;
        }

        if (hasInf)
        {
            reportFailure(instrumentId.toStdString(), presetName, "Audio Anomaly",
                         "Output contains Inf values");
            valid = false;
        }

        if (maxSample > 10.0f)
        {
            reportFailure(instrumentId.toStdString(), presetName, "Audio Anomaly",
                         "Output contains extremely high values (max: " +
                         std::to_string(maxSample) + ")");
            valid = false;
        }

        // Warn if render time is excessive (> 5ms for 512 samples @ 44.1kHz)
        if (duration > 5000)
        {
            std::cout << "[WARN] " << instrumentId << " / " << presetName
                     << " - Slow render: " << duration << " us" << std::endl;
        }

        // Release resources
        processor->releaseResources();
    }
    catch (const std::exception& e)
    {
        reportFailure(instrumentId.toStdString(), presetName, "Audio Rendering",
                     "Exception thrown: " + std::string(e.what()));
        valid = false;
    }
    catch (...)
    {
        reportFailure(instrumentId.toStdString(), presetName, "Audio Rendering",
                     "Unknown exception thrown");
        valid = false;
    }

    return valid;
}

//==============================================================================
// Test 4: Load and Validate All Presets
//==============================================================================

bool testInstrumentPresets(const juce::String& instrumentId,
                           const InstrumentMetadata* metadata)
{
    bool allValid = true;

    ZenithPresetManager presetManager;
    auto presets = presetManager.getPresetsForInstrument(instrumentId.toStdString());

    if (presets.empty())
    {
        std::cout << "[INFO] " << instrumentId << " has no factory presets" << std::endl;

        // Test with default state anyway
        auto instrument = InstrumentRegistry::getInstance().createInstrument(instrumentId);
        if (instrument != nullptr)
        {
            allValid &= testAudioRendering(instrument.get(), instrumentId, "Default");
        }
        return allValid;
    }

    std::cout << "[INFO] Testing " << presets.size() << " presets for "
              << instrumentId << std::endl;

    for (const auto& preset : presets)
    {
        // Validate preset parameters
        bool presetValid = validatePresetParameters(preset, metadata);
        allValid &= presetValid;

        // Create instrument instance
        auto instrument = InstrumentRegistry::getInstance().createInstrument(instrumentId);
        if (instrument == nullptr)
        {
            reportFailure(instrumentId.toStdString(), preset.name, "Instrument Creation",
                         "Failed to create instrument instance");
            allValid = false;
            continue;
        }

        // Apply preset
        try
        {
            for (const auto& [paramId, value] : preset.parameters)
            {
                instrument->setParameterValue(paramId, value);
            }
        }
        catch (const std::exception& e)
        {
            reportFailure(instrumentId.toStdString(), preset.name, "Preset Application",
                         "Exception applying preset: " + std::string(e.what()));
            allValid = false;
            continue;
        }

        // Test audio rendering with this preset
        allValid &= testAudioRendering(instrument.get(), instrumentId, preset.name);
    }

    return allValid;
}

//==============================================================================
// Test 5: RT Safety Analysis
//==============================================================================

void performRTSafetyAnalysis()
{
    std::cout << "\n=== RT Safety Analysis ===" << std::endl;
    std::cout << "Manual code review required for:" << std::endl;
    std::cout << "  - Memory allocation in processBlock (new, malloc, vector.push_back)" << std::endl;
    std::cout << "  - Mutex locks in processBlock" << std::endl;
    std::cout << "  - File I/O or logging in processBlock" << std::endl;
    std::cout << "  - System calls in processBlock" << std::endl;
    std::cout << "\nSource files to review:" << std::endl;
    std::cout << "  - zenith-core/Source/instruments/ZenithPolySynth.cpp:52-58" << std::endl;
    std::cout << "  - zenith-core/Source/instruments/ZenithPolySynth.cpp:90-131 (renderNextBlock)" << std::endl;
    std::cout << "  - zenith-core/Source/instruments/ZenithSampler.cpp (processBlock)" << std::endl;
    std::cout << "  - zenith-core/src/Engine.cpp:590-712 (renderBlock)" << std::endl;
    std::cout << "\nCommon RT violations to check:" << std::endl;
    std::cout << "  ✓ Check: No 'new' or 'delete' calls" << std::endl;
    std::cout << "  ✓ Check: No malloc/free calls" << std::endl;
    std::cout << "  ✓ Check: No std::vector::push_back (may allocate)" << std::endl;
    std::cout << "  ✓ Check: No std::mutex locks (use lock-free structures)" << std::endl;
    std::cout << "  ✓ Check: No std::cout/cerr/logging" << std::endl;
    std::cout << "  ✓ Check: No file operations" << std::endl;
    std::cout << "  ✓ Check: Use std::atomic for shared state" << std::endl;
    std::cout << "  ✓ Check: Use pre-allocated buffers only" << std::endl;
}

//==============================================================================
// Main Test Runner
//==============================================================================

int main()
{
    std::cout << "=== Zenith DAW Instrument Stack Validation Tests ===" << std::endl;
    std::cout << std::endl;

    // Initialize JUCE
    juce::MessageManager::getInstance();

    // Register built-in instruments
    RegisterBuiltInInstruments();

    auto& registry = InstrumentRegistry::getInstance();
    auto instrumentIds = registry.getInstrumentIds();

    std::cout << "Found " << instrumentIds.size() << " instruments:" << std::endl;
    for (const auto& id : instrumentIds)
    {
        std::cout << "  - " << id << std::endl;
    }
    std::cout << std::endl;

    bool allTestsPassed = true;

    // Test each instrument
    for (const auto& instrumentId : instrumentIds)
    {
        std::cout << "=== Testing Instrument: " << instrumentId << " ===" << std::endl;

        // Get metadata
        const auto* metadata = registry.getMetadata(instrumentId);

        // Test 1: Validate metadata
        bool metadataValid = validateInstrumentMetadata(instrumentId, metadata);
        allTestsPassed &= metadataValid;

        if (!metadataValid)
        {
            std::cout << "[SKIP] Skipping preset tests due to invalid metadata" << std::endl;
            continue;
        }

        // Test 2-4: Validate and test presets
        bool presetsValid = testInstrumentPresets(instrumentId, metadata);
        allTestsPassed &= presetsValid;

        std::cout << std::endl;
    }

    // Test 5: RT Safety Analysis
    performRTSafetyAnalysis();

    // Print summary
    std::cout << "\n=== Test Summary ===" << std::endl;

    if (g_failures.empty())
    {
        std::cout << "✓ All validation tests passed!" << std::endl;
        std::cout << "✓ " << instrumentIds.size() << " instruments validated" << std::endl;
    }
    else
    {
        std::cout << "✗ " << g_failures.size() << " validation failures detected:" << std::endl;
        std::cout << std::endl;

        // Group failures by type
        std::map<std::string, std::vector<ValidationFailure>> failuresByType;
        for (const auto& failure : g_failures)
        {
            failuresByType[failure.failureType].push_back(failure);
        }

        for (const auto& [type, failures] : failuresByType)
        {
            std::cout << type << " failures (" << failures.size() << "):" << std::endl;
            for (const auto& failure : failures)
            {
                std::cout << "  - " << failure.instrumentId << " / "
                         << failure.presetName << ": "
                         << failure.description << std::endl;
            }
            std::cout << std::endl;
        }
    }

    // Cleanup
    juce::MessageManager::deleteInstance();

    return allTestsPassed ? 0 : 1;
}
