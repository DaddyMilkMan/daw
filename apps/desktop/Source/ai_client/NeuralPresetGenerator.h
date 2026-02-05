/*
  ==============================================================================

    NeuralPresetGenerator.h
    Created: 2025-12-31
    Author:  Deep-Synth Agent

    High-level coordinator for AI preset generation.
    Connects NeuralInferenceBridge with PresetSchemaValidator.
    
    Anti-Corner-Cutting Protocol:
    1. Schema Adherence: validated via PresetSchemaValidator
    2. Thread Safety: Manages async inference request/response
    3. Error Handling: Robust error propagation

  ==============================================================================
*/

#pragma once

#include "NeuralInferenceBridge.h"
#include "PresetSchemaValidator.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <vector>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Result of a preset generation request
*/
struct GenerationResult {
    bool success = false;
    Preset preset;
    juce::String error;
    double generationTimeMs = 0.0;
};

//==============================================================================
/**
    Callback for generation completion
*/
using GenerationCallback = std::function<void(const GenerationResult&)>;

//==============================================================================
/**
    NeuralPresetGenerator
    
    Orchestrates the process of generating a synthesizer preset from a neural model.
    
    Workflow:
    1. Takes a seed vector (latent space coordinate)
    2. Sends to NeuralInferenceBridge for inference
    3. Receives raw output vector
    4. Converts to normalized parameters
    5. Validates and clamps using PresetSchemaValidator
    6. Returns final valid Preset
*/
class NeuralPresetGenerator {
public:
    //==========================================================================
    NeuralPresetGenerator(NeuralInferenceBridge& bridge);
    ~NeuralPresetGenerator() = default;

    //==========================================================================
    /**
     * @brief Generate a preset from a model and seed
     * @param modelPath Path to the ONNX model
     * @param seedVector Input latent vector (size must match model input)
     * @param onComplete Callback with result (called on message thread)
     * 
     * This is an asynchronous operation.
     */
    void generatePreset(const juce::File& modelPath, 
                        const std::vector<float>& seedVector,
                        GenerationCallback onComplete);

    /**
     * @brief Cancel any pending generation
     */
    void cancelGeneration();

private:
    //==========================================================================
    NeuralInferenceBridge& bridge_;
    PresetSchemaValidator validator_;
    
    // Track current job for cancellation
    std::atomic<int64_t> currentJobId_{0};
    
    // Internal helper to process inference output
    void processInferenceOutput(const std::vector<float>& output, 
                                const juce::File& modelPath,
                                GenerationCallback onComplete);
    
    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuralPresetGenerator)
};

} // namespace ai
} // namespace zenith
