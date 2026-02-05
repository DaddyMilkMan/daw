/*
  ==============================================================================
    AICommon.h
    Common structures for AI system
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <functional>

namespace zenith {
namespace ai {

// Training dataset structure
struct TrainingSample {
    std::vector<float> features;
    std::vector<float> targets;
    juce::String label;
    float weight = 1.0f;
    juce::String metadata;
    juce::Time timestamp;
};

// Training configuration
struct TrainingConfig {
    int epochs = 100;
    float learningRate = 0.01f;
    float batchSize = 32.0f;
    float validationSplit = 0.2f;
    float testSplit = 0.1f;
    bool earlyStopping = true;
    int patience = 10;
    float minDelta = 0.001f;
    bool shuffleData = true;
    int randomSeed = 42;
    
    // Regularization
    float l2Regularization = 0.0f;
    float dropoutRate = 0.0f;
    
    // Optimization
    juce::String optimizer = "adam";  // "sgd", "adam", "rmsprop"
    float beta1 = 0.9f;
    float beta2 = 0.999f;
    float epsilon = 1e-8f;
};

// Training progress callback
using TrainingProgressCallback = std::function<void(int epoch, float loss, float accuracy, float validationLoss)>;

// Model validation metrics
struct ValidationMetrics {
    float accuracy = 0.0f;
    float precision = 0.0f;
    float recall = 0.0f;
    float f1Score = 0.0f;
    float loss = 0.0f;
    int totalSamples = 0;
    int correctPredictions = 0;
    
    float getConfidence() const {
        return (precision + recall) / 2.0f;
    }
};

// Model training results
struct TrainingResults {
    bool success = false;
    float finalLoss = 0.0f;
    float finalAccuracy = 0.0f;
    float bestValidationLoss = 0.0f;
    int bestEpoch = 0;
    int totalEpochs = 0;
    float trainingTime = 0.0f;  // seconds
    
    ValidationMetrics validationMetrics;
    ValidationMetrics testMetrics;
    
    std::vector<float> lossHistory;
    std::vector<float> accuracyHistory;
    std::vector<float> validationLossHistory;
    std::vector<float> validationAccuracyHistory;
    
    juce::String error;
    juce::String warnings;
};

} // namespace ai
} // namespace zenith
