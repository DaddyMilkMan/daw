/*
  ==============================================================================

    NeuralPresetGenerator.cpp
    Created: 2025-12-31
    Author:  Deep-Synth Agent

    Implementation of NeuralPresetGenerator.

  ==============================================================================
*/

#include "NeuralPresetGenerator.h"

namespace zenith {
namespace ai {

//==============================================================================
NeuralPresetGenerator::NeuralPresetGenerator(NeuralInferenceBridge& bridge)
    : bridge_(bridge)
{
}

void NeuralPresetGenerator::generatePreset(const juce::File& modelPath, 
                                           const std::vector<float>& seedVector,
                                           GenerationCallback onComplete)
{
    // 1. Load model if needed
    // Note: For efficiency in a real scenario, we might keep the model loaded.
    // For this implementation, we ensure the correct model is loaded.
    
    // Helper to run inference once model is ready
    auto runInference = [this, seedVector, modelPath, onComplete]() {
        currentJobId_ = bridge_.requestInference(
            seedVector,
            // On Success
            [this, modelPath, onComplete](std::vector<float> output) {
                processInferenceOutput(output, modelPath, onComplete);
                currentJobId_ = 0;
            },
            // On Error
            [onComplete](InferenceError error, const juce::String& msg) {
                GenerationResult result;
                result.success = false;
                result.error = "Inference failed: " + msg;
                onComplete(result);
            }
        );
    };

    // Check if correct model is already loaded
    ModelInfo activeModel = bridge_.getModelInfo();
    if (activeModel.path == modelPath.getFullPathName() && bridge_.isModelLoaded()) {
        runInference();
    } else {
        // Load the model first
        bridge_.loadModel(
            modelPath, 
            // On Error
            [onComplete](InferenceError error, const juce::String& msg) {
                GenerationResult result;
                result.success = false;
                result.error = "Model load failed: " + msg;
                onComplete(result);
            },
            // On Progress
            [runInference](float progress, const juce::String& status) {
                 if (progress >= 1.0f) {
                     runInference();
                 }
            }
        );
    }
}

void NeuralPresetGenerator::cancelGeneration()
{
    int64_t jobId = currentJobId_.load();
    if (jobId != 0) {
        bridge_.cancelInference(jobId);
        currentJobId_ = 0;
    }
}

void NeuralPresetGenerator::processInferenceOutput(const std::vector<float>& output, 
                                                   const juce::File& modelPath,
                                                   GenerationCallback onComplete)
{
    GenerationResult result;
    auto startTime = juce::Time::getMillisecondCounterHiRes();
    
    try {
        // 1. Convert output to preset using Schema Validator
        // The validator maps the flat vector to the named parameter schema
        juce::String presetName = "AI_" + modelPath.getFileNameWithoutExtension() + "_" + 
                                  juce::String::toHexString(juce::Random::getSystemRandom().nextInt());
        
        Preset rawPreset = validator_.createPresetFromNormalized(output, presetName);
        
        // 2. Validate and Clamp
        // Force strict adherence to schema ranges
        Preset validPreset = validator_.clampToSchema(rawPreset);
        
        // 3. Final Verification
        PresetValidationResult validation = validator_.validate(validPreset);
        
        if (validation.isValid) {
            result.success = true;
            result.preset = validPreset;
            result.preset.author = "Deep-Synth AI";
            result.preset.category = "AI_Generated";
            
            // Add metadata tags
            result.preset.tags.push_back("model:" + modelPath.getFileName());
            result.preset.tags.push_back("validity:checked");
            
        } else {
            result.success = false;
            // Should be rare since we clamped, but possible if schema logic fails
            result.error = "Generated preset failed validation: " + 
                           (validation.errors.isEmpty() ? "Unknown error" : validation.errors[0]);
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error = "Processing error: " + juce::String(e.what());
    }
    
    result.generationTimeMs = juce::Time::getMillisecondCounterHiRes() - startTime;
    onComplete(result);
}

} // namespace ai
} // namespace zenith
